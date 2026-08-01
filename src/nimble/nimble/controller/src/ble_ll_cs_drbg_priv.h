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

#ifndef H_BLE_LL_CS_DRBG_PRIV
#define H_BLE_LL_CS_DRBG_PRIV

#ifdef __cplusplus
extern "C" {
#endif

#define BLE_LL_CS_DRBG_HOP_CHAN_NON_MODE0       (0x00)
#define BLE_LL_CS_DRBG_HOP_CHAN_MODE0           (0x01)
#define BLE_LL_CS_DRBG_SUBEVT_SUBMODE           (0x02)
#define BLE_LL_CS_DRBG_T_PM_TONE_SLOT_PRESENCE  (0x03)
#define BLE_LL_CS_DRBG_ANTENNA_PATH_PERMUTATION (0x04)
#define BLE_LL_CS_DRBG_ACCESS_ADDRESS           (0x05)
#define BLE_LL_CS_DRBG_SEQ_MARKER_POSITION      (0x06)
#define BLE_LL_CS_DRBG_SEQ_MARKER_SIGNAL        (0x07)
#define BLE_LL_CS_DRBG_RAND_SEQ_GENERATION      (0x08)
#define BLE_LL_CS_DRBG_BACKTRACKING_RESISTANCE  (0x09)
#define BLE_LL_CS_DRBG_TRANSACTION_IDS_NUMBER   (0x0a)

#define BLE_LL_CS_RTT_AA_ONLY                  (0x00)
#define BLE_LL_CS_RTT_32_BIT_SOUNDING_SEQUENCE (0x01)
#define BLE_LL_CS_RTT_96_BIT_SOUNDING_SEQUENCE (0x02)
#define BLE_LL_CS_RTT_32_BIT_RANDOM_SEQUENCE   (0x03)
#define BLE_LL_CS_RTT_64_BIT_RANDOM_SEQUENCE   (0x04)
#define BLE_LL_CS_RTT_96_BIT_RANDOM_SEQUENCE   (0x05)
#define BLE_LL_CS_RTT_128_BIT_RANDOM_SEQUENCE  (0x06)

struct ble_ll_cs_transaction_cache {
    uint16_t last_step_count;
    /* 1-octet CSTransactionCounter per each CS Transaction ID. Should
     * be reset each time the nonce V CS Step_Counter field is set to
     * a new value.
     */
    uint8_t transaction_counter;
    /* Random bits cache */
    uint8_t random_bytes[16];
    /* The number of cached bytes that have been already used */
    uint8_t free_bytes;
};

/* DRBG context */
struct ble_ll_cs_drbg_ctx {
    /* Initialization vector, entropy input */
    uint8_t iv[16];
    /* Instantiation nonce */
    uint8_t in[8];
    /* Personalization vector/string */
    uint8_t pv[16];
    /* Temporal key K */
    uint8_t key[16];
    /* DRBG nonce/counter (V), the starting value from which the DRBG operates.
     * Initialized once per LE Connection.
     */
    uint8_t nonce_v[16];
    /* Cache bits generated with single DRBG transation */
    struct ble_ll_cs_transaction_cache t_cache[BLE_LL_CS_DRBG_TRANSACTION_IDS_NUMBER];

    uint8_t marker_selection_free_bits;
    uint8_t marker_selection_cache;

    uint8_t tone_ext_presence_free_bits;
    uint8_t tone_ext_presence_cache;
};

int ble_ll_cs_drbg_e(const uint8_t *key, const uint8_t *data, uint8_t *out_data);
int ble_ll_cs_drbg_f7(const uint8_t *k, const uint8_t *in, uint8_t len, uint8_t *out);
int ble_ll_cs_drbg_f8(const uint8_t *input, uint8_t *sm);
int ble_ll_cs_drbg_f9(const uint8_t *sm, uint8_t *k, uint8_t *v);
int ble_ll_cs_drbg_h9(const uint8_t *iv, const uint8_t *in, const uint8_t *pv,
                      uint8_t *key, uint8_t *nonce_v);
int ble_ll_cs_drbg_rand(struct ble_ll_cs_drbg_ctx *drbg_ctx, uint16_t step_count,
                        uint8_t transaction_id, uint8_t *output, uint8_t len);
int ble_ll_cs_drbg_rand_hr1(struct ble_ll_cs_drbg_ctx *drbg_ctx, uint16_t step_count,
                            uint8_t transaction_id, uint8_t r, uint8_t *r_out);
int ble_ll_cs_drbg_shuffle_cr1(struct ble_ll_cs_drbg_ctx *drbg_ctx, uint16_t step_count,
                               uint8_t transaction_id, uint8_t *channel_array,
                               uint8_t *shuffled_array, uint8_t len);
int ble_ll_cs_drbg_generate_aa(struct ble_ll_cs_drbg_ctx *drbg_ctx, uint16_t step_count,
                               uint32_t *initiator_aa, uint32_t *reflector_aa);
int ble_ll_cs_drbg_rand_marker_position(struct ble_ll_cs_drbg_ctx *drbg_ctx,
                                        uint16_t step_count, uint8_t rtt_type,
                                        uint8_t *position1, uint8_t *position2);
int ble_ll_cs_drbg_rand_marker_selection(struct ble_ll_cs_drbg_ctx *drbg_ctx,
                                         uint8_t step_count,
                                         uint8_t *marker_selection);
int ble_ll_cs_drbg_rand_main_mode_steps(struct ble_ll_cs_drbg_ctx *drbg_ctx,
                                        uint8_t step_count, uint8_t main_mode_min_steps,
                                        uint8_t main_mode_max_steps,
                                        uint8_t *main_mode_steps);
int ble_ll_cs_drbg_generate_sync_sequence(struct ble_ll_cs_drbg_ctx *drbg_ctx,
                                          uint8_t step_count, uint8_t rtt_type,
                                          uint8_t *buf, uint8_t *sequence_len);
int ble_ll_cs_drbg_rand_antenna_path_perm_id(struct ble_ll_cs_drbg_ctx *drbg_ctx,
                                             uint16_t step_count, uint8_t n_ap,
                                             uint8_t *ap_id);
int ble_ll_cs_drbg_rand_tone_ext_presence(struct ble_ll_cs_drbg_ctx *drbg_ctx,
                                          uint8_t step_count, uint8_t *presence);
int ble_ll_cs_drbg_init(struct ble_ll_cs_drbg_ctx *drbg_ctx);
void ble_ll_cs_drbg_free(struct ble_ll_cs_drbg_ctx *drbg_ctx);
void ble_ll_cs_drbg_clear_cache(struct ble_ll_cs_drbg_ctx *drbg_ctx);

#ifdef __cplusplus
}
#endif

#endif /* H_BLE_LL_CS_DRBG_PRIV */
