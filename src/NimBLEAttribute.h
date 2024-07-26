/*
 * NimBLEAttribute.h
 *
 *  Created: on July 28 2024
 *      Author H2zero
 */

#ifndef NIMBLE_CPP_ATTRIBUTE_H_
#define NIMBLE_CPP_ATTRIBUTE_H_

#include "nimconfig.h"
#if defined(CONFIG_BT_ENABLED) && (defined(CONFIG_BT_NIMBLE_ROLE_PERIPHERAL) || defined(CONFIG_BT_NIMBLE_ROLE_CENTRAL))

# include "NimBLEUUID.h"

/**
 * @brief A base class for BLE attributes.
 */
class NimBLEAttribute {
  public:
    /**
     * @brief Get the UUID of the attribute.
     * @return The UUID.
     */
    const NimBLEUUID& getUUID() const { return m_uuid; }

    /**
     * @brief Get the handle of the attribute.
     */
    uint16_t getHandle() const { return m_handle; };

  protected:
    /**
     * @brief Construct a new NimBLEAttribute object.
     * @param [in] handle The handle of the attribute.
     * @param [in] uuid The UUID of the attribute.
     */
    NimBLEAttribute(const NimBLEUUID& uuid, uint16_t handle) : m_uuid{uuid}, m_handle{handle} {}

    /**
     * @brief Destroy the NimBLEAttribute object.
     */
    ~NimBLEAttribute() = default;

    const NimBLEUUID m_uuid{};
    uint16_t   m_handle{0};
};

#endif // CONFIG_BT_ENABLED && (CONFIG_BT_NIMBLE_ROLE_PERIPHERAL || CONFIG_BT_NIMBLE_ROLE_CENTRAL)
#endif // NIMBLE_CPP_ATTRIBUTE_H_
