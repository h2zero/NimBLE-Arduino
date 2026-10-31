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

#ifndef NIMBLE_CPP_CHARACTERISTIC_H_
#define NIMBLE_CPP_CHARACTERISTIC_H_

#include "syscfg/syscfg.h"
#if CONFIG_BT_NIMBLE_ENABLED && MYNEWT_VAL(BLE_ROLE_PERIPHERAL)

class NimBLECharacteristicCallbacks;
class NimBLEService;
class NimBLECharacteristic;
class NimBLEDescriptor;
class NimBLE2904;

# include "NimBLELocalValueAttribute.h"

# include <string>
# include <vector>
# include <array>

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
                         uint16_t       maxLen     = BLE_ATT_ATTR_MAX_LEN,
                         NimBLEService* pService   = nullptr);
    NimBLECharacteristic(const NimBLEUUID& uuid,
                         uint16_t          properties = NIMBLE_PROPERTY::READ | NIMBLE_PROPERTY::WRITE,
                         uint16_t          maxLen     = BLE_ATT_ATTR_MAX_LEN,
                         NimBLEService*    pService   = nullptr);

    ~NimBLECharacteristic();

    std::string toString() const;
    void        addDescriptor(NimBLEDescriptor* pDescriptor);
    void        removeDescriptor(NimBLEDescriptor* pDescriptor, bool deleteDsc = false);
    uint16_t    getProperties() const;
    void        setCallbacks(NimBLECharacteristicCallbacks& callbacks);
    void        setCallbacks(NimBLECharacteristicCallbacks&&) = delete; // Block rvalues
    void        resetCallbacks();
    bool        indicate(uint16_t connHandle = BLE_HS_CONN_HANDLE_NONE) const;
    bool        indicate(const uint8_t* value, size_t length, uint16_t connHandle = BLE_HS_CONN_HANDLE_NONE) const;
    bool        notify(uint16_t connHandle = BLE_HS_CONN_HANDLE_NONE) const;
    bool        notify(const uint8_t* value, size_t length, uint16_t connHandle = BLE_HS_CONN_HANDLE_NONE) const;

    NimBLEDescriptor* createDescriptor(const char* uuid,
                                       uint32_t    properties = NIMBLE_PROPERTY::READ | NIMBLE_PROPERTY::WRITE,
                                       uint16_t    maxLen     = BLE_ATT_ATTR_MAX_LEN);
    NimBLEDescriptor* createDescriptor(const NimBLEUUID& uuid,
                                       uint32_t          properties = NIMBLE_PROPERTY::READ | NIMBLE_PROPERTY::WRITE,
                                       uint16_t          maxLen     = BLE_ATT_ATTR_MAX_LEN);
    NimBLE2904*       create2904();
    NimBLEDescriptor* getDescriptorByUUID(const char* uuid, uint16_t index = 0) const;
    NimBLEDescriptor* getDescriptorByUUID(const NimBLEUUID& uuid, uint16_t index = 0) const;
    NimBLEDescriptor* getDescriptorByHandle(uint16_t handle) const;
    NimBLEService*    getService() const;

    NimBLECharacteristicCallbacks* getCallbacks() const;

    /*********************** Template Functions ************************/

    /**
     * @brief Template to send a notification from a container, string or trivially copyable object.
     * @tparam T The type of the value, must not be a pointer or array.
     * @param [in] value The value to send.
     * @param [in] connHandle Optional, a connection handle to send the notification to.
     * @details Types with `data()` and `size()` or `c_str()` and `length()` (including `std::string_view` and
     * `std::span`) are sent by their contents, anything else is sent by its object representation using `sizeof()`.
     */
    template <NimBLEByteValue T>
        requires(!std::is_array_v<T>)
    bool notify(const T& value, uint16_t connHandle = BLE_HS_CONN_HANDLE_NONE) const {
        const auto bytes = NimBLEAsBytes(value);
        return notify(bytes.data(), bytes.size(), connHandle);
    }

    /**
     * @brief Template to send an indication from a container, string or trivially copyable object.
     * @tparam T The type of the value, must not be a pointer or array.
     * @param [in] value The value to send.
     * @param [in] connHandle Optional, a connection handle to send the indication to.
     * @details See notify(const T&, uint16_t) for how the bytes are determined.
     */
    template <NimBLEByteValue T>
        requires(!std::is_array_v<T>)
    bool indicate(const T& value, uint16_t connHandle = BLE_HS_CONN_HANDLE_NONE) const {
        const auto bytes = NimBLEAsBytes(value);
        return indicate(bytes.data(), bytes.size(), connHandle);
    }

  private:
    friend class NimBLEServer;
    friend class NimBLEService;

    void    setService(NimBLEService* pService);
    uint8_t readEvent(NimBLEConnInfo& connInfo) override;
    uint8_t writeEvent(NimBLEAttValue& val, NimBLEConnInfo& connInfo) override;
    bool    sendValue(const uint8_t* value,
                      size_t         length,
                      bool           is_notification = true,
                      uint16_t       connHandle      = BLE_HS_CONN_HANDLE_NONE) const;

    struct SubPeerEntry {
        enum : uint8_t { AWAITING_SECURE = 1 << 0, SECURE = 1 << 1, SUB_NOTIFY = 1 << 2, SUB_INDICATE = 1 << 3 };
        void     setConnHandle(uint16_t connHandle) { m_connHandle = connHandle; }
        uint16_t getConnHandle() const { return m_connHandle; }
        void setAwaitingSecure(bool awaiting) { awaiting ? m_flags |= AWAITING_SECURE : m_flags &= ~AWAITING_SECURE; }
        void setSecured(bool secure) { secure ? m_flags |= SECURE : m_flags &= ~SECURE; }
        void setSubNotify(bool notify) { notify ? m_flags |= SUB_NOTIFY : m_flags &= ~SUB_NOTIFY; }
        void setSubIndicate(bool indicate) { indicate ? m_flags |= SUB_INDICATE : m_flags &= ~SUB_INDICATE; }
        bool isSubNotify() const { return m_flags & SUB_NOTIFY; }
        bool isSubIndicate() const { return m_flags & SUB_INDICATE; }
        bool isSecured() const { return m_flags & SECURE; }
        bool isAwaitingSecure() const { return m_flags & AWAITING_SECURE; }

      private:
        uint16_t m_connHandle{BLE_HS_CONN_HANDLE_NONE};
        uint8_t  m_flags{0};
    } __attribute__((packed));

    using SubPeerArray = std::array<SubPeerEntry, MYNEWT_VAL(BLE_MAX_CONNECTIONS)>;
    SubPeerArray getSubscribers() const { return m_subPeers; }
    void         processSubRequest(NimBLEConnInfo& connInfo, uint8_t subVal) const;
    void         updatePeerStatus(const NimBLEConnInfo& peerInfo) const;

    NimBLECharacteristicCallbacks* m_pCallbacks{nullptr};
    NimBLEService*                 m_pService{nullptr};
    std::vector<NimBLEDescriptor*> m_vDescriptors{};
    mutable SubPeerArray           m_subPeers{};
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
    virtual uint8_t onRead(NimBLECharacteristic* pCharacteristic, NimBLEConnInfo& connInfo);
    virtual uint8_t onWrite(NimBLECharacteristic* pCharacteristic, NimBLEAttValue& value, NimBLEConnInfo& connInfo);
    virtual void    onStatus(NimBLECharacteristic* pCharacteristic, NimBLEConnInfo& connInfo, int code);
    virtual void    onSubscribe(NimBLECharacteristic* pCharacteristic, NimBLEConnInfo& connInfo, uint16_t subValue);
};

#endif // CONFIG_BT_NIMBLE_ENABLED  && MYNEWT_VAL(BLE_ROLE_PERIPHERAL)
#endif // NIMBLE_CPP_CHARACTERISTIC_H_
