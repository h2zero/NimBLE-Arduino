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

#ifndef NIMBLE_CPP_2905_H_
#define NIMBLE_CPP_2905_H_

#include "syscfg/syscfg.h"
#if CONFIG_BT_NIMBLE_ENABLED && MYNEWT_VAL(BLE_ROLE_PERIPHERAL)

# include "NimBLEDescriptor.h"

/**
 * @brief Characteristic Aggregate Format descriptor (UUID: 0x2905).
 *
 * @details Contains an ordered list of handles referencing 0x2904 Presentation Format
 * descriptors that define the parent characteristic’s value.
 */
class NimBLE2905 : public NimBLEDescriptor {
  public:
    NimBLE2905(NimBLECharacteristic* pChr = nullptr);

    void add2904Descriptor(const NimBLE2904* presentationFormat);
  private:

    /**
     * @brief Descriptor for the Characteristic Aggregate Format (UUID: 0x2905).
     *
     * @details Contains an ordered list of handles referencing the 0x2904
     * Presentation Format descriptors that define the parent characteristic's value.
     *
     * @see NimBLEServer::start()
     */
    void initValue();

    friend class NimBLECharacteristic;
    friend class NimBLEServer;

    std::vector<const NimBLE2904*> m_vAggregatedDescriptors;
}; // NimBLE2904

#endif // CONFIG_BT_NIMBLE_ENABLED && MYNEWT_VAL(BLE_ROLE_PERIPHERAL)
#endif // NIMBLE_CPP_2904_H_
