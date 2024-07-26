/*
 * NimBLERemoteDescriptor.cpp
 *
 *  Created: on Jan 27 2020
 *      Author H2zero
 *
 * Originally:
 *
 * BLERemoteDescriptor.cpp
 *
 *  Created on: Jul 8, 2017
 *      Author: kolban
 */

#include "nimconfig.h"
#if defined(CONFIG_BT_ENABLED) && defined(CONFIG_BT_NIMBLE_ROLE_CENTRAL)

# include "NimBLERemoteDescriptor.h"
# include "NimBLERemoteCharacteristic.h"

/**
 * @brief Remote descriptor constructor.
 * @param [in] pRemoteCharacteristic A pointer to the Characteristic that this belongs to.
 * @param [in] dsc A pointer to the struct that contains the descriptor information.
 */
NimBLERemoteDescriptor::NimBLERemoteDescriptor(const NimBLERemoteCharacteristic* pRemoteCharacteristic,
                                               const ble_gatt_dsc*               dsc)
    : NimBLERemoteValueAttribute{dsc->uuid, dsc->handle}, m_pRemoteCharacteristic{pRemoteCharacteristic} {} // NimBLERemoteDescriptor

/**
 * @brief Get the characteristic that owns this descriptor.
 * @return The characteristic that owns this descriptor.
 */
NimBLERemoteCharacteristic* NimBLERemoteDescriptor::getRemoteCharacteristic() const {
    return const_cast<NimBLERemoteCharacteristic*>(m_pRemoteCharacteristic);
} // getRemoteCharacteristic

/**
 * @brief Return a string representation of this Remote Descriptor.
 * @return A string representation of this Remote Descriptor.
 */
std::string NimBLERemoteDescriptor::toString() const {
    std::string res = "Descriptor: uuid: " + getUUID().toString();
    char        val[6];
    res += ", handle: ";
    snprintf(val, sizeof(val), "%d", getHandle());
    res += val;

    return res;
} // toString

NimBLEClient* NimBLERemoteDescriptor::getClient() const {
    return m_pRemoteCharacteristic->getClient();
}

#endif /* CONFIG_BT_ENABLED && CONFIG_BT_NIMBLE_ROLE_CENTRAL */
