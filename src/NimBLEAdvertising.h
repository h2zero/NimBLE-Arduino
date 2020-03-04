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

#ifndef MAIN_BLEADVERTISING_H_
#define MAIN_BLEADVERTISING_H_
#include "sdkconfig.h"
#if defined(CONFIG_BT_ENABLED)
//#include <esp_gap_ble_api.h>
#include "NimBLEUUID.h"
#include "FreeRTOS.h"

#include <vector>


/**
 * @brief Advertisement data set by the programmer to be published by the %BLE server.
 */
class NimBLEAdvertisementData {
	// Only a subset of the possible BLE architected advertisement fields are currently exposed.  Others will
	// be exposed on demand/request or as time permits.
	//
public:
	void setAppearance(uint16_t appearance);
	void setCompleteServices(NimBLEUUID uuid);
	void setFlags(uint8_t);
	void setManufacturerData(std::string data);
	void setName(std::string name);
	void setPartialServices(NimBLEUUID uuid);
	void setServiceData(NimBLEUUID uuid, std::string data);
	void setShortName(std::string name);
	void        addData(std::string data);  // Add data to the payload.
	std::string getPayload();               // Retrieve the current advert payload.

private:
	friend class NimBLEAdvertising;
	std::string m_payload;   // The payload of the advertisement.
};   // NimBLEAdvertisementData


/**
 * @brief Perform and manage %BLE advertising.
 *
 * A %BLE server will want to perform advertising in order to make itself known to %BLE clients.
 */
class NimBLEAdvertising {
public:
	NimBLEAdvertising();
	void addServiceUUID(NimBLEUUID serviceUUID);
	void addServiceUUID(const char* serviceUUID);
	void start();
	void stop();
	void setAppearance(uint16_t appearance);
	void setMaxInterval(uint16_t maxinterval);
	void setMinInterval(uint16_t mininterval);
	void setAdvertisementData(NimBLEAdvertisementData& advertisementData);
	void setScanFilter(bool scanRequertWhitelistOnly, bool connectWhitelistOnly);
	void setScanResponseData(NimBLEAdvertisementData& advertisementData);
	void setPrivateAddress(uint8_t type = BLE_ADDR_TYPE_RANDOM);

	//void handleGAPEvent(esp_gap_ble_cb_event_t  event, esp_ble_gap_cb_param_t* param);
	void setMinPreferred(uint16_t);
	void setMaxPreferred(uint16_t);
	void setScanResponse(bool);

private:
	ble_hs_adv_fields    m_advData;
	ble_gap_adv_params   m_advParams;
	std::vector<NimBLEUUID> m_serviceUUIDs;
	bool                 m_customAdvData = false;  // Are we using custom advertising data?
	bool                 m_customScanResponseData = false;  // Are we using custom scan response data?
//	FreeRTOS::Semaphore  m_semaphoreSetAdv = FreeRTOS::Semaphore("startAdvert");
	bool				m_scanResp = true;

};
#endif /* CONFIG_BT_ENABLED */
#endif /* MAIN_BLEADVERTISING_H_ */