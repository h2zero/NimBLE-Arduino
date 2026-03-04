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

/* All Channel Sounding APIs are experimental and subject to change at any time */

#ifndef H_BLE_LL_CS
#define H_BLE_LL_CS

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* HCI handlers */
int ble_ll_cs_hci_rd_loc_supp_cap(uint8_t *rspbuf, uint8_t *rsplen);
int ble_ll_cs_hci_rd_rem_supp_cap(const uint8_t *cmdbuf, uint8_t cmdlen);
int ble_ll_cs_hci_wr_cached_rem_supp_cap(const uint8_t *cmdbuf, uint8_t cmdlen, uint8_t *rspbuf, uint8_t *rsplen);
int ble_ll_cs_hci_sec_enable(const uint8_t *cmdbuf, uint8_t cmdlen);
int ble_ll_cs_hci_set_def_settings(const uint8_t *cmdbuf, uint8_t cmdlen, uint8_t *rspbuf, uint8_t *rsplen);
int ble_ll_cs_hci_rd_rem_fae(const uint8_t *cmdbuf, uint8_t cmdlen);
int ble_ll_cs_hci_wr_cached_rem_fae(const uint8_t *cmdbuf, uint8_t cmdlen, uint8_t *rspbuf, uint8_t *rsplen);
int ble_ll_cs_hci_create_config(const uint8_t *cmdbuf, uint8_t cmdlen);
int ble_ll_cs_hci_remove_config(const uint8_t *cmdbuf, uint8_t cmdlen);
int ble_ll_cs_hci_set_chan_class(const uint8_t *cmdbuf, uint8_t cmdlen);
int ble_ll_cs_hci_set_proc_params(const uint8_t *cmdbuf, uint8_t cmdlen, uint8_t *rspbuf, uint8_t *rsplen);
int ble_ll_cs_hci_proc_enable(const uint8_t *cmdbuf, uint8_t cmdlen);
int ble_ll_cs_hci_test(const uint8_t *cmdbuf, uint8_t cmdlen, uint8_t *rspbuf, uint8_t *rsplen);
int ble_ll_cs_hci_test_end(void);

#ifdef __cplusplus
}
#endif

#endif
