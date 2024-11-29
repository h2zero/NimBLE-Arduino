/*
 * NimBLEBeacon.h
 *
 *  Created: on March 15 2020
 *      Author H2zero
 *
 * Originally:
 *
 * BLEBeacon.h
 *
 *  Created on: Jan 4, 2018
 *      Author: kolban
 */

#ifndef NIMBLE_CPP_BEACON_H_
#define NIMBLE_CPP_BEACON_H_

#include "nimconfig.h"
#if defined(CONFIG_BT_ENABLED) && defined(CONFIG_BT_NIMBLE_ROLE_BROADCASTER)

class NimBLEUUID;

# include <cstdint>

/**
 * @brief Representation of a beacon.
 * See:
 * * https://en.wikipedia.org/wiki/IBeacon
 */
class NimBLEBeacon {
  public:
    struct BeaconData {
        uint16_t manufacturerId{0x4c00};
        uint8_t  subType{0x02};
        uint8_t  subTypeLength{0x15};
        uint8_t  proximityUUID[16]{};
        uint16_t major{};
        uint16_t minor{};
        int8_t   signalPower{};
    } __attribute__((packed));

    const BeaconData& getData();
    uint16_t          getMajor();
    uint16_t          getMinor();
    uint16_t          getManufacturerId();
    NimBLEUUID        getProximityUUID();
    int8_t            getSignalPower();
    void              setData(const uint8_t* data, uint8_t length);
    void              setData(const BeaconData& data);
    void              setMajor(uint16_t major);
    void              setMinor(uint16_t minor);
    void              setManufacturerId(uint16_t manufacturerId);
    void              setProximityUUID(const NimBLEUUID& uuid);
    void              setSignalPower(int8_t signalPower);

  private:
    BeaconData m_beaconData;
}; // NimBLEBeacon

#endif // NIMBLE_CPP_BEACON_H_
#endif // CONFIG_BT_ENABLED && CONFIG_BT_NIMBLE_ROLE_PERIPHERAL
