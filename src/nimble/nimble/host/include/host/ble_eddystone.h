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

#ifndef H_BLE_EDDYSTONE_
#define H_BLE_EDDYSTONE_

/**
 * @brief Eddystone - BLE beacon from Google
 * @defgroup bt_eddystone Eddystone - BLE beacon from Google
 * @ingroup bt_host
 * @{
 */

#include <inttypes.h>
#ifdef __cplusplus
extern "C" {
#endif

struct ble_hs_adv_fields;

/**
 * @defgroup ble_eddystone Eddystone Constants
 * @ingroup bt_host
 * @{
 */

/** Maximum number of 16-bit UUIDs in Eddystone advertisement data. */
#define BLE_EDDYSTONE_MAX_UUIDS16           3

/** Maximum length of Eddystone URL. */
#define BLE_EDDYSTONE_URL_MAX_LEN           17


/** Eddystone URL Scheme: "http://www." prefix. */
#define BLE_EDDYSTONE_URL_SCHEME_HTTP_WWW   0

/** Eddystone URL Scheme: "https://www." prefix. */
#define BLE_EDDYSTONE_URL_SCHEME_HTTPS_WWW  1

/** Eddystone URL Scheme: "http://" prefix. */
#define BLE_EDDYSTONE_URL_SCHEME_HTTP       2

/** Eddystone URL Scheme: "https://" prefix. */
#define BLE_EDDYSTONE_URL_SCHEME_HTTPS      3


/** Eddystone URL Suffix: ".com/". */
#define BLE_EDDYSTONE_URL_SUFFIX_COM_SLASH  0x00

/** Eddystone URL Suffix: ".org/". */
#define BLE_EDDYSTONE_URL_SUFFIX_ORG_SLASH  0x01

/** Eddystone URL Suffix: ".edu/". */
#define BLE_EDDYSTONE_URL_SUFFIX_EDU_SLASH  0x02

/** Eddystone URL Suffix: ".net/". */
#define BLE_EDDYSTONE_URL_SUFFIX_NET_SLASH  0x03

/** Eddystone URL Suffix: ".info/". */
#define BLE_EDDYSTONE_URL_SUFFIX_INFO_SLASH 0x04

/** Eddystone URL Suffix: ".biz/". */
#define BLE_EDDYSTONE_URL_SUFFIX_BIZ_SLASH  0x05

/** Eddystone URL Suffix: ".gov/". */
#define BLE_EDDYSTONE_URL_SUFFIX_GOV_SLASH  0x06

/** Eddystone URL Suffix: ".com". */
#define BLE_EDDYSTONE_URL_SUFFIX_COM        0x07

/** Eddystone URL Suffix: ".org". */
#define BLE_EDDYSTONE_URL_SUFFIX_ORG        0x08

/** Eddystone URL Suffix: ".edu". */
#define BLE_EDDYSTONE_URL_SUFFIX_EDU        0x09

/** Eddystone URL Suffix: ".net". */
#define BLE_EDDYSTONE_URL_SUFFIX_NET        0x0a

/** Eddystone URL Suffix: ".info". */
#define BLE_EDDYSTONE_URL_SUFFIX_INFO       0x0b

/** Eddystone URL Suffix: ".biz". */
#define BLE_EDDYSTONE_URL_SUFFIX_BIZ        0x0c

/** Eddystone URL Suffix: ".gov". */
#define BLE_EDDYSTONE_URL_SUFFIX_GOV        0x0d

/** Eddystone URL Suffix: None. */
#define BLE_EDDYSTONE_URL_SUFFIX_NONE       0xff

/** @} */

/**
 * Configures the device to advertise Eddystone UID beacons.
 *
 * @param adv_fields            The base advertisement fields to transform into
 *                                  an eddystone beacon.  All configured fields
 *                                  are preserved; you probably want to clear
 *                                  this struct before calling this function.
 * @param uid                   The 16-byte UID to advertise.
 * @param measured_power        The Measured Power (RSSI value at 0 Meter).
 *
 * @return                      0 on success;
 *                              BLE_HS_EBUSY if advertising is in progress;
 *                              BLE_HS_EMSGSIZE if the specified data is too
 *                                  large to fit in an advertisement;
 *                              Other nonzero on failure.
 */
int ble_eddystone_set_adv_data_uid(struct ble_hs_adv_fields *adv_fields,
                                   void *uid, int8_t measured_power);

/**
 * Configures the device to advertise Eddystone URL beacons.
 *
 * @param adv_fields            The base advertisement fields to transform into
 *                                  an eddystone beacon.  All configured fields
 *                                  are preserved; you probably want to clear
 *                                  this struct before calling this function.
 * @param url_scheme            The prefix of the URL; one of the
 *                                  BLE_EDDYSTONE_URL_SCHEME values.
 * @param url_body              The middle of the URL.  Don't include the
 *                                  suffix if there is a suitable suffix code.
 * @param url_body_len          The string length of the url_body argument.
 * @param url_suffix            The suffix of the URL; one of the
 *                                  BLE_EDDYSTONE_URL_SUFFIX values; use
 *                                  BLE_EDDYSTONE_URL_SUFFIX_NONE if the suffix
 *                                  is embedded in the body argument.
 * @param measured_power        The Measured Power (RSSI value at 0 Meter).
 *
 * @return                      0 on success;
 *                              BLE_HS_EBUSY if advertising is in progress;
 *                              BLE_HS_EMSGSIZE if the specified data is too
 *                                  large to fit in an advertisement;
 *                              Other nonzero on failure.
 */
int ble_eddystone_set_adv_data_url(struct ble_hs_adv_fields *adv_fields,
                                   uint8_t url_scheme, char *url_body,
                                   uint8_t url_body_len, uint8_t url_suffix,
                                   int8_t measured_power);

#ifdef __cplusplus
}
#endif

/**
 * @}
 */

#endif
