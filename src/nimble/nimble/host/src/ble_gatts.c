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

#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include "nimble/nimble/include/nimble/ble.h"
#include "nimble/nimble/host/include/host/ble_gatt.h"
#include "nimble/nimble/host/include/host/ble_uuid.h"
#include "nimble/nimble/host/include/host/ble_store.h"
#include "ble_hs_priv.h"
#include "nimble/esp_port/port/include/esp_nimble_mem.h"

#if MYNEWT_VAL(BLE_DYNAMIC_SERVICE)
#include "nimble/nimble/host/services/gatt/include/services/gatt/ble_svc_gatt.h"
#endif
#if MYNEWT_VAL(BLE_SVC_HID_SERVICE)
#include "nimble/nimble/host/services/gatt/include/services/hid/ble_svc_hid.h"
#endif

#define BLE_GATTS_INCLUDE_SZ    6
#define BLE_GATTS_CHR_MAX_SZ    19

#if MYNEWT_VAL(BLE_DYNAMIC_SERVICE)
enum {
    CONN_CLT_CFG_ADD = 1,
    CONN_CLT_CFG_REMOVE = 2,
};
#endif

#if MYNEWT_VAL(BLE_GATT_CACHING)
/* store the aware state only for the bonded peers */
struct ble_gatts_aware_state ble_gatts_conn_aware_states[MYNEWT_VAL(BLE_STORE_MAX_BONDS)];
/* index of latest bonded peer */
static int last_conn_aware_state_index;
#endif
static const ble_uuid_t *uuid_pri =
    BLE_UUID16_DECLARE(BLE_ATT_UUID_PRIMARY_SERVICE);
static const ble_uuid_t *uuid_sec =
    BLE_UUID16_DECLARE(BLE_ATT_UUID_SECONDARY_SERVICE);
static const ble_uuid_t *uuid_inc =
    BLE_UUID16_DECLARE(BLE_ATT_UUID_INCLUDE);
static const ble_uuid_t *uuid_chr =
    BLE_UUID16_DECLARE(BLE_ATT_UUID_CHARACTERISTIC);
static const ble_uuid_t *uuid_ccc =
    BLE_UUID16_DECLARE(BLE_GATT_DSC_CLT_CFG_UUID16);
static const ble_uuid_t *uuid_cpf =
    BLE_UUID16_DECLARE(BLE_GATT_DSC_CLT_PRE_FMT16);
static const ble_uuid_t *uuid_caf =
    BLE_UUID16_DECLARE(BLE_GATT_DSC_CLT_AGG_FMT16);

static const struct ble_gatt_svc_def **ble_gatts_svc_defs;
static int ble_gatts_num_svc_defs;

struct ble_gatts_svc_entry {
#if MYNEWT_VAL(BLE_DYNAMIC_SERVICE)
    STAILQ_ENTRY(ble_gatts_svc_entry) next;
#endif
    const struct ble_gatt_svc_def *svc;
    uint16_t handle;            /* 0 means unregistered. */
    uint16_t end_group_handle;  /* 0xffff means unset. */
};

#if MYNEWT_VAL(BLE_DYNAMIC_SERVICE)
STAILQ_HEAD(ble_gatts_svc_entry_list, ble_gatts_svc_entry);
static struct ble_gatts_svc_entry_list ble_gatts_svc_entries;
static void *ble_gatts_svc_entry_mem;
static struct os_mempool ble_gatts_svc_entry_pool;
#else
static struct ble_gatts_svc_entry *ble_gatts_svc_entries;
static uint16_t ble_gatts_num_svc_entries;
#endif

static os_membuf_t *ble_gatts_clt_cfg_mem;
static struct os_mempool ble_gatts_clt_cfg_pool;

#if MYNEWT_VAL(BLE_DYNAMIC_SERVICE)
/** A cached list of handles for the configurable characteristics. */
static struct ble_gatts_clt_cfg_list ble_gatts_clt_cfgs;
#else
struct ble_gatts_clt_cfg {
    uint16_t chr_val_handle;
    uint8_t flags;
    uint8_t allowed;
};

/** A cached array of handles for the configurable characteristics. */
static struct ble_gatts_clt_cfg *ble_gatts_clt_cfgs;
#endif
static int ble_gatts_num_cfgable_chrs;

STATS_SECT_DECL(ble_gatts_stats) ble_gatts_stats;
STATS_NAME_START(ble_gatts_stats)
    STATS_NAME(ble_gatts_stats, svcs)
    STATS_NAME(ble_gatts_stats, chrs)
    STATS_NAME(ble_gatts_stats, dscs)
    STATS_NAME(ble_gatts_stats, svc_def_reads)
    STATS_NAME(ble_gatts_stats, svc_inc_reads)
    STATS_NAME(ble_gatts_stats, chr_def_reads)
    STATS_NAME(ble_gatts_stats, chr_val_reads)
    STATS_NAME(ble_gatts_stats, chr_val_writes)
    STATS_NAME(ble_gatts_stats, dsc_reads)
    STATS_NAME(ble_gatts_stats, dsc_writes)
STATS_NAME_END(ble_gatts_stats)

#if MYNEWT_VAL(BLE_DYNAMIC_SERVICE)
static struct ble_gatts_svc_entry *
ble_gatts_svc_entry_alloc(void)
{
    struct ble_gatts_svc_entry *entry;

    entry = os_memblock_get(&ble_gatts_svc_entry_pool);
    /* if dynamic services are enabled, try to allocate from heap */
    if (entry == NULL) {
        entry = nimble_platform_mem_malloc(sizeof *entry);
    }
    if (entry != NULL) {
        memset(entry, 0, sizeof *entry);
    }

    return entry;
}

static void
ble_gatts_svc_entry_free(struct ble_gatts_svc_entry *entry)
{
    if (os_memblock_from(&ble_gatts_svc_entry_pool, entry)) {
        os_memblock_put(&ble_gatts_svc_entry_pool, entry);
    }
    else {
        nimble_platform_mem_free(entry);
    }
}

static struct ble_gatts_clt_cfg *
ble_gatts_clt_cfg_alloc(void)
{
    struct ble_gatts_clt_cfg *cfg;

    cfg = os_memblock_get(&ble_gatts_clt_cfg_pool);
    /* if dynamic services are enabled, try to allocate from heap */
    if (cfg == NULL) {
        cfg = nimble_platform_mem_malloc(sizeof *cfg);
    }
    if (cfg != NULL) {
        memset(cfg, 0, sizeof *cfg);
    }
    return cfg;
}

static void
ble_gatts_clt_cfg_free(struct ble_gatts_clt_cfg *cfg)
{
    if (os_memblock_from(&ble_gatts_clt_cfg_pool, cfg)) {
        os_memblock_put(&ble_gatts_clt_cfg_pool, cfg);
    }
    else {
        nimble_platform_mem_free(cfg);
    }
}
#endif

static int
ble_gatts_svc_access(uint16_t conn_handle, uint16_t attr_handle,
                     uint8_t op, uint16_t offset, struct os_mbuf **om,
                     void *arg)
{
    const struct ble_gatt_svc_def *svc;
    uint8_t *buf;

    STATS_INC(ble_gatts_stats, svc_def_reads);

    BLE_HS_DBG_ASSERT(op == BLE_ATT_ACCESS_OP_READ);

    svc = arg;

    buf = os_mbuf_extend(*om, ble_uuid_length(svc->uuid));
    if (buf == NULL) {
        return BLE_ATT_ERR_INSUFFICIENT_RES;
    }

    ble_uuid_flat(svc->uuid, buf);

    return 0;
}

static int
ble_gatts_inc_access(uint16_t conn_handle, uint16_t attr_handle,
                     uint8_t op, uint16_t offset, struct os_mbuf **om,
                     void *arg)
{
    const struct ble_gatts_svc_entry *entry;
    uint16_t uuid16;
    uint8_t *buf;

    STATS_INC(ble_gatts_stats, svc_inc_reads);

    BLE_HS_DBG_ASSERT(op == BLE_ATT_ACCESS_OP_READ);

    entry = arg;

    buf = os_mbuf_extend(*om, 4);
    if (buf == NULL) {
        return BLE_ATT_ERR_INSUFFICIENT_RES;
    }
    put_le16(buf + 0, entry->handle);
    put_le16(buf + 2, entry->end_group_handle);

    /* Only include the service UUID if it has a 16-bit representation. */
    uuid16 = ble_uuid_u16(entry->svc->uuid);
    if (uuid16 != 0) {
        buf = os_mbuf_extend(*om, 2);
        if (buf == NULL) {
            return BLE_ATT_ERR_INSUFFICIENT_RES;
        }
        put_le16(buf, uuid16);
    }

    return 0;
}

static uint16_t
ble_gatts_chr_clt_cfg_allowed(const struct ble_gatt_chr_def *chr)
{
    uint16_t flags;

    flags = 0;
    if (chr->flags & BLE_GATT_CHR_F_NOTIFY) {
        flags |= BLE_GATTS_CLT_CFG_F_NOTIFY;
    }
    if (chr->flags & BLE_GATT_CHR_F_INDICATE) {
        flags |= BLE_GATTS_CLT_CFG_F_INDICATE;
    }

    return flags;
}

static uint8_t
ble_gatts_att_flags_from_chr_flags(ble_gatt_chr_flags chr_flags)
{
    uint8_t att_flags;

    att_flags = 0;
    if (chr_flags & BLE_GATT_CHR_F_READ) {
        att_flags |= BLE_ATT_F_READ;
    }
    if (chr_flags & (BLE_GATT_CHR_F_WRITE_NO_RSP | BLE_GATT_CHR_F_WRITE)) {
        att_flags |= BLE_ATT_F_WRITE;
    }
    if (chr_flags & BLE_GATT_CHR_F_READ_ENC) {
        att_flags |= BLE_ATT_F_READ_ENC;
    }
    if (chr_flags & BLE_GATT_CHR_F_READ_AUTHEN) {
        att_flags |= BLE_ATT_F_READ_AUTHEN;
    }
    if (chr_flags & BLE_GATT_CHR_F_READ_AUTHOR) {
        att_flags |= BLE_ATT_F_READ_AUTHOR;
    }
    if (chr_flags & BLE_GATT_CHR_F_WRITE_ENC) {
        att_flags |= BLE_ATT_F_WRITE_ENC;
    }
    if (chr_flags & BLE_GATT_CHR_F_WRITE_AUTHEN) {
        att_flags |= BLE_ATT_F_WRITE_AUTHEN;
    }
    if (chr_flags & BLE_GATT_CHR_F_WRITE_AUTHOR) {
        att_flags |= BLE_ATT_F_WRITE_AUTHOR;
    }

    return att_flags;
}

static uint8_t
ble_gatts_chr_properties(const struct ble_gatt_chr_def *chr)
{
    uint8_t properties;

    properties = 0;

    if (chr->flags & BLE_GATT_CHR_F_BROADCAST) {
        properties |= BLE_GATT_CHR_PROP_BROADCAST;
    }
    if (chr->flags & BLE_GATT_CHR_F_READ) {
        properties |= BLE_GATT_CHR_PROP_READ;
    }
    if (chr->flags & BLE_GATT_CHR_F_WRITE_NO_RSP) {
        properties |= BLE_GATT_CHR_PROP_WRITE_NO_RSP;
    }
    if (chr->flags & BLE_GATT_CHR_F_WRITE) {
        properties |= BLE_GATT_CHR_PROP_WRITE;
    }
    if (chr->flags & BLE_GATT_CHR_F_NOTIFY) {
        properties |= BLE_GATT_CHR_PROP_NOTIFY;
    }
    if (chr->flags & BLE_GATT_CHR_F_INDICATE) {
        properties |= BLE_GATT_CHR_PROP_INDICATE;
    }
    if (chr->flags & BLE_GATT_CHR_F_AUTH_SIGN_WRITE) {
        properties |= BLE_GATT_CHR_PROP_AUTH_SIGN_WRITE;
    }
    if (chr->flags &
        (BLE_GATT_CHR_F_RELIABLE_WRITE | BLE_GATT_CHR_F_AUX_WRITE)) {

        properties |= BLE_GATT_CHR_PROP_EXTENDED;
    }

    return properties;
}

static int
ble_gatts_chr_def_access(uint16_t conn_handle, uint16_t attr_handle,
                         uint8_t op, uint16_t offset, struct os_mbuf **om,
                         void *arg)
{
    const struct ble_gatt_chr_def *chr;
    uint8_t *buf;

    STATS_INC(ble_gatts_stats, chr_def_reads);

    BLE_HS_DBG_ASSERT(op == BLE_ATT_ACCESS_OP_READ);

    chr = arg;

    buf = os_mbuf_extend(*om, 3);
    if (buf == NULL) {
        return BLE_ATT_ERR_INSUFFICIENT_RES;
    }

    buf[0] = ble_gatts_chr_properties(chr);

    /* The value attribute is always immediately after the declaration. */
    put_le16(buf + 1, attr_handle + 1);

    buf = os_mbuf_extend(*om, ble_uuid_length(chr->uuid));
    if (buf == NULL) {
        return BLE_ATT_ERR_INSUFFICIENT_RES;
    }

    ble_uuid_flat(chr->uuid, buf);

    return 0;
}

static int
ble_gatts_chr_is_sane(const struct ble_gatt_chr_def *chr)
{
    if (chr->uuid == NULL) {
        return 0;
    }

    if (chr->access_cb == NULL) {
        return 0;
    }

    /* XXX: Check properties. */

    return 1;
}

static uint8_t
ble_gatts_chr_op(uint8_t att_op)
{
    switch (att_op) {
    case BLE_ATT_ACCESS_OP_READ:
        return BLE_GATT_ACCESS_OP_READ_CHR;

    case BLE_ATT_ACCESS_OP_WRITE:
        return BLE_GATT_ACCESS_OP_WRITE_CHR;

    default:
        BLE_HS_DBG_ASSERT(0);
        return BLE_GATT_ACCESS_OP_READ_CHR;
    }
}

static void
ble_gatts_chr_inc_val_stat(uint8_t gatt_op)
{
    switch (gatt_op) {
    case BLE_GATT_ACCESS_OP_READ_CHR:
        STATS_INC(ble_gatts_stats, chr_val_reads);
        break;

    case BLE_GATT_ACCESS_OP_WRITE_CHR:
        STATS_INC(ble_gatts_stats, chr_val_writes);
        break;

    default:
        break;
    }
}

/**
 * Indicates whether the set of registered services can be modified.  The
 * service set is mutable if:
 *     o No peers are connected, and
 *     o No GAP operations are active (advertise, discover, or connect).
 *
 * @return                      true if the GATT service set can be modified;
 *                              false otherwise.
 */
static bool
ble_gatts_mutable(void)
{
    /* Ensure no active GAP procedures. */
    if (ble_gap_adv_active() ||
        ble_gap_disc_active() ||
        ble_gap_conn_active()) {

        return false;
    }

    /* Ensure no established connections. */
    if (ble_hs_conn_first() != NULL) {
        return false;
    }

    return true;
}

static int
ble_gatts_val_access(uint16_t conn_handle, uint16_t attr_handle,
                     uint16_t offset, struct ble_gatt_access_ctxt *gatt_ctxt,
                     struct os_mbuf **om, ble_gatt_access_fn *access_cb,
                     void *cb_arg)
{
    uint16_t initial_len;
    int attr_len;
    int new_om;
    int rc;

    switch (gatt_ctxt->op) {
    case BLE_GATT_ACCESS_OP_READ_CHR:
    case BLE_GATT_ACCESS_OP_READ_DSC:
        /* A characteristic value is being read.
         *
         * If the read specifies an offset of 0:
         *     just append the characteristic value directly onto the response
         *     mbuf.
         *
         * Else:
         *     allocate a new mbuf to hold the characteristic data, then append
         *     the requested portion onto the response mbuf.
         */
        if (offset == 0) {
            new_om = 0;
            gatt_ctxt->om = *om;
        } else {
            new_om = 1;
            gatt_ctxt->om = ble_hs_mbuf_att_pkt();
            if (gatt_ctxt->om == NULL) {
                return BLE_ATT_ERR_INSUFFICIENT_RES;
            }
        }

        initial_len = OS_MBUF_PKTLEN(gatt_ctxt->om);
        rc = access_cb(conn_handle, attr_handle, gatt_ctxt, cb_arg);
        if (rc == 0) {
            attr_len = OS_MBUF_PKTLEN(gatt_ctxt->om) - initial_len - offset;
            if (attr_len >= 0) {
                if (new_om) {
                    os_mbuf_appendfrom(*om, gatt_ctxt->om, offset, attr_len);
                }
            } else {
                rc = BLE_ATT_ERR_INVALID_OFFSET;
            }
        }

        if (new_om) {
            os_mbuf_free_chain(gatt_ctxt->om);
        }
        return rc;

    case BLE_GATT_ACCESS_OP_WRITE_CHR:
    case BLE_GATT_ACCESS_OP_WRITE_DSC:
        gatt_ctxt->om = *om;
        rc = access_cb(conn_handle, attr_handle, gatt_ctxt, cb_arg);
        *om = gatt_ctxt->om;
        return rc;

    default:
        BLE_HS_DBG_ASSERT(0);
        return BLE_ATT_ERR_UNLIKELY;
    }
}

static int
ble_gatts_chr_val_access(uint16_t conn_handle, uint16_t attr_handle,
                         uint8_t att_op, uint16_t offset,
                         struct os_mbuf **om, void *arg)
{
    const struct ble_gatt_chr_def *chr_def;
    struct ble_gatt_access_ctxt gatt_ctxt;
    int rc;

    chr_def = arg;
    BLE_HS_DBG_ASSERT(chr_def != NULL && chr_def->access_cb != NULL);

    gatt_ctxt.op = ble_gatts_chr_op(att_op);
    gatt_ctxt.chr = chr_def;

    ble_gatts_chr_inc_val_stat(gatt_ctxt.op);
    rc = ble_gatts_val_access(conn_handle, attr_handle, offset, &gatt_ctxt, om,
                              chr_def->access_cb, chr_def->arg);

    return rc;
}

#if MYNEWT_VAL(BLE_DYNAMIC_SERVICE)
static struct ble_gatts_svc_entry*
ble_gatts_find_svc_entry(const struct ble_gatt_svc_def *svc)
{
    struct ble_gatts_svc_entry *entry;

    STAILQ_FOREACH(entry, &ble_gatts_svc_entries, next) {
        if (entry->svc == svc) {
            return entry;
        }
    }

    return NULL;
}
#else
static int
ble_gatts_find_svc_entry_idx(const struct ble_gatt_svc_def *svc)
{
    int i;

    for (i = 0; i < ble_gatts_num_svc_entries; i++) {
        if (ble_gatts_svc_entries[i].svc == svc) {
            return i;
        }
    }

    return -1;
}
#endif

static int
ble_gatts_svc_incs_satisfied(const struct ble_gatt_svc_def *svc)
{
#if MYNEWT_VAL(BLE_DYNAMIC_SERVICE)
    int i;
    struct ble_gatts_svc_entry *entry;
#else
    int idx;
    int i;
#endif

    if (svc->includes == NULL) {
        /* No included services. */
        return 1;
    }

    for (i = 0; svc->includes[i] != NULL; i++) {
#if MYNEWT_VAL(BLE_DYNAMIC_SERVICE)
        entry = ble_gatts_find_svc_entry(svc->includes[i]);
        if (entry == NULL || entry -> handle == 0) {
#else
        idx = ble_gatts_find_svc_entry_idx(svc->includes[i]);
        if (idx == -1 || ble_gatts_svc_entries[idx].handle == 0) {
#endif
            return 0;
        }
    }

    return 1;
}


#if MYNEWT_VAL(BLE_GATT_CACHING)
int
ble_gatts_calculate_hash(uint8_t *out_hash_key)
{
    int size;
    int rc;
    uint8_t *buf;
    uint8_t key[16];

    memset(key, 0, sizeof(key));
    /* data with all zeroes */
    rc = ble_att_get_database_size(&size);
    if(rc != 0) {
        return rc;
    }
    buf = nimble_platform_mem_malloc(sizeof(uint8_t) * size);
    if(buf == NULL) {
        rc = BLE_HS_ENOMEM;
        return rc;
    }

    rc = ble_att_fill_database_info(buf);
    if(rc != 0) {
        return rc;
    }

    rc = ble_sm_alg_aes_cmac(key, buf, size, out_hash_key);
    if(rc != 0) {
        return rc;
    }

    swap_in_place(out_hash_key, 16);
    return 0;
}
#endif

static int
ble_gatts_register_inc(struct ble_gatts_svc_entry *entry)
{
    uint16_t handle;
    int rc;

    BLE_HS_DBG_ASSERT(entry->handle != 0);
    BLE_HS_DBG_ASSERT(entry->end_group_handle != 0xffff);

    rc = ble_att_svr_register(uuid_inc, BLE_ATT_F_READ, 0, &handle,
                              ble_gatts_inc_access, entry);
    if (rc != 0) {
        return rc;
    }

    return 0;
}

static uint8_t
ble_gatts_dsc_op(uint8_t att_op)
{
    switch (att_op) {
    case BLE_ATT_ACCESS_OP_READ:
        return BLE_GATT_ACCESS_OP_READ_DSC;

    case BLE_ATT_ACCESS_OP_WRITE:
        return BLE_GATT_ACCESS_OP_WRITE_DSC;

    default:
        BLE_HS_DBG_ASSERT(0);
        return BLE_GATT_ACCESS_OP_READ_DSC;
    }
}

static void
ble_gatts_dsc_inc_stat(uint8_t gatt_op)
{
    switch (gatt_op) {
    case BLE_GATT_ACCESS_OP_READ_DSC:
        STATS_INC(ble_gatts_stats, dsc_reads);
        break;

    case BLE_GATT_ACCESS_OP_WRITE_DSC:
        STATS_INC(ble_gatts_stats, dsc_writes);
        break;

    default:
        break;
    }
}

static int
ble_gatts_dsc_access(uint16_t conn_handle, uint16_t attr_handle,
                     uint8_t att_op, uint16_t offset, struct os_mbuf **om,
                     void *arg)
{
    const struct ble_gatt_dsc_def *dsc_def;
    struct ble_gatt_access_ctxt gatt_ctxt;
    int rc;

    dsc_def = arg;
    BLE_HS_DBG_ASSERT(dsc_def != NULL && dsc_def->access_cb != NULL);

    gatt_ctxt.op = ble_gatts_dsc_op(att_op);
    gatt_ctxt.dsc = dsc_def;

    ble_gatts_dsc_inc_stat(gatt_ctxt.op);
    rc = ble_gatts_val_access(conn_handle, attr_handle, offset, &gatt_ctxt, om,
                              dsc_def->access_cb, dsc_def->arg);

    return rc;
}

static int
ble_gatts_dsc_is_sane(const struct ble_gatt_dsc_def *dsc)
{
    if (dsc->uuid == NULL) {
        return 0;
    }

    if (dsc->access_cb == NULL) {
        return 0;
    }

    return 1;
}

static int
ble_gatts_register_dsc(const struct ble_gatt_svc_def *svc,
                       const struct ble_gatt_chr_def *chr,
                       const struct ble_gatt_dsc_def *dsc,
                       uint16_t chr_def_handle,
                       ble_gatt_register_fn *register_cb, void *cb_arg)
{
    struct ble_gatt_register_ctxt register_ctxt;
    uint16_t dsc_handle;
    int rc;

    if (!ble_gatts_dsc_is_sane(dsc)) {
        return BLE_HS_EINVAL;
    }

    rc = ble_att_svr_register(dsc->uuid, dsc->att_flags, dsc->min_key_size,
                              &dsc_handle, ble_gatts_dsc_access, (void *)dsc);
    if (rc != 0) {
        return rc;
    }

    if (register_cb != NULL) {
        register_ctxt.op = BLE_GATT_REGISTER_OP_DSC;
        register_ctxt.dsc.handle = dsc_handle;
        register_ctxt.dsc.svc_def = svc;
        register_ctxt.dsc.chr_def = chr;
        register_ctxt.dsc.dsc_def = dsc;
        register_cb(&register_ctxt, cb_arg);
    }

    STATS_INC(ble_gatts_stats, dscs);

    return 0;

}

static int
ble_gatts_cpfd_is_sane(const struct ble_gatt_cpfd *cpfd)
{
    /** As per Assigned Numbers Specification (2023-09-07) */
    if ((cpfd->format < 0x01) || (cpfd->format > 0x1C)) {
        return 0;
    }

    if ((cpfd->unit < 0x2700) ||
        ((cpfd->unit > 0x2707) && (cpfd->unit < 0x2710)) ||
        (cpfd->unit == 0x271F) ||
        ((cpfd->unit > 0x2735) && (cpfd->unit < 0x2740)) ||
        ((cpfd->unit > 0x2757) && (cpfd->unit < 0x2760)) ||
        ((cpfd->unit > 0x2768) && (cpfd->unit < 0x2780)) ||
        ((cpfd->unit > 0x2787) && (cpfd->unit < 0x27A0)) ||
        (cpfd->unit == 0x27BB) ||
        (cpfd->unit > 0x27C8)) {
        return 0;
    }

    if ((cpfd->name_space == BLE_GATT_CHR_NAMESPACE_BT_SIG) && (cpfd->description > 0x0110)) {
        return 0;
    }

    return 1;
}

#if MYNEWT_VAL(BLE_DYNAMIC_SERVICE)
static struct ble_gatts_clt_cfg *
ble_gatts_clt_cfg_find(struct ble_gatts_clt_cfg_list *ble_gatts_clt_cfgs,
                           uint16_t chr_val_handle)
{
    struct ble_gatts_clt_cfg *cfg;

    STAILQ_FOREACH(cfg, ble_gatts_clt_cfgs, next) {
        if (cfg->chr_val_handle == chr_val_handle) {
            return cfg;
        }
    }

    return NULL;
}

#else
static int
ble_gatts_clt_cfg_find_idx(struct ble_gatts_clt_cfg *cfgs,
                           uint16_t chr_val_handle)
{
    struct ble_gatts_clt_cfg *cfg;
    int i;

    for (i = 0; i < ble_gatts_num_cfgable_chrs; i++) {
        cfg = cfgs + i;
        if (cfg->chr_val_handle == chr_val_handle) {
            return i;
        }
    }

    return -1;
}

static struct ble_gatts_clt_cfg *
ble_gatts_clt_cfg_find(struct ble_gatts_clt_cfg *cfgs,
                       uint16_t chr_val_handle)
{
    int idx;

    idx = ble_gatts_clt_cfg_find_idx(cfgs, chr_val_handle);
    if (idx == -1) {
        return NULL;
    } else {
        return cfgs + idx;
    }
}
#endif

static void
ble_gatts_subscribe_event(uint16_t conn_handle, uint16_t attr_handle,
                          uint8_t reason,
                          uint8_t prev_flags, uint8_t cur_flags)
{
    if ((prev_flags ^ cur_flags) & ~BLE_GATTS_CLT_CFG_F_RESERVED) {
        ble_gap_subscribe_event(conn_handle,
                                attr_handle,
                                reason,
                                prev_flags  & BLE_GATTS_CLT_CFG_F_NOTIFY,
                                cur_flags   & BLE_GATTS_CLT_CFG_F_NOTIFY,
                                prev_flags  & BLE_GATTS_CLT_CFG_F_INDICATE,
                                cur_flags   & BLE_GATTS_CLT_CFG_F_INDICATE);
    }
}

/**
 * Performs a read or write access on a client characteritic configuration
 * descriptor (CCCD).
 *
 * @param conn                  The connection of the peer doing the accessing.
 * @apram attr_handle           The handle of the CCCD.
 * @param att_op                The ATT operation being performed (read or
 *                                  write).
 * @param ctxt                  Communication channel between this function and
 *                                  the caller within the nimble stack.
 *                                  Semantics depends on the operation being
 *                                  performed.
 * @param out_cccd              If the CCCD should be persisted as a result of
 *                                  the access, the data-to-be-persisted gets
 *                                  written here.  If no persistence is
 *                                  necessary, out_cccd->chr_val_handle is set
 *                                  to 0.
 *
 * @return                      0 on success; nonzero on failure.
 */
static int
ble_gatts_clt_cfg_access_locked(struct ble_hs_conn *conn, uint16_t attr_handle,
                                uint8_t att_op, uint16_t offset,
                                struct os_mbuf *om,
                                struct ble_store_value_cccd *out_cccd,
                                uint8_t *out_prev_clt_cfg_flags,
                                uint8_t *out_cur_clt_cfg_flags)
{
    struct ble_gatts_clt_cfg *clt_cfg;
    uint16_t chr_val_handle;
    uint16_t flags;
    uint8_t gatt_op;
    uint8_t *buf;

    /* Assume nothing needs to be persisted. */
    out_cccd->chr_val_handle = 0;

    /* We always register the client characteristics descriptor with handle
     * (chr_val + 1).
     */
    chr_val_handle = attr_handle - 1;
    if (chr_val_handle > attr_handle) {
        /* Attribute handle wrapped somehow. */
        return BLE_ATT_ERR_UNLIKELY;
    }

#if MYNEWT_VAL(BLE_DYNAMIC_SERVICE)
    clt_cfg = ble_gatts_clt_cfg_find(&conn->bhc_gatt_svr.clt_cfgs,
                                     chr_val_handle);
#else
    clt_cfg = ble_gatts_clt_cfg_find(conn->bhc_gatt_svr.clt_cfgs,
                                     chr_val_handle);
#endif
    if (clt_cfg == NULL) {
        return BLE_ATT_ERR_UNLIKELY;
    }

    /* Assume no change in flags. */
    *out_prev_clt_cfg_flags = clt_cfg->flags;
    *out_cur_clt_cfg_flags = clt_cfg->flags;

    gatt_op = ble_gatts_dsc_op(att_op);
    ble_gatts_dsc_inc_stat(gatt_op);

    switch (gatt_op) {
    case BLE_GATT_ACCESS_OP_READ_DSC:
        STATS_INC(ble_gatts_stats, dsc_reads);
        buf = os_mbuf_extend(om, 2);
        if (buf == NULL) {
            return BLE_ATT_ERR_INSUFFICIENT_RES;
        }
        put_le16(buf, clt_cfg->flags & ~BLE_GATTS_CLT_CFG_F_RESERVED);
        break;

    case BLE_GATT_ACCESS_OP_WRITE_DSC:
        STATS_INC(ble_gatts_stats, dsc_writes);
        if (OS_MBUF_PKTLEN(om) != 2) {
            return BLE_ATT_ERR_INVALID_ATTR_VALUE_LEN;
        }

        om = os_mbuf_pullup(om, 2);
        BLE_HS_DBG_ASSERT(om != NULL);

        flags = get_le16(om->om_data);
        if ((flags & ~clt_cfg->allowed) != 0) {
            return BLE_ATT_ERR_REQ_NOT_SUPPORTED;
        }

        if (clt_cfg->flags != flags) {
            clt_cfg->flags = flags;
            *out_cur_clt_cfg_flags = flags;

            /* Successful writes get persisted for bonded connections. */
            if (conn->bhc_sec_state.bonded) {
                out_cccd->peer_addr = conn->bhc_peer_addr;
                out_cccd->peer_addr.type =
                    ble_hs_misc_peer_addr_type_to_id(conn->bhc_peer_addr.type);
                out_cccd->chr_val_handle = chr_val_handle;
                out_cccd->flags = clt_cfg->flags;
                out_cccd->value_changed = 0;
            }
        }
        break;

    default:
        BLE_HS_DBG_ASSERT(0);
        return BLE_ATT_ERR_UNLIKELY;
    }

    return 0;
}

int
ble_gatts_clt_cfg_access(uint16_t conn_handle, uint16_t attr_handle,
                         uint8_t op, uint16_t offset, struct os_mbuf **om,
                         void *arg)
{
    struct ble_store_value_cccd cccd_value;
    struct ble_store_key_cccd cccd_key;
    struct ble_hs_conn *conn;
    uint16_t chr_val_handle;
    uint8_t prev_flags;
    uint8_t cur_flags;
    int rc;

    ble_hs_lock();

    conn = ble_hs_conn_find(conn_handle);
    if (conn == NULL) {
        rc = BLE_ATT_ERR_UNLIKELY;
    } else {
        rc = ble_gatts_clt_cfg_access_locked(conn, attr_handle, op, offset,
                                             *om, &cccd_value, &prev_flags,
                                             &cur_flags);
    }

    ble_hs_unlock();

    if (rc != 0) {
        return rc;
    }

    /* The value attribute is always immediately after the declaration. */
    chr_val_handle = attr_handle - 1;

    /* Tell the application if the peer changed its subscription state. */
    ble_gatts_subscribe_event(conn_handle, chr_val_handle,
                              BLE_GAP_SUBSCRIBE_REASON_WRITE,
                              prev_flags, cur_flags);

    /* Persist the CCCD if required. */
    if (cccd_value.chr_val_handle != 0) {
        if (cccd_value.flags == 0) {
            ble_store_key_from_value_cccd(&cccd_key, &cccd_value);
            rc = ble_store_delete_cccd(&cccd_key);
        } else {
            rc = ble_store_write_cccd(&cccd_value);
        }
    }

    return rc;
}

static int
ble_gatts_register_clt_cfg_dsc(uint16_t *att_handle)
{
    int rc;

    rc = ble_att_svr_register(uuid_ccc, BLE_ATT_F_READ | BLE_ATT_F_WRITE, 0,
                              att_handle, ble_gatts_clt_cfg_access, NULL);
    if (rc != 0) {
        return rc;
    }

    STATS_INC(ble_gatts_stats, dscs);

    return 0;
}

static int
ble_gatts_cafd_access(uint16_t conn_handle, uint16_t attr_handle,
                      uint8_t op, uint16_t offset, struct os_mbuf **om,
                      void *arg)
{
    struct ble_att_svr_entry * cpfd_entry;
    uint16_t handle;
    int rc;

    BLE_HS_DBG_ASSERT(op == BLE_ATT_ACCESS_OP_READ);

    STATS_INC(ble_gatts_stats, dsc_reads);

    cpfd_entry = arg;

    /**
     * All the Client Presentation Format Descriptors of this characteristic
     * are registered just before the Client Aggregate Format Descriptor.
     * The handle of the first one is retrieved from arg.
     */
    rc = 0;
    for (handle = cpfd_entry->ha_handle_id; handle < attr_handle; handle++) {
        rc += os_mbuf_append(*om, &handle, sizeof(handle));
    }

    return ((rc == 0) ? 0 : BLE_ATT_ERR_INSUFFICIENT_RES);
}

static int
ble_gatts_cpfd_access(uint16_t conn_handle, uint16_t attr_handle,
                      uint8_t op, uint16_t offset, struct os_mbuf **om,
                      void *arg)
{
    struct ble_gatt_cpfd * cpfd;
    int rc;

    BLE_HS_DBG_ASSERT(op == BLE_ATT_ACCESS_OP_READ);

    STATS_INC(ble_gatts_stats, dsc_reads);

    cpfd = arg;

    rc = 0;
    rc += os_mbuf_append(*om, &(cpfd->format), sizeof(cpfd->format));
    rc += os_mbuf_append(*om, &(cpfd->exponent), sizeof(cpfd->exponent));
    rc += os_mbuf_append(*om, &(cpfd->unit), sizeof(cpfd->unit));
    rc += os_mbuf_append(*om, &(cpfd->name_space), sizeof(cpfd->name_space));
    rc += os_mbuf_append(*om, &(cpfd->description), sizeof(cpfd->description));

    return ((rc == 0) ? 0 : BLE_ATT_ERR_INSUFFICIENT_RES);
}

static int
ble_gatts_register_cpfds(const struct ble_gatt_cpfd *cpfds)
{
    int idx;
    int rc;
    uint16_t first_cpfd_handle;
    struct ble_att_svr_entry * first_cpfd_entry;

    if (cpfds == NULL) {
        /** No Client Presentation Format Descriptors to add */
        return 0;
    }

    for (idx = 0; cpfds[idx].format != 0; idx++) {
        rc = ble_att_svr_register(uuid_cpf, BLE_ATT_F_READ, 0, &first_cpfd_handle,
                                  ble_gatts_cpfd_access, (void *)(cpfds + idx));
        if (rc != 0) {
            return rc;
        }

        STATS_INC(ble_gatts_stats, dscs);
    }

    /**
     * Client Aggregate Format Descriptor required if
     * more than one CPFDs are registered for the same characteristic
     */
    if (idx > 1) {
        first_cpfd_handle -= (idx - 1);
        first_cpfd_entry = ble_att_svr_find_by_handle(first_cpfd_handle);
        if (first_cpfd_entry == NULL) {
            return BLE_HS_ENOENT;
        }

        /**
         * The First CPFD entry will contain it's handle,
         * Using that and the handle of this descriptor we can
         * find the handles all CPFDs.
         */
        rc = ble_att_svr_register(uuid_caf, BLE_ATT_F_READ, 0, NULL,
                                  ble_gatts_cafd_access, (void *)(first_cpfd_entry));
        if (rc != 0) {
            return rc;
        }

        STATS_INC(ble_gatts_stats, dscs);
    }

    return 0;
}


static int
ble_gatts_register_chr(const struct ble_gatt_svc_def *svc,
                       const struct ble_gatt_chr_def *chr,
                       ble_gatt_register_fn *register_cb, void *cb_arg)
{
    struct ble_gatt_register_ctxt register_ctxt;
    struct ble_gatt_dsc_def *dsc;
    uint16_t def_handle;
    uint16_t val_handle;
    uint16_t dsc_handle;
    uint8_t att_flags;
    int rc;

    if (!ble_gatts_chr_is_sane(chr)) {
        return BLE_HS_EINVAL;
    }

    if (ble_gatts_chr_clt_cfg_allowed(chr) != 0) {
#if !MYNEWT_VAL(BLE_DYNAMIC_SERVICE)
        if (ble_gatts_num_cfgable_chrs > ble_hs_max_client_configs) {
            return BLE_HS_ENOMEM;
        }
#endif
        ble_gatts_num_cfgable_chrs++;
    }

    /* Register characteristic definition attribute (cast away const on
     * callback arg).
     */
    rc = ble_att_svr_register(uuid_chr, BLE_ATT_F_READ, 0, &def_handle,
                              ble_gatts_chr_def_access, (void *)chr);
    if (rc != 0) {
        return rc;
    }

    /* Register characteristic value attribute (cast away const on callback
     * arg).
     */
    att_flags = ble_gatts_att_flags_from_chr_flags(chr->flags);
    rc = ble_att_svr_register(chr->uuid, att_flags, chr->min_key_size,
                              &val_handle, ble_gatts_chr_val_access,
                              (void *)chr);
    if (rc != 0) {
        return rc;
    }
    BLE_HS_DBG_ASSERT(val_handle == def_handle + 1);

    if (chr->val_handle != NULL) {
        *chr->val_handle = val_handle;
    }

    if (register_cb != NULL) {
        register_ctxt.op = BLE_GATT_REGISTER_OP_CHR;
        register_ctxt.chr.def_handle = def_handle;
        register_ctxt.chr.val_handle = val_handle;
        register_ctxt.chr.svc_def = svc;
        register_ctxt.chr.chr_def = chr;
        register_cb(&register_ctxt, cb_arg);
    }

    if (ble_gatts_chr_clt_cfg_allowed(chr) != 0) {
        rc = ble_gatts_register_clt_cfg_dsc(&dsc_handle);
        if (rc != 0) {
            return rc;
        }
        BLE_HS_DBG_ASSERT(dsc_handle == def_handle + 2);
    }

    /* Register each Client Presentation Format Descriptor. */
    rc = ble_gatts_register_cpfds(chr->cpfd);
    if (rc != 0) {
        return rc;
    }

    /* Register each descriptor. */
    if (chr->descriptors != NULL) {
        for (dsc = chr->descriptors; dsc->uuid != NULL; dsc++) {
            rc = ble_gatts_register_dsc(svc, chr, dsc, def_handle, register_cb,
                                        cb_arg);
            if (rc != 0) {
                return rc;
            }
        }
    }

    STATS_INC(ble_gatts_stats, chrs);

    return 0;
}

static int
ble_gatts_svc_type_to_uuid(uint8_t svc_type, const ble_uuid_t **uuid)
{
    switch (svc_type) {
    case BLE_GATT_SVC_TYPE_PRIMARY:
        *uuid = uuid_pri;
        return 0;

    case BLE_GATT_SVC_TYPE_SECONDARY:
        *uuid = uuid_sec;
        return 0;

    default:
        return BLE_HS_EINVAL;
    }
}

static int
ble_gatts_svc_is_sane(const struct ble_gatt_svc_def *svc)
{
    if (svc->type != BLE_GATT_SVC_TYPE_PRIMARY &&
        svc->type != BLE_GATT_SVC_TYPE_SECONDARY) {

        return 0;
    }

    if (svc->uuid == NULL) {
        return 0;
    }

    return 1;
}

static int
ble_gatts_register_svc(const struct ble_gatt_svc_def *svc,
                       uint16_t *out_handle,
                       ble_gatt_register_fn *register_cb, void *cb_arg)
{
    const struct ble_gatt_chr_def *chr;
    struct ble_gatt_register_ctxt register_ctxt;
    const ble_uuid_t *uuid;
#if MYNEWT_VAL(BLE_DYNAMIC_SERVICE)
    struct ble_gatts_svc_entry *entry;
#else
    int idx;
#endif
    int rc;
    int i;

    if (!ble_gatts_svc_incs_satisfied(svc)) {
        return BLE_HS_EAGAIN;
    }

    if (!ble_gatts_svc_is_sane(svc)) {
        return BLE_HS_EINVAL;
    }

    /* Prevent spurious maybe-uninitialized gcc warning. */
    uuid = NULL;

    rc = ble_gatts_svc_type_to_uuid(svc->type, &uuid);
    BLE_HS_DBG_ASSERT_EVAL(rc == 0);

    /* Register service definition attribute (cast away const on callback
     * arg).
     */
    rc = ble_att_svr_register(uuid, BLE_ATT_F_READ, 0, out_handle,
                              ble_gatts_svc_access, (void *)svc);
    if (rc != 0) {
        return rc;
    }

    if (register_cb != NULL) {
        register_ctxt.op = BLE_GATT_REGISTER_OP_SVC;
        register_ctxt.svc.handle = *out_handle;
        register_ctxt.svc.svc_def = svc;
        register_cb(&register_ctxt, cb_arg);
    }

    /* Register each include. */
    if (svc->includes != NULL) {
        for (i = 0; svc->includes[i] != NULL; i++) {
#if MYNEWT_VAL(BLE_DYNAMIC_SERVICE)
            entry = ble_gatts_find_svc_entry(svc->includes[i]);
            BLE_HS_DBG_ASSERT_EVAL(entry != NULL);

            rc = ble_gatts_register_inc(entry);
#else
            idx = ble_gatts_find_svc_entry_idx(svc->includes[i]);
            BLE_HS_DBG_ASSERT_EVAL(idx != -1);

            rc = ble_gatts_register_inc(ble_gatts_svc_entries + idx);
#endif
            if (rc != 0) {
                return rc;
            }
        }
    }

    /* Register each characteristic. */
    if (svc->characteristics != NULL) {
        for (chr = svc->characteristics; chr->uuid != NULL; chr++) {
            rc = ble_gatts_register_chr(svc, chr, register_cb, cb_arg);
            if (rc != 0) {
                return rc;
            }
        }
    }

    STATS_INC(ble_gatts_stats, svcs);

    return 0;
}

static int
ble_gatts_register_round(int *out_num_registered, ble_gatt_register_fn *cb,
                         void *cb_arg)
{
    struct ble_gatts_svc_entry *entry;
    uint16_t handle;
    int rc;
#if !MYNEWT_VAL(BLE_DYNAMIC_SERVICE)
    int i;
#endif

    *out_num_registered = 0;
#if MYNEWT_VAL(BLE_DYNAMIC_SERVICE)
    STAILQ_FOREACH(entry, &ble_gatts_svc_entries, next) {
#else
    for (i = 0; i < ble_gatts_num_svc_entries; i++) {
        entry = ble_gatts_svc_entries + i;

#endif
        if (entry->handle == 0) {
            rc = ble_gatts_register_svc(entry->svc, &handle, cb, cb_arg);
            switch (rc) {
            case 0:
                /* Service successfully registered. */
                entry->handle = handle;
                entry->end_group_handle = ble_att_svr_prev_handle();
                (*out_num_registered)++;
                break;

            case BLE_HS_EAGAIN:
                /* Service could not be registered due to unsatisfied includes.
                 * Try again on the next iteration.
                 */
                break;

            default:
                return rc;
            }
        }
    }

    if (*out_num_registered == 0) {
        /* There is a circular dependency. */
        return BLE_HS_EINVAL;
    }

    return 0;
}

/**
 * Registers a set of services, characteristics, and descriptors to be accessed
 * by GATT clients.
 *
 * @param svcs                  A table of the service definitions to be
 *                                  registered.
 * @param cb                    The function to call for each service,
 *                                  characteristic, and descriptor that gets
 *                                  registered.
 * @param cb_arg                The optional argument to pass to the callback
 *                                  function.
 *
 * @return                      0 on success;
 *                              BLE_HS_ENOMEM if registration failed due to
 *                                  resource exhaustion;
 *                              BLE_HS_EINVAL if the service definition table
 *                                  contains an invalid element.
 */
int
ble_gatts_register_svcs(const struct ble_gatt_svc_def *svcs,
                        ble_gatt_register_fn *cb, void *cb_arg)
{
    int total_registered;
    int cur_registered;
    int num_svcs;
    int rc;
    int i;
#if MYNEWT_VAL(BLE_DYNAMIC_SERVICE)
    struct ble_gatts_svc_entry *entry;
#else
    int idx;
#endif

    for (i = 0; svcs[i].type != BLE_GATT_SVC_TYPE_END; i++) {
#if MYNEWT_VAL(BLE_DYNAMIC_SERVICE)
        entry = ble_gatts_svc_entry_alloc();
        if (entry == NULL) {
            return BLE_HS_ENOMEM;
        }
        entry -> svc = svcs + i;
        entry -> handle = 0;
        entry -> end_group_handle = 0xffff;
        STAILQ_INSERT_TAIL(&ble_gatts_svc_entries, entry, next);
#else
        idx = ble_gatts_num_svc_entries + i;
        if (idx >= ble_hs_max_services) {
            return BLE_HS_ENOMEM;
        }

        ble_gatts_svc_entries[idx].svc = svcs + i;
        ble_gatts_svc_entries[idx].handle = 0;
        ble_gatts_svc_entries[idx].end_group_handle = 0xffff;
#endif
    }
    num_svcs = i;
#if !MYNEWT_VAL(BLE_DYNAMIC_SERVICE)
    ble_gatts_num_svc_entries += num_svcs;
#endif

    total_registered = 0;
    while (total_registered < num_svcs) {
        rc = ble_gatts_register_round(&cur_registered, cb, cb_arg);
        if (rc != 0) {
            return rc;
        }
        total_registered += cur_registered;
    }

    return 0;
}

#if !MYNEWT_VAL(BLE_DYNAMIC_SERVICE)
static int
ble_gatts_clt_cfg_size(void)
{
    return ble_gatts_num_cfgable_chrs * sizeof (struct ble_gatts_clt_cfg);
}

#endif

/**
 * Handles GATT server clean up for a terminated connection:
 *     o Informs the application that the peer is no longer subscribed to any
 *       characteristic updates.
 *     o Frees GATT server resources consumed by the connection (CCCDs).
 */
void
ble_gatts_connection_broken(uint16_t conn_handle)
{
#if MYNEWT_VAL(BLE_DYNAMIC_SERVICE)
    struct ble_gatts_clt_cfg_list clt_cfgs;
    struct ble_hs_conn *conn;
    struct ble_gatts_clt_cfg *clt_cfg;
#else
    struct ble_gatts_clt_cfg *clt_cfgs;
    struct ble_hs_conn *conn;
    int num_clt_cfgs;
    int rc;
    int i;
#endif

#if MYNEWT_VAL(BLE_GATT_CACHING)
    struct ble_hs_conn_addrs addrs;
    int i;
#endif

    /* Find the specified connection and extract its CCCD entries.  Extracting
     * the clt_cfg pointer and setting the original to null is done for two
     * reasons:
     *     1. So that the CCCD entries can be safely processed after unlocking
     *        the mutex.
     *     2. To ensure a subsequent indicate procedure for this peer is not
     *        attempted, as the connection is about to be terminated.  This
     *        avoids a spurious notify-tx GAP event callback to the
     *        application.  By setting the clt_cfg pointer to null, it is
     *        assured that the connection has no pending indications to send.
     */
    ble_hs_lock();
    conn = ble_hs_conn_find(conn_handle);
    if (conn != NULL) {
#if MYNEWT_VAL(BLE_DYNAMIC_SERVICE)
        memcpy(&clt_cfgs, &(conn->bhc_gatt_svr.clt_cfgs), sizeof(struct ble_gatts_clt_cfg_list));

        STAILQ_INIT(&(conn->bhc_gatt_svr.clt_cfgs));
#else
        clt_cfgs = conn->bhc_gatt_svr.clt_cfgs;
        num_clt_cfgs = conn->bhc_gatt_svr.num_clt_cfgs;

        conn->bhc_gatt_svr.clt_cfgs = NULL;
#endif
        conn->bhc_gatt_svr.num_clt_cfgs = 0;

#if MYNEWT_VAL(BLE_GATT_CACHING)
        /* update bonded peer aware state */
        if(conn->bhc_sec_state.bonded) {
            ble_hs_conn_addrs(conn, &addrs);
            for(i = 0; i < MYNEWT_VAL(BLE_STORE_MAX_BONDS); i++) {
                if(memcmp(ble_gatts_conn_aware_states[i].peer_id_addr,
                          addrs.peer_id_addr.val, sizeof addrs.peer_id_addr.val)) {
                    ble_gatts_conn_aware_states[i].aware = conn->bhc_gatt_svr.aware_state;
                }
            }
        }
#endif
    }
    ble_hs_unlock();

    if (conn == NULL) {
        return;
    }

    /* If there is an indicate procedure in progress for this connection,
     * inform the application that it has failed.
     */
    ble_gatts_indicate_fail_notconn(conn_handle);

    /* Now that the mutex is unlocked, inform the application that the peer is
     * no longer subscribed to any characteristic updates.
     */
#if MYNEWT_VAL(BLE_DYNAMIC_SERVICE)
    if (STAILQ_FIRST(&clt_cfgs) != NULL) {
        clt_cfg = NULL;
        while ((clt_cfg = STAILQ_FIRST(&clt_cfgs)) != NULL) {
            ble_gatts_subscribe_event(conn_handle, clt_cfg->chr_val_handle,
                    BLE_GAP_SUBSCRIBE_REASON_TERM,
                    clt_cfg->flags, 0);
            STAILQ_REMOVE_HEAD(&clt_cfgs, next);
            ble_gatts_clt_cfg_free(clt_cfg);
        }
    }
#else
    if (clt_cfgs != NULL) {
        for (i = 0; i < num_clt_cfgs; i++) {
            ble_gatts_subscribe_event(conn_handle, clt_cfgs[i].chr_val_handle,
                                      BLE_GAP_SUBSCRIBE_REASON_TERM,
                                      clt_cfgs[i].flags, 0);
        }

        rc = os_memblock_put(&ble_gatts_clt_cfg_pool, clt_cfgs);
        BLE_HS_DBG_ASSERT_EVAL(rc == 0);
    }
#endif
}

static void
ble_gatts_free_svc_defs(void)
{
#ifdef ESP_PLATFORM
    nimble_platform_mem_free(ble_gatts_svc_defs);
#else
    free(ble_gatts_svc_defs);
#endif
    ble_gatts_svc_defs = NULL;
    ble_gatts_num_svc_defs = 0;
}

static void
ble_gatts_free_mem(void)
{

#if MYNEWT_VAL(BLE_DYNAMIC_SERVICE)
    struct ble_gatts_svc_entry *entry;
    struct ble_gatts_clt_cfg *clt_cfg;
    /* free client configs memory */
    if (STAILQ_FIRST(&ble_gatts_clt_cfgs) != NULL) {
        clt_cfg = NULL;
        while ((clt_cfg = STAILQ_FIRST(&ble_gatts_clt_cfgs)) != NULL) {
            STAILQ_REMOVE_HEAD(&ble_gatts_clt_cfgs, next);
            ble_gatts_clt_cfg_free(clt_cfg);
        }
    }
#endif
#ifdef ESP_PLATFORM
    nimble_platform_mem_free(ble_gatts_clt_cfg_mem);
    ble_gatts_clt_cfg_mem = NULL;

#if MYNEWT_VAL(BLE_DYNAMIC_SERVICE)
    /* free services memory */
    if (STAILQ_FIRST(&ble_gatts_svc_entries) != NULL) {
        entry = NULL;
        while ((entry = STAILQ_FIRST(&ble_gatts_svc_entries)) != NULL) {
            STAILQ_REMOVE_HEAD(&ble_gatts_svc_entries, next);
            ble_gatts_svc_entry_free(entry);
        }
    }
    nimble_platform_mem_free(ble_gatts_svc_entry_mem);
    ble_gatts_svc_entry_mem = NULL;
#else
    nimble_platform_mem_free(ble_gatts_svc_entries);
    ble_gatts_svc_entries = NULL;
#endif
#else
    free(ble_gatts_clt_cfg_mem);
    ble_gatts_clt_cfg_mem = NULL;

    free(ble_gatts_svc_entries);
    ble_gatts_svc_entries = NULL;
#endif
}


void
ble_gatts_stop(void)
{

    ble_hs_max_services = 0;
    ble_hs_max_attrs = 0;
    ble_hs_max_client_configs = 0;

    ble_gatts_free_mem();
    ble_gatts_free_svc_defs();
    ble_att_svr_stop();
}

int
ble_gatts_start(void)
{
    struct ble_att_svr_entry *ha;
    struct ble_gatt_chr_def *chr;
    uint16_t allowed_flags;
    ble_uuid16_t uuid = BLE_UUID16_INIT(BLE_ATT_UUID_CHARACTERISTIC);
    int num_elems;
#if MYNEWT_VAL(BLE_DYNAMIC_SERVICE)
    struct ble_gatts_clt_cfg *clt_cfg;
#else
    int idx;
#endif
    int rc;
    int i;

    ble_hs_lock();
    if (!ble_gatts_mutable()) {
        rc = BLE_HS_EBUSY;
        goto done;
    }

    ble_gatts_free_mem();

    rc = ble_att_svr_start();
    if (rc != 0) {
        goto done;
    }

    if (ble_hs_max_client_configs > 0) {
#ifdef ESP_PLATFORM
        ble_gatts_clt_cfg_mem = nimble_platform_mem_malloc(
#else
        ble_gatts_clt_cfg_mem = malloc(
#endif
            OS_MEMPOOL_BYTES(ble_hs_max_client_configs,
                             sizeof (struct ble_gatts_clt_cfg)));
        if (ble_gatts_clt_cfg_mem == NULL) {
            rc = BLE_HS_ENOMEM;
            goto done;
        }
    }

    if (ble_hs_max_services > 0) {
#if MYNEWT_VAL(BLE_DYNAMIC_SERVICE)
        ble_gatts_svc_entry_mem =
            nimble_platform_mem_malloc(ble_hs_max_services * sizeof(struct ble_gatts_svc_entry));
        if (ble_gatts_svc_entry_mem == NULL) {
#else
        ble_gatts_svc_entries =
#ifdef ESP_PLATFORM
            nimble_platform_mem_malloc(ble_hs_max_services * sizeof *ble_gatts_svc_entries);
#else
            malloc(ble_hs_max_services * sizeof *ble_gatts_svc_entries);
#endif
        if (ble_gatts_svc_entries == NULL) {
#endif
            rc = BLE_HS_ENOMEM;
            goto done;
        }
    }

#if MYNEWT_VAL(BLE_DYNAMIC_SERVICE)
    rc = os_mempool_init(&ble_gatts_svc_entry_pool, ble_hs_max_services,
                         sizeof (struct ble_gatts_svc_entry),
                         ble_gatts_svc_entry_mem, "ble_gatts_svc_entry_pool");
    if (rc != 0) {
        rc = BLE_HS_EOS;
        goto done;
    }
#else

    ble_gatts_num_svc_entries = 0;
#endif

    for (i = 0; i < ble_gatts_num_svc_defs; i++) {
        rc = ble_gatts_register_svcs(ble_gatts_svc_defs[i],
                                     ble_hs_cfg.gatts_register_cb,
                                     ble_hs_cfg.gatts_register_arg);
        if (rc != 0) {
            goto done;
        }
    }
    ble_gatts_free_svc_defs();

    if (ble_gatts_num_cfgable_chrs == 0) {
        rc = 0;
        goto done;
    }

    /* Initialize client-configuration memory pool. */
#if MYNEWT_VAL(BLE_DYNAMIC_SERVICE)
    num_elems = ble_hs_max_client_configs;
    rc = os_mempool_init(&ble_gatts_clt_cfg_pool, num_elems,
                            sizeof(struct ble_gatts_clt_cfg),
                            ble_gatts_clt_cfg_mem,
                            "ble_gatts_clt_cfg_pool");
#else
    num_elems = ble_hs_max_client_configs / ble_gatts_num_cfgable_chrs;
    rc = os_mempool_init(&ble_gatts_clt_cfg_pool, num_elems,
                         ble_gatts_clt_cfg_size(), ble_gatts_clt_cfg_mem,
                         "ble_gatts_clt_cfg_pool");
#endif
    if (rc != 0) {
        rc = BLE_HS_EOS;
        goto done;
    }

#if !MYNEWT_VAL(BLE_DYNAMIC_SERVICE)
    /* Allocate the cached array of handles for the configuration
     * characteristics.
     */
    ble_gatts_clt_cfgs = os_memblock_get(&ble_gatts_clt_cfg_pool);
    if (ble_gatts_clt_cfgs == NULL) {
        rc = BLE_HS_ENOMEM;
        goto done;
    }

    memset (ble_gatts_clt_cfgs, 0, sizeof *ble_gatts_clt_cfgs);

#endif
    /* Fill the cache. */
#if MYNEWT_VAL(BLE_DYNAMIC_SERVICE)
    clt_cfg = NULL;
#else
    idx = 0;
#endif
    ha = NULL;
    while ((ha = ble_att_svr_find_by_uuid(ha, &uuid.u, 0xffff)) != NULL) {
        chr = ha->ha_cb_arg;
        allowed_flags = ble_gatts_chr_clt_cfg_allowed(chr);
        if (allowed_flags != 0) {
#if MYNEWT_VAL(BLE_DYNAMIC_SERVICE)
            clt_cfg = ble_gatts_clt_cfg_alloc();
            BLE_HS_DBG_ASSERT_EVAL(clt_cfg != NULL);

            clt_cfg->chr_val_handle = ha->ha_handle_id + 1;
            clt_cfg->allowed = allowed_flags;
            clt_cfg->flags = 0;
            STAILQ_INSERT_TAIL(&ble_gatts_clt_cfgs, clt_cfg, next);
#else
            BLE_HS_DBG_ASSERT_EVAL(idx < ble_gatts_num_cfgable_chrs);

            ble_gatts_clt_cfgs[idx].chr_val_handle = ha->ha_handle_id + 1;
            ble_gatts_clt_cfgs[idx].allowed = allowed_flags;
            ble_gatts_clt_cfgs[idx].flags = 0;
            idx++;
#endif
        }
    }

done:
    if (rc != 0) {
        ble_gatts_free_mem();
        ble_gatts_free_svc_defs();
    }

    ble_hs_unlock();
    return rc;
}

int
ble_gatts_conn_can_alloc(void)
{
    return ble_gatts_num_cfgable_chrs == 0 ||
           ble_gatts_clt_cfg_pool.mp_num_free > 0;
}

int
ble_gatts_conn_init(struct ble_gatts_conn *gatts_conn)
{
#if MYNEWT_VAL(BLE_DYNAMIC_SERVICE)
    struct ble_gatts_clt_cfg *clt_cfg;
    struct ble_gatts_clt_cfg *clt_cfg_new;
    struct ble_gatts_clt_cfg_list clt_cfgs;
    int rc;
    STAILQ_INIT(&clt_cfgs);
    rc = 0;

    if (ble_gatts_num_cfgable_chrs > 0) {
        /* Initialize the client configuration with a copy of the cache. */
        STAILQ_FOREACH(clt_cfg, &ble_gatts_clt_cfgs, next) {
            clt_cfg_new = ble_gatts_clt_cfg_alloc();
            if (clt_cfg_new == NULL) {
                rc = BLE_HS_ENOMEM;
                goto done;
            }
            memcpy(clt_cfg_new, clt_cfg, sizeof(struct ble_gatts_clt_cfg));
            STAILQ_INSERT_TAIL(&clt_cfgs, clt_cfg_new, next);
        }
        memcpy(&(gatts_conn->clt_cfgs), &clt_cfgs, sizeof(struct ble_gatts_clt_cfg_list));
        gatts_conn->num_clt_cfgs = ble_gatts_num_cfgable_chrs;
    } else {
        STAILQ_INIT(&(gatts_conn->clt_cfgs));
        gatts_conn->num_clt_cfgs = 0;
    }

done:
    if (rc != 0) {
    /* free clt_cfgs_list entries */
        clt_cfg = NULL;
        while ((clt_cfg = STAILQ_FIRST(&clt_cfgs)) != NULL) {
            STAILQ_REMOVE_HEAD(&clt_cfgs, next);
            ble_gatts_clt_cfg_free(clt_cfg);
        }
        return rc;
    }
#else
    if (ble_gatts_num_cfgable_chrs > 0) {
        gatts_conn->clt_cfgs = os_memblock_get(&ble_gatts_clt_cfg_pool);
        if (gatts_conn->clt_cfgs == NULL) {
            return BLE_HS_ENOMEM;
        }

        /* Initialize the client configuration with a copy of the cache. */
        memcpy(gatts_conn->clt_cfgs, ble_gatts_clt_cfgs,
               ble_gatts_clt_cfg_size());
        gatts_conn->num_clt_cfgs = ble_gatts_num_cfgable_chrs;
    } else {
        gatts_conn->clt_cfgs = NULL;
        gatts_conn->num_clt_cfgs = 0;
    }
#endif

    return 0;
}

/**
 * Schedules a notification or indication for the specified peer-CCCD pair.  If
 * the update should be sent immediately, it is indicated in the return code.
 *
 * @param conn                  The connection to schedule the update for.
 * @param clt_cfg               The client config entry corresponding to the
 *                                  peer and affected characteristic.
 *
 * @return                      The att_op of the update to send immediately,
 *                                  if any.  0 if nothing should get sent.
 */
static uint8_t
ble_gatts_schedule_update(struct ble_hs_conn *conn,
                          struct ble_gatts_clt_cfg *clt_cfg)
{
    uint8_t att_op;

    if (!(clt_cfg->flags & BLE_GATTS_CLT_CFG_F_MODIFIED)) {
        /* Characteristic not modified.  Nothing to send. */
        att_op = 0;
    } else if (clt_cfg->flags & BLE_GATTS_CLT_CFG_F_NOTIFY) {
        /* Notifications always get sent immediately. */
        att_op = BLE_ATT_OP_NOTIFY_REQ;
    } else if (clt_cfg->flags & BLE_GATTS_CLT_CFG_F_INDICATE) {
        /* Only one outstanding indication per peer is allowed.  If we
         * are still awaiting an ack, mark this CCCD as updated so that
         * we know to send the indication upon receiving the expected ack.
         * If there isn't an outstanding indication, send this one now.
         */
        if (conn->bhc_gatt_svr.indicate_val_handle != 0) {
            att_op = 0;
        } else {
            att_op = BLE_ATT_OP_INDICATE_REQ;
        }
    } else {
        /* Peer isn't subscribed to notifications or indications.  Nothing to
         * send.
         */
        att_op = 0;
    }

    /* If we will be sending an update, clear the modified flag so that we
     * don't double-send.
     */
    if (att_op != 0) {
        clt_cfg->flags &= ~BLE_GATTS_CLT_CFG_F_MODIFIED;
    }

    return att_op;
}

int
ble_gatts_send_next_indicate(uint16_t conn_handle)
{
    struct ble_gatts_clt_cfg *clt_cfg;
    struct ble_hs_conn *conn;
    uint16_t chr_val_handle;
    int rc;
#if !MYNEWT_VAL(BLE_DYNAMIC_SERVICE)
    int i;
#endif

    /* Assume no pending indications. */
    chr_val_handle = 0;

    ble_hs_lock();

    conn = ble_hs_conn_find(conn_handle);
    if (conn != NULL) {
#if MYNEWT_VAL(BLE_DYNAMIC_SERVICE)
        STAILQ_FOREACH(clt_cfg, &conn->bhc_gatt_svr.clt_cfgs, next) {
#else
        for (i = 0; i < conn->bhc_gatt_svr.num_clt_cfgs; i++) {
            clt_cfg = conn->bhc_gatt_svr.clt_cfgs + i;
#endif
            if (clt_cfg->flags & BLE_GATTS_CLT_CFG_F_MODIFIED) {
                BLE_HS_DBG_ASSERT(clt_cfg->flags &
                                  BLE_GATTS_CLT_CFG_F_INDICATE);

                chr_val_handle = clt_cfg->chr_val_handle;

                /* Clear pending flag in anticipation of indication tx. */
                clt_cfg->flags &= ~BLE_GATTS_CLT_CFG_F_MODIFIED;
                break;
            }
        }
    }

    ble_hs_unlock();

    if (conn == NULL) {
        return BLE_HS_ENOTCONN;
    }

    if (chr_val_handle == 0) {
        return BLE_HS_ENOENT;
    }

    rc = ble_gatts_indicate(conn_handle, chr_val_handle);
    if (rc != 0) {
        return rc;
    }

    return 0;
}

int
ble_gatts_rx_indicate_ack(uint16_t conn_handle, uint16_t chr_val_handle)
{
    struct ble_store_value_cccd cccd_value;
    struct ble_gatts_clt_cfg *clt_cfg;
    struct ble_hs_conn *conn;
#if !MYNEWT_VAL(BLE_DYNAMIC_SERVICE)
    int clt_cfg_idx;
#endif
    int persist;
    int rc;
#if MYNEWT_VAL(BLE_GATT_CACHING)
    uint16_t svc_change_handle;

    svc_change_handle = ble_svc_gatt_changed_handle();
#endif

#if MYNEWT_VAL(BLE_DYNAMIC_SERVICE)
    clt_cfg = ble_gatts_clt_cfg_find(&ble_gatts_clt_cfgs,
                                             chr_val_handle);
    if (clt_cfg == NULL) {
#else
    clt_cfg_idx = ble_gatts_clt_cfg_find_idx(ble_gatts_clt_cfgs,
                                             chr_val_handle);
    if (clt_cfg_idx == -1) {
#endif
        /* This characteristic does not have a CCCD. */
        return BLE_HS_ENOENT;
    }

#if !MYNEWT_VAL(BLE_DYNAMIC_SERVICE)
    clt_cfg = ble_gatts_clt_cfgs + clt_cfg_idx;
#endif
    if (!(clt_cfg->allowed & BLE_GATTS_CLT_CFG_F_INDICATE)) {
        /* This characteristic does not allow indications. */
        return BLE_HS_ENOENT;
    }

    ble_hs_lock();

    conn = ble_hs_conn_find(conn_handle);
    BLE_HS_DBG_ASSERT(conn != NULL);
    if (conn->bhc_gatt_svr.indicate_val_handle == chr_val_handle) {
        /* This acknowledgement is expected. */
        rc = 0;

        /* Mark that there is no longer an outstanding txed indicate. */
        conn->bhc_gatt_svr.indicate_val_handle = 0;

        /* Determine if we need to persist that there is no pending indication
         * for this peer-characteristic pair.  If the characteristic has not
         * been modified since we sent the indication, there is no indication
         * pending.
         */
#if MYNEWT_VAL(BLE_GATT_CACHING)
    if(chr_val_handle == svc_change_handle) {
        conn->bhc_gatt_svr.aware_state = true;
    }
#endif
#if MYNEWT_VAL(BLE_DYNAMIC_SERVICE)
        clt_cfg = ble_gatts_clt_cfg_find(&conn->bhc_gatt_svr.clt_cfgs, chr_val_handle);
        BLE_HS_DBG_ASSERT(clt_cfg != NULL);
#else
        BLE_HS_DBG_ASSERT(conn->bhc_gatt_svr.num_clt_cfgs > clt_cfg_idx);
        clt_cfg = conn->bhc_gatt_svr.clt_cfgs + clt_cfg_idx;
#endif
        BLE_HS_DBG_ASSERT(clt_cfg->chr_val_handle == chr_val_handle);

        persist = conn->bhc_sec_state.bonded &&
                  !(clt_cfg->flags & BLE_GATTS_CLT_CFG_F_MODIFIED);
        if (persist) {
            cccd_value.peer_addr = conn->bhc_peer_addr;
            cccd_value.peer_addr.type =
                ble_hs_misc_peer_addr_type_to_id(conn->bhc_peer_addr.type);
            cccd_value.chr_val_handle = chr_val_handle;
            cccd_value.flags = clt_cfg->flags;
            cccd_value.value_changed = 0;
        }
    } else {
        /* This acknowledgement doesn't correspond to the outstanding
         * indication; ignore it.
         */
        rc = BLE_HS_ENOENT;
    }

    ble_hs_unlock();

    if (rc != 0) {
        return rc;
    }

    if (persist) {
        rc = ble_store_write_cccd(&cccd_value);
        if (rc != 0) {
            /* XXX: How should this error get reported? */
        }
    }

    return 0;
}

void
ble_gatts_chr_updated(uint16_t chr_val_handle)
{
    struct ble_store_value_cccd cccd_value;
    struct ble_store_key_cccd cccd_key;
    struct ble_gatts_clt_cfg *clt_cfg;
    struct ble_hs_conn *conn;
    int new_notifications = 0;
#if !MYNEWT_VAL(BLE_DYNAMIC_SERVICE)
    int clt_cfg_idx;
#endif
    int persist;
    int rc;
    int i;

    /* Determine if notifications or indications are allowed for this
     * characteristic.  If not, return immediately.
     */
#if MYNEWT_VAL(BLE_DYNAMIC_SERVICE)
    clt_cfg = ble_gatts_clt_cfg_find(&ble_gatts_clt_cfgs,
                                             chr_val_handle);
    if (clt_cfg == NULL) {
#else
    clt_cfg_idx = ble_gatts_clt_cfg_find_idx(ble_gatts_clt_cfgs,
                                             chr_val_handle);
    if (clt_cfg_idx == -1) {
#endif
        return;
    }

    /*** Send notifications and indications to connected devices. */

    ble_hs_lock();
    for (i = 0; ; i++) {
        /* XXX: This is inefficient when there are a lot of connections.
         * Consider using a "foreach" function to walk the connection list.
         */
        conn = ble_hs_conn_find_by_idx(i);
        if (conn == NULL) {
            break;
        }

#if MYNEWT_VAL(BLE_DYNAMIC_SERVICE)
        clt_cfg = ble_gatts_clt_cfg_find(&conn->bhc_gatt_svr.clt_cfgs,
                                         chr_val_handle);

	if (clt_cfg == NULL) {
	    break;
	}
#else
	if (conn->bhc_gatt_svr.clt_cfgs == NULL ) {
	    break;
	}
        BLE_HS_DBG_ASSERT_EVAL(conn->bhc_gatt_svr.num_clt_cfgs >
                               clt_cfg_idx);
        clt_cfg = conn->bhc_gatt_svr.clt_cfgs + clt_cfg_idx;
#endif
        BLE_HS_DBG_ASSERT_EVAL(clt_cfg->chr_val_handle == chr_val_handle);

        /* Mark the CCCD entry as modified. */
        clt_cfg->flags |= BLE_GATTS_CLT_CFG_F_MODIFIED;
        new_notifications = 1;
    }
    ble_hs_unlock();

    if (new_notifications) {
        ble_hs_notifications_sched();
    }

    /*** Persist updated flag for unconnected and not-yet-bonded devices. */

    /* Retrieve each record corresponding to the modified characteristic. */
    cccd_key.peer_addr = *BLE_ADDR_ANY;
    cccd_key.chr_val_handle = chr_val_handle;
    cccd_key.idx = 0;

    while (1) {
        rc = ble_store_read_cccd(&cccd_key, &cccd_value);
        if (rc != 0) {
            /* Read error or no more CCCD records. */
            break;
        }

        /* Determine if this record needs to be rewritten. */
        ble_hs_lock();
        conn = ble_hs_conn_find_by_addr(&cccd_key.peer_addr);

        if (conn == NULL) {
            /* Device isn't connected; persist the changed flag so that an
             * update can be sent when the device reconnects and rebonds.
             */
            persist = 1;
        } else if (cccd_value.flags & BLE_GATTS_CLT_CFG_F_INDICATE) {
            /* Indication for a connected device; record that the
             * characteristic has changed until we receive the ack.
             */
            persist = 1;
        } else {
            /* Notification for a connected device; we already sent it so there
             * is no need to persist.
             */
            persist = 0;
        }

        ble_hs_unlock();

        /* Only persist if the value changed flag wasn't already sent (i.e.,
         * don't overwrite with identical data).
         */
        if (persist && !cccd_value.value_changed) {
            cccd_value.value_changed = 1;
            ble_store_write_cccd(&cccd_value);
        }

        /* Read the next matching record. */
        cccd_key.idx++;
    }
}

int
ble_gatts_peer_cl_sup_feat_get(uint16_t conn_handle, uint8_t *out_supported_feat, uint8_t len)
{
    struct ble_hs_conn *conn;
    int rc = 0;

    if (out_supported_feat == NULL) {
        return BLE_HS_EINVAL;
    }

    ble_hs_lock();
    conn = ble_hs_conn_find(conn_handle);
    if (conn == NULL) {
        rc = BLE_HS_ENOTCONN;
        goto done;
    }

    if (BLE_GATT_CHR_CLI_SUP_FEAT_SZ < len) {
        len = BLE_GATT_CHR_CLI_SUP_FEAT_SZ;
    }

    memcpy(out_supported_feat, conn->bhc_gatt_svr.peer_cl_sup_feat,
           sizeof(uint8_t) * len);

done:
    ble_hs_unlock();
    return rc;
}

int
ble_gatts_peer_cl_sup_feat_update(uint16_t conn_handle, struct os_mbuf *om)
{
    struct ble_hs_conn *conn;
    uint8_t feat[BLE_GATT_CHR_CLI_SUP_FEAT_SZ] = {};
    uint16_t len;
    int rc = 0;
    int i;

    if (!om) {
        return BLE_ATT_ERR_INSUFFICIENT_RES;
    }

    /* RFU bits are ignored so we can skip any bytes larger than supported */
    len = os_mbuf_len(om);
    if (len > BLE_GATT_CHR_CLI_SUP_FEAT_SZ) {
        len = BLE_GATT_CHR_CLI_SUP_FEAT_SZ;
    }

    if (os_mbuf_copydata(om, 0, len, feat) < 0) {
        return BLE_ATT_ERR_UNLIKELY;
    }

    /* clear RFU bits */
    for (i = 0; i < BLE_GATT_CHR_CLI_SUP_FEAT_SZ; i++) {
        feat[i] &= (BLE_GATT_CHR_CLI_SUP_FEAT_MASK >> (8 * i));
    }

    ble_hs_lock();
    conn = ble_hs_conn_find(conn_handle);
    if (conn == NULL) {
        rc = BLE_ATT_ERR_UNLIKELY;
        goto done;
    }

    /**
     * Disabling already enabled features is not permitted
     * (Vol. 3, Part F, 3.3.3)
     */
    for (i = 0; i < BLE_GATT_CHR_CLI_SUP_FEAT_SZ; i++) {
        if ((conn->bhc_gatt_svr.peer_cl_sup_feat[i] & feat[i]) !=
            conn->bhc_gatt_svr.peer_cl_sup_feat[i]) {
            rc = BLE_ATT_ERR_VALUE_NOT_ALLOWED;
            goto done;
        }
    }

    memcpy(conn->bhc_gatt_svr.peer_cl_sup_feat, feat, BLE_GATT_CHR_CLI_SUP_FEAT_SZ);

done:
    ble_hs_unlock();
    return rc;
}

/**
 * Sends notifications or indications for the specified characteristic to all
 * connected devices.  The bluetooth spec does not allow more than one
 * concurrent indication for a single peer, so this function will hold off on
 * sending such indications.
 */
static void
ble_gatts_tx_notifications_one_chr(uint16_t chr_val_handle)
{
    struct ble_gatts_clt_cfg *clt_cfg;
    struct ble_hs_conn *conn;
    uint16_t conn_handle;
    uint8_t att_op;
#if !MYNEWT_VAL(BLE_DYNAMIC_SERVICE)
    int clt_cfg_idx;
#endif
    int i;

    /* Determine if notifications / indications are enabled for this
     * characteristic.
     */
#if MYNEWT_VAL(BLE_DYNAMIC_SERVICE)
    clt_cfg = ble_gatts_clt_cfg_find(&ble_gatts_clt_cfgs,
                                             chr_val_handle);
    if (clt_cfg == NULL) {
#else
    clt_cfg_idx = ble_gatts_clt_cfg_find_idx(ble_gatts_clt_cfgs,
                                             chr_val_handle);
    if (clt_cfg_idx == -1) {
#endif
        return;
    }

    for (i = 0; ; i++) {
        ble_hs_lock();

        conn = ble_hs_conn_find_by_idx(i);
        if (conn != NULL) {
#if MYNEWT_VAL(BLE_DYNAMIC_SERVICE)
            clt_cfg = ble_gatts_clt_cfg_find(&conn->bhc_gatt_svr.clt_cfgs,
                                             chr_val_handle);
#else
            BLE_HS_DBG_ASSERT_EVAL(conn->bhc_gatt_svr.num_clt_cfgs >
                                   clt_cfg_idx);
            clt_cfg = conn->bhc_gatt_svr.clt_cfgs + clt_cfg_idx;
#endif
            BLE_HS_DBG_ASSERT_EVAL(clt_cfg->chr_val_handle == chr_val_handle);

            /* Determine what type of command should get sent, if any. */
            att_op = ble_gatts_schedule_update(conn, clt_cfg);
            conn_handle = conn->bhc_handle;
        } else {
            /* Silence some spurious gcc warnings. */
            att_op = 0;
            conn_handle = BLE_HS_CONN_HANDLE_NONE;
        }
        ble_hs_unlock();

        if (conn == NULL) {
            /* No more connected devices. */
            break;
        }

        switch (att_op) {
        case 0:
            break;

        case BLE_ATT_OP_NOTIFY_REQ:
            ble_gatts_notify(conn_handle, chr_val_handle);
            break;

        case BLE_ATT_OP_INDICATE_REQ:
            ble_gatts_indicate(conn_handle, chr_val_handle);
            break;

        default:
            BLE_HS_DBG_ASSERT(0);
            break;
        }
    }
}

/**
 * Sends all pending notifications and indications.  The bluetooth spec does
 * not allow more than one concurrent indication for a single peer, so this
 * function will hold off on sending such indications.
 */
void
ble_gatts_tx_notifications(void)
{
    uint16_t chr_val_handle;
#if MYNEWT_VAL(BLE_DYNAMIC_SERVICE)
    struct ble_gatts_clt_cfg *clt_cfg;
    STAILQ_FOREACH(clt_cfg, &ble_gatts_clt_cfgs, next) {
        chr_val_handle = clt_cfg->chr_val_handle;
#else
    int i;

    for (i = 0; i < ble_gatts_num_cfgable_chrs; i++) {
        chr_val_handle = ble_gatts_clt_cfgs[i].chr_val_handle;
#endif
        ble_gatts_tx_notifications_one_chr(chr_val_handle);
    }
}

void
ble_gatts_bonding_established(uint16_t conn_handle)
{
    struct ble_store_value_cccd cccd_value;
    struct ble_gatts_clt_cfg *clt_cfg;
    struct ble_gatts_conn *gatt_srv;
    struct ble_hs_conn *conn;
#if !MYNEWT_VAL(BLE_DYNAMIC_SERVICE)
    int i;
#endif
#if MYNEWT_VAL(BLE_GATT_CACHING)
    struct ble_hs_conn_addrs addrs;
    int new_idx;
#endif

    ble_hs_lock();

    conn = ble_hs_conn_find(conn_handle);
    BLE_HS_DBG_ASSERT(conn != NULL);
    BLE_HS_DBG_ASSERT(conn->bhc_sec_state.bonded);

    cccd_value.peer_addr = conn->bhc_peer_addr;
    cccd_value.peer_addr.type =
        ble_hs_misc_peer_addr_type_to_id(conn->bhc_peer_addr.type);
    gatt_srv = &conn->bhc_gatt_svr;

#if MYNEWT_VAL(BLE_DYNAMIC_SERVICE)
    STAILQ_FOREACH(clt_cfg, &gatt_srv->clt_cfgs, next) {
#else
    for (i = 0; i < gatt_srv->num_clt_cfgs; ++i) {
        clt_cfg = &gatt_srv->clt_cfgs[i];
#endif

        if (clt_cfg->flags != 0) {
            cccd_value.chr_val_handle = clt_cfg->chr_val_handle;
            cccd_value.flags = clt_cfg->flags;
            cccd_value.value_changed = 0;

            /* Store write use ble_hs_lock */
            ble_hs_unlock();
            ble_store_write_cccd(&cccd_value);
            ble_hs_lock();

            conn = ble_hs_conn_find(conn_handle);
            BLE_HS_DBG_ASSERT(conn != NULL);
        }
    }

#if MYNEWT_VAL(BLE_GATT_CACHING)
    /* store the bonded peer aware_state
       if space not available delete the
       oldest bond */
    ble_hs_conn_addrs(conn, &addrs);
    new_idx = (last_conn_aware_state_index + 1) %
              MYNEWT_VAL(BLE_STORE_MAX_BONDS);
    memset(&ble_gatts_conn_aware_states[new_idx], 0,
           sizeof(struct ble_gatts_aware_state));
    memcpy(ble_gatts_conn_aware_states[new_idx].peer_id_addr,
           addrs.peer_id_addr.val, sizeof addrs.peer_id_addr.val);
    last_conn_aware_state_index = new_idx;
#endif
    ble_hs_unlock();
}

/**
 * Called when bonding has been restored via the encryption procedure.  This
 * function:
 *     o Restores persisted CCCD entries for the connected peer.
 *     o Sends all pending notifications to the connected peer.
 *     o Sends up to one pending indication to the connected peer; schedules
 *       any remaining pending indications.
 */
void
ble_gatts_bonding_restored(uint16_t conn_handle)
{
    struct ble_store_value_cccd cccd_value;
    struct ble_store_key_cccd cccd_key;
    struct ble_gatts_clt_cfg *clt_cfg;
    struct ble_hs_conn *conn;
    uint8_t att_op;
    int rc;
#if MYNEWT_VAL(BLE_GATT_CACHING)
    struct ble_hs_conn_addrs addrs;
    int i;
#endif

    ble_hs_lock();

    conn = ble_hs_conn_find(conn_handle);
    BLE_HS_DBG_ASSERT(conn != NULL);
    BLE_HS_DBG_ASSERT(conn->bhc_sec_state.bonded);

    cccd_key.peer_addr = conn->bhc_peer_addr;
    cccd_key.peer_addr.type =
        ble_hs_misc_peer_addr_type_to_id(conn->bhc_peer_addr.type);
    cccd_key.chr_val_handle = 0;
    cccd_key.idx = 0;

#if MYNEWT_VAL(BLE_GATT_CACHING)
    /* update the aware state of the client */
    ble_hs_conn_addrs(conn, &addrs);
    for(i = 0; i < MYNEWT_VAL(BLE_STORE_MAX_BONDS); i++) {
        if(memcmp(ble_gatts_conn_aware_states[i].peer_id_addr,
                          addrs.peer_id_addr.val, sizeof addrs.peer_id_addr.val)) {
            conn->bhc_gatt_svr.aware_state = ble_gatts_conn_aware_states[i].aware;
        }
    }
#endif
    ble_hs_unlock();

    while (1) {
        rc = ble_store_read_cccd(&cccd_key, &cccd_value);
        if (rc != 0) {
            break;
        }

        /* Assume no notification or indication will get sent. */
        att_op = 0;

        ble_hs_lock();

        conn = ble_hs_conn_find(conn_handle);
        BLE_HS_DBG_ASSERT(conn != NULL);

#if MYNEWT_VAL(BLE_DYNAMIC_SERVICE)
        clt_cfg = ble_gatts_clt_cfg_find(&conn->bhc_gatt_svr.clt_cfgs,
                                         cccd_value.chr_val_handle);
#else
        clt_cfg = ble_gatts_clt_cfg_find(conn->bhc_gatt_svr.clt_cfgs,
                                         cccd_value.chr_val_handle);
#endif
        if (clt_cfg != NULL) {
            clt_cfg->flags = cccd_value.flags;

            if (cccd_value.value_changed) {
                /* The characteristic's value changed while the device was
                 * disconnected or unbonded.  Schedule the notification or
                 * indication now.
                 */
                clt_cfg->flags |= BLE_GATTS_CLT_CFG_F_MODIFIED;
                att_op = ble_gatts_schedule_update(conn, clt_cfg);
            }
        }

        ble_hs_unlock();

        /* Tell the application if the peer changed its subscription state
         * when it was restored from persistence.
         */
        ble_gatts_subscribe_event(conn_handle, cccd_value.chr_val_handle,
                                  BLE_GAP_SUBSCRIBE_REASON_RESTORE,
                                  0, cccd_value.flags);

        switch (att_op) {
        case 0:
            break;

        case BLE_ATT_OP_NOTIFY_REQ:
            rc = ble_gatts_notify(conn_handle, cccd_value.chr_val_handle);
            if (rc == 0) {
                cccd_value.value_changed = 0;
                ble_store_write_cccd(&cccd_value);
            }
            break;

        case BLE_ATT_OP_INDICATE_REQ:
            ble_gatts_indicate(conn_handle, cccd_value.chr_val_handle);
            break;

        default:
            BLE_HS_DBG_ASSERT(0);
            break;
        }

        cccd_key.idx++;
    }
}

#if MYNEWT_VAL(BLE_DYNAMIC_SERVICE)
static struct ble_gatts_svc_entry *
ble_gatts_find_svc_entry_by_uuid(const ble_uuid_t *uuid)
{
    struct ble_gatts_svc_entry *entry;

    STAILQ_FOREACH(entry, &ble_gatts_svc_entries, next) {
        if (ble_uuid_cmp(uuid, entry->svc->uuid) == 0) {
            return entry;
        }
    }

    return NULL;
}
#else
static struct ble_gatts_svc_entry *
ble_gatts_find_svc_entry(const ble_uuid_t *uuid)
{
    struct ble_gatts_svc_entry *entry;
    int i;

    for (i = 0; i < ble_gatts_num_svc_entries; i++) {
        entry = ble_gatts_svc_entries + i;
        if (ble_uuid_cmp(uuid, entry->svc->uuid) == 0) {
            return entry;
        }
    }

    return NULL;
}
#endif

static int
ble_gatts_find_svc_chr_attr(const ble_uuid_t *svc_uuid,
                            const ble_uuid_t *chr_uuid,
                            struct ble_gatts_svc_entry **out_svc_entry,
                            struct ble_att_svr_entry **out_att_chr)
{
    struct ble_gatts_svc_entry *svc_entry;
    struct ble_att_svr_entry *att_svc;
    struct ble_att_svr_entry *next;
    struct ble_att_svr_entry *cur;

#if MYNEWT_VAL(BLE_DYNAMIC_SERVICE)
    svc_entry = ble_gatts_find_svc_entry_by_uuid(svc_uuid);
#else
    svc_entry = ble_gatts_find_svc_entry(svc_uuid);
#endif
    if (svc_entry == NULL) {
        return BLE_HS_ENOENT;
    }

    att_svc = ble_att_svr_find_by_handle(svc_entry->handle);
    if (att_svc == NULL) {
        return BLE_HS_EUNKNOWN;
    }

    cur = STAILQ_NEXT(att_svc, ha_next);
    while (1) {
        if (cur == NULL) {
            /* Reached end of attribute list without a match. */
            return BLE_HS_ENOENT;
        }
        next = STAILQ_NEXT(cur, ha_next);

        if (cur->ha_handle_id == svc_entry->end_group_handle) {
            /* Reached end of service without a match. */
            return BLE_HS_ENOENT;
        }

        if (ble_uuid_u16(cur->ha_uuid) == BLE_ATT_UUID_CHARACTERISTIC &&
            next != NULL &&
            ble_uuid_cmp(next->ha_uuid, chr_uuid) == 0) {

            if (out_svc_entry != NULL) {
                *out_svc_entry = svc_entry;
            }
            if (out_att_chr != NULL) {
                *out_att_chr = next;
            }
            return 0;
        }

        cur = next;
    }
}

int
ble_gatts_find_svc(const ble_uuid_t *uuid, uint16_t *out_handle)
{
    struct ble_gatts_svc_entry *entry;

#if MYNEWT_VAL(BLE_DYNAMIC_SERVICE)
    entry = ble_gatts_find_svc_entry_by_uuid(uuid);
#else
    entry = ble_gatts_find_svc_entry(uuid);
#endif
    if (entry == NULL) {
        return BLE_HS_ENOENT;
    }

    if (out_handle != NULL) {
        *out_handle = entry->handle;
    }
    return 0;
}

int
ble_gatts_find_chr(const ble_uuid_t *svc_uuid, const ble_uuid_t *chr_uuid,
                   uint16_t *out_def_handle, uint16_t *out_val_handle)
{
    struct ble_att_svr_entry *att_chr;
    int rc;

    rc = ble_gatts_find_svc_chr_attr(svc_uuid, chr_uuid, NULL, &att_chr);
    if (rc != 0) {
        return rc;
    }

    if (out_def_handle) {
        *out_def_handle = att_chr->ha_handle_id - 1;
    }
    if (out_val_handle) {
        *out_val_handle = att_chr->ha_handle_id;
    }
    return 0;
}

int
ble_gatts_find_dsc(const ble_uuid_t *svc_uuid, const ble_uuid_t *chr_uuid,
                   const ble_uuid_t *dsc_uuid, uint16_t *out_handle)
{
    struct ble_gatts_svc_entry *svc_entry;
    struct ble_att_svr_entry *att_chr;
    struct ble_att_svr_entry *cur;
    uint16_t uuid16;
    int rc;

    rc = ble_gatts_find_svc_chr_attr(svc_uuid, chr_uuid, &svc_entry,
                                     &att_chr);
    if (rc != 0) {
        return rc;
    }

    cur = STAILQ_NEXT(att_chr, ha_next);
    while (1) {
        if (cur == NULL) {
            /* Reached end of attribute list without a match. */
            return BLE_HS_ENOENT;
        }

        if (cur->ha_handle_id > svc_entry->end_group_handle) {
            /* Reached end of service without a match. */
            return BLE_HS_ENOENT;
        }

        uuid16 = ble_uuid_u16(cur->ha_uuid);
        if (uuid16 == BLE_ATT_UUID_CHARACTERISTIC) {
            /* Reached end of characteristic without a match. */
            return BLE_HS_ENOENT;
        }

        if (ble_uuid_cmp(cur->ha_uuid, dsc_uuid) == 0) {
            if (out_handle != NULL) {
                *out_handle = cur->ha_handle_id;
                return 0;
            }
        }
        cur = STAILQ_NEXT(cur, ha_next);
    }
}

#if MYNEWT_VAL(BLE_DYNAMIC_SERVICE)
static void ble_gatts_add_clt_cfg(struct ble_gatts_clt_cfg_list *clt_cfgs, uint16_t chr_val_handle, uint16_t allowed_flags, uint8_t flags) {
    struct ble_gatts_clt_cfg *cfg;
    cfg = ble_gatts_clt_cfg_alloc();
    BLE_HS_DBG_ASSERT_EVAL(cfg != NULL);

    cfg->chr_val_handle = chr_val_handle;
    cfg->allowed = allowed_flags;
    cfg->flags = flags;
    STAILQ_INSERT_TAIL(clt_cfgs, cfg, next);
}

static void ble_gatts_remove_clt_cfg(struct ble_gatts_clt_cfg_list *clt_cfgs, uint16_t chr_val_handle) {
    struct ble_gatts_clt_cfg *cfg;

    STAILQ_FOREACH(cfg, clt_cfgs, next) {
        if (cfg->chr_val_handle == chr_val_handle) {
            break;
        }
    }
    if (cfg != NULL) {
        STAILQ_REMOVE(clt_cfgs, cfg, ble_gatts_clt_cfg, next);
        ble_gatts_clt_cfg_free(cfg);
    }
}

#if MYNEWT_VAL(BLE_GATT_CACHING)
static int
ble_gatts_conn_unaware(struct ble_hs_conn *conn, void *arg) {
    conn->bhc_gatt_svr.aware_state = false;
    return 0;
}
#endif

/* takes two arguments
arg[0] : added/removed
arg[1] : affected chr_val_handle
arg[2] : allowed_flags
*/
static int ble_gatts_update_conn_clt_cfg(struct ble_hs_conn *conn, void *arg) {
    uint16_t action = ((uint16_t *) arg)[0];
    uint16_t chr_val_handle = ((uint16_t *) arg)[1];
    uint16_t allowed_flags;
    switch(action) {
    case 1:
        /* added */
        allowed_flags = ((uint16_t *) arg)[2];
        ble_gatts_add_clt_cfg(&conn->bhc_gatt_svr.clt_cfgs, chr_val_handle,
                                allowed_flags, 0);
        (conn->bhc_gatt_svr.num_clt_cfgs)++;
        return 0;
    case 2:
        /* removed */
        ble_gatts_remove_clt_cfg(&conn->bhc_gatt_svr.clt_cfgs,
                                chr_val_handle);
        (conn->bhc_gatt_svr.num_clt_cfgs)--;
        return 0;
    default:
        BLE_HS_DBG_ASSERT(0);
    }
    return 0;
}
static struct ble_gatts_clt_cfg * ble_gatts_get_last_cfg(struct ble_gatts_clt_cfg_list *ble_gatts_clt_cfgs)
{
    struct ble_gatts_clt_cfg *cfg, *prev;
    prev = NULL;
    STAILQ_FOREACH(cfg, ble_gatts_clt_cfgs, next) {
        prev = cfg;
    }
    return prev;
}
int ble_gatts_add_dynamic_svcs(const struct ble_gatt_svc_def *svcs) {
    void *p;
    int i;
    int rc = 0;
    struct ble_att_svr_entry *ha;
    struct ble_gatt_chr_def *chr;
    struct ble_gatts_svc_entry *entry;
    ble_uuid16_t uuid = BLE_UUID16_INIT(BLE_ATT_UUID_CHARACTERISTIC);
    uint16_t allowed_flags;
    struct ble_gatts_clt_cfg *cfg;
    uint16_t arg[3];
    uint16_t start_handle, end_handle;

    p = nimble_platform_mem_malloc(sizeof *ble_gatts_svc_defs);
    if (p == NULL) {
        rc = BLE_HS_ENOMEM;
        goto done;
    }
    ble_hs_lock();

    ble_gatts_svc_defs = p;
    ble_gatts_svc_defs[0] = svcs;
    rc = ble_gatts_register_svcs(ble_gatts_svc_defs[0],
                                 ble_hs_cfg.gatts_register_cb,
                                 ble_hs_cfg.gatts_register_arg);
    if (rc != 0) {
        goto done;
#if BLE_HS_DEBUG
        BLE_HS_DBG_ASSERT(0); /* memory leak expected */
#endif
    }
    ble_gatts_free_svc_defs();
     /* Fill the cache. */
    cfg = ble_gatts_get_last_cfg(&ble_gatts_clt_cfgs);
    ha = ble_att_svr_find_by_handle(cfg->chr_val_handle - 1);
    while ((ha = ble_att_svr_find_by_uuid(ha, &uuid.u, 0xffff)) != NULL) {
        chr = ha->ha_cb_arg;
        allowed_flags = ble_gatts_chr_clt_cfg_allowed(chr);
        if (allowed_flags != 0) {
            ble_gatts_add_clt_cfg(&ble_gatts_clt_cfgs, ha->ha_handle_id + 1, allowed_flags, 0);
            /* update connections */
            arg[0] = CONN_CLT_CFG_ADD;
            arg[1] = ha->ha_handle_id + 1;
            arg[2] = allowed_flags;
            ble_hs_conn_foreach(ble_gatts_update_conn_clt_cfg, arg);
        }
    }
    i = 0;
    entry = ble_gatts_find_svc_entry(&svcs[i]);
    start_handle = entry->handle;
    while(svcs[i].type != BLE_GATT_SVC_TYPE_END) {
        i++;
    }
    entry = ble_gatts_find_svc_entry(&svcs[i - 1]);
    end_handle = entry->end_group_handle;
#if MYNEWT_VAL(BLE_GATT_CACHING)
    /* make all bonded connections unaware */
    for(i = 0; i < MYNEWT_VAL(BLE_STORE_MAX_BONDS); i++) {
        ble_gatts_conn_aware_states[i].aware = false;
    }
    ble_hs_conn_foreach(ble_gatts_conn_unaware, NULL);
#endif

    /* send service change indication */
    ble_svc_gatt_changed(start_handle, end_handle);
done:
    ble_hs_unlock();
    return rc;
}

static int
ble_gatts_deregister_svc(const ble_uuid_t *uuid) {
    int rc;

    struct ble_gatts_svc_entry *entry;
    entry = ble_gatts_find_svc_entry_by_uuid(uuid);
    if (entry == NULL) {
        /* no such service */
        return BLE_HS_ENOENT;
    }
    rc = 0;
    /* if the service is not yet registered, no need to deregister */
    if (entry->handle != 0) {
        rc = ble_att_svr_deregister(entry->handle, entry->end_group_handle);
    }
    return rc;
}

static int
ble_gatts_remove_svc_entry(const ble_uuid_t *uuid)
{
    struct ble_gatts_svc_entry *entry;

    STAILQ_FOREACH(entry, &ble_gatts_svc_entries, next) {
        if (ble_uuid_cmp(uuid, entry->svc->uuid) == 0) {
            break;
        }
    }
    if (entry == NULL) {
        return BLE_HS_ENOENT;
    }
    STAILQ_REMOVE(&ble_gatts_svc_entries, entry, ble_gatts_svc_entry, next);
    ble_gatts_svc_entry_free(entry);
    return 0;
}

int ble_gatts_delete_svc(const ble_uuid_t *uuid) {
    int rc;
    struct ble_gatts_svc_entry * entry;
    int chr_val_handle;
    struct ble_gatt_chr_def *chr;
    uint16_t allowed_flags;
    uint16_t arg[2];
    ble_uuid16_t uuid_chr = BLE_UUID16_INIT(BLE_ATT_UUID_CHARACTERISTIC);
    struct ble_att_svr_entry *ha;
    uint16_t start_handle, end_handle;
#if MYNEWT_VAL(BLE_GATT_CACHING)
    int i;
#endif

    /* Update the cache. and connections*/
    ble_hs_lock();
    entry = ble_gatts_find_svc_entry_by_uuid(uuid);
    if (entry == NULL) {
        rc = BLE_HS_ENOENT;
        goto done;
    }
    ha = ble_att_svr_find_by_handle(entry->handle);
    if (ha == NULL) {
        rc = BLE_HS_ENOENT;
        goto done;
    }
    while ((ha = ble_att_svr_find_by_uuid(ha, &uuid_chr.u, entry->end_group_handle)) != NULL) {
        chr = ha->ha_cb_arg;
        allowed_flags = ble_gatts_chr_clt_cfg_allowed(chr);
        if (allowed_flags != 0) {
            chr_val_handle = ha->ha_handle_id + 1;
            ble_gatts_remove_clt_cfg(&ble_gatts_clt_cfgs, chr_val_handle);

            /* update connections */
            arg[0] = CONN_CLT_CFG_REMOVE;
            arg[1] = chr_val_handle;
            ble_hs_conn_foreach(ble_gatts_update_conn_clt_cfg, arg);
        }
    }
    /* keep the start handle and end handle before deleting the service */
    entry = ble_gatts_find_svc_entry_by_uuid(uuid);
    start_handle = entry->handle;
    end_handle = entry->end_group_handle;
    /* deregister service now */
    rc = ble_gatts_deregister_svc(uuid);

done:
    if (rc == 0) {
        rc = ble_gatts_remove_svc_entry(uuid);
#if MYNEWT_VAL(BLE_GATT_CACHING)
        /* make all bonded connections them unaware */
        for(i = 0; i < MYNEWT_VAL(BLE_STORE_MAX_BONDS); i++) {
            ble_gatts_conn_aware_states[i].aware = false;
        }
        ble_hs_conn_foreach(ble_gatts_conn_unaware, NULL);
#endif

        /* send service change indication */
        ble_svc_gatt_changed(start_handle, end_handle);
    }
    ble_hs_unlock();
    return rc;
}
#endif

int
ble_gatts_add_svcs(const struct ble_gatt_svc_def *svcs)
{
    void *p;
    int rc;

    ble_hs_lock();
    if (!ble_gatts_mutable()) {
        rc = BLE_HS_EBUSY;
        goto done;
    }

    p = realloc(ble_gatts_svc_defs,
                (ble_gatts_num_svc_defs + 1) * sizeof *ble_gatts_svc_defs);
    if (p == NULL) {
        rc = BLE_HS_ENOMEM;
        goto done;
    }

    ble_gatts_svc_defs = p;
    ble_gatts_svc_defs[ble_gatts_num_svc_defs] = svcs;
    ble_gatts_num_svc_defs++;

    rc = 0;

done:
    ble_hs_unlock();
    return rc;
}

int
ble_gatts_svc_set_visibility(uint16_t handle, int visible)
{
#if MYNEWT_VAL(BLE_DYNAMIC_SERVICE)
    struct ble_gatts_svc_entry *entry;

    STAILQ_FOREACH(entry, &ble_gatts_svc_entries, next) {
#else
    int i;

    for (i = 0; i < ble_gatts_num_svc_entries; i++) {
        struct ble_gatts_svc_entry *entry = &ble_gatts_svc_entries[i];
#endif

        if (entry->handle == handle) {
            if (visible) {
                ble_att_svr_restore_range(entry->handle, entry->end_group_handle);
            } else {
                ble_att_svr_hide_range(entry->handle, entry->end_group_handle);
            }
            return 0;
        }
    }

    return BLE_HS_ENOENT;
}

/**
 * Accumulates counts of each resource type required by the specified service
 * definition array.  This function is generally used to calculate some host
 * configuration values prior to initialization.  This function adds the counts
 * to the appropriate fields in the supplied ble_gatt_resources object without
 * clearing them first, so it can be called repeatedly with different inputs to
 * calculate totals.  Be sure to zero the resource struct prior to the first
 * call to this function.
 *
 * @param svcs                  The service array containing the resource
 *                                  definitions to be counted.
 * @param res                   The resource counts are accumulated in this
 *                                  struct.
 *
 * @return                      0 on success;
 *                              BLE_HS_EINVAL if the svcs array contains an
 *                                  invalid resource definition.
 */
static int
ble_gatts_count_resources(const struct ble_gatt_svc_def *svcs,
                          struct ble_gatt_resources *res)
{
    const struct ble_gatt_svc_def *svc;
    const struct ble_gatt_chr_def *chr;
    int s;
    int i;
    int c;
    int d;
    int pf;

    for (s = 0; svcs[s].type != BLE_GATT_SVC_TYPE_END; s++) {
        svc = svcs + s;

        if (!ble_gatts_svc_is_sane(svc)) {
            BLE_HS_DBG_ASSERT(0);
            return BLE_HS_EINVAL;
        }

        /* Each service requires:
         *     o 1 service
         *     o 1 attribute
         */
        res->svcs++;
        res->attrs++;

        if (svc->includes != NULL) {
            for (i = 0; svc->includes[i] != NULL; i++) {
                /* Each include requires:
                 *     o 1 include
                 *     o 1 attribute
                 */
                res->incs++;
                res->attrs++;
            }
        }

        if (svc->characteristics != NULL) {
            for (c = 0; svc->characteristics[c].uuid != NULL; c++) {
                chr = svc->characteristics + c;

                if (!ble_gatts_chr_is_sane(chr)) {
                    BLE_HS_DBG_ASSERT(0);
                    return BLE_HS_EINVAL;
                }

                /* Each characteristic requires:
                 *     o 1 characteristic
                 *     o 2 attributes
                 */
                res->chrs++;
                res->attrs += 2;

                /* If the characteristic permits notifications or indications,
                 * it has a CCCD.
                 */
                if (chr->flags & BLE_GATT_CHR_F_NOTIFY ||
                    chr->flags & BLE_GATT_CHR_F_INDICATE) {

                    /* Each CCCD requires:
                     *     o 1 descriptor
                     *     o 1 CCCD
                     *     o 1 attribute
                     */
                    res->dscs++;
                    res->cccds++;
                    res->attrs++;
                }

                if (chr->descriptors != NULL) {
                    for (d = 0; chr->descriptors[d].uuid != NULL; d++) {
                        if (!ble_gatts_dsc_is_sane(chr->descriptors + d)) {
                            BLE_HS_DBG_ASSERT(0);
                            return BLE_HS_EINVAL;
                        }

                        /* Each descriptor requires:
                         *     o 1 descriptor
                         *     o 1 attribute
                         */
                        res->dscs++;
                        res->attrs++;
                    }
                }

                if (chr->cpfd != NULL) {
                    for (pf = 0; chr->cpfd[pf].format != 0; pf++) {
                        if (!ble_gatts_cpfd_is_sane(chr->cpfd + pf)) {
                            BLE_HS_DBG_ASSERT(0);
                            return BLE_HS_EINVAL;
                        }

                        /** Each CPFD requires:
                         *      o 1 descriptor
                         *      o 1 CPFD
                         *      o 1 attribute
                         */
                        res->dscs++;
                        res->cpfds++;
                        res->attrs++;
                    }

                    /** If more than one CPFD is present for a characteristic, one CAFD is required. */
                    if (pf > 1) {
                        res->cafds++;
                        res->dscs++;
                        res->attrs++;
                    }
                }
            }
        }
    }

    return 0;
}
int
ble_gatts_count_cfg(const struct ble_gatt_svc_def *defs)
{
    struct ble_gatt_resources res = { 0 };
    int rc;

    rc = ble_gatts_count_resources(defs, &res);
    if (rc != 0) {
        return rc;
    }

    ble_hs_max_services += res.svcs;
    ble_hs_max_attrs += res.attrs;

    /* Reserve an extra CCCD for the cache. */
    ble_hs_max_client_configs +=
        res.cccds * (MYNEWT_VAL(BLE_MAX_CONNECTIONS) + 1);

    return 0;
}

int
ble_gatts_get_cfgable_chrs(void)
{
    return ble_gatts_num_cfgable_chrs;
}

void
ble_gatts_lcl_svc_foreach(ble_gatt_svc_foreach_fn cb, void *arg)
{
#if MYNEWT_VAL(BLE_DYNAMIC_SERVICE)
    struct ble_gatts_svc_entry *entry;

    STAILQ_FOREACH(entry, &ble_gatts_svc_entries, next) {
        cb(entry -> svc,
           entry -> handle,
           entry -> end_group_handle, arg);
    }
#else
    int i;

    for (i = 0; i < ble_gatts_num_svc_entries; i++) {
        cb(ble_gatts_svc_entries[i].svc,
           ble_gatts_svc_entries[i].handle,
           ble_gatts_svc_entries[i].end_group_handle, arg);
    }
#endif
}

int
ble_gatts_reset(void)
{
    int rc;
#if MYNEWT_VAL(BLE_DYNAMIC_SERVICE)
    struct ble_gatts_svc_entry *entry;
#endif

    ble_hs_lock();

    if (!ble_gatts_mutable()) {
        rc = BLE_HS_EBUSY;
    } else {
        /* Unregister all ATT attributes. */
        ble_att_svr_reset();
        ble_gatts_num_cfgable_chrs = 0;
        rc = 0;
#if MYNEWT_VAL(BLE_DYNAMIC_SERVICE)
        /* free svc entries */
        while ((entry = STAILQ_FIRST(&ble_gatts_svc_entries)) != NULL) {
            STAILQ_REMOVE_HEAD(&ble_gatts_svc_entries, next);
            ble_gatts_svc_entry_free(entry);
        }

#endif

        /* Note: gatts memory gets freed on next call to ble_gatts_start(). */
    }

#if MYNEWT_VAL(BLE_SVC_HID_SERVICE)
    ble_svc_hid_reset();
#endif
    ble_hs_unlock();

    return rc;
}

int
ble_gatts_init(void)
{
    int rc;

    ble_gatts_num_cfgable_chrs = 0;
#if MYNEWT_VAL(BLE_DYNAMIC_SERVICE)
    STAILQ_INIT(&ble_gatts_clt_cfgs);
#else
    ble_gatts_clt_cfgs = NULL;
#endif

    rc = stats_init_and_reg(
        STATS_HDR(ble_gatts_stats), STATS_SIZE_INIT_PARMS(ble_gatts_stats,
        STATS_SIZE_32), STATS_NAME_INIT_PARMS(ble_gatts_stats), "ble_gatts");
    if (rc != 0) {
        return BLE_HS_EOS;
    }

#if MYNEWT_VAL(BLE_DYNAMIC_SERVICE)
    STAILQ_INIT(&ble_gatts_svc_entries);
#endif
#if MYNEWT_VAL(BLE_GATT_CACHING)
    memset(ble_gatts_conn_aware_states, 0, sizeof ble_gatts_conn_aware_states);
    last_conn_aware_state_index = 0;
#endif

    return 0;
}
