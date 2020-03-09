/*
 * NimBLEService.h
 *
 *  Created: on March 2, 2020
 *      Author H2zero
 * 
 * Originally:
 *
 * BLEService.h
 *
 *  Created on: Mar 25, 2017
 *      Author: kolban
 */

#ifndef MAIN_NIMBLESERVICE_H_
#define MAIN_NIMBLESERVICE_H_
#include "sdkconfig.h"
#if defined(CONFIG_BT_ENABLED)

//#include <esp_gatts_api.h>

#include "NimBLEServer.h"
#include "NimBLEUUID.h"
#include "FreeRTOS.h"
#include "NimBLECharacteristic.h"

class NimBLEServer;
class NimBLECharacteristic;

/**
 * @brief A data mapping used to manage the set of %BLE characteristics known to the server.
 */
class NimBLECharacteristicMap {
public:
	void setByUUID(NimBLECharacteristic* pCharacteristic, const char* uuid);
	void setByUUID(NimBLECharacteristic* pCharacteristic, NimBLEUUID uuid);
	void setByHandle(uint16_t handle, NimBLECharacteristic* pCharacteristic);
	NimBLECharacteristic* getByUUID(const char* uuid);	
	NimBLECharacteristic* getByUUID(NimBLEUUID uuid);
	NimBLECharacteristic* getByHandle(uint16_t handle);
	NimBLECharacteristic* getFirst();
	NimBLECharacteristic* getNext();
	uint8_t getSize();
	std::string toString();
//	void handleGATTServerEvent(esp_gatts_cb_event_t event, esp_gatt_if_t gatts_if, esp_ble_gatts_cb_param_t* param);

private:
	std::map<NimBLECharacteristic*, std::string> m_uuidMap;
	std::map<uint16_t, NimBLECharacteristic*> m_handleMap;
	std::map<NimBLECharacteristic*, std::string>::iterator m_iterator;
};


/**
 * @brief The model of a %BLE service.
 *
 */
class NimBLEService {
public:
	void               addCharacteristic(NimBLECharacteristic* pCharacteristic);
	NimBLECharacteristic* createCharacteristic(const char* uuid, uint32_t properties);
	NimBLECharacteristic* createCharacteristic(NimBLEUUID uuid, uint32_t properties);
	void               dump();
//	void               executeCreate(NimBLEServer* pServer);
//	void			   executeDelete();
	NimBLECharacteristic* getCharacteristic(const char* uuid);
	NimBLECharacteristic* getCharacteristic(NimBLEUUID uuid);
	NimBLEUUID            getUUID();
	NimBLEServer*         getServer();
	bool               start();
//	void			   stop();
	std::string        toString();
	uint16_t           getHandle();
	uint8_t			   m_instId = 0;

private:
	NimBLEService(const char* uuid, uint16_t numHandles, NimBLEServer* pServer);
	NimBLEService(NimBLEUUID uuid, uint16_t numHandles, NimBLEServer* pServer);
	friend class NimBLEServer;
//	friend class BLEServiceMap;
//	friend class BLEDescriptor;
//	friend class BLECharacteristic;
	friend class NimBLEDevice;

	NimBLECharacteristicMap m_characteristicMap;
	uint16_t             m_handle;
	NimBLECharacteristic*   m_lastCreatedCharacteristic = nullptr;
	NimBLEServer*           m_pServer = nullptr;
	NimBLEUUID              m_uuid;

//	FreeRTOS::Semaphore  m_semaphoreCreateEvt = FreeRTOS::Semaphore("CreateEvt");
//	FreeRTOS::Semaphore  m_semaphoreDeleteEvt = FreeRTOS::Semaphore("DeleteEvt");
//	FreeRTOS::Semaphore  m_semaphoreStartEvt  = FreeRTOS::Semaphore("StartEvt");
//	FreeRTOS::Semaphore  m_semaphoreStopEvt   = FreeRTOS::Semaphore("StopEvt");

	uint16_t             m_numHandles;

	NimBLECharacteristic* getLastCreatedCharacteristic();
//	void handleGATTServerEvent(esp_gatts_cb_event_t event, esp_gatt_if_t gatts_if, esp_ble_gatts_cb_param_t* param);
	void               setHandle(uint16_t handle);
	//void               setService(esp_gatt_srvc_id_t srvc_id);
}; // BLEService


#endif // CONFIG_BT_ENABLED
#endif /* MAIN_NIMBLESERVICE_H_ */