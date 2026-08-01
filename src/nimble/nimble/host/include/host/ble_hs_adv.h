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

#ifndef H_BLE_HS_ADV_
#define H_BLE_HS_ADV_

/**
 * @brief Bluetooth Host Advertising API
 * @defgroup bt_adv Bluetooth Host Advertising API
 * @ingroup bt_host
 * @{
 */

#include <inttypes.h>
#include "nimble/nimble/host/include/host/ble_uuid.h"

#ifdef __cplusplus
extern "C" {
#endif

/** Max Advertising Data Size. */
#define BLE_HS_ADV_MAX_SZ           BLE_HCI_MAX_ADV_DATA_LEN

/** Max field payload size (account for 2-byte header). */
#define BLE_HS_ADV_MAX_FIELD_SZ     (BLE_HS_ADV_MAX_SZ - 2)

/** Represents advertising data packet in BLE advertisement or scan response. */
struct ble_hs_adv_field {
    /** Length of the advertising data (type and value). */
    uint8_t length;

    /** Type of the advertising data field. */
    uint8_t type;

    /** Value of the advertising data field. */
    uint8_t value[0];
};

/** Function pointer typedef for parsing an advertising data field. */
typedef int (* ble_hs_adv_parse_func_t) (const struct ble_hs_adv_field *,
                                         void *);

/** Advertising Data Fields. */
struct ble_hs_adv_fields {
    /** 0x01 - Flags. */
    uint8_t flags;

    /** 0x02,0x03 - 16-bit service class UUIDs. */
    const ble_uuid16_t *uuids16;

    /** Number of 16-bit UUIDs. */
    uint8_t num_uuids16;

    /** Indicates if the list of 16-bit UUIDs is complete. */
    unsigned uuids16_is_complete:1;


    /** 0x04,0x05 - 32-bit service class UUIDs. */
    const ble_uuid32_t *uuids32;

    /** Number of 32-bit UUIDs. */
    uint8_t num_uuids32;

    /** Indicates if the list of 32-bit UUIDs is complete. */
    unsigned uuids32_is_complete:1;


    /** 0x06,0x07 - 128-bit service class UUIDs. */
    const ble_uuid128_t *uuids128;

    /** Number of 128-bit UUIDs. */
    uint8_t num_uuids128;

    /** Indicates if the list of 128-bit UUIDs is complete. */
    unsigned uuids128_is_complete:1;


    /** 0x08,0x09 - Local name. */
    const uint8_t *name;

    /** Length of the local name. */
    uint8_t name_len;

    /** Indicates if the list of local names if complete. */
    unsigned name_is_complete:1;


    /** 0x0a - Tx power level. */
    int8_t tx_pwr_lvl;

    /** Indicates if Tx power level is present. */
    unsigned tx_pwr_lvl_is_present:1;


    /** 0x12 - Slave connection interval range. */
    const uint8_t *slave_itvl_range;

    /** 0x16 - Service data - 16-bit UUID. */
    const uint8_t *svc_data_uuid16;

    /** Length of the service data with 16-bit UUID. */
    uint8_t svc_data_uuid16_len;


    /** 0x17 - Public target address. */
    const uint8_t *public_tgt_addr;

    /** Number of public target addresses. */
    uint8_t num_public_tgt_addrs;

    /** 0x19 - Appearance. */
    uint16_t appearance;

    /** Indicates if Appearance is present. */
    unsigned appearance_is_present:1;


    /** 0x1a - Advertising interval. */
    uint16_t adv_itvl;

    /** Indicates if advertising interval is present. */
    unsigned adv_itvl_is_present:1;


    /** 0x20 - Service data - 32-bit UUID. */
    const uint8_t *svc_data_uuid32;

    /** Length of the service data with 32-bit UUID. */
    uint8_t svc_data_uuid32_len;


    /** 0x21 - Service data - 128-bit UUID. */
    const uint8_t *svc_data_uuid128;

    /** Length of service data with 128-bit UUID. */
    uint8_t svc_data_uuid128_len;


    /** 0x24 - URI. */
    const uint8_t *uri;

    /** Length of the URI. */
    uint8_t uri_len;


    /** 0xff - Manufacturer specific data. */
    const uint8_t *mfg_data;

    /** Length of manufacturer specific data. */
    uint8_t mfg_data_len;

    /** 0x30 - Broadcast name. */
    const uint8_t *broadcast_name;

    /** Length of the Broadcast name. */
    uint8_t broadcast_name_len;
};

/**
 * @defgroup ble_hs_adv_types BLE Advertising Common Data Types
 * @{
 */

/** Common Data Type: Flags. */
#define BLE_HS_ADV_TYPE_FLAGS                   0x01

/** Common Data Type: Incomplete List of 16-bit Service Class UUIDs. */
#define BLE_HS_ADV_TYPE_INCOMP_UUIDS16          0x02

/** Common Data Type: Complete List of 16-bit Service Class UUIDs. */
#define BLE_HS_ADV_TYPE_COMP_UUIDS16            0x03

/** Common Data Type: Incomplete List of 32-bit Service Class UUIDs. */
#define BLE_HS_ADV_TYPE_INCOMP_UUIDS32          0x04

/** Common Data Type: Complete List of 32-bit Service Class UUIDs. */
#define BLE_HS_ADV_TYPE_COMP_UUIDS32            0x05

/** Common Data Type: Incomplete List of 128-bit Service Class UUIDs. */
#define BLE_HS_ADV_TYPE_INCOMP_UUIDS128         0x06

/** Common Data Type: Complete List of 128-bit Service Class UUIDs. */
#define BLE_HS_ADV_TYPE_COMP_UUIDS128           0x07

/** Common Data Type: Shortened Local Name. */
#define BLE_HS_ADV_TYPE_INCOMP_NAME             0x08

/** Common Data Type: Complete Local Name. */
#define BLE_HS_ADV_TYPE_COMP_NAME               0x09

/** Common Data Type: Tx Power Level. */
#define BLE_HS_ADV_TYPE_TX_PWR_LVL              0x0a

/** Common Data Type: Peripheral Connection Interval Range. */
#define BLE_HS_ADV_TYPE_SLAVE_ITVL_RANGE        0x12

/** Common Data Type: List of 16-bit Service Solicitation UUIDs. */
#define BLE_HS_ADV_TYPE_SOL_UUIDS16             0x14

/** Common Data Type: List of 128-bit Service Solicitation UUIDs. */
#define BLE_HS_ADV_TYPE_SOL_UUIDS128            0x15

/** Common Data Type: Service Data - 16-bit UUID. */
#define BLE_HS_ADV_TYPE_SVC_DATA_UUID16         0x16

/** Common Data Type: Public Target Address. */
#define BLE_HS_ADV_TYPE_PUBLIC_TGT_ADDR         0x17

/** Common Data Type: Random Target Address. */
#define BLE_HS_ADV_TYPE_RANDOM_TGT_ADDR         0x18

/** Common Data Type: Appearance. */
#define BLE_HS_ADV_TYPE_APPEARANCE              0x19

/** Common Data Type: Advertising Interval. */
#define BLE_HS_ADV_TYPE_ADV_ITVL                0x1a

/** Common Data Type: Service Data - 32-bit UUID. */
#define BLE_HS_ADV_TYPE_SVC_DATA_UUID32         0x20

/** Common Data Type: Service Data - 128-bit UUID. */
#define BLE_HS_ADV_TYPE_SVC_DATA_UUID128        0x21

/** Common Data Type: URI. */
#define BLE_HS_ADV_TYPE_URI                     0x24

/** Common Data Type: PB-ADV. */
#define BLE_HS_ADV_TYPE_MESH_PROV               0x29

/** Common Data Type: Mesh Message. */
#define BLE_HS_ADV_TYPE_MESH_MESSAGE            0x2a

/** Common Data Type: Mesh Beacon. */
#define BLE_HS_ADV_TYPE_MESH_BEACON             0x2b

/** Common Data Type: Broadcast Name. */
#define BLE_HS_ADV_TYPE_BROADCAST_NAME          0x30

/** Common Data Type: Manufacturer Specific Data. */
#define BLE_HS_ADV_TYPE_MFG_DATA                0xff

/**
 * @}
 */

/**
 * @defgroup ble_hs_adv_flags BLE Advertising Flags
 * @{
 */
/** Length of BLE Advertising Flags field. */
#define BLE_HS_ADV_FLAGS_LEN                    1

/** Limited Discoverable Mode Flag. */
#define BLE_HS_ADV_F_DISC_LTD                   0x01

/** General Discoverable Mode Flag. */
#define BLE_HS_ADV_F_DISC_GEN                   0x02

/** BR/EDR Not Supported Flag. */
#define BLE_HS_ADV_F_BREDR_UNSUP                0x04

/** @} */

/**
 * @defgroup ble_hs_adv_misc BLE Advertising Miscellaneous
 * @{
 */
/** Length of BLE advertising transmit power level field. */
#define BLE_HS_ADV_TX_PWR_LVL_LEN               1

/**
 * Set the tx_pwr_lvl field to this if you want the stack to fill in the tx
 * power level field.
 */
#define BLE_HS_ADV_TX_PWR_LVL_AUTO              (-128)

/** Length of the Peripheral Connection Interval Range field. */
#define BLE_HS_ADV_SLAVE_ITVL_RANGE_LEN         4

/** Minimum length of the Service Data - 16-bit UUID field. */
#define BLE_HS_ADV_SVC_DATA_UUID16_MIN_LEN      2

/** Length of a Public Target Address entry. */
#define BLE_HS_ADV_PUBLIC_TGT_ADDR_ENTRY_LEN    6

/** Length of the Appearance field. */
#define BLE_HS_ADV_APPEARANCE_LEN               2

/** Length of the Advertising Interval field. */
#define BLE_HS_ADV_ADV_ITVL_LEN                 2

/** Minimum length of the Service Data - 32-bit UUID field. */
#define BLE_HS_ADV_SVC_DATA_UUID32_MIN_LEN      4

/** Minimum length of the Service Data - 128-bit UUID field. */
#define BLE_HS_ADV_SVC_DATA_UUID128_MIN_LEN     16

/**
 * @}
 */

/**
 * Set the advertising data fields in an os_mbuf.
 *
 * @param adv_fields     Pointer to the advertising data structure.
 * @param om             Pointer to the memory buffer that will be written with
 *                       advertising data.
 *
 * @return               0 on success; non-zero on failure.
 */
int ble_hs_adv_set_fields_mbuf(const struct ble_hs_adv_fields *adv_fields,
                               struct os_mbuf *om);

/**
 * Set the advertising data fields in a destination buffer.
 *
 * @param adv_fields     Pointer to the advertising data structure.
 * @param dst            Pointer to the destination buffer that will be written
 *                       with advertising data.
 * @param dst_len        Pointer to the variable that will hold the length of
 *                       the data written.
 * @param max_len        Maximum length of the destination buffer.
 *
 * @return               0 on success; nonzero on failure.
 */
int ble_hs_adv_set_fields(const struct ble_hs_adv_fields *adv_fields,
                          uint8_t *dst, uint8_t *dst_len, uint8_t max_len);

/**
 * Parse the advertising data fields from a source buffer.
 *
 * @param adv_fields     Pointer to the advertising data fields structure
 *                       to populate.
 * @param src            Pointer to the source buffer containing the data
 *                       to parse.
 * @param src_len        Length of the source buffer.
 *
 * @return               0 on success; nonzero on failure.
 */
int ble_hs_adv_parse_fields(struct ble_hs_adv_fields *adv_fields,
                            const uint8_t *src, uint8_t src_len);

/**
 * Parse the advertising data using the provided parsing function.
 *
 * @param data           Pointer to the advertising data buffer to parse.
 * @param length         Length of the advertising data buffer.
 * @param func           Pointer to the parsing function to apply to each
 *                       field.
 * @param user_data      User-defined data to pass to the parsing function.
 *
 * @return               0 on success; nonzero on failure.
 */
int ble_hs_adv_parse(const uint8_t *data, uint8_t length,
                     ble_hs_adv_parse_func_t func, void *user_data);

#ifdef __cplusplus
}
#endif

/**
 * @}
 */

#endif
