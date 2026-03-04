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

#include "syscfg/syscfg.h"

#if MYNEWT_VAL(BLE_ISO)
#include "nimble/porting/nimble/include/os/os_mbuf.h"
#include "nimble/nimble/host/include/host/ble_hs_log.h"
#include "nimble/nimble/host/include/host/ble_hs.h"
#include "nimble/nimble/host/include/host/ble_iso.h"
#include "nimble/nimble/include/nimble/hci_common.h"
#include "sys/queue.h"
#include "ble_hs_priv.h"
#include "ble_hs_hci_priv.h"
#include "ble_hs_mbuf_priv.h"

#ifndef min
#define min(a, b) ((a) < (b) ? (a) : (b))
#endif

#define ble_iso_big_conn_handles_init(_big, _handles, _num_handles)         \
    do {                                                                    \
        struct ble_iso_conn *conn = SLIST_FIRST(&ble_iso_conns);            \
                                                                            \
        for (uint8_t i = 0; i < (_num_handles); i++) {                      \
            while (conn != NULL) {                                          \
                if (conn->type == BLE_ISO_CONN_BIS) {                       \
                    struct ble_iso_bis *bis;                                \
                                                                            \
                    bis = CONTAINER_OF(conn, struct ble_iso_bis, conn);     \
                    if (bis->big == (_big)) {                               \
                        conn->handle = le16toh((_handles)[i]);              \
                        conn = SLIST_NEXT(conn, next);                      \
                        break;                                              \
                    }                                                       \
                }                                                           \
                                                                            \
                conn = SLIST_NEXT(conn, next);                              \
            }                                                               \
        }                                                                   \
    } while (0);

enum ble_iso_conn_type {
    BLE_ISO_CONN_BIS,
};

struct ble_iso_big {
    SLIST_ENTRY(ble_iso_big) next;
    uint8_t handle;
    uint16_t max_pdu;
    uint8_t bis_cnt;

    ble_iso_event_fn *cb;
    void *cb_arg;
};

struct ble_iso_conn {
    SLIST_ENTRY(ble_iso_conn) next;
    enum ble_iso_conn_type type;
    uint8_t handle;

    struct ble_iso_rx_data_info rx_info;
    struct os_mbuf *rx_buf;

    ble_iso_event_fn *cb;
    void *cb_arg;
};

struct ble_iso_bis {
    struct ble_iso_conn conn;
    struct ble_iso_big *big;
};

static SLIST_HEAD(, ble_iso_big) ble_iso_bigs;
static SLIST_HEAD(, ble_iso_conn) ble_iso_conns;
static struct os_mempool ble_iso_big_pool;
static os_membuf_t ble_iso_big_mem[
    OS_MEMPOOL_SIZE(MYNEWT_VAL(BLE_ISO_MAX_BIGS), sizeof (struct ble_iso_big))];
static struct os_mempool ble_iso_bis_pool;
static os_membuf_t ble_iso_bis_mem[
    OS_MEMPOOL_SIZE(MYNEWT_VAL(BLE_ISO_MAX_BISES), sizeof (struct ble_iso_bis))];

static void
ble_iso_conn_append(struct ble_iso_conn *conn)
{
    struct ble_iso_conn *entry, *prev = NULL;

    SLIST_FOREACH(entry, &ble_iso_conns, next) {
        prev = entry;
    }

    if (prev == NULL) {
        SLIST_INSERT_HEAD(&ble_iso_conns, conn, next);
    } else {
        SLIST_INSERT_AFTER(prev, conn, next);
    }
}

static int
ble_iso_big_handle_set(struct ble_iso_big *big)
{
    static uint8_t free_handle;
    uint8_t i;

    /* Set next free handle */
    for (i = BLE_HCI_ISO_BIG_HANDLE_MIN; i < BLE_HCI_ISO_BIG_HANDLE_MAX; i++) {
        struct ble_iso_big *node = NULL;

        if (free_handle > BLE_HCI_ISO_BIG_HANDLE_MAX) {
            free_handle = BLE_HCI_ISO_BIG_HANDLE_MIN;
        }

        big->handle = free_handle++;

        SLIST_FOREACH(node, &ble_iso_bigs, next) {
            if (node->handle == big->handle) {
                break;
            }
        }

        if (node == NULL || node->handle != big->handle) {
            return 0;
        }
    }

    BLE_HS_DBG_ASSERT(0);

    return BLE_HS_EOS;
}

static struct ble_iso_big *
ble_iso_big_alloc(void)
{
    struct ble_iso_big *new_big;
    int rc;

    new_big = os_memblock_get(&ble_iso_big_pool);
    if (new_big == NULL) {
        BLE_HS_LOG_ERROR("No more memory in pool\n");
        /* Out of memory. */
        return NULL;
    }

    memset(new_big, 0, sizeof *new_big);

    rc = ble_iso_big_handle_set(new_big);
    if (rc != 0) {
        os_memblock_put(&ble_iso_big_pool, new_big);
        return NULL;
    }

    SLIST_INSERT_HEAD(&ble_iso_bigs, new_big, next);

    return new_big;
}

static struct ble_iso_bis *
ble_iso_bis_alloc(struct ble_iso_big *big)
{
    struct ble_iso_bis *new_bis;

    new_bis = os_memblock_get(&ble_iso_bis_pool);
    if (new_bis == NULL) {
        BLE_HS_LOG_ERROR("No more memory in pool\n");
        /* Out of memory. */
        return NULL;
    }

    memset(new_bis, 0, sizeof *new_bis);
    new_bis->conn.type = BLE_ISO_CONN_BIS;
    new_bis->big = big;

    ble_iso_conn_append(&new_bis->conn);

    return new_bis;
}

static struct ble_iso_big *
ble_iso_big_find_by_handle(uint8_t big_handle)
{
    struct ble_iso_big *big;

    SLIST_FOREACH(big, &ble_iso_bigs, next) {
        if (big->handle == big_handle) {
            return big;
        }
    }

    return NULL;
}

static int
ble_iso_big_free(struct ble_iso_big *big)
{
    struct ble_iso_conn *conn;
    struct ble_iso_bis *rem_bis[MYNEWT_VAL(BLE_ISO_MAX_BISES)] = {
        [0 ... MYNEWT_VAL(BLE_ISO_MAX_BISES) - 1] = NULL
    };
    uint8_t i = 0;

    SLIST_FOREACH(conn, &ble_iso_conns, next) {
        struct ble_iso_bis *bis;

        if (conn->type != BLE_ISO_CONN_BIS) {
            continue;
        }

        bis = CONTAINER_OF(conn, struct ble_iso_bis, conn);
        if (bis->big == big) {
            SLIST_REMOVE(&ble_iso_conns, conn, ble_iso_conn, next);
            rem_bis[i++] = bis;
        }
    }

    while (i > 0) {
        os_memblock_put(&ble_iso_bis_pool, rem_bis[--i]);
    }

    SLIST_REMOVE(&ble_iso_bigs, big, ble_iso_big, next);
    os_memblock_put(&ble_iso_big_pool, big);
    return 0;
}

static struct ble_iso_conn *
ble_iso_conn_lookup_handle(uint16_t handle)
{
    struct ble_iso_conn *conn;

    SLIST_FOREACH(conn, &ble_iso_conns, next) {
        if (conn->handle == handle) {
            return conn;
        }
    }

    return NULL;
}

#if MYNEWT_VAL(BLE_ISO_BROADCAST_SOURCE)
int
ble_iso_create_big(const struct ble_iso_create_big_params *create_params,
                   const struct ble_iso_big_params *big_params,
                   uint8_t *big_handle)
{
    struct ble_hci_le_create_big_cp cp = { 0 };
    struct ble_iso_big *big;
    int rc;

    cp.adv_handle = create_params->adv_handle;
    if (create_params->bis_cnt > MYNEWT_VAL(BLE_ISO_MAX_BISES)) {
        return BLE_HS_EINVAL;
    }

    big = ble_iso_big_alloc();
    if (big == NULL) {
        return BLE_HS_ENOMEM;
    }

    big->bis_cnt = create_params->bis_cnt;
    big->cb = create_params->cb;
    big->cb_arg = create_params->cb_arg;

    for (uint8_t i = 0; i < create_params->bis_cnt; i++) {
        struct ble_iso_bis *bis;

        bis = ble_iso_bis_alloc(big);
        if (bis == NULL) {
            ble_iso_big_free(big);
            return BLE_HS_ENOMEM;
        }
    }

    cp.adv_handle = create_params->adv_handle;
    cp.num_bis = create_params->bis_cnt;
    put_le24(cp.sdu_interval, big_params->sdu_interval);
    cp.max_sdu = big_params->max_sdu;
    cp.max_transport_latency = big_params->max_transport_latency;
    cp.rtn = big_params->rtn;
    cp.phy = big_params->phy;
    cp.packing = big_params->packing;
    cp.framing = big_params->framing;
    cp.encryption = big_params->encryption;
    if (big_params->encryption) {
        memcpy(cp.broadcast_code, big_params->broadcast_code, 16);
    }

    rc = ble_hs_hci_cmd_tx(BLE_HCI_OP(BLE_HCI_OGF_LE,
                                      BLE_HCI_OCF_LE_CREATE_BIG),
                           &cp, sizeof(cp),NULL, 0);
    if (rc != 0) {
        ble_iso_big_free(big);
    } else {
        *big_handle = big->handle;
    }

    return rc;
}

int
ble_iso_terminate_big(uint8_t big_handle)
{
    struct ble_hci_le_terminate_big_cp cp;
    struct ble_iso_big *big;
    int rc;

    big = ble_iso_big_find_by_handle(big_handle);
    if (big == NULL) {
        BLE_HS_LOG_ERROR("No BIG with handle=%d\n", big_handle);
        return BLE_HS_ENOENT;
    }

    cp.big_handle = big->handle;
    cp.reason = BLE_ERR_CONN_TERM_LOCAL;

    rc = ble_hs_hci_cmd_tx(BLE_HCI_OP(BLE_HCI_OGF_LE,
                                      BLE_HCI_OCF_LE_TERMINATE_BIG),
                           &cp, sizeof(cp),NULL, 0);

    return rc;
}

void
ble_iso_rx_create_big_complete(const struct ble_hci_ev_le_subev_create_big_complete *ev)
{
    struct ble_iso_event event;
    struct ble_iso_big *big;

    big = ble_iso_big_find_by_handle(ev->big_handle);
    if (big == NULL) {
        BLE_HS_LOG_ERROR("No BIG with handle=%d\n", ev->big_handle);
        return;
    }

    memset(&event, 0, sizeof(event));
    event.type = BLE_ISO_EVENT_BIG_CREATE_COMPLETE;
    event.big_created.status = ev->status;

    if (event.big_created.status != 0) {
        ble_iso_big_free(big);
    } else {
        if (big->bis_cnt != ev->num_bis) {
            BLE_HS_LOG_ERROR("Unexpected num_bis=%d != bis_cnt=%d\n",
                             ev->num_bis, big->bis_cnt);
            /* XXX: Should we destroy the group? */
        }

        ble_iso_big_conn_handles_init(big, ev->conn_handle, ev->num_bis);

        big->max_pdu = ev->max_pdu;

        event.big_created.desc.big_handle = ev->big_handle;
        event.big_created.desc.big_sync_delay = get_le24(ev->big_sync_delay);
        event.big_created.desc.transport_latency_big =
            get_le24(ev->transport_latency_big);
        event.big_created.desc.nse = ev->nse;
        event.big_created.desc.bn = ev->bn;
        event.big_created.desc.pto = ev->pto;
        event.big_created.desc.irc = ev->irc;
        event.big_created.desc.max_pdu = ev->max_pdu;
        event.big_created.desc.iso_interval = ev->iso_interval;
        event.big_created.desc.num_bis = ev->num_bis;
        memcpy(event.big_created.desc.conn_handle, ev->conn_handle,
               ev->num_bis * sizeof(uint16_t));
        event.big_created.phy = ev->phy;
    }

    if (big->cb != NULL) {
        big->cb(&event, big->cb_arg);
    }
}

void
ble_iso_rx_terminate_big_complete(const struct ble_hci_ev_le_subev_terminate_big_complete *ev)
{
    struct ble_iso_event event;
    struct ble_iso_big *big;

    big = ble_iso_big_find_by_handle(ev->big_handle);
    if (big == NULL) {
        BLE_HS_LOG_ERROR("No BIG with handle=%d\n", ev->big_handle);
        return;
    }

    event.type = BLE_ISO_EVENT_BIG_TERMINATE_COMPLETE;
    event.big_terminated.big_handle = ev->big_handle;
    event.big_terminated.reason = ev->reason;

    if (big->cb != NULL) {
        big->cb(&event, big->cb_arg);
    }

    ble_iso_big_free(big);
}

static int
ble_iso_tx_complete(uint16_t conn_handle, const uint8_t *data,
                    uint16_t data_len)
{
    struct os_mbuf *om;
    int rc;

    om = ble_hs_mbuf_bare_pkt();
    if (!om) {
        return BLE_HS_ENOMEM;
    }

    os_mbuf_extend(om, 8);
    /* Connection_Handle, PB_Flag, TS_Flag */
    put_le16(&om->om_data[0],
             BLE_HCI_ISO_HANDLE(conn_handle, BLE_HCI_ISO_PB_COMPLETE, 0));
    /* Data_Total_Length = Data length + Packet_Sequence_Number placeholder */
    put_le16(&om->om_data[2], data_len + 4);
    /* Packet_Sequence_Number placeholder */
    put_le16(&om->om_data[4], 0);
    /* ISO_SDU_Length */
    put_le16(&om->om_data[6], data_len);

    rc = os_mbuf_append(om, data, data_len);
    if (rc) {
        return rc;
    }

    return ble_transport_to_ll_iso(om);
}

static int
ble_iso_tx_segmented(uint16_t conn_handle, const uint8_t *data,
                     uint16_t data_len)
{
    struct os_mbuf *om;
    uint16_t data_left = data_len;
    uint16_t packet_len;
    uint16_t offset = 0;
    uint8_t pb;
    int rc;

    while (data_left) {
        packet_len = min(MYNEWT_VAL(BLE_TRANSPORT_ISO_SIZE), data_left);
        if (data_left == data_len) {
            pb = BLE_HCI_ISO_PB_FIRST;
        } else if (packet_len == data_left) {
            pb = BLE_HCI_ISO_PB_LAST;
        } else {
            pb = BLE_HCI_ISO_PB_CONTINUATION;
        }

        om = ble_hs_mbuf_bare_pkt();
        if (!om) {
            return BLE_HS_ENOMEM;
        }

        os_mbuf_extend(om, pb == BLE_HCI_ISO_PB_FIRST ? 8: 4);

        /* Connection_Handle, PB_Flag, TS_Flag */
        put_le16(&om->om_data[0],
                 BLE_HCI_ISO_HANDLE(conn_handle, pb, 0));

        if (pb == BLE_HCI_ISO_PB_FIRST) {
            /* Data_Total_Length = Data length +
             * Packet_Sequence_Number placeholder*/
            put_le16(&om->om_data[2], packet_len + 4);

            /* Packet_Sequence_Number placeholder */
            put_le16(&om->om_data[8], 0);

            /* ISO_SDU_Length */
            put_le16(&om->om_data[10], packet_len);
        } else {
            put_le16(&om->om_data[2], packet_len);
        }

        rc = os_mbuf_append(om, data + offset, packet_len);
        if (rc) {
            return rc;
        }

        ble_transport_to_ll_iso(om);

        offset += packet_len;
        data_left -= packet_len;
    }

    return 0;
}

int
ble_iso_tx(uint16_t conn_handle, void *data, uint16_t data_len)
{
    int rc;

    if (data_len <= MYNEWT_VAL(BLE_TRANSPORT_ISO_SIZE)) {
        rc = ble_iso_tx_complete(conn_handle, data, data_len);
    } else {
        rc = ble_iso_tx_segmented(conn_handle, data, data_len);
    }

    return rc;
}
#endif /* BLE_ISO_BROADCAST_SOURCE */

#if MYNEWT_VAL(BLE_ISO_BROADCAST_SINK)
int
ble_iso_big_sync_create(const struct ble_iso_big_sync_create_params *param,
                        uint8_t *big_handle)
{
    struct ble_hci_le_big_create_sync_cp *cp;
    uint8_t buf[sizeof(*cp) + MYNEWT_VAL(BLE_ISO_MAX_BISES)];
    struct ble_iso_big *big;
    int rc;

    big = ble_iso_big_alloc();
    if (big == NULL) {
        return BLE_HS_ENOMEM;
    }

    big->bis_cnt = param->bis_cnt;
    big->cb = param->cb;
    big->cb_arg = param->cb_arg;

    cp = (void *)buf;
    cp->big_handle = big->handle;
    put_le16(&cp->sync_handle, param->sync_handle);

    if (param->broadcast_code != NULL) {
        cp->encryption = BLE_HCI_ISO_BIG_ENCRYPTION_ENCRYPTED;
        memcpy(cp->broadcast_code, param->broadcast_code, sizeof(cp->broadcast_code));
    } else {
        cp->encryption = BLE_HCI_ISO_BIG_ENCRYPTION_UNENCRYPTED;
        memset(cp->broadcast_code, 0, sizeof(cp->broadcast_code));
    }

    cp->mse = param->mse;
    put_le16(&cp->sync_timeout, param->sync_timeout);
    cp->num_bis = param->bis_cnt;

    for (uint8_t i = 0; i < param->bis_cnt; i++) {
        struct ble_iso_bis *bis;

        bis = ble_iso_bis_alloc(big);
        if (bis == NULL) {
            ble_iso_big_free(big);
            return BLE_HS_ENOMEM;
        }

        cp->bis[i] = param->bis_params[i].bis_index;
    }

    rc = ble_hs_hci_cmd_tx(BLE_HCI_OP(BLE_HCI_OGF_LE,
                                      BLE_HCI_OCF_LE_BIG_CREATE_SYNC),
                           cp, sizeof(*cp) + cp->num_bis, NULL, 0);
    if (rc != 0) {
        ble_iso_big_free(big);
    } else {
        *big_handle = big->handle;
    }

    return rc;
}

int
ble_iso_big_sync_terminate(uint8_t big_handle)
{
    struct ble_hci_le_big_terminate_sync_cp cp;
    struct ble_hci_le_big_terminate_sync_rp rp;
    struct ble_iso_big *big;
    int rc;

    big = ble_iso_big_find_by_handle(big_handle);
    if (big == NULL) {
        BLE_HS_LOG_ERROR("No BIG with handle=%d\n", big_handle);
        return BLE_HS_ENOENT;
    }

    cp.big_handle = big->handle;

    rc = ble_hs_hci_cmd_tx(BLE_HCI_OP(BLE_HCI_OGF_LE,
                                      BLE_HCI_OCF_LE_BIG_TERMINATE_SYNC),
                           &cp, sizeof(cp), &rp, sizeof(rp));
    if (rc == 0) {
        struct ble_iso_event event;
        ble_iso_event_fn *cb;
        void *cb_arg;

        event.type = BLE_ISO_EVENT_BIG_SYNC_TERMINATED;
        event.big_terminated.big_handle = big_handle;
        event.big_terminated.reason = BLE_ERR_CONN_TERM_LOCAL;

        cb = big->cb;
        cb_arg = big->cb_arg;

        ble_iso_big_free(big);

        if (cb != NULL) {
            cb(&event, cb_arg);
        }
    }

    return rc;
}

void
ble_iso_rx_big_sync_established(const struct ble_hci_ev_le_subev_big_sync_established *ev)
{
    struct ble_iso_event event;
    struct ble_iso_big *big;
    ble_iso_event_fn *cb;
    void *cb_arg;

    big = ble_iso_big_find_by_handle(ev->big_handle);
    if (big == NULL) {
        return;
    }

    cb = big->cb;
    cb_arg = big->cb_arg;

    memset(&event, 0, sizeof(event));
    event.type = BLE_ISO_EVENT_BIG_SYNC_ESTABLISHED;
    event.big_sync_established.status = ev->status;

    if (event.big_sync_established.status != 0) {
        ble_iso_big_free(big);
    } else {
        if (big->bis_cnt != ev->num_bis) {
            BLE_HS_LOG_ERROR("Unexpected num_bis=%d != bis_cnt=%d\n",
                             ev->num_bis, big->bis_cnt);
            /* XXX: Should we destroy the group? */
        }

        ble_iso_big_conn_handles_init(big, ev->conn_handle, ev->num_bis);

        event.big_sync_established.desc.big_handle = ev->big_handle;
        event.big_sync_established.desc.transport_latency_big =
            get_le24(ev->transport_latency_big);
        event.big_sync_established.desc.nse = ev->nse;
        event.big_sync_established.desc.bn = ev->bn;
        event.big_sync_established.desc.pto = ev->pto;
        event.big_sync_established.desc.irc = ev->irc;
        event.big_sync_established.desc.max_pdu = le16toh(ev->max_pdu);
        event.big_sync_established.desc.iso_interval = le16toh(ev->iso_interval);
        event.big_sync_established.desc.num_bis = ev->num_bis;
        memcpy(event.big_sync_established.desc.conn_handle, ev->conn_handle,
               ev->num_bis * sizeof(uint16_t));
    }

    if (cb != NULL) {
        cb(&event, cb_arg);
    }
}

void
ble_iso_rx_big_sync_lost(const struct ble_hci_ev_le_subev_big_sync_lost *ev)
{
    struct ble_iso_event event;
    struct ble_iso_big *big;
    ble_iso_event_fn *cb;
    void *cb_arg;

    big = ble_iso_big_find_by_handle(ev->big_handle);
    if (big == NULL) {
        BLE_HS_LOG_ERROR("No BIG with handle=%d\n", ev->big_handle);
        return;
    }

    event.type = BLE_ISO_EVENT_BIG_SYNC_TERMINATED;
    event.big_terminated.big_handle = ev->big_handle;
    event.big_terminated.reason = ev->reason;

    cb = big->cb;
    cb_arg = big->cb_arg;

    ble_iso_big_free(big);

    if (cb != NULL) {
        cb(&event, cb_arg);
    }
}

static int
ble_iso_rx_data_info_parse(struct os_mbuf *om, bool ts_available,
                           struct ble_iso_rx_data_info *info)
{
    struct ble_hci_iso_data *iso_data;
    uint16_t u16;

    if (ts_available) {
        if (os_mbuf_len(om) < sizeof(info->ts)) {
            BLE_HS_LOG_DEBUG("Data missing\n");
            return BLE_HS_EMSGSIZE;
        }

        info->ts = get_le32(om->om_data);
        os_mbuf_adj(om, sizeof(info->ts));
    } else {
        info->ts = 0;
    }

    if (os_mbuf_len(om) < sizeof(*iso_data)) {
        BLE_HS_LOG_DEBUG("Data missing\n");
        return BLE_HS_EMSGSIZE;
    }

    iso_data = (void *)(om->om_data);

    info->seq_num = le16toh(iso_data->packet_seq_num);
    u16 = le16toh(iso_data->sdu_len);
    info->sdu_len = BLE_HCI_ISO_SDU_LENGTH(u16);
    info->status = BLE_HCI_ISO_PKT_STATUS_FLAG(u16);
    info->ts_valid = ts_available;

    os_mbuf_adj(om, sizeof(*iso_data));

    return 0;
}

static void
ble_iso_conn_rx_reset(struct ble_iso_conn *conn)
{
    memset(&conn->rx_info, 0, sizeof(conn->rx_info));
    conn->rx_buf = NULL;
}

static void
ble_iso_conn_rx_data_discard(struct ble_iso_conn *conn)
{
    os_mbuf_free_chain(conn->rx_buf);
    ble_iso_conn_rx_reset(conn);
}

static void
ble_iso_event_iso_rx_emit(struct ble_iso_conn *conn)
{
    struct ble_iso_event event = {
        .type = BLE_ISO_EVENT_ISO_RX,
        .iso_rx.conn_handle = conn->handle,
        .iso_rx.info = &conn->rx_info,
        .iso_rx.om = conn->rx_buf,
    };

    if (conn->cb != NULL) {
        conn->cb(&event, conn->cb_arg);
    }
}

static int
ble_iso_conn_rx_data_load(struct ble_iso_conn *conn, struct os_mbuf *frag,
                          uint8_t pb_flag, bool ts_available, void *arg)
{
    int len_remaining;
    int rc;

    switch (pb_flag) {
    case BLE_HCI_ISO_PB_FIRST:
    case BLE_HCI_ISO_PB_COMPLETE:
        if (conn->rx_buf != NULL) {
            /* Previous data packet never completed. Discard old packet. */
            ble_iso_conn_rx_data_discard(conn);
        }

        rc = ble_iso_rx_data_info_parse(frag, ts_available, &conn->rx_info);
        if (rc != 0) {
            return rc;
        }

        conn->rx_buf = frag;
        break;

    case BLE_HCI_ISO_PB_CONTINUATION:
    case BLE_HCI_ISO_PB_LAST:
        if (conn->rx_buf == NULL) {
            /* Last fragment without the start. Discard new packet. */
            return BLE_HS_EBADDATA;
        }

        /* Determine whether the total length won't exceed the declared SDU length */
        len_remaining = conn->rx_info.sdu_len - OS_MBUF_PKTLEN(conn->rx_buf);
        if (len_remaining - os_mbuf_len(frag) < 0) {
            /* SDU Length exceeded. Discard all packets. */
            ble_iso_conn_rx_data_discard(conn);
            return BLE_HS_EBADDATA;
        }

        os_mbuf_concat(conn->rx_buf, frag);
        break;

    default:
        BLE_HS_LOG_ERROR("Invalid pb_flag %d\n", pb_flag);
        return BLE_HS_EBADDATA;
    }

    if (pb_flag == BLE_HCI_ISO_PB_COMPLETE || pb_flag == BLE_HCI_ISO_PB_LAST) {
        ble_iso_event_iso_rx_emit(conn);
        ble_iso_conn_rx_reset(conn);
    }

    return 0;
}

int
ble_iso_rx_data(struct os_mbuf *om, void *arg)
{
    struct ble_iso_conn *conn;
    struct ble_hci_iso *hci_iso;
    uint16_t conn_handle;
    uint16_t length;
    uint16_t pb_flag;
    uint16_t ts_flag;
    uint16_t u16;
    int rc;

    if (os_mbuf_len(om) < sizeof(*hci_iso)) {
        BLE_HS_LOG_DEBUG("Data missing\n");
        os_mbuf_free_chain(om);
        return BLE_HS_EMSGSIZE;
    }

    hci_iso = (void *)om->om_data;

    u16 = le16toh(hci_iso->handle);
    conn_handle = BLE_HCI_ISO_CONN_HANDLE(u16);
    pb_flag = BLE_HCI_ISO_PB_FLAG(u16);
    ts_flag = BLE_HCI_ISO_TS_FLAG(u16);
    length = BLE_HCI_ISO_LENGTH(le16toh(hci_iso->length));

    os_mbuf_adj(om, sizeof(*hci_iso));

    if (os_mbuf_len(om) < length) {
        BLE_HS_LOG_DEBUG("Data missing\n");
        os_mbuf_free_chain(om);
        return BLE_HS_EMSGSIZE;
    }

    conn = ble_iso_conn_lookup_handle(conn_handle);
    if (conn == NULL) {
        BLE_HS_LOG_DEBUG("Unknown handle=%d\n", conn_handle);
        os_mbuf_free_chain(om);
        return BLE_HS_EMSGSIZE;
    }

    rc = ble_iso_conn_rx_data_load(conn, om, pb_flag, ts_flag > 0, arg);
    if (rc != 0) {
        os_mbuf_free_chain(om);
        return rc;
    }

    return 0;
}
#endif /* BLE_ISO_BROADCAST_SINK */

int
ble_iso_data_path_setup(const struct ble_iso_data_path_setup_params *param)
{
    struct ble_hci_le_setup_iso_data_path_rp rp;
    struct ble_hci_le_setup_iso_data_path_cp *cp;
    uint8_t buf[sizeof(*cp) + UINT8_MAX];
    struct ble_iso_conn *conn;
    int rc;

    conn = ble_iso_conn_lookup_handle(param->conn_handle);
    if (conn == NULL) {
        BLE_HS_LOG_ERROR("invalid conn_handle\n");
        return BLE_HS_ENOTCONN;
    }

    if (param->codec_config_len > 0 && param->codec_config == NULL) {
        BLE_HS_LOG_ERROR("Missing codec_config\n");
        return BLE_HS_EINVAL;
    }

    cp = (void *)buf;
    put_le16(&cp->conn_handle, param->conn_handle);
    cp->data_path_dir = 0;

    if (param->data_path_dir & BLE_ISO_DATA_DIR_TX) {
        /* Input (Host to Controller) */
        cp->data_path_dir |= BLE_HCI_ISO_DATA_PATH_DIR_INPUT;
    }

    if (param->data_path_dir & BLE_ISO_DATA_DIR_RX) {
        /* Output (Controller to Host) */
        cp->data_path_dir |= BLE_HCI_ISO_DATA_PATH_DIR_OUTPUT;

        if (cp->data_path_id == BLE_HCI_ISO_DATA_PATH_ID_HCI && param->cb == NULL) {
            BLE_HS_LOG_ERROR("param->cb is NULL\n");
            return BLE_HS_EINVAL;
        }

        conn->cb = param->cb;
        conn->cb_arg = param->cb_arg;
    }

    cp->data_path_id = param->data_path_id;
    cp->codec_id[0] = param->codec_id.format;
    put_le16(&cp->codec_id[1], param->codec_id.company_id);
    put_le16(&cp->codec_id[3], param->codec_id.vendor_specific);
    put_le24(cp->controller_delay, param->ctrl_delay);

    cp->codec_config_len = param->codec_config_len;
    memcpy(cp->codec_config, param->codec_config, cp->codec_config_len);

    rc = ble_hs_hci_cmd_tx(BLE_HCI_OP(BLE_HCI_OGF_LE,
                                      BLE_HCI_OCF_LE_SETUP_ISO_DATA_PATH),
                           cp, sizeof(*cp) + cp->codec_config_len, &rp,
                           sizeof(rp));

    return rc;
}

int
ble_iso_data_path_remove(const struct ble_iso_data_path_remove_params *param)
{
    struct ble_hci_le_remove_iso_data_path_rp rp;
    struct ble_hci_le_remove_iso_data_path_cp cp = { 0 };
    struct ble_iso_conn *conn;
    int rc;

    conn = ble_iso_conn_lookup_handle(param->conn_handle);
    if (conn == NULL) {
        BLE_HS_LOG_ERROR("invalid conn_handle\n");
        return BLE_HS_ENOTCONN;
    }

    put_le16(&cp.conn_handle, param->conn_handle);

    if (param->data_path_dir & BLE_ISO_DATA_DIR_TX) {
        /* Input (Host to Controller) */
        cp.data_path_dir |= BLE_HCI_ISO_DATA_PATH_DIR_INPUT;
    }

    if (param->data_path_dir & BLE_ISO_DATA_DIR_RX) {
        /* Output (Controller to Host) */
        cp.data_path_dir |= BLE_HCI_ISO_DATA_PATH_DIR_OUTPUT;

        conn->cb = NULL;
        conn->cb_arg = NULL;
    }

    rc = ble_hs_hci_cmd_tx(BLE_HCI_OP(BLE_HCI_OGF_LE,
                                      BLE_HCI_OCF_LE_REMOVE_ISO_DATA_PATH),
                           &cp, sizeof(cp), &rp, sizeof(rp));

    return rc;
}

int
ble_iso_init(void)
{
    int rc;

    SLIST_INIT(&ble_iso_bigs);

    rc = os_mempool_init(&ble_iso_big_pool,
                         MYNEWT_VAL(BLE_ISO_MAX_BIGS),
                         sizeof (struct ble_iso_big),
                         ble_iso_big_mem, "ble_iso_big_pool");
    SYSINIT_PANIC_ASSERT(rc == 0);

    rc = os_mempool_init(&ble_iso_bis_pool,
                         MYNEWT_VAL(BLE_ISO_MAX_BISES),
                         sizeof (struct ble_iso_bis),
                         ble_iso_bis_mem, "ble_iso_bis_pool");
    SYSINIT_PANIC_ASSERT(rc == 0);

    return 0;
}
#endif /* BLE_ISO */
