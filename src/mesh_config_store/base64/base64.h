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
#ifndef __UTIL_BASE64_H
#define __UTIL_BASE64_H

#include <stdint.h>
#include <string.h>

#ifdef __cplusplus
extern "C" {
#endif

struct base64_decoder {
    /*** public */
    const char *src;
    void *dst;
    int src_len; /* <=0 if src ends with '\0' */
    int dst_len; /* <=0 if dst unbounded */

    /*** private */
    char buf[4];
    int buf_len;
};

static const char base64_chars[] =
    "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

static int
pos(char c)
{
    const char *p;
    for (p = base64_chars; *p; p++)
        if (*p == c)
            return p - base64_chars;
    return -1;
}

int
base64_encode(const void *data, int size, char *s, uint8_t should_pad)
{
    char *p;
    int i;
    int c;
    const unsigned char *q;
    char *last;
    int diff;

    p = s;

    q = (const unsigned char *) data;
    last = NULL;
    i = 0;
    while (i < size) {
        c = q[i++];
        c *= 256;
        if (i < size)
            c += q[i];
        i++;
        c *= 256;
        if (i < size)
            c += q[i];
        i++;
        p[0] = base64_chars[(c & 0x00fc0000) >> 18];
        p[1] = base64_chars[(c & 0x0003f000) >> 12];
        p[2] = base64_chars[(c & 0x00000fc0) >> 6];
        p[3] = base64_chars[(c & 0x0000003f) >> 0];
        last = p;
        p += 4;
    }

    if (last) {
        diff = i - size;
        if (diff > 0) {
            if (should_pad) {
                memset(last + (4 - diff), '=', diff);
            } else {
                p = last + (4 - diff);
            }
        }
    }

    *p = 0;

    return (p - s);
}

int
base64_pad(char *buf, int len)
{
    int remainder;

    remainder = len % 4;
    if (remainder == 0) {
        return (0);
    }

    memset(buf, '=', 4 - remainder);

    return (4 - remainder);
}

#define DECODE_ERROR -1

static unsigned int
token_decode(const char *token, int len)
{
    int i;
    unsigned int val = 0;
    int marker = 0;

    if (len < 4) {
        return DECODE_ERROR;
    }

    for (i = 0; i < 4; i++) {
        val *= 64;
        if (token[i] == '=') {
            marker++;
        } else if (marker > 0) {
            return DECODE_ERROR;
        } else {
            val += pos(token[i]);
        }
    }

    if (marker > 2) {
        return DECODE_ERROR;
    }

    return (marker << 24) | val;
}

int
base64_decoder_go(struct base64_decoder *dec)
{
    unsigned int marker;
    unsigned int val;
    uint8_t *dst;
    char sval;
    int read_len;
    int src_len;
    int src_rem;
    int src_off;
    int dst_len;
    int dst_off;
    int i;

    dst = dec->dst;
    dst_off = 0;
    src_off = 0;

    /* A length <= 0 means "unbounded". */
    if (dec->src_len <= 0) {
        src_len = INT_MAX;
    } else {
        src_len = dec->src_len;
    }
    if (dec->dst_len <= 0) {
        dst_len = INT_MAX;
    } else {
        dst_len = dec->dst_len;
    }

    while (1) {
        src_rem = src_len - src_off;
        if (src_rem == 0) {
            /* End of source input. */
            break;
        }

        if (dec->src[src_off] == '\0') {
            /* End of source string. */
            break;
        }

        /* Account for possibility of partial token from previous call. */
        read_len = 4 - dec->buf_len;

        /* Detect invalid input. */
        for (i = 0; i < read_len; i++) {
            sval = dec->src[src_off + i];
            if (sval == '\0') {
                /* Incomplete input. */
                return -1;
            }
            if (sval != '=' && strchr(base64_chars, sval) == NULL) {
                /* Invalid base64 character. */
                return -1;
            }
        }

        if (src_rem < read_len) {
            /* Input contains a partial token.  Stash it for use during the
             * next call.
             */
            memcpy(&dec->buf[dec->buf_len], &dec->src[src_off], src_rem);
            dec->buf_len += src_rem;
            break;
        }

        /* Copy full token into buf and decode it. */
        memcpy(&dec->buf[dec->buf_len], &dec->src[src_off], read_len);
        val = token_decode(dec->buf, read_len);
        if (val == DECODE_ERROR) {
            return -1;
        }
        src_off += read_len;
        dec->buf_len = 0;

        marker = (val >> 24) & 0xff;

        if (dst_off >= dst_len) {
            break;
        }
        dst[dst_off] = (val >> 16) & 0xff;
        dst_off++;

        if (marker < 2) {
            if (dst_off >= dst_len) {
                break;
            }
            dst[dst_off] = (val >> 8) & 0xff;
            dst_off++;
        }

        if (marker < 1) {
            if (dst_off >= dst_len) {
                break;
            }
            dst[dst_off] = val & 0xff;
            dst_off++;
        }
    }

    return dst_off;
}

int
base64_decode(const char *str, void *data)
{
    struct base64_decoder dec = {
        .src = str,
        .dst = data,
    };

    return base64_decoder_go(&dec);
}

int
base64_decode_maxlen(const char *str, void *data, int len)
{
    struct base64_decoder dec = {
        .src = str,
        .dst = data,
        .dst_len = len,
    };

    return base64_decoder_go(&dec);
}

int
base64_decode_len(const char *str)
{
    int len;

    len = strlen(str);
    while (len && str[len - 1] == '=') {
        len--;
    }
    return len * 3 / 4;
}

#define BASE64_ENCODE_SIZE(__size) (((((__size) - 1) / 3) * 4) + 4)

#ifdef __cplusplus
}
#endif

#endif /* __UTIL_BASE64_H__ */
