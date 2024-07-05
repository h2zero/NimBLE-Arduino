/*
 * NimBLEAddress.cpp
 *
 *  Created: on Jan 24 2020
 *      Author H2zero
 *
 * Originally:
 *
 * BLEAddress.cpp
 *
 *  Created on: Jul 2, 2017
 *      Author: kolban
 */
#include "nimconfig.h"
#if defined(CONFIG_BT_ENABLED)

# include "NimBLEAddress.h"
# include "NimBLELog.h"

# include <algorithm>

static const char* LOG_TAG = "NimBLEAddress";

/*************************************************
 * NOTE: NimBLE address bytes are in INVERSE ORDER!
 * We will accommodate that fact in these methods.
 *************************************************/

/**
 * @brief Create an address from the native NimBLE representation.
 * @param [in] address The native NimBLE address.
 */
NimBLEAddress::NimBLEAddress(ble_addr_t address) : ble_addr_t{address} {}

/**
 * @brief Create an address from a hex string.
 *
 * A hex string is of the format:
 * ```
 * 00:00:00:00:00:00
 * ```
 * which is 17 characters in length.
 * @param [in] addr The hex string representation of the address.
 * @param [in] type The type of the address.
 */
NimBLEAddress::NimBLEAddress(const std::string& addr, uint8_t type) {
    this->type = type;

    if (addr.length() == BLE_DEV_ADDR_LEN) {
        std::reverse_copy(addr.data(), addr.data() + BLE_DEV_ADDR_LEN, this->val);
        return;
    }

    if (addr.length() == 17) {
        std::string mac{addr};
        mac.erase(std::remove(mac.begin(), mac.end(), ':'), mac.end());
        uint64_t address = std::stoul(mac, nullptr, 16);
        memcpy(this->val, &address, sizeof this->val);
        return;
    }

    *this = NimBLEAddress{};
    NIMBLE_LOGE(LOG_TAG, "Invalid address '%s'", addr.c_str());
} // NimBLEAddress

/**
 * @brief Constructor for compatibility with bluedroid esp library using native ESP representation.
 * @param [in] address A uint8_t[6] or esp_bd_addr_t containing the address.
 * @param [in] type The type of the address.
 */
NimBLEAddress::NimBLEAddress(const uint8_t address[BLE_DEV_ADDR_LEN], uint8_t type) {
    std::reverse_copy(address, address + BLE_DEV_ADDR_LEN, this->val);
    this->type = type;
} // NimBLEAddress

/**
 * @brief Constructor for address using a hex value.\n
 * Use the same byte order, so use 0xa4c1385def16 for "a4:c1:38:5d:ef:16"
 * @param [in] address uint64_t containing the address.
 * @param [in] type The type of the address.
 */
NimBLEAddress::NimBLEAddress(const uint64_t& address, uint8_t type) {
    memcpy(this->val, &address, sizeof this->val);
    this->type = type;
} // NimBLEAddress

/**
 * @brief Determine if this address equals another.
 * @param [in] otherAddress The other address to compare against.
 * @return True if the addresses are equal.
 */
bool NimBLEAddress::equals(const NimBLEAddress& otherAddress) const {
    return *this == otherAddress;
} // equals

/**
 * @brief Get the NimBLE base struct of the address.
 * @return A read only reference to the NimBLE base struct of the address.
 */
const ble_addr_t* NimBLEAddress::getBase() const {
    return reinterpret_cast<const ble_addr_t*>(this);
} // getBase

/**
 * @brief Get the address type.
 * @return The address type.
 */
uint8_t NimBLEAddress::getType() const {
    return this->type;
} // getType

/**
 * @brief Get the address value.
 * @return A read only reference to the address value.
 */
const uint8_t* NimBLEAddress::getVal() const {
    return this->val;
} // getVal

/**
 * @brief Determine if this address is a Resolvable Private Address.
 * @return True if the address is a RPA.
 */
bool NimBLEAddress::isRpa() const {
    return BLE_ADDR_IS_RPA(this);
} // isRpa

/**
 * @brief Determine if this address is a Non-Resolvable Private Address.
 * @return True if the address is a NRPA.
 */
bool NimBLEAddress::isNrpa() const {
    return BLE_ADDR_IS_NRPA(this);
} // isNrpa

/**
 * @brief Determine if this address is a Static Address.
 * @return True if the address is a Static Address.
 */
bool NimBLEAddress::isStatic() const {
    return BLE_ADDR_IS_STATIC(this);
} // isStatic

/**
 * @brief Determine if this address is a Public Address.
 * @return True if the address is a Public Address.
 */
bool NimBLEAddress::isPublic() const {
    return this->type == BLE_ADDR_PUBLIC;
} // isPublic

/**
 * @brief Determine if this address is a NULL Address.
 * @return True if the address is a NULL Address.
 */
bool NimBLEAddress::isNull() const {
    return *this == NimBLEAddress{};
} // isNull

/**
 * @brief Convert a BLE address to a string.
 * @return The string representation of the address.
 * @deprecated Use std::string() operator instead.
 */
std::string NimBLEAddress::toString() const {
    return std::string(*this);
} // toString

/**
 * @brief Reverse the byte order of the address.
 * @return A reference to this address.
 */
const NimBLEAddress& NimBLEAddress::reverseByteOrder() {
    std::reverse(this->val, this->val + BLE_DEV_ADDR_LEN);
    return *this;
} // reverseByteOrder

/**
 * @brief Convenience operator to check if this address is equal to another.
 */
bool NimBLEAddress::operator==(const NimBLEAddress& rhs) const {
    if (this->type != rhs.type) {
        return false;
    }

    return memcmp(rhs.val, this->val, sizeof this->val) == 0;
} // operator ==

/**
 * @brief Convenience operator to check if this address is not equal to another.
 */
bool NimBLEAddress::operator!=(const NimBLEAddress& rhs) const {
    return !this->operator==(rhs);
} // operator !=

/**
 * @brief Convenience operator to convert this address to string representation.
 * @details This allows passing NimBLEAddress to functions that accept std::string and/or it's methods as a parameter.
 */
NimBLEAddress::operator std::string() const {
    char buffer[18];
    snprintf(buffer,
             sizeof(buffer),
             "%02x:%02x:%02x:%02x:%02x:%02x",
             this->val[5],
             this->val[4],
             this->val[3],
             this->val[2],
             this->val[1],
             this->val[0]);
    return std::string{buffer};
} // operator std::string

/**
 * @brief Convenience operator to convert the native address representation to uint_64.
 */
NimBLEAddress::operator uint64_t() const {
    uint64_t address = 0;
    memcpy(&address, this->val, sizeof this->val);
    return address;
} // operator uint64_t

#endif
