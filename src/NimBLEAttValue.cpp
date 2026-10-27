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

#include "NimBLEAttValue.h"
#if CONFIG_BT_NIMBLE_ENABLED

# include "NimBLEUtils.h"
# include "NimBLELog.h"

# if defined(__ARM_ARCH_6M__)
// Cortex-M0/M0+ has no LDREX/STREX, so GCC emits libcalls for 4 byte atomic read-modify-write
// operations that are not provided by libgcc. Implement them by briefly masking interrupts.
extern "C" {
unsigned int __atomic_fetch_add_4(volatile void* ptr, unsigned int val, int) {
    volatile unsigned int* p = static_cast<volatile unsigned int*>(ptr);
    unsigned int           primask;
    __asm volatile("mrs %0, primask\n\tcpsid i" : "=r"(primask)::"memory");
    unsigned int old = *p;
    *p               = old + val;
    __asm volatile("msr primask, %0" ::"r"(primask) : "memory");
    return old;
}

unsigned int __atomic_fetch_sub_4(volatile void* ptr, unsigned int val, int) {
    volatile unsigned int* p = static_cast<volatile unsigned int*>(ptr);
    unsigned int           primask;
    __asm volatile("mrs %0, primask\n\tcpsid i" : "=r"(primask)::"memory");
    unsigned int old = *p;
    *p               = old - val;
    __asm volatile("msr primask, %0" ::"r"(primask) : "memory");
    return old;
}

unsigned char __atomic_exchange_1(volatile void* ptr, unsigned char val, int) {
    volatile unsigned char* p = static_cast<volatile unsigned char*>(ptr);
    unsigned int            primask;
    __asm volatile("mrs %0, primask\n\tcpsid i" : "=r"(primask)::"memory");
    unsigned char old = *p;
    *p                = val;
    __asm volatile("msr primask, %0" ::"r"(primask) : "memory");
    return old;
}
}
# endif

static const char* LOG_TAG = "NimBLEAttValue";
ble_npl_mutex     NimBLEAttValue::Lock::m_mutex{};
std::atomic<bool> NimBLEAttValue::Lock::m_initialized{false};
std::atomic<int>  NimBLEAttValue::Lock::m_activeCount{0};

// Default constructor implementation.
NimBLEAttValue::NimBLEAttValue(uint16_t init_len, uint16_t max_len)
    : m_attr_value{static_cast<uint8_t*>(calloc(init_len + 1, 1))},
      m_attr_max_len{std::min<uint16_t>(BLE_ATT_ATTR_MAX_LEN, max_len)},
      m_attr_len{},
      m_capacity{init_len}
# if MYNEWT_VAL(NIMBLE_CPP_ATT_VALUE_TIMESTAMP_ENABLED)
      ,
      m_timestamp{}
# endif
{
    NIMBLE_CPP_DEBUG_ASSERT(m_attr_value);
    if (m_attr_value == nullptr) {
        NIMBLE_LOGE(LOG_TAG, "Failed to allocate value buffer");
        m_attr_len = 0;
        m_capacity = 0;
        m_attr_max_len = 0;
    }
}

// Value constructor implementation.
NimBLEAttValue::NimBLEAttValue(const uint8_t* value, uint16_t len, uint16_t max_len) : NimBLEAttValue(len, max_len) {
    if (m_attr_value != nullptr && len > 0) {
        memcpy(m_attr_value, value, len);
        m_attr_len = len;
    }
}

// Destructor implementation.
NimBLEAttValue::~NimBLEAttValue() {
    if (m_attr_value != nullptr) {
        free(m_attr_value);
    }
}

// Move assignment operator implementation.
NimBLEAttValue& NimBLEAttValue::operator=(NimBLEAttValue&& source) {
    if (this != &source) {
        Lock lock;
        free(m_attr_value);
        m_attr_value   = source.m_attr_value;
        m_attr_max_len = source.m_attr_max_len;
        m_attr_len     = source.m_attr_len;
        m_capacity     = source.m_capacity;
        setTimeStamp(source.getTimeStamp());
        source.m_attr_value = nullptr;
    }

    return *this;
}

// Copy assignment implementation.
NimBLEAttValue& NimBLEAttValue::operator=(const NimBLEAttValue& source) {
    if (this != &source) {
        deepCopy(source);
    }
    return *this;
}

// Copy all the data from the source object to this object, including allocated space.
bool NimBLEAttValue::deepCopy(const NimBLEAttValue& source) {
    Lock     lock;
    uint8_t* res = static_cast<uint8_t*>(realloc(m_attr_value, source.m_capacity + 1));
    NIMBLE_CPP_DEBUG_ASSERT(res);
    if (res == nullptr) {
        NIMBLE_LOGE(LOG_TAG, "Failed to realloc deepCopy");
        return false;
    }

    m_attr_value   = res;
    m_attr_max_len = source.m_attr_max_len;
    m_attr_len     = source.m_attr_len;
    m_capacity     = source.m_capacity;
    setTimeStamp(source.getTimeStamp());
    memcpy(m_attr_value, source.m_attr_value, m_attr_len + 1);
    return true;
}

// Set the value of the attribute.
bool NimBLEAttValue::setValue(const uint8_t* value, uint16_t len) {
    Lock lock;
    m_attr_len      = 0;    // Just set the value length to 0 and append instead of repeating code.
    m_attr_value[0] = '\0'; // Set the first byte to 0 incase the len of the new value is 0.
    append(value, len);
    return memcmp(m_attr_value, value, len) == 0 && m_attr_len == len;
}

// Append the new data, allocate as necessary.
NimBLEAttValue& NimBLEAttValue::append(const uint8_t* value, uint16_t len) {
    Lock lock;
    if (len == 0) {
        return *this;
    }

    if ((m_attr_len + len) > m_attr_max_len) {
        NIMBLE_LOGE(LOG_TAG, "val > max, len=%u, max=%u", len, m_attr_max_len);
        return *this;
    }

    uint8_t* res     = m_attr_value;
    uint16_t new_len = m_attr_len + len;
    bool     grow    = new_len > m_capacity;
    if (grow) {
        res = static_cast<uint8_t*>(realloc(m_attr_value, (new_len + 1)));
    }
    NIMBLE_CPP_DEBUG_ASSERT(res);
    if (res == nullptr) {
        NIMBLE_LOGE(LOG_TAG, "Failed to realloc append");
        return *this;
    }

    if (grow) {
        m_capacity = new_len;
    }

# if MYNEWT_VAL(NIMBLE_CPP_ATT_VALUE_TIMESTAMP_ENABLED)
    time_t t = time(nullptr);
# else
    time_t t = 0;
# endif

    memcpy(res + m_attr_len, value, len);
    m_attr_value             = res;
    m_attr_len               = new_len;
    m_attr_value[m_attr_len] = '\0';
    setTimeStamp(t);

    return *this;
}

uint8_t NimBLEAttValue::operator[](int pos) const {
    Lock lock;
    NIMBLE_CPP_DEBUG_ASSERT(pos < m_attr_len);
    if (pos >= m_attr_len) {
        NIMBLE_LOGE(LOG_TAG, "pos >= len, pos=%u, len=%u", pos, m_attr_len);
        return 0;
    }
    return m_attr_value[pos];
}

#endif // CONFIG_BT_NIMBLE_ENABLED
