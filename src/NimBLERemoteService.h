/*
 * NimBLERemoteService.h
 *
 *  Created: on Jan 27 2020
 *      Author H2zero
 *
 * Originally:
 *
 * BLERemoteService.h
 *
 *  Created on: Jul 8, 2017
 *      Author: kolban
 */

#ifndef COMPONENTS_NIMBLEREMOTESERVICE_H_
#define COMPONENTS_NIMBLEREMOTESERVICE_H_
#include "sdkconfig.h"
#if defined(CONFIG_BT_ENABLED)

#include "nimconfig.h"
#if defined( CONFIG_BT_NIMBLE_ROLE_CENTRAL)

#include "NimBLEClient.h"
#include "NimBLEUUID.h"
#include "FreeRTOS.h"
#include "NimBLERemoteCharacteristic.h"

#include <vector>

class NimBLEClient;
class NimBLERemoteCharacteristic;


/**
 * @brief A model of a remote %BLE service.
 */
class NimBLERemoteService {
public:
    virtual ~NimBLERemoteService();

    // Public methods
    NimBLERemoteCharacteristic* getCharacteristic(const char* uuid);      // Get the specified characteristic reference.
    NimBLERemoteCharacteristic* getCharacteristic(const NimBLEUUID &uuid);       // Get the specified characteristic reference.
    void                        clear(); // Clear any existing characteristics.
    size_t                      clear(const NimBLEUUID &uuid); // Clear characteristic by UUID
//  BLERemoteCharacteristic* getCharacteristic(uint16_t uuid);      // Get the specified characteristic reference.
    std::vector<NimBLERemoteCharacteristic*>* getCharacteristics();
//  void getCharacteristics(std::map<uint16_t, BLERemoteCharacteristic*>* pCharacteristicMap);

    NimBLEClient*            getClient(void);                                           // Get a reference to the client associated with this service.
    uint16_t                 getHandle();                                               // Get the handle of this service.
    NimBLEUUID               getUUID(void);                                             // Get the UUID of this service.
    std::string              getValue(const NimBLEUUID &characteristicUuid);                      // Get the value of a characteristic.
    bool                     setValue(const NimBLEUUID &characteristicUuid, const std::string &value);   // Set the value of a characteristic.
    std::string              toString(void);

private:
    // Private constructor ... never meant to be created by a user application.
    NimBLERemoteService(NimBLEClient* pClient, const struct ble_gatt_svc *service);

    // Friends
    friend class NimBLEClient;
    friend class NimBLERemoteCharacteristic;

    // Private methods
    bool                retrieveCharacteristics(void);   // Retrieve the characteristics from the BLE Server.
    static int          characteristicDiscCB(uint16_t conn_handle,
                                const struct ble_gatt_error *error,
                                const struct ble_gatt_chr *chr, void *arg);

    uint16_t            getStartHandle();                // Get the start handle for this service.
    uint16_t            getEndHandle();                  // Get the end handle for this service.
    void                releaseSemaphores();

    // Properties

    // We maintain a vector of characteristics owned by this service.
    std::vector<NimBLERemoteCharacteristic*> m_characteristicVector;

    bool                m_haveCharacteristics; // Have we previously obtained the characteristics.
    NimBLEClient*       m_pClient;
    FreeRTOS::Semaphore m_semaphoreGetCharEvt = FreeRTOS::Semaphore("GetCharEvt");
    NimBLEUUID          m_uuid;             // The UUID of this service.
    uint16_t            m_startHandle;      // The starting handle of this service.
    uint16_t            m_endHandle;        // The ending handle of this service.
}; // BLERemoteService

#endif // #if defined( CONFIG_BT_NIMBLE_ROLE_CENTRAL)
#endif /* CONFIG_BT_ENABLED */
#endif /* COMPONENTS_NIMBLEREMOTESERVICE_H_ */
