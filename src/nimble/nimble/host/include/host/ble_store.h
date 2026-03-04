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

#ifndef H_BLE_STORE_
#define H_BLE_STORE_

/**
 * @brief Bluetooth Store API
 * @defgroup bt_store Bluetooth Store
 * @ingroup bt_host
 * @{
 */

#include <inttypes.h>
#include "nimble/nimble/include/nimble/ble.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @defgroup bt_store_obj_types Bluetooth Store Object Types
 * @ingroup bt_host
 * @{
 */
/** Object type: Our security material. */
#define BLE_STORE_OBJ_TYPE_OUR_SEC      1

/** Object type: Peer security material. */
#define BLE_STORE_OBJ_TYPE_PEER_SEC     2

/** Object type: Client Characteristic Configuration Descriptor. */
#define BLE_STORE_OBJ_TYPE_CCCD         3

#define BLE_STORE_OBJ_TYPE_PEER_DEV_REC      4

#if MYNEWT_VAL(ENC_ADV_DATA)
#define BLE_STORE_OBJ_TYPE_ENC_ADV_DATA      5
#endif
#define BLE_STORE_OBJ_TYPE_PEER_ADDR         6

#define BLE_STORE_OBJ_TYPE_LOCAL_IRK         7
#define BLE_STORE_OBJ_TYPE_CSFC              8

/** @} */

/**
 * @defgroup bt_store_event_types Bluetooth Store Event Types
 * @ingroup bt_host
 * @{
 */
/** Failed to persist record; insufficient storage capacity. */
#define BLE_STORE_EVENT_OVERFLOW        1

/** About to execute a procedure that may fail due to overflow. */
#define BLE_STORE_EVENT_FULL            2

/** @} */

/**
 * Used as a key for lookups of security material.  This struct corresponds to
 * the following store object types:
 *     o BLE_STORE_OBJ_TYPE_OUR_SEC
 *     o BLE_STORE_OBJ_TYPE_PEER_SEC
 */
struct ble_store_key_sec {
    /**
     * Key by peer identity address;
     * peer_addr=BLE_ADDR_NONE means don't key off peer.
     */
    ble_addr_t peer_addr;

    /** Number of results to skip; 0 means retrieve the first match. */
    uint8_t idx;
};

/**
 * Represents stored security material.  This struct corresponds to the
 * following store object types:
 *     o BLE_STORE_OBJ_TYPE_OUR_SEC
 *     o BLE_STORE_OBJ_TYPE_PEER_SEC
 */
struct ble_store_value_sec {
    /** Peer address for which the security material is stored. */
    ble_addr_t peer_addr;

    /* Espressif added for bond age tracking persistence */
    uint16_t bond_count;

    /** Encryption key size. */
    uint8_t key_size;
    /** Encrypted Diversifier used for encryption key generation. */
    uint16_t ediv;
    /** Random number used for encryption key generation. */
    uint64_t rand_num;
    /** Long Term Key. */
    uint8_t ltk[16];
    /** Flag indicating whether Long Term Key is present. */
    uint8_t ltk_present:1;

    /** Identity Resolving Key. */
    uint8_t irk[16];
    /** Flag indicating whether Identity Resolving Key is present. */
    uint8_t irk_present:1;

    /** Connection Signature Resolving Key. */
    uint8_t csrk[16];
    /** Flag indicating if Connection Signature Resolving Key is present. */
    uint8_t csrk_present:1;

    /* Espressif added for signed write support */
    uint32_t sign_counter;

    /** Flag indicating whether the connection is authenticated. */
    unsigned authenticated:1;
    /** Flag indicating Secure Connections support. */
    uint8_t sc:1;
};

/**
 * Used as a key for lookups of stored client characteristic configuration
 * descriptors (CCCDs).  This struct corresponds to the BLE_STORE_OBJ_TYPE_CCCD
 * store object type.
 */
struct ble_store_key_cccd {
    /**
     * Key by peer identity address;
     * peer_addr=BLE_ADDR_NONE means don't key off peer.
     */
    ble_addr_t peer_addr;

    /**
     * Key by characteristic value handle;
     * chr_val_handle=0 means don't key off characteristic handle.
     */
    uint16_t chr_val_handle;

    /** Number of results to skip; 0 means retrieve the first match. */
    uint8_t idx;
};

/**
 * Represents a stored client characteristic configuration descriptor (CCCD).
 * This struct corresponds to the BLE_STORE_OBJ_TYPE_CCCD store object type.
 */
struct ble_store_value_cccd {
    /** The peer address associated with the stored CCCD. */
    ble_addr_t peer_addr;
    /** The handle of the characteristic value. */
    uint16_t chr_val_handle;
    /** Flags associated with the CCCD. */
    uint16_t flags;
    /** Flag indicating whether the value has changed. */
    unsigned value_changed:1;
};

/**
 * Used as a key for lookups of stored client supported features characteristic (CSFC).
 * This struct corresponds to the BLE_STORE_OBJ_TYPE_CSFC store object type.
 */
struct ble_store_key_csfc {
    /**
     * Key by peer identity address;
     */
    ble_addr_t peer_addr;

    /** Number of results to skip; 0 means retrieve the first match. */
    uint8_t idx;
};

/**
 * Represents a stored client supported features characteristic (CSFC).
 * This struct corresponds to the BLE_STORE_OBJ_TYPE_CSFC store object type.
 */
struct ble_store_value_csfc {
    ble_addr_t peer_addr;
    uint8_t csfc[MYNEWT_VAL(BLE_GATT_CSFC_SIZE)];
};

#if MYNEWT_VAL(ENC_ADV_DATA)
/**
 * Used as a key for lookups of encrypted advertising data. This struct corresponds
 * to the BLE_STORE_OBJ_TYPE_ENC_ADV_DATA store object type.
 */
struct ble_store_key_ead {
    /**
     * Key by peer identity address;
     * peer_addr=BLE_ADDR_NONE means don't key off peer.
     */
    ble_addr_t peer_addr;

    /** Number of results to skip; 0 means retrieve the first match. */
    uint8_t idx;
};

/**
 * Represents a stored encrypted advertising data. This struct corresponds
 * to the BLE_STORE_OBJ_TYPE_ENC_ADV_DATA store object type.
 */
struct ble_store_value_ead {
    ble_addr_t peer_addr;
    unsigned km_present:1;
    struct key_material *km;
};
#endif

struct ble_store_key_local_irk {
    ble_addr_t addr;

    uint8_t idx;
};

struct ble_store_value_local_irk {
     ble_addr_t addr;

     uint8_t irk[16];
};

struct ble_store_key_rpa_rec{
    ble_addr_t peer_rpa_addr;
    uint8_t idx;
};

struct ble_store_value_rpa_rec{
    ble_addr_t peer_rpa_addr;
    ble_addr_t peer_addr;
};

/**
 * Used as a key for store lookups.  This union must be accompanied by an
 * object type code to indicate which field is valid.
 */
union ble_store_key {
    /** Key for security material store lookups. */
    struct ble_store_key_sec sec;
    /** Key for Client Characteristic Configuration Descriptor store lookups. */
    struct ble_store_key_cccd cccd;
#if MYNEWT_VAL(ENC_ADV_DATA)
    struct ble_store_key_ead ead;
#endif
    struct ble_store_key_rpa_rec rpa_rec;
    struct ble_store_key_local_irk local_irk;
    struct ble_store_key_csfc csfc;
};

/**
 * Represents stored data.  This union must be accompanied by an object type
 * code to indicate which field is valid.
 */
union ble_store_value {
    /** Stored security material. */
    struct ble_store_value_sec sec;
    /** Stored Client Characteristic Configuration Descriptor. */
    struct ble_store_value_cccd cccd;
#if MYNEWT_VAL(ENC_ADV_DATA)
    struct ble_store_value_ead ead;
#endif
    struct ble_store_value_rpa_rec rpa_rec;
    struct ble_store_value_local_irk local_irk;
    struct ble_store_value_csfc csfc;
};

/** Represents an event associated with the BLE Store. */
struct ble_store_status_event {
    /**
     * The type of event being reported; one of the BLE_STORE_EVENT_TYPE_[...]
     * codes.
     */
    int event_code;

    /**
     * Additional data related to the event; the valid field is inferred from
     * the obj_type,event_code pair.
     */
    union {
        /**
         * Represents a write that failed due to storage exhaustion.  Valid for
         * the following event types:
         *     o BLE_STORE_EVENT_OVERFLOW
         */
        struct {
            /** The type of object that failed to be written. */
            int obj_type;

            /** The object that failed to be written. */
            const union ble_store_value *value;
        } overflow;

        /**
         * Represents the possibility that a scheduled write will fail due to
         * storage exhaustion.  Valid for the following event types:
         *     o BLE_STORE_EVENT_FULL
         */
        struct {
            /** The type of object that may fail to be written. */
            int obj_type;

            /** The handle of the connection which prompted the write. */
            uint16_t conn_handle;
        } full;
    };
};

/** Generate LTK, EDIT and Rand. */
#define BLE_STORE_GEN_KEY_LTK       0x01
/** Generate IRK. */
#define BLE_STORE_GEN_KEY_IRK       0x02
/** Generate CSRK. */
#define BLE_STORE_GEN_KEY_CSRK      0x03

/** Represents a storage for generated key. */
struct ble_store_gen_key {
    union {
        /** Long Term Key (LTK) for peripheral role. */
        uint8_t ltk_periph[16];
        /** Identity Resolving Key (IRK). */
        uint8_t irk[16];
        /** Connection Signature Resolving Key (CSRK). */
        uint8_t csrk[16];
    };
    /** Encrypted Diversifier (EDIV). */
    uint16_t ediv;
    /** Random Number for key generation. */
    uint64_t rand;
};

/**
 * Generates key required by security module.
 * This can be used to use custom routines to generate keys instead of simply
 * randomizing them.
 *
 * \p conn_handle is set to \p BLE_HS_CONN_HANDLE_NONE if key is not requested
 * for a specific connection (e.g. an IRK).
 *
 * @param key                   Key that shall be generated.
 * @param gen_key               Storage for generated key.
 * @param conn_handle           Connection handle for which keys are generated.
 *
 * @return                      0 if keys were generated successfully
 *                              Other nonzero on error.
 */
typedef int ble_store_gen_key_fn(uint8_t key,
                                 struct ble_store_gen_key *gen_key,
                                 uint16_t conn_handle);

/**
 * Searches the store for an object matching the specified criteria.  If a
 * match is found, it is read from the store and the dst parameter is populated
 * with the retrieved object.
 *
 * @param obj_type              The type of object to search for; one of the
 *                                  BLE_STORE_OBJ_TYPE_[...] codes.
 * @param key                   Specifies properties of the object to search
 *                                  for.  An object is retrieved if it matches
 *                                  these criteria.
 * @param dst                   On success, this is populated with the
 *                                  retrieved object.
 *
 * @return                      0 if an object was successfully retreived;
 *                              BLE_HS_ENOENT if no matching object was found;
 *                              Other nonzero on error.
 */
typedef int ble_store_read_fn(int obj_type, const union ble_store_key *key,
                              union ble_store_value *dst);

/**
 * Writes the specified object to the store.  If an object with the same
 * identity is already in the store, it is replaced.  If the store lacks
 * sufficient capacity to write the object, this function may remove previously
 * stored values to make room.
 *
 * @param obj_type              The type of object being written; one of the
 *                                  BLE_STORE_OBJ_TYPE_[...] codes.
 * @param val                   The object to persist.
 *
 * @return                      0 if the object was successfully written;
 *                              Other nonzero on error.
 */
typedef int ble_store_write_fn(int obj_type, const union ble_store_value *val);

/**
 * Searches the store for the first object matching the specified criteria.  If
 * a match is found, it is deleted from the store.
 *
 * @param obj_type              The type of object to delete; one of the
 *                                  BLE_STORE_OBJ_TYPE_[...] codes.
 * @param key                   Specifies properties of the object to search
 *                                  for.  An object is deleted if it matches
 *                                  these criteria.
 * @return                      0 if an object was successfully retrieved;
 *                              BLE_HS_ENOENT if no matching object was found;
 *                              Other nonzero on error.
 */
typedef int ble_store_delete_fn(int obj_type, const union ble_store_key *key);

/**
 * Indicates an inability to perform a store operation.  This callback should
 * do one of two things:
 *     o Address the problem and return 0, indicating that the store operation
 *       should proceed.
 *     o Return nonzero to indicate that the store operation should be aborted.
 *
 * @param event                 Describes the store event being reported.
 * @param arg                   Optional user argument.
 *
 * @return                      0 if the store operation should proceed;
 *                              nonzero if the store operation should be
 *                                  aborted.
 */
typedef int ble_store_status_fn(struct ble_store_status_event *event,
                                void *arg);

/**
 * Reads data from a storage.
 *
 * @param obj_type              The type of the object to read.
 * @param key                   Pointer to the key used for the lookup.
 * @param val                   Pointer to store the retrieved data.
 *
 * @return                      0 if the read operation is successful;
 *                              Non-zero on error.
 */
int ble_store_read(int obj_type, const union ble_store_key *key,
                   union ble_store_value *val);

/**
 * Writes data to a storage.
 *
 * @param obj_type              The type of the object to write.
 * @param val                   Pointer to the data to be written.
 *
 * @return                      0 if the write operation is successful;
 *                              Non-zero on error.
 */
int ble_store_write(int obj_type, const union ble_store_value *val);

/**
 * Deletes data from a storage.
 *
 * @param obj_type              The type of the object to delete.
 * @param key                   Pointer to the key used for the lookup.
 *
 * @return                      0 if the deletion operation is successful;
 *                              Non-zero on error.
 */
int ble_store_delete(int obj_type, const union ble_store_key *key);

/**
 * @brief Handles a storage overflow event.
 *
 * This function is called when a storage overflow event occurs. It constructs
 * an event structure and passes it to the general storage status handler using
 * the `ble_store_status` function.
 *
 * @param obj_type              The type of the object for which the overflow
 *                                  occurred.
 * @param value                 Pointer to the value associated with the
 *                                  overflow.
 *
 * @return                      0 if the event is successfully handled;
 *                              Non-zero on error.
 */
int ble_store_overflow_event(int obj_type, const union ble_store_value *value);

/**
 * @brief Handles a storage full event.
 *
 * This function is called when a storage full event occurs, typically during
 * a write operation. It constructs an event structure and passes it to the
 * general storage status handler using the `ble_store_status` function.
 *
 * @param obj_type              The type of the object that may fail to be
 *                                  written.
 * @param conn_handle           The connection handle associated with the write
 *                                  operation.
 *
 * @return                      0 if the event is successfully handled;
 *                              Non-zero on error.
 */
int ble_store_full_event(int obj_type, uint16_t conn_handle);

/**
 * @brief Reads our security material from a storage.
 *
 * This function reads our security material from a storage, using the provided
 * key and populates the `ble_store_value_sec` structure with the retrieved data.
 *
 * @param key_sec               The key identifying the security material to
 *                                  read.
 * @param value_sec             A pointer to a `ble_store_value_sec` structure
 *                                  where the retrieved security material will
 *                                  be stored.
 *
 * @return                      0 if the security material was successfully read
 *                                  from a storage;
 *                              Non-zero on error.
 */
int ble_store_read_our_sec(const struct ble_store_key_sec *key_sec,
                           struct ble_store_value_sec *value_sec);

/**
 * @brief Writes our security material to a storage.
 *
 * This function writes our security material to a storage, using the provided
 * `ble_store_value_sec` structure.
 *
 * @param value_sec             A pointer to a `ble_store_value_sec` structure
 *                                  containing the security material to be
 *                                  stored.
 *
 * @return                      0 if the security material was successfully
 *                                  written to a storage;
 *                              Non-zero on error.
 */
int ble_store_write_our_sec(const struct ble_store_value_sec *value_sec);

/**
 * @brief Deletes our security material from a storage.
 *
 * This function deletes our security material from a storage, identified by the
 * provided `ble_store_key_sec`.
 *
 * @param key_sec               A pointer to a `ble_store_key_sec` structure
 *                                  that specifies the security material to be
 *                                  deleted.
 *
 * @return                      0 if the security material was successfully
 *                                  deleted from a storage;
 *                              Non-zero on error.
 */
int ble_store_delete_our_sec(const struct ble_store_key_sec *key_sec);

/**
 * @brief Reads peer security material from a storage.
 *
 * This function reads peer security material from a storage, identified by the
 * provided `ble_store_key_sec`. The retrieved security material is stored in
 * the `ble_store_value_sec` structure.
 *
 * @param key_sec               A pointer to a `ble_store_key_sec` structure
 *                                  that specifies the peer's security material
 *                                  to be retrieved.
 * @param value_sec             A pointer to a `ble_store_value_sec` structure
 *                                  where the retrieved security material will
 *                                  be stored.
 *
 * @return                      0 if the security material was successfully
 *                                  retrieved;
 *                              Non-zero on error.
 */
int ble_store_read_peer_sec(const struct ble_store_key_sec *key_sec,
                            struct ble_store_value_sec *value_sec);

/**
 * @brief Writes peer security material to a storage.
 *
 * This function writes the provided peer security material, specified by a
 * `ble_store_value_sec` structure, to a storage. Additionally, if the provided
 * peer IRK is present and the peer address is not `BLE_ADDR_ANY`, it is also
 * written to the controller's key cache.
 *
 * @param value_sec             A pointer to a `ble_store_value_sec` structure
 *                                  containing the peer's security material
 *                                  to be written.
 *
 * @return                      0 if the peer's security material was
 *                                  successfully written to a storage and
 *                                  the peer IRK was added to the controller key
 *                                  cache if present;
 *                              Non-zero on error.
 */
int ble_store_write_peer_sec(const struct ble_store_value_sec *value_sec);

/**
 * @brief Deletes peer security material from a storage.
 *
 * This function deletes the peer security material associated with the provided
 * key from a storage.
 *
 * @param key_sec               A pointer to a `ble_store_key_sec` structure
 *                                  identifying the security material to be
 *                                  deleted.
 *
 * @return                      0 if the peer's security material was
 *                                  successfully deleted from a storage;
 *                              Non-zero on error.
 */
int ble_store_delete_peer_sec(const struct ble_store_key_sec *key_sec);


/**
 * @brief Reads a Client Characteristic Configuration Descriptor (CCCD) from
 * a storage.
 *
 * This function reads a CCCD from a storage based on the provided key and
 * stores the retrieved value in the specified output structure.
 *
 * @param key                   A pointer to a `ble_store_key_cccd` structure
 *                                  representing the key to identify the CCCD to
 *                                  be read.
 * @param out_value             A pointer to a `ble_store_value_cccd` structure
 *                                  to store the CCCD value read from a storage.
 *
 * @return                      0 if the CCCD was successfully read and stored
 *                                  in the `out_value` structure;
 *                              Non-zero on error.
 */
int ble_store_read_cccd(const struct ble_store_key_cccd *key,
                        struct ble_store_value_cccd *out_value);

/**
 * @brief Writes a Client Characteristic Configuration Descriptor (CCCD) to
 * a storage.
 *
 * This function writes a CCCD to a storage based on the provided value.
 *
 * @param value                 A pointer to a `ble_store_value_cccd` structure
 *                                  representing the CCCD value to be written to
 *                                  a storage.
 *
 * @return                      0 if the CCCD was successfully written to
 *                                  a storage;
 *                              Non-zero on error.
 */
int ble_store_write_cccd(const struct ble_store_value_cccd *value);

/**
 * @brief Deletes a Client Characteristic Configuration Descriptor (CCCD) from
 * a storage.
 *
 * This function deletes a CCCD from a storage based on the provided key.
 *
 * @param key                   A pointer to a `ble_store_key_cccd` structure
 *                                  identifying the CCCD to be deleted from
 *                                  a storage.
 *
 * @return                      0 if the CCCD was successfully deleted from
 *                                  a storage;
 *                              Non-zero on error.
 */
int ble_store_delete_cccd(const struct ble_store_key_cccd *key);

int ble_store_read_csfc(const struct ble_store_key_csfc *key,
                        struct ble_store_value_csfc *out_value);
int ble_store_write_csfc(const struct ble_store_value_csfc *value);
int ble_store_delete_csfc(const struct ble_store_key_csfc *key);

/**
 * @brief Generates a storage key for a security material entry from its value.
 *
 * This function generates a storage key for a security material entry based on
 * the provided security material value.
 *
 * @param out_key               A pointer to a `ble_store_key_sec` structure
 *                                  where the generated key will be stored.
 * @param value                 A pointer to a `ble_store_value_sec` structure
 *                                  containing the security material value from
 *                                  which the key will be generated.
 */
void ble_store_key_from_value_sec(struct ble_store_key_sec *out_key,
                                  const struct ble_store_value_sec *value);

/**
 * @brief Generates a storage key for a CCCD entry from its value.
 *
 * This function generates a storage key for a Client Characteristic
 * Configuration Descriptor (CCCD) entry based on the provided CCCD value.
 *
 * @param out_key               A pointer to a `ble_store_key_cccd` structure
 *                                  where the generated key will be stored.
 * @param value                 A pointer to a `ble_store_value_cccd` structure
 *                                  containing the CCCD value from which the key
 *                                  will be generated.
 */
void ble_store_key_from_value_cccd(struct ble_store_key_cccd *out_key,
                                   const struct ble_store_value_cccd *value);

void ble_store_key_from_value_csfc(struct ble_store_key_csfc *out_key,
                                   const struct ble_store_value_csfc *value);

#if MYNEWT_VAL(ENC_ADV_DATA)
int ble_store_read_ead(const struct ble_store_key_ead *key,
                       struct ble_store_value_ead *out_value);
int ble_store_write_ead(const struct ble_store_value_ead *value);
int ble_store_delete_ead(const struct ble_store_key_ead *key);
void ble_store_key_from_value_ead(struct ble_store_key_ead *out_key,
                                  const struct ble_store_value_ead *value);
#endif
/* irk store*/
int ble_store_read_local_irk(const struct ble_store_key_local_irk *key,
                             struct ble_store_value_local_irk *out_value);
int ble_store_write_local_irk(const struct ble_store_value_local_irk *value);
int ble_store_delete_local_irk(const struct ble_store_key_local_irk *key);
void ble_store_key_from_value_local_irk(struct ble_store_key_local_irk *out_key,
                                        const struct ble_store_value_local_irk *value);
/*irk store */
/* rpa mapping*/
int ble_store_read_rpa_rec(const struct ble_store_key_rpa_rec *key,
                           struct ble_store_value_rpa_rec *out_value);
int ble_store_write_rpa_rec(const struct ble_store_value_rpa_rec *value);
int ble_store_delete_rpa_rec(const struct ble_store_key_rpa_rec *key);
void ble_store_key_from_value_rpa_rec(struct ble_store_key_rpa_rec *out_key,
                                      const struct ble_store_value_rpa_rec *value);
/* rpa mapping*/

/**
 * @brief Generates a storage key from a value based on the object type.
 *
 * This function generates a storage key from a value based on the provided
 * object type.
 *
 * @param obj_type              The type of object for which the key is
 *                                  generated.
 * @param out_key               A pointer to a `ble_store_key` union where
 *                                  the generated key will be stored.
 * @param value                 A pointer to a `ble_store_value` union
 *                                  containing the value from which the key will
 *                                  be generated.
 */
void ble_store_key_from_value(int obj_type,
                              union ble_store_key *out_key,
                              const union ble_store_value *value);


/**
 * @brief Function signature for the storage iterator callback.
 *
 * This function signature represents a callback function used for iterating
 * over stored objects in a store.
 *
 * @param obj_type              The type of object being iterated.
 * @param val                   A pointer to the stored value of the object.
 * @param cookie                A user-defined pointer for additional data.
 *
 * @return                      0 to continue iterating;
 *                              Non-zero value to stop the iteration.
 */
typedef int ble_store_iterator_fn(int obj_type,
                                  union ble_store_value *val,
                                  void *cookie);


/**
 * @brief Iterates over stored objects of a specific type in a store.
 *
 * This function allows you to iterate over stored objects of a specific type in
 * the store and invoke a user-defined callback function for each object.
 *
 * @param obj_type              The type of objects to iterate over.
 * @param callback              A pointer to the user-defined callback function
 *                                  that will be invoked for each stored object.
 * @param cookie                A user-defined pointer for additional data to
 *                                  pass to the callback function.
 *
 * @return                      0 on success;
 *                              Non-zero on error.
 */
int ble_store_iterate(int obj_type,
                      ble_store_iterator_fn *callback,
                      void *cookie);


/**
 * @brief Clears all stored objects from a store.
 *
 * This function removes all stored objects from a store, effectively clearing
 * the storage.
 *
 * @return                      0 on success;
 *                              Non-zero on error.
 */
int ble_store_clear(void);

/**
 * @defgroup ble_store_util Bluetooth Store Utility Functions
 * @{
 */

/**
 * Retrieves the set of peer addresses for which a bond has been established.
 *
 * @param out_peer_id_addrs     The set of bonded peer addresses.
 * @param out_num_peers         The number of bonds that have been established.
 * @param max_peers             The capacity of the destination buffer.
 *
 * @return                      0 on success;
 *                              BLE_HS_ENOMEM if the destination buffer is too
 *                                  small;
 *                              Other non-zero on error.
 */
int ble_store_util_bonded_peers(ble_addr_t *out_peer_id_addrs,
                                int *out_num_peers,
                                int max_peers);

/**
 * Deletes all entries from a store that match the specified key.
 *
 * @param type                  The type of store entry to delete.
 * @param key                   Entries matching this key get deleted.
 *
 * @return                      0 on success;
 *                              Non-zero on error.
 */
int ble_store_util_delete_all(int type, const union ble_store_key *key);

/**
 * Deletes all entries from a store that are attached to the specified peer
 * address. This function deletes security entries and CCCD records.
 *
 * @param peer_id_addr          Entries with this peer address get deleted.
 *
 * @return                      0 on success;
 *                              Non-zero on error.
 */
int ble_store_util_delete_peer(const ble_addr_t *peer_id_addr);

/**
 * @brief Deletes the oldest peer from a store.
 *
 * This function deletes the oldest bonded peer from the storage. If there are
 * no bonded peers in the storage, the function returns success.
 *
 * @return                      0 on success;
 *                              Non-zero on error.
 */
int ble_store_util_delete_oldest_peer(void);

/**
 * @brief Counts the number of stored objects of a given type.
 *
 * This function counts the number of stored objects of a specific type in
 * a store and returns the count in the `out_count` parameter.
 *
 * @param type                  The type of the objects to count.
 * @param out_count             The count of stored objects.
 *
 * @return                      0 on success;
 *                              Non-zero on error.
 */
int ble_store_util_count(int type, int *out_count);

/**
 * @brief Round-robin status callback for handling store status events.
 *
 * This function handles store status events, particularly in cases where there
 * is insufficient storage capacity for new records.
 * It attempts to resolve overflow issues by deleting the oldest bond and
 * proceeds with the persist operation.
 *
 * @note This behavior may not be suitable for production use as it may lead to
 * removal of important bonds by less relevant peers. It is more useful for
 * demonstration purposes and sample applications.
 *
 * @param event                 A pointer to the store status event.
 * @param arg                   A pointer to additional user-defined arguments.
 *
 * @return                      0 on success;
 *                              Non-zero on error.
 */
int ble_store_util_status_rr(struct ble_store_status_event *event, void *arg);

/** @} */

#ifdef __cplusplus
}
#endif

/**
 * @}
 */

#endif
