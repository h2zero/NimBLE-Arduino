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

#ifdef ARDUINO_ARCH_ESP32
#include "nimconfig.h"
#endif

#if defined( CONFIG_BT_NIMBLE_ROLE_CENTRAL)
#include "NimBLEScan.h"
#endif

#include "NimBLEUtils.h"
#include "NimBLESecurity.h"

#if defined( CONFIG_BT_NIMBLE_ROLE_CENTRAL)
#include "NimBLEClient.h"
#endif

#if defined(CONFIG_BT_NIMBLE_ROLE_PERIPHERAL)
#include "NimBLEServer.h"
#endif

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
#define BLEAdvertisementData		NimBLEAdvertisementData
#define BLEDescriptor			NimBLEDescriptor
#define BLE2902				NimBLE2902
#define BLE2904				NimBLE2904
#define BLEDescriptorCallbacks		NimBLEDescriptorCallbacks
#define BLEBeacon                       NimBLEBeacon
#define BLEEddystoneTLM                 NimBLEEddystoneTLM
#define BLEEddystoneURL                 NimBLEEddystoneURL

#ifdef CONFIG_BT_NIMBLE_MAX_CONNECTIONS
#define NIMBLE_MAX_CONNECTIONS          CONFIG_BT_NIMBLE_MAX_CONNECTIONS
#else 
#define NIMBLE_MAX_CONNECTIONS          CONFIG_NIMBLE_MAX_CONNECTIONS
#endif
    
/**
 * @brief BLE functions.
 */
 typedef int (*gap_event_handler)(ble_gap_event *event, void *arg);
//typedef void (*gattc_event_handler)(esp_gattc_cb_event_t event, esp_gatt_if_t gattc_if, esp_ble_gattc_cb_param_t* param);
//typedef void (*gatts_event_handler)(esp_gatts_cb_event_t event, esp_gatt_if_t gattc_if, esp_ble_gatts_cb_param_t* param);

extern "C" void ble_store_config_init(void);

class NimBLEDevice {
public:
    static void             init(const std::string &deviceName);   // Initialize the local BLE environment.
    static void             deinit();
    static bool             getInitialized();
    static NimBLEAddress    getAddress();
    static std::string      toString();
#if defined( CONFIG_BT_NIMBLE_ROLE_CENTRAL)
    static NimBLEScan*      getScan();                     // Get the scan object
    static NimBLEClient*    createClient();
#endif
#if defined(CONFIG_BT_NIMBLE_ROLE_PERIPHERAL)
    static NimBLEServer*    createServer();
#endif
#if defined( CONFIG_BT_NIMBLE_ROLE_CENTRAL)
    static bool             deleteClient(NimBLEClient* pClient);
#endif
    static void             setPower(esp_power_level_t powerLevel, esp_ble_power_type_t powerType=ESP_BLE_PWR_TYPE_DEFAULT);
    static int              getPower(esp_ble_power_type_t powerType=ESP_BLE_PWR_TYPE_DEFAULT);
    static void             setCustomGapHandler(gap_event_handler handler);
    static void             setSecurityAuth(bool bonding, bool mitm, bool sc);
    static void             setSecurityAuth(uint8_t auth_req);
    static void             setSecurityIOCap(uint8_t iocap);
    static void             setSecurityInitKey(uint8_t init_key);
    static void             setSecurityRespKey(uint8_t init_key);
    static void             setSecurityPasskey(uint32_t pin);
    static uint32_t         getSecurityPasskey();
    static void             setSecurityCallbacks(NimBLESecurityCallbacks* pCallbacks);
    static int              setMTU(uint16_t mtu);
    static uint16_t         getMTU();
#if defined( CONFIG_BT_NIMBLE_ROLE_CENTRAL)
    static bool             isIgnored(const NimBLEAddress &address);
    static void             addIgnored(const NimBLEAddress &address);
    static void             removeIgnored(const NimBLEAddress &address);
#endif
#if defined(CONFIG_BT_NIMBLE_ROLE_PERIPHERAL)
    static NimBLEAdvertising* getAdvertising();
    static void		    startAdvertising();
    static void		    stopAdvertising();
#endif

#if defined( CONFIG_BT_NIMBLE_ROLE_CENTRAL)
    static NimBLEClient*    getClientByID(uint16_t conn_id);
    static NimBLEClient*    getClientByPeerAddress(const NimBLEAddress &peer_addr);
    static NimBLEClient*    getDisconnectedClient();
    static size_t           getClientListSize(); 
    static std::list<NimBLEClient*>* getClientList(); 
#endif
        
private:
#if defined( CONFIG_BT_NIMBLE_ROLE_CENTRAL)
    friend class NimBLEClient;
    friend class NimBLEScan;
#endif
#if defined(CONFIG_BT_NIMBLE_ROLE_PERIPHERAL)
    friend class NimBLEServer;
    friend class NimBLEAdvertising;
    friend class NimBLECharacteristic;
#endif
    
    static void        onReset(int reason);
    static void        onSync(void);
    static void        host_task(void *param);
    static int         startSecurity(   uint16_t conn_id);
    
    static bool                       m_synced;
#if defined( CONFIG_BT_NIMBLE_ROLE_CENTRAL)
    static NimBLEScan*                m_pScan;
#endif

#if defined(CONFIG_BT_NIMBLE_ROLE_PERIPHERAL)
    static NimBLEServer*              m_pServer;
    static NimBLEAdvertising*         m_bleAdvertising;
#endif

    static ble_gap_event_listener     m_listener;
    static uint32_t                   m_passkey;
#if defined( CONFIG_BT_NIMBLE_ROLE_CENTRAL)
    static std::list <NimBLEClient*>  m_cList;
#endif
    static std::list <NimBLEAddress>  m_ignoreList;
    static NimBLESecurityCallbacks*   m_securityCallbacks;

public:
    static gap_event_handler          m_customGapHandler;
};


#endif // CONFIG_BT_ENABLED
#endif // MAIN_NIMBLEDEVICE_H_
