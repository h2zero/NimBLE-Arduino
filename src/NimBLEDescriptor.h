/*
 * NimBLEDescriptor.h
 *
 *  Created: on March 10, 2020
 *      Author H2zero
 *
 * Originally:
 *
 * BLEDescriptor.h
 *
 *  Created on: Jun 22, 2017
 *      Author: kolban
 */

#ifndef NIMBLE_CPP_DESCRIPTOR_H_
#define NIMBLE_CPP_DESCRIPTOR_H_

#include "nimconfig.h"
#if defined(CONFIG_BT_ENABLED) && defined(CONFIG_BT_NIMBLE_ROLE_PERIPHERAL)

class NimBLEDescriptor;
class NimBLEDescriptorCallbacks;

# include "NimBLELocalValueAttribute.h"
# include "NimBLECharacteristic.h"
# include "NimBLEUUID.h"
# include "NimBLEAttValue.h"
# include "NimBLEConnInfo.h"

# include <string>

/**
 * @brief A model of a BLE descriptor.
 */
class NimBLEDescriptor : public NimBLELocalValueAttribute {
  public:
    NimBLEDescriptor(const char* uuid, uint16_t properties, uint16_t maxLen, NimBLECharacteristic* pCharacteristic = nullptr);

    NimBLEDescriptor(const NimBLEUUID&     uuid,
                     uint16_t              properties,
                     uint16_t              maxLen,
                     NimBLECharacteristic* pCharacteristic = nullptr);
    ~NimBLEDescriptor() = default;

    std::string           toString() const;
    void                  setCallbacks(NimBLEDescriptorCallbacks* pCallbacks);
    NimBLECharacteristic* getCharacteristic() const;

  private:
    friend class NimBLECharacteristic;
    friend class NimBLEService;

    void setCharacteristic(NimBLECharacteristic* pChar);
    void readEvent(NimBLEConnInfo& connInfo) override;
    void writeEvent(const uint8_t* val, uint16_t len, NimBLEConnInfo& connInfo) override;

    NimBLEDescriptorCallbacks* m_pCallbacks{nullptr};
    NimBLECharacteristic*      m_pCharacteristic{nullptr};
}; // NimBLEDescriptor

/**
 * @brief Callbacks that can be associated with a %BLE descriptors to inform of events.
 *
 * When a server application creates a %BLE descriptor, we may wish to be informed when there is either
 * a read or write request to the descriptors value.  An application can register a
 * sub-classed instance of this class and will be notified when such an event happens.
 */
class NimBLEDescriptorCallbacks {
  public:
    virtual ~NimBLEDescriptorCallbacks() = default;
    virtual void onRead(NimBLEDescriptor* pDescriptor, NimBLEConnInfo& connInfo);
    virtual void onWrite(NimBLEDescriptor* pDescriptor, NimBLEConnInfo& connInfo);
};

# include "NimBLE2904.h"

#endif /* CONFIG_BT_ENABLED && CONFIG_BT_NIMBLE_ROLE_PERIPHERAL */
#endif /* NIMBLE_CPP_DESCRIPTOR_H_ */
