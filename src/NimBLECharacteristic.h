/*
 * NimBLECharacteristic.h
 *
 *  Created: on March 3, 2020
 *      Author H2zero
 *
 * Originally:
 * BLECharacteristic.h
 *
 *  Created on: Jun 22, 2017
 *      Author: kolban
 */

#ifndef NIMBLE_CPP_CHARACTERISTIC_H_
#define NIMBLE_CPP_CHARACTERISTIC_H_
#include "nimconfig.h"
#if defined(CONFIG_BT_ENABLED) && defined(CONFIG_BT_NIMBLE_ROLE_PERIPHERAL)

class NimBLECharacteristicCallbacks;
class NimBLECharacteristic;

# include "NimBLELocalValueAttribute.h"
# include "NimBLEServer.h"
# include "NimBLEService.h"
# include "NimBLEDescriptor.h"
# include "NimBLEAttValue.h"
# include "NimBLEConnInfo.h"

# include <string>
# include <vector>

/**
 * @brief The model of a BLE Characteristic.
 *
 * A BLE Characteristic is an identified value container that manages a value. It is exposed by a BLE service and
 * can be read and written to by a BLE client.
 */
class NimBLECharacteristic : public NimBLELocalValueAttribute {
  public:
    NimBLECharacteristic(const char*    uuid,
                         uint16_t       properties = NIMBLE_PROPERTY::READ | NIMBLE_PROPERTY::WRITE,
                         uint16_t       maxLen    = BLE_ATT_ATTR_MAX_LEN,
                         NimBLEService* pService   = nullptr);
    NimBLECharacteristic(const NimBLEUUID& uuid,
                         uint16_t          properties = NIMBLE_PROPERTY::READ | NIMBLE_PROPERTY::WRITE,
                         uint16_t          maxLen    = BLE_ATT_ATTR_MAX_LEN,
                         NimBLEService*    pService   = nullptr);

    ~NimBLECharacteristic();

    std::string toString() const;
    void        addDescriptor(NimBLEDescriptor* pDescriptor);
    void        removeDescriptor(NimBLEDescriptor* pDescriptor, bool deleteDsc = false);
    uint16_t    getProperties() const;
    void        setCallbacks(NimBLECharacteristicCallbacks* pCallbacks);
    bool        indicate(uint16_t connHandle = BLE_HS_CONN_HANDLE_NONE) const;
    bool        indicate(const uint8_t* value, size_t length, uint16_t connHandle = BLE_HS_CONN_HANDLE_NONE) const;
    bool        notify(uint16_t connHandle = BLE_HS_CONN_HANDLE_NONE) const;
    bool        notify(const uint8_t* value, size_t length, uint16_t connHandle = BLE_HS_CONN_HANDLE_NONE) const;

    NimBLEDescriptor* createDescriptor(const char* uuid,
                                       uint32_t    properties = NIMBLE_PROPERTY::READ | NIMBLE_PROPERTY::WRITE,
                                       uint16_t    maxLen    = BLE_ATT_ATTR_MAX_LEN);
    NimBLEDescriptor* createDescriptor(const NimBLEUUID& uuid,
                                       uint32_t          properties = NIMBLE_PROPERTY::READ | NIMBLE_PROPERTY::WRITE,
                                       uint16_t          maxLen    = BLE_ATT_ATTR_MAX_LEN);
    NimBLEDescriptor* getDescriptorByUUID(const char* uuid) const;
    NimBLEDescriptor* getDescriptorByUUID(const NimBLEUUID& uuid) const;
    NimBLEDescriptor* getDescriptorByHandle(uint16_t handle) const;
    NimBLEService*    getService() const;

    NimBLECharacteristicCallbacks* getCallbacks() const;

    /*********************** Template Functions ************************/

    /**
     * @brief Template to send a notification for classes which may have
     *        data()/size() or c_str()/length() methods. Falls back to sending
     *        the data by casting the first element of the array to a uint8_t
     *        pointer and getting the length of the array using sizeof.
     * @tparam T The a reference to a class containing the data to send.
     * @param[in] value The <type\>value to set.
     * @param[in] connHandle The connection handle to send the notification to.
     * @note This function is only available if the type T is not a pointer.
     */
    template <typename T>
    typename std::enable_if<!std::is_pointer<T>::value, bool>::type notify(const T& value,
                                                                           uint16_t connHandle = BLE_HS_CONN_HANDLE_NONE) const {
        if constexpr (Has_data_size<T>::value) {
            return notify(reinterpret_cast<const uint8_t*>(value.data()), value.size(), connHandle);
        } else if constexpr (Has_c_str_length<T>::value) {
            return notify(reinterpret_cast<const uint8_t*>(value.c_str()), value.length(), connHandle);
        } else {
            return notify(reinterpret_cast<const uint8_t*>(&value), sizeof(value), connHandle);
        }
    }

    /**
     * @brief Template to send an indication for classes which may have
     *       data()/size() or c_str()/length() methods. Falls back to sending
     *       the data by casting the first element of the array to a uint8_t
     *       pointer and getting the length of the array using sizeof.
     * @tparam T The a reference to a class containing the data to send.
     * @param[in] value The <type\>value to set.
     * @param[in] connHandle The connection handle to send the indication to.
     * @note This function is only available if the type T is not a pointer.
     */
    template <typename T>
    typename std::enable_if<!std::is_pointer<T>::value, bool>::type indicate(
        const T& value, uint16_t connHandle = BLE_HS_CONN_HANDLE_NONE) const {
        if constexpr (Has_data_size<T>::value) {
            return indicate(reinterpret_cast<const uint8_t*>(value.data()), value.size(), connHandle);
        } else if constexpr (Has_c_str_length<T>::value) {
            return indicate(reinterpret_cast<const uint8_t*>(value.c_str()), value.length(), connHandle);
        } else {
            return indicate(reinterpret_cast<const uint8_t*>(&value), sizeof(value), connHandle);
        }
    }

  private:
    friend class NimBLEServer;
    friend class NimBLEService;

    void setService(NimBLEService* pService);
    void readEvent(NimBLEConnInfo& connInfo) override;
    void writeEvent(const uint8_t* val, uint16_t len, NimBLEConnInfo& connInfo) override;
    bool sendValue(const uint8_t* value,
                   size_t         length,
                   bool           is_notification = true,
                   uint16_t       connHandle     = BLE_HS_CONN_HANDLE_NONE) const;

    NimBLECharacteristicCallbacks* m_pCallbacks{nullptr};
    NimBLEService*                 m_pService{nullptr};
    std::vector<NimBLEDescriptor*> m_vDescriptors{};
}; // NimBLECharacteristic

/**
 * @brief Callbacks that can be associated with a %BLE characteristic to inform of events.
 *
 * When a server application creates a %BLE characteristic, we may wish to be informed when there is either
 * a read or write request to the characteristic's value. An application can register a
 * sub-classed instance of this class and will be notified when such an event happens.
 */
class NimBLECharacteristicCallbacks {
  public:
    virtual ~NimBLECharacteristicCallbacks() {}
    virtual void onRead(NimBLECharacteristic* pCharacteristic, NimBLEConnInfo& connInfo);
    virtual void onWrite(NimBLECharacteristic* pCharacteristic, NimBLEConnInfo& connInfo);
    virtual void onStatus(NimBLECharacteristic* pCharacteristic, int code);
    virtual void onSubscribe(NimBLECharacteristic* pCharacteristic, NimBLEConnInfo& connInfo, uint16_t subValue);
};

#endif /* CONFIG_BT_ENABLED  && CONFIG_BT_NIMBLE_ROLE_PERIPHERAL */
#endif /*NIMBLE_CPP_CHARACTERISTIC_H_*/
