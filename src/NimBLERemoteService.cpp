/*
 * NimBLERemoteService.cpp
 *
 *  Created: on Jan 27 2020
 *      Author H2zero
 *
 * Originally:
 *
 * BLERemoteService.cpp
 *
 *  Created on: Jul 8, 2017
 *      Author: kolban
 */
#include "sdkconfig.h"
#if defined(CONFIG_BT_ENABLED)

#include "nimconfig.h"
#if defined( CONFIG_BT_NIMBLE_ROLE_CENTRAL)

#include "NimBLERemoteService.h"
#include "NimBLEUtils.h"
#include "NimBLEDevice.h"
#include "NimBLELog.h"

static const char* LOG_TAG = "NimBLERemoteService";

/**
 * @brief Remote Service constructor.
 * @param [in] Reference to the client this belongs to.
 * @param [in] Refernce to the structure with the services' information.
 */
NimBLERemoteService::NimBLERemoteService(NimBLEClient* pClient, const struct ble_gatt_svc* service) {

    NIMBLE_LOGD(LOG_TAG, ">> NimBLERemoteService()");
    m_pClient = pClient;
    switch (service->uuid.u.type) {
        case BLE_UUID_TYPE_16:
            m_uuid = NimBLEUUID(service->uuid.u16.value);
            break;
        case BLE_UUID_TYPE_32:
            m_uuid = NimBLEUUID(service->uuid.u32.value);
            break;
        case BLE_UUID_TYPE_128:
            m_uuid = NimBLEUUID(const_cast<ble_uuid128_t*>(&service->uuid.u128));
            break;
        default:
            m_uuid = nullptr;
            break;
    }
    m_startHandle         = service->start_handle;
    m_endHandle           = service->end_handle;
    m_haveCharacteristics = false;
    m_pSemaphore          = nullptr;

    NIMBLE_LOGD(LOG_TAG, "<< NimBLERemoteService()");
}


/**
 * @brief When deleting the service make sure we delete all characteristics and descriptors.
 * Also release any semaphores they may be holding.
 */
NimBLERemoteService::~NimBLERemoteService() {
    removeCharacteristics();
}


/**
 * @brief Get the remote characteristic object for the characteristic UUID.
 * @param [in] uuid Remote characteristic uuid.
 * @return Reference to the remote characteristic object.
 */
NimBLERemoteCharacteristic* NimBLERemoteService::getCharacteristic(const char* uuid) {
    return getCharacteristic(NimBLEUUID(uuid));
} // getCharacteristic


/**
 * @brief Get the characteristic object for the UUID.
 * @param [in] uuid Characteristic uuid.
 * @return Reference to the characteristic object, or nullptr if not found.
 */
NimBLERemoteCharacteristic* NimBLERemoteService::getCharacteristic(const NimBLEUUID &uuid) {
    if (m_haveCharacteristics) {
        for(auto &it: m_characteristicVector) {
            if(it->getUUID() == uuid) {
                return it;
            }
        }
    }

    return nullptr;
} // getCharacteristic


/**
 * @brief Callback for Characterisic discovery.
 */
int NimBLERemoteService::characteristicDiscCB(uint16_t conn_handle,
                                const struct ble_gatt_error *error,
                                const struct ble_gatt_chr *chr, void *arg)
{
    NIMBLE_LOGD(LOG_TAG,"Characteristic Discovered >> status: %d handle: %d", error->status, conn_handle);

    NimBLERemoteService *service = (NimBLERemoteService*)arg;
    int rc=0;

    // Make sure the discovery is for this device
    if(service->getClient()->getConnId() != conn_handle){
        return 0;
    }

    switch (error->status) {
        case 0: {
            // Found a service - add it to the vector
            NimBLERemoteCharacteristic* pRemoteCharacteristic = new NimBLERemoteCharacteristic(service, chr);
            service->m_characteristicVector.push_back(pRemoteCharacteristic);
            break;
        }
        case BLE_HS_EDONE:{
            /** All characteristics in this service discovered; start discovering
             *  characteristics in the next service.
             */
            NIMBLE_SEMAPHORE_GIVE(service->m_pSemaphore, 0)
            rc = 0;
            break;
        }
        default:
            rc = error->status;
            break;
    }
    if (rc != 0) {
        /* Error; abort discovery. */
        // pass non-zero to semaphore on error to indicate an error finding characteristics
        NIMBLE_LOGE(LOG_TAG, "characteristicDiscCB() rc=%d %s", rc, NimBLEUtils::returnCodeToString(rc));
        NIMBLE_SEMAPHORE_GIVE(service->m_pSemaphore, 1)
    }
    NIMBLE_LOGD(LOG_TAG,"<< Characteristic Discovered. status: %d", rc);
    return rc;
}


/**
 * @brief Retrieve all the characteristics for this service.
 * This function will not return until we have all the characteristics.
 * @return N/A
 */
bool NimBLERemoteService::retrieveCharacteristics() {
    NIMBLE_LOGD(LOG_TAG, ">> retrieveCharacteristics() for service: %s", getUUID().toString().c_str());

    int rc = 0;

    NIMBLE_SEMAPHORE_TAKE(m_pSemaphore, "Retrieve Chars")

    rc = ble_gattc_disc_all_chrs(m_pClient->getConnId(),
                                 m_startHandle,
                                 m_endHandle,
                                 NimBLERemoteService::characteristicDiscCB,
                                 this);
    if (rc != 0) {
        NIMBLE_LOGE(LOG_TAG, "ble_gattc_disc_all_chrs: rc=%d %s", rc, NimBLEUtils::returnCodeToString(rc));
        m_haveCharacteristics = false;
        NIMBLE_SEMAPHORE_DELETE(m_pSemaphore)
        return false;
    }

    rc = NIMBLE_SEMAPHORE_WAIT(m_pSemaphore)
    NIMBLE_SEMAPHORE_DELETE(m_pSemaphore)

    if(rc == 0){
        m_haveCharacteristics = true;
        uint16_t endHdl = 0xFFFF;

        NIMBLE_LOGD(LOG_TAG, "Found %d Characteristics", m_characteristicVector.size());
        for(auto it = m_characteristicVector.cbegin(); it != m_characteristicVector.cend(); ++it) {
            NIMBLE_LOGD(LOG_TAG, "Found UUID: %s Handle: %d Def Handle: %d", (*it)->getUUID().toString().c_str(), (*it)->getHandle(), (*it)->getDefHandle());
            // The descriptor handle is between this characteristic val_handle and the next ones def_handle
            // so make the end of the scan at the handle before the next characteristic def_handle

            // Make sure we don't go past the service end handle
            if(++it != m_characteristicVector.cend()){
                NIMBLE_LOGD(LOG_TAG, "Next UUID: %s Handle: %d Def Handle: %d", (*it)->getUUID().toString().c_str(), (*it)->getHandle(),(*it)->getDefHandle());

                endHdl = (*it)->getDefHandle()-1;
            }
            else{
                NIMBLE_LOGD(LOG_TAG, "END CHARS");
                endHdl = m_endHandle;
            }
            --it;

            //If there is no handles between this characteristic and the next there is no descriptor so skip to the next
            if((*it)->getHandle() != endHdl){
                if(!m_pClient->m_isConnected || !(*it)->retrieveDescriptors(endHdl)) {
                    return false;
                }
            }
            //NIMBLE_LOGD(LOG_TAG, "Found %d Characteristics in service UUID: %s", chars->size(), myPair.first.c_str());
        }

        NIMBLE_LOGD(LOG_TAG, "<< retrieveCharacteristics()");
        return true;
    }

    NIMBLE_LOGE(LOG_TAG, "Could not retrieve characteristics");
    return false;

} // retrieveCharacteristics


/**
 * @brief Retrieve a vector of all the characteristics of this service.
 * @return A vector of all the characteristics of this service.
 */
std::vector<NimBLERemoteCharacteristic*>* NimBLERemoteService::getCharacteristics() {
    return &m_characteristicVector;
} // getCharacteristics


/**
 * @brief Get the client associated with this service.
 * @return A reference to the client associated with this service.
 */
NimBLEClient* NimBLERemoteService::getClient() {
    return m_pClient;
} // getClient


/**
 * @brief Get the service end handle.
 */
uint16_t NimBLERemoteService::getEndHandle() {
    return m_endHandle;
} // getEndHandle


/**
 * @brief Get the service start handle.
 */
uint16_t NimBLERemoteService::getStartHandle() {
    return m_startHandle;
} // getStartHandle


/**
 * @brief Get the service UUID.
 */
NimBLEUUID NimBLERemoteService::getUUID() {
    return m_uuid;
}


/**
 * @brief Read the value of a characteristic associated with this service.
 * @param [in] characteristicUuid The characteristic to read.
 * @returns a string containing the value or an empty string if not found or error.
 */
std::string NimBLERemoteService::getValue(const NimBLEUUID &characteristicUuid) {
    NIMBLE_LOGD(LOG_TAG, ">> readValue: uuid: %s", characteristicUuid.toString().c_str());

    std::string ret = "";
    NimBLERemoteCharacteristic* pChar = getCharacteristic(characteristicUuid);

    if(pChar != nullptr) {
        ret =  pChar->readValue();
    }

    NIMBLE_LOGD(LOG_TAG, "<< readValue");
    return ret;
} // readValue


/**
 * @brief Set the value of a characteristic.
 * @param [in] characteristicUuid The characteristic to set.
 * @param [in] value The value to set.
 * @returns true on success, false if not found or error
 */
bool NimBLERemoteService::setValue(const NimBLEUUID &characteristicUuid, const std::string &value) {
    NIMBLE_LOGD(LOG_TAG, ">> setValue: uuid: %s", characteristicUuid.toString().c_str());

    bool ret = false;
    NimBLERemoteCharacteristic* pChar = getCharacteristic(characteristicUuid);

    if(pChar != nullptr) {
         ret =  pChar->writeValue(value);
    }

    NIMBLE_LOGD(LOG_TAG, "<< setValue");
    return ret;
} // setValue


/**
 * @brief Delete the characteristics in the characteristics vector.
 * We maintain a vector called m_characteristicsVector that contains pointers to BLERemoteCharacteristic
 * object references. Since we allocated these in this class, we are also responsible for deleting
 * them. This method does just that.
 * @return N/A.
 */
void NimBLERemoteService::removeCharacteristics() {
    for(auto &it: m_characteristicVector) {
        delete it;
    }
    m_characteristicVector.clear();   // Clear the vector
} // removeCharacteristics


/**
 * @brief Create a string representation of this remote service.
 * @return A string representation of this remote service.
 */
std::string NimBLERemoteService::toString() {
    std::string res = "Service: uuid: " + m_uuid.toString();
    char val[6];
    res += ", start_handle: ";
    snprintf(val, sizeof(val), "%d", m_startHandle);
    res += val;
    snprintf(val, sizeof(val), "%04x", m_startHandle);
    res += " 0x";
    res += val;
    res += ", end_handle: ";
    snprintf(val, sizeof(val), "%d", m_endHandle);
    res += val;
    snprintf(val, sizeof(val), "%04x", m_endHandle);
    res += " 0x";
    res += val;

    for (auto &it: m_characteristicVector) {
        res += "\n" + it->toString();
    }

    return res;
} // toString


#endif // #if defined( CONFIG_BT_NIMBLE_ROLE_CENTRAL)
#endif /* CONFIG_BT_ENABLED */
