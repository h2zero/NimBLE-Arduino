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

#ifndef H_BLE_CS_PRIV_
#define H_BLE_CS_PRIV_

int ble_hs_hci_evt_le_cs_rd_rem_supp_cap_complete(uint8_t subevent, const void *data, unsigned int len);
int ble_hs_hci_evt_le_cs_rd_rem_fae_complete(uint8_t subevent, const void *data, unsigned int len);
int ble_hs_hci_evt_le_cs_sec_enable_complete(uint8_t subevent, const void *data, unsigned int len);
int ble_hs_hci_evt_le_cs_config_complete(uint8_t subevent, const void *data, unsigned int len);
int ble_hs_hci_evt_le_cs_proc_enable_complete(uint8_t subevent, const void *data, unsigned int len);
int ble_hs_hci_evt_le_cs_subevent_result(uint8_t subevent, const void *data, unsigned int len);
int ble_hs_hci_evt_le_cs_subevent_result_continue(uint8_t subevent, const void *data, unsigned int len);
int ble_hs_hci_evt_le_cs_test_end_complete(uint8_t subevent, const void *data, unsigned int len);
#endif
