/*
 * NimBLERemoteDescriptor.h
 *
 *  Created: on Jan 27 2020
 *      Author H2zero
 *
 * Originally:
 *
 * BLERemoteDescriptor.h
 *
 *  Created on: Jul 8, 2017
 *      Author: kolban
 */

#ifndef NIMBLE_CPP_REMOTE_DESCRIPTOR_H_
#define NIMBLE_CPP_REMOTE_DESCRIPTOR_H_

#include "nimconfig.h"
#if defined(CONFIG_BT_ENABLED) && defined(CONFIG_BT_NIMBLE_ROLE_CENTRAL)

# include "NimBLERemoteValueAttribute.h"

class NimBLERemoteCharacteristic;
class NimBLEClient;

/**
 * @brief A model of remote BLE descriptor.
 */
class NimBLERemoteDescriptor : public NimBLERemoteValueAttribute {
  public:
    NimBLERemoteCharacteristic* getRemoteCharacteristic() const;
    std::string                 toString(void) const;
    NimBLEClient*               getClient() const override;

  private:
    friend class NimBLERemoteCharacteristic;

    NimBLERemoteDescriptor(const NimBLERemoteCharacteristic* pRemoteCharacteristic, const ble_gatt_dsc* dsc);
    ~NimBLERemoteDescriptor() = default;

    const NimBLERemoteCharacteristic* m_pRemoteCharacteristic;
};

#endif /* CONFIG_BT_ENABLED && CONFIG_BT_NIMBLE_ROLE_CENTRAL */
#endif /* NIMBLE_CPP_REMOTE_DESCRIPTOR_H_ */
