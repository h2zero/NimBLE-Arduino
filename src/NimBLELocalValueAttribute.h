/*
 * NimBLELocalValueAttribute.cpp
 *
 *  Created: on July 28 2024
 *      Author H2zero
 */

#ifndef NIMBLE_LOCAL_VALUE_ATTRIBUTE_H_
#define NIMBLE_LOCAL_VALUE_ATTRIBUTE_H_

#include "nimconfig.h"
#if defined(CONFIG_BT_ENABLED) && defined(CONFIG_BT_NIMBLE_ROLE_PERIPHERAL)

# if defined(CONFIG_NIMBLE_CPP_IDF)
#  include "host/ble_hs.h"
# else
#  include "nimble/nimble/host/include/host/ble_hs.h"
# endif

typedef enum {
    READ         = BLE_GATT_CHR_F_READ,
    READ_ENC     = BLE_GATT_CHR_F_READ_ENC,
    READ_AUTHEN  = BLE_GATT_CHR_F_READ_AUTHEN,
    READ_AUTHOR  = BLE_GATT_CHR_F_READ_AUTHOR,
    WRITE        = BLE_GATT_CHR_F_WRITE,
    WRITE_NR     = BLE_GATT_CHR_F_WRITE_NO_RSP,
    WRITE_ENC    = BLE_GATT_CHR_F_WRITE_ENC,
    WRITE_AUTHEN = BLE_GATT_CHR_F_WRITE_AUTHEN,
    WRITE_AUTHOR = BLE_GATT_CHR_F_WRITE_AUTHOR,
    BROADCAST    = BLE_GATT_CHR_F_BROADCAST,
    NOTIFY       = BLE_GATT_CHR_F_NOTIFY,
    INDICATE     = BLE_GATT_CHR_F_INDICATE
} NIMBLE_PROPERTY;

# include "NimBLELocalAttribute.h"
# include "NimBLEAttValue.h"
# include <vector>
class NimBLEConnInfo;

class NimBLELocalValueAttribute : public NimBLELocalAttribute {
  public:
    /**
     * @brief Get the properties of the attribute.
     */
    uint16_t getProperties() const { return m_properties; }

    /**
     * @brief Get the length of the attribute value.
     * @return The length of the attribute value.
     */
    size_t getLength() const { return m_value.size(); }

    /**
     * @brief Get a copy of the value of the attribute value.
     * @param [in] timestamp (Optional) A pointer to a time_t struct to get the time the value set.
     * @return A copy of the attribute value.
     */
    NimBLEAttValue getValue(time_t* timestamp = nullptr) const { return m_value; }

    /**
     * @brief Set the value of the attribute value.
     * @param [in] data The data to set the value to.
     * @param [in] size The size of the data.
     */
    void setValue(const uint8_t* data, size_t size) { m_value.setValue(data, size); }

    /**
     * @brief Set the value of the attribute value.
     * @param [in] str The string to set the value to.
     */
    void setValue(const char* str) { m_value.setValue(str); }

    /**
     * @brief Set the value of the attribute value.
     * @param [in] vec The vector to set the value to.
     */
    void setValue(const std::vector<uint8_t>& vec) { m_value.setValue(vec); }

    /**
     * @brief Template to set the value to <type\>val.
     * @param [in] val The value to set.
     */
    template <typename T>
    void setValue(const T& val) {
        m_value.setValue<T>(val);
    }

    /**
     * @brief Template to convert the data to <type\>.
     * @tparam T The type to convert the data to.
     * @param [in] timestamp (Optional) A pointer to a time_t struct to get the time the value set.
     * @param [in] skipSizeCheck (Optional) If true it will skip checking if the data size is less than <tt>sizeof(<type\>)</tt>.
     * @return The data converted to <type\> or NULL if skipSizeCheck is false and the data is less than <tt>sizeof(<type\>)</tt>.
     * @details <b>Use:</b> <tt>getValue<type>(&timestamp, skipSizeCheck);</tt>
     */
    template <typename T>
    T getValue(time_t* timestamp = nullptr, bool skipSizeCheck = false) const {
        return m_value.getValue<T>(timestamp, skipSizeCheck);
    }

  protected:
    friend class NimBLEServer;

    /**
     * @brief Construct a new NimBLELocalValueAttribute object.
     * @param [in] uuid The UUID of the attribute.
     * @param [in] handle The handle of the attribute.
     * @param [in] maxLen The maximum length of the attribute value.
     * @param [in] initLen The initial length of the attribute value.
     */
    NimBLELocalValueAttribute(const NimBLEUUID& uuid,
                              uint16_t          handle,
                              uint16_t          maxLen,
                              uint16_t          initLen = CONFIG_NIMBLE_CPP_ATT_VALUE_INIT_LENGTH)
        : NimBLELocalAttribute(uuid, handle), m_value(initLen, maxLen) {}

    /**
     * @brief Destroy the NimBLELocalValueAttribute object.
     */
    virtual ~NimBLELocalValueAttribute() = default;

    /**
     * @brief Callback function to support a read request.
     * @param [in] connInfo A reference to a NimBLEConnInfo instance containing the peer info.
     * @details This function is called by NimBLEServer when a read request is received.
     */
    virtual void readEvent(NimBLEConnInfo& connInfo) = 0;

    /**
     * @brief Callback function to support a write request.
     * @param [in] val The value to write.
     * @param [in] len The length of the value.
     * @param [in] connInfo A reference to a NimBLEConnInfo instance containing the peer info.
     * @details This function is called by NimBLEServer when a write request is received.
     */
    virtual void writeEvent(const uint8_t* val, uint16_t len, NimBLEConnInfo& connInfo) = 0;

    /**
     * @brief Get a pointer to value of the attribute.
     * @return A pointer to the value of the attribute.
     * @details This function is used by NimBLEServer when handling read/write requests.
     */
    const NimBLEAttValue& getAttVal() const { return m_value; }

    /**
     * @brief Set the properties of the attribute.
     * @param [in] properties The properties of the attribute.
     */
    void setProperties(uint16_t properties) { m_properties = properties; }

    NimBLEAttValue m_value{};
    uint16_t       m_properties{0};
};

#endif // CONFIG_BT_ENABLED && CONFIG_BT_NIMBLE_ROLE_PERIPHERAL
#endif // NIMBLE_LOCAL_VALUE_ATTRIBUTE_H_
