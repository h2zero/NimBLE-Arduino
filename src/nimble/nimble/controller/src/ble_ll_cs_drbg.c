#ifndef ESP_PLATFORM

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

#include <syscfg/syscfg.h>
#if MYNEWT_VAL(BLE_LL_CHANNEL_SOUNDING)
#include <stdint.h>
#include <assert.h>
#include "nimble/porting/nimble/include/os/endian.h"
#include "nimble/nimble/include/nimble/ble.h"
#include "nimble/ext/tinycrypt/include/tinycrypt/aes.h"
#include "ble_ll_cs_drbg_priv.h"
#if !(BABBLESIM || MYNEWT_VAL(SELFTEST))
#include "nimble/nimble/controller/include/controller/ble_hw.h"
#endif

static const uint8_t rtt_seq_len[] = { 0, 4, 12, 4, 8, 12, 16 };

/**
 * Security function e generates 128-bit encrypted_data from a 128-bit key
 * and 128-bit data using the AES-128-bit block cypher.
 */
int
ble_ll_cs_drbg_e(const uint8_t *key, const uint8_t *data, uint8_t *out_data)
{
    struct ble_encryption_block ecb;
    int rc = 0;

    /* The cryptographic function uses the leftmost to rightmost
     * representation (MSO to LSO).
     */
    swap_buf(ecb.key, key, BLE_ENC_BLOCK_SIZE);
    swap_buf(ecb.plain_text, data, BLE_ENC_BLOCK_SIZE);

#if BABBLESIM || MYNEWT_VAL(SELFTEST)
    /* Use software to encrypt the data */
    mbedtls_aes_context aes_ctx;
    mbedtls_aes_init(&aes_ctx);
    mbedtls_aes_setkey_enc(&aes_ctx, ecb.key, 16 * 8);
    rc = mbedtls_aes_crypt_ecb(&aes_ctx, MBEDTLS_AES_ENCRYPT, ecb.plain_text,
                               ecb.cipher_text);
    mbedtls_aes_free(&aes_ctx);
#else
    /* Use hardware to encrypt the data */
    rc = ble_hw_encrypt_block(&ecb);
#endif

    if (!rc) {
        swap_buf(out_data, ecb.cipher_text, BLE_ENC_BLOCK_SIZE);
    } else {
        rc = -1;
    }

    return rc;
}

/**
 * DRBG chain function f7.
 * - k - 128-bit key
 * - in - an input bit string whose length is a multiple of 128 bits and
 *        generates an output that is 128 bits long using a cipher block
 *        chaining technique.
 * - out - processed bit string
 */
int
ble_ll_cs_drbg_f7(const uint8_t *k, const uint8_t *in, uint8_t len, uint8_t *out)
{
    int rc = 0;
    int i;
    const uint64_t *block;
    uint64_t *hout = (uint64_t *)out;

    memset(hout, 0, 16);

    /* Starting with the leftmost bits (MSO) of input_bit_string, split into
     * 128-bit blocks
     */
    for (i = len - 16; i >= 0; i -= 16) {
        block = (uint64_t *)&in[i];
        /* XOR a 128-bit block in two steps */
        *hout ^= *block;
        *(hout + 1) ^= *(block + 1);

        rc = ble_ll_cs_drbg_e(k, out, out);
        if (rc) {
            break;
        }
    }

    return rc;
}

/**
 * DRBG derivation function f8.
 * - input - 320-bit input bit string
 * - sm - output, generated 256-bit seed material (SM)
 */
int
ble_ll_cs_drbg_f8(const uint8_t *input, uint8_t *sm)
{
    int rc;
    uint8_t k[16] = { 0x0F, 0x0E, 0x0D, 0x0C, 0x0B, 0x0A, 0x09, 0x08,
                      0x07, 0x06, 0x05, 0x04, 0x03, 0x02, 0x01, 0x00 };
    uint8_t k2[16] = { 0 };
    uint8_t x[16] = { 0 };
    /* buf contains V || S */
    uint8_t buf[80] = { 0 };
    uint8_t *s = buf;
    uint8_t *v = buf + 64;

    /* S = 0x0000002800000020 || input_bit_string || 0x80 ||
     *     0x000000000000000000000000000000
     */
    s[15] = 0x80;
    memcpy(s + 16, input, 40);
    put_le64(s + 56, 0x0000002800000020);

    /* K2 = f7( K, V || S ) */
    rc = ble_ll_cs_drbg_f7(k, buf, 80, k2);
    if (rc) {
        return rc;
    }

    /* V = 0x00000001000000000000000000000000 */
    v[12] = 0x01;

    /* X = f7( K, V || S ) */
    rc = ble_ll_cs_drbg_f7(k, buf, 80, x);
    if (rc) {
        return rc;
    }

    /* Calculate the most significant part of SM:
     * SM = e( K2, X )
     */
    rc = ble_ll_cs_drbg_e(k2, x, sm + 16);
    if (rc) {
        return rc;
    }

    /* Calculate the least significant part of SM and concatenate
     * both parts:
     * SM = SM || e( K2, SM )
     */
    rc = ble_ll_cs_drbg_e(k2, sm + 16, sm);

    return rc;
}

/**
 * DRBG update function f9 is used to update and refresh a DRBG 128-bit
 * temporal key K and a 128-bit nonce vector V using a 256-bit seed material
 * (SM) that may carry fresh entropy. The SM value may also be 0 if f9 is
 * called for backtracking purposes.
 * - sm - 256-bit seed material
 * - k_in - 128-bit key
 * - v_in - 128-bit nonce vector
 */
int
ble_ll_cs_drbg_f9(const uint8_t *sm, uint8_t *k, uint8_t *v)
{
    int rc;
    uint8_t x[32] = { 0 };
    uint64_t *x_p = (uint64_t *)x;
    uint64_t *sm_p = (uint64_t *)sm;

    /* V = V[127:8] || (( V[7:0] + 1 ) mod 2^8) */
    v[0]++;
    rc = ble_ll_cs_drbg_e(k, v, x + 16);
    if (rc) {
        return rc;
    }

    v[0]++;
    /* Again V = V[127:8] || (( V[7:0] + 1 ) mod 2^8) */
    rc = ble_ll_cs_drbg_e(k, v, x);
    if (rc) {
        return rc;
    }

    /* X = X ⊕ SM */
    x_p[0] ^= sm_p[0];
    x_p[1] ^= sm_p[1];
    x_p[2] ^= sm_p[2];
    x_p[3] ^= sm_p[3];

    memcpy(v, x, 16);
    memcpy(k, x + 16, 16);

    return 0;
}

/**
 * DRBG instantiation function h9.
 * - iv - 128-bit initialization vector (CS_IV)
 * - in - 64-bit instantiation nonce (CS_IN)
 * - pv - 128-bit personalization vector (CS_PV)
 * - key - output, 128-bit temporal key (K_DRBG)
 * - nonce_v - output, 128-bit nonce vector (V_DRBG)
 */
int
ble_ll_cs_drbg_h9(const uint8_t *iv, const uint8_t *in, const uint8_t *pv,
                  uint8_t *key, uint8_t *nonce_v)
{
    int rc;
    uint8_t input_bit_string[40] = { 0 };
    uint8_t sm[32] = { 0 };

    /* 320-bit input bit string created from concatenated vectors
     * CS_IV || CS_IN || CS_PV
     */
    memcpy(input_bit_string, pv, 16);
    memcpy(input_bit_string + 16, in, 8);
    memcpy(input_bit_string + 24, iv, 16);

    /* Generate seed material (SM) */
    rc = ble_ll_cs_drbg_f8(input_bit_string, sm);
    if (rc) {
        return rc;
    }

    /* Generate K_DRBG and V_DRBG */
    memset(key, 0, 16);
    memset(nonce_v, 0, 16);
    rc = ble_ll_cs_drbg_f9(sm, key, nonce_v);

    return rc;
}

/**
 * Random bit generation function CS_DRBG
 * - transaction_id - CSTransactionID,
 * - drbg_ctx - the drbg context, already inited with keys,
 * - output - output buffer,
 * - len - number of bytes to be generated.
 */
int
ble_ll_cs_drbg_rand(struct ble_ll_cs_drbg_ctx *drbg_ctx, uint16_t step_count,
                    uint8_t transaction_id, uint8_t *output, uint8_t len)
{
    int rc;
    uint8_t rand_len = 0;
    uint8_t nonce[16];
    struct ble_ll_cs_transaction_cache *cache = &drbg_ctx->t_cache[transaction_id];

    /* Set the fixed values of the DRGB nonce */
    memcpy(nonce, drbg_ctx->nonce_v, sizeof(nonce));
    /* Set the Transaction_Counter */
    if (cache->last_step_count != step_count) {
        cache->transaction_counter = 0;
        cache->last_step_count = step_count;
    }
    nonce[0] += cache->transaction_counter;
    /* Set the Transaction_Identifier */
    nonce[1] += transaction_id;
    /* Set the CS Step_Counter */
    put_le16(&nonce[2], step_count + get_le16(&nonce[2]));

    while (len > 0) {
        if (cache->free_bytes > 0) {
            /* Use bytes from previous DRBG invocation */

            if (len < cache->free_bytes) {
                rand_len = len;
            } else {
                rand_len = cache->free_bytes;
            }

            cache->free_bytes -= rand_len;
            /* [0] is LSO, [15] is MSO. Return cached bytes starting from MSO. */
            memcpy(output, cache->random_bytes + cache->free_bytes, rand_len);

            len -= rand_len;
            output += rand_len;
        } else {
            /* Invoke CS_DRBG to get fresh 128-bit sequence */
            rc = ble_ll_cs_drbg_e(drbg_ctx->key, nonce, cache->random_bytes);
            if (rc) {
                return rc;
            }

            cache->free_bytes = sizeof(cache->random_bytes);
            /* Increment the transaction counter */
            ++nonce[0];
            ++cache->transaction_counter;
        }
    }

    return 0;
}

/** Channel Sounding random number generation function hr1 */
int
ble_ll_cs_drbg_rand_hr1(struct ble_ll_cs_drbg_ctx *drbg_ctx, uint16_t step_count,
                        uint8_t transaction_id, uint8_t r, uint8_t *r_out)
{
    int rc;
    uint16_t t_rand;
    uint8_t random_bits;

    if (r <= 1) {
        *r_out = 0;

        return 0;
    }

    rc = ble_ll_cs_drbg_rand(drbg_ctx, step_count, transaction_id, &random_bits, 1);
    if (rc) {
        return rc;
    }

    t_rand = r * random_bits;

    if ((t_rand & 0xFF) < (256 % r)) {
        rc = ble_ll_cs_drbg_rand(drbg_ctx, step_count, transaction_id,
                                 &random_bits, 1);
        *r_out = ((256 * random_bits * r) + t_rand) / 65536;
    } else {
        *r_out = t_rand / 256;
    }

    return rc;
}

int
ble_ll_cs_drbg_shuffle_cr1(struct ble_ll_cs_drbg_ctx *drbg_ctx, uint16_t step_count,
                           uint8_t transaction_id, uint8_t *channel_array,
                           uint8_t *shuffled_array, uint8_t len)
{
    int rc;
    uint8_t i, j;

    for (i = 0; i < len; ++i) {
        rc = ble_ll_cs_drbg_rand_hr1(drbg_ctx, step_count, transaction_id, i + 1, &j);
        if (rc) {
            return rc;
        }

        if (i != j) {
            shuffled_array[i] = shuffled_array[j];
        }

        shuffled_array[j] = channel_array[i];
    }

    return 0;
}

static uint32_t
cs_autocorrelation_score(uint32_t sequence)
{
    int c;
    uint32_t s, score = 0;
    uint8_t i, k;

    for (k = 1; k <= 3; ++k) {
        c = 0;
        s = sequence;
        for (i = 1; i <= 32 - k; ++i) {
            c += (s & 1) ^ ((s >> k) & 1);
            s >>= 1;
        }
        score += abs(2 * c - (32 - k));
    }

    return score;
}

int
ble_ll_cs_drbg_generate_aa(struct ble_ll_cs_drbg_ctx *drbg_ctx, uint16_t step_count,
                           uint32_t *initiator_aa, uint32_t *reflector_aa)
{
    int rc;
    uint8_t buf[16];
    uint32_t s0, s1, s2, s3;

    rc = ble_ll_cs_drbg_rand(drbg_ctx, step_count,
                             BLE_LL_CS_DRBG_ACCESS_ADDRESS, buf, sizeof(buf));

    if (rc) {
        return rc;
    }

    /* The buf[15] is the first generated octet */
    s0 = get_le32(&buf[12]);
    s1 = get_le32(&buf[8]);
    s2 = get_le32(&buf[4]);
    s3 = get_le32(&buf[0]);

    /* The sequence with the lower autocorrelation score is selected
     * as the CS Access Address. See 2.2.1 Channel Sounding Access Address
     * selection rules.
     */
    if (cs_autocorrelation_score(s0) < cs_autocorrelation_score(s1)) {
        *initiator_aa = s0;
    } else {
        *initiator_aa = s1;
    }

    if (cs_autocorrelation_score(s2) < cs_autocorrelation_score(s3)) {
        *reflector_aa = s2;
    } else {
        *reflector_aa = s3;
    }

    return 0;
}

int
ble_ll_cs_drbg_rand_marker_position(struct ble_ll_cs_drbg_ctx *drbg_ctx,
                                    uint16_t step_count, uint8_t rtt_type,
                                    uint8_t *position1, uint8_t *position2)
{
    int rc;
    uint8_t rand_range;

    if (rtt_type == BLE_LL_CS_RTT_32_BIT_SOUNDING_SEQUENCE) {
        rand_range = 29;
    } else { /* BLE_LL_CS_RTT_96_BIT_SOUNDING_SEQUENCE */
        rand_range = 64;
    }

    rc = ble_ll_cs_drbg_rand_hr1(drbg_ctx, step_count,
                                 BLE_LL_CS_DRBG_SEQ_MARKER_POSITION,
                                 rand_range, position1);
    if (rc) {
        return -1;
    }

    if (rtt_type == BLE_LL_CS_RTT_32_BIT_SOUNDING_SEQUENCE) {
        *position2 = 0xFF;

        return 0;
    }

    rc = ble_ll_cs_drbg_rand_hr1(drbg_ctx, step_count,
                                 BLE_LL_CS_DRBG_SEQ_MARKER_POSITION, 75, position2);
    if (rc) {
        return -1;
    }

    *position2 += 67;
    if (*position2 > 92) {
        /* Omit the second marker */
        *position2 = 0xFF;
    }

    return 0;
}

int
ble_ll_cs_drbg_rand_marker_selection(struct ble_ll_cs_drbg_ctx *drbg_ctx,
                                     uint8_t step_count, uint8_t *marker_selection)
{
    int rc;

    if (drbg_ctx->marker_selection_free_bits == 0) {
        rc = ble_ll_cs_drbg_rand(drbg_ctx, step_count, BLE_LL_CS_DRBG_SEQ_MARKER_SIGNAL,
                                 &drbg_ctx->marker_selection_cache, 1);
        if (rc) {
            return rc;
        }

        drbg_ctx->marker_selection_free_bits = 8;
    }

    *marker_selection = drbg_ctx->marker_selection_cache & 0x80;
    drbg_ctx->marker_selection_cache <<= 1;
    --drbg_ctx->marker_selection_free_bits;

    return 0;
}

int
ble_ll_cs_drbg_rand_main_mode_steps(struct ble_ll_cs_drbg_ctx *drbg_ctx,
                                    uint8_t step_count, uint8_t main_mode_min_steps,
                                    uint8_t main_mode_max_steps,
                                    uint8_t *main_mode_steps)
{
    int rc;
    uint8_t r;
    uint8_t r_out;

    r = main_mode_max_steps - main_mode_min_steps + 1;
    rc = ble_ll_cs_drbg_rand_hr1(drbg_ctx, step_count,
                                 BLE_LL_CS_DRBG_SUBEVT_SUBMODE, r, &r_out);
    if (rc) {
        return rc;
    }

    *main_mode_steps = r_out + main_mode_min_steps;

    return 0;
}

static int
ble_ll_cs_drbg_apply_marker_signal(struct ble_ll_cs_drbg_ctx *drbg_ctx,
                                   uint8_t step_count, uint8_t *buf, uint8_t position)
{
    int rc;
    uint16_t *byte_ptr;
    uint16_t marker_signal;
    uint8_t marker_selection;
    uint8_t byte_id = 0;
    uint8_t bit_offset = 0;

    rc = ble_ll_cs_drbg_rand_marker_selection(drbg_ctx, step_count, &marker_selection);
    if (rc) {
        return rc;
    }

    if (marker_selection) {
        /* '0011' in transmission order */
        marker_signal = 0b1100;
    } else {
        /* '1100' in transmission order */
        marker_signal = 0b0011;
    }

    byte_id = position / 8;
    byte_ptr = (uint16_t *)&buf[byte_id];
    bit_offset = position % 8;
    *byte_ptr &= ~(0xF << bit_offset);
    *byte_ptr |= ~(marker_signal << bit_offset);

    return 0;
}

static int
ble_ll_cs_generate_sounding_sequence(struct ble_ll_cs_drbg_ctx *drbg_ctx,
                                     uint8_t step_count, uint8_t rtt_type,
                                     uint8_t *buf, uint8_t sequence_len)
{
    int rc;
    uint8_t i;
    uint8_t position1;
    uint8_t position2;

    for (i = 0; i < sequence_len; ++i) {
        buf[i] = 0b10101010;
    }

    rc = ble_ll_cs_drbg_rand_marker_position(drbg_ctx, step_count, rtt_type,
                                             &position1, &position2);
    if (rc) {
        return rc;
    }

    rc = ble_ll_cs_drbg_apply_marker_signal(drbg_ctx, step_count, buf, position1);
    if (rc) {
        return rc;
    }

    if (position2 != 0xFF) {
        rc = ble_ll_cs_drbg_apply_marker_signal(drbg_ctx, step_count, buf, position2);
    }

    return rc;
}

int
ble_ll_cs_drbg_generate_sync_sequence(struct ble_ll_cs_drbg_ctx *drbg_ctx,
                                      uint8_t step_count, uint8_t rtt_type,
                                      uint8_t *buf, uint8_t *sequence_len)
{
    int rc = -1;

    *sequence_len = rtt_seq_len[rtt_type];

    switch (rtt_type) {
    case BLE_LL_CS_RTT_32_BIT_SOUNDING_SEQUENCE:
        rc = ble_ll_cs_generate_sounding_sequence(drbg_ctx, step_count,
                                                  rtt_type, buf, *sequence_len);
        break;
    case BLE_LL_CS_RTT_96_BIT_SOUNDING_SEQUENCE:
        rc = ble_ll_cs_generate_sounding_sequence(drbg_ctx, step_count,
                                                  rtt_type, buf, *sequence_len);
        break;
    case BLE_LL_CS_RTT_32_BIT_RANDOM_SEQUENCE:
    case BLE_LL_CS_RTT_64_BIT_RANDOM_SEQUENCE:
    case BLE_LL_CS_RTT_96_BIT_RANDOM_SEQUENCE:
    case BLE_LL_CS_RTT_128_BIT_RANDOM_SEQUENCE:
        rc = ble_ll_cs_drbg_rand(drbg_ctx, step_count,
                                 BLE_LL_CS_DRBG_RAND_SEQ_GENERATION, buf,
                                 *sequence_len);
        break;
    default:
        break;
    }

    return rc;
}

int
ble_ll_cs_drbg_rand_tone_ext_presence(struct ble_ll_cs_drbg_ctx *drbg_ctx,
                                      uint8_t step_count, uint8_t *presence)
{
    int rc;

    if (drbg_ctx->tone_ext_presence_free_bits == 0) {
        rc = ble_ll_cs_drbg_rand(drbg_ctx, step_count,
                                 BLE_LL_CS_DRBG_T_PM_TONE_SLOT_PRESENCE,
                                 &drbg_ctx->tone_ext_presence_cache, 1);
        if (rc) {
            return rc;
        }

        drbg_ctx->tone_ext_presence_free_bits = 8;
    }

    *presence = drbg_ctx->tone_ext_presence_cache & 0x80 ? 1 : 0;
    drbg_ctx->tone_ext_presence_cache <<= 1;
    --drbg_ctx->tone_ext_presence_free_bits;

    return 0;
}

int
ble_ll_cs_drbg_rand_antenna_path_perm_id(struct ble_ll_cs_drbg_ctx *drbg_ctx,
                                         uint16_t step_count, uint8_t n_ap,
                                         uint8_t *ap_id)
{
    int rc;
    uint8_t i;
    uint8_t n_ap_f = 0;

    if (n_ap <= 1) {
        *ap_id = 0;

        return 0;
    }

    assert(n_ap <= 4);

    for (i = 1; i <= n_ap; ++i) {
        n_ap_f *= i;
    }

    rc = ble_ll_cs_drbg_rand_hr1(drbg_ctx, step_count,
                                 BLE_LL_CS_DRBG_ANTENNA_PATH_PERMUTATION,
                                 n_ap_f, ap_id);

    return rc;
}

void
ble_ll_cs_drbg_clear_cache(struct ble_ll_cs_drbg_ctx *drbg_ctx)
{
    memset(drbg_ctx->t_cache, 0, sizeof(drbg_ctx->t_cache));
    drbg_ctx->marker_selection_cache = 0;
    drbg_ctx->marker_selection_free_bits = 0;
}

int
ble_ll_cs_drbg_init(struct ble_ll_cs_drbg_ctx *drbg_ctx)
{
    /* Calculate temporal key K and nonce vector V */
    return ble_ll_cs_drbg_h9(drbg_ctx->iv, drbg_ctx->in, drbg_ctx->pv,
                             drbg_ctx->key, drbg_ctx->nonce_v);
}

void
ble_ll_cs_drbg_free(struct ble_ll_cs_drbg_ctx *drbg_ctx)
{
    memset(drbg_ctx, 0, sizeof(*drbg_ctx));
}
#endif /* BLE_LL_CHANNEL_SOUNDING */

#endif /* ESP_PLATFORM */
