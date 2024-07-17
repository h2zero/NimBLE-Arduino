/*
 * NimBLEAttValue.cpp
 *
 *  Created: on July 17, 2024
 *      Author H2zero
 *
 */

#include "nimconfig.h"
#if defined(CONFIG_BT_ENABLED)

# if defined(CONFIG_NIMBLE_CPP_IDF)
#  include "nimble/nimble_npl.h"
# else
#  include "nimble/nimble/include/nimble/nimble_npl.h"
# endif

# include "NimBLEAttValue.h"

// Default constructor implementation.
NimBLEAttValue::NimBLEAttValue(uint16_t init_len, uint16_t max_len)
    : m_attr_value{static_cast<uint8_t*>(calloc(init_len + 1, 1))},
      m_attr_max_len{std::min<uint16_t>(BLE_ATT_ATTR_MAX_LEN, max_len)},
      m_attr_len{},
      m_capacity{init_len}
# if CONFIG_NIMBLE_CPP_ATT_VALUE_TIMESTAMP_ENABLED
      ,
      m_timestamp{}
# endif
{
    NIMBLE_CPP_DEBUG_ASSERT(m_attr_value);
}

// Value constructor implementation.
NimBLEAttValue::NimBLEAttValue(const uint8_t *value, uint16_t len, uint16_t max_len)
: NimBLEAttValue(len, max_len) {
    memcpy(m_attr_value, value, len);
    m_attr_value[len] = '\0';
    m_attr_len        = len;
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
void NimBLEAttValue::deepCopy(const NimBLEAttValue& source) {
    uint8_t* res = static_cast<uint8_t*>(realloc(m_attr_value, source.m_capacity + 1));
    NIMBLE_CPP_DEBUG_ASSERT(res);

    ble_npl_hw_enter_critical();
    m_attr_value   = res;
    m_attr_max_len = source.m_attr_max_len;
    m_attr_len     = source.m_attr_len;
    m_capacity     = source.m_capacity;
    setTimeStamp(source.getTimeStamp());
    memcpy(m_attr_value, source.m_attr_value, m_attr_len + 1);
    ble_npl_hw_exit_critical(0);
}

// Set the value of the attribute.
bool NimBLEAttValue::setValue(const uint8_t* value, uint16_t len) {
    m_attr_len = 0; // Just set the value length to 0 and append instead of repeating code.
    append(value, len);
    return memcmp(m_attr_value, value, len) == 0 && m_attr_len == len;
}

// Append the new data, allocate as necessary.
NimBLEAttValue& NimBLEAttValue::append(const uint8_t* value, uint16_t len) {
    if (len == 0) {
        return *this;
    }

    if ((m_attr_len + len) > m_attr_max_len) {
        NIMBLE_LOGE("NimBLEAttValue", "val > max, len=%u, max=%u", len, m_attr_max_len);
        return *this;
    }

    uint8_t* res     = m_attr_value;
    uint16_t new_len = m_attr_len + len;
    if (new_len > m_capacity) {
        res        = static_cast<uint8_t*>(realloc(m_attr_value, (new_len + 1)));
        m_capacity = new_len;
    }
    NIMBLE_CPP_DEBUG_ASSERT(res);

# if CONFIG_NIMBLE_CPP_ATT_VALUE_TIMESTAMP_ENABLED
    time_t t = time(nullptr);
# else
    time_t t = 0;
# endif

    ble_npl_hw_enter_critical();
    memcpy(res + m_attr_len, value, len);
    m_attr_value             = res;
    m_attr_len               = new_len;
    m_attr_value[m_attr_len] = '\0';
    setTimeStamp(t);
    ble_npl_hw_exit_critical(0);

    return *this;
}

#endif // CONFIG_BT_ENABLED
