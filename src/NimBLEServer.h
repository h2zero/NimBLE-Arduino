/*
 * NimBLEServer.h
 *
 *  Created: on March 2, 2020
 *      Author H2zero
 *
 * Originally:
 *
 * BLEServer.h
 *
 *  Created on: Apr 16, 2017
 *      Author: kolban
 */

#ifndef NIMBLE_CPP_SERVER_H_
#define NIMBLE_CPP_SERVER_H_

#include "nimconfig.h"
#if defined(CONFIG_BT_ENABLED) && defined(CONFIG_BT_NIMBLE_ROLE_PERIPHERAL)

# if defined(CONFIG_NIMBLE_CPP_IDF)
#  include "host/ble_gap.h"
# else
#  include "nimble/nimble/host/include/host/ble_gap.h"
# endif

/****  FIX COMPILATION ****/
# undef min
# undef max
/**************************/

# include <string>
# include <vector>
# include <array>

# define NIMBLE_ATT_REMOVE_HIDE   1
# define NIMBLE_ATT_REMOVE_DELETE 2

class NimBLEService;
class NimBLEServerCallbacks;
class NimBLEUUID;
class NimBLEConnInfo;
class NimBLEAddress;
class NimBLEService;
class NimBLECharacteristic;
# if CONFIG_BT_NIMBLE_EXT_ADV
class NimBLEExtAdvertising;
# else
class NimBLEAdvertising;
# endif

/**
 * @brief The model of a BLE server.
 */
class NimBLEServer {
  public:
    void    start();
    uint8_t getConnectedCount() const;
    bool    disconnect(uint16_t connHandle, uint8_t reason = BLE_ERR_REM_USER_CONN_TERM) const;
    bool    disconnect(const NimBLEConnInfo& connInfo, uint8_t reason = BLE_ERR_REM_USER_CONN_TERM) const;
    void    setCallbacks(NimBLEServerCallbacks* pCallbacks, bool deleteCallbacks = true);
    void updateConnParams(uint16_t connHandle, uint16_t minInterval, uint16_t maxInterval, uint16_t latency, uint16_t timeout) const;
    NimBLEService*        createService(const char* uuid);
    NimBLEService*        createService(const NimBLEUUID& uuid);
    NimBLEService*        getServiceByUUID(const char* uuid, uint16_t instanceId = 0) const;
    NimBLEService*        getServiceByUUID(const NimBLEUUID& uuid, uint16_t instanceId = 0) const;
    NimBLEService*        getServiceByHandle(uint16_t handle) const;
    void                  removeService(NimBLEService* service, bool deleteSvc = false);
    void                  addService(NimBLEService* service);
    uint16_t              getPeerMTU(uint16_t connHandle) const;
    std::vector<uint16_t> getPeerDevices() const;
    NimBLEConnInfo        getPeerInfo(uint8_t index) const;
    NimBLEConnInfo        getPeerInfo(const NimBLEAddress& address) const;
    NimBLEConnInfo        getPeerInfoByHandle(uint16_t connHandle) const;
    std::string           getPeerName(const NimBLEConnInfo& connInfo) const;
    void                  getPeerNameOnConnect(bool enable);
    void                  advertiseOnDisconnect(bool enable);
    void                  setDataLen(uint16_t connHandle, uint16_t tx_octets) const;

# if CONFIG_BT_NIMBLE_EXT_ADV
    NimBLEExtAdvertising* getAdvertising() const;
    bool                  startAdvertising(uint8_t instanceId, int duration = 0, int maxEvents = 0) const;
    bool                  stopAdvertising(uint8_t instanceId) const;
    bool                  updatePhy(uint16_t connHandle, uint8_t txPhysMask, uint8_t rxPhysMask, uint16_t phyOptions);
    bool                  getPhy(uint16_t connHandle, uint8_t* txPhy, uint8_t* rxPhy);
# endif

# if !CONFIG_BT_NIMBLE_EXT_ADV || defined(_DOXYGEN_)
    NimBLEAdvertising* getAdvertising() const;
    bool               startAdvertising(uint32_t duration = 0) const;
    bool               stopAdvertising() const;
# endif

  private:
    friend class NimBLEDevice;
    friend class NimBLEService;
    friend class NimBLECharacteristic;
# if CONFIG_BT_NIMBLE_EXT_ADV
    friend class NimBLEExtAdvertising;
# else
    friend class NimBLEAdvertising;
# endif

    NimBLEServer();
    ~NimBLEServer();

    bool m_gattsStarted : 1;
    bool m_getPeerNameOnConnect : 1;
    bool m_svcChanged : 1;
    bool m_deleteCallbacks : 1;
# if !CONFIG_BT_NIMBLE_EXT_ADV
    bool m_advertiseOnDisconnect : 1;
# endif
    NimBLEServerCallbacks*                                 m_pServerCallbacks;
    std::vector<NimBLEService*>                            m_svcVec;
    std::array<uint16_t, CONFIG_BT_NIMBLE_MAX_CONNECTIONS> m_connectedPeers;

    static int  handleGapEvent(struct ble_gap_event* event, void* arg);
    static int  handleGattEvent(uint16_t connHandle, uint16_t attrHandle, ble_gatt_access_ctxt* ctxt, void* arg);
    static int  peerNameCB(uint16_t connHandle, const ble_gatt_error* error, ble_gatt_attr* attr, void* arg);
    std::string getPeerNameImpl(uint16_t connHandle, int cb_type = -1) const;
    void        serviceChanged();
    void        resetGATT();

}; // NimBLEServer

/**
 * @brief Callbacks associated with the operation of a %BLE server.
 */
class NimBLEServerCallbacks {
  public:
    virtual ~NimBLEServerCallbacks() {};

    /**
     * @brief Handle a client connection.
     * This is called when a client connects.
     * @param [in] pServer A pointer to the %BLE server that received the client connection.
     * @param [in] connInfo A reference to a NimBLEConnInfo instance with information.
     * about the peer connection parameters.
     */
    virtual void onConnect(NimBLEServer* pServer, NimBLEConnInfo& connInfo);

    /**
     * @brief Handle a client connection.
     * This is called when a client connects.
     * @param [in] pServer A pointer to the %BLE server that received the client connection.
     * @param [in] connInfo A reference to a NimBLEConnInfo instance with information.
     * @param [in] name The name of the connected peer device.
     * about the peer connection parameters.
     */
    virtual void onConnect(NimBLEServer* pServer, NimBLEConnInfo& connInfo, std::string& name);

    /**
     * @brief Handle a client disconnection.
     * This is called when a client discconnects.
     * @param [in] pServer A pointer to the %BLE server that received the client disconnection.
     * @param [in] connInfo A reference to a NimBLEConnInfo instance with information
     * about the peer connection parameters.
     * @param [in] reason The reason code for the disconnection.
     */
    virtual void onDisconnect(NimBLEServer* pServer, NimBLEConnInfo& connInfo, int reason);

    /**
     * @brief Called when the connection MTU changes.
     * @param [in] MTU The new MTU value.
     * @param [in] connInfo A reference to a NimBLEConnInfo instance with information
     * about the peer connection parameters.
     */
    virtual void onMTUChange(uint16_t MTU, NimBLEConnInfo& connInfo);

    /**
     * @brief Called when a client requests a passkey for pairing (display).
     * @return The passkey to be sent to the client.
     */
    virtual uint32_t onPassKeyDisplay();

    /**
     * @brief Called when using numeric comparision for pairing.
     * @param [in] connInfo A reference to a NimBLEConnInfo instance with information
     * Should be passed back to NimBLEDevice::injectConfirmPasskey
     * @param [in] pin The pin to compare with the client.
     */
    virtual void onConfirmPassKey(NimBLEConnInfo& connInfo, uint32_t pin);

    /**
     * @brief Called when the pairing procedure is complete.
     * @param [in] connInfo A reference to a NimBLEConnInfo instance with information
     * about the peer connection parameters.
     */
    virtual void onAuthenticationComplete(NimBLEConnInfo& connInfo);

    /**
     * @brief Called when the pairing procedure is complete.
     * @param [in] connInfo A reference to a NimBLEConnInfo instance with information
     * @param [in] name The name of the connected peer device.
     * about the peer connection parameters.
     */
    virtual void onAuthenticationComplete(NimBLEConnInfo& connInfo, const std::string& name);

    /**
     * @brief Called when the peer identity address is resolved.
     * @param [in] connInfo A reference to a NimBLEConnInfo instance with information
     */
    virtual void onIdentity(NimBLEConnInfo& connInfo);

    /**
     * @brief Called when connection parameters are updated following a request to
     * update via NimBLEServer::updateConnParams
     * @param [in] connInfo A reference to a NimBLEConnInfo instance containing the
     * updated connection parameters.
     */
    virtual void onConnParamsUpdate(NimBLEConnInfo& connInfo);

# if CONFIG_BT_NIMBLE_EXT_ADV
    /**
     * @brief Called when the PHY update procedure is complete.
     * @param [in] connInfo A reference to a NimBLEConnInfo instance with information
     * about the peer connection parameters.
     * @param [in] txPhy The transmit PHY.
     * @param [in] rxPhy The receive PHY.
     * Possible values:
     * * BLE_GAP_LE_PHY_1M
     * * BLE_GAP_LE_PHY_2M
     * * BLE_GAP_LE_PHY_CODED
     */
    virtual void onPhyUpdate(NimBLEConnInfo& connInfo, uint8_t txPhy, uint8_t rxPhy);
# endif
}; // NimBLEServerCallbacks

#endif // CONFIG_BT_ENABLED && CONFIG_BT_NIMBLE_ROLE_PERIPHERAL
#endif // NIMBLE_CPP_SERVER_H_
