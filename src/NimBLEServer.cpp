/*
 * NimBLEServer.cpp
 *
 *  Created: on March 2, 2020
 *      Author H2zero
 * 
 * Originally:
 *
 * BLEServer.cpp
 *
 *  Created on: Apr 16, 2017
 *      Author: kolban
 */

#include "sdkconfig.h"
#if defined(CONFIG_BT_ENABLED)

#include "NimBLEServer.h"
#include "NimBLEUtils.h"
#include "NimBLEDevice.h"
#include "NimBLELog.h"

static const char* LOG_TAG = "NimBLEServer";


/**
 * @brief Construct a %BLE Server
 *
 * This class is not designed to be individually instantiated.  Instead one should create a server by asking
 * the BLEDevice class.
 */
NimBLEServer::NimBLEServer() {
//	m_appId            = ESP_GATT_IF_NONE;
//	m_gatts_if         = ESP_GATT_IF_NONE;
//	m_connectedCount   = 0;
	m_connId           = BLE_HS_CONN_HANDLE_NONE;
    m_svcChgChrHdl     = 0xffff;
	m_pServerCallbacks = nullptr;
	m_gattsStarted 	   = false;
} // BLEServer

/*
void BLEServer::createApp(uint16_t appId) {
	m_appId = appId;
	registerApp(appId);
} // createApp
*/

/**
 * @brief Create a %BLE Service.
 *
 * With a %BLE server, we can host one or more services.  Invoking this function causes the creation of a definition
 * of a new service.  Every service must have a unique UUID.
 * @param [in] uuid The UUID of the new service.
 * @return A reference to the new service object.
 */
 
NimBLEService* NimBLEServer::createService(const char* uuid) {
	return createService(NimBLEUUID(uuid));
}


/**
 * @brief Create a %BLE Service.
 *
 * With a %BLE server, we can host one or more services.  Invoking this function causes the creation of a definition
 * of a new service.  Every service must have a unique UUID.
 * @param [in] uuid The UUID of the new service.
 * @param [in] numHandles The maximum number of handles associated with this service.
 * @param [in] inst_id With multiple services with the same UUID we need to provide inst_id value different for each service.
 * @return A reference to the new service object.
 */
 
NimBLEService* NimBLEServer::createService(NimBLEUUID uuid, uint32_t numHandles, uint8_t inst_id) {
	NIMBLE_LOGD(LOG_TAG, ">> createService - %s", uuid.toString().c_str());
	//m_semaphoreCreateEvt.take("createService");

	// Check that a service with the supplied UUID does not already exist.
	if (m_serviceMap.getByUUID(uuid) != nullptr) {
		NIMBLE_LOGW(LOG_TAG, "<< Attempt to create a new service with uuid %s but a service with that UUID already exists.",
			uuid.toString().c_str());
	}

	NimBLEService* pService = new NimBLEService(uuid, numHandles, this);
	pService->m_instId = inst_id;
	m_serviceMap.setByUUID(uuid, pService); // Save a reference to this service being on this server.
//	pService->executeCreate(this);          // Perform the API calls to actually create the service.

//	m_semaphoreCreateEvt.wait("createService");

	NIMBLE_LOGD(LOG_TAG, "<< createService");
	return pService;
} // createService


/**
 * @brief Get a %BLE Service by its UUID
 * @param [in] uuid The UUID of the new service.
 * @return A reference to the service object.
 */
NimBLEService* NimBLEServer::getServiceByUUID(const char* uuid) {
	return m_serviceMap.getByUUID(uuid);
}


/**
 * @brief Get a %BLE Service by its UUID
 * @param [in] uuid The UUID of the new service.
 * @return A reference to the service object.
 */
NimBLEService* NimBLEServer::getServiceByUUID(NimBLEUUID uuid) {
	return m_serviceMap.getByUUID(uuid);
}


/**
 * @brief Retrieve the advertising object that can be used to advertise the existence of the server.
 *
 * @return An advertising object.
 */
NimBLEAdvertising* NimBLEServer::getAdvertising() {
	return BLEDevice::getAdvertising();
}


/**
 * @brief Retrieve the connection id of the last connected client.
 * @todo Not very useful, should refactor or remove.
 * @return Client connection id.
 */
uint16_t NimBLEServer::getConnId() {
	return m_connId;
}


/**
 * @brief Start the GATT server, required to be called after setup of all 
 * services and characteristics / descriptors for the NimBLE host to register them
 */
void NimBLEServer::start() {
	if(m_gattsStarted) {
		NIMBLE_LOGW(LOG_TAG, "Gatt server already started");
		return;
	}
	
    int rc = ble_gatts_start();
    if (rc != 0) {
        NIMBLE_LOGE(LOG_TAG, "ble_gatts_start; rc=%d, %s", rc, NimBLEUtils::returnCodeToString(rc));
        abort();
    }
    
    ble_gatts_show_local();
    
    ble_uuid16_t svc = {BLE_UUID_TYPE_16, 0x1801};
    ble_uuid16_t chr = {BLE_UUID_TYPE_16, 0x2a05};
    
    //int ble_gatts_find_chr(const ble_uuid_t * svc_uuid, const ble_uuid_t * chr_uuid, uint16_t * out_def_handle, uint16_t * out_val_handle)
    rc = ble_gatts_find_chr(&svc.u, &chr.u, NULL, &m_svcChgChrHdl); 
    if(rc != 0) {
        NIMBLE_LOGE(LOG_TAG, "ble_gatts_find_chr: rc=%d, %s", rc, NimBLEUtils::returnCodeToString(rc));
        abort();
    }
    
    NIMBLE_LOGI(LOG_TAG, "Service changed characterisic handle: %d", m_svcChgChrHdl);
	m_gattsStarted = true;
}


/**
 * @brief Disconnect the specified client with optional reason.
 * @param [in] Connection Id of the client to disconnect.
 * @param [in] Reason code for disconnecting.
 * @return NimBLE host return code.
 */
int NimBLEServer::disconnect(uint16_t connId, uint8_t reason) {
    NIMBLE_LOGD(LOG_TAG, ">> disconnect()");
    
    int rc = ble_gap_terminate(connId, reason);
    if(rc != 0){
        NIMBLE_LOGE(LOG_TAG, "ble_gap_terminate failed: rc=%d %s", rc, NimBLEUtils::returnCodeToString(rc));
    }
    
    return rc;
    NIMBLE_LOGD(LOG_TAG, "<< disconnect()");
}


/**
 * @brief Return the number of connected clients.
 * @return The number of connected clients.
 */
uint32_t NimBLEServer::getConnectedCount() {
	return m_connectedServersMap.size();
} // getConnectedCount


/**
 * @brief Handle a GATT Server Event.
 *
 * @param [in] event
 * @param [in] gatts_if
 * @param [in] param
 *
 */
/*STATIC*/int NimBLEServer::handleGapEvent(struct ble_gap_event *event, void *arg) {
	NimBLEServer* server = (NimBLEServer*)arg;
	NIMBLE_LOGD(LOG_TAG, ">> handleGapEvent: %s",
		NimBLEUtils::gapEventToString(event->type));

	switch(event->type) {
        
		case BLE_GAP_EVENT_CONNECT: {
            if (event->connect.status != 0) {
                /* Connection failed; resume advertising */
                NimBLEDevice::startAdvertising();
                server->m_connId = BLE_HS_CONN_HANDLE_NONE;
            }
            else {
                server->m_connId = event->connect.conn_handle;
                server->addPeerDevice((void*)server, false, server->m_connId);
                if (server->m_pServerCallbacks != nullptr) {
                    server->m_pServerCallbacks->onConnect(server);
                    //m_pServerCallbacks->onConnect(this, param);			
                }
            }

            break;
		} // BLE_GAP_EVENT_CONNECT
        
        
        case BLE_GAP_EVENT_DISCONNECT: {
            server->m_connId = BLE_HS_CONN_HANDLE_NONE;
            if (server->m_pServerCallbacks != nullptr) {       
				server->m_pServerCallbacks->onDisconnect(server);
			}
            /* Connection terminated; resume advertising */
            //NimBLEDevice::startAdvertising();
            server->removePeerDevice(event->disconnect.conn.conn_handle, false);
            break;
        } // BLE_GAP_EVENT_DISCONNECT
        
        case BLE_GAP_EVENT_SUBSCRIBE: {
            NIMBLE_LOGI(LOG_TAG, "subscribe event; cur_notify=%d\n value handle; "
                              "val_handle=%d\n",
                        event->subscribe.cur_notify, event->subscribe.attr_handle);
                     
            NimBLECharacteristic* pChr = server->getChrByHandle(event->subscribe.attr_handle);
            if(pChr != nullptr) {
                pChr->setSubscribe(event);
            }
        /*    
            uint8_t numSvcs = server->m_serviceMap.getRegisteredServiceCount();
            NimBLEService* pService = server->m_serviceMap.getFirst();
            
            for(int i = 0; i < numSvcs; i++) {
                uint8_t numChrs = pService->m_characteristicMap.getSize();
                NimBLECharacteristic* pChr = pService->m_characteristicMap.getFirst(); 
                
                for( int d = 0; d < numChrs; d++) {
                    if(pChr->m_handle == event->subscribe.attr_handle) {
                        pChr->setSubscribe(event);
                        return 0;
                    }
                    pChr = pService->m_characteristicMap.getNext();
                }
                
                pService = server->m_serviceMap.getNext();
            }
            
            NIMBLE_LOGE(LOG_TAG, "Subscribe handle not found");
        */
            break;
        } // BLE_GAP_EVENT_SUBSCRIBE
 
        case BLE_GAP_EVENT_MTU: {
            NIMBLE_LOGI(LOG_TAG, "mtu update event; conn_handle=%d mtu=%d",
                        event->mtu.conn_handle,
                        event->mtu.value);
            server->updatePeerMTU(event->mtu.conn_handle, event->mtu.value);
            break;
        } // BLE_GAP_EVENT_MTU
        
        case BLE_GAP_EVENT_NOTIFY_TX: {
            if(event->notify_tx.indication && event->notify_tx.status != 0) {
                NimBLECharacteristic* pChr = server->getChrByHandle(event->notify_tx.attr_handle);
                if(pChr != nullptr) {
                    pChr->m_semaphoreConfEvt.give(event->notify_tx.status);
                }
            /*    
                uint8_t numSvcs = server->m_serviceMap.getRegisteredServiceCount();
                NimBLEService* pService = server->m_serviceMap.getFirst();
                
                for(int i = 0; i < numSvcs; i++) {
                    uint8_t numChrs = pService->m_characteristicMap.getSize();
                    NimBLECharacteristic* pChr = pService->m_characteristicMap.getFirst(); 
                    
                    for( int d = 0; d < numChrs; d++) {
                        if(pChr->m_handle == event->notify_tx.attr_handle) {
                            pChr->m_semaphoreConfEvt.give(event->notify_tx.status);
                            return 0;
                        }
                        pChr = pService->m_characteristicMap.getNext();
                    }
                    
                    pService = server->m_serviceMap.getNext();
                }
                
                NIMBLE_LOGE(LOG_TAG, "Subscribe handle not found");
            */
            }
            break;
        } // BLE_GAP_EVENT_NOTIFY_TX

		default:
			break;
	}

	NIMBLE_LOGD(LOG_TAG, "<< handleGATTServerEvent");
    return 0;
} // handleGATTServerEvent


/**
 * @brief Set the server callbacks.
 *
 * As a %BLE server operates, it will generate server level events such as a new client connecting or a previous client
 * disconnecting.  This function can be called to register a callback handler that will be invoked when these
 * events are detected.
 *
 * @param [in] pCallbacks The callbacks to be invoked.
 */
void NimBLEServer::setCallbacks(NimBLEServerCallbacks* pCallbacks) {
	m_pServerCallbacks = pCallbacks;
} // setCallbacks

/*
 * Remove service
 */
/*
void BLEServer::removeService(BLEService* service) {
	service->stop();
	service->executeDelete();	
	m_serviceMap.removeService(service);
}
*/
/**
 * @brief Start advertising.
 *
 * Start the server advertising its existence.  This is a convenience function and is equivalent to
 * retrieving the advertising object and invoking start upon it.
 */
void NimBLEServer::startAdvertising() {
	NIMBLE_LOGD(LOG_TAG, ">> startAdvertising");
	NimBLEDevice::startAdvertising();
	NIMBLE_LOGD(LOG_TAG, "<< startAdvertising");
} // startAdvertising


/**
 * @brief Stop advertising.
 */
void NimBLEServer::stopAdvertising() {
	NIMBLE_LOGD(LOG_TAG, ">> stopAdvertising");
	NimBLEDevice::stopAdvertising();
	NIMBLE_LOGD(LOG_TAG, "<< stopAdvertising");
} // startAdvertising


NimBLECharacteristic* NimBLEServer::getChrByHandle(uint16_t handle) {
    if(handle == m_svcChgChrHdl) {
        return nullptr;
    }
    uint8_t numSvcs = m_serviceMap.getRegisteredServiceCount();
    NimBLEService* pService = m_serviceMap.getFirst();
    
    for(int i = 0; i < numSvcs; i++) {
        uint8_t numChrs = pService->m_characteristicMap.getSize();
        NimBLECharacteristic* pChr = pService->m_characteristicMap.getFirst(); 
        
        for( int d = 0; d < numChrs; d++) {
            if(pChr->m_handle == handle) {
                return pChr;
            }
            pChr = pService->m_characteristicMap.getNext();
        }
        
        pService = m_serviceMap.getNext();
    }
    
    NIMBLE_LOGE(LOG_TAG, "Characteristic by handle not found");
    return nullptr;
}
    
/**
 * Allow to connect GATT server to peer device
 * Probably can be used in ANCS for iPhone
 */
 /*
bool BLEServer::connect(BLEAddress address) {
	esp_bd_addr_t addr;
	memcpy(&addr, address.getNative(), 6);
	// Perform the open connection request against the target BLE Server.
	m_semaphoreOpenEvt.take("connect");
	esp_err_t errRc = ::esp_ble_gatts_open(
		getGattsIf(),
		addr, // address
		1                              // direct connection
	);
	if (errRc != ESP_OK) {
		ESP_LOGE(LOG_TAG, "esp_ble_gattc_open: rc=%d %s", errRc, GeneralUtils::errorToString(errRc));
		return false;
	}

	uint32_t rc = m_semaphoreOpenEvt.wait("connect");   // Wait for the connection to complete.
	ESP_LOGD(LOG_TAG, "<< connect(), rc=%d", rc==ESP_GATT_OK);
	return rc == ESP_GATT_OK;
} // connect
*/


void NimBLEServerCallbacks::onConnect(NimBLEServer* pServer) {
	NIMBLE_LOGD("BLEServerCallbacks", ">> onConnect(): Default");
	NIMBLE_LOGD("BLEServerCallbacks", "Device: %s", NimBLEDevice::toString().c_str());
	NIMBLE_LOGD("BLEServerCallbacks", "<< onConnect()");
} // onConnect

/*
void NimBLEServerCallbacks::onConnect(NimBLEServer* pServer, esp_ble_gatts_cb_param_t* param) {
	NIMBLE_LOGD("BLEServerCallbacks", ">> onConnect(): Default");
	NIMBLE_LOGD("BLEServerCallbacks", "Device: %s", NimBLEDevice::toString().c_str());
	NIMBLE_LOGD("BLEServerCallbacks", "<< onConnect()");
} // onConnect
*/

void NimBLEServerCallbacks::onDisconnect(NimBLEServer* pServer) {
	NIMBLE_LOGD("BLEServerCallbacks", ">> onDisconnect(): Default");
	NIMBLE_LOGD("BLEServerCallbacks", "Device: %s", NimBLEDevice::toString().c_str());
	NIMBLE_LOGD("BLEServerCallbacks", "<< onDisconnect()");
} // onDisconnect


/* multi connect support */
void NimBLEServer::updatePeerMTU(uint16_t conn_id, uint16_t mtu) {
	const std::map<uint16_t, conn_status_t>::iterator it = m_connectedServersMap.find(conn_id);
	if (it != m_connectedServersMap.end()) {
		it->second.mtu = mtu;
	}
}

std::map<uint16_t, conn_status_t> NimBLEServer::getPeerDevices() {
	return m_connectedServersMap;
}


/**
 * @brief Get the MTU of the client.
 * @returns The client MTU or 0 if not found/connected.
 */
uint16_t NimBLEServer::getPeerMTU(uint16_t conn_id) {
    auto it = m_connectedServersMap.find(conn_id);
    if(it != m_connectedServersMap.cend()) {
        return (*it).second.mtu;
    } else {
        return 0;
    }
}

void NimBLEServer::addPeerDevice(void* peer, bool _client, uint16_t conn_id) {
	conn_status_t status = {
		.peer_device = peer,
		.connected = true,
		.mtu = 23
	};

	m_connectedServersMap.insert(std::pair<uint16_t, conn_status_t>(conn_id, status));	
}

void NimBLEServer::removePeerDevice(uint16_t conn_id, bool _client) {
	m_connectedServersMap.erase(conn_id);
}
/* multi connect support */


/**
 * Update connection parameters can be called only after connection has been established
 */
 /*
void BLEServer::updateConnParams(esp_bd_addr_t remote_bda, uint16_t minInterval, uint16_t maxInterval, uint16_t latency, uint16_t timeout) {
	esp_ble_conn_update_params_t conn_params;
	memcpy(conn_params.bda, remote_bda, sizeof(esp_bd_addr_t));
	conn_params.latency = latency;
	conn_params.max_int = maxInterval;    // max_int = 0x20*1.25ms = 40ms
	conn_params.min_int = minInterval;    // min_int = 0x10*1.25ms = 20ms
	conn_params.timeout = timeout;    // timeout = 400*10ms = 4000ms
	esp_ble_gap_update_conn_params(&conn_params); 
}
*/

#endif // CONFIG_BT_ENABLED