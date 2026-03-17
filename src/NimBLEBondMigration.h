#pragma once

#if !defined(ESP_PLATFORM)
# error "NimBLEBondMigration.h currently requires ESP_PLATFORM (NVS backend)."
#endif

#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "esp_err.h"
#include "esp_log.h"
#include "nvs.h"
#include "syscfg/syscfg.h"

namespace NimBLEBondMigration {

namespace detail {

static constexpr const char* kLogTag        = "NimBLEBondMigration";
static constexpr const char* kBondNamespace = "nimble_bond";
static constexpr const char* kOurSecPrefix  = "our_sec";
static constexpr const char* kPeerSecPrefix = "peer_sec";
static constexpr size_t      kNvsKeyMaxLen  = 16;

typedef struct {
    uint8_t type;
    uint8_t val[6];
} ble_addr_t;

struct BleStoreValueSecV1 {
    ble_addr_t peer_addr;

    uint8_t key_size;
    uint16_t ediv;
    uint64_t rand_num;
    uint8_t ltk[16];
    uint8_t ltk_present : 1;

    uint8_t irk[16];
    uint8_t irk_present : 1;

    uint8_t csrk[16];
    uint8_t csrk_present : 1;

    unsigned authenticated : 1;
    uint8_t sc : 1;
};

struct BleStoreValueSecCurrent {
    ble_addr_t peer_addr;
    uint16_t   bond_count;

    uint8_t key_size;
    uint16_t ediv;
    uint64_t rand_num;
    uint8_t ltk[16];
    uint8_t ltk_present : 1;

    uint8_t irk[16];
    uint8_t irk_present : 1;

    uint8_t csrk[16];
    uint8_t csrk_present : 1;
    uint32_t sign_counter;

    unsigned authenticated : 1;
    uint8_t sc : 1;
};

struct MigrationStats {
    uint16_t scanned;
    uint16_t converted;
    uint16_t alreadyTarget;
    uint16_t skippedUnknownSize;
};

inline bool makeBondKey(char* out, size_t outLen, const char* prefix, uint16_t index) {
    const int written = snprintf(out, outLen, "%s_%u", prefix, index);
    return written > 0 && static_cast<size_t>(written) < outLen;
}

inline esp_err_t migrateEntryToCurrent(nvs_handle_t nvsHandle,
                                       const char* key,
                                       uint16_t index,
                                       MigrationStats* stats) {
    size_t blobSize = 0;
    esp_err_t err = nvs_get_blob(nvsHandle, key, nullptr, &blobSize);
    if (err == ESP_ERR_NVS_NOT_FOUND) {
        return ESP_OK;
    }
    if (err != ESP_OK) {
        return err;
    }

    stats->scanned++;

    if (blobSize == sizeof(BleStoreValueSecCurrent)) {
        stats->alreadyTarget++;
        return ESP_OK;
    }

    if (blobSize != sizeof(BleStoreValueSecV1)) {
        stats->skippedUnknownSize++;
        ESP_LOGW(kLogTag,
                 "Skipping key=%s due to unexpected size=%u",
                 key,
                 static_cast<unsigned>(blobSize));
        return ESP_OK;
    }

    BleStoreValueSecV1 oldValue{};
    size_t readSize = sizeof(oldValue);
    err = nvs_get_blob(nvsHandle, key, &oldValue, &readSize);
    if (err != ESP_OK) {
        return err;
    }

    BleStoreValueSecCurrent newValue{};
    newValue.peer_addr = oldValue.peer_addr;
    newValue.bond_count = index;
    newValue.key_size = oldValue.key_size;
    newValue.ediv = oldValue.ediv;
    newValue.rand_num = oldValue.rand_num;
    memcpy(newValue.ltk, oldValue.ltk, sizeof(newValue.ltk));
    newValue.ltk_present = oldValue.ltk_present;
    memcpy(newValue.irk, oldValue.irk, sizeof(newValue.irk));
    newValue.irk_present = oldValue.irk_present;
    memcpy(newValue.csrk, oldValue.csrk, sizeof(newValue.csrk));
    newValue.csrk_present = oldValue.csrk_present;
    newValue.sign_counter = 0;
    newValue.authenticated = oldValue.authenticated;
    newValue.sc = oldValue.sc;

    err = nvs_set_blob(nvsHandle, key, &newValue, sizeof(newValue));
    if (err != ESP_OK) {
        return err;
    }

    stats->converted++;
    return ESP_OK;
}

inline esp_err_t migrateEntryToV1(nvs_handle_t nvsHandle,
                                  const char* key,
                                  MigrationStats* stats) {
    size_t blobSize = 0;
    esp_err_t err = nvs_get_blob(nvsHandle, key, nullptr, &blobSize);
    if (err == ESP_ERR_NVS_NOT_FOUND) {
        return ESP_OK;
    }
    if (err != ESP_OK) {
        return err;
    }

    stats->scanned++;

    if (blobSize == sizeof(BleStoreValueSecV1)) {
        stats->alreadyTarget++;
        return ESP_OK;
    }

    if (blobSize != sizeof(BleStoreValueSecCurrent)) {
        stats->skippedUnknownSize++;
        ESP_LOGW(kLogTag,
                 "Skipping key=%s due to unexpected size=%u",
                 key,
                 static_cast<unsigned>(blobSize));
        return ESP_OK;
    }

    BleStoreValueSecCurrent curValue{};
    size_t readSize = sizeof(curValue);
    err = nvs_get_blob(nvsHandle, key, &curValue, &readSize);
    if (err != ESP_OK) {
        return err;
    }

    BleStoreValueSecV1 oldValue{};
    oldValue.peer_addr = curValue.peer_addr;
    oldValue.key_size = curValue.key_size;
    oldValue.ediv = curValue.ediv;
    oldValue.rand_num = curValue.rand_num;
    memcpy(oldValue.ltk, curValue.ltk, sizeof(oldValue.ltk));
    oldValue.ltk_present = curValue.ltk_present;
    memcpy(oldValue.irk, curValue.irk, sizeof(oldValue.irk));
    oldValue.irk_present = curValue.irk_present;
    memcpy(oldValue.csrk, curValue.csrk, sizeof(oldValue.csrk));
    oldValue.csrk_present = curValue.csrk_present;
    oldValue.authenticated = curValue.authenticated;
    oldValue.sc = curValue.sc;

    err = nvs_set_blob(nvsHandle, key, &oldValue, sizeof(oldValue));
    if (err != ESP_OK) {
        return err;
    }

    stats->converted++;
    return ESP_OK;
}

inline esp_err_t migrateBondStore(bool toCurrent, uint16_t maxEntries) {
    nvs_handle_t nvsHandle;
    esp_err_t err = nvs_open(kBondNamespace, NVS_READWRITE, &nvsHandle);
    if (err != ESP_OK) {
        ESP_LOGE(kLogTag,
                 "Failed to open NVS namespace '%s', err=%d",
                 kBondNamespace,
                 static_cast<int>(err));
        return err;
    }

    MigrationStats stats{};
    char key[kNvsKeyMaxLen]{};

    for (uint16_t i = 1; i <= maxEntries; ++i) {
        if (!makeBondKey(key, sizeof(key), kOurSecPrefix, i)) {
            nvs_close(nvsHandle);
            return ESP_FAIL;
        }

        err = toCurrent ? migrateEntryToCurrent(nvsHandle, key, i, &stats)
                        : migrateEntryToV1(nvsHandle, key, &stats);
        if (err != ESP_OK) {
            nvs_close(nvsHandle);
            return err;
        }

        if (!makeBondKey(key, sizeof(key), kPeerSecPrefix, i)) {
            nvs_close(nvsHandle);
            return ESP_FAIL;
        }

        err = toCurrent ? migrateEntryToCurrent(nvsHandle, key, i, &stats)
                        : migrateEntryToV1(nvsHandle, key, &stats);
        if (err != ESP_OK) {
            nvs_close(nvsHandle);
            return err;
        }
    }

    if (stats.converted > 0) {
        err = nvs_commit(nvsHandle);
        if (err != ESP_OK) {
            nvs_close(nvsHandle);
            return err;
        }
    }

    nvs_close(nvsHandle);

    ESP_LOGI(kLogTag,
             "Bond migration %s: scanned=%u converted=%u already=%u skipped=%u",
             toCurrent ? "to-current" : "to-v1",
             stats.scanned,
             stats.converted,
             stats.alreadyTarget,
             stats.skippedUnknownSize);

    return ESP_OK;
}

} // namespace detail

inline esp_err_t migrateBondStoreToCurrent(uint16_t maxEntries = MYNEWT_VAL(BLE_STORE_MAX_BONDS)) {
    return detail::migrateBondStore(true, maxEntries);
}

inline esp_err_t migrateBondStoreToV1(uint16_t maxEntries = MYNEWT_VAL(BLE_STORE_MAX_BONDS)) {
    return detail::migrateBondStore(false, maxEntries);
}

} // namespace NimBLEBondMigration
