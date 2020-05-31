/*
 * NimBLECharacteristicMap.cpp
 *
 *  Created: on March 3, 2020
 *      Author H2zero
 *
 * BLECharacteristicMap.cpp
 *
 *  Created on: Jun 22, 2017
 *      Author: kolban
 */
#include "sdkconfig.h"
#if defined(CONFIG_BT_ENABLED)

#include "nimconfig.h"
#if defined(CONFIG_BT_NIMBLE_ROLE_PERIPHERAL)

#include "NimBLEService.h"
#include "NimBLELog.h"


/**
 * @brief Return the characteristic by handle.
 * @param [in] handle The handle to look up the characteristic.
 * @return The characteristic.
 */
NimBLECharacteristic* NimBLECharacteristicMap::getByHandle(uint16_t handle) {
    for(auto &it : m_chrVec) {
        if(it->getHandle() == handle) {
            return it;
        }
    }
    return nullptr;
} // getByHandle


/**
 * @brief Return the characteristic by UUID.
 * @param [in] UUID The UUID to look up the characteristic.
 * @return The characteristic.
 */
NimBLECharacteristic* NimBLECharacteristicMap::getByUUID(const char* uuid) {
    return getByUUID(NimBLEUUID(uuid));
}


/**
 * @brief Return the characteristic by UUID.
 * @param [in] UUID The UUID to look up the characteristic.
 * @return The characteristic.
 */
NimBLECharacteristic* NimBLECharacteristicMap::getByUUID(const NimBLEUUID &uuid) {
    for (auto &it : m_chrVec) {
        if (it->getUUID() == uuid) {
            return it;
        }
    }

    return nullptr;
} // getByUUID

/**
 * @brief Get the number of characteristics in the map.
 */
size_t NimBLECharacteristicMap::getSize() {
    return m_chrVec.size();
} // getSize

/**
 * @brief Get the first characteristic in the map.
 * @return The first characteristic in the map.
 */
NimBLECharacteristic* NimBLECharacteristicMap::getFirst() {
    m_iterator = m_chrVec.begin();
    if (m_iterator == m_chrVec.end()) return nullptr;
    NimBLECharacteristic* pRet = *m_iterator;
    m_iterator++;
    return pRet;
} // getFirst


/**
 * @brief Get the next characteristic in the map.
 * @return The next characteristic in the map.
 */
NimBLECharacteristic* NimBLECharacteristicMap::getNext() {
    if (m_iterator == m_chrVec.end()) return nullptr;
    NimBLECharacteristic* pRet = *m_iterator;
    m_iterator++;
    return pRet;
} // getNext


/**
 * @brief Set the characteristic by handle.
 * @param [in] handle The handle of the characteristic.
 * @param [in] characteristic The characteristic to cache.
 * @return N/A.
 *//*
void NimBLECharacteristicMap::setByHandle(uint16_t handle, NimBLECharacteristic* characteristic) {
    m_handleMap.insert(std::pair<uint16_t, NimBLECharacteristic*>(handle, characteristic));
} // setByHandle

*/
/**
 * @brief Set the characteristic by UUID.
 * @param [in] uuid The uuid of the characteristic.
 * @param [in] characteristic The characteristic to cache.
 * @return N/A.
 */
void NimBLECharacteristicMap::addCharacteristic(NimBLECharacteristic* pCharacteristic) {
    m_chrVec.push_back(pCharacteristic);
} // setByUUID


/**
 * @brief Return a string representation of the characteristic map.
 * @return A string representation of the characteristic map.
 */
std::string NimBLECharacteristicMap::toString() {
    std::string res;
    int count = 0;
    char hex[5];
    for (auto &it: m_chrVec) {
        if (count > 0) {res += "\n";}
        snprintf(hex, sizeof(hex), "%04x", it->getHandle());
        count++;
        res += "handle: 0x";
        res += hex;
        res += ", uuid: " + std::string(it->getUUID());
    }
    return res;
} // toString


#endif // #if defined(CONFIG_BT_NIMBLE_ROLE_PERIPHERAL)
#endif /* CONFIG_BT_ENABLED */
