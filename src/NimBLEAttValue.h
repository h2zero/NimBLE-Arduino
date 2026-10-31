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

#ifndef NIMBLE_CPP_ATTVALUE_H
#define NIMBLE_CPP_ATTVALUE_H

#include "syscfg/syscfg.h"
#if CONFIG_BT_NIMBLE_ENABLED

# ifdef USING_NIMBLE_ARDUINO_HEADERS
#  include "nimble/nimble/include/nimble/nimble_npl.h"
# else
#  include "nimble/nimble_npl.h"
# endif

/* Enables the use of Arduino String class for attribute values */
# ifndef NIMBLE_CPP_ARDUINO_STRING_AVAILABLE
#  define NIMBLE_CPP_ARDUINO_STRING_AVAILABLE (__has_include(<Arduino.h>))
# endif

# if NIMBLE_CPP_ARDUINO_STRING_AVAILABLE
#  include <WString.h>
# endif

# include <string>
# include <string_view>
# include <span>
# include <concepts>
# include <type_traits>
# include <vector>
# include <ctime>
# include <cstring>
# include <cstdint>
# include <atomic>

# ifndef MYNEWT_VAL_NIMBLE_CPP_ATT_VALUE_TIMESTAMP_ENABLED
#  ifndef CONFIG_NIMBLE_CPP_ATT_VALUE_TIMESTAMP_ENABLED
#   define MYNEWT_VAL_NIMBLE_CPP_ATT_VALUE_TIMESTAMP_ENABLED 0
#  else
#   define MYNEWT_VAL_NIMBLE_CPP_ATT_VALUE_TIMESTAMP_ENABLED CONFIG_NIMBLE_CPP_ATT_VALUE_TIMESTAMP_ENABLED
#  endif
# endif

# ifndef BLE_ATT_ATTR_MAX_LEN
#  define BLE_ATT_ATTR_MAX_LEN 512
# endif

# ifndef MYNEWT_VAL_NIMBLE_CPP_ATT_VALUE_INIT_LENGTH
#  ifndef CONFIG_NIMBLE_CPP_ATT_VALUE_INIT_LENGTH
#   define MYNEWT_VAL_NIMBLE_CPP_ATT_VALUE_INIT_LENGTH 20
#  else
#   define MYNEWT_VAL_NIMBLE_CPP_ATT_VALUE_INIT_LENGTH CONFIG_NIMBLE_CPP_ATT_VALUE_INIT_LENGTH
#  endif
# endif

# if MYNEWT_VAL(NIMBLE_CPP_ATT_VALUE_INIT_LENGTH) > BLE_ATT_ATTR_MAX_LEN
#  error NIMBLE_CPP_ATT_VALUE_INIT_LENGTH cannot be larger than 512 (BLE_ATT_ATTR_MAX_LEN)
# elif MYNEWT_VAL(NIMBLE_CPP_ATT_VALUE_INIT_LENGTH) < 1
#  error NIMBLE_CPP_ATT_VALUE_INIT_LENGTH cannot be less than 1; Range = 1 : 512
# endif

/** @brief Satisfied by types with `data()` and `size()` methods (std::vector, std::array, std::string, std::span, etc.). */
template <typename T>
concept NimBLEDataContainer = requires(const T& t) {
    t.data();
    t.size();
};

/** @brief Satisfied by types with `c_str()` and `length()` methods (Arduino String, etc.). */
template <typename T>
concept NimBLEStringLike = requires(const T& t) {
    t.c_str();
    t.length();
};

/** @brief Satisfied by arrays of char, e.g. string literals and char buffers. */
template <typename T>
concept NimBLECharArray = std::is_bounded_array_v<T> && std::same_as<std::remove_extent_t<T>, char>;

/** @brief Satisfied by types that can be sent/stored as raw bytes through the value templates. */
template <typename T>
concept NimBLEByteValue = !std::is_pointer_v<T>;

/**
 * @brief Get the bytes of a value as a span.
 * @details Containers with `data()` and `size()` are viewed element-wise (size is scaled by the element size),
 * types with `c_str()` and `length()` are viewed as characters, everything else is viewed as its object representation.
 * The returned span references the argument and must not outlive it.
 */
template <typename T>
std::span<const uint8_t> NimBLEAsBytes(const T& v) {
    if constexpr (NimBLEDataContainer<T>) {
        return {reinterpret_cast<const uint8_t*>(v.data()), v.size() * sizeof(*v.data())};
    } else if constexpr (NimBLEStringLike<T>) {
        return {reinterpret_cast<const uint8_t*>(v.c_str()), static_cast<size_t>(v.length())};
    } else {
        return {reinterpret_cast<const uint8_t*>(&v), sizeof(T)};
    }
}

/**
 * @brief A specialized container class to hold BLE attribute values.
 * @details This class is designed to be more memory efficient than using\n
 * standard container types for value storage, while being convertible to\n
 * many different container classes.
 */
class NimBLEAttValue {
    // Shared, never-freed buffer that m_attr_value points to when nothing is allocated (reset or allocation failure).
    // Non-const so it can be pointed at by a uint8_t*.
    static inline uint8_t EMPTY_VALUE{0};
    uint8_t*       m_attr_value{&EMPTY_VALUE};
    uint16_t m_attr_max_len{};
    uint16_t m_attr_len{};
    uint16_t m_capacity{};
# if MYNEWT_VAL(NIMBLE_CPP_ATT_VALUE_TIMESTAMP_ENABLED)
    time_t m_timestamp{};
# endif
    bool deepCopy(const NimBLEAttValue& source);

    // The heap buffer owned by this object, or nullptr if m_attr_value points at EMPTY_VALUE.
    uint8_t* ownedBuffer() const { return m_attr_value == &EMPTY_VALUE ? nullptr : m_attr_value; }

    // Clamp so a large container size can't wrap when narrowed to uint16_t; the constructor then rejects it as > max_len.
    static constexpr uint16_t clampLen(size_t len) {
        return len > UINT16_MAX ? UINT16_MAX : static_cast<uint16_t>(len);
    }

  public:
    class Lock {
        static inline ble_npl_mutex     m_mutex{};
        static inline std::atomic<bool> m_initialized{false};
        static inline std::atomic<int>  m_activeCount{0};
        bool                     m_locked{false};

      public:
        Lock() {
            // Increment active count to signal that a constructor is running
            m_activeCount.fetch_add(1);

            if (m_initialized.load()) {
                m_locked = (ble_npl_mutex_pend(&m_mutex, BLE_NPL_TIME_FOREVER) == BLE_NPL_OK);

                // Post-lock check: If deinit flipped the flag while we were waiting in line,
                // we must abort, give it back, and act un-locked.
                if (!m_initialized.load() && m_locked) {
                    ble_npl_mutex_release(&m_mutex);
                    m_locked = false;
                }
            }

            // Decrement active count to signal that the constructor has finished processing
            m_activeCount.fetch_sub(1);
        }

        ~Lock() {
            if (m_locked) {
                ble_npl_mutex_release(&m_mutex);
            }
        }

        static bool init() {
            if (!m_initialized.load()) {
                if (ble_npl_mutex_init(&m_mutex) == BLE_NPL_OK) {
                    m_initialized.store(true);
                }
            }
            return m_initialized.load();
        }

        static void deinit() {
            if (m_initialized.load()) {
                // Prevent any new instances from acquiring the lock while we deinitialize
                m_initialized.store(false);

                // Wait for any active users to finish
                while (m_activeCount.load() > 0) {
                    ble_npl_time_delay(1);
                }

                // Acquire the mutex to ensure no other threads are holding it before deinitializing.
                if (ble_npl_mutex_pend(&m_mutex, BLE_NPL_TIME_FOREVER) == BLE_NPL_OK) {
                    // Deinitialize the mutex now that we have exclusive ownership and no active users.
                    ble_npl_mutex_deinit(&m_mutex);
                }
            }
        }
    };

    /**
     * @brief Default constructor.
     * @param[in] init_len The initial size in bytes.
     * @param[in] max_len The max size in bytes that the value can be.
     */
    NimBLEAttValue(uint16_t init_len = MYNEWT_VAL(NIMBLE_CPP_ATT_VALUE_INIT_LENGTH),
                   uint16_t max_len  = BLE_ATT_ATTR_MAX_LEN);

    /**
     * @brief Construct with an initial value from a buffer.
     * @param value A pointer to the initial value to set.
     * @param[in] len The size in bytes of the value to set.
     * @param[in] max_len The max size in bytes that the value can be.
     */
    NimBLEAttValue(const uint8_t* value, uint16_t len, uint16_t max_len = BLE_ATT_ATTR_MAX_LEN);

    /**
     * @brief Construct with an initial value from a const char string.
     * @param value A pointer to the initial value to set.
     * @param[in] max_len The max size in bytes that the value can be.
     */
    NimBLEAttValue(const char* value, uint16_t max_len = BLE_ATT_ATTR_MAX_LEN)
        : NimBLEAttValue(reinterpret_cast<const uint8_t*>(value), clampLen(strlen(value)), max_len) {}

    /**
     * @brief Construct with an initializer list.
     * @param list An initializer list containing the initial value to set.
     * @param[in] max_len The max size in bytes that the value can be.
     */
    NimBLEAttValue(std::initializer_list<uint8_t> list, uint16_t max_len = BLE_ATT_ATTR_MAX_LEN)
        : NimBLEAttValue(list.begin(), clampLen(list.size()), max_len) {}

    /**
     * @brief Construct with an initial value from a std::string.
     * @param str A std::string containing to the initial value to set.
     * @param[in] max_len The max size in bytes that the value can be.
     */
    NimBLEAttValue(const std::string& str, uint16_t max_len = BLE_ATT_ATTR_MAX_LEN)
        : NimBLEAttValue(reinterpret_cast<const uint8_t*>(str.data()), clampLen(str.length()), max_len) {}

    /**
     * @brief Construct with an initial value from a std::string_view.
     * @param str A std::string_view containing the initial value to set.
     * @param[in] max_len The max size in bytes that the value can be.
     */
    NimBLEAttValue(std::string_view str, uint16_t max_len = BLE_ATT_ATTR_MAX_LEN)
        : NimBLEAttValue(reinterpret_cast<const uint8_t*>(str.data()), clampLen(str.size()), max_len) {}

    /**
     * @brief Construct with an initial value from a std::vector<uint8_t>.
     * @param vec A std::vector<uint8_t> containing to the initial value to set.
     * @param[in] max_len The max size in bytes that the value can be.
     */
    NimBLEAttValue(const std::vector<uint8_t>& vec, uint16_t max_len = BLE_ATT_ATTR_MAX_LEN)
        : NimBLEAttValue(vec.data(), clampLen(vec.size()), max_len) {}

# if NIMBLE_CPP_ARDUINO_STRING_AVAILABLE
    /**
     * @brief Construct with an initial value from an Arduino String.
     * @param str An Arduino String containing to the initial value to set.
     * @param[in] max_len The max size in bytes that the value can be.
     */
    NimBLEAttValue(const String& str, uint16_t max_len = BLE_ATT_ATTR_MAX_LEN)
        : NimBLEAttValue(reinterpret_cast<const uint8_t*>(str.c_str()), clampLen(str.length()), max_len) {}

    /**
     * @brief Construct with an initial value from an Arduino String rvalue.
     * @param str An Arduino String containing to the initial value to set.
     * @param[in] max_len The max size in bytes that the value can be.
     */
    NimBLEAttValue(String&& str, uint16_t max_len = BLE_ATT_ATTR_MAX_LEN)
        : NimBLEAttValue(reinterpret_cast<const uint8_t*>(str.c_str()), clampLen(str.length()), max_len) {}
# endif

    /** @brief Copy constructor */
    NimBLEAttValue(const NimBLEAttValue& source) { deepCopy(source); }

    /** @brief Move constructor */
    NimBLEAttValue(NimBLEAttValue&& source) { *this = std::move(source); }

    /**
     * @brief Construct with an initial value from a std::string rvalue.
     * @param value A std::string containing the initial value to set.
     * @param[in] max_len The max size in bytes that the value can be.
     * @note The bytes are copied into this object's own buffer; the string's storage is not taken over.
     * If the string is larger than max_len the value is left empty.
     */
    NimBLEAttValue(std::string&& value, uint16_t max_len = BLE_ATT_ATTR_MAX_LEN) noexcept
        : NimBLEAttValue(reinterpret_cast<const uint8_t*>(value.data()), clampLen(value.size()), max_len) {}

    /**
     * @brief Construct with an initial value from a std::vector<uint8_t> rvalue.
     * @param value A std::vector<uint8_t> containing the initial value to set.
     * @param[in] max_len The max size in bytes that the value can be.
     * @note The bytes are copied into this object's own buffer; the vector's storage is not taken over.
     * If the vector is larger than max_len the value is left empty.
     */
    NimBLEAttValue(std::vector<uint8_t>&& value, uint16_t max_len = BLE_ATT_ATTR_MAX_LEN) noexcept
        : NimBLEAttValue(value.data(), clampLen(value.size()), max_len) {}

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

    /**
     * @brief Returns a pointer to the internal buffer of the value
     * @note The pointer is not protected by the lock and can be invalidated by a concurrent update.
     * Only use the pointer getters (`data()`, `c_str()`, `begin()`, `end()`) on a copy of the value.
     */
    const uint8_t* data() const { return m_attr_value; }

    /** @brief Returns a pointer to the internal buffer of the value as a const char*
     *  @note See data() for thread safety restrictions.
     */
    const char* c_str() const { return reinterpret_cast<const char*>(m_attr_value); }

    /** @brief Iterator begin; see data() for thread safety restrictions. */
    const uint8_t* begin() const { return m_attr_value; }

    /** @brief Iterator end; see data() for thread safety restrictions. */
    const uint8_t* end() const { return m_attr_value + m_attr_len; }

# if MYNEWT_VAL(NIMBLE_CPP_ATT_VALUE_TIMESTAMP_ENABLED)
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
# if MYNEWT_VAL(NIMBLE_CPP_ATT_VALUE_TIMESTAMP_ENABLED)
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

    /**
     * @brief Copy the value from another NimBLEAttValue instance.
     * @param[in] source The NimBLEAttValue instance to copy from.
     * @returns True if the copy was successful.
     */
    bool copyFrom(const NimBLEAttValue& source) { return deepCopy(source); }

    /*********************** Template Functions ************************/

    /**
     * @brief Template to set the value from a container, string, char array or trivially copyable object.
     * @tparam T The type of the value, must not be a pointer.
     * @param [in] s The value to set.
     * @details Containers with `data()` and `size()` and types with `c_str()` and `length()` (including
     * `std::string_view` and `std::span`) are copied by their contents, char arrays are measured with `strnlen`,
     * anything else is copied by its object representation using `sizeof()`.
     */
    template <NimBLEByteValue T>
    bool setValue(const T& s) {
        if constexpr (NimBLECharArray<T>) {
            return setValue(reinterpret_cast<const uint8_t*>(s), strnlen(s, sizeof(s)));
        } else {
            const auto bytes = NimBLEAsBytes(s);
            return setValue(bytes.data(), clampLen(bytes.size()));
        }
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
        Lock lock;
        if (timestamp != nullptr) {
# if MYNEWT_VAL(NIMBLE_CPP_ATT_VALUE_TIMESTAMP_ENABLED)
            *timestamp = m_timestamp;
# else
            *timestamp = 0;
# endif
        }

        if (!skipSizeCheck && size() < sizeof(T)) {
            return T();
        }
        return *(reinterpret_cast<const T*>(m_attr_value));
    }

    /*********************** Operators ************************/

    /** @brief Subscript operator */
    uint8_t operator[](int pos) const;

    /** @brief Operator; Get the value as a std::vector<uint8_t>. */
    operator std::vector<uint8_t>() const {
        Lock lock;
        return std::vector<uint8_t>(m_attr_value, m_attr_value + m_attr_len);
    }

    /** @brief Operator; Get the value as a std::string. */
    operator std::string() const {
        Lock lock;
        return std::string(reinterpret_cast<char*>(m_attr_value), m_attr_len);
    }

    /**
     * @brief Operator; Get a std::string_view of the value.
     * @note See data() for thread safety restrictions, the view is invalidated by a concurrent update.
     */
    operator std::string_view() const { return std::string_view(reinterpret_cast<const char*>(m_attr_value), m_attr_len); }

    /** @brief Operator; Get the value as a const uint8_t*. */
    operator const uint8_t*() const { return m_attr_value; }

    /** @brief Operator; Append another NimBLEAttValue. */
    NimBLEAttValue& operator+=(const NimBLEAttValue& source) {
        Lock lock; // Recursive; keeps the source data and size consistent while appending
        if (&source == this) {
            // append() may reallocate the buffer, so append from a copy of our own data
            std::vector<uint8_t> copy(m_attr_value, m_attr_value + m_attr_len);
            return append(copy.data(), copy.size());
        }
        return append(source.data(), source.size());
    }

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
        Lock lock;
        return (m_attr_len == source.size()) ? memcmp(m_attr_value, source.data(), m_attr_len) == 0 : false;
    }

    /** @brief Inequality operator */
    bool operator!=(const NimBLEAttValue& source) const { return !(*this == source); }

# if NIMBLE_CPP_ARDUINO_STRING_AVAILABLE
    /** @brief Operator; Get the value as an Arduino String value. */
    operator String() const {
        Lock lock;
        return String(reinterpret_cast<char*>(m_attr_value));
    }
# endif
};

#endif // CONFIG_BT_NIMBLE_ENABLED
#endif // NIMBLE_CPP_ATTVALUE_H_
