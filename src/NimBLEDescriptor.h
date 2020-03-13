/*
 * NimBLEDescriptor.h
 *
 *  Created: on March 10, 2020
 *      Author H2zero
 * 
 * Originally:
 *
 * BLEDescriptor.h
 *
 *  Created on: Jun 22, 2017
 *      Author: kolban
 */

#ifndef MAIN_NIMBLEDESCRIPTOR_H_
#define MAIN_NIMBLEDESCRIPTOR_H_
#include "sdkconfig.h"
#if defined(CONFIG_BT_ENABLED)
#include <string>
#include "NimBLEUUID.h"
#include "NimBLECharacteristic.h"
#include "FreeRTOS.h"


typedef struct
{
    uint16_t attr_max_len;                                  /*!<  attribute max value length */
    uint16_t attr_len;                                      /*!<  attribute current value length */
    uint8_t  *attr_value;                                   /*!<  the pointer to attribute value */
} attr_value_t;

typedef attr_value_t esp_attr_value_t; /*!< compatibility for esp32 */

class NimBLEService;
class NimBLECharacteristic;
class NimBLEDescriptorCallbacks;

/**
 * @brief A model of a %BLE descriptor.
 */
class NimBLEDescriptor {
public:
	NimBLEDescriptor(const char* uuid, uint16_t max_len = 100);
	NimBLEDescriptor(NimBLEUUID uuid, uint16_t max_len = 100);
	virtual ~NimBLEDescriptor();

	uint16_t getHandle();                                   // Get the handle of the descriptor.
	size_t   getLength();                                   // Get the length of the value of the descriptor.
	NimBLEUUID  getUUID();                                     // Get the UUID of the descriptor.
	uint8_t* getValue();                                    // Get a pointer to the value of the descriptor.
/*	void handleGATTServerEvent(
			esp_gatts_cb_event_t      event,
			esp_gatt_if_t             gatts_if,
			esp_ble_gatts_cb_param_t* param);
*/
	void setAccessPermissions(uint8_t perm);	      // Set the permissions of the descriptor.
	void setCallbacks(NimBLEDescriptorCallbacks* pCallbacks);  // Set callbacks to be invoked for the descriptor.
	void setValue(uint8_t* data, size_t size);              // Set the value of the descriptor as a pointer to data.
	void setValue(std::string value);                       // Set the value of the descriptor as a data buffer.

	std::string toString();                                 // Convert the descriptor to a string representation.

private:
	friend class NimBLEDescriptorMap;
	friend class NimBLECharacteristic;
    friend class NimBLEService;
    
	NimBLEUUID                 m_uuid;
	uint16_t                   m_handle;
	NimBLEDescriptorCallbacks* m_pCallbacks;
	NimBLECharacteristic*      m_pCharacteristic;
	uint8_t     			   m_permissions = BLE_GATT_CHR_PROP_READ | BLE_GATT_CHR_PROP_WRITE;
//	FreeRTOS::Semaphore     m_semaphoreCreateEvt = FreeRTOS::Semaphore("CreateEvt");
	attr_value_t               m_value;

//	void executeCreate(BLECharacteristic* pCharacteristic);
	static int handleGapEvent(uint16_t conn_handle, uint16_t attr_handle,
                           struct ble_gatt_access_ctxt *ctxt, void *arg);
                           
	void setHandle(uint16_t handle);
}; // BLEDescriptor


/**
 * @brief Callbacks that can be associated with a %BLE descriptors to inform of events.
 *
 * When a server application creates a %BLE descriptor, we may wish to be informed when there is either
 * a read or write request to the descriptors value.  An application can register a
 * sub-classed instance of this class and will be notified when such an event happens.
 */
class NimBLEDescriptorCallbacks {
public:
	virtual ~NimBLEDescriptorCallbacks();
	virtual void onRead(NimBLEDescriptor* pDescriptor);
	virtual void onWrite(NimBLEDescriptor* pDescriptor);
};
#endif /* CONFIG_BT_ENABLED */
#endif /* MAIN_NIMBLEDESCRIPTOR_H_ */