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

#ifndef H_BLE_SM_
#define H_BLE_SM_

/**
 * @brief Bluetooth Security Manager (SM)
 * @defgroup bt_sm Bluetooth Security Manager (SM)
 * @ingroup bt_host
 * @{
 */

#include <inttypes.h>
#include "syscfg/syscfg.h"
#include <stdbool.h>
#include "nimble/nimble/include/nimble/ble.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @defgroup ble_sm_err Security Manager (SM) Error Codes
 * @{
 */

/** SM Error Code: Success */
#define BLE_SM_ERR_SUCCESS                      0x00

/** SM Error Code: Passkey entry failed */
#define BLE_SM_ERR_PASSKEY                      0x01

/** SM Error Code: Out Of Band (OOB) not available */
#define BLE_SM_ERR_OOB                          0x02

/** SM Error Code: Authentication Requirements */
#define BLE_SM_ERR_AUTHREQ                      0x03

/** SM Error Code: Confirm Value failed */
#define BLE_SM_ERR_CONFIRM_MISMATCH             0x04

/** SM Error Code: Pairing Not Supported */
#define BLE_SM_ERR_PAIR_NOT_SUPP                0x05

/** SM Error Code: Encryption Key Size */
#define BLE_SM_ERR_ENC_KEY_SZ                   0x06

/** SM Error Code: Command Not Supported */
#define BLE_SM_ERR_CMD_NOT_SUPP                 0x07

/** SM Error Code: Unspecified Reason */
#define BLE_SM_ERR_UNSPECIFIED                  0x08

/** SM Error Code: Repeated Attempts */
#define BLE_SM_ERR_REPEATED                     0x09

/** SM Error Code: Invalid Parameters */
#define BLE_SM_ERR_INVAL                        0x0a

/** SM Error Code: DHKey Check failed */
#define BLE_SM_ERR_DHKEY                        0x0b

/** SM Error Code: Numeric Comparison failed */
#define BLE_SM_ERR_NUMCMP                       0x0c

/** SM Error Code: Pairing in progress */
#define BLE_SM_ERR_ALREADY                      0x0d

/** SM Error Code: Cross-transport Key Derivation/Generation not allowed */
#define BLE_SM_ERR_CROSS_TRANS                  0x0e

/** SM Error Code: Key Rejected */
#define BLE_SM_ERR_KEY_REJ                      0x0f

/** SM Error Code: Out Of Boundary Code Value */
#define BLE_SM_ERR_MAX_PLUS_1                   0x10

/** @} */

/**
 * @defgroup ble_sm_pair_alg Security Manager (SM) Pairing Algorithms
 * @{
 */

/** SM Pairing Algorithm: Just Works */
#define BLE_SM_PAIR_ALG_JW                      0

/** SM Pairing Algorithm: Passkey Entry */
#define BLE_SM_PAIR_ALG_PASSKEY                 1

/** SM Pairing Algorithm: Out Of Band */
#define BLE_SM_PAIR_ALG_OOB                     2

/** SM Pairing Algorithm: Numeric Comparison */
#define BLE_SM_PAIR_ALG_NUMCMP                  3

/** @} */

/**
 * @defgroup ble_sm_pair_key_dist Security Manager (SM) Key Distribution Flags
 * @{
 */

/** SM Key Distribution: Distribute Long Term Key (LTK) */
#define BLE_SM_PAIR_KEY_DIST_ENC                0x01

/** SM Key Distribution: Distribute Identity Resolving Key (IRK) */
#define BLE_SM_PAIR_KEY_DIST_ID                 0x02

/** SM Key Distribution: Distribute Connection Signature Resolving Key (CSRK) */
#define BLE_SM_PAIR_KEY_DIST_SIGN               0x04

/** SM Key Distribution: Derive the Link Key from the LTK */
#define BLE_SM_PAIR_KEY_DIST_LINK               0x08

/** SM Key Distribution: Reserved For Future Use */
#define BLE_SM_PAIR_KEY_DIST_RESERVED           0xf0

/** @} */

/**
 * @defgroup ble_sm_io_cap Security Manager (SM) Input/Output Capabilities
 * @{
 */

/** SM IO Capabilities: Display Only */
#define BLE_SM_IO_CAP_DISP_ONLY                 0x00

/** SM IO Capabilities: Display Yes No */
#define BLE_SM_IO_CAP_DISP_YES_NO               0x01

/** SM IO Capabilities: Keyboard Only */
#define BLE_SM_IO_CAP_KEYBOARD_ONLY             0x02

/** SM IO Capabilities: No Input No Output */
#define BLE_SM_IO_CAP_NO_IO                     0x03

/** SM IO Capabilities: Keyboard Display */
#define BLE_SM_IO_CAP_KEYBOARD_DISP             0x04

/** SM IO Capabilities: Reserved For Future Use */
#define BLE_SM_IO_CAP_RESERVED                  0x05

/** @} */

/**
 * @defgroup ble_sm_pair_oob Security Manager (SM) Out Of Band Data (OOB) Flags
 * @{
 */

/** SM OOB: Out Of Band Data Not Available */
#define BLE_SM_PAIR_OOB_NO                      0x00

/** SM OOB: Out Of Band Data Available */
#define BLE_SM_PAIR_OOB_YES                     0x01

/** SM OOB: Reserved For Future Use */
#define BLE_SM_PAIR_OOB_RESERVED                0x02

/** @} */

/**
 * @defgroup ble_sm_authreq Security Manager (SM) Authentication Requirements Flags
 * @{
 */

/** SM Authentication Requirement: Bonding */
#define BLE_SM_PAIR_AUTHREQ_BOND                0x01

/** SM Authentication Requirement: MITM protection */
#define BLE_SM_PAIR_AUTHREQ_MITM                0x04

/** SM Authentication Requirement: Secure Connections */
#define BLE_SM_PAIR_AUTHREQ_SC                  0x08

/** SM Authentication Requirement: Keypress notifications */
#define BLE_SM_PAIR_AUTHREQ_KEYPRESS            0x10

/** SM Authentication Requirement: Reserved For Future Use */
#define BLE_SM_PAIR_AUTHREQ_RESERVED            0xe2

/** @} */

/**
 * @defgroup ble_sm_pair_key_sz Security Manager (SM) Key Sizes
 * @{
 */

/** SM Key Size: Minimum supported encryption key size in octets */
#define BLE_SM_PAIR_KEY_SZ_MIN                  7

/** SM Key Size: Maximum supported encryption key size in octets */
#define BLE_SM_PAIR_KEY_SZ_MAX                  16

/** @} */

/**
 * @defgroup ble_sm_ioact Security Manager (SM) Key Generation Action
 * @{
 * The security manager asks the application to perform a key generation
 * action.  The application passes the passkey back to SM via
 * ble_sm_inject_io().
 */

/** SM IO Action: None (Just Works pairing) */
#define BLE_SM_IOACT_NONE                       0

/** SM IO Action: Out Of Band (OOB) */
#define BLE_SM_IOACT_OOB                        1

/** SM IO Action: Input (Passkey Entry) */
#define BLE_SM_IOACT_INPUT                      2

/** SM IO Action: Passkey Display */
#define BLE_SM_IOACT_DISP                       3

/** SM IO Action: Numeric Comparison */
#define BLE_SM_IOACT_NUMCMP                     4

/** SM IO Action: Out Of Band (OOB) Secure Connections */
#define BLE_SM_IOACT_OOB_SC                     5

/** SM IO Action: Out Of Boundary Code Value */
#define BLE_SM_IOACT_MAX_PLUS_ONE               6

/** @} */

/** Represents Out Of Band (OOB) data used in Secure Connections pairing */
struct ble_sm_sc_oob_data {
    /** Random Number. */
    uint8_t r[16];

    /** Confirm Value. */
    uint8_t c[16];
};

/** Represents Input/Output data for Security Manager used during pairing process */
struct ble_sm_io {
    /** Pairing action, indicating the type of pairing method. Can be one of the
     *  following:
     *      o BLE_SM_IOACT_NONE
     *      o BLE_SM_IOACT_OOB
     *      o BLE_SM_IOACT_INPUT
     *      o BLE_SM_IOACT_DISP
     *      o BLE_SM_IOACT_NUMCMP
     *      o BLE_SM_IOACT_OOB_SC
     */
    uint8_t action;

    /** Union holding different types of pairing data. The valid field is inferred
     *  from the action field. */
    union {
        /** Passkey value between 000000 and 999999.
         *  Valid for the following actions:
         *      o BLE_SM_IOACT_INPUT
         *      o BLE_SM_IOACT_DISP
         */
        uint32_t passkey;

        /** Temporary Key random value used in Legacy Pairing.
         *  Valid for the following actions:
         *      o BLE_SM_IOACT_OOB
         */
        uint8_t  oob[16];

        /** Numeric Comparison acceptance indicator.
         *  Valid for the following actions:
         *      o BLE_SM_IOACT_NUMCMP
         */
        uint8_t  numcmp_accept;

        /** Out Of Band (OOB) data used in Secure Connections.
         *  Valid for the following actions:
         *      o BLE_SM_IOACT_OOB_SC
         */
        struct {
            /** Remote Secure Connections Out Of Band (OOB) data */
            struct ble_sm_sc_oob_data *remote;
            /** Local Secure Connections Out Of Band (OOB) data */
            struct ble_sm_sc_oob_data *local;
        } oob_sc_data;
    };
};

/**
 * Generates Out of Band (OOB) data used during the authentication process.
 * The data consists of 128-bit Random Number and 128-bit Confirm Value.
 *
 * @param oob_data              A pointer to the structure where the generated
 *                              OOB data will be stored.
 *
 * @return                      0 on success;
 *                              Non-zero on failure.
 */
int ble_sm_sc_oob_generate_data(struct ble_sm_sc_oob_data *oob_data);

#if MYNEWT_VAL(BLE_SM_CSIS_SIRK)
/**
 * Resolves CSIS RSI to check if advertising device is part of the same Coordinated Set,
 * as the device with specified SIRK
 *
 * @param rsi                   RSI value from Advertising Data
 * @param sirk                  SIRK
 * @param ltk_peer_addr         If SIRK is in plaintext form this should be set to NULL,
 *                              otherwise peer address should be passed here to get
 *                              LTK and decrypt SIRK
 *
 * @return                      0 if RSI was resolved succesfully; nonzero on failure.
 */
int ble_sm_csis_resolve_rsi(const uint8_t *rsi, const uint8_t *sirk,
                            const ble_addr_t *ltk_peer_addr);
#endif

#if NIMBLE_BLE_SM
/**
 * @brief Passes the IO data from an application to the Security Manager during the pairing
 * process.
 *
 * It should be used after a pairing method has been established for given connection
 * and once the appropriate key generation information (e.g. passkey) has been obtained.
 *
 * @param conn_handle           The connection handle of the relevant connection.
 * @param pkey                  A pointer to the structure where IO data is stored.
 *
 * @return                      0 on success;
 *                              Non-zero on failure.
 */
int ble_sm_inject_io(uint16_t conn_handle, struct ble_sm_io *pkey);
#else
/** This macro replaces the function to return BLE_HS_ENOTSUP when SM is disabled. */
#define ble_sm_inject_io(conn_handle, pkey) \
    ((void)(conn_handle), BLE_HS_ENOTSUP)
#endif

#ifdef __cplusplus
}
#endif

/**
 * @}
 */

#endif /* H_BLE_SM_ */
