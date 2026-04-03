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

#ifndef H_BLE_LL_ISO
#define H_BLE_LL_ISO

#include <stdint.h>
#include "nimble/nimble/controller/include/controller/ble_ll_isoal.h"

#ifdef __cplusplus
extern "C" {
#endif

struct ble_ll_iso_data_path {
    uint8_t data_path_id;
    uint8_t enabled : 1;
};
struct ble_ll_iso_test_mode {
    struct {
        uint32_t rand;
        uint8_t payload_type;
        uint8_t enabled : 1;
    } transmit;
};
struct ble_ll_iso_conn {
    /* Connection handle */
    uint16_t handle;

    /* Maximum SDU size */
    uint16_t max_sdu;

    /* ISO Data Path */
    struct ble_ll_iso_data_path data_path;

    /* ISO Test Mode */
    struct ble_ll_iso_test_mode test_mode;

    /* ISOAL Multiplexer */
    struct ble_ll_isoal_mux mux;

    /* HCI SDU Fragment */
    struct os_mbuf *frag;

    /* Number of Completed Packets */
    uint16_t num_completed_pkt;

    STAILQ_ENTRY(ble_ll_iso_conn) iso_conn_q_next;
};

/* HCI command handlers */
int ble_ll_iso_read_tx_sync(const uint8_t *cmdbuf, uint8_t len, uint8_t *rspbuf, uint8_t *rsplen);
int ble_ll_iso_set_cig_param(const uint8_t *cmdbuf, uint8_t len, uint8_t *rspbuf, uint8_t *rsplen);
int ble_ll_iso_set_cig_param_test(const uint8_t *cmdbuf, uint8_t len, uint8_t *rspbuf, uint8_t *rsplen);
int ble_ll_iso_create_cis(const uint8_t *cmdbuf, uint8_t len);
int ble_ll_iso_remove_cig(const uint8_t *cmdbuf, uint8_t len, uint8_t *rspbuf, uint8_t *rsplen);
int ble_ll_iso_accept_cis_req(const uint8_t *cmdbuf, uint8_t len);
int ble_ll_iso_reject_cis_req(const uint8_t *cmdbuf, uint8_t len);
int ble_ll_iso_create_big(const uint8_t *cmdbuf, uint8_t len);
int ble_ll_iso_create_big_test(const uint8_t *cmdbuf, uint8_t len);
int ble_ll_iso_terminate_big(const uint8_t *cmdbuf, uint8_t len);
int ble_ll_iso_big_create_sync(const uint8_t *cmdbuf, uint8_t len);
int ble_ll_iso_big_terminate_sync(const uint8_t *cmdbuf, uint8_t len);
int ble_ll_iso_setup_iso_data_path(const uint8_t *cmdbuf, uint8_t len, uint8_t *rspbuf, uint8_t *rsplen);
int ble_ll_iso_remove_iso_data_path(const uint8_t *cmdbuf, uint8_t len, uint8_t *rspbuf, uint8_t *rsplen);
int ble_ll_iso_transmit_test(const uint8_t *cmdbuf, uint8_t len, uint8_t *rspbuf, uint8_t *rsplen);
int ble_ll_iso_receive_test(const uint8_t *cmdbuf, uint8_t len);
int ble_ll_iso_read_counters_test(const uint8_t *cmdbuf, uint8_t len);
int ble_ll_iso_end_test(const uint8_t *cmdbuf, uint8_t len, uint8_t *rspbuf, uint8_t *rsplen);

void ble_ll_iso_init(void);
void ble_ll_iso_reset(void);

/* ISO Data handler */
int ble_ll_iso_data_in(struct os_mbuf *om);

int ble_ll_iso_pdu_get(struct ble_ll_iso_conn *conn, uint8_t idx, uint32_t pkt_counter, uint8_t *llid, void *dptr);

struct ble_ll_iso_conn_init_param {
    uint32_t iso_interval_us;
    uint32_t sdu_interval_us;
    uint16_t conn_handle;
    uint16_t max_sdu;
    uint8_t max_pdu;
    uint8_t framing;
    uint8_t pte;
    uint8_t bn;
};

void ble_ll_iso_conn_init(struct ble_ll_iso_conn *conn, struct ble_ll_iso_conn_init_param *param);
void ble_ll_iso_conn_free(struct ble_ll_iso_conn *conn);

int ble_ll_iso_conn_event_start(struct ble_ll_iso_conn *conn, uint32_t timestamp);
int ble_ll_iso_conn_event_done(struct ble_ll_iso_conn *conn);

struct ble_ll_iso_conn *ble_ll_iso_conn_find_by_handle(uint16_t conn_handle);

#ifdef __cplusplus
}
#endif

#endif
