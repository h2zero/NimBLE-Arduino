#ifndef ESP_PLATFORM

/*
 * Licensed to the Apache Software Foundation (ASF) under one
 * or more contributor license agreements.  See the NOTICE file
 * distributed with this work for additional information
 * regarding copyright ownership.  The ASF licenses this file
 * to you under the Apache License, Version 2.0 (the
 * "License"); you may not use this file except in compliance
 * with the License.  You may obtain a copy of the License at
 *
 *  http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing,
 * software distributed under the License is distributed on an
 * "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
 * KIND, either express or implied.  See the License for the
 * specific language governing permissions and limitations
 * under the License.
 */

#include <stdint.h>
#include <syscfg/syscfg.h>
#include "nimble/nimble/controller/include/controller/ble_ll.h"
#include "nimble/nimble/controller/include/controller/ble_ll_isoal.h"

#ifndef min
#define min(a, b) ((a) < (b) ? (a) : (b))
#endif

#if MYNEWT_VAL(BLE_LL_ISO)

void
ble_ll_isoal_mux_init(struct ble_ll_isoal_mux *mux, uint8_t max_pdu,
                      uint32_t iso_interval_us, uint32_t sdu_interval_us,
                      uint8_t bn, uint8_t pte, bool framed, uint8_t framing_mode)
{
    memset(mux, 0, sizeof(*mux));

    mux->max_pdu = max_pdu;
    /* Core 5.3, Vol 6, Part G, 2.1 */
    mux->sdu_per_interval = iso_interval_us / sdu_interval_us;

    if (framed) {
        /* TODO */
    } else {
        mux->pdu_per_sdu = bn / mux->sdu_per_interval;
    }

    mux->sdu_per_event = (1 + pte) * mux->sdu_per_interval;

    mux->bn = bn;

    STAILQ_INIT(&mux->sdu_q);
    mux->sdu_q_len = 0;

    mux->framed = framed;
    mux->framing_mode = framing_mode;
}

void
ble_ll_isoal_mux_free(struct ble_ll_isoal_mux *mux)
{
    struct os_mbuf_pkthdr *pkthdr;

    pkthdr = STAILQ_FIRST(&mux->sdu_q);
    while (pkthdr) {
        /* remove from list before freeing om */
        STAILQ_REMOVE_HEAD(&mux->sdu_q, omp_next);

        os_mbuf_free_chain(OS_MBUF_PKTHDR_TO_MBUF(pkthdr));

        pkthdr = STAILQ_FIRST(&mux->sdu_q);
    }

    STAILQ_INIT(&mux->sdu_q);
    mux->sdu_q_len = 0;
}

void
ble_ll_isoal_mux_sdu_enqueue(struct ble_ll_isoal_mux *mux, struct os_mbuf *om)
{
    struct os_mbuf_pkthdr *pkthdr;
    os_sr_t sr;

    BLE_LL_ASSERT(mux);

    OS_ENTER_CRITICAL(sr);
    pkthdr = OS_MBUF_PKTHDR(om);
    STAILQ_INSERT_TAIL(&mux->sdu_q, pkthdr, omp_next);
    mux->sdu_q_len++;
#if MYNEWT_VAL(BLE_LL_ISOAL_MUX_PREFILL)
    if (mux->sdu_q_len >= mux->sdu_per_event) {
        mux->active = 1;
    }
#endif
    OS_EXIT_CRITICAL(sr);
}

int
ble_ll_isoal_mux_event_start(struct ble_ll_isoal_mux *mux, uint32_t timestamp)
{
#if MYNEWT_VAL(BLE_LL_ISOAL_MUX_PREFILL)
    /* If prefill is enabled, we always expect to have required number of SDUs
     * in queue, otherwise we disable mux until enough SDUs are queued again.
     */
    if (mux->sdu_per_event > mux->sdu_q_len) {
        mux->active = 0;
    }
    if (mux->active && mux->framed) {
        mux->sdu_in_event = mux->sdu_q_len;
    } else if (mux->active) {
        mux->sdu_in_event = mux->sdu_per_event;
    } else {
        mux->sdu_in_event = 0;
    }
#else
    if (mux->framed) {
        mux->sdu_in_event = mux->sdu_q_len;
    } else {
        mux->sdu_in_event = min(mux->sdu_q_len, mux->sdu_per_event);
    }
#endif
    mux->event_tx_timestamp = timestamp;

    return mux->sdu_in_event;
}

static int
ble_ll_isoal_mux_unframed_event_done(struct ble_ll_isoal_mux *mux)
{
    struct os_mbuf_pkthdr *pkthdr;
    struct os_mbuf *om;
    struct os_mbuf *om_next;
    uint8_t num_sdu;
    int pkt_freed = 0;
    os_sr_t sr;

    num_sdu = min(mux->sdu_in_event, mux->sdu_per_interval);

#if MYNEWT_VAL(BLE_LL_ISO_HCI_DISCARD_THRESHOLD)
    /* Drop queued SDUs if number of queued SDUs exceeds defined threshold.
     * Threshold is defined as number of ISO events. If number of queued SDUs
     * exceeds number of SDUs required for single event (i.e. including pt)
     * and number of subsequent ISO events defined by threshold value, we'll
     * drop any excessive SDUs and notify host as if they were sent.
     */
    uint32_t thr = MYNEWT_VAL(BLE_LL_ISO_HCI_DISCARD_THRESHOLD);
    if (mux->sdu_q_len > mux->sdu_per_event + thr * mux->sdu_per_interval) {
        num_sdu = mux->sdu_q_len - mux->sdu_per_event -
                  thr * mux->sdu_per_interval;
    }
#endif

    pkthdr = STAILQ_FIRST(&mux->sdu_q);
    while (pkthdr && num_sdu--) {
        OS_ENTER_CRITICAL(sr);
        STAILQ_REMOVE_HEAD(&mux->sdu_q, omp_next);
        BLE_LL_ASSERT(mux->sdu_q_len > 0);
        mux->sdu_q_len--;
        OS_EXIT_CRITICAL(sr);

        om = OS_MBUF_PKTHDR_TO_MBUF(pkthdr);
        while (om) {
            om_next = SLIST_NEXT(om, om_next);
            os_mbuf_free(om);
            pkt_freed++;
            om = om_next;
        }

        pkthdr = STAILQ_FIRST(&mux->sdu_q);
    }

    mux->sdu_in_event = 0;

    return pkt_freed;
}

static struct os_mbuf *
ble_ll_isoal_sdu_trim(struct os_mbuf *om, int *pkt_freed)
{
    /* Count the number of packets that will be freed. */
    for (struct os_mbuf *mbuf = om; mbuf; mbuf = SLIST_NEXT(mbuf, om_next)) {
        if (mbuf->om_len == 0) {
            (*pkt_freed)++;
        }
    }

    return os_mbuf_trim_front(om);
}

static int
ble_ll_isoal_mux_framed_event_done(struct ble_ll_isoal_mux *mux)
{
    struct os_mbuf_pkthdr *pkthdr;
    struct os_mbuf_pkthdr *pkthdr_temp;
    struct os_mbuf *om;
    uint8_t num_sdu;
    uint8_t num_pdu;
    uint8_t pdu_len = 0;
    uint8_t frag_len = 0;
    uint8_t hdr_len = 0;
    int pkt_freed = 0;
    os_sr_t sr;

    num_sdu = mux->sdu_in_event;
    if (num_sdu == 0) {
        return 0;
    }

    num_pdu = mux->bn;

#if MYNEWT_VAL(BLE_LL_ISO_HCI_DISCARD_THRESHOLD)
    /* Drop queued SDUs if number of queued SDUs exceeds defined threshold.
     * Threshold is defined as number of ISO events. If number of queued SDUs
     * exceeds number of SDUs required for single event (i.e. including pt)
     * and number of subsequent ISO events defined by threshold value, we'll
     * drop any excessive SDUs and notify host as if they were sent.
     */
    uint32_t thr = MYNEWT_VAL(BLE_LL_ISO_HCI_DISCARD_THRESHOLD);
    if (mux->sdu_q_len > mux->sdu_per_event + thr * mux->sdu_per_interval) {
        num_sdu = mux->sdu_q_len - mux->sdu_per_event -
                  thr * mux->sdu_per_interval;
    }
#endif

    /* Iterate over all queued SDUs. */
    STAILQ_FOREACH_SAFE(pkthdr, &mux->sdu_q, omp_next, pkthdr_temp) {
        if (num_sdu == 0) {
            break;
        }

        BLE_LL_ASSERT(mux->sdu_q_len > 0);

        /* Remove the SDU from the queue, as we are about to free some buffers from a chain.
         * Otherwise, it will result in invalid head pointed by sdu_q entry.
         */
        OS_ENTER_CRITICAL(sr);
        STAILQ_REMOVE_HEAD(&mux->sdu_q, omp_next);
        mux->sdu_q_len--;
        OS_EXIT_CRITICAL(sr);

        om = OS_MBUF_PKTHDR_TO_MBUF(pkthdr);

        /* Iterate over all buffers in the SDU chain. */
        while (num_pdu > 0 && OS_MBUF_PKTLEN(om) > 0) {
            hdr_len = mux->sc ? 2 /* Segmentation Header */
                              : 5 /* Segmentation Header + TimeOffset */;

            /* If the next SDU fragment header size exceeds remaining space in the PDU, advance to next PDU. */
            if (mux->max_pdu <= hdr_len + pdu_len) {
                pdu_len = 0;
                num_pdu--;
                continue;
            }

            /* "Put" the header in the PDU. */
            pdu_len += hdr_len;

            /* SDU fragment length that can be fit in the PDU. */
            frag_len = min(OS_MBUF_PKTLEN(om), mux->max_pdu - pdu_len);

            /* Increase PDU length by the length of the SDU fragment. */
            pdu_len += frag_len;

            os_mbuf_adj(om, frag_len);

            /* If the SDU fragment does not fit in the PDU, set the SC flag. */
            if (OS_MBUF_PKTLEN(om) > frag_len) {
                mux->sc = 1;
            } else {
                mux->sc = 0;
            }
        }

        om = ble_ll_isoal_sdu_trim(om, &pkt_freed);
        if (OS_MBUF_PKTLEN(om) > 0) {
            /* If there is still data in the SDU chain, place the SDU back in the queue. */
            OS_ENTER_CRITICAL(sr);
            STAILQ_INSERT_HEAD(&mux->sdu_q, pkthdr, omp_next);
            mux->sdu_q_len++;
            OS_EXIT_CRITICAL(sr);
        } else {
            /* If there are no more data in the SDU chain, free the SDU chain. */
            os_mbuf_free_chain(om);
            num_sdu--;
        }

        if (num_pdu == 0) {
            break;
        }
    }

    mux->sdu_in_event = 0;

    return pkt_freed;
}

int
ble_ll_isoal_mux_event_done(struct ble_ll_isoal_mux *mux)
{
    struct os_mbuf_pkthdr *pkthdr;
    struct ble_mbuf_hdr *blehdr;
    struct os_mbuf *om;

    pkthdr = STAILQ_FIRST(&mux->sdu_q);
    if (pkthdr) {
        om = OS_MBUF_PKTHDR_TO_MBUF(pkthdr);
        blehdr = BLE_MBUF_HDR_PTR(om);
        mux->last_tx_timestamp = mux->event_tx_timestamp;
        mux->last_tx_packet_seq_num = blehdr->txiso.packet_seq_num;
    }

    if (mux->framed) {
        return ble_ll_isoal_mux_framed_event_done(mux);
    }

    return ble_ll_isoal_mux_unframed_event_done(mux);
}

static int
ble_ll_isoal_mux_unframed_get(struct ble_ll_isoal_mux *mux, uint8_t idx,
                              uint8_t *llid, void *dptr)
{
    struct os_mbuf_pkthdr *pkthdr;
    struct os_mbuf *om;
    int32_t rem_len;
    uint8_t sdu_idx;
    uint8_t pdu_idx;
    uint16_t sdu_offset;
    uint8_t pdu_len;

    sdu_idx = idx / mux->pdu_per_sdu;
    pdu_idx = idx - sdu_idx * mux->pdu_per_sdu;

    if (sdu_idx >= mux->sdu_in_event) {
        *llid = 0;
        return 0;
    }

    pkthdr = STAILQ_FIRST(&mux->sdu_q);
    while (pkthdr && sdu_idx--) {
        pkthdr = STAILQ_NEXT(pkthdr, omp_next);
    }

    if (!pkthdr) {
        *llid = 0;
        return 0;
    }

    om = OS_MBUF_PKTHDR_TO_MBUF(pkthdr);
    sdu_offset = pdu_idx * mux->max_pdu;
    rem_len = OS_MBUF_PKTLEN(om) - sdu_offset;

    if (OS_MBUF_PKTLEN(om) == 0) {
        /* LLID = 0b00: Zero-Length SDU (complete SDU) */
        *llid = 0;
        pdu_len = 0;
    } else if (rem_len <= 0) {
        /* LLID = 0b01: ISO Data PDU used as padding */
        *llid = 1;
        pdu_len = 0;
    } else {
        /* LLID = 0b00: Data remaining fits the ISO Data PDU size,
         *              it's end fragment of an SDU or complete SDU.
         * LLID = 0b01: Data remaining exceeds the ISO Data PDU size,
         *              it's start or continuation fragment of an SDU.
         */
        *llid = rem_len > mux->max_pdu;
        pdu_len = min(mux->max_pdu, rem_len);
    }

    os_mbuf_copydata(om, sdu_offset, pdu_len, dptr);

    return pdu_len;
}

static int
ble_ll_isoal_mux_framed_get(struct ble_ll_isoal_mux *mux, uint8_t idx,
                            uint8_t *llid, uint8_t *dptr)
{
    struct ble_mbuf_hdr *blehdr;
    struct os_mbuf_pkthdr *pkthdr;
    struct os_mbuf *om;
    uint32_t time_offset;
    uint16_t seghdr;
    uint16_t rem_len = 0;
    uint16_t sdu_offset = 0;
    uint8_t num_sdu;
    uint8_t num_pdu;
    uint8_t frag_len;
    uint8_t pdu_offset = 0;
    bool sc = mux->sc;
    uint8_t hdr_len = 0;

    *llid = 0b10;

    num_sdu = mux->sdu_in_event;
    if (num_sdu == 0) {
        return 0;
    }

    num_pdu = idx;

    /* Skip the idx PDUs */
    pkthdr = STAILQ_FIRST(&mux->sdu_q);
    while (pkthdr && num_sdu > 0 && num_pdu > 0) {
        om = OS_MBUF_PKTHDR_TO_MBUF(pkthdr);

        rem_len = OS_MBUF_PKTLEN(om) - sdu_offset;
        hdr_len = sc ? 2 /* Segmentation Header */
                     : 5 /* Segmentation Header + TimeOffset */;

        if (mux->max_pdu <= hdr_len + pdu_offset) {
            /* Advance to next PDU */
            pdu_offset = 0;
            num_pdu--;
            continue;
        }

        frag_len = min(rem_len, mux->max_pdu - hdr_len - pdu_offset);

        pdu_offset += hdr_len + frag_len;

        if (frag_len == rem_len) {
            /* Process next SDU */
            sdu_offset = 0;
            num_sdu--;
            pkthdr = STAILQ_NEXT(pkthdr, omp_next);

            sc = 0;
        } else {
            sdu_offset += frag_len;

            sc = 1;
        }
    }

    if (num_pdu > 0) {
        return 0;
    }

    BLE_LL_ASSERT(pdu_offset == 0);

    while (pkthdr && num_sdu > 0) {
        om = OS_MBUF_PKTHDR_TO_MBUF(pkthdr);

        rem_len = OS_MBUF_PKTLEN(om) - sdu_offset;
        hdr_len = sc ? 2 /* Segmentation Header */
                     : 5 /* Segmentation Header + TimeOffset */;

        if (mux->max_pdu <= hdr_len + pdu_offset) {
            break;
        }

        frag_len = min(rem_len, mux->max_pdu - hdr_len - pdu_offset);

        /* Segmentation Header */
        seghdr = BLE_LL_ISOAL_SEGHDR(sc, frag_len == rem_len, frag_len + hdr_len - 2);
        put_le16(dptr + pdu_offset, seghdr);
        pdu_offset += 2;

        /* Time Offset */
        if (hdr_len > 2) {
            blehdr = BLE_MBUF_HDR_PTR(om);

            time_offset = mux->event_tx_timestamp -
                          blehdr->txiso.cpu_timestamp;
            put_le24(dptr + pdu_offset, time_offset);
            pdu_offset += 3;
        }

        /* ISO Data Fragment */
        os_mbuf_copydata(om, sdu_offset, frag_len, dptr + pdu_offset);
        pdu_offset += frag_len;

        if (frag_len == rem_len) {
            /* Process next SDU */
            sdu_offset = 0;
            num_sdu--;
            pkthdr = STAILQ_NEXT(pkthdr, omp_next);

            sc = 0;
        } else {
            sdu_offset += frag_len;

            sc = 1;
        }
    }

    return pdu_offset;
}

int
ble_ll_isoal_mux_pdu_get(struct ble_ll_isoal_mux *mux, uint8_t idx,
                         uint8_t *llid, void *dptr)
{
    if (mux->framed) {
        return ble_ll_isoal_mux_framed_get(mux, idx, llid, dptr);
    }

    return ble_ll_isoal_mux_unframed_get(mux, idx, llid, dptr);
}

void
ble_ll_isoal_init(void)
{
}

void
ble_ll_isoal_reset(void)
{
}

#endif /* BLE_LL_ISO */

#endif /* ESP_PLATFORM */
