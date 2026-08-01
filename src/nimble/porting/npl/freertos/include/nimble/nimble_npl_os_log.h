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

#ifndef _NIMBLE_NPL_OS_LOG_H_
#define _NIMBLE_NPL_OS_LOG_H_

#ifdef ESP_PLATFORM
#include <stdarg.h>
#include "esp_log.h"
#include "nimble/porting/nimble/include/log_common/log_common.h"

/* Default log level (can be overridden via kconfig) */
#ifndef BLE_HS_LOG_LVL
#define BLE_HS_LOG_LVL LOG_LEVEL_INFO
#endif

/* Map NimBLE log levels to ESP-IDF log levels */
#define _BLE_TO_ESP_LEVEL(lvl) _BLE_TO_ESP_LEVEL_##lvl
#define _BLE_TO_ESP_LEVEL_DEBUG    ESP_LOG_DEBUG
#define _BLE_TO_ESP_LEVEL_INFO     ESP_LOG_INFO
#define _BLE_TO_ESP_LEVEL_WARN     ESP_LOG_WARN
#define _BLE_TO_ESP_LEVEL_ERROR    ESP_LOG_ERROR
#define _BLE_TO_ESP_LEVEL_CRITICAL ESP_LOG_ERROR

/* Map log level names to numeric values */
#define _BLE_LOG_LEVEL_VALUE(lvl) _BLE_LOG_LEVEL_VALUE_##lvl
#define _BLE_LOG_LEVEL_VALUE_DEBUG    LOG_LEVEL_DEBUG
#define _BLE_LOG_LEVEL_VALUE_INFO     LOG_LEVEL_INFO
#define _BLE_LOG_LEVEL_VALUE_WARN     LOG_LEVEL_WARN
#define _BLE_LOG_LEVEL_VALUE_ERROR    LOG_LEVEL_ERROR
#define _BLE_LOG_LEVEL_VALUE_CRITICAL LOG_LEVEL_CRITICAL

/* Generate module logging functions with compile-time filtering */
#define BLE_NPL_LOG_IMPL(lvl)                                                 \
    static inline void _BLE_NPL_LOG_CAT(                                      \
        BLE_NPL_LOG_MODULE, _BLE_NPL_LOG_CAT(_, lvl))(const char *fmt, ...)   \
    {                                                                         \
        if (BLE_HS_LOG_LVL <= _BLE_LOG_LEVEL_VALUE(lvl)) {                    \
            va_list args;                                                     \
            va_start(args, fmt);                                              \
            esp_log_writev(_BLE_TO_ESP_LEVEL(lvl), "NimBLE", fmt, args);      \
            va_end(args);                                                     \
        }                                                                     \
    }

# else
#define BLE_NPL_LOG_IMPL(lvl) \
    static inline void _BLE_NPL_LOG_CAT(BLE_NPL_LOG_MODULE, _BLE_NPL_LOG_CAT(_, lvl))(const char *fmt, ...) { }
#endif
#endif  /* _NIMBLE_NPL_OS_LOG_H_ */
