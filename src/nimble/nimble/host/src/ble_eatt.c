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
#define BLE_NPL_LOG_MODULE BLE_EATT_LOG
#include "nimble/nimble/include/nimble/nimble_npl_log.h"

#if MYNEWT_VAL(BLE_EATT_CHAN_NUM) > 0

#include "os/mynewt.h"
#include "nimble/porting/nimble/include/mem/mem.h"
#include "nimble/nimble/host/include/host/ble_hs_log.h"
#include "ble_att_cmd_priv.h"
#include "ble_hs_priv.h"
#include "ble_l2cap_priv.h"
#include "ble_eatt_priv.h"
#include "nimble/nimble/host/services/gatt/include/services/gatt/ble_svc_gatt.h"

struct ble_eatt {
    SLIST_ENTRY(ble_eatt) next;
    uint16_t conn_handle;
    struct ble_l2cap_chan *chan;
    uint8_t client_op;

    /* Packet transmit queue */
    STAILQ_HEAD(, os_mbuf_pkthdr) eatt_tx_q;

    struct ble_npl_event setup_ev;
    struct ble_npl_event wakeup_ev;
};

SLIST_HEAD(ble_eatt_list, ble_eatt);

static struct ble_eatt_list g_ble_eatt_list;
static ble_eatt_att_rx_fn ble_eatt_att_rx_cb;

#define BLE_EATT_DATABUF_SIZE  ( \
        MYNEWT_VAL(BLE_EATT_MTU) + \
        2 + \
        sizeof (struct os_mbuf_pkthdr) +   \
        sizeof (struct os_mbuf))

#define BLE_EATT_MEMBLOCK_SIZE   \
    (OS_ALIGN(BLE_EATT_DATABUF_SIZE, 4))

#define BLE_EATT_MEMPOOL_SIZE    \
    OS_MEMPOOL_SIZE(MYNEWT_VAL(BLE_EATT_CHAN_NUM) + 1, BLE_EATT_MEMBLOCK_SIZE)
static os_membuf_t ble_eatt_conn_mem[
    OS_MEMPOOL_SIZE(MYNEWT_VAL(BLE_EATT_CHAN_NUM),
    sizeof(struct ble_eatt))
];
static struct os_mempool ble_eatt_conn_pool;
static os_membuf_t ble_eatt_sdu_coc_mem[BLE_EATT_MEMPOOL_SIZE];
struct os_mbuf_pool ble_eatt_sdu_os_mbuf_pool;
static struct os_mempool ble_eatt_sdu_mbuf_mempool;

static struct ble_gap_event_listener ble_eatt_listener;

static struct ble_npl_event g_read_sup_cl_feat_ev;

static void ble_eatt_setup_cb(struct ble_npl_event *ev);
static void ble_eatt_start(uint16_t conn_handle);

static struct ble_eatt *
ble_eatt_find_not_busy(uint16_t conn_handle)
{
    struct ble_eatt *eatt;

    SLIST_FOREACH(eatt, &g_ble_eatt_list, next) {
        if ((eatt->conn_handle == conn_handle) && !eatt->client_op) {
            return eatt;
        }
    }

    return NULL;
}

static struct ble_eatt *
ble_eatt_find_by_conn_handle(uint16_t conn_handle)
{
    struct ble_eatt *eatt;

    SLIST_FOREACH(eatt, &g_ble_eatt_list, next) {
        if (eatt->conn_handle == conn_handle) {
            return eatt;
        }
    }

    return NULL;
}

static struct ble_eatt *
ble_eatt_find_by_conn_handle_and_busy_op(uint16_t conn_handle, uint8_t op)
{
    struct ble_eatt *eatt;

    SLIST_FOREACH(eatt, &g_ble_eatt_list, next) {
        if (eatt->conn_handle == conn_handle && eatt->client_op == op) {
            return eatt;
        }
    }

    return NULL;
}

static struct ble_eatt *
ble_eatt_find(uint16_t conn_handle, uint16_t cid)
{
    struct ble_eatt *eatt;

    SLIST_FOREACH(eatt, &g_ble_eatt_list, next) {
        if ((eatt->conn_handle == conn_handle) &&
            (eatt->chan) &&
            (eatt->chan->scid == cid)) {
            return eatt;
        }
    }

    return NULL;
}

static int
ble_eatt_prepare_rx_sdu(struct ble_l2cap_chan *chan)
{
    int rc;
    struct os_mbuf *om;

    om = os_mbuf_get_pkthdr(&ble_eatt_sdu_os_mbuf_pool, 0);
    if (!om) {
        BLE_EATT_LOG_ERROR("eatt: no memory for sdu\n");
        return BLE_HS_ENOMEM;
    }

    rc = ble_l2cap_recv_ready(chan, om);
    if (rc) {
        BLE_EATT_LOG_ERROR("eatt: Failed to supply RX SDU conn_handle 0x%04x (status=%d)\n",
                            chan->conn_handle, rc);
        os_mbuf_free_chain(om);
    }

    return rc;
}

static void
ble_eatt_wakeup_cb(struct ble_npl_event *ev)
{
    struct ble_eatt *eatt;
    struct os_mbuf *txom;
    struct os_mbuf_pkthdr *omp;
    struct ble_l2cap_chan_info info;

    eatt = ble_npl_event_get_arg(ev);
    assert(eatt);

    omp = STAILQ_FIRST(&eatt->eatt_tx_q);
    if (omp != NULL) {
        STAILQ_REMOVE_HEAD(&eatt->eatt_tx_q, omp_next);

        txom = OS_MBUF_PKTHDR_TO_MBUF(omp);
        ble_l2cap_get_chan_info(eatt->chan, &info);
        ble_eatt_tx(eatt->conn_handle, info.dcid, txom);
    }
}

static struct ble_eatt *
ble_eatt_alloc(void)
{
    struct ble_eatt *eatt;

    eatt = os_memblock_get(&ble_eatt_conn_pool);
    if (!eatt) {
        BLE_EATT_LOG_WARN("eatt: Failed to allocate new eatt context\n");
        return NULL;
    }

    SLIST_INSERT_HEAD(&g_ble_eatt_list, eatt, next);

    eatt->conn_handle = BLE_HS_CONN_HANDLE_NONE;
    eatt->chan = NULL;
    eatt->client_op = 0;

    STAILQ_INIT(&eatt->eatt_tx_q);
    ble_npl_event_init(&eatt->setup_ev, ble_eatt_setup_cb, eatt);
    ble_npl_event_init(&eatt->wakeup_ev, ble_eatt_wakeup_cb, eatt);

    return eatt;
}

static void
ble_eatt_free(struct ble_eatt *eatt)
{
    struct os_mbuf_pkthdr *omp;

    while ((omp = STAILQ_FIRST(&eatt->eatt_tx_q)) != NULL) {
        STAILQ_REMOVE_HEAD(&eatt->eatt_tx_q, omp_next);
        os_mbuf_free_chain(OS_MBUF_PKTHDR_TO_MBUF(omp));
    }

    SLIST_REMOVE(&g_ble_eatt_list, eatt, ble_eatt, next);
    os_memblock_put(&ble_eatt_conn_pool, eatt);
}

static int
ble_eatt_l2cap_event_fn(struct ble_l2cap_event *event, void *arg)
{
    struct ble_eatt *eatt = arg;
    struct ble_gap_conn_desc desc;
    uint8_t opcode;
    int rc;

    switch (event->type) {
    case BLE_L2CAP_EVENT_COC_CONNECTED:
        BLE_EATT_LOG_DEBUG("eatt: Connected \n");
        if (event->connect.status) {
            ble_eatt_free(eatt);
            return 0;
        }
        eatt->chan = event->connect.chan;
        break;
    case BLE_L2CAP_EVENT_COC_DISCONNECTED:
        BLE_EATT_LOG_DEBUG("eatt: Disconnected \n");
        ble_eatt_free(eatt);
        break;
    case BLE_L2CAP_EVENT_COC_ACCEPT:
        BLE_EATT_LOG_DEBUG("eatt: Accept request\n");
        eatt = ble_eatt_find_by_conn_handle(event->accept.conn_handle);
        if (eatt) {
            /* For now we accept only one additional coc channel per ACL
             * TODO: improve it
             */
            return BLE_HS_ENOMEM;
        }

        eatt = ble_eatt_alloc();
        if (!eatt) {
            return BLE_HS_ENOMEM;
        }

        eatt->conn_handle = event->accept.conn_handle;
        event->accept.chan->cb_arg = eatt;

        rc = ble_eatt_prepare_rx_sdu(event->accept.chan);
        if (rc) {
            ble_eatt_free(eatt);
            return rc;
        }

        break;
    case BLE_L2CAP_EVENT_COC_TX_UNSTALLED:
        ble_npl_eventq_put(ble_hs_evq_get(), &eatt->wakeup_ev);
        break;
    case BLE_L2CAP_EVENT_COC_DATA_RECEIVED:
        assert(eatt->chan == event->receive.chan);
        opcode = event->receive.sdu_rx->om_data[0];
        if (ble_eatt_supported_rsp(opcode)) {
            ble_npl_eventq_put(ble_hs_evq_get(), &eatt->wakeup_ev);
        } else if (!ble_eatt_supported_req(opcode)) {
            /* If an ATT PDU is supported on any ATT bearer, then it shall be
             * supported on all supported ATT bearers with the following
             * exceptions:
             *  • The Exchange MTU sub-procedure shall only be supported on the
             *  LE Fixed Channel Unenhanced ATT bearer.
             *  • The Signed Write Without Response sub-procedure shall only be
             *  supported on the LE Fixed Channel Unenhanced ATT bearer.
             */
            ble_l2cap_disconnect(eatt->chan);
            return BLE_HS_EREJECT;
        }

        assert (!ble_gap_conn_find(event->receive.conn_handle, &desc));
        /* As per BLE 5.4 Standard, Vol. 3, Part F, section 5.3.2
         * (ENHANCED ATT BEARER L2CAP INTEROPERABILITY REQUIREMENTS:
         * Channel Requirements):
         * The channel shall be encrypted.
         *
         * Disconnect peer with invalid behavior - ATT PDU received before
         * encryption.
         */
        if (!desc.sec_state.encrypted) {
            ble_l2cap_disconnect(eatt->chan);
            return BLE_HS_EREJECT;
        }

        ble_eatt_att_rx_cb(event->receive.conn_handle, eatt->chan->scid, &event->receive.sdu_rx);
        if (event->receive.sdu_rx) {
            os_mbuf_free_chain(event->receive.sdu_rx);
            event->receive.sdu_rx = NULL;
        }

        /* Receiving L2CAP data is no longer possible, terminate connection */
        rc = ble_eatt_prepare_rx_sdu(event->receive.chan);
        if (rc) {
            ble_l2cap_disconnect(eatt->chan);
            return BLE_HS_ENOMEM;
        }
        break;
    default:
        break;
    }

    return 0;
}

static void
ble_eatt_setup_cb(struct ble_npl_event *ev)
{
    struct ble_eatt *eatt;
    struct os_mbuf *om;
    int rc;

    eatt = ble_npl_event_get_arg(ev);
    assert(eatt);

    om = os_mbuf_get_pkthdr(&ble_eatt_sdu_os_mbuf_pool, 0);
    if (!om) {
        ble_eatt_free(eatt);
        BLE_EATT_LOG_ERROR("eatt: no memory for sdu\n");
        return;
    }

    BLE_EATT_LOG_DEBUG("eatt: connecting eatt on conn_handle 0x%04x\n", eatt->conn_handle);

    rc = ble_l2cap_enhanced_connect(eatt->conn_handle, BLE_EATT_PSM,
                                    MYNEWT_VAL(BLE_EATT_MTU), 1, &om,
                                    ble_eatt_l2cap_event_fn, eatt);
    if (rc) {
        BLE_EATT_LOG_ERROR("eatt: Failed to connect EATT on conn_handle 0x%04x (status=%d)\n",
                            eatt->conn_handle, rc);
        os_mbuf_free_chain(om);
        ble_eatt_free(eatt);
    }
}

static int
ble_gatt_eatt_write_cl_cb(uint16_t conn_handle,
                          const struct ble_gatt_error *error,
                          struct ble_gatt_attr *attr, void *arg)
{
    if (error == NULL || (error->status != 0 && error->status != BLE_HS_EDONE)) {
        BLE_EATT_LOG_DEBUG("eatt: Cannot write to Client Supported features on peer device\n");
        return 0;
    }

    ble_eatt_start(conn_handle);

    return 0;
}

static int
ble_gatt_eatt_read_cl_uuid_cb(uint16_t conn_handle,
                              const struct ble_gatt_error *error,
                              struct ble_gatt_attr *attr, void *arg)
{
    uint8_t client_supported_feat;
    int rc;

    if (error == NULL || (error->status != 0 && error->status != BLE_HS_EDONE)) {
        BLE_EATT_LOG_DEBUG("eatt: Cannot find Client Supported features on peer device\n");
        return BLE_HS_EDONE;
    }

    if (attr == NULL) {
        BLE_EATT_LOG_ERROR("eatt: Invalid attribute \n");
        return BLE_HS_EDONE;
    }

    if (error->status == 0) {
        client_supported_feat = ble_svc_gatt_get_local_cl_supported_feat();
        rc = ble_gattc_write_flat(conn_handle, attr->handle, &client_supported_feat, 1,
                                  ble_gatt_eatt_write_cl_cb, NULL);
        BLE_EATT_LOG_DEBUG("eatt: %s , write rc = %d \n", __func__, rc);
        assert(rc == 0);
        return 0;
    }

    return BLE_HS_EDONE;
}

static void
ble_gatt_eatt_read_cl_uuid(struct ble_npl_event *ev)
{
    uint16_t conn_handle;

    conn_handle = POINTER_TO_UINT(ble_npl_event_get_arg(ev));

    ble_gattc_read_by_uuid(conn_handle, 1, 0xffff,
                           BLE_UUID16_DECLARE(BLE_SVC_GATT_CHR_CLIENT_SUPPORTED_FEAT_UUID16),
                           ble_gatt_eatt_read_cl_uuid_cb, NULL);
}

static int
ble_eatt_gap_event(struct ble_gap_event *event, void *arg)
{
    struct ble_eatt *eatt;

    switch (event->type) {
    case BLE_GAP_EVENT_ENC_CHANGE:
        if (event->enc_change.status != 0) {
            return 0;
        }

        /* don't try to connect if already connected */
        if (ble_eatt_find_by_conn_handle(event->enc_change.conn_handle)) {
            return 0;
        }

        BLE_EATT_LOG_DEBUG("eatt: Encryption enabled, connecting EATT (conn_handle=0x%04x)\n",
                            event->enc_change.conn_handle);

        ble_npl_event_set_arg(&g_read_sup_cl_feat_ev, UINT_TO_POINTER(event->enc_change.conn_handle));
        ble_npl_eventq_put(ble_hs_evq_get(), &g_read_sup_cl_feat_ev);

        break;
    case BLE_GAP_EVENT_DISCONNECT:
        eatt = ble_eatt_find_by_conn_handle(event->disconnect.conn.conn_handle);
        assert(eatt == NULL);
        break;
    default:
        break;
    }

    return 0;
}

uint16_t
ble_eatt_get_available_chan_cid(uint16_t conn_handle, uint8_t op)
{
    struct ble_eatt * eatt;

    eatt = ble_eatt_find_not_busy(conn_handle);
    if (!eatt) {
        return BLE_L2CAP_CID_ATT;
    }

    eatt->client_op = op;

    return eatt->chan->scid;
}

void
ble_eatt_release_chan(uint16_t conn_handle, uint8_t op)
{
    struct ble_eatt * eatt;

    eatt = ble_eatt_find_by_conn_handle_and_busy_op(conn_handle, op);
    if (!eatt) {
        BLE_EATT_LOG_WARN("ble_eatt_release_chan:"
                          "EATT not found for conn_handle 0x%04x, operation 0x%02\n", conn_handle, op);
        return;
    }

    eatt->client_op = 0;
}

int
ble_eatt_tx(uint16_t conn_handle, uint16_t cid, struct os_mbuf *txom)
{
    struct ble_eatt *eatt;
    int rc;

    BLE_EATT_LOG_DEBUG("eatt: %s, size %d ", __func__, OS_MBUF_PKTLEN(txom));
    eatt = ble_eatt_find(conn_handle, cid);
    if (!eatt || !eatt->chan) {
        BLE_EATT_LOG_ERROR("Eatt not available");
        rc = BLE_HS_ENOENT;
        goto error;
    }

    rc = ble_l2cap_send(eatt->chan, txom);
    if (rc == 0) {
        goto done;
    }

    if (rc == BLE_HS_ESTALLED) {
        BLE_EATT_LOG_DEBUG("ble_eatt_tx: Eatt stalled");
    } else if (rc == BLE_HS_EBUSY) {
        BLE_EATT_LOG_DEBUG("ble_eatt_tx: Message queued");
        STAILQ_INSERT_HEAD(&eatt->eatt_tx_q, OS_MBUF_PKTHDR(txom), omp_next);
        ble_npl_eventq_put(ble_hs_evq_get(), &eatt->wakeup_ev);
    } else {
        BLE_EATT_LOG_ERROR("eatt: %s, ERROR %d ", __func__, rc);
        assert(0);
    }
done:
    return 0;

error:
    os_mbuf_free_chain(txom);

    return rc;
}

static void
ble_eatt_start(uint16_t conn_handle)
{
    struct ble_gap_conn_desc desc;
    struct ble_eatt *eatt;
    int rc;

    rc = ble_gap_conn_find(conn_handle, &desc);
    assert(rc == 0);
    if (desc.role != BLE_GAP_ROLE_MASTER) {
        /* Let master to create ecoc.
         * TODO: Slave could setup after some timeout
         */
        return;
    }

    eatt = ble_eatt_alloc();
    if (!eatt) {
        return;
    }

    eatt->conn_handle = conn_handle;

    /* Setup EATT  */
    ble_npl_eventq_put(ble_hs_evq_get(), &eatt->setup_ev);
}

void
ble_eatt_init(ble_eatt_att_rx_fn att_rx_cb)
{
    int rc;

    rc = mem_init_mbuf_pool(ble_eatt_sdu_coc_mem,
                            &ble_eatt_sdu_mbuf_mempool,
                            &ble_eatt_sdu_os_mbuf_pool,
                            MYNEWT_VAL(BLE_EATT_CHAN_NUM) + 1,
                            BLE_EATT_MEMBLOCK_SIZE,
                            "ble_eatt_sdu");
    BLE_HS_DBG_ASSERT_EVAL(rc == 0);

    rc = os_mempool_init(&ble_eatt_conn_pool, MYNEWT_VAL(BLE_EATT_CHAN_NUM),
                         sizeof (struct ble_eatt),
                         ble_eatt_conn_mem, "ble_eatt_conn_pool");
    BLE_HS_DBG_ASSERT_EVAL(rc == 0);

    ble_gap_event_listener_register(&ble_eatt_listener, ble_eatt_gap_event, NULL);
    ble_l2cap_create_server(BLE_EATT_PSM, MYNEWT_VAL(BLE_EATT_MTU), ble_eatt_l2cap_event_fn, NULL);

    ble_npl_event_init(&g_read_sup_cl_feat_ev, ble_gatt_eatt_read_cl_uuid, NULL);

    ble_eatt_att_rx_cb = att_rx_cb;
}
#endif
