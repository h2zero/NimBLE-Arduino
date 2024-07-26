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
                         uint16_t       max_len    = BLE_ATT_ATTR_MAX_LEN,
                         NimBLEService* pService   = nullptr);
    NimBLECharacteristic(const NimBLEUUID& uuid,
                         uint16_t          properties = NIMBLE_PROPERTY::READ | NIMBLE_PROPERTY::WRITE,
                         uint16_t          max_len    = BLE_ATT_ATTR_MAX_LEN,
                         NimBLEService*    pService   = nullptr);

    ~NimBLECharacteristic();

    std::string toString() const;
    size_t      getSubscribedCount() const;
    void        addDescriptor(NimBLEDescriptor* pDescriptor);
    void        removeDescriptor(NimBLEDescriptor* pDescriptor, bool deleteDsc = false);
    uint16_t    getProperties() const;
    void        setCallbacks(NimBLECharacteristicCallbacks* pCallbacks);
    void        indicate(uint16_t conn_handle = BLE_HS_CONN_HANDLE_NONE) const;
    void        indicate(const uint8_t* value, size_t length, uint16_t conn_handle = BLE_HS_CONN_HANDLE_NONE) const;
    void        indicate(const std::vector<uint8_t>& value, uint16_t conn_handle = BLE_HS_CONN_HANDLE_NONE) const;
    void        notify(uint16_t conn_handle = BLE_HS_CONN_HANDLE_NONE) const;
    void        notify(const uint8_t* value, size_t length, uint16_t conn_handle = BLE_HS_CONN_HANDLE_NONE) const;
    void        notify(const std::vector<uint8_t>& value, uint16_t conn_handle = BLE_HS_CONN_HANDLE_NONE) const;

    NimBLEDescriptor* createDescriptor(const char* uuid,
                                       uint32_t    properties = NIMBLE_PROPERTY::READ | NIMBLE_PROPERTY::WRITE,
                                       uint16_t    max_len    = BLE_ATT_ATTR_MAX_LEN);
    NimBLEDescriptor* createDescriptor(const NimBLEUUID& uuid,
                                       uint32_t          properties = NIMBLE_PROPERTY::READ | NIMBLE_PROPERTY::WRITE,
                                       uint16_t          max_len    = BLE_ATT_ATTR_MAX_LEN);
    NimBLEDescriptor* getDescriptorByUUID(const char* uuid) const;
    NimBLEDescriptor* getDescriptorByUUID(const NimBLEUUID& uuid) const;
    NimBLEDescriptor* getDescriptorByHandle(uint16_t handle) const;
    NimBLEService*    getService() const;

    NimBLECharacteristicCallbacks* getCallbacks() const;

    /*********************** Template Functions ************************/

    /**
     * @brief Template to send a notification from a class type that has a c_str() and length() method.
     * @tparam T The a reference to a class containing the data to send.
     * @param[in] value The <type\>value to set.
     * @param[in] is_notification if true sends a notification, false sends an indication.
     * @details Only used if the <type\> has a `c_str()` method.
     */
    template <typename T>
# ifdef _DOXYGEN_
    void
# else
    typename std::enable_if<Has_c_str_len<T>::value, void>::type
# endif
    notify(const T& value, bool is_notification = true) const {
        notify(reinterpret_cast<const uint8_t*>(value.c_str()), value.length(), is_notification);
    }

    /**
     * @brief Template to send an indication from a class type that has a c_str() and length() method.
     * @tparam T The a reference to a class containing the data to send.
     * @param[in] value The <type\>value to set.
     * @details Only used if the <type\> has a `c_str()` method.
     */
    template <typename T>
# ifdef _DOXYGEN_
    void
# else
    typename std::enable_if<Has_c_str_len<T>::value, void>::type
# endif
    indicate(const T& value) const {
        indicate(reinterpret_cast<const uint8_t*>(value.c_str()), value.length());
    }

  private:
    friend class NimBLEServer;
    friend class NimBLEService;

    void setService(NimBLEService* pService);
    void setSubscribe(const ble_gap_event* event, NimBLEConnInfo& connInfo);
    void readEvent(NimBLEConnInfo& connInfo) override;
    void writeEvent(const uint8_t* val, uint16_t len, NimBLEConnInfo& connInfo) override;
    void sendValue(const uint8_t* value,
                   size_t         length,
                   bool           is_notification = true,
                   uint16_t       conn_handle     = BLE_HS_CONN_HANDLE_NONE) const;

    NimBLECharacteristicCallbacks*             m_pCallbacks{nullptr};
    NimBLEService*                             m_pService{nullptr};
    std::vector<NimBLEDescriptor*>             m_vDescriptors{};
    std::vector<std::pair<uint16_t, uint16_t>> m_subscribedVec{};
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
