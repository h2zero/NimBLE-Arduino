/*
 * NimBLEAddress.h
 *
 *  Created: on Jan 24 2020
 *      Author H2zero
 *
 * Originally:
 *
 * BLEAddress.h
 *
 *  Created on: Jul 2, 2017
 *      Author: kolban
 */

#ifndef NIMBLE_CPP_ADDRESS_H_
#define NIMBLE_CPP_ADDRESS_H_
#include "nimconfig.h"
#if defined(CONFIG_BT_ENABLED)

# if defined(CONFIG_NIMBLE_CPP_IDF)
#  include "nimble/ble.h"
# else
#  include "nimble/nimble/include/nimble/ble.h"
# endif

/****  FIX COMPILATION ****/
# undef min
# undef max
/**************************/

# include <string>

/**
 * @brief A %BLE device address.
 *
 * Every %BLE device has a unique address which can be used to identify it and form connections.
 */
class NimBLEAddress : private ble_addr_t {
  public:
    /**
     * @brief Create a blank address, i.e. 00:00:00:00:00:00, type 0.
     */
    NimBLEAddress() = default;
    NimBLEAddress(const ble_addr_t address);
    NimBLEAddress(const uint8_t address[BLE_DEV_ADDR_LEN], uint8_t type = BLE_ADDR_PUBLIC);
    NimBLEAddress(const std::string& stringAddress, uint8_t type = BLE_ADDR_PUBLIC);
    NimBLEAddress(const uint64_t& address, uint8_t type = BLE_ADDR_PUBLIC);

    bool                 isRpa() const;
    bool                 isNrpa() const;
    bool                 isStatic() const;
    bool                 isPublic() const;
    bool                 isNull() const;
    bool                 equals(const NimBLEAddress& otherAddress) const;
    const ble_addr_t*    getBase() const;
    std::string          toString() const;
    uint8_t              getType() const;
    const uint8_t*       getVal() const;
    const NimBLEAddress& reverseByteOrder();
    bool                 operator==(const NimBLEAddress& rhs) const;
    bool                 operator!=(const NimBLEAddress& rhs) const;
                         operator std::string() const;
                         operator uint64_t() const;
};

#endif /* CONFIG_BT_ENABLED */
#endif /* NIMBLE_CPP_ADDRESS_H_ */
