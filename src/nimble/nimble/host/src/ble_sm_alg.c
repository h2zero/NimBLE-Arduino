/*
 * Copyright (c) 2015 Intel Corporation
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

/* These functions are adapted from the Intel Zephyr BLE security manager
 * code.
 */

#include <inttypes.h>
#include <string.h>
#include "syscfg/syscfg.h"
#include "nimble/nimble/include/nimble/nimble_opt.h"

#if NIMBLE_BLE_CONNECT
#if NIMBLE_BLE_SM

#include "nimble/nimble/include/nimble/ble.h"
#include "ble_hs_priv.h"

#if MYNEWT_VAL(BLE_CRYPTO_STACK_MBEDTLS)
#if CONFIG_MBEDTLS_VER_4_X_SUPPORT
#include <psa/crypto.h>
#else
#include <mbedtls/aes.h>
#if MYNEWT_VAL(BLE_SM_SC)
#include <mbedtls/cmac.h>
#include <mbedtls/ecp.h>
#include <mbedtls/ecdh.h>
#endif // MYNEWT_VAL(BLE_SM_SC)
#endif // CONFIG_MBEDTLS_VER_4_X_SUPPORT
#else  // !MYNEWT_VAL(BLE_CRYPTO_STACK_MBEDTLS)
#include "nimble/ext/tinycrypt/include/tinycrypt/aes.h"
#include "nimble/ext/tinycrypt/include/tinycrypt/constants.h"
#include "nimble/ext/tinycrypt/include/tinycrypt/utils.h"

#if MYNEWT_VAL(BLE_SM_SC)
#include "nimble/ext/tinycrypt/include/tinycrypt/cmac_mode.h"
#include "nimble/ext/tinycrypt/include/tinycrypt/ecc_dh.h"
#endif
#endif // MYNEWT_VAL(BLE_CRYPTO_STACK_MBEDTLS)

#if !CONFIG_MBEDTLS_VER_4_X_SUPPORT
#if MYNEWT_VAL(TRNG)
#include "trng/trng.h"
#endif

#if MYNEWT_VAL(BLE_SM_SC) && MYNEWT_VAL(TRNG)
static struct trng_dev *g_trng;
#endif
#endif // !CONFIG_MBEDTLS_VER_4_X_SUPPORT

// Helper to prevent optimization when zeroing memory
static inline void ble_sm_alg_secure_zero(void *v, size_t n) {
#if MYNEWT_VAL(BLE_CRYPTO_STACK_MBEDTLS)
    mbedtls_platform_zeroize(v, n);
#else
    if (v == NULL || n == 0) {
        return;
    }

    volatile unsigned char *p = (volatile unsigned char *)v;
    while (n--) {
        *p++ = 0;
    }

    // Compiler memory barrier
    __asm__ __volatile__("" : : "r"(v) : "memory");
#endif
}

static void
ble_sm_alg_xor_128(const uint8_t *p, const uint8_t *q, uint8_t *r)
{
    int i;

    for (i = 0; i < 16; i++) {
        r[i] = p[i] ^ q[i];
    }
}

int
ble_sm_alg_encrypt(const uint8_t *key, const uint8_t *plaintext,
                   uint8_t *enc_data)
{
    int ret = 0;
    uint8_t tmp[16];

    swap_buf(tmp, key, 16);

#if MYNEWT_VAL(BLE_CRYPTO_STACK_MBEDTLS)
#if CONFIG_MBEDTLS_VER_4_X_SUPPORT
    psa_status_t status;
    psa_key_attributes_t key_attributes = PSA_KEY_ATTRIBUTES_INIT;
    psa_key_id_t key_id = 0;
    psa_set_key_usage_flags(&key_attributes, PSA_KEY_USAGE_ENCRYPT);
    psa_set_key_algorithm(&key_attributes, PSA_ALG_ECB_NO_PADDING);
    psa_set_key_type(&key_attributes, PSA_KEY_TYPE_AES);
    psa_set_key_bits(&key_attributes, 128);

    status = psa_import_key(&key_attributes, tmp, 16, &key_id);
    if (status != PSA_SUCCESS) {
        ret = BLE_HS_EUNKNOWN;
        goto done;
    }

    psa_reset_key_attributes(&key_attributes);
    swap_buf(tmp, plaintext, 16);
    size_t output_len = 0;
    status = psa_cipher_encrypt(key_id, PSA_ALG_ECB_NO_PADDING, tmp,
                                16, enc_data, 16, &output_len);
    if (status != PSA_SUCCESS || output_len != 16) {
        psa_destroy_key(key_id);
        ret = BLE_HS_EUNKNOWN;
        goto done;
    }

    psa_destroy_key(key_id);

#else
    mbedtls_aes_context s = {0};

    mbedtls_aes_init(&s);
    if (mbedtls_aes_setkey_enc(&s, tmp, 128) != 0) {
        mbedtls_aes_free(&s);
        ret = BLE_HS_EUNKNOWN;
        goto done;
    }

    swap_buf(tmp, plaintext, 16);

    if (mbedtls_aes_crypt_ecb(&s, MBEDTLS_AES_ENCRYPT, tmp, enc_data) != 0) {
        mbedtls_aes_free(&s);
        ret = BLE_HS_EUNKNOWN;
        goto done;
    }

    mbedtls_aes_free(&s);
#endif /* CONFIG_MBEDTLS_VER_4_X_SUPPORT */
#else /* !MYNEWT_VAL(BLE_CRYPTO_STACK_MBEDTLS) */
    struct tc_aes_key_sched_struct s;

    if (tc_aes128_set_encrypt_key(&s, tmp) == TC_CRYPTO_FAIL) {
        ble_sm_alg_secure_zero(&s, sizeof(s));
        ret = BLE_HS_EUNKNOWN;
        goto done;
    }

    swap_buf(tmp, plaintext, 16);

    if (tc_aes_encrypt(enc_data, tmp, &s) == TC_CRYPTO_FAIL) {
        ble_sm_alg_secure_zero(&s, sizeof(s));
        ret = BLE_HS_EUNKNOWN;
        goto done;
    }

    ble_sm_alg_secure_zero(&s, sizeof(s));
#endif

    swap_in_place(enc_data, 16);

done:
    ble_sm_alg_secure_zero(tmp, sizeof(tmp));
    return ret;
}

int
ble_sm_alg_s1(const uint8_t *k, const uint8_t *r1, const uint8_t *r2,
              uint8_t *out)
{
    int rc;

    /* The most significant 64-bits of r1 are discarded to generate
     * r1' and the most significant 64-bits of r2 are discarded to
     * generate r2'.
     * r1' is concatenated with r2' to generate r' which is used as
     * the 128-bit input parameter plaintextData to security function e:
     *
     *    r' = r1' || r2'
     */
    memcpy(out, r2, 8);
    memcpy(out + 8, r1, 8);

    /* s1(k, r1 , r2) = e(k, r') */
    rc = ble_sm_alg_encrypt(k, out, out);
    if (rc != 0) {
        return rc;
    }

    BLE_HS_LOG(DEBUG, "ble_sm_alg_s1()\n    k=");
    ble_hs_log_flat_buf(k, 16);
    BLE_HS_LOG(DEBUG, "\n    r1=");
    ble_hs_log_flat_buf(r1, 16);
    BLE_HS_LOG(DEBUG, "\n    r2=");
    ble_hs_log_flat_buf(r2, 16);
    BLE_HS_LOG(DEBUG, "\n    out=");
    ble_hs_log_flat_buf(out, 16);
    BLE_HS_LOG(DEBUG, "\n");

    return 0;
}

int
ble_sm_alg_c1(const uint8_t *k, const uint8_t *r,
              const uint8_t *preq, const uint8_t *pres,
              uint8_t iat, uint8_t rat,
              const uint8_t *ia, const uint8_t *ra,
              uint8_t *out_enc_data)
{
    uint8_t p1[16], p2[16];
    int rc;

    BLE_HS_LOG(DEBUG, "ble_sm_alg_c1()\n    k=");
    ble_hs_log_flat_buf(k, 16);
    BLE_HS_LOG(DEBUG, "\n    r=");
    ble_hs_log_flat_buf(r, 16);
    BLE_HS_LOG(DEBUG, "\n    iat=%d rat=%d", iat, rat);
    BLE_HS_LOG(DEBUG, "\n    ia=");
    ble_hs_log_flat_buf(ia, 6);
    BLE_HS_LOG(DEBUG, "\n    ra=");
    ble_hs_log_flat_buf(ra, 6);
    BLE_HS_LOG(DEBUG, "\n    preq=");
    ble_hs_log_flat_buf(preq, 7);
    BLE_HS_LOG(DEBUG, "\n    pres=");
    ble_hs_log_flat_buf(pres, 7);

    /* pres, preq, rat and iat are concatenated to generate p1 */
    p1[0] = iat;
    p1[1] = rat;
    memcpy(p1 + 2, preq, 7);
    memcpy(p1 + 9, pres, 7);

    BLE_HS_LOG(DEBUG, "\n    p1=");
    ble_hs_log_flat_buf(p1, sizeof p1);

    /* c1 = e(k, e(k, r XOR p1) XOR p2) */

    /* Using out_enc_data as temporary output buffer */
    ble_sm_alg_xor_128(r, p1, out_enc_data);

    rc = ble_sm_alg_encrypt(k, out_enc_data, out_enc_data);
    if (rc != 0) {
        rc = BLE_HS_EUNKNOWN;
        goto done;
    }

    /* ra is concatenated with ia and padding to generate p2 */
    memcpy(p2, ra, 6);
    memcpy(p2 + 6, ia, 6);
    memset(p2 + 12, 0, 4);

    BLE_HS_LOG(DEBUG, "\n    p2=");
    ble_hs_log_flat_buf(p2, sizeof p2);

    ble_sm_alg_xor_128(out_enc_data, p2, out_enc_data);

    rc = ble_sm_alg_encrypt(k, out_enc_data, out_enc_data);
    if (rc != 0) {
        rc = BLE_HS_EUNKNOWN;
        goto done;
    }

    BLE_HS_LOG(DEBUG, "\n    out_enc_data=");
    ble_hs_log_flat_buf(out_enc_data, 16);

    rc = 0;

done:
    BLE_HS_LOG(DEBUG, "\n    rc=%d\n", rc);
    return rc;
}

#if MYNEWT_VAL(BLE_SM_SC)

static void
ble_sm_alg_log_buf(const char *name, const uint8_t *buf, int len)
{
    BLE_HS_LOG(DEBUG, "    %s=", name);
    ble_hs_log_flat_buf(buf, len);
    BLE_HS_LOG(DEBUG, "\n");
}

/**
 * Cypher based Message Authentication Code (CMAC) with AES 128 bit
 *
 * @param key                   128-bit key.
 * @param in                    Message to be authenticated.
 * @param len                   Length of the message in octets.
 * @param out                   Output; message authentication code.
 */
int
ble_sm_alg_aes_cmac(const uint8_t *key, const uint8_t *in, size_t len,
                    uint8_t *out)
{
#if MYNEWT_VAL(BLE_CRYPTO_STACK_MBEDTLS)
#if CONFIG_MBEDTLS_VER_4_X_SUPPORT
    psa_status_t status;
    psa_key_attributes_t key_attributes = PSA_KEY_ATTRIBUTES_INIT;
    psa_key_id_t key_id = 0;
    psa_algorithm_t alg = PSA_ALG_CMAC;
    size_t output_len = 0;

    psa_set_key_usage_flags(&key_attributes, PSA_KEY_USAGE_SIGN_MESSAGE);
    psa_set_key_algorithm(&key_attributes, PSA_ALG_CMAC);
    psa_set_key_type(&key_attributes, PSA_KEY_TYPE_AES);
    psa_set_key_bits(&key_attributes, 128);

    status = psa_import_key(&key_attributes, key, 16, &key_id);
    if (status != PSA_SUCCESS) {
        return BLE_HS_EUNKNOWN;
    }

    psa_reset_key_attributes(&key_attributes);

    psa_mac_operation_t operation = PSA_MAC_OPERATION_INIT;
    status = psa_mac_sign_setup(&operation, key_id, alg);
    if (status != PSA_SUCCESS) {
        psa_destroy_key(key_id);
        return BLE_HS_EUNKNOWN;
    }

    status = psa_mac_update(&operation, in, len);
    if (status == PSA_SUCCESS) {
        status = psa_mac_sign_finish(&operation, out, 16, &output_len);
    }

    psa_mac_abort(&operation);
    psa_destroy_key(key_id);
    return (status == PSA_SUCCESS && output_len == 16) ? 0 : BLE_HS_EUNKNOWN;
#else
    if (mbedtls_cipher_cmac(mbedtls_cipher_info_from_type(MBEDTLS_CIPHER_AES_128_ECB),
                            key, 128, in, len, out)) {
        return BLE_HS_EUNKNOWN;
    }

    return 0;
#endif // CONFIG_MBEDTLS_VER_4_X_SUPPORT
#else
    int rc = 0;
    struct tc_aes_key_sched_struct sched;
    struct tc_cmac_struct state;

    rc = (tc_cmac_setup(&state, key, &sched) == TC_CRYPTO_FAIL) ? BLE_HS_EUNKNOWN : 0;

    if (rc == 0) {
        rc = (tc_cmac_update(&state, in, len) == TC_CRYPTO_FAIL) ? BLE_HS_EUNKNOWN : 0;
    }

    if (rc == 0) {
        rc = (tc_cmac_final(out, &state) == TC_CRYPTO_FAIL) ? BLE_HS_EUNKNOWN : 0;
    }

    ble_sm_alg_secure_zero(&sched, sizeof(sched));
    ble_sm_alg_secure_zero(&state, sizeof(state));
    return rc;
#endif // MYNEWT_VAL(BLE_CRYPTO_STACK_MBEDTLS)
}

int
ble_sm_alg_f4(const uint8_t *u, const uint8_t *v, const uint8_t *x,
              uint8_t z, uint8_t *out_enc_data)
{
    uint8_t xs[16];
    uint8_t m[65];
    int rc;

    BLE_HS_LOG(DEBUG, "ble_sm_alg_f4()\n    u=");
    ble_hs_log_flat_buf(u, 32);
    BLE_HS_LOG(DEBUG, "\n    v=");
    ble_hs_log_flat_buf(v, 32);
    BLE_HS_LOG(DEBUG, "\n    x=");
    ble_hs_log_flat_buf(x, 16);
    BLE_HS_LOG(DEBUG, "\n    z=0x%02x\n", z);

    /*
     * U, V and Z are concatenated and used as input m to the function
     * AES-CMAC and X is used as the key k.
     *
     * Core Spec 4.2 Vol 3 Part H 2.2.5
     *
     * note:
     * ble_sm_alg_aes_cmac uses BE data; ble_sm_alg_f4 accepts LE so we swap.
     */
    swap_buf(m, u, 32);
    swap_buf(m + 32, v, 32);
    m[64] = z;

    swap_buf(xs, x, 16);

    rc = ble_sm_alg_aes_cmac(xs, m, sizeof(m), out_enc_data);
    if (rc != 0) {
        return BLE_HS_EUNKNOWN;
    }

    swap_in_place(out_enc_data, 16);

    BLE_HS_LOG(DEBUG, "    out_enc_data=");
    ble_hs_log_flat_buf(out_enc_data, 16);
    BLE_HS_LOG(DEBUG, "\n");

    return 0;
}

int
ble_sm_alg_f5(const uint8_t *w, const uint8_t *n1, const uint8_t *n2,
              uint8_t a1t, const uint8_t *a1, uint8_t a2t, const uint8_t *a2,
              uint8_t *mackey, uint8_t *ltk)
{
    static const uint8_t salt[16] = { 0x6c, 0x88, 0x83, 0x91, 0xaa, 0xf5,
                      0xa5, 0x38, 0x60, 0x37, 0x0b, 0xdb,
                      0x5a, 0x60, 0x83, 0xbe };
    uint8_t m[53] = {
        0x00, /* counter */
        0x62, 0x74, 0x6c, 0x65, /* keyID */
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, /*n1*/
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, /*2*/
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, /* a1 */
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, /* a2 */
        0x01, 0x00 /* length */
    };
    uint8_t ws[32];
    uint8_t t[16];
    int rc;

    BLE_HS_LOG(DEBUG, "ble_sm_alg_f5()\n");
    ble_sm_alg_log_buf("w", w, 32);
    ble_sm_alg_log_buf("n1", n1, 16);
    ble_sm_alg_log_buf("n2", n2, 16);

    swap_buf(ws, w, 32);

    rc = ble_sm_alg_aes_cmac(salt, ws, 32, t);
    if (rc != 0) {
        return BLE_HS_EUNKNOWN;
    }

    ble_sm_alg_log_buf("t", t, 16);

    swap_buf(m + 5, n1, 16);
    swap_buf(m + 21, n2, 16);
    m[37] = a1t;
    swap_buf(m + 38, a1, 6);
    m[44] = a2t;
    swap_buf(m + 45, a2, 6);

    rc = ble_sm_alg_aes_cmac(t, m, sizeof(m), mackey);
    if (rc != 0) {
        return BLE_HS_EUNKNOWN;
    }

    ble_sm_alg_log_buf("mackey", mackey, 16);

    swap_in_place(mackey, 16);

    /* Counter for ltk is 1. */
    m[0] = 0x01;

    rc = ble_sm_alg_aes_cmac(t, m, sizeof(m), ltk);
    if (rc != 0) {
        return BLE_HS_EUNKNOWN;
    }

    ble_sm_alg_log_buf("ltk", ltk, 16);

    swap_in_place(ltk, 16);

    return 0;
}

int
ble_sm_alg_f6(const uint8_t *w, const uint8_t *n1, const uint8_t *n2,
              const uint8_t *r, const uint8_t *iocap, uint8_t a1t,
              const uint8_t *a1, uint8_t a2t, const uint8_t *a2,
              uint8_t *check)
{
    uint8_t ws[16];
    uint8_t m[65];
    int rc;

    BLE_HS_LOG(DEBUG, "ble_sm_alg_f6()\n");
    ble_sm_alg_log_buf("w", w, 16);
    ble_sm_alg_log_buf("n1", n1, 16);
    ble_sm_alg_log_buf("n2", n2, 16);
    ble_sm_alg_log_buf("r", r, 16);
    ble_sm_alg_log_buf("iocap", iocap, 3);
    ble_sm_alg_log_buf("a1t", &a1t, 1);
    ble_sm_alg_log_buf("a1", a1, 6);
    ble_sm_alg_log_buf("a2t", &a2t, 1);
    ble_sm_alg_log_buf("a2", a2, 6);

    swap_buf(m, n1, 16);
    swap_buf(m + 16, n2, 16);
    swap_buf(m + 32, r, 16);
    swap_buf(m + 48, iocap, 3);

    m[51] = a1t;
    memcpy(m + 52, a1, 6);
    swap_buf(m + 52, a1, 6);

    m[58] = a2t;
    memcpy(m + 59, a2, 6);
    swap_buf(m + 59, a2, 6);

    swap_buf(ws, w, 16);

    rc = ble_sm_alg_aes_cmac(ws, m, sizeof(m), check);
    if (rc != 0) {
        return BLE_HS_EUNKNOWN;
    }

    ble_sm_alg_log_buf("res", check, 16);

    swap_in_place(check, 16);

    return 0;
}

int
ble_sm_alg_g2(const uint8_t *u, const uint8_t *v, const uint8_t *x,
              const uint8_t *y, uint32_t *passkey)
{
    uint8_t m[80], xs[16];
    int rc;

    BLE_HS_LOG(DEBUG, "ble_sm_alg_g2()\n");
    ble_sm_alg_log_buf("u", u, 32);
    ble_sm_alg_log_buf("v", v, 32);
    ble_sm_alg_log_buf("x", x, 16);
    ble_sm_alg_log_buf("y", y, 16);

    swap_buf(m, u, 32);
    swap_buf(m + 32, v, 32);
    swap_buf(m + 64, y, 16);

    swap_buf(xs, x, 16);

    /* reuse xs (key) as buffer for result */
    rc = ble_sm_alg_aes_cmac(xs, m, sizeof(m), xs);
    if (rc != 0) {
        return BLE_HS_EUNKNOWN;
    }

    ble_sm_alg_log_buf("res", xs, 16);

    *passkey = get_be32(xs + 12) % 1000000;
    BLE_HS_LOG(DEBUG, "    passkey=%u\n", *passkey);

    return 0;
}

int
ble_sm_alg_csis_k1(const uint8_t *n, size_t n_len, const uint8_t *salt,
                   const uint8_t *p, size_t p_len, uint8_t *out)
{
    int rc;
    uint8_t t[16] = {0};
    uint8_t salt_be[16] = {0};
    uint8_t n_be[16] = {0};

    /* XXX: Spec does not specify the maximum N and P parameters length.
     * We assume that 16 bytes is enough and return error if passed len value is greater
     * than that */
    if ((n_len > 16) || (p_len > 16)) {
        return BLE_HS_EINVAL;
    }

    swap_buf(salt_be, salt, 16);
    swap_buf(n_be, n, n_len);

    /* T = AES-CMAC_SALT (N) */
    rc = ble_sm_alg_aes_cmac(salt_be, n_be, n_len, t);
    if (rc != 0) {
        return rc;
    }

    /* AES-CMAC_T (P) */
    rc = ble_sm_alg_aes_cmac(t, p, p_len, out);
    if (rc != 0) {
        return rc;
    }

    swap_in_place(out, 16);

    return 0;
}

int
ble_sm_alg_csis_s1(const uint8_t *m, size_t m_len, uint8_t *out)
{
    int rc;
    uint8_t k_zero[16] = {0};

    /* XXX: Spec does not specify the maximum M parameter length.
     * We assume that 16 bytes is enough and return error if passed len value is greater
     * than that */
    if (m_len > 16) {
        return BLE_HS_EINVAL;
    }

    /* AES-CMAC_zero (M) */
    rc = ble_sm_alg_aes_cmac(k_zero, m, m_len, out);
    if (rc != 0) {
        return rc;
    }

    swap_in_place(out, 16);

    return 0;
}

int
ble_sm_alg_csis_sef(const uint8_t *k, const uint8_t *plaintext_sirk, uint8_t *out)
{
    uint8_t salt[16];
    int rc;
    int i;

    /* s1("SIRKenc") */
    rc = ble_sm_alg_csis_s1((const uint8_t *) "SIRKenc", 7, salt);
    if (rc != 0) {
        return rc;
    }

    /* k1(K, s1("SIRKenc"), "csis") */
    rc = ble_sm_alg_csis_k1(k, 16, salt, (const uint8_t *) "csis", 4, out);
    if (rc != 0) {
        return rc;
    }

    /* k1(K, s1("SIRKenc"), "csis") ^ SIRK */
    for (i = 0; i < 16; i++) {
        out[i] ^= plaintext_sirk[i];
    }

    return 0;
}

int
ble_sm_alg_csis_sdf(const uint8_t *k, const uint8_t *enc_sirk, uint8_t *out)
{
    uint8_t salt[16];
    int rc;
    int i;

    /* s1("SIRKenc") */
    rc = ble_sm_alg_csis_s1((const uint8_t *) "SIRKenc", 7, salt);
    if (rc != 0) {
        return rc;
    }

    /* k1(K, s1("SIRKenc"), "csis") */
    rc = ble_sm_alg_csis_k1(k, 16, salt, (const uint8_t *) "csis", 4, out);
    if (rc != 0) {
        return rc;
    }

    /* k1(K, s1("SIRKenc"), "csis") ^ EncSIRK */
    for (i = 0; i < 16; i++) {
        out[i] ^= enc_sirk[i];
    }

    return 0;
}

int
ble_sm_alg_csis_sih(const uint8_t *k, const uint8_t *r, uint8_t *out)
{
    uint8_t r1[16];
    int rc;

    memcpy(r1, r, 3);
    memset(r1 + 3, 0, 13);

    rc = ble_sm_alg_encrypt(k, r1, r1);
    if (rc != 0) {
        return rc;
    }

    memcpy(out, r1, 3);

    return 0;
}

int
ble_sm_alg_rng(void *arg, unsigned char *buf, size_t size)
{
    (void)arg;

#if MYNEWT_VAL(SELFTEST)
    while (size--) {
        buf[size] = 9;
    }

    return 0;
#elif MYNEWT_VAL(TRNG)
    size_t num;

    if (!g_trng) {
        g_trng = (struct trng_dev *)os_dev_open("trng", OS_WAIT_FOREVER, NULL);
        assert(g_trng);
    }

    while (size) {
        num = trng_read(g_trng, buf, size);
        buf += num;
        size -= num;
    }

    return 0;
#else
    return ble_hs_hci_rand(buf, size);
#endif
}

int
ble_sm_alg_gen_dhkey(const uint8_t *peer_pub_key_x, const uint8_t *peer_pub_key_y,
                     const uint8_t *our_priv_key, uint8_t *out_dhkey)
{
    int err = 0;
#if MYNEWT_VAL(BLE_CRYPTO_STACK_MBEDTLS)
#if CONFIG_MBEDTLS_VER_4_X_SUPPORT
    // PSA/mbedTLS expects 65 bytes: 0x04 prefix + X (32 bytes) + Y (32 bytes)
    uint8_t pk[65];
    uint8_t dh[32];
    uint8_t priv[32];

    psa_key_id_t key_id = 0;
    psa_status_t status;
    psa_key_attributes_t key_attributes = PSA_KEY_ATTRIBUTES_INIT;
    size_t output_len = 0;

    swap_buf(priv, our_priv_key, 32);

    pk[0] = 0x04; // Uncompressed format for public key
    swap_buf(&pk[1], peer_pub_key_x, 32);
    swap_buf(&pk[33], peer_pub_key_y, 32);

    psa_set_key_type(&key_attributes, PSA_KEY_TYPE_ECC_KEY_PAIR(PSA_ECC_FAMILY_SECP_R1));
    psa_set_key_bits(&key_attributes, 256);
    psa_set_key_algorithm(&key_attributes, PSA_ALG_ECDH);
    psa_set_key_usage_flags(&key_attributes, PSA_KEY_USAGE_DERIVE);

    status = psa_import_key(&key_attributes, priv, 32, &key_id);
    if (status == PSA_SUCCESS) {
        psa_reset_key_attributes(&key_attributes);
        status = psa_raw_key_agreement(PSA_ALG_ECDH, key_id, pk, 65, dh, sizeof(dh), &output_len);
    }

    if (status != PSA_SUCCESS || output_len != 32) {
        err = BLE_HS_EUNKNOWN;
    } else {
        swap_buf(out_dhkey, dh, 32);
    }

    if (key_id != 0) {
        psa_destroy_key(key_id);
    }

    ble_sm_alg_secure_zero(priv, sizeof(priv));
    ble_sm_alg_secure_zero(dh, sizeof(dh));
    ble_sm_alg_secure_zero(pk, sizeof(pk));

    return err;
#else
    mbedtls_mpi priv;
    mbedtls_mpi shared;
    mbedtls_ecp_group group;
    mbedtls_ecp_point point;
    uint8_t pub_key[65];

    mbedtls_mpi_init(&priv);
    mbedtls_mpi_init(&shared);
    mbedtls_ecp_point_init(&point);
    mbedtls_ecp_group_init(&group);

    pub_key[0] = 0x04; /* Uncompressed point identifier */
    swap_buf(&pub_key[1], peer_pub_key_x, 32);
    swap_buf(&pub_key[33], peer_pub_key_y, 32);

    err = mbedtls_ecp_group_load(&group, MBEDTLS_ECP_DP_SECP256R1);

    if (err == 0) {
        err = mbedtls_ecp_point_read_binary(&group, &point, pub_key, sizeof(pub_key));
    }

    if (err == 0) {
        err = mbedtls_ecp_check_pubkey(&group, &point);
    }

    if (err == 0) {
        err = mbedtls_mpi_read_binary_le(&priv, our_priv_key, 32);
    }

    if (err == 0) {
        err = mbedtls_ecdh_compute_shared(&group, &shared, &point, &priv,
                                          ble_sm_alg_rng, NULL);
    }

    if (err == 0) {
        err = mbedtls_mpi_write_binary_le(&shared, out_dhkey, 32);
    }

    mbedtls_ecp_group_free(&group);
    mbedtls_ecp_point_free(&point);
    mbedtls_mpi_free(&priv);
    mbedtls_mpi_free(&shared);
    ble_sm_alg_secure_zero(pub_key, sizeof(pub_key));
    if (err) {
        return BLE_HS_EUNKNOWN;
    }

    return 0;
#endif
#else
    uint8_t dh[32];
    uint8_t pk[64];
    uint8_t priv[32];

    swap_buf(pk, peer_pub_key_x, 32);
    swap_buf(&pk[32], peer_pub_key_y, 32);
    swap_buf(priv, our_priv_key, 32);

    if (uECC_valid_public_key(pk, &curve_secp256r1) < 0) {
        err = BLE_HS_EUNKNOWN;
        goto done;
    }

    err = uECC_shared_secret(pk, priv, dh, &curve_secp256r1);
    if (err == TC_CRYPTO_FAIL) {
        err = BLE_HS_EUNKNOWN;
        goto done;
    }

    err = 0;
    swap_buf(out_dhkey, dh, 32);

done:
    ble_sm_alg_secure_zero(priv, sizeof(priv));
    ble_sm_alg_secure_zero(dh, sizeof(dh));
    ble_sm_alg_secure_zero(pk, sizeof(pk));
    return err;
#endif
}

/* based on Core Specification 4.2 Vol 3. Part H 2.3.5.6.1 */
static const uint8_t ble_sm_alg_dbg_priv_key[32] = {
    0x3f, 0x49, 0xf6, 0xd4, 0xa3, 0xc5, 0x5f, 0x38, 0x74, 0xc9, 0xb3, 0xe3,
    0xd2, 0x10, 0x3f, 0x50, 0x4a, 0xff, 0x60, 0x7b, 0xeb, 0x40, 0xb7, 0x99,
    0x58, 0x99, 0xb8, 0xa6, 0xcd, 0x3c, 0x1a, 0xbd
};

#if MYNEWT_VAL(BLE_SM_SC_DEBUG_KEYS)
static const uint8_t ble_sm_alg_dbg_pub_key[64] = {
    /* X */
    0x20, 0xb0, 0x03, 0xd2, 0xf2, 0x97, 0xbe, 0x2c, 0x5e, 0x2c, 0x83, 0xa7,
    0xe9, 0xf9, 0xa5, 0xb9, 0xef, 0xf4, 0x91, 0x11, 0xac, 0xf4, 0xfd, 0xdb,
    0xcc, 0x03, 0x01, 0x48, 0x0e, 0x35, 0x9d, 0xe6,
    /* Y */
    0xdc, 0x80, 0x9c, 0x49, 0x65, 0x2a, 0xeb, 0x6d, 0x63, 0x32, 0x9a, 0xbf,
    0x5a, 0x52, 0x15, 0x5c, 0x76, 0x63, 0x45, 0xc2, 0x8f, 0xed, 0x30, 0x24,
    0x74, 0x1c, 0x8e, 0xd0, 0x15, 0x89, 0xd2, 0x8b,
};
#endif

/**
 * pub: 64 bytes
 * priv: 32 bytes
 */
int
ble_sm_alg_gen_key_pair(uint8_t *pub, uint8_t *priv)
{
#if MYNEWT_VAL(BLE_SM_SC_DEBUG_KEYS)
    swap_buf(pub, ble_sm_alg_dbg_pub_key, 32);
    swap_buf(&pub[32], &ble_sm_alg_dbg_pub_key[32], 32);
    swap_buf(priv, ble_sm_alg_dbg_priv_key, 32);
    return 0;
#else

#if MYNEWT_VAL(BLE_CRYPTO_STACK_MBEDTLS)
#if CONFIG_MBEDTLS_VER_4_X_SUPPORT
    int ret = 0;

    psa_status_t status;
    psa_key_attributes_t key_attributes = PSA_KEY_ATTRIBUTES_INIT;
    psa_key_id_t key_id = 0;
    size_t olen = 0;

    // Temporary flat arrays to hold raw Big-Endian values from PSA
    uint8_t tmp_priv[32];
    uint8_t tmp_pub[65]; // 0x04 uncompressed prefix + 32-byte X + 32-byte Y

    psa_set_key_type(&key_attributes, PSA_KEY_TYPE_ECC_KEY_PAIR(PSA_ECC_FAMILY_SECP_R1));
    psa_set_key_bits(&key_attributes, 256);
    psa_set_key_algorithm(&key_attributes, PSA_ALG_ECDH);
    psa_set_key_usage_flags(&key_attributes, PSA_KEY_USAGE_EXPORT);

    do {
        status = psa_generate_key(&key_attributes, &key_id);
        if (status != PSA_SUCCESS) {
            ret = BLE_HS_EUNKNOWN;
            goto psa_exit;
        }

        status = psa_export_key(key_id, tmp_priv, sizeof(tmp_priv), &olen);
        if (status != PSA_SUCCESS || olen != 32) {
            ret = BLE_HS_EUNKNOWN;
            goto psa_exit;
        }

        // Convert Big-Endian to Little-Endian to evaluate against the debug key rule
        swap_buf(priv, tmp_priv, 32);

        if (memcmp(priv, ble_sm_alg_dbg_priv_key, 32) == 0) {
            psa_destroy_key(key_id);
            key_id = 0;
            continue; // Regenerate keypair
        }
        break;
    } while (1);

    psa_reset_key_attributes(&key_attributes);

    status = psa_export_public_key(key_id, tmp_pub, sizeof(tmp_pub), &olen);
    if (status != PSA_SUCCESS || olen != 65 || tmp_pub[0] != 0x04) {
        ret = BLE_HS_EUNKNOWN;
        goto psa_exit;
    }

    // Unpack Big-Endian coordinates and transform them to NimBLE's Little-Endian requirement
    swap_buf(pub, &tmp_pub[1], 32);       // Extract X
    swap_buf(pub + 32, &tmp_pub[33], 32);  // Extract Y

psa_exit:
    if (key_id != 0) {
        psa_destroy_key(key_id);
    }
    ble_sm_alg_secure_zero(tmp_priv, sizeof(tmp_priv));
    ble_sm_alg_secure_zero(tmp_pub, sizeof(tmp_pub));
    return ret;

#else // Legacy MbedTLS Fallback Block
    uint8_t pub_buf[65];
    size_t pub_len = 0;
    mbedtls_ecp_keypair keypair;
    mbedtls_ecp_group group;
    mbedtls_ecp_point point;
    mbedtls_mpi d;
    int err;

    mbedtls_ecp_keypair_init(&keypair);
    mbedtls_ecp_group_init(&group);
    mbedtls_ecp_point_init(&point);
    mbedtls_mpi_init(&d);

    do {
        err = mbedtls_ecp_gen_key(MBEDTLS_ECP_DP_SECP256R1, &keypair,
                                  ble_sm_alg_rng, NULL);
        if (err) {
            break;
        }

        err = mbedtls_ecp_export(&keypair, &group, &d, &point);
        if (err) {
            break;
        }

        err = mbedtls_mpi_write_binary_le(&d, priv, 32);
        if (err) {
            break;
        }
        /* Make sure generated key isn't debug key. */
    } while (memcmp(priv, ble_sm_alg_dbg_priv_key, 32) == 0);

    if (err == 0) {
        err = mbedtls_ecp_point_write_binary(&group, &point, MBEDTLS_ECP_PF_UNCOMPRESSED,
                                             &pub_len, pub_buf, sizeof(pub_buf));
    }

    if (err == 0) {
        if (pub_len == sizeof(pub_buf)) {
            swap_buf(pub, &pub_buf[1], 32);
            swap_buf(pub + 32, &pub_buf[33], 32);
        } else {
            err = BLE_HS_EUNKNOWN;
        }
    }

    mbedtls_ecp_keypair_free(&keypair);
    mbedtls_ecp_group_free(&group);
    mbedtls_ecp_point_free(&point);
    mbedtls_mpi_free(&d);

    if (err) {
        return BLE_HS_EUNKNOWN;
    }

    return 0;
#endif // CONFIG_MBEDTLS_VER_4_X_SUPPORT

#else // TinyCrypt / uECC Block
    int ret = 0;
    uint8_t tmp_priv[32];
    uint8_t tmp_pub[64]; // uECC uses native 64 bytes (omits the 0x04 uncompressed prefix)

    do {
        // uECC uses Big-Endian mapping internally
        if (uECC_make_key(tmp_pub, tmp_priv, &curve_secp256r1) != 1) {
            ret = BLE_HS_EUNKNOWN;
            goto tc_exit;
        }

        // Swap to Little-Endian for validation
        swap_buf(priv, tmp_priv, 32);

        if (memcmp(priv, ble_sm_alg_dbg_priv_key, 32) == 0) {
            continue;
        }
        break;
    } while (1);

    // Unpack Big-Endian coordinates to NimBLE's Little-Endian public array destination
    swap_buf(pub, tmp_pub, 32);
    swap_buf(pub + 32, &tmp_pub[32], 32);

tc_exit:
    ble_sm_alg_secure_zero(tmp_priv, sizeof(tmp_priv));
    ble_sm_alg_secure_zero(tmp_pub, sizeof(tmp_pub));
    return ret;
#endif // MYNEWT_VAL(BLE_CRYPTO_STACK_MBEDTLS)
#endif // MYNEWT_VAL(BLE_SM_SC_DEBUG_KEYS)
}

#if MYNEWT_VAL(SELFTEST)
/* Unit tests rely on custom RNG function not being set */
#define ble_sm_alg_rand NULL
#elif !(MYNEWT_VAL(BLE_CRYPTO_STACK_MBEDTLS))
/* used by uECC to get random data */
static int
ble_sm_alg_rand(uint8_t *dst, unsigned int size)
{
#if MYNEWT_VAL(TRNG)
    size_t num;

    if (!g_trng) {
        g_trng = (struct trng_dev *)os_dev_open("trng", OS_WAIT_FOREVER, NULL);
        assert(g_trng);
    }

    while (size) {
        num = trng_read(g_trng, dst, size);
        dst += num;
        size -= num;
    }
#else
    if (!ble_hs_is_enabled()) {
        return 0;
    }

    if (ble_hs_hci_rand(dst, size)) {
        return 0;
    }
#endif

    return 1;
}
#endif

void
ble_sm_alg_ecc_init(void) {
#if (!MYNEWT_VAL(BLE_CRYPTO_STACK_MBEDTLS))
    uECC_set_rng(ble_sm_alg_rand);
#endif
    return;
}

#endif
#endif
#endif
