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
    uint8_t   *m_attr_value;    /*!<  the pointer to attribute value */
    uint16_t  m_attr_max_len;  /*!<  attribute max value length */
    uint16_t  m_attr_len;      /*!<  attribute current value length */

    void      setValue(uint8_t *value, uint8_t len) {memcpy(m_attr_value, value, len); m_attr_len = len;}
    uint8_t  *getValue(){return m_attr_value;}
    uint16_t  getLength(){return m_attr_len;}
    uint16_t  getMaxLength(){return m_attr_max_len;}
    void      setMaxLength(uint16_t len){m_attr_max_len = len;}

public:
    NimBLEAttValue(uint16_t init_len, uint16_t max_len) {
        m_attr_value = (uint8_t*)calloc(init_len, 1);
        m_attr_max_len = max_len;
        m_attr_len = 0;
    }

    ~NimBLEAttValue() {free(m_attr_value);}

    uint8_t * checkMemAlloc(uint16_t len) {
        uint8_t *res = nullptr;

        if (length > m_attr_len && length > NIMBLE_ATT_INIT_LENGTH) {
            res = (uint8_t*)realloc(m_attr_value, length);
            if(res != nullptr) {
                m_attr_value = res;
            }
            return res;
        }
    }


    void setValue(uint8_t *data, uint16_t len) {memcpy(m_attr_value, data, length);}

    operator std::string() { return std::string((char*)attr_value, attr_len);}

    template<typename T>
    operator std::vector<T>() { return std::vector<T>(*attr_value, attr_len);}
};

#endif /*(CONFIG_BT_ENABLED) */
#endif /* MAIN_NIMBLEATTVALUE_H_ */