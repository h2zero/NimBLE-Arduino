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

#ifndef H_BLE_ISO_PRIV_
#define H_BLE_ISO_PRIV_

#include "nimble/nimble/include/nimble/hci_common.h"
#ifdef __cplusplus
extern "C" {
#endif

void
ble_iso_rx_create_big_complete(const struct ble_hci_ev_le_subev_create_big_complete *ev);

void
ble_iso_rx_terminate_big_complete(const struct ble_hci_ev_le_subev_terminate_big_complete *ev);

void
ble_iso_rx_big_sync_established(const struct ble_hci_ev_le_subev_big_sync_established *ev);

void
ble_iso_rx_big_sync_lost(const struct ble_hci_ev_le_subev_big_sync_lost *ev);

int
ble_iso_rx_data(struct os_mbuf *om, void *arg);

#ifdef __cplusplus
}
#endif

#endif /* H_BLE_ISO_PRIV_ */
