/*
 * NimBLERemoteCharacteristic.cpp
 *
 *  Created: on Jan 27 2020
 *      Author H2zero
 *
 * Originally:
 *
 * BLERemoteCharacteristic.cpp
 *
 *  Created on: Mar 16, 2017
 *      Author: kolban
 */

#include "sdkconfig.h"
#if defined(CONFIG_BT_ENABLED)

#include "nimconfig.h"
#if defined( CONFIG_BT_NIMBLE_ROLE_CENTRAL)

#include "NimBLERemoteCharacteristic.h"
#include "NimBLEUtils.h"
#include "NimBLELog.h"

static const char* LOG_TAG = "NimBLERemoteCharacteristic";

/**
 * @brief Constructor.
 * @param [in] reference to the service this characteristic belongs to.
 * @param [in] ble_gatt_chr struct defined as:
 *  struct ble_gatt_chr {
 *      uint16_t def_handle;
 *      uint16_t val_handle;
 *      uint8_t properties;
 *      ble_uuid_any_t uuid;
 *  };
 */
 NimBLERemoteCharacteristic::NimBLERemoteCharacteristic(NimBLERemoteService *pRemoteService, const struct ble_gatt_chr *chr) {

     switch (chr->uuid.u.type) {
        case BLE_UUID_TYPE_16:
            m_uuid = NimBLEUUID(chr->uuid.u16.value);
            break;
        case BLE_UUID_TYPE_32:
            m_uuid = NimBLEUUID(chr->uuid.u32.value);
            break;
        case BLE_UUID_TYPE_128:
            m_uuid = NimBLEUUID(const_cast<ble_uuid128_t*>(&chr->uuid.u128));
            break;
        default:
            m_uuid = nullptr;
            break;
    }
    m_handle         = chr->val_handle;
    m_defHandle      = chr->def_handle;
    m_charProp       = chr->properties;
    m_pRemoteService = pRemoteService;
    m_notifyCallback = nullptr;
    m_rawData        = nullptr;
    m_dataLen        = 0;
    m_pSemaphore     = nullptr;
 } // NimBLERemoteCharacteristic


/**
 *@brief Destructor.
 */
NimBLERemoteCharacteristic::~NimBLERemoteCharacteristic() {
    removeDescriptors();   // Release resources for any descriptor information we may have allocated.
    if(m_rawData != nullptr) {
        free(m_rawData);
    }
} // ~NimBLERemoteCharacteristic

/*
#define BLE_GATT_CHR_PROP_BROADCAST                     0x01
#define BLE_GATT_CHR_PROP_READ                          0x02
#define BLE_GATT_CHR_PROP_WRITE_NO_RSP                  0x04
#define BLE_GATT_CHR_PROP_WRITE                         0x08
#define BLE_GATT_CHR_PROP_NOTIFY                        0x10
#define BLE_GATT_CHR_PROP_INDICATE                      0x20
#define BLE_GATT_CHR_PROP_AUTH_SIGN_WRITE               0x40
#define BLE_GATT_CHR_PROP_EXTENDED                      0x80
*/

/**
 * @brief Does the characteristic support broadcasting?
 * @return True if the characteristic supports broadcasting.
 */
bool NimBLERemoteCharacteristic::canBroadcast() {
    return (m_charProp & BLE_GATT_CHR_PROP_BROADCAST) != 0;
} // canBroadcast


/**
 * @brief Does the characteristic support indications?
 * @return True if the characteristic supports indications.
 */
bool NimBLERemoteCharacteristic::canIndicate() {
    return (m_charProp & BLE_GATT_CHR_PROP_INDICATE) != 0;
} // canIndicate


/**
 * @brief Does the characteristic support notifications?
 * @return True if the characteristic supports notifications.
 */
bool NimBLERemoteCharacteristic::canNotify() {
    return (m_charProp & BLE_GATT_CHR_PROP_NOTIFY) != 0;
} // canNotify


/**
 * @brief Does the characteristic support reading?
 * @return True if the characteristic supports reading.
 */
bool NimBLERemoteCharacteristic::canRead() {
    return (m_charProp & BLE_GATT_CHR_PROP_READ) != 0;
} // canRead


/**
 * @brief Does the characteristic support writing?
 * @return True if the characteristic supports writing.
 */
bool NimBLERemoteCharacteristic::canWrite() {
    return (m_charProp & BLE_GATT_CHR_PROP_WRITE) != 0;
} // canWrite


/**
 * @brief Does the characteristic support writing with no response?
 * @return True if the characteristic supports writing with no response.
 */
bool NimBLERemoteCharacteristic::canWriteNoResponse() {
    return (m_charProp & BLE_GATT_CHR_PROP_WRITE_NO_RSP) != 0;
} // canWriteNoResponse


/**
 * @brief Callback used by the API when a descriptor is discovered or search complete.
 */
int NimBLERemoteCharacteristic::descriptorDiscCB(uint16_t conn_handle,
                                    const struct ble_gatt_error *error,
                                    uint16_t chr_val_handle,
                                    const struct ble_gatt_dsc *dsc,
                                    void *arg)
{
    NIMBLE_LOGD(LOG_TAG,"Descriptor Discovered >> status: %d handle: %d", error->status, conn_handle);

    NimBLERemoteCharacteristic *characteristic = (NimBLERemoteCharacteristic*)arg;
    int rc=0;

    // Make sure the discovery is for this device
    if(characteristic->getRemoteService()->getClient()->getConnId() != conn_handle){
        return 0;
    }

    switch (error->status) {
        case 0: {
            // Found a descriptor - add it to the vector
            NimBLERemoteDescriptor* pNewRemoteDescriptor = new NimBLERemoteDescriptor(characteristic, dsc);
            characteristic->m_descriptorVector.push_back(pNewRemoteDescriptor);
            break;
        }
        case BLE_HS_EDONE:{
            /* All descriptors in this characteristic discovered; */
            NIMBLE_SEMAPHORE_GIVE(characteristic->m_pSemaphore, 0)
            rc = 0;
            break;
        }
        default:
            rc = error->status;
            break;
    }
    if (rc != 0) {
        /* Error; abort discovery. */
        // pass non-zero to semaphore on error to indicate an error finding descriptors
        NIMBLE_SEMAPHORE_GIVE(characteristic->m_pSemaphore, 1)
    }
    NIMBLE_LOGD(LOG_TAG,"<< Descriptor Discovered. status: %d", rc);
    return rc;
}

/**
 * @brief Populate the descriptors (if any) for this characteristic.
 * @param [in] the end handle of the characteristic, or the service, whichever comes first.
 */
bool NimBLERemoteCharacteristic::retrieveDescriptors(uint16_t endHdl) {
    NIMBLE_LOGD(LOG_TAG, ">> retrieveDescriptors() for characteristic: %s", getUUID().toString().c_str());

    int rc = 0;

    NIMBLE_SEMAPHORE_TAKE(m_pSemaphore, "Retrieve Descs")

    rc = ble_gattc_disc_all_dscs(getRemoteService()->getClient()->getConnId(),
                                 m_handle,
                                 endHdl,
                                 NimBLERemoteCharacteristic::descriptorDiscCB,
                                 this);
    if (rc != 0) {
        NIMBLE_LOGE(LOG_TAG, "ble_gattc_disc_all_chrs: rc=%d %s", rc, NimBLEUtils::returnCodeToString(rc));
        NIMBLE_SEMAPHORE_DELETE(m_pSemaphore)
        return false;
    }

    rc = NIMBLE_SEMAPHORE_WAIT(m_pSemaphore)
    NIMBLE_SEMAPHORE_DELETE(m_pSemaphore)

    if(rc != 0) {
        return false;
    }

    return true;
    NIMBLE_LOGD(LOG_TAG, "<< retrieveDescriptors(): Found %d descriptors.", m_descriptorVector.size());
} // getDescriptors


/**
 * @brief Retrieve the vector of descriptors.
 */
std::vector<NimBLERemoteDescriptor*>* NimBLERemoteCharacteristic::getDescriptors() {
    return &m_descriptorVector;
} // getDescriptors


/**
 * @brief Get the handle for this characteristic.
 * @return The handle for this characteristic.
 */
uint16_t NimBLERemoteCharacteristic::getHandle() {
    return m_handle;
} // getHandle

/**
 * @brief Get the handle for this characteristics definition.
 * @return The handle for this characteristic definition.
 */
uint16_t NimBLERemoteCharacteristic::getDefHandle() {
    return m_defHandle;
} // getDefHandle


/**
 * @brief Get the descriptor instance with the given UUID that belongs to this characteristic.
 * @param [in] uuid The UUID of the descriptor to find.
 * @return The Remote descriptor (if present) or null if not present.
 */
NimBLERemoteDescriptor* NimBLERemoteCharacteristic::getDescriptor(const NimBLEUUID &uuid) {
    NIMBLE_LOGD(LOG_TAG, ">> getDescriptor: uuid: %s", uuid.toString().c_str());

    for(auto &it: m_descriptorVector) {
          if(it->getUUID() == uuid) {
              NIMBLE_LOGD(LOG_TAG, "<< getDescriptor: found");
              return it;
          }
    }
    NIMBLE_LOGD(LOG_TAG, "<< getDescriptor: Not found");
    return nullptr;
} // getDescriptor


/**
 * @brief Get the remote service associated with this characteristic.
 * @return The remote service associated with this characteristic.
 */
NimBLERemoteService* NimBLERemoteCharacteristic::getRemoteService() {
    return m_pRemoteService;
} // getRemoteService


/**
 * @brief Get the UUID for this characteristic.
 * @return The UUID for this characteristic.
 */
NimBLEUUID NimBLERemoteCharacteristic::getUUID() {
    return m_uuid;
} // getUUID


/**
 * @brief Read an unsigned 16 bit value
 * @return The unsigned 16 bit value.
 */
uint16_t NimBLERemoteCharacteristic::readUInt16() {
    std::string value = readValue();
    if (value.length() >= 2) {
        return *(uint16_t*)(value.data());
    }
    return 0;
} // readUInt16


/**
 * @brief Read an unsigned 32 bit value.
 * @return the unsigned 32 bit value.
 */
uint32_t NimBLERemoteCharacteristic::readUInt32() {
    std::string value = readValue();
    if (value.length() >= 4) {
        return *(uint32_t*)(value.data());
    }
    return 0;
} // readUInt32


/**
 * @brief Read a byte value
 * @return The value as a byte
 */
uint8_t NimBLERemoteCharacteristic::readUInt8() {
    std::string value = readValue();
    if (value.length() >= 1) {
        return (uint8_t)value[0];
    }
    return 0;
} // readUInt8


/**
 * @brief Read the value of the remote characteristic.
 * @return The value of the remote characteristic.
 */
std::string NimBLERemoteCharacteristic::readValue() {
    NIMBLE_LOGD(LOG_TAG, ">> readValue(): uuid: %s, handle: %d 0x%.2x", getUUID().toString().c_str(), getHandle(), getHandle());

    int rc = 0;
    int retryCount = 1;
    // Clear the value before reading.
    m_value = "";

    NimBLEClient* pClient = getRemoteService()->getClient();

    // Check to see that we are connected.
    if (!pClient->isConnected()) {
        NIMBLE_LOGE(LOG_TAG, "Disconnected");
        return "";
    }

    do {
        NIMBLE_SEMAPHORE_TAKE(m_pSemaphore, "Read value")

        rc = ble_gattc_read_long(pClient->getConnId(), m_handle, 0,
                                 NimBLERemoteCharacteristic::onReadCB,
                                 this);
        if (rc != 0) {
            NIMBLE_LOGE(LOG_TAG, "Error: Failed to read characteristic; rc=%d, %s",
                                  rc, NimBLEUtils::returnCodeToString(rc));
            NIMBLE_SEMAPHORE_DELETE(m_pSemaphore)
            return "";
        }

        rc = NIMBLE_SEMAPHORE_WAIT(m_pSemaphore)
        switch(rc){
            case 0:
            case BLE_HS_EDONE:
                rc = 0;
                break;
            // Characteristic is not long-readable, return with what we have.
            case BLE_HS_ATT_ERR(BLE_ATT_ERR_ATTR_NOT_LONG):
                NIMBLE_LOGI(LOG_TAG, "Attribute not long");
                rc = 0;
                break;
            case BLE_HS_ATT_ERR(BLE_ATT_ERR_INSUFFICIENT_AUTHEN):
            case BLE_HS_ATT_ERR(BLE_ATT_ERR_INSUFFICIENT_AUTHOR):
            case BLE_HS_ATT_ERR(BLE_ATT_ERR_INSUFFICIENT_ENC):
                if (retryCount && pClient->secureConnection())
                    break;
            /* Else falls through. */
            default:
                NIMBLE_SEMAPHORE_DELETE(m_pSemaphore)
                return "";
        }
    } while(rc != 0 && retryCount--);

    NIMBLE_SEMAPHORE_DELETE(m_pSemaphore)
    NIMBLE_LOGD(LOG_TAG, "<< readValue(): length: %d", m_value.length());
    return m_value;
} // readValue


/**
 * @brief Callback for characteristic read operation.
 * @return 0 or error code.
 */
int NimBLERemoteCharacteristic::onReadCB(uint16_t conn_handle,
                const struct ble_gatt_error *error,
                struct ble_gatt_attr *attr, void *arg)
{
    NimBLERemoteCharacteristic* characteristic = (NimBLERemoteCharacteristic*)arg;
    uint16_t conn_id = characteristic->getRemoteService()->getClient()->getConnId();

    // Make sure the read is for this client
    if(conn_id != conn_handle) {
        return 0;
    }
    NIMBLE_LOGI(LOG_TAG, "Read complete; status=%d conn_handle=%d", error->status, conn_handle);

    if(error->status == 0) {
        if(attr) {
            NIMBLE_LOGD(LOG_TAG, "Got %d bytes", attr->om->om_len);

            characteristic->m_value += std::string((char*) attr->om->om_data, attr->om->om_len);
            return 0;
        }
    }
    // Read complete release semaphore and let the app can continue.
    NIMBLE_SEMAPHORE_GIVE(characteristic->m_pSemaphore, error->status)
    return 0;
}


/**
 * @brief Register for notifications.
 * @param [in] notifyCallback A callback to be invoked for a notification.  If NULL is provided then we are
 * unregistering for notifications.
 * @param [in] bool if true, register for notifications, false register for indications.
 * @param [in] bool if true, require a write response from the descriptor write operation.
 * @return true if successful.
 */
bool NimBLERemoteCharacteristic::registerForNotify(notify_callback notifyCallback, bool notifications, bool response) {
    NIMBLE_LOGD(LOG_TAG, ">> registerForNotify(): %s", toString().c_str());

    m_notifyCallback = notifyCallback;   // Save the notification callback.

    uint8_t val[] = {0x01, 0x00};

    NimBLERemoteDescriptor* desc = getDescriptor(NimBLEUUID((uint16_t)0x2902));
    if(desc == nullptr)
        return false;

    if(notifyCallback != nullptr){
        if(!notifications){
            val[0] = 0x02;
        }
    }

    else if (notifyCallback == nullptr){
        val[0] = 0x00;
    }

    NIMBLE_LOGD(LOG_TAG, "<< registerForNotify()");

    return desc->writeValue(val, 2, response);
} // registerForNotify


/**
 * @brief Delete the descriptors in the descriptor vector.
 * We maintain a vector called m_descriptorVector that contains pointers to BLERemoteDescriptors
 * object references. Since we allocated these in this class, we are also responsible for deleteing
 * them. This method does just that.
 * @return N/A.
 */
void NimBLERemoteCharacteristic::removeDescriptors() {
    // Iterate through all the descriptors releasing their storage and erasing them from the vector.
    for(auto &it: m_descriptorVector) {
        delete it;
    }
    m_descriptorVector.clear();
} // removeCharacteristics


/**
 * @brief Convert a BLERemoteCharacteristic to a string representation;
 * @return a String representation.
 */
std::string NimBLERemoteCharacteristic::toString() {
    std::string res = "Characteristic: uuid: " + m_uuid.toString();
    char val[6];
    res += ", handle: ";
    snprintf(val, sizeof(val), "%d", getHandle());
    res += val;
    res += " 0x";
    snprintf(val, sizeof(val), "%04x", getHandle());
    res += val;
    res += ", props: ";
    res += " 0x";
    snprintf(val, sizeof(val), "%02x", m_charProp);
    res += val;

    for(auto &it: m_descriptorVector) {
        res += "\n" + it->toString();
    }

    return res;
} // toString


/**
 * @brief Write the new value for the characteristic.
 * @param [in] newValue The new value to write.
 * @param [in] response Do we expect a response?
 * @return false if not connected or cant perform write for some reason.
 */
bool NimBLERemoteCharacteristic::writeValue(const std::string &newValue, bool response) {
    return writeValue((uint8_t*)newValue.c_str(), strlen(newValue.c_str()), response);
} // writeValue


/**
 * @brief Write the new value for the characteristic.
 *
 * This is a convenience function.  Many BLE characteristics are a single byte of data.
 * @param [in] newValue The new byte value to write.
 * @param [in] response Whether we require a response from the write.
 * @return false if not connected or cant perform write for some reason.
 */
bool NimBLERemoteCharacteristic::writeValue(uint8_t newValue, bool response) {
    return writeValue(&newValue, 1, response);
} // writeValue


/**
 * @brief Write the new value for the characteristic from a data buffer.
 * @param [in] data A pointer to a data buffer.
 * @param [in] length The length of the data in the data buffer.
 * @param [in] response Whether we require a response from the write.
 * @return false if not connected or cant perform write for some reason.
 */
bool NimBLERemoteCharacteristic::writeValue(const uint8_t* data, size_t length, bool response) {

    NIMBLE_LOGD(LOG_TAG, ">> writeValue(), length: %d", length);

    NimBLEClient* pClient = getRemoteService()->getClient();
    int rc = 0;
    int retryCount = 1;
    uint16_t mtu;

    // Check to see that we are connected.
    if (!pClient->isConnected()) {
        NIMBLE_LOGE(LOG_TAG, "Disconnected");
        return false;
    }

    mtu = ble_att_mtu(pClient->getConnId()) - 3;

    // Check if the data length is longer than we can write in 1 connection event.
    // If so we must do a long write which requires a response.
    if(length <= mtu && !response) {
        rc =  ble_gattc_write_no_rsp_flat(pClient->getConnId(), m_handle, data, length);
        return (rc==0);
    }

    do {
        NIMBLE_SEMAPHORE_TAKE(m_pSemaphore, "Write Value")

        if(length > mtu) {
            NIMBLE_LOGI(LOG_TAG,"long write %d bytes", length);
            os_mbuf *om = ble_hs_mbuf_from_flat(data, length);
            rc = ble_gattc_write_long(pClient->getConnId(), m_handle, 0, om,
                                      NimBLERemoteCharacteristic::onWriteCB,
                                      this);
        } else {
            rc = ble_gattc_write_flat(pClient->getConnId(), m_handle,
                                      data, length,
                                      NimBLERemoteCharacteristic::onWriteCB,
                                      this);
        }
        if (rc != 0) {
            NIMBLE_LOGE(LOG_TAG, "Error: Failed to write characteristic; rc=%d", rc);
            NIMBLE_SEMAPHORE_DELETE(m_pSemaphore)
            return false;
        }

        rc = NIMBLE_SEMAPHORE_WAIT(m_pSemaphore)

        switch(rc){
            case 0:
            case BLE_HS_EDONE:
                rc = 0;
                break;
            case BLE_HS_ATT_ERR(BLE_ATT_ERR_ATTR_NOT_LONG):
                NIMBLE_LOGE(LOG_TAG, "Long write not supported by peer; Truncating length to %d", mtu);
                retryCount++;
                length = mtu;
                break;

            case BLE_HS_ATT_ERR(BLE_ATT_ERR_INSUFFICIENT_AUTHEN):
            case BLE_HS_ATT_ERR(BLE_ATT_ERR_INSUFFICIENT_AUTHOR):
            case BLE_HS_ATT_ERR(BLE_ATT_ERR_INSUFFICIENT_ENC):
                if (retryCount && pClient->secureConnection())
                    break;
            /* Else falls through. */
            default:
                NIMBLE_SEMAPHORE_DELETE(m_pSemaphore)
                return false;
        }
    } while(rc != 0 && retryCount--);

    NIMBLE_SEMAPHORE_DELETE(m_pSemaphore)
    NIMBLE_LOGD(LOG_TAG, "<< writeValue, rc: %d",rc);
    return (rc == 0);
} // writeValue


/**
 * @brief Callback for characteristic write operation.
 * @return 0 or error code.
 */
int NimBLERemoteCharacteristic::onWriteCB(uint16_t conn_handle,
                const struct ble_gatt_error *error,
                struct ble_gatt_attr *attr, void *arg)
{
    NimBLERemoteCharacteristic* characteristic = (NimBLERemoteCharacteristic*)arg;

    // Make sure the discovery is for this device
    if(characteristic->getRemoteService()->getClient()->getConnId() != conn_handle){
        return 0;
    }

    NIMBLE_LOGI(LOG_TAG, "Write complete; status=%d conn_handle=%d", error->status, conn_handle);
    NIMBLE_SEMAPHORE_GIVE(characteristic->m_pSemaphore, error->status)

    return 0;
}


/**
 * @brief Read raw data from remote characteristic as hex bytes
 * @return uint8_t pointer to the data read.
 */
uint8_t* NimBLERemoteCharacteristic::readRawData() {
    if(m_rawData != nullptr) {
        free(m_rawData);
        m_rawData = nullptr;
    }

    m_dataLen = m_value.length();
    // If we have data copy it to rawData
    if(m_dataLen) {
        m_rawData = (uint8_t*) calloc(m_dataLen, sizeof(uint8_t));
        memcpy(m_rawData, m_value.data(), m_dataLen);
    }

    return m_rawData;
}


/**
 * @brief Get the length of the data read from the remote characteristic.
 * @return size_t length of the data in bytes.
 */
size_t NimBLERemoteCharacteristic::getDataLength() {
    return m_value.length();
}


#endif // #if defined( CONFIG_BT_NIMBLE_ROLE_CENTRAL)
#endif /* CONFIG_BT_ENABLED */
