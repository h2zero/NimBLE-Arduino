/*
 * NimBLEAttValue.h
 *
 *  Created: on March 18, 2021
 *      Author H2zero
 *
 */

#ifndef NIMBLE_CPP_ATTVALUE_H
#define NIMBLE_CPP_ATTVALUE_H
#include "nimconfig.h"
#if defined(CONFIG_BT_ENABLED)

# ifdef NIMBLE_CPP_ARDUINO_STRING_AVAILABLE
#  include <Arduino.h>
# endif

# include "NimBLELog.h"
# include <string>
# include <vector>
# include <ctime>
# include <cstring>
# include <cstdint>

# ifndef CONFIG_NIMBLE_CPP_ATT_VALUE_TIMESTAMP_ENABLED
#  define CONFIG_NIMBLE_CPP_ATT_VALUE_TIMESTAMP_ENABLED 0
# endif

# ifndef BLE_ATT_ATTR_MAX_LEN
#  define BLE_ATT_ATTR_MAX_LEN 512
# endif

# if !defined(CONFIG_NIMBLE_CPP_ATT_VALUE_INIT_LENGTH)
#  define CONFIG_NIMBLE_CPP_ATT_VALUE_INIT_LENGTH 20
# elif CONFIG_NIMBLE_CPP_ATT_VALUE_INIT_LENGTH > BLE_ATT_ATTR_MAX_LEN
#  error CONFIG_NIMBLE_CPP_ATT_VALUE_INIT_LENGTH cannot be larger than 512 (BLE_ATT_ATTR_MAX_LEN)
# elif CONFIG_NIMBLE_CPP_ATT_VALUE_INIT_LENGTH < 1
#  error CONFIG_NIMBLE_CPP_ATT_VALUE_INIT_LENGTH cannot be less than 1; Range = 1 : 512
# endif

/* Used to determine if the type passed to a template has a c_str() and length() method. */
template <typename T, typename = void, typename = void>
struct Has_c_str_len : std::false_type {};

template <typename T>
struct Has_c_str_len<T, decltype(void(std::declval<T&>().c_str())), decltype(void(std::declval<T&>().length()))>
    : std::true_type {};

/**
 * @brief A specialized container class to hold BLE attribute values.
 * @details This class is designed to be more memory efficient than using\n
 * standard container types for value storage, while being convertible to\n
 * many different container classes.
 */
class NimBLEAttValue {
    uint8_t* m_attr_value{};
    uint16_t m_attr_max_len{};
    uint16_t m_attr_len{};
    uint16_t m_capacity{};
# if CONFIG_NIMBLE_CPP_ATT_VALUE_TIMESTAMP_ENABLED
    time_t m_timestamp{};
# endif
    void deepCopy(const NimBLEAttValue& source);

  public:
    /**
     * @brief Default constructor.
     * @param[in] init_len The initial size in bytes.
     * @param[in] max_len The max size in bytes that the value can be.
     */
    NimBLEAttValue(uint16_t init_len = CONFIG_NIMBLE_CPP_ATT_VALUE_INIT_LENGTH, uint16_t max_len = BLE_ATT_ATTR_MAX_LEN);

    /**
     * @brief Construct with an initial value from a buffer.
     * @param value A pointer to the initial value to set.
     * @param[in] len The size in bytes of the value to set.
     * @param[in] max_len The max size in bytes that the value can be.
     */
    NimBLEAttValue(const uint8_t *value, uint16_t len,
                   uint16_t max_len = BLE_ATT_ATTR_MAX_LEN);

    /**
     * @brief Construct with an initial value from a const char string.
     * @param value A pointer to the initial value to set.
     * @param[in] max_len The max size in bytes that the value can be.
     */
    NimBLEAttValue(const char *value, uint16_t max_len = BLE_ATT_ATTR_MAX_LEN)
                   :NimBLEAttValue((uint8_t*)value, (uint16_t)strlen(value), max_len){}

    /**
     * @brief Construct with an initializer list.
     * @param list An initializer list containing the initial value to set.
     * @param[in] max_len The max size in bytes that the value can be.
     */
    NimBLEAttValue(std::initializer_list<uint8_t> list, uint16_t max_len = BLE_ATT_ATTR_MAX_LEN)
        : NimBLEAttValue(list.begin(), list.size(), max_len) {}

    /**
     * @brief Construct with an initial value from a std::string.
     * @param str A std::string containing to the initial value to set.
     * @param[in] max_len The max size in bytes that the value can be.
     */
    NimBLEAttValue(const std::string str, uint16_t max_len = BLE_ATT_ATTR_MAX_LEN)
        : NimBLEAttValue(reinterpret_cast<const uint8_t*>(&str[0]), str.length(), max_len) {}

    /**
     * @brief Construct with an initial value from a std::vector<uint8_t>.
     * @param vec A std::vector<uint8_t> containing to the initial value to set.
     * @param[in] max_len The max size in bytes that the value can be.
     */
    NimBLEAttValue(const std::vector<uint8_t> vec, uint16_t max_len = BLE_ATT_ATTR_MAX_LEN)
        : NimBLEAttValue(&vec[0], vec.size(), max_len) {}

# ifdef NIMBLE_CPP_ARDUINO_STRING_AVAILABLE
    /**
     * @brief Construct with an initial value from an Arduino String.
     * @param str An Arduino String containing to the initial value to set.
     * @param[in] max_len The max size in bytes that the value can be.
     */
    NimBLEAttValue(const String str, uint16_t max_len = BLE_ATT_ATTR_MAX_LEN)
        : NimBLEAttValue(reinterpret_cast<const uint8_t*>(str.c_str()), str.length(), max_len) {}
# endif

    /** @brief Copy constructor */
    NimBLEAttValue(const NimBLEAttValue& source) { deepCopy(source); }

    /** @brief Move constructor */
    NimBLEAttValue(NimBLEAttValue&& source) { *this = std::move(source); }

    /** @brief Destructor */
    ~NimBLEAttValue();

    /** @brief Returns the max size in bytes */
    uint16_t max_size() const { return m_attr_max_len; }

    /** @brief Returns the currently allocated capacity in bytes */
    uint16_t capacity() const { return m_capacity; }

    /** @brief Returns the current length of the value in bytes */
    uint16_t length() const { return m_attr_len; }

    /** @brief Returns the current size of the value in bytes */
    uint16_t size() const { return m_attr_len; }

    /** @brief Returns a pointer to the internal buffer of the value */
    const uint8_t* data() const { return m_attr_value; }

    /** @brief Returns a pointer to the internal buffer of the value as a const char* */
    const char* c_str() const { return reinterpret_cast<const char*>(m_attr_value); }

    /** @brief Iterator begin */
    const uint8_t* begin() const { return m_attr_value; }

    /** @brief Iterator end */
    const uint8_t* end() const { return m_attr_value + m_attr_len; }

# if CONFIG_NIMBLE_CPP_ATT_VALUE_TIMESTAMP_ENABLED
    /** @brief Returns a timestamp of when the value was last updated */
    time_t getTimeStamp() const { return m_timestamp; }

    /** @brief Set the timestamp to the current time */
    void setTimeStamp() { m_timestamp = time(nullptr); }

    /**
     * @brief Set the timestamp to the specified time
     * @param[in] t The timestamp value to set
     */
    void setTimeStamp(time_t t) { m_timestamp = t; }
# else
    time_t getTimeStamp() const { return 0; }
    void   setTimeStamp() {}
    void   setTimeStamp(time_t t) {}
# endif

    /**
     * @brief Set the value from a buffer
     * @param[in] value A pointer to a buffer containing the value.
     * @param[in] len The length of the value in bytes.
     * @returns True if successful.
     */
    bool setValue(const uint8_t* value, uint16_t len);

    /**
     * @brief Set value to the value of const char*.
     * @param [in] s A pointer to a const char value to set.
     * @param [in] len The length of the value in bytes, defaults to strlen(s).
     */
    bool setValue(const char* s, uint16_t len = 0) {
        if (len == 0) {
            len = strlen(s);
        }
        return setValue(reinterpret_cast<const uint8_t*>(s), len);
    }

    const NimBLEAttValue& getValue(time_t* timestamp = nullptr) const {
        if (timestamp != nullptr) {
# if CONFIG_NIMBLE_CPP_ATT_VALUE_TIMESTAMP_ENABLED
            *timestamp = m_timestamp;
# else
            *timestamp = 0;
# endif
        }
        return *this;
    }

    /**
     * @brief Append data to the value.
     * @param[in] value A ponter to a data buffer with the value to append.
     * @param[in] len The length of the value to append in bytes.
     * @returns A reference to the appended NimBLEAttValue.
     */
    NimBLEAttValue& append(const uint8_t* value, uint16_t len);

    /*********************** Template Functions ************************/

    /**
     * @brief Template to set value to the value of <type\>val.
     * @param [in] s The <type\>value to set.
     * @param [in] len The length of the value in bytes, defaults to sizeof(T).
     * @details Only used for types without a `c_str()` method.
     */
    template <typename T>
# ifdef _DOXYGEN_
    bool
# else
    typename std::enable_if<!Has_c_str_len<T>::value, bool>::type
# endif
    setValue(const T& s, uint16_t len = 0) {
        if (len == 0) {
            len = sizeof(T);
        }
        return setValue(reinterpret_cast<const uint8_t*>(&s), len);
    }

    /**
     * @brief Template to set value to the value of <type\>val.
     * @param [in] s The <type\>value to set.
     * @param [in] len The length of the value in bytes, defaults to string.length().
     * @details Only used if the <type\> has a `c_str()` method.
     */
    template <typename T>
# ifdef _DOXYGEN_
    bool
# else
    typename std::enable_if<Has_c_str_len<T>::value, bool>::type
# endif
    setValue(const T& s, uint16_t len = 0) {
        if (len == 0) {
            len = s.length();
        }
        return setValue(reinterpret_cast<const uint8_t*>(s.c_str()), len);
    }

    /**
     * @brief Template to return the value as a <type\>.
     * @tparam T The type to convert the data to.
     * @param [in] timestamp A pointer to a time_t struct to store the time the value was read.
     * @param [in] skipSizeCheck If true it will skip checking if the data size is less than\n
     * <tt>sizeof(<type\>)</tt>.
     * @return The data converted to <type\> or NULL if skipSizeCheck is false and the data is\n
     * less than <tt>sizeof(<type\>)</tt>.
     * @details <b>Use:</b> <tt>getValue<type>(&timestamp, skipSizeCheck);</tt>
     */
    template <typename T>
    T getValue(time_t* timestamp = nullptr, bool skipSizeCheck = false) const {
        if (!skipSizeCheck && size() < sizeof(T)) {
            return T();
        }
        if (timestamp != nullptr) {
# if CONFIG_NIMBLE_CPP_ATT_VALUE_TIMESTAMP_ENABLED
            *timestamp = m_timestamp;
# else
            *timestamp = 0;
# endif
        }

        return *(reinterpret_cast<const T*>(m_attr_value));
    }

    /*********************** Operators ************************/

    /** @brief Subscript operator */
    uint8_t operator[](int pos) const {
        NIMBLE_CPP_DEBUG_ASSERT(pos < m_attr_len);
        return m_attr_value[pos];
    }

    /** @brief Operator; Get the value as a std::vector<uint8_t>. */
    operator std::vector<uint8_t>() const { return std::vector<uint8_t>(m_attr_value, m_attr_value + m_attr_len); }

    /** @brief Operator; Get the value as a std::string. */
    operator std::string() const { return std::string(reinterpret_cast<char*>(m_attr_value), m_attr_len); }

    /** @brief Operator; Get the value as a const uint8_t*. */
    operator const uint8_t*() const { return m_attr_value; }

    /** @brief Operator; Append another NimBLEAttValue. */
    NimBLEAttValue& operator+=(const NimBLEAttValue& source) { return append(source.data(), source.size()); }

    /** @brief Operator; Set the value from a std::string source. */
    NimBLEAttValue& operator=(const std::string& source) {
        setValue(reinterpret_cast<const uint8_t*>(&source[0]), source.size());
        return *this;
    }

    /** @brief Move assignment operator */
    NimBLEAttValue& operator=(NimBLEAttValue&& source);

    /** @brief Copy assignment operator */
    NimBLEAttValue& operator=(const NimBLEAttValue& source);

    /** @brief Equality operator */
    bool operator==(const NimBLEAttValue& source) const {
        return (m_attr_len == source.size()) ? memcmp(m_attr_value, source.data(), m_attr_len) == 0 : false;
    }

    /** @brief Inequality operator */
    bool operator!=(const NimBLEAttValue& source) const { return !(*this == source); }

# ifdef NIMBLE_CPP_ARDUINO_STRING_AVAILABLE
    /** @brief Operator; Get the value as an Arduino String value. */
    operator String() const { return String(reinterpret_cast<char*>(m_attr_value)); }
# endif
};

#endif /*(CONFIG_BT_ENABLED) */
#endif /* NIMBLE_CPP_ATTVALUE_H_ */
