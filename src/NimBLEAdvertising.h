/*
 * NimBLEAdvertising.h
 *
 *  Created: on March 3, 2020
 *      Author H2zero
 *
 * Originally:
 *
 * BLEAdvertising.h
 *
 *  Created on: Jun 21, 2017
 *      Author: kolban
 */

#ifndef NIMBLE_CPP_ADVERTISING_H_
#define NIMBLE_CPP_ADVERTISING_H_

#include "nimconfig.h"
#if (defined(CONFIG_BT_ENABLED) && defined(CONFIG_BT_NIMBLE_ROLE_BROADCASTER) && !CONFIG_BT_NIMBLE_EXT_ADV) || \
    defined(_DOXYGEN_)

# if defined(CONFIG_NIMBLE_CPP_IDF)
#  include "host/ble_gap.h"
# else
#  include "nimble/nimble/host/include/host/ble_gap.h"
# endif

/****  FIX COMPILATION ****/
# undef min
# undef max
/**************************/

# include "NimBLEUUID.h"
# include "NimBLEAddress.h"
# include "NimBLEAdvertisementData.h"

# include <functional>
# include <string>
# include <vector>

class NimBLEAdvertising;
typedef std::function<void(NimBLEAdvertising*)> advCompleteCB_t;

/**
 * @brief Perform and manage BLE advertising.
 *
 * A BLE server will want to perform advertising in order to make itself known to BLE clients.
 */
class NimBLEAdvertising {
  public:
    NimBLEAdvertising();
    bool start(uint32_t duration = 0, const NimBLEAddress* dirAddr = nullptr);
    void setAdvertisingCompleteCallback(advCompleteCB_t callback);
    bool stop();
    bool setConnectableMode(uint8_t mode);
    bool setDiscoverableMode(uint8_t mode);
    bool reset();
    bool isAdvertising();
    void setScanFilter(bool scanRequestWhitelistOnly, bool connectWhitelistOnly);
    void enableScanResponse(bool enable);
    void setAdvertisingInterval(uint16_t interval);
    void setMaxInterval(uint16_t maxInterval);
    void setMinInterval(uint16_t minInterval);

    bool                           setAdvertisementData(const NimBLEAdvertisementData& advertisementData);
    bool                           setScanResponseData(const NimBLEAdvertisementData& advertisementData);
    const NimBLEAdvertisementData& getAdvertisementData();
    const NimBLEAdvertisementData& getScanData();
    void                           clearData();
    bool                           refreshAdvertisingData();

    bool addServiceUUID(const NimBLEUUID& serviceUUID);
    bool addServiceUUID(const char* serviceUUID);
    bool removeServiceUUID(const NimBLEUUID& serviceUUID);
    bool removeServiceUUID(const char* serviceUUID);
    bool removeServices();
    bool setAppearance(uint16_t appearance);
    bool setPreferredParams(uint16_t minInterval, uint16_t maxInterval);
    bool addTxPower();
    bool setName(const std::string& name);
    bool setManufacturerData(const uint8_t* data, size_t length);
    bool setManufacturerData(const std::string& data);
    bool setManufacturerData(const std::vector<uint8_t>& data);
    bool setURI(const std::string& uri);
    bool setServiceData(const NimBLEUUID& uuid, const uint8_t* data, size_t length);
    bool setServiceData(const NimBLEUUID& uuid, const std::string& data);
    bool setServiceData(const NimBLEUUID& uuid, const std::vector<uint8_t>& data);

  private:
    friend class NimBLEDevice;
    friend class NimBLEServer;

    void       onHostSync();
    static int handleGapEvent(ble_gap_event* event, void* arg);

    NimBLEAdvertisementData m_advData;
    NimBLEAdvertisementData m_scanData;
    ble_gap_adv_params      m_advParams;
    advCompleteCB_t         m_advCompCb;
    uint8_t                 m_slaveItvl[4];
    uint32_t                m_duration;
    bool                    m_scanResp : 1;
    bool                    m_advDataSet : 1;
};

#endif /* CONFIG_BT_ENABLED && CONFIG_BT_NIMBLE_ROLE_BROADCASTER  && !CONFIG_BT_NIMBLE_EXT_ADV */
#endif /* NIMBLE_CPP_ADVERTISING_H_ */
