/*
 * Copyright 2020-2025 Ryan Powell <ryan@nable-embedded.io> and
 * esp-nimble-cpp, NimBLE-Arduino contributors.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "NimBLE2905.h"
#include "NimBLELog.h"

#include "NimBLECharacteristic.h"
#if CONFIG_BT_NIMBLE_ENABLED && MYNEWT_VAL(BLE_ROLE_PERIPHERAL)

// Define default if not already defined
#ifndef NIMBLE_MAX_AGGREGATE_FORMAT_DESCRIPTORS
    #define NIMBLE_MAX_AGGREGATE_FORMAT_DESCRIPTORS 5  // Default value
#else
    // Ensure the value is within a valid range
    #if NIMBLE_MAX_AGGREGATE_FORMAT_DESCRIPTORS < 1 || NIMBLE_MAX_AGGREGATE_FORMAT_DESCRIPTORS > 255
        #error "NIMBLE_MAX_AGGREGATE_FORMAT_DESCRIPTORS must be between 1 and 128"
    #endif
#endif

static const char* LOG_TAG = "NimBLE2905";

NimBLE2905::NimBLE2905(NimBLECharacteristic* pChr)
    : NimBLEDescriptor(NimBLEUUID(static_cast<uint16_t>(0x2905)), BLE_GATT_CHR_F_READ, NIMBLE_MAX_AGGREGATE_FORMAT_DESCRIPTORS * sizeof(uint16_t), pChr) {
} // NimBLE2905

void NimBLE2905::initValue() {
    const size_t count = m_vAggregatedDescriptors.size();
    uint16_t aggregatedHandles[count];

    for (size_t i = 0; i < count; ++i) {
        auto* desc = m_vAggregatedDescriptors[i];
        uint16_t handle = desc->getHandle();

        if (handle == 0) {
            NIMBLE_LOGE(LOG_TAG, "Failed to initialize value: presentation format descriptor handle is not initialized");
            return;
        }

        aggregatedHandles[i] = desc->getHandle();
    } // initValue

    setValue(reinterpret_cast<const uint8_t*>(aggregatedHandles), sizeof(aggregatedHandles));

    // Presentation formats no longer needed, let's free some memory
    m_vAggregatedDescriptors.clear();
    m_vAggregatedDescriptors.shrink_to_fit();
} // initValue


/**
 * @brief Add presentation format descriptor.
 * @param [in] presentationFormat The 2904 descriptor to aggregate.
 */
void NimBLE2905::add2904Descriptor(const NimBLE2904* presentationFormat) {
    if (presentationFormat == nullptr) {
        NIMBLE_LOGE(LOG_TAG, "Failed to add presentation format descriptor: nullptr");
        return;
    }

    if (m_vAggregatedDescriptors.size() < NIMBLE_MAX_AGGREGATE_FORMAT_DESCRIPTORS) {
        m_vAggregatedDescriptors.push_back(presentationFormat);
    } else {
        NIMBLE_LOGE(LOG_TAG, "Failed to add presentation format descriptor: maximum capacity reached");
    }
} // add2904Descriptor


#endif // CONFIG_BT_NIMBLE_ENABLED && MYNEWT_VAL(BLE_ROLE_PERIPHERAL)
