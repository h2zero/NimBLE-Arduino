/*
 * NimBLECharacteristic.cpp
 *
 *  Created: on March 3, 2020
 *      Author H2zero
 *
 * BLECharacteristic.cpp
 *
 *  Created on: Jun 22, 2017
 *      Author: kolban
 */

#include "nimconfig.h"
#if defined(CONFIG_BT_ENABLED) && defined(CONFIG_BT_NIMBLE_ROLE_PERIPHERAL)

# include "NimBLECharacteristic.h"
# include "NimBLE2904.h"
# include "NimBLEDevice.h"
# include "NimBLELog.h"

# define NIMBLE_SUB_NOTIFY   0x0001
# define NIMBLE_SUB_INDICATE 0x0002

static NimBLECharacteristicCallbacks defaultCallback;
static const char*                   LOG_TAG = "NimBLECharacteristic";

/**
 * @brief Construct a characteristic
 * @param [in] uuid - UUID (const char*) for the characteristic.
 * @param [in] properties - Properties for the characteristic.
 * @param [in] max_len - The maximum length in bytes that the characteristic value can hold. (Default: 512 bytes for esp32, 20 for all others).
 * @param [in] pService - pointer to the service instance this characteristic belongs to.
 */
NimBLECharacteristic::NimBLECharacteristic(const char* uuid, uint16_t properties, uint16_t max_len, NimBLEService* pService)
    : NimBLECharacteristic(NimBLEUUID(uuid), properties, max_len, pService) {}

/**
 * @brief Construct a characteristic
 * @param [in] uuid - UUID for the characteristic.
 * @param [in] properties - Properties for the characteristic.
 * @param [in] max_len - The maximum length in bytes that the characteristic value can hold. (Default: 512 bytes for esp32, 20 for all others).
 * @param [in] pService - pointer to the service instance this characteristic belongs to.
 */
NimBLECharacteristic::NimBLECharacteristic(const NimBLEUUID& uuid, uint16_t properties, uint16_t max_len, NimBLEService* pService)
    : NimBLELocalValueAttribute{uuid, 0, max_len}, m_pCallbacks{&defaultCallback}, m_pService{pService} {
    setProperties(properties);
} // NimBLECharacteristic

/**
 * @brief Destructor.
 */
NimBLECharacteristic::~NimBLECharacteristic() {
    for (const auto& dsc : m_vDescriptors) {
        delete dsc;
    }
} // ~NimBLECharacteristic

/**
 * @brief Create a new BLE Descriptor associated with this characteristic.
 * @param [in] uuid - The UUID of the descriptor.
 * @param [in] properties - The properties of the descriptor.
 * @param [in] max_len - The max length in bytes of the descriptor value.
 * @return The new BLE descriptor.
 */
NimBLEDescriptor* NimBLECharacteristic::createDescriptor(const char* uuid, uint32_t properties, uint16_t max_len) {
    return createDescriptor(NimBLEUUID(uuid), properties, max_len);
}

/**
 * @brief Create a new BLE Descriptor associated with this characteristic.
 * @param [in] uuid - The UUID of the descriptor.
 * @param [in] properties - The properties of the descriptor.
 * @param [in] max_len - The max length in bytes of the descriptor value.
 * @return The new BLE descriptor.
 */
NimBLEDescriptor* NimBLECharacteristic::createDescriptor(const NimBLEUUID& uuid, uint32_t properties, uint16_t max_len) {
    NimBLEDescriptor* pDescriptor = nullptr;
    if (uuid == NimBLEUUID(uint16_t(0x2904))) {
        pDescriptor = new NimBLE2904(this);
    } else {
        pDescriptor = new NimBLEDescriptor(uuid, properties, max_len, this);
    }

    addDescriptor(pDescriptor);
    return pDescriptor;
} // createDescriptor

/**
 * @brief Add a descriptor to the characteristic.
 * @param [in] pDescriptor A pointer to the descriptor to add.
 */
void NimBLECharacteristic::addDescriptor(NimBLEDescriptor* pDescriptor) {
    bool foundRemoved = false;
    if (pDescriptor->getRemoved() > 0) {
        for (const auto& dsc : m_vDescriptors) {
            if (dsc == pDescriptor) {
                foundRemoved = true;
                pDescriptor->setRemoved(0);
            }
        }
    }

    // Check if the descriptor is already in the vector and if so, return.
    for (const auto& dsc : m_vDescriptors) {
        if (dsc == pDescriptor) {
            pDescriptor->setCharacteristic(this); // Update the characteristic pointer in the descriptor.
            return;
        }
    }

    if (!foundRemoved) {
        m_vDescriptors.push_back(pDescriptor);
    }

    pDescriptor->setCharacteristic(this);
    NimBLEDevice::getServer()->serviceChanged();
}

/**
 * @brief Remove a descriptor from the characteristic.
 * @param[in] pDescriptor A pointer to the descriptor instance to remove from the characteristic.
 * @param[in] deleteDsc If true it will delete the descriptor instance and free it's resources.
 */
void NimBLECharacteristic::removeDescriptor(NimBLEDescriptor* pDescriptor, bool deleteDsc) {
    // Check if the descriptor was already removed and if so, check if this
    // is being called to delete the object and do so if requested.
    // Otherwise, ignore the call and return.
    if (pDescriptor->getRemoved() > 0) {
        if (deleteDsc) {
            for (auto it = m_vDescriptors.begin(); it != m_vDescriptors.end(); ++it) {
                if ((*it) == pDescriptor) {
                    delete (*it);
                    m_vDescriptors.erase(it);
                    break;
                }
            }
        }

        return;
    }

    pDescriptor->setRemoved(deleteDsc ? NIMBLE_ATT_REMOVE_DELETE : NIMBLE_ATT_REMOVE_HIDE);
    NimBLEDevice::getServer()->serviceChanged();
} // removeDescriptor

/**
 * @brief Return the BLE Descriptor for the given UUID.
 * @param [in] uuid The UUID of the descriptor.
 * @return A pointer to the descriptor object or nullptr if not found.
 */
NimBLEDescriptor* NimBLECharacteristic::getDescriptorByUUID(const char* uuid) const {
    return getDescriptorByUUID(NimBLEUUID(uuid));
} // getDescriptorByUUID

/**
 * @brief Return the BLE Descriptor for the given UUID.
 * @param [in] uuid The UUID of the descriptor.
 * @return A pointer to the descriptor object or nullptr if not found.
 */
NimBLEDescriptor* NimBLECharacteristic::getDescriptorByUUID(const NimBLEUUID& uuid) const {
    for (const auto& dsc : m_vDescriptors) {
        if (dsc->getUUID() == uuid) {
            return dsc;
        }
    }
    return nullptr;
} // getDescriptorByUUID

/**
 * @brief Return the BLE Descriptor for the given handle.
 * @param [in] handle The handle of the descriptor.
 * @return A pointer to the descriptor object or nullptr if not found.
 */
NimBLEDescriptor* NimBLECharacteristic::getDescriptorByHandle(uint16_t handle) const {
    for (const auto& dsc : m_vDescriptors) {
        if (dsc->getHandle() == handle) {
            return dsc;
        }
    }
    return nullptr;
} // getDescriptorByHandle

/**
 * @brief Get the properties of the characteristic.
 * @return The properties of the characteristic.
 */
uint16_t NimBLECharacteristic::getProperties() const {
    return m_properties;
} // getProperties

/**
 * @brief Get the service that owns this characteristic.
 */
NimBLEService* NimBLECharacteristic::getService() const {
    return m_pService;
} // getService

void NimBLECharacteristic::setService(NimBLEService* pService) {
    m_pService = pService;
} // setService

/**
 * @brief Get the number of clients subscribed to the characteristic.
 * @returns Number of clients subscribed to notifications / indications.
 */
size_t NimBLECharacteristic::getSubscribedCount() const {
    return m_subscribedVec.size();
}

/**
 * @brief Set the subscribe status for this characteristic.\n
 * This will maintain a vector of subscribed clients and their indicate/notify status.
 */
void NimBLECharacteristic::setSubscribe(const ble_gap_event* event, NimBLEConnInfo& connInfo) {
    uint16_t subVal = 0;
    if (event->subscribe.cur_notify > 0 && (m_properties & NIMBLE_PROPERTY::NOTIFY)) {
        subVal |= NIMBLE_SUB_NOTIFY;
    }
    if (event->subscribe.cur_indicate && (m_properties & NIMBLE_PROPERTY::INDICATE)) {
        subVal |= NIMBLE_SUB_INDICATE;
    }

    NIMBLE_LOGI(LOG_TAG, "New subscribe value for conn: %d val: %d", connInfo.getConnHandle(), subVal);

    if (!event->subscribe.cur_indicate && event->subscribe.prev_indicate) {
        NimBLEDevice::getServer()->clearIndicateWait(connInfo.getConnHandle());
    }

    auto it = m_subscribedVec.begin();
    for (; it != m_subscribedVec.end(); ++it) {
        if ((*it).first == connInfo.getConnHandle()) {
            break;
        }
    }

    if (subVal > 0) {
        if (it == m_subscribedVec.end()) {
            m_subscribedVec.push_back({connInfo.getConnHandle(), subVal});
        } else {
            (*it).second = subVal;
        }
    } else if (it != m_subscribedVec.end()) {
        m_subscribedVec.erase(it);
    }

    m_pCallbacks->onSubscribe(this, connInfo, subVal);
}

/**
 * @brief Send an indication.
 * @param[in] conn_handle Connection handle to send an individual indication, or BLE_HS_CONN_HANDLE_NONE to send
 * the indication to all subscribed clients.
 */
void NimBLECharacteristic::indicate(uint16_t conn_handle) const {
    sendValue(m_value.data(), m_value.size(), false, conn_handle);
} // indicate

/**
 * @brief Send an indication.
 * @param[in] value A pointer to the data to send.
 * @param[in] length The length of the data to send.
 * @param[in] conn_handle Connection handle to send an individual indication, or BLE_HS_CONN_HANDLE_NONE to send
 * the indication to all subscribed clients.
 */
void NimBLECharacteristic::indicate(const uint8_t* value, size_t length, uint16_t conn_handle) const {
    sendValue(value, length, false, conn_handle);
} // indicate

/**
 * @brief Send a notification.
 * @param[in] conn_handle Connection handle to send an individual notification, or BLE_HS_CONN_HANDLE_NONE to send
 * the notification to all subscribed clients.
 */
void NimBLECharacteristic::notify(uint16_t conn_handle) const {
    sendValue(m_value.data(), m_value.size(), true, conn_handle);
} // notify

/**
 * @brief Send a notification.
 * @param[in] value A pointer to the data to send.
 * @param[in] length The length of the data to send.
 * @param[in] conn_handle Connection handle to send an individual notification, or BLE_HS_CONN_HANDLE_NONE to send
 * the notification to all subscribed clients.
 */
void NimBLECharacteristic::notify(const uint8_t* value, size_t length, uint16_t conn_handle) const {
    sendValue(value, length, true, conn_handle);
} // indicate

/**
 * @brief Sends a notification or indication.
 * @param[in] value A pointer to the data to send.
 * @param[in] length The length of the data to send.
 * @param[in] is_notification if true sends a notification, false sends an indication.
 * @param[in] conn_handle Connection handle to send to a specific peer, or BLE_HS_CONN_HANDLE_NONE to send
 * to all subscribed clients.
 */
void NimBLECharacteristic::sendValue(const uint8_t* value, size_t length, bool is_notification, uint16_t conn_handle) const {
    NIMBLE_LOGD(LOG_TAG, ">> sendValue");

    if (is_notification && !(getProperties() & NIMBLE_PROPERTY::NOTIFY)) {
        NIMBLE_LOGE(LOG_TAG, "<< sendValue: notification not enabled for characteristic");
        return;
    }

    if (!is_notification && !(getProperties() & NIMBLE_PROPERTY::INDICATE)) {
        NIMBLE_LOGE(LOG_TAG, "<< sendValue: indication not enabled for characteristic");
        return;
    }

    if (!m_subscribedVec.size()) {
        NIMBLE_LOGD(LOG_TAG, "<< sendValue: No clients subscribed.");
        return;
    }

    for (const auto& it : m_subscribedVec) {
        // check if connected and subscribed
        if (!it.second) {
            continue;
        }

        // sending to a specific client?
        if ((conn_handle <= BLE_HCI_LE_CONN_HANDLE_MAX) && (it.first != conn_handle)) {
            continue;
        }

        if (is_notification && !(it.second & NIMBLE_SUB_NOTIFY)) {
            continue;
        }

        if (!is_notification && !(it.second & NIMBLE_SUB_INDICATE)) {
            continue;
        }

        // check if security requirements are satisfied
        if ((getProperties() & BLE_GATT_CHR_F_READ_AUTHEN) || (getProperties() & BLE_GATT_CHR_F_READ_AUTHOR) ||
            (getProperties() & BLE_GATT_CHR_F_READ_ENC)) {
            ble_gap_conn_desc desc;
            if (ble_gap_conn_find(it.first, &desc) != 0 || !desc.sec_state.encrypted) {
                continue;
            }
        }

        // don't create the m_buf until we are sure to send the data or else
        // we could be allocating a buffer that doesn't get released.
        // We also must create it in each loop iteration because it is consumed with each host call.
        os_mbuf* om = ble_hs_mbuf_from_flat(value, length);
        if (!om) {
            NIMBLE_LOGE(LOG_TAG, "<< sendValue: failed to allocate mbuf");
            return;
        }

        if (is_notification) {
            ble_gattc_notify_custom(it.first, getHandle(), om);
        } else {
            if (!NimBLEDevice::getServer()->setIndicateWait(it.first)) {
                NIMBLE_LOGE(LOG_TAG, "<< sendValue: waiting for previous indicate");
                os_mbuf_free_chain(om);
                return;
            }

            if (ble_gattc_indicate_custom(it.first, getHandle(), om) != 0) {
                NimBLEDevice::getServer()->clearIndicateWait(it.first);
            }
        }
    }

    NIMBLE_LOGD(LOG_TAG, "<< sendValue");
} // sendValue

void NimBLECharacteristic::readEvent(NimBLEConnInfo& connInfo) {
    m_pCallbacks->onRead(this, connInfo);
}

void NimBLECharacteristic::writeEvent(const uint8_t* val, uint16_t len, NimBLEConnInfo& connInfo) {
    setValue(val, len);
    m_pCallbacks->onWrite(this, connInfo);
}

/**
 * @brief Set the callback handlers for this characteristic.
 * @param [in] pCallbacks An instance of a NimBLECharacteristicCallbacks class\n
 * used to define any callbacks for the characteristic.
 */
void NimBLECharacteristic::setCallbacks(NimBLECharacteristicCallbacks* pCallbacks) {
    if (pCallbacks != nullptr) {
        m_pCallbacks = pCallbacks;
    } else {
        m_pCallbacks = &defaultCallback;
    }
} // setCallbacks

/**
 * @brief Get the callback handlers for this characteristic.
 */
NimBLECharacteristicCallbacks* NimBLECharacteristic::getCallbacks() const {
    return m_pCallbacks;
} // getCallbacks

/**
 * @brief Return a string representation of the characteristic.
 * @return A string representation of the characteristic.
 */
std::string NimBLECharacteristic::toString() const {
    std::string res = "UUID: " + m_uuid.toString() + ", handle : 0x";
    char        hex[5];
    snprintf(hex, sizeof(hex), "%04x", getHandle());
    res += hex;
    res += " ";
    if (m_properties & BLE_GATT_CHR_PROP_READ) res += "Read ";
    if (m_properties & BLE_GATT_CHR_PROP_WRITE) res += "Write ";
    if (m_properties & BLE_GATT_CHR_PROP_WRITE_NO_RSP) res += "WriteNoResponse ";
    if (m_properties & BLE_GATT_CHR_PROP_BROADCAST) res += "Broadcast ";
    if (m_properties & BLE_GATT_CHR_PROP_NOTIFY) res += "Notify ";
    if (m_properties & BLE_GATT_CHR_PROP_INDICATE) res += "Indicate ";
    return res;
} // toString

/**
 * @brief Callback function to support a read request.
 * @param [in] pCharacteristic The characteristic that is the source of the event.
 * @param [in] connInfo A reference to a NimBLEConnInfo instance containing the peer info.
 */
void NimBLECharacteristicCallbacks::onRead(NimBLECharacteristic* pCharacteristic, NimBLEConnInfo& connInfo) {
    NIMBLE_LOGD("NimBLECharacteristicCallbacks", "onRead: default");
} // onRead

/**
 * @brief Callback function to support a write request.
 * @param [in] pCharacteristic The characteristic that is the source of the event.
 * @param [in] connInfo A reference to a NimBLEConnInfo instance containing the peer info.
 */
void NimBLECharacteristicCallbacks::onWrite(NimBLECharacteristic* pCharacteristic, NimBLEConnInfo& connInfo) {
    NIMBLE_LOGD("NimBLECharacteristicCallbacks", "onWrite: default");
} // onWrite

/**
 * @brief Callback function to support a Notify/Indicate Status report.
 * @param [in] pCharacteristic The characteristic that is the source of the event.
 * @param [in] code Status return code from the NimBLE stack.
 * @details The status code for success is 0 for notifications and BLE_HS_EDONE for indications,
 * any other value is an error.
 */
void NimBLECharacteristicCallbacks::onStatus(NimBLECharacteristic* pCharacteristic, int code) {
    NIMBLE_LOGD("NimBLECharacteristicCallbacks", "onStatus: default");
} // onStatus

/**
 * @brief Callback function called when a client changes subscription status.
 * @param [in] pCharacteristic The characteristic that is the source of the event.
 * @param [in] connInfo A reference to a NimBLEConnInfo instance containing the peer info.
 * @param [in] subValue The subscription status:
 * * 0 = Un-Subscribed
 * * 1 = Notifications
 * * 2 = Indications
 * * 3 = Notifications and Indications
 */
void NimBLECharacteristicCallbacks::onSubscribe(NimBLECharacteristic* pCharacteristic,
                                                NimBLEConnInfo& connInfo,
                                                uint16_t              subValue) {
    NIMBLE_LOGD("NimBLECharacteristicCallbacks", "onSubscribe: default");
}

#endif /* CONFIG_BT_ENABLED && CONFIG_BT_NIMBLE_ROLE_PERIPHERAL */
