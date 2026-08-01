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
#include "nimble/nimble/include/nimble/hci_common.h"
#include "nimble/nimble/controller/include/controller/ble_ll.h"
#include "nimble/nimble/controller/include/controller/ble_ll_isoal.h"
#include "nimble/nimble/controller/include/controller/ble_ll_iso.h"
#include "nimble/nimble/controller/include/controller/ble_ll_tmr.h"

#if MYNEWT_VAL(BLE_LL_ISO)

STAILQ_HEAD(ble_ll_iso_conn_q, ble_ll_iso_conn);
struct ble_ll_iso_conn_q ll_iso_conn_q;

int
ble_ll_iso_setup_iso_data_path(const uint8_t *cmdbuf, uint8_t cmdlen,
                               uint8_t *rspbuf, uint8_t *rsplen)
{
    const struct ble_hci_le_setup_iso_data_path_cp *cmd = (const void *)cmdbuf;
    struct ble_hci_le_setup_iso_data_path_rp *rsp = (void *)rspbuf;
    struct ble_ll_iso_conn *conn;
    uint16_t conn_handle;

    conn_handle = le16toh(cmd->conn_handle);

    conn = ble_ll_iso_conn_find_by_handle(conn_handle);
    if (!conn) {
        return BLE_ERR_UNK_CONN_ID;
    }

    if (conn->mux.bn == 0) {
        return BLE_ERR_UNSUPPORTED;
    }

    if (conn->data_path.enabled) {
        return BLE_ERR_CMD_DISALLOWED;
    }

    /* Only input for now since we only support BIS */
    if (cmd->data_path_dir) {
        return BLE_ERR_CMD_DISALLOWED;
    }

    /* We do not (yet) support any vendor-specific data path */
    if (cmd->data_path_id) {
        return BLE_ERR_CMD_DISALLOWED;
    }

    conn->data_path.enabled = 1;
    conn->data_path.data_path_id = cmd->data_path_id;

    rsp->conn_handle = cmd->conn_handle;
    *rsplen = sizeof(*rsp);

    return 0;
}

int
ble_ll_iso_remove_iso_data_path(const uint8_t *cmdbuf, uint8_t cmdlen,
                                uint8_t *rspbuf, uint8_t *rsplen)
{
    const struct ble_hci_le_remove_iso_data_path_cp *cmd = (const void *)cmdbuf;
    struct ble_hci_le_remove_iso_data_path_rp *rsp = (void *)rspbuf;
    struct ble_ll_iso_conn *conn;
    uint16_t conn_handle;

    conn_handle = le16toh(cmd->conn_handle);

    conn = ble_ll_iso_conn_find_by_handle(conn_handle);
    if (!conn) {
        return BLE_ERR_UNK_CONN_ID;
    }

    /* Only input for now since we only support BIS */
    if (cmd->data_path_dir) {
        return BLE_ERR_CMD_DISALLOWED;
    }

    conn->data_path.enabled = 0;

    rsp->conn_handle = cmd->conn_handle;
    *rsplen = sizeof(*rsp);

    return 0;
}

int
ble_ll_iso_read_tx_sync(const uint8_t *cmdbuf, uint8_t cmdlen,
                        uint8_t *rspbuf, uint8_t *rsplen)
{
    const struct ble_hci_le_read_iso_tx_sync_cp *cmd = (const void *)cmdbuf;
    struct ble_hci_le_read_iso_tx_sync_rp *rsp = (void *)rspbuf;
    struct ble_ll_iso_conn *iso_conn;
    uint16_t handle;

    handle = le16toh(cmd->conn_handle);
    iso_conn = ble_ll_iso_conn_find_by_handle(handle);
    if (!iso_conn) {
        return BLE_ERR_UNK_CONN_ID;
    }

    rsp->conn_handle = cmd->conn_handle;
    rsp->packet_seq_num = htole16(iso_conn->mux.last_tx_packet_seq_num);
    rsp->tx_timestamp = htole32(iso_conn->mux.last_tx_timestamp);
    put_le24(rsp->time_offset, 0);

    *rsplen = sizeof(*rsp);

    return 0;
}

int
ble_ll_iso_transmit_test(const uint8_t *cmdbuf, uint8_t cmdlen, uint8_t *rspbuf, uint8_t *rsplen)
{
    const struct ble_hci_le_iso_transmit_test_cp *cmd = (const void *)cmdbuf;
    struct ble_hci_le_iso_transmit_test_rp *rsp = (void *)rspbuf;
    struct ble_ll_iso_conn *conn;
    uint16_t handle;

    handle = le16toh(cmd->conn_handle);

    conn = ble_ll_iso_conn_find_by_handle(handle);
    if (!conn) {
        return BLE_ERR_UNK_CONN_ID;
    }

    if (conn->mux.bn == 0) {
        return BLE_ERR_UNSUPPORTED;
    }

    if (conn->data_path.enabled) {
        return BLE_ERR_CMD_DISALLOWED;
    }

    if (cmd->payload_type > BLE_HCI_PAYLOAD_TYPE_MAXIMUM_LENGTH) {
        return BLE_ERR_INV_LMP_LL_PARM;
    }

    conn->data_path.enabled = 1;
    conn->data_path.data_path_id = BLE_HCI_ISO_DATA_PATH_ID_HCI;
    conn->test_mode.transmit.enabled = 1;
    conn->test_mode.transmit.payload_type = cmd->payload_type;

    rsp->conn_handle = cmd->conn_handle;

    *rsplen = sizeof(*rsp);

    return 0;
}

int
ble_ll_iso_end_test(const uint8_t *cmdbuf, uint8_t len, uint8_t *rspbuf, uint8_t *rsplen)
{
    const struct ble_hci_le_iso_test_end_cp *cmd = (const void *)cmdbuf;
    struct ble_hci_le_iso_test_end_rp *rsp = (void *)rspbuf;
    struct ble_ll_iso_conn *iso_conn;
    uint16_t handle;

    handle = le16toh(cmd->conn_handle);
    iso_conn = ble_ll_iso_conn_find_by_handle(handle);
    if (!iso_conn) {
        return BLE_ERR_UNK_CONN_ID;
    }

    if (!iso_conn->test_mode.transmit.enabled) {
        return BLE_ERR_UNSUPPORTED;
    }

    iso_conn->data_path.enabled = 0;
    iso_conn->test_mode.transmit.enabled = 0;

    rsp->conn_handle = cmd->conn_handle;
    rsp->received_sdu_count = 0;
    rsp->missed_sdu_count = 0;
    rsp->failed_sdu_count = 0;

    *rsplen = sizeof(*rsp);

    return 0;
}

struct ble_ll_iso_conn *
ble_ll_iso_conn_find_by_handle(uint16_t conn_handle)
{
    struct ble_ll_iso_conn *conn;

    STAILQ_FOREACH(conn, &ll_iso_conn_q, iso_conn_q_next) {
        if (conn_handle == conn->handle) {
            return conn;
        }
    }

    return NULL;
}

void
ble_ll_iso_init(void)
{
    STAILQ_INIT(&ll_iso_conn_q);
    ble_ll_isoal_init();
}

void
ble_ll_iso_reset(void)
{
    STAILQ_INIT(&ll_iso_conn_q);
    ble_ll_isoal_reset();
}

int
ble_ll_iso_data_in(struct os_mbuf *om)
{
    struct ble_hci_iso *hci_iso;
    struct ble_hci_iso_data *hci_iso_data;
    struct ble_ll_iso_conn *conn;
    struct ble_mbuf_hdr *blehdr;
    uint16_t data_hdr_len;
    uint16_t handle;
    uint16_t conn_handle;
    uint16_t length;
    uint16_t pb_flag;
    uint16_t ts_flag;
    uint32_t timestamp = 0;

    hci_iso = (void *)om->om_data;

    handle = le16toh(hci_iso->handle);
    conn_handle = BLE_HCI_ISO_CONN_HANDLE(handle);
    pb_flag = BLE_HCI_ISO_PB_FLAG(handle);
    ts_flag = BLE_HCI_ISO_TS_FLAG(handle);
    length = BLE_HCI_ISO_LENGTH(le16toh(hci_iso->length));

    conn = ble_ll_iso_conn_find_by_handle(conn_handle);
    if (!conn) {
        os_mbuf_free_chain(om);
        return BLE_ERR_UNK_CONN_ID;
    }

    data_hdr_len = 0;
    if ((pb_flag == BLE_HCI_ISO_PB_FIRST) ||
        (pb_flag == BLE_HCI_ISO_PB_COMPLETE)) {
        blehdr = BLE_MBUF_HDR_PTR(om);
        blehdr->txiso.packet_seq_num = ++conn->mux.sdu_counter;
        blehdr->txiso.cpu_timestamp = ble_ll_tmr_get();

        if (ts_flag) {
            timestamp = get_le32(om->om_data + sizeof(*hci_iso));
            data_hdr_len += sizeof(uint32_t);
        }
        blehdr->txiso.hci_timestamp = timestamp;

        hci_iso_data = (void *)(om->om_data + sizeof(*hci_iso) + data_hdr_len);
        data_hdr_len += sizeof(*hci_iso_data);
    }
    os_mbuf_adj(om, sizeof(*hci_iso) + data_hdr_len);

    if (OS_MBUF_PKTLEN(om) != length - data_hdr_len) {
        os_mbuf_free_chain(om);
        return BLE_ERR_MEM_CAPACITY;
    }

    switch (pb_flag) {
    case BLE_HCI_ISO_PB_FIRST:
        BLE_LL_ASSERT(!conn->frag);
        conn->frag = om;
        om = NULL;
        break;
    case BLE_HCI_ISO_PB_CONTINUATION:
        BLE_LL_ASSERT(conn->frag);
        os_mbuf_concat(conn->frag, om);
        om = NULL;
        break;
    case BLE_HCI_ISO_PB_COMPLETE:
        BLE_LL_ASSERT(!conn->frag);
        break;
    case BLE_HCI_ISO_PB_LAST:
        BLE_LL_ASSERT(conn->frag);
        os_mbuf_concat(conn->frag, om);
        om = conn->frag;
        conn->frag = NULL;
        break;
    default:
        BLE_LL_ASSERT(0);
        break;
    }

    if (om) {
        ble_ll_isoal_mux_sdu_enqueue(&conn->mux, om);
    }

    return 0;
}

static int
ble_ll_iso_test_pdu_get(struct ble_ll_iso_conn *conn, uint8_t idx, uint32_t pkt_counter, uint8_t *llid, uint8_t *dptr)
{
    uint32_t payload_len;
    uint16_t rem_len;
    uint8_t sdu_idx;
    uint8_t pdu_idx;
    int pdu_len;

    BLE_LL_ASSERT(!conn->mux.framed);

    sdu_idx = idx / conn->mux.pdu_per_sdu;
    pdu_idx = idx - sdu_idx * conn->mux.pdu_per_sdu;

    switch (conn->test_mode.transmit.payload_type) {
    case BLE_HCI_PAYLOAD_TYPE_ZERO_LENGTH:
        *llid = 0b00;
        pdu_len = 0;
        break;
    case BLE_HCI_PAYLOAD_TYPE_VARIABLE_LENGTH:
        payload_len = max(conn->test_mode.transmit.rand + (sdu_idx * pdu_idx), 4);

        rem_len = payload_len - pdu_idx * conn->mux.max_pdu;
        if (rem_len == 0) {
            *llid = 0b01;
            pdu_len = 0;
        } else {
            *llid = rem_len > conn->mux.max_pdu;
            pdu_len = min(conn->mux.max_pdu, rem_len);
        }

        memset(dptr, 0, pdu_len);

        if (payload_len == rem_len) {
            put_le32(dptr, pkt_counter);
        }

        break;
    case BLE_HCI_PAYLOAD_TYPE_MAXIMUM_LENGTH:
        payload_len = conn->max_sdu;

        rem_len = payload_len - pdu_idx * conn->mux.max_pdu;
        if (rem_len == 0) {
            *llid = 0b01;
            pdu_len = 0;
        } else {
            *llid = rem_len > conn->mux.max_pdu;
            pdu_len = min(conn->mux.max_pdu, rem_len);
        }

        memset(dptr, 0, pdu_len);

        if (payload_len == rem_len) {
            put_le32(dptr, pkt_counter);
        }

        break;
    default:
        BLE_LL_ASSERT(0);
    }

    return pdu_len;
}

int
ble_ll_iso_pdu_get(struct ble_ll_iso_conn *conn, uint8_t idx, uint32_t pkt_counter, uint8_t *llid, void *dptr)
{
    if (conn->test_mode.transmit.enabled) {
        return ble_ll_iso_test_pdu_get(conn, idx, pkt_counter, llid, dptr);
    }

    return ble_ll_isoal_mux_pdu_get(&conn->mux, idx, llid, dptr);
}

void
ble_ll_iso_conn_init(struct ble_ll_iso_conn *conn, struct ble_ll_iso_conn_init_param *param)
{
    os_sr_t sr;

    memset(conn, 0, sizeof(*conn));

    conn->handle = param->conn_handle;
    conn->max_sdu = param->max_sdu;

    ble_ll_isoal_mux_init(&conn->mux, param->max_pdu, param->iso_interval_us, param->sdu_interval_us,
                          param->bn, param->pte, BLE_LL_ISOAL_MUX_IS_FRAMED(param->framing),
                          param->framing == BLE_HCI_ISO_FRAMING_FRAMED_UNSEGMENTED);

    OS_ENTER_CRITICAL(sr);
    STAILQ_INSERT_TAIL(&ll_iso_conn_q, conn, iso_conn_q_next);
    OS_EXIT_CRITICAL(sr);
}

void
ble_ll_iso_conn_free(struct ble_ll_iso_conn *conn)
{
    os_sr_t sr;

    OS_ENTER_CRITICAL(sr);
    STAILQ_REMOVE(&ll_iso_conn_q, conn, ble_ll_iso_conn, iso_conn_q_next);
    OS_EXIT_CRITICAL(sr);

    ble_ll_isoal_mux_free(&conn->mux);
}

int
ble_ll_iso_conn_event_start(struct ble_ll_iso_conn *conn, uint32_t timestamp)
{
    if (conn->test_mode.transmit.enabled) {
        conn->test_mode.transmit.rand = ble_ll_rand() % conn->max_sdu;
    }

    ble_ll_isoal_mux_event_start(&conn->mux, timestamp);

    return 0;
}

int
ble_ll_iso_conn_event_done(struct ble_ll_iso_conn *conn)
{
    conn->num_completed_pkt += ble_ll_isoal_mux_event_done(&conn->mux);

    return conn->num_completed_pkt;
}

#endif /* BLE_LL_ISO */

#endif /* ESP_PLATFORM */
