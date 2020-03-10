/*
 * NimBLEDevice.h
 *
 *  Created: on Jan 24 2020
 *      Author H2zero
 * 
 * Originally:
 *
 * BLEDevice.h
 *
 *  Created on: Mar 16, 2017
 *      Author: kolban
 */

#ifndef MAIN_NIMBLEDEVICE_H_
#define MAIN_NIMBLEDEVICE_H_
#include "sdkconfig.h"
#if defined(CONFIG_BT_ENABLED)

#include "NimbleScan.h"
#include "NimBLEUtils.h"
#include "NimBLEClient.h"
#include "NimBLEServer.h"
#include "NimBLESecurity.h"

#include "esp_bt.h"

#include <map>
#include <string>
#include <list>

#define BLEDevice                       NimBLEDevice
#define BLEClient                       NimBLEClient
#define BLERemoteService                NimBLERemoteService
#define BLERemoteCharacteristic         NimBLERemoteCharacteristic
#define BLERemoteDescriptor             NimBLERemoteDescriptor
#define BLEAdvertisedDevice             NimBLEAdvertisedDevice
#define BLEScan                         NimBLEScan
#define BLEUUID                         NimBLEUUID
#define BLESecurity                     NimBLESecurity
#define BLESecurityCallbacks            NimBLESecurityCallbacks
#define BLEAddress                      NimBLEAddress
#define BLEUtils                        NimBLEUtils
#define BLEClientCallbacks              NimBLEClientCallbacks
#define BLEAdvertisedDeviceCallbacks    NimBLEAdvertisedDeviceCallbacks
#define BLEScanResults                  NimBLEScanResults
#define BLEServer                       NimBLEServer
#define BLEService                      NimBLEService
#define BLECharacteristic               NimBLECharacteristic
#define BLEAdvertising                  NimBLEAdvertising
#define BLEServerCallbacks              NimBLEServerCallbacks
#define BLECharacteristicCallbacks      NimBLECharacteristicCallbacks
#define BLEAdvertisementData			NimBLEAdvertisementData

/**
 * @brief BLE functions.
 */
 typedef void (*gap_event_handler)(ble_gap_event *event, void *arg);
//typedef void (*gattc_event_handler)(esp_gattc_cb_event_t event, esp_gatt_if_t gattc_if, esp_ble_gattc_cb_param_t* param);
//typedef void (*gatts_event_handler)(esp_gatts_cb_event_t event, esp_gatt_if_t gattc_if, esp_ble_gatts_cb_param_t* param);

extern "C" void ble_store_config_init(void);

class NimBLEDevice {
public:
    static void             init(std::string deviceName);   // Initialize the local BLE environment.
    static void             deinit();
    static bool             getInitialized();
    static NimBLEAddress    getAddress();
    static std::string      toString();
    static NimBLEScan*      getScan();                     // Get the scan object
    static NimBLEClient*    createClient();
	static NimBLEServer*    createServer();
    static bool             deleteClient(NimBLEClient* pClient);
    static void             setPower(esp_power_level_t powerLevel);
    static void             setCustomGapHandler(gap_event_handler handler);
    static void             setSecuityAuth(bool bonding, bool mitm, bool sc);
    static void             setSecuityAuth(uint8_t auth_req);
    static void             setSecurityIOCap(uint8_t iocap);
    static void             setsScurityInitKey(uint8_t init_key);
    static void             setsScurityRespKey(uint8_t init_key);
    static void             setSecurityPasskey(uint32_t pin);
    static uint32_t         getSecurityPasskey();
    static void             setSecurityCallbacks(NimBLESecurityCallbacks* pCallbacks);
    static int              setMTU(uint16_t mtu);
    static uint16_t         getMTU();
    static bool             isIgnored(NimBLEAddress address);
    static void             addIgnored(NimBLEAddress address);
    static void             removeIgnored(NimBLEAddress address);
    static NimBLEAdvertising* getAdvertising();
	static void		   		startAdvertising();
	static void		   		stopAdvertising();
    
    static std::list<NimBLEClient*>* getClientList(); 
        
private:
    friend class NimBLEClient;
    friend class NimBLEScan;
//  friend class NimBLERemoteService;
//  friend class NimBLERemoteCharacteristic;
    
    static void        onReset(int reason);
    static void        onSync(void);
    static void        host_task(void *param);
    static int         startSecurity(   uint16_t conn_id);
    
    static bool                       m_synced;
    static NimBLEScan*                m_pScan;
    static NimBLEServer*              m_pServer;
    static NimBLEAdvertising*         m_bleAdvertising;
    static ble_gap_event_listener     m_listener;
    static uint32_t                   m_passkey;
    static std::list <NimBLEClient*>  m_cList;
    static std::list <NimBLEAddress>  m_ignoreList;
    static NimBLESecurityCallbacks*   m_securityCallbacks;
    
public:
    static gap_event_handler          m_customGapHandler;
};


#endif // CONFIG_BT_ENABLED
#endif // MAIN_NIMBLEDEVICE_H_
