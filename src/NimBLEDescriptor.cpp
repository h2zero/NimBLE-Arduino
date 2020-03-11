/*
 * NimBLEDescriptor.cpp
 *
 *  Created: on March 10, 2020
 *      Author H2zero
 * 
 * Originally:
 *
 * BLEDescriptor.cpp
 *
 *  Created on: Jun 22, 2017
 *      Author: kolban
 */
#include "sdkconfig.h"
#if defined(CONFIG_BT_ENABLED)

#include "NimBLEService.h"
#include "NimBLEDescriptor.h"
#include "NimBLELog.h"

#include <string>

static const char* LOG_TAG = "NimBLEDescriptor";

#define NULL_HANDLE (0xffff)


/**
 * @brief NimBLEDescriptor constructor.
 */
NimBLEDescriptor::NimBLEDescriptor(const char* uuid, uint16_t len) : NimBLEDescriptor(NimBLEUUID(uuid), len) {
}	

/**
 * @brief NimBLEDescriptor constructor.
 */
NimBLEDescriptor::NimBLEDescriptor(NimBLEUUID uuid, uint16_t max_len) {
	m_uuid               = uuid;
	m_value.attr_len     = 0;                           // Initial length is 0.
	m_value.attr_max_len = max_len;                     // Maximum length of the data.
	m_handle             = NULL_HANDLE;                 // Handle is initially unknown.
	m_pCharacteristic    = nullptr;                     // No initial characteristic.
	m_pCallbacks         = nullptr;                     // No initial callback.

	m_value.attr_value   = (uint8_t*) malloc(max_len);  // Allocate storage for the value.
} // NimBLEDescriptor


/**
 * @brief NimBLEDescriptor destructor.
 */
NimBLEDescriptor::~NimBLEDescriptor() {
	free(m_value.attr_value);   // Release the storage we created in the constructor.
} // ~NimBLEDescriptor


/**
 * @brief Execute the creation of the descriptor with the BLE runtime in ESP.
 * @param [in] pCharacteristic The characteristic to which to register this descriptor.
 */
 /*
void NimBLEDescriptor::executeCreate(BLECharacteristic* pCharacteristic) {
	NIMBLE_LOGD(LOG_TAG, ">> executeCreate(): %s", toString().c_str());

	if (m_handle != NULL_HANDLE) {
		NIMBLE_LOGE(LOG_TAG, "Descriptor already has a handle.");
		return;
	}

	m_pCharacteristic = pCharacteristic; // Save the characteristic associated with this service.

	esp_attr_control_t control;
	control.auto_rsp = ESP_GATT_AUTO_RSP;
	m_semaphoreCreateEvt.take("executeCreate");
	esp_err_t errRc = ::esp_ble_gatts_add_char_descr(
			pCharacteristic->getService()->getHandle(),
			getUUID().getNative(),
			(esp_gatt_perm_t)m_permissions,
			&m_value,
			&control);
	if (errRc != ESP_OK) {
		NIMBLE_LOGE(LOG_TAG, "<< esp_ble_gatts_add_char_descr: rc=%d %s", errRc, GeneralUtils::errorToString(errRc));
		return;
	}

	m_semaphoreCreateEvt.wait("executeCreate");
	NIMBLE_LOGD(LOG_TAG, "<< executeCreate");
} // executeCreate
*/

/**
 * @brief Get the BLE handle for this descriptor.
 * @return The handle for this descriptor.
 */
uint16_t NimBLEDescriptor::getHandle() {
	return m_handle;
} // getHandle


/**
 * @brief Get the length of the value of this descriptor.
 * @return The length (in bytes) of the value of this descriptor.
 */
size_t NimBLEDescriptor::getLength() {
	return m_value.attr_len;
} // getLength


/**
 * @brief Get the UUID of the descriptor.
 */
NimBLEUUID NimBLEDescriptor::getUUID() {
	return m_uuid;
} // getUUID


/**
 * @brief Get the value of this descriptor.
 * @return A pointer to the value of this descriptor.
 */
uint8_t* NimBLEDescriptor::getValue() {
	return m_value.attr_value;
} // getValue


/**
 * @brief Handle GATT server events for the descripttor.
 * @param [in] event
 * @param [in] gatts_if
 * @param [in] param
 */
 /*
void NimBLEDescriptor::handleGATTServerEvent(
		esp_gatts_cb_event_t      event,
		esp_gatt_if_t             gatts_if,
		esp_ble_gatts_cb_param_t* param) {
	switch (event) {
		// ESP_GATTS_ADD_CHAR_DESCR_EVT
		//
		// add_char_descr:
		// - esp_gatt_status_t status
		// - uint16_t          attr_handle
		// - uint16_t          service_handle
		// - esp_bt_uuid_t     char_uuid
		case ESP_GATTS_ADD_CHAR_DESCR_EVT: {
			if (m_pCharacteristic != nullptr &&
					m_uuid.equals(NimBLEUUID(param->add_char_descr.descr_uuid)) &&
					m_pCharacteristic->getService()->getHandle() == param->add_char_descr.service_handle &&
					m_pCharacteristic == m_pCharacteristic->getService()->getLastCreatedCharacteristic()) {
				setHandle(param->add_char_descr.attr_handle);
				m_semaphoreCreateEvt.give();
			}
			break;
		} // ESP_GATTS_ADD_CHAR_DESCR_EVT

		// ESP_GATTS_WRITE_EVT - A request to write the value of a descriptor has arrived.
		//
		// write:
		// - uint16_t conn_id
		// - uint16_t trans_id
		// - esp_bd_addr_t bda
		// - uint16_t handle
		// - uint16_t offset
		// - bool need_rsp
		// - bool is_prep
		// - uint16_t len
		// - uint8_t *value
		case ESP_GATTS_WRITE_EVT: {
			if (param->write.handle == m_handle) {
				setValue(param->write.value, param->write.len);   // Set the value of the descriptor.

				if (m_pCallbacks != nullptr) {   // We have completed the write, if there is a user supplied callback handler, invoke it now.
					m_pCallbacks->onWrite(this);   // Invoke the onWrite callback handler.
				}
			}  // End of ... this is our handle.

			break;
		} // ESP_GATTS_WRITE_EVT

		// ESP_GATTS_READ_EVT - A request to read the value of a descriptor has arrived.
		//
		// read:
		// - uint16_t conn_id
		// - uint32_t trans_id
		// - esp_bd_addr_t bda
		// - uint16_t handle
		// - uint16_t offset
		// - bool is_long
		// - bool need_rsp
		//
		case ESP_GATTS_READ_EVT: {
			if (param->read.handle == m_handle) {  // If this event is for this descriptor ... process it

				if (m_pCallbacks != nullptr) {   // If we have a user supplied callback, invoke it now.
					m_pCallbacks->onRead(this);    // Invoke the onRead callback method in the callback handler.
				}

			} // End of this is our handle
			break;
		} // ESP_GATTS_READ_EVT

		default:
			break;
	} // switch event
} // handleGATTServerEvent
*/

/**
 * @brief Set the callback handlers for this descriptor.
 * @param [in] pCallbacks An instance of a callback structure used to define any callbacks for the descriptor.
 */
void NimBLEDescriptor::setCallbacks(NimBLEDescriptorCallbacks* pCallback) {
	m_pCallbacks = pCallback;
} // setCallbacks


/**
 * @brief Set the handle of this descriptor.
 * Set the handle of this descriptor to be the supplied value.
 * @param [in] handle The handle to be associated with this descriptor.
 * @return N/A.
 */
void NimBLEDescriptor::setHandle(uint16_t handle) {
	NIMBLE_LOGD(LOG_TAG, ">> setHandle(0x%.2x): Setting descriptor handle to be 0x%.2x", handle, handle);
	m_handle = handle;
	NIMBLE_LOGD(LOG_TAG, "<< setHandle()");
} // setHandle


/**
 * @brief Set the value of the descriptor.
 * @param [in] data The data to set for the descriptor.
 * @param [in] length The length of the data in bytes.
 */
void NimBLEDescriptor::setValue(uint8_t* data, size_t length) {
	if (length > BLE_ATT_ATTR_MAX_LEN) {
		NIMBLE_LOGE(LOG_TAG, "Size %d too large, must be no bigger than %d", length, BLE_ATT_ATTR_MAX_LEN);
		return;
	}
	m_value.attr_len = length;
	memcpy(m_value.attr_value, data, length);
} // setValue


/**
 * @brief Set the value of the descriptor.
 * @param [in] value The value of the descriptor in string form.
 */
void NimBLEDescriptor::setValue(std::string value) {
	setValue((uint8_t*) value.data(), value.length());
} // setValue

void NimBLEDescriptor::setAccessPermissions(uint8_t perm) {
	m_permissions = perm;
}

/**
 * @brief Return a string representation of the descriptor.
 * @return A string representation of the descriptor.
 */
std::string NimBLEDescriptor::toString() {
	char hex[5];
	snprintf(hex, sizeof(hex), "%04x", m_handle);
	std::string res = "UUID: " + m_uuid.toString() + ", handle: 0x" + hex;
	return res;
} // toString


NimBLEDescriptorCallbacks::~NimBLEDescriptorCallbacks() {}

/**
 * @brief Callback function to support a read request.
 * @param [in] pDescriptor The descriptor that is the source of the event.
 */
void NimBLEDescriptorCallbacks::onRead(NimBLEDescriptor* pDescriptor) {
	NIMBLE_LOGD("NimBLEDescriptorCallbacks", "onRead: default");
} // onRead


/**
 * @brief Callback function to support a write request.
 * @param [in] pDescriptor The descriptor that is the source of the event.
 */
void NimBLEDescriptorCallbacks::onWrite(NimBLEDescriptor* pDescriptor) {
	NIMBLE_LOGD("NimBLEDescriptorCallbacks", "onWrite: default");
} // onWrite


#endif /* CONFIG_BT_ENABLED */