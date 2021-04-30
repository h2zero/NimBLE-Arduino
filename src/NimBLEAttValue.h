/*
 * NimBLEAttValue.h
 *
 *  Created: on March 18, 2021
 *      Author H2zero
 *
 */

#ifndef MAIN_NIMBLEATTVALUE_H_
#define MAIN_NIMBLEATTVALUE_H_
#include "nimconfig.h"
#if defined(CONFIG_BT_ENABLED)

#ifdef NIMBLE_ARDUINO_AVAILABLE
#include <Arduino.h>
#endif

#include "NimBLELog.h"

#include <string>
#include <vector>
#include <time.h>

class NimBLEAttValue
{
    uint8_t*     m_attr_value = nullptr;    /*!<  the pointer to attribute value */
    uint16_t     m_attr_max_len = 0;  /*!<  attribute max value length */
    uint16_t     m_attr_len = 0;      /*!<  attribute current value length */
    time_t       m_timestamp = 0;
#ifdef ESP_PLATFORM
    portMUX_TYPE m_valMux = portMUX_INITIALIZER_UNLOCKED;
#endif

    void deepCopy(const NimBLEAttValue & source);

public:
    NimBLEAttValue(uint16_t init_len, uint16_t max_len);
    ~NimBLEAttValue();
    NimBLEAttValue(const NimBLEAttValue & source) { deepCopy(source); }

    void      lock();
    void      unlock();
    uint16_t  getMaxLength() { return m_attr_max_len; }
    uint16_t  getLength() { return m_attr_len; }
    uint8_t*  getValue(time_t *timestamp);
    uint8_t*  getValue() { return m_attr_value; }
    char*     c_str() { return (char*)m_attr_value; }
    bool      setValue(const uint8_t *value, uint16_t len);

    NimBLEAttValue& operator =(const NimBLEAttValue & source);
    operator std::string() { return std::string((char*)m_attr_value, m_attr_len); }
    operator const uint8_t*() { return m_attr_value; }
    template<typename T>
    operator std::vector<T>() { return std::vector<T>(*m_attr_value, m_attr_len); }

#ifdef NIMBLE_ARDUINO_AVAILABLE
    operator String() { return String((char*)m_attr_value); }
#endif
};


inline NimBLEAttValue::NimBLEAttValue(uint16_t init_len, uint16_t max_len) {
    m_attr_value   = (uint8_t*)calloc(init_len + 1, 1);
    assert(m_attr_value != nullptr && "No Mem");
    m_attr_max_len = max_len;
    m_attr_len     = 0;
    m_timestamp    = 0;
}


inline NimBLEAttValue::~NimBLEAttValue() {
    if(m_attr_value != nullptr) {
        free(m_attr_value);
    }
}


inline NimBLEAttValue& NimBLEAttValue::operator =(const NimBLEAttValue & source) {
    if (this != &source) {
        deepCopy(source);
    }
    return *this;
}


inline void NimBLEAttValue::deepCopy(const NimBLEAttValue & source) {
    if (m_attr_value != nullptr) {
        free(m_attr_value);
    }

#ifdef ESP_PLATFORM
    m_valMux = portMUX_INITIALIZER_UNLOCKED;
#endif

    lock();
    m_attr_max_len = source.m_attr_max_len;
    m_attr_len = source.m_attr_len;
    m_timestamp = source.m_timestamp;
    m_attr_value = (uint8_t*)malloc( (m_attr_len == 0 ? NIMBLE_ATT_INIT_LENGTH : m_attr_len) + 1 );

    if(m_attr_value != nullptr) {
        memcpy(m_attr_value, source.m_attr_value, m_attr_len + 1);
    } else {
        NIMBLE_LOGE("NimBLEAttValue", "NO MEM");
    }
    unlock();
}


inline void NimBLEAttValue::lock() {
    #ifdef ESP_PLATFORM
    portENTER_CRITICAL(&m_valMux);
    #else
    portENTER_CRITICAL();
    #endif
}


inline void NimBLEAttValue::unlock() {
    #ifdef ESP_PLATFORM
    portEXIT_CRITICAL(&m_valMux);
    #else
    portEXIT_CRITICAL();
    #endif
}


inline uint8_t*  NimBLEAttValue::getValue(time_t *timestamp) {
    if(timestamp != nullptr) {
        lock();
        *timestamp = m_timestamp;
        unlock();
    }
    return m_attr_value;
}


inline bool NimBLEAttValue::setValue(const uint8_t *value, uint16_t len) {
    if (len > m_attr_max_len) {
        NIMBLE_LOGE("NimBLEAttValue", "val > max, len=%u, max=%u", len, m_attr_max_len);
        return false;
    }

    uint8_t *res = m_attr_value;
    if (len > m_attr_len && len > NIMBLE_ATT_INIT_LENGTH) {
        res = (uint8_t*)realloc(m_attr_value, (len + 1));
    }

    if (res != nullptr) {
        time_t t = time(nullptr);
        lock();
        m_attr_value = res;
        memcpy(m_attr_value, value, len);
        m_attr_value[len] = '\0';
        m_attr_len = len;
        m_timestamp = t;
        unlock();
        return true;
    }

    NIMBLE_LOGE("NimBLEAttValue", "realloc failed len=%u, max=%u", len, m_attr_max_len);
    return false;
}

#endif /*(CONFIG_BT_ENABLED) */
#endif /* MAIN_NIMBLEATTVALUE_H_ */
