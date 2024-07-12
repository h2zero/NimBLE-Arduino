/*
 * NimBLEAdvertisementData.h
 *
 *  Created: on November 24, 2024
 *      Author H2zero
 *
 */

#ifndef NIMBLE_CPP_ADVERTISEMENT_DATA_H_
#define NIMBLE_CPP_ADVERTISEMENT_DATA_H_

#include "nimconfig.h"
#if (defined(CONFIG_BT_ENABLED) && defined(CONFIG_BT_NIMBLE_ROLE_BROADCASTER) && !CONFIG_BT_NIMBLE_EXT_ADV) || \
    defined(_DOXYGEN_)

#include <cstdint>
#include <string>
#include <vector>

class NimBLEUUID;
/**
 * @brief Advertisement data set by the programmer to be published by the BLE server.
 */
class NimBLEAdvertisementData {
    // Only a subset of the possible BLE architected advertisement fields are currently exposed.  Others will
    // be exposed on demand/request or as time permits.
    //
  public:
    bool addData(const uint8_t* data, size_t length);
    bool addData(const std::vector<uint8_t>& data);
    bool setAppearance(uint16_t appearance);
    bool setFlags(uint8_t);
    bool addTxPower();
    bool setPreferredParams(uint16_t minInterval, uint16_t maxInterval);
    bool addServiceUUID(const NimBLEUUID& serviceUUID);
    bool addServiceUUID(const char* serviceUUID);
    bool removeServiceUUID(const NimBLEUUID& serviceUUID);
    bool removeServiceUUID(const char* serviceUUID);
    bool removeServices();
    bool setManufacturerData(const uint8_t* data, size_t length);
    bool setManufacturerData(const std::string& data);
    bool setManufacturerData(const std::vector<uint8_t>& data);
    bool setURI(const std::string& uri);
    bool setName(const std::string& name, bool isComplete = true);
    bool setShortName(const std::string& name);
    bool setCompleteServices(const NimBLEUUID& uuid);
    bool setCompleteServices16(const std::vector<NimBLEUUID>& uuids);
    bool setCompleteServices32(const std::vector<NimBLEUUID>& uuids);
    bool setPartialServices(const NimBLEUUID& uuid);
    bool setPartialServices16(const std::vector<NimBLEUUID>& uuids);
    bool setPartialServices32(const std::vector<NimBLEUUID>& uuids);
    bool setServiceData(const NimBLEUUID& uuid, const uint8_t* data, size_t length);
    bool setServiceData(const NimBLEUUID& uuid, const std::string& data);
    bool setServiceData(const NimBLEUUID& uuid, const std::vector<uint8_t>& data);
    bool removeData(uint8_t type);
    void clearData();
    int  getDataLocation(uint8_t type) const;

    std::string          toString() const;
    std::vector<uint8_t> getPayload() const;

  private:
    friend class NimBLEAdvertising;

    bool                 setServices(bool complete, uint8_t size, const std::vector<NimBLEUUID>& v_uuid);
    std::vector<uint8_t> m_payload{};
}; // NimBLEAdvertisementData

#endif // CONFIG_BT_ENABLED && CONFIG_BT_NIMBLE_ROLE_BROADCASTER && !CONFIG_BT_NIMBLE_EXT_ADV
#endif // NIMBLE_CPP_ADVERTISEMENT_DATA_H_
