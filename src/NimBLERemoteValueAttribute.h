/*
 * NimBLERemoteValueAttribute.h
 *
 *  Created: on July 28 2024
 *      Author H2zero
 */

#ifndef NIMBLE_CPP_REMOTE_VALUE_ATTRIBUTE_H_
#define NIMBLE_CPP_REMOTE_VALUE_ATTRIBUTE_H_

#include "nimconfig.h"
#if defined(CONFIG_BT_ENABLED) && defined(CONFIG_BT_NIMBLE_ROLE_CENTRAL)

# if defined(CONFIG_NIMBLE_CPP_IDF)
#  include <host/ble_gatt.h>
# else
#  include <nimble/nimble/host/include/host/ble_gatt.h>
# endif

/****  FIX COMPILATION ****/
# undef min
# undef max
/**************************/

# include "NimBLEAttribute.h"
# include "NimBLEAttValue.h"

class NimBLEClient;

class NimBLERemoteValueAttribute : public NimBLEAttribute {
  public:
    /**
     * @brief Read the value of the remote attribute.
     * @param [in] timestamp A pointer to a time_t struct to store the time the value was read.
     * @return The value of the remote attribute.
     */
    NimBLEAttValue readValue(time_t* timestamp = nullptr) const;

    /**
     * @brief Get the length of the remote attribute value.
     * @return The length of the remote attribute value.
     */
    size_t getLength() const { return m_value.size(); }

    /**
     * @brief Get the value of the remote attribute.
     * @return The value of the remote attribute.
     * @details This returns a copy of the value to avoid potential race conditions.
     */
    NimBLEAttValue getValue() const { return m_value; }

    /**
     * Get the client instance that owns this attribute.
     */
    virtual NimBLEClient* getClient() const = 0;

    /**
     * @brief Write a new value to the remote characteristic from a data buffer.
     * @param [in] data A pointer to a data buffer.
     * @param [in] length The length of the data in the data buffer.
     * @param [in] response Whether we require a response from the write.
     * @return false if not connected or otherwise cannot perform write.
     */
    bool writeValue(const uint8_t* data, size_t length, bool response = false) const;

    /**
     * @brief Write a new value to the remote characteristic from a std::vector<uint8_t>.
     * @param [in] vec A std::vector<uint8_t> value to write to the remote characteristic.
     * @param [in] response Whether we require a response from the write.
     * @return false if not connected or otherwise cannot perform write.
     */
    bool writeValue(const std::vector<uint8_t>& v, bool response = false) const {
        return writeValue(&v[0], v.size(), response);
    }

    /**
     * @brief Write a new value to the remote characteristic from a const char*.
     * @param [in] str A character string to write to the remote characteristic.
     * @param [in] length (optional) The length of the character string, uses strlen if omitted.
     * @param [in] response Whether we require a response from the write.
     * @return false if not connected or otherwise cannot perform write.
     */
    bool writeValue(const char* str, size_t length = 0, bool response = false) const {
        return writeValue(reinterpret_cast<const uint8_t*>(str), length ? length : strlen(str), response);
    }

    /**
     * @brief Template to set the remote characteristic value to <type\>val.
     * @param [in] s The value to write.
     * @param [in] response True == request write response.
     * @details Only used for non-arrays and types without a `c_str()` method.
     */
    template <typename T>
# ifdef _DOXYGEN_
    bool
# else
    typename std::enable_if<!std::is_array<T>::value && !Has_c_str_len<T>::value, bool>::type
# endif
    writeValue(const T& v, bool response = false) const {
        return writeValue(reinterpret_cast<const uint8_t*>(&v), sizeof(T), response);
    }

    /**
     * @brief Template to set the remote characteristic value to <type\>val.
     * @param [in] s The value to write.
     * @param [in] response True == request write response.
     * @details Only used if the <type\> has a `c_str()` method.
     */
    template <typename T>
# ifdef _DOXYGEN_
    bool
# else
    typename std::enable_if<Has_c_str_len<T>::value, bool>::type
# endif
    writeValue(const T& s, bool response = false) const {
        return writeValue(reinterpret_cast<const uint8_t*>(s.c_str()), s.length(), response);
    }

    /**
     * @brief Template to convert the remote characteristic data to <type\>.
     * @tparam T The type to convert the data to.
     * @param [in] timestamp A pointer to a time_t struct to store the time the value was read.
     * @param [in] skipSizeCheck If true it will skip checking if the data size is less than <tt>sizeof(<type\>)</tt>.
     * @return The data converted to <type\> or NULL if skipSizeCheck is false and the data is
     * less than <tt>sizeof(<type\>)</tt>.
     * @details <b>Use:</b> <tt>getValue<type>(&timestamp, skipSizeCheck);</tt>
     */
    template <typename T>
    T getValue(time_t* timestamp = nullptr, bool skipSizeCheck = false) const {
        return m_value.getValue<T>(timestamp, skipSizeCheck);
    }

    /**
     * @brief Template to convert the remote characteristic data to <type\>.
     * @tparam T The type to convert the data to.
     * @param [in] timestamp A pointer to a time_t struct to store the time the value was read.
     * @param [in] skipSizeCheck If true it will skip checking if the data size is less than <tt>sizeof(<type\>)</tt>.
     * @return The data converted to <type\> or NULL if skipSizeCheck is false and the data is
     * less than <tt>sizeof(<type\>)</tt>.
     * @details <b>Use:</b> <tt>readValue<type>(&timestamp, skipSizeCheck);</tt>
     */
    template <typename T>
    T readValue(time_t* timestamp = nullptr, bool skipSizeCheck = false) const {
        readValue();
        return m_value.getValue<T>(timestamp, skipSizeCheck);
    }

  protected:
    /**
     * @brief Construct a new NimBLERemoteValueAttribute object.
     */
    NimBLERemoteValueAttribute(const ble_uuid_any_t& uuid, uint16_t handle) : NimBLEAttribute(uuid, handle) {}

    /**
     * @brief Destroy the NimBLERemoteValueAttribute object.
     */
    virtual ~NimBLERemoteValueAttribute() = default;

    static int onReadCB(uint16_t conn_handle, const ble_gatt_error* error, ble_gatt_attr* attr, void* arg);
    static int onWriteCB(uint16_t conn_handle, const ble_gatt_error* error, ble_gatt_attr* attr, void* arg);

    mutable NimBLEAttValue m_value{};
};

#endif /* CONFIG_BT_ENABLED && CONFIG_BT_NIMBLE_ROLE_CENTRAL */
#endif // NIMBLE_CPP_REMOTE_VALUE_ATTRIBUTE_H_
