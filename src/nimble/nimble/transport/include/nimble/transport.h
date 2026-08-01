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

#ifndef H_NIMBLE_TRANSPORT_
#define H_NIMBLE_TRANSPORT_

#ifdef __cplusplus
extern "C" {
#endif

#include "nimble/nimble/transport/include/nimble/transport_impl.h"
#include "nimble/nimble/transport/include/nimble/transport/monitor.h"
#include "nimble/porting/nimble/include/os/os_mempool.h"
#if MYNEWT_PKG_apache_mynewt_nimble__nimble_transport_common_hci_ipc
#include "nimble/nimble/transport/include/nimble/transport/transport_ipc.h"
#endif

struct os_mbuf;

/* Initialization */
void ble_transport_init(void);

/* Common HCI RX task functions */
typedef void ble_transport_rx_func_t(void *arg);
void ble_transport_rx_register(ble_transport_rx_func_t *func, void *arg);
void ble_transport_rx(void);

/* Allocators for supported data types */
void *ble_transport_alloc_cmd(void);
void *ble_transport_alloc_evt(int discardable);
struct os_mbuf *ble_transport_alloc_acl_from_hs(void);
struct os_mbuf *ble_transport_alloc_iso_from_hs(void);
struct os_mbuf *ble_transport_alloc_acl_from_ll(void);
struct os_mbuf *ble_transport_alloc_iso_from_ll(void);

/* Generic deallocator for cmd/evt buffers */
void ble_transport_free(void *buf);

/* Register put callback on acl_from_ll mbufs (for ll-hs flow control) */
int ble_transport_register_put_acl_from_ll_cb(os_mempool_put_fn *cb);

/* Send data to hs/ll side */
int ble_transport_to_ll_cmd(void *buf);
int ble_transport_to_ll_acl(struct os_mbuf *om);
int ble_transport_to_ll_iso(struct os_mbuf *om);
int ble_transport_to_hs_evt(void *buf);
int ble_transport_to_hs_acl(struct os_mbuf *om);
int ble_transport_to_hs_iso(struct os_mbuf *om);

#ifdef __cplusplus
}
#endif

#endif /* H_NIMBLE_TRANSPORT_ */
