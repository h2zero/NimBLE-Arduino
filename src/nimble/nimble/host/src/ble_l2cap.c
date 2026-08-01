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

#include <string.h>
#include <errno.h>
#include "syscfg/syscfg.h"
#include "nimble/porting/nimble/include/os/os.h"
#include "nimble/nimble/host/include/host/ble_l2cap.h"
#include "nimble/nimble/include/nimble/ble.h"
#include "nimble/nimble/include/nimble/hci_common.h"
#include "ble_hs_priv.h"
#include "ble_l2cap_coc_priv.h"

#if NIMBLE_BLE_CONNECT
_Static_assert(sizeof (struct ble_l2cap_hdr) == BLE_L2CAP_HDR_SZ,
               "struct ble_l2cap_hdr must be 4 bytes");

struct os_mempool ble_l2cap_chan_pool;

static os_membuf_t ble_l2cap_chan_mem[
    OS_MEMPOOL_SIZE(MYNEWT_VAL(BLE_L2CAP_MAX_CHANS) +
                    MYNEWT_VAL(BLE_L2CAP_COC_MAX_NUM),
                    sizeof (struct ble_l2cap_chan))
];

STATS_SECT_DECL(ble_l2cap_stats) ble_l2cap_stats;
STATS_NAME_START(ble_l2cap_stats)
    STATS_NAME(ble_l2cap_stats, chan_create)
    STATS_NAME(ble_l2cap_stats, chan_delete)
    STATS_NAME(ble_l2cap_stats, update_init)
    STATS_NAME(ble_l2cap_stats, update_rx)
    STATS_NAME(ble_l2cap_stats, update_fail)
    STATS_NAME(ble_l2cap_stats, proc_timeout)
    STATS_NAME(ble_l2cap_stats, sig_tx)
    STATS_NAME(ble_l2cap_stats, sig_rx)
    STATS_NAME(ble_l2cap_stats, sm_tx)
    STATS_NAME(ble_l2cap_stats, sm_rx)
STATS_NAME_END(ble_l2cap_stats)

struct ble_l2cap_chan *
ble_l2cap_chan_alloc(uint16_t conn_handle)
{
    struct ble_l2cap_chan *chan;

    chan = os_memblock_get(&ble_l2cap_chan_pool);
    if (chan == NULL) {
        return NULL;
    }

    memset(chan, 0, sizeof *chan);
    chan->conn_handle = conn_handle;

    STATS_INC(ble_l2cap_stats, chan_create);

    return chan;
}

void
ble_l2cap_chan_free(struct ble_hs_conn *conn, struct ble_l2cap_chan *chan)
{
    int rc;

    if (chan == NULL) {
        return;
    }

    ble_l2cap_coc_cleanup_chan(conn, chan);

#if MYNEWT_VAL(BLE_HS_DEBUG)
    memset(chan, 0xff, sizeof *chan);
#endif
    rc = os_memblock_put(&ble_l2cap_chan_pool, chan);
    BLE_HS_DBG_ASSERT_EVAL(rc == 0);

    STATS_INC(ble_l2cap_stats, chan_delete);
}

bool
ble_l2cap_is_mtu_req_sent(const struct ble_l2cap_chan *chan)
{
    return (chan->flags & BLE_L2CAP_CHAN_F_TXED_MTU);
}

int
ble_l2cap_parse_hdr(struct os_mbuf *om, struct ble_l2cap_hdr *hdr)
{
    int rc;

    rc = os_mbuf_copydata(om, 0, sizeof(*hdr), hdr);
    if (rc != 0) {
        return BLE_HS_EMSGSIZE;
    }

    hdr->len = get_le16(&hdr->len);
    hdr->cid = get_le16(&hdr->cid);

    return 0;
}

struct os_mbuf *
ble_l2cap_prepend_hdr(struct os_mbuf *om, uint16_t cid, uint16_t len)
{
    struct ble_l2cap_hdr hdr;

    put_le16(&hdr.len, len);
    put_le16(&hdr.cid, cid);

    om = os_mbuf_prepend_pullup(om, sizeof hdr);
    if (om == NULL) {
        return NULL;
    }

    memcpy(om->om_data, &hdr, sizeof hdr);

    return om;
}

uint16_t
ble_l2cap_get_conn_handle(struct ble_l2cap_chan *chan)
{
    if (!chan) {
        return BLE_HS_CONN_HANDLE_NONE;
    }

    return chan->conn_handle;
}

int
ble_l2cap_create_server(uint16_t psm, uint16_t mtu,
                        ble_l2cap_event_fn *cb, void *cb_arg)
{
    int rc;

    ble_hs_lock();
    rc = ble_l2cap_coc_create_server_nolock(psm, mtu, cb, cb_arg);
    ble_hs_unlock();

    return rc;
}

int
ble_l2cap_remove_server(uint16_t psm)
{
    int rc;

    ble_hs_lock();
    rc = ble_l2cap_coc_remove_server_nolock(psm);
    ble_hs_unlock();

    return rc;
}

int
ble_l2cap_connect(uint16_t conn_handle, uint16_t psm, uint16_t mtu,
                  struct os_mbuf *sdu_rx, ble_l2cap_event_fn *cb, void *cb_arg)
{
    int rc;

    ble_hs_lock();
    rc = ble_l2cap_sig_connect_nolock(conn_handle, psm, mtu, sdu_rx, cb, cb_arg);
    ble_hs_unlock();

    return rc;
}

int
ble_l2cap_get_chan_info(struct ble_l2cap_chan *chan, struct ble_l2cap_chan_info *chan_info)
{
    if (!chan || !chan_info) {
        return BLE_HS_EINVAL;
    }

    memset(chan_info, 0, sizeof(*chan_info));
    chan_info->dcid = chan->dcid;
    chan_info->scid = chan->scid;
    chan_info->our_l2cap_mtu = chan->my_mtu;
    chan_info->peer_l2cap_mtu = chan->peer_mtu;

#if MYNEWT_VAL(BLE_L2CAP_COC_MAX_NUM)
    chan_info->psm = chan->psm;
    chan_info->our_coc_mtu = chan->coc_rx.mtu;
    chan_info->peer_coc_mtu = chan->coc_tx.mtu;
#endif

    return 0;
}

int
ble_l2cap_enhanced_connect(uint16_t conn_handle,
                               uint16_t psm, uint16_t mtu,
                               uint8_t num, struct os_mbuf *sdu_rx[],
                               ble_l2cap_event_fn *cb, void *cb_arg)
{
    int rc;

    ble_hs_lock();
    rc = ble_l2cap_sig_ecoc_connect_nolock(conn_handle, psm, mtu, num, sdu_rx,
                                           cb, cb_arg);
    ble_hs_unlock();

    return rc;
}

int
ble_l2cap_reconfig(struct ble_l2cap_chan *chans[], uint8_t num, uint16_t new_mtu)
{
    int i;
    uint16_t conn_handle;
    int rc;

    if (num == 0 || !chans) {
        return BLE_HS_EINVAL;
    }

    conn_handle = chans[0]->conn_handle;

    for (i = 1; i < num; i++) {
        if (conn_handle != chans[i]->conn_handle) {
            BLE_HS_LOG(ERROR, "All channels should have same conn handle\n");
            return BLE_HS_EINVAL;
        }
    }

    ble_hs_lock();
    rc = ble_l2cap_sig_coc_reconfig_nolock(conn_handle, chans, num, new_mtu);
    ble_hs_unlock();

    return rc;
}

int
ble_l2cap_disconnect(struct ble_l2cap_chan *chan)
{
    int rc;

    ble_hs_lock();
    rc = ble_l2cap_sig_disconnect_nolock(chan);
    ble_hs_unlock();

    return rc;
}

/**
 * Transmits a packet over an L2CAP channel.  This function only consumes the
 * supplied mbuf on success.
 */
int
ble_l2cap_send(struct ble_l2cap_chan *chan, struct os_mbuf *sdu)
{
    return ble_l2cap_coc_send(chan, sdu);
}

int
ble_l2cap_recv_ready(struct ble_l2cap_chan *chan, struct os_mbuf *sdu_rx)
{
    return ble_l2cap_coc_recv_ready(chan, sdu_rx);
}

static uint16_t
ble_l2cap_get_mtu(struct ble_l2cap_chan *chan)
{
    if (chan->scid == BLE_L2CAP_CID_ATT) {
        /* In case of ATT chan->my_mtu keeps preferred MTU which is later
         * used during exchange MTU procedure. Helper below gives us actual
         * MTU on the channel, which is 23 or higher if exchange MTU has been
         * done
         */
        return ble_att_chan_mtu(chan);
    }

    return chan->my_mtu;
}

static void
ble_l2cap_rx_free(struct ble_hs_conn *conn)
{
    os_mbuf_free_chain(conn->rx_frags);
    conn->rx_frags = NULL;
    conn->rx_len = 0;
    conn->rx_cid = 0;
}

static int
ble_l2cap_rx_frags_process(struct ble_hs_conn *conn)
{
    int rem_rx_len;
    int rc;

    rem_rx_len = conn->rx_len - OS_MBUF_PKTLEN(conn->rx_frags);
    if (rem_rx_len == 0) {
        rc = 0;
    } else if (rem_rx_len > 0) {
#if MYNEWT_VAL(BLE_L2CAP_RX_FRAG_TIMEOUT) != 0
        conn->rx_frag_tmo =
            ble_npl_time_get() +
            ble_npl_time_ms_to_ticks32(MYNEWT_VAL(BLE_L2CAP_RX_FRAG_TIMEOUT));

        ble_hs_timer_resched();
#endif
        rc = BLE_HS_EAGAIN;
    } else {
        ble_l2cap_rx_free(conn);
        rc = BLE_HS_EBADDATA;
    }

    return rc;
}

int
ble_l2cap_rx(uint16_t conn_handle, uint8_t pb, struct os_mbuf *om)
{
    struct ble_hs_conn *conn;
    struct ble_l2cap_chan *chan;
    struct ble_l2cap_hdr hdr;
    struct os_mbuf *rx_frags;
    uint16_t rx_len;
    uint16_t rx_cid;
    int rc;

    ble_hs_lock();

    conn = ble_hs_conn_find(conn_handle);
    if (!conn) {
        /* Invalid connection handle, discard packet */
        os_mbuf_free_chain(om);
        rc = BLE_HS_ENOTCONN;
        goto done;
    }

    switch (pb) {
    case BLE_HCI_PB_FIRST_FLUSH:
        if (conn->rx_frags) {
            /* Previously received data is incomplete, discard it */
            ble_l2cap_rx_free(conn);
        }
        conn->rx_frags = om;
        break;
    case BLE_HCI_PB_MIDDLE:
        if (!conn->rx_frags) {
            /* Received continuation without 1st packet, discard it.
             * This can also happen if we received invalid data earlier which
             * was discarded, so we'll just keep discarding until valid 1st
             * packet is received.
             */
            os_mbuf_free_chain(om);
            rc = BLE_HS_EBADDATA;
            goto done;
        }

        /* Append fragment to rx buffer */
#if MYNEWT_VAL(BLE_L2CAP_JOIN_RX_FRAGS)
        os_mbuf_pack_chains(conn->rx_frags, om);
#else
        os_mbuf_concat(conn->rx_frag, om);
#endif
        break;
    default:
        /* Invalid PB, discard packet */
        os_mbuf_free_chain(om);
        ble_l2cap_rx_free(conn);
        rc = BLE_HS_EBADDATA;
        goto done;
    }

    /* Parse L2CAP header if not yet done */
    if (!conn->rx_len) {
        rc = ble_l2cap_parse_hdr(conn->rx_frags, &hdr);
        if (rc) {
            /* Incomplete header, wait for continuation */
            rc = BLE_HS_EAGAIN;
            goto done;
        }

        os_mbuf_adj(conn->rx_frags, BLE_L2CAP_HDR_SZ);

        conn->rx_len = hdr.len;
        conn->rx_cid = hdr.cid;
    }

    /* Process fragments */
    rc = ble_l2cap_rx_frags_process(conn);
    if (rc) {
        goto done;
    }

    rx_frags = conn->rx_frags;
    rx_len = conn->rx_len;
    rx_cid = conn->rx_cid;

    conn->rx_frags = NULL;
    ble_l2cap_rx_free(conn);

    chan = ble_hs_conn_chan_find_by_scid(conn, rx_cid);

    ble_hs_unlock();

    if (!chan) {
        ble_l2cap_sig_reject_invalid_cid_tx(conn_handle, 0, 0, rx_cid);
        os_mbuf_free_chain(rx_frags);
        return BLE_HS_ENOENT;
    }

    /* disconnect pending, drop data */
    if (chan->flags & BLE_L2CAP_CHAN_F_DISCONNECTING) {
        os_mbuf_free_chain(rx_frags);
        return 0;
    }

    if (chan->dcid >= BLE_L2CAP_COC_CID_START &&
        chan->dcid <= BLE_L2CAP_COC_CID_END && rx_len > chan->my_coc_mps) {
        ble_l2cap_disconnect(chan);
        os_mbuf_free_chain(rx_frags);
        return BLE_HS_EBADDATA;
    }

    if (rx_len > ble_l2cap_get_mtu(chan)) {
        ble_l2cap_disconnect(chan);
        os_mbuf_free_chain(rx_frags);
        return BLE_HS_EBADDATA;
    }

    rc = chan->rx_fn(chan, &rx_frags);
    os_mbuf_free_chain(rx_frags);

    return rc;

done:
    ble_hs_unlock();

    return rc;
}

/**
 * Transmits the L2CAP payload contained in the specified mbuf.  The supplied
 * mbuf is consumed, regardless of the outcome of the function call.
 *
 * @param chan                  The L2CAP channel to transmit over.
 * @param txom                  The data to transmit.
 *
 * @return                      0 on success; nonzero on error.
 */
int
ble_l2cap_tx(struct ble_hs_conn *conn, struct ble_l2cap_chan *chan,
             struct os_mbuf *txom)
{
    int rc;

    txom = ble_l2cap_prepend_hdr(txom, chan->dcid, OS_MBUF_PKTLEN(txom));
    if (txom == NULL) {
        return BLE_HS_ENOMEM;
    }

    rc = ble_hs_hci_acl_tx(conn, &txom);
    switch (rc) {
    case 0:
        /* Success. */
        return 0;

    case BLE_HS_EAGAIN:
        /* Controller could not accommodate full packet.  Enqueue remainder. */
        STAILQ_INSERT_TAIL(&conn->bhc_tx_q, OS_MBUF_PKTHDR(txom), omp_next);
        return 0;

    default:
        /* Error. */
        return rc;
    }
}

int
ble_l2cap_init(void)
{
    int rc;

    rc = os_mempool_init(&ble_l2cap_chan_pool,
                         MYNEWT_VAL(BLE_L2CAP_MAX_CHANS) +
                         MYNEWT_VAL(BLE_L2CAP_COC_MAX_NUM),
                         sizeof (struct ble_l2cap_chan),
                         ble_l2cap_chan_mem, "ble_l2cap_chan_pool");
    if (rc != 0) {
        return BLE_HS_EOS;
    }

    rc = ble_l2cap_sig_init();
    if (rc != 0) {
        return rc;
    }

    rc = ble_l2cap_coc_init();
    if (rc != 0) {
        return rc;
    }

    rc = ble_sm_init();
    if (rc != 0) {
        return rc;
    }

    rc = stats_init_and_reg(
        STATS_HDR(ble_l2cap_stats), STATS_SIZE_INIT_PARMS(ble_l2cap_stats,
        STATS_SIZE_32), STATS_NAME_INIT_PARMS(ble_l2cap_stats), "ble_l2cap");
    if (rc != 0) {
        return BLE_HS_EOS;
    }

    return 0;
}

#endif
