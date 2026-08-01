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

#ifndef H_BLE_ISO_
#define H_BLE_ISO_

/**
 * @file ble_iso.h
 *
 * @brief Bluetooth ISO
 * @defgroup bt_iso Bluetooth ISO
 * @ingroup bt_host
 * @{
 */

#include <inttypes.h>
#include "nimble/nimble/include/nimble/hci_common.h"
#include "syscfg/syscfg.h"

/**
 * @defgroup ble_iso_events ISO Events
 * @{
 */

/** ISO event: BIG Create Completed */
#define BLE_ISO_EVENT_BIG_CREATE_COMPLETE                   0

/** ISO event: BIG Terminate Completed */
#define BLE_ISO_EVENT_BIG_TERMINATE_COMPLETE                1

/** ISO event: BIG Sync Established */
#define BLE_ISO_EVENT_BIG_SYNC_ESTABLISHED                  2

/** ISO event: BIG Sync Terminated */
#define BLE_ISO_EVENT_BIG_SYNC_TERMINATED                   3

/** ISO event: ISO Data received */
#define BLE_ISO_EVENT_ISO_RX                                4

/** @} */

/** @brief Broadcast Isochronous Group (BIG) description */
struct ble_iso_big_desc {
    /**
     * The identifier of the BIG. Assigned by the Host when a new BIG is
     * created.
     */
    uint8_t big_handle;

    /**
     * The maximum time in microseconds for transmission of PDUs of all BISes in
     * a BIG event.
     */
    uint32_t big_sync_delay;

    /**
     * The actual transport latency of transmitting payloads of all BISes in the
     * BIG in microseconds.
     */
    uint32_t transport_latency_big;

    /** The number of subevents per BIS in each BIG event. */
    uint8_t nse;

    /**
     * The Burst Number (BN) specifies the number of new payloads in each BIS
     * event.
     */
    uint8_t bn;

    /**
     * The Pre-Transmission Offset (PTO) specifies the offset of groups that
     * carry data associated with the future BIS events.
     */
    uint8_t pto;

    /**
     * The Immediate Repetition Count (IRC) specifies the number of groups that
     * carry the data associated with the current BIS event.
     */
    uint8_t irc;

    /**
     * The maximum number of data octets (excluding the MIC, if any) that can be
     * carried in each BIS Data PDU in the BIG.
     */
    uint16_t max_pdu;

    /** The time between two adjacent BIG anchor points in units of 1.25 ms. */
    uint16_t iso_interval;

    /** The total number of BISes in the BIG. */
    uint8_t num_bis;

    /** The connection handles of all the BIS in the BIG. */
    uint16_t conn_handle[MYNEWT_VAL(BLE_ISO_MAX_BISES)];
};

/** @brief Received ISO Data status possible values */
enum ble_iso_rx_data_status {
    /** The complete SDU was received correctly. */
    BLE_ISO_DATA_STATUS_VALID = BLE_HCI_ISO_PKT_STATUS_VALID,

    /** May contain errors or part of the SDU may be missing. */
    BLE_ISO_DATA_STATUS_ERROR = BLE_HCI_ISO_PKT_STATUS_INVALID,

    /** Part(s) of the SDU were not received correctly */
    BLE_ISO_DATA_STATUS_LOST = BLE_HCI_ISO_PKT_STATUS_LOST,
};

/** @brief Received ISO data info structure */
struct ble_iso_rx_data_info {
    /** ISO Data timestamp. Valid if @ref ble_iso_rx_data_info.ts_valid is set */
    uint32_t ts;

    /** Packet sequence number */
    uint16_t seq_num;

    /** SDU length */
    uint16_t sdu_len : 12;

    /** ISO Data status. See @ref ble_iso_rx_data_status */
    uint16_t status : 2;

    /** Timestamp is valid */
    uint16_t ts_valid : 1;
};

/**
 * Represents a ISO-related event.  When such an event occurs, the host
 * notifies the application by passing an instance of this structure to an
 * application-specified callback.
 */
struct ble_iso_event {
    /**
     * Indicates the type of ISO event that occurred.  This is one of the
     * BLE_ISO_EVENT codes.
     */
    uint8_t type;

    /**
     * A discriminated union containing additional details concerning the ISO
     * event.  The 'type' field indicates which member of the union is valid.
     */
    union {
        /**
         * Represents a completion of BIG creation. Valid for the following
         * event types:
         *     o BLE_ISO_EVENT_BIG_CREATE_COMPLETE
         */
        struct {
            struct ble_iso_big_desc desc;
            uint8_t status;
            uint8_t phy;
        } big_created;

        /**
         * Represents a completion of BIG termination. Valid for the following
         * event types:
         *     o BLE_ISO_EVENT_BIG_TERMINATE_COMPLETE
         *     o BLE_ISO_EVENT_BIG_SYNC_TERMINATED
         */
        struct {
            uint16_t big_handle;
            uint8_t reason;
        } big_terminated;

        /**
         * Represents a completion of BIG synchronization. Valid for the following
         * event types:
         *     o BLE_ISO_EVENT_BIG_SYNC_ESTABLISHED
         */
        struct {
            struct ble_iso_big_desc desc;
            uint8_t status;
        } big_sync_established;

        /**
         * Represents a reception of ISO Data. Valid for the following
         * event types:
         *     o BLE_ISO_EVENT_ISO_RX
         */
        struct {
            uint16_t conn_handle;
            const struct ble_iso_rx_data_info *info;
            struct os_mbuf *om;
        } iso_rx;
    };
};

/** Function prototype for isochronous event callback. */
typedef int ble_iso_event_fn(struct ble_iso_event *event, void *arg);

/** Broadcast Isochronous Group (BIG) parameters */
struct ble_iso_big_params {
    /**
     * The time interval of the periodic SDUs in microseconds. The value shall
     * be between 0x0000FF and 0x0FFFFF.
     */
    uint32_t sdu_interval;

    /**
     * The maximum size of an SDU in octets. The value shall be between 0x0001
     * and 0x0FFF.
     */
    uint16_t max_sdu;

    /**
     * The maximum transport latency in milliseconds. The value shall be between
     * 0x0005 and 0x0FA0.
     */
    uint16_t max_transport_latency;

    /**
     * The Retransmission Number (RTN) parameter contains the number of times
     * every PDU should be retransmitted, irrespective of which BIG events the
     * retransmissions occur in. The value shall be between 0x00 and 0x1E.
     */
    uint8_t rtn;

    /**
     * The PHY parameter is a bit field that indicates the PHY used for
     * transmission of PDUs of BISes in the BIG. The value shall be one of the
     * following:
     *     o BLE_HCI_LE_PHY_1M_PREF_MASK
     *     o BLE_HCI_LE_PHY_2M_PREF_MASK
     *     o BLE_HCI_LE_PHY_CODED_PREF_MASK
     */
    uint8_t phy;

    /**
     * Indicates the preferred method of arranging subevents of multiple BISes.
     * The value shall be one of the following:
     *     o 0x00 - Sequential
     *     o 0x01 - Interleaved
     */
    uint8_t packing;

    /**
     * Indicates whether the BIG carries framed or unframed data. The value
     * shall be one of the following:
     *     o 0x00 - Unframed
     *     o 0x01 - Framed
     */
    uint8_t framing;

    /**
     * Indicates whether the BIG is encrypted or not. The value shall be one of
     * the following:
     *     o 0x00 - Unencrypted
     *     o 0x01 - Encrypted
     */
    uint8_t encryption;

    /**
     * The 128-bit code used to derive the session key that is used to encrypt
     * and decrypt BIS payloads.
     */
    const char *broadcast_code;
};

/** Create BIG parameters */
struct ble_iso_create_big_params {
    /** The associated periodic advertising train of the BIG. */
    uint8_t adv_handle;

    /** The total number of BISes in the BIG. */
    uint8_t bis_cnt;

    /** Callback function for reporting the status of the procedure. */
    ble_iso_event_fn *cb;

    /**
     * An optional user-defined argument to be passed to the callback function.
     */
    void *cb_arg;
};

/**
 * Initiates the creation of Broadcast Isochronous Group (BIG). It configures
 * the BIG parameters based on the provided input and triggers the corresponding
 * HCI command.
 *
 * @param create_params         A pointer to the structure holding the
 *                                  parameters specific to creating the BIG.
 *                                  These parameters define the general settings
 *                                  and include a callback function for handling
 *                                  creation events.
 * @param big_params            A pointer to the structure holding detailed
 *                                  parameters specific to the configuration of
 *                                  the BIG. These parameters include settings
 *                                  such as SDU interval, maximum SDU size,
 *                                  transport latency, etc.
 * @param[out] big_handle       BIG instance handle
 *
 * @return                      0 on success;
 *                              an error code on failure.
 *
 * @note The actual BIG creation result will be reported through the callback
 * function specified in @p create_params.
 */
int ble_iso_create_big(const struct ble_iso_create_big_params *create_params,
                       const struct ble_iso_big_params *big_params,
                       uint8_t *big_handle);

/**
 * Terminates an existing Broadcast Isochronous Group (BIG).
 *
 * @param big_handle            The identifier of the BIG to be terminated.
 *
 * @return                      0 on success;
 *                              an error code on failure.
 */
int ble_iso_terminate_big(uint8_t big_handle);

/** @brief BIS parameters for @ref ble_iso_big_sync_create */
struct ble_iso_bis_params {
    /** BIS index */
    uint8_t bis_index;
};

/** @brief BIG Sync parameters for @ref ble_iso_big_sync_create */
struct ble_iso_big_sync_create_params {
    /** Periodic advertising train sync handle */
    uint16_t sync_handle;

    /** Null-terminated broadcast code for encrypted BIG or
     *  NULL if the BIG is unencrypted
     */
    const char *broadcast_code;

    /** Maximum Subevents to be used to receive data payloads in each BIS event */
    uint8_t mse;

    /** The maximum permitted time between successful receptions of BIS PDUs */
    uint16_t sync_timeout;

    /** The callback to associate with this sync procedure.
     *  Sync establishment and termination are reported through this callback.
     */
    ble_iso_event_fn *cb;

    /** The optional argument to pass to the callback function */
    void *cb_arg;

    /** Number of a BISes */
    uint8_t bis_cnt;

    /** BIS parameters */
    struct ble_iso_bis_params *bis_params;
};

/**
 * @brief Synchronize to Broadcast Isochronous Group (BIG)
 *
 * This function is used to synchronize to a BIG described in the periodic
 * advertising train specified by the @p param->pa_sync_handle parameter.
 *
 * @param[in] params            BIG synchronization parameters
 * @param[out] big_handle       BIG instance handle
 *
 * @return                      0 on success;
 *                              A non-zero value on failure.
 */
int ble_iso_big_sync_create(const struct ble_iso_big_sync_create_params *params,
                            uint8_t *big_handle);

/**
 * @brief Terminate Broadcast Isochronous Group (BIG) sync
 *
 * This function is used to stop synchronizing or cancel the process of
 * synchronizing to the BIG identified by the @p big_handle parameter.
 * The command also terminates the reception of BISes in the BIG.
 *
 * @param[in] big_handle        BIG handle
 *
 * @return                      0 on success;
 *                              A non-zero value on failure.
 */
int ble_iso_big_sync_terminate(uint8_t big_handle);

/** @brief ISO Data direction */
enum ble_iso_data_dir {
    BLE_ISO_DATA_DIR_TX,
    BLE_ISO_DATA_DIR_RX,
};

/** @brief ISO Codec ID */
struct ble_iso_codec_id {
    /** Coding Format */
    uint8_t format;

    /** Company ID */
    uint16_t company_id;

    /** Vendor Specific Codec ID */
    uint16_t vendor_specific;
};

/** @brief Setup ISO Data Path parameters */
struct ble_iso_data_path_setup_params {
    /** Connection handle of the CIS or BIS */
    uint16_t conn_handle;

    /** Data path direction */
    enum ble_iso_data_dir data_path_dir;

    /** Data path ID. 0x00 for HCI */
    uint8_t data_path_id;

    /** Controller delay */
    uint32_t ctrl_delay;

    /** Codec ID */
    struct ble_iso_codec_id codec_id;

    /** Codec Configuration Length */
    uint8_t codec_config_len;

    /** Codec Configuration */
    const uint8_t *codec_config;

    /**
     * The ISO Data callback. Must be set if @p data_path_id is HCI.
     * Received ISO data is reported through this callback.
     */
    ble_iso_event_fn *cb;

    /** The optional argument to pass to the callback function */
    void *cb_arg;
};

/**
 * @brief Setup ISO Data Path
 *
 * This function is used to identify and create the isochronous data path
 * between the Host and the Controller for a CIS, CIS configuration, or BIS
 * identified by the @p param->conn_handle parameter.
 *
 * @param[in] param             BIG synchronization parameters
 *
 * @return                      0 on success;
 *                              A non-zero value on failure.
 */
int ble_iso_data_path_setup(const struct ble_iso_data_path_setup_params *param);

/** @brief @brief Remove ISO Data Path parameters */
struct ble_iso_data_path_remove_params {
    /** Connection handle of the CIS or BIS */
    uint16_t conn_handle;

    /** Data path direction */
    enum ble_iso_data_dir data_path_dir;
};

/**
 * @brief Remove ISO Data Path
 *
 * This function is used to remove the input and/or output data path(s)
 * associated with a CIS, CIS configuration, or BIS identified by the
 * @p param->conn_handle parameter.
 *
 * @param[in] param             BIG synchronization parameters
 *
 * @return                      0 on success;
 *                              A non-zero value on failure.
 */
int ble_iso_data_path_remove(const struct ble_iso_data_path_remove_params *param);

/**
 * Initiates the transmission of isochronous data.
 *
 * @param conn_handle           The connection over which to execute the procedure.
 * @param data                  A pointer to the data to be transmitted.
 * @param data_len              Number of the data octets to be transmitted.
 *
 * @return                      0 on success;
 *                              an error code on failure.
 */
int ble_iso_tx(uint16_t conn_handle, void *data, uint16_t data_len);

/**
 * Initializes memory for ISO.
 *
 * @return                      0 on success
 */
int ble_iso_init(void);

/**
 * @}
 */

#endif /* H_BLE_ISO_ */
