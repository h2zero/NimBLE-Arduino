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

#ifndef _NIMBLE_NPL_LOG_H_
#define _NIMBLE_NPL_LOG_H_

#ifdef __cplusplus
extern "C" {
#endif

#ifndef BLE_NPL_LOG_MODULE
#error "NPL Logging module not specified"
#endif

/* helper macros */
#define _BLE_NPL_LOG_CAT(a, ...) _BLE_NPL_LOG_PRIMITIVE_CAT(a, __VA_ARGS__)
#define _BLE_NPL_LOG_PRIMITIVE_CAT(a, ...) a ## __VA_ARGS__

/* used to generate proper log function call (used by stack) */
#define BLE_NPL_LOG(lvl, ...) _BLE_NPL_LOG_CAT(BLE_NPL_LOG_MODULE, _BLE_NPL_LOG_CAT(_, lvl))(__VA_ARGS__)

/* Include OS-specific LOG implementation, should provide implementation for
 * logging macros used by NiBLE (eg via modlog) or BLE_NPL_LOG_IMPL
 * macro implementation that generates required logging functions/macros
 */
#include "nimble/porting/npl/freertos/include/nimble/nimble_npl_os_log.h"

/* generate logging functions for modules, can be macro or function  */
#ifdef BLE_NPL_LOG_IMPL
BLE_NPL_LOG_IMPL(DEBUG);
BLE_NPL_LOG_IMPL(INFO);
BLE_NPL_LOG_IMPL(WARN);
BLE_NPL_LOG_IMPL(ERROR);
BLE_NPL_LOG_IMPL(CRITICAL);
#endif

#ifdef __cplusplus
}
#endif

#endif  /* _NIMBLE_NPL_LOG_H_ */
