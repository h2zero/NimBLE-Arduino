/*
 * NimBLELocalAttribute.cpp
 *
 *  Created: on July 28 2024
 *      Author H2zero
 */

#ifndef NIMBLE_CPP_LOCAL_ATTRIBUTE_H_
#define NIMBLE_CPP_LOCAL_ATTRIBUTE_H_

#include "nimconfig.h"
#if defined(CONFIG_BT_ENABLED) && defined(CONFIG_BT_NIMBLE_ROLE_PERIPHERAL)

# include "NimBLEAttribute.h"

/**
 * @brief A base class for local BLE attributes.
 */
class NimBLELocalAttribute : public NimBLEAttribute {
  public:
    /**
     * @brief Get the removed flag.
     * @return The removed flag.
     */
    uint8_t getRemoved() const { return m_removed; }

  protected:
    /**
     * @brief Construct a local attribute.
     */
    NimBLELocalAttribute(const NimBLEUUID& uuid, uint16_t handle) : NimBLEAttribute{uuid, handle}, m_removed{0} {}

    /**
     * @brief Destroy the local attribute.
     */
    ~NimBLELocalAttribute() = default;

    /**
     * @brief Set the removed flag.
     * @param [in] removed The removed flag.
     */
    void setRemoved(uint8_t removed) { m_removed = removed; }

    uint8_t m_removed{0};
};

#endif // CONFIG_BT_ENABLED && CONFIG_BT_NIMBLE_ROLE_PERIPHERAL
#endif // NIMBLE_CPP_LOCAL_ATTRIBUTE_H_
