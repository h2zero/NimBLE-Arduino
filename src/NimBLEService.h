/*
 * NimBLEService.h
 *
 *  Created: on March 2, 2020
 *      Author H2zero
 *
 * Originally:
 *
 * BLEService.h
 *
 *  Created on: Mar 25, 2017
 *      Author: kolban
 */

#ifndef NIMBLE_CPP_SERVICE_H_
#define NIMBLE_CPP_SERVICE_H_

#include "nimconfig.h"
#if defined(CONFIG_BT_ENABLED) && defined(CONFIG_BT_NIMBLE_ROLE_PERIPHERAL)

class NimBLEService;

# include "NimBLEAttribute.h"
# include "NimBLEServer.h"
# include "NimBLECharacteristic.h"

/**
 * @brief The model of a BLE service.
 *
 */
class NimBLEService : public NimBLELocalAttribute {
  public:
    NimBLEService(const char* uuid);
    NimBLEService(const NimBLEUUID& uuid);
    ~NimBLEService();

    NimBLEServer*         getServer() const;
    std::string           toString() const;
    void                  dump() const;
    bool                  isStarted() const;
    bool                  start();
    NimBLECharacteristic* createCharacteristic(const char* uuid,
                                               uint32_t    properties = NIMBLE_PROPERTY::READ | NIMBLE_PROPERTY::WRITE,
                                               uint16_t    max_len    = BLE_ATT_ATTR_MAX_LEN);

    NimBLECharacteristic* createCharacteristic(const NimBLEUUID& uuid,
                                               uint32_t properties = NIMBLE_PROPERTY::READ | NIMBLE_PROPERTY::WRITE,
                                               uint16_t max_len    = BLE_ATT_ATTR_MAX_LEN);
    void                  addCharacteristic(NimBLECharacteristic* pCharacteristic);
    void                  removeCharacteristic(NimBLECharacteristic* pCharacteristic, bool deleteChr = false);
    NimBLECharacteristic* getCharacteristic(const char* uuid, uint16_t instanceId = 0) const;
    NimBLECharacteristic* getCharacteristic(const NimBLEUUID& uuid, uint16_t instanceId = 0) const;
    NimBLECharacteristic* getCharacteristicByHandle(uint16_t handle) const;

    const std::vector<NimBLECharacteristic*>& getCharacteristics() const;
    std::vector<NimBLECharacteristic*>        getCharacteristics(const char* uuid) const;
    std::vector<NimBLECharacteristic*>        getCharacteristics(const NimBLEUUID& uuid) const;

  private:
    friend class NimBLEServer;

    std::vector<NimBLECharacteristic*> m_vChars{};
    // Nimble requires an array of services to be sent to the api
    // Since we are adding 1 at a time we create an array of 2 and set the type
    // of the second service to 0 to indicate the end of the array.
    ble_gatt_svc_def                   m_pSvcDef[2]{};
}; // NimBLEService

#endif /* CONFIG_BT_ENABLED && CONFIG_BT_NIMBLE_ROLE_PERIPHERAL */
#endif /* NIMBLE_CPP_SERVICE_H_ */
