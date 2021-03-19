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

#include <string>
#include <vector>

class NimBLEAttValue
{
    friend class NimBLECharacteristic;
    friend class NimBLEDescriptor;

    uint8_t*     m_attr_value;    /*!<  the pointer to attribute value */
    uint16_t     m_attr_max_len;  /*!<  attribute max value length */
    uint16_t     m_attr_len;      /*!<  attribute current value length */
    time_t       m_timestamp;
#ifdef ESP_PLATFORM
    portMUX_TYPE m_valMux = portMUX_INITIALIZER_UNLOCKED;
#endif

public:
    NimBLEAttValue(uint16_t init_len, uint16_t max_len) {
        m_attr_value   = (uint8_t*)calloc(init_len, 1);
        m_attr_max_len = max_len;
        m_attr_len     = 0;
        m_timestamp    = 0;
    }

    ~NimBLEAttValue() {free(m_attr_value);}

    uint16_t  getMaxLength() {return m_attr_max_len;}

    uint16_t  getLength(){
    #ifdef ESP_PLATFORM
        portENTER_CRITICAL(&m_valMux);
        uint16_t len = m_attr_len;;
        portEXIT_CRITICAL(&m_valMux);
    #else
        portENTER_CRITICAL();
        size_t len = m_attr_len;;
        portEXIT_CRITICAL();
    #endif
    return len;
    }

    uint8_t* getValue(time_t *timestamp = nullptr) {
    #ifdef ESP_PLATFORM
        portENTER_CRITICAL(&m_valMux);
        if(timestamp != nullptr) {
            *timestamp = m_timestamp;
        }
        portEXIT_CRITICAL(&m_valMux);
    #else
        portENTER_CRITICAL();
        if(timestamp != nullptr) {
            *timestamp = m_timestamp;
        }
        portEXIT_CRITICAL();

        return m_attr_value;
    #endif
    }

    bool setValue(const uint8_t *value, uint16_t len) {
        if (len > m_attr_max_len) {
            return false;
        }

        uint8_t *res = nullptr;
        if (len > m_attr_len && len > NIMBLE_ATT_INIT_LENGTH) {
            res = (uint8_t*)realloc(m_attr_value, len);
        }

        if (res != nullptr) {
            time_t t = time(nullptr);
    #ifdef ESP_PLATFORM
            portENTER_CRITICAL(&m_valMux);
            m_attr_value = res;
            memcpy(m_attr_value, value, len);
            m_attr_len = len;
            m_timestamp = t;
            portEXIT_CRITICAL(&m_valMux);
    #else
            portENTER_CRITICAL();
            m_attr_value = res;
            memcpy(m_attr_value, value, len);
            m_attr_len = len;
            m_timestamp = t;
            portEXIT_CRITICAL();
    #endif
            return true;
        }

        return false;
    }

    operator std::string() { return std::string((char*)m_attr_value, m_attr_len);}

    template<typename T>
    operator std::vector<T>() { return std::vector<T>(*m_attr_value, m_attr_len);}
};

#endif /*(CONFIG_BT_ENABLED) */
#endif /* MAIN_NIMBLEATTVALUE_H_ */