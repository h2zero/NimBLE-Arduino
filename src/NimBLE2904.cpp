/*
 * NimBLE2904.cpp
 *
 *  Created: on March 13, 2020
 *      Author H2zero
 *
 * Originally:
 *
 * BLE2904.cpp
 *
 *  Created on: Dec 23, 2017
 *      Author: kolban
 */

#include "nimconfig.h"
#if defined(CONFIG_BT_ENABLED) && defined(CONFIG_BT_NIMBLE_ROLE_PERIPHERAL)

# include "NimBLE2904.h"

NimBLE2904::NimBLE2904(NimBLECharacteristic* pChr)
    : NimBLEDescriptor(NimBLEUUID((uint16_t)0x2904), BLE_GATT_CHR_F_READ, sizeof(NimBLE2904Data), pChr) {
    setValue(m_data);
} // NimBLE2904

/**
 * @brief Set the description.
 * @param [in] description The description value to set.
 */
void NimBLE2904::setDescription(uint16_t description) {
    m_data.m_description = description;
    setValue(m_data);
} // setDescription

/**
 * @brief Set the exponent.
 * @param [in] exponent The exponent value to set.
 */
void NimBLE2904::setExponent(int8_t exponent) {
    m_data.m_exponent = exponent;
    setValue(m_data);
} // setExponent

/**
 * @brief Set the format.
 * @param [in] format The format value to set.
 */
void NimBLE2904::setFormat(uint8_t format) {
    m_data.m_format = format;
    setValue(m_data);
} // setFormat

/**
 * @brief Set the namespace.
 * @param [in] namespace_value The namespace value toset.
 */
void NimBLE2904::setNamespace(uint8_t namespace_value) {
    m_data.m_namespace = namespace_value;
    setValue(m_data);
} // setNamespace

/**
 * @brief Set the units for this value.
 * @param [in] unit The type of units of this characteristic as defined by assigned numbers.
 * @details See https://www.bluetooth.com/specifications/assigned-numbers/units
 */
void NimBLE2904::setUnit(uint16_t unit) {
    m_data.m_unit = unit;
    setValue(m_data);
} // setUnit

#endif /* CONFIG_BT_ENABLED && CONFIG_BT_NIMBLE_ROLE_PERIPHERAL */
