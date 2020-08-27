/*
 * NimBLEMeshNoce.cpp
 *
 *  Created: on July 22 2020
 *      Author H2zero
 *
 */

#include "sdkconfig.h"
#if defined(CONFIG_BT_ENABLED)

#include "NimBLEMeshNode.h"
#include "NimBLELog.h"
#include "NimBLEDevice.h"

#include "services/gap/ble_svc_gap.h"
#include "services/gatt/ble_svc_gatt.h"

#define CID_VENDOR 0x05C3

static const char* LOG_TAG = "NimBLEMeshNode";

/**
 * Health server callback struct
 */
static const struct bt_mesh_health_srv_cb health_srv_cb = {
    NimBLEHealthSrvCallbacks::faultGetCurrent,
    NimBLEHealthSrvCallbacks::faultGetRegistered,
    NimBLEHealthSrvCallbacks::faultClear,
    NimBLEHealthSrvCallbacks::faultTest,
    NimBLEHealthSrvCallbacks::attentionOn,
    NimBLEHealthSrvCallbacks::attentionOff
};

/**
 * @brief Construct a mesh node.
 * @param [in] uuid The uuid used to advertise for provisioning.
 * @param [in] type Bitmask of the node features supported.
 */
NimBLEMeshNode::NimBLEMeshNode(const NimBLEUUID &uuid, uint8_t type) {
    assert(uuid.bitSize() == 128);

    memset(&m_serverConfig, 0, sizeof(m_serverConfig));
    memset(&m_prov, 0, sizeof(m_prov));
    memset(&m_comp, 0, sizeof(m_comp));
    memset(&m_healthPub, 0, sizeof(m_healthPub));

    // Default server config
    m_serverConfig.relay = BT_MESH_RELAY_DISABLED;/*(type & NIMBLE_MESH::RELAY) ?
                           BT_MESH_RELAY_ENABLED :
                           BT_MESH_RELAY_DISABLED;*/

    m_serverConfig.beacon = BT_MESH_BEACON_ENABLED;
    m_serverConfig.frnd = BT_MESH_FRIEND_DISABLED;/*(type & NIMBLE_MESH::FRIEND) ?
                          BT_MESH_FRIEND_ENABLED :
                          BT_MESH_FRIEND_DISABLED;*/

    m_serverConfig.gatt_proxy = BT_MESH_GATT_PROXY_ENABLED; /*(type & NIMBLE_MESH::RELAY) ?
                                BT_MESH_GATT_PROXY_ENABLED :
                                BT_MESH_GATT_PROXY_DISABLED;*/

    m_serverConfig.default_ttl = 7;

    // 3 transmissions with 20ms interval
    m_serverConfig.net_transmit = BT_MESH_TRANSMIT(2, 20);
    m_serverConfig.relay_retransmit = BT_MESH_TRANSMIT(2, 20);

    // Default health server config
    m_healthSrv     = {0};
    m_healthSrv.cb  = &health_srv_cb;

    // Default health pub config
    m_healthPub.msg = BT_MESH_HEALTH_FAULT_MSG(0);

    // Provisioning config
    m_uuid          = uuid;
    m_prov.uuid     = m_uuid.getNative()->u128.value;
    m_prov.complete = NimBLEMeshNode::provComplete;
    m_prov.reset    = NimBLEMeshNode::provReset;

    m_configSrvModel = nullptr;
    m_configHthModel = nullptr;

    // Create the primary element
    m_elemVec.push_back(new NimBLEMeshElement());
}


/**
 * @brief Destructor, cleanup any resources created.
 */
NimBLEMeshNode::~NimBLEMeshNode() {
    if(m_configSrvModel != nullptr) {
        delete m_configSrvModel;
    }

    if(m_configHthModel != nullptr) {
        delete m_configHthModel;
    }

    if(m_comp.elem != nullptr) {
        free (m_comp.elem);
    }
}


/**
 * @brief Called from the callbacks when provisioning changes.
 */
void NimBLEMeshNode::setProvData(uint16_t netIdx, uint16_t addr) {
    m_primAddr = addr;
    m_primNetIdx = netIdx;
}


/**
 * @brief callback, Called by NimBLE stack when provisioning is complete.
 */
void NimBLEMeshNode::provComplete(uint16_t netIdx, uint16_t addr) {
    NIMBLE_LOGI(LOG_TAG,
                "provisioning complete for netIdx 0x%04x addr 0x%04x",
                netIdx, addr);
    NimBLEDevice::getMeshNode()->setProvData(netIdx, addr);
}


/**
 * @brief callback, Called by NimBLE stack when provisioning is reset.
 */
void NimBLEMeshNode::provReset() {
        NIMBLE_LOGI(LOG_TAG, "provisioning reset");
        NimBLEDevice::getMeshNode()->setProvData(0, 0);
}


/**
 * @brief get a pointer an element.
 * @param [in] index The element vector index of the element.
 * @returns a pointer to the element requested.
 */
NimBLEMeshElement* NimBLEMeshNode::getElement(uint8_t index) {
    return m_elemVec[index];
}

/**
 * @brief Create a new mesh element.
 * @returns a pointer to the newly created element.
 */
NimBLEMeshElement* NimBLEMeshNode::createElement() {
    m_elemVec.push_back(new NimBLEMeshElement());
    return m_elemVec.back();
}


/**
 * @brief Start the Mesh mode.
 * @returns true on success.
 */
bool NimBLEMeshNode::start() {
    // Reset and restart gatts so we can register mesh gatt
    ble_gatts_reset();
    ble_svc_gap_init();
    ble_svc_gatt_init();
    bt_mesh_register_gatt();
    ble_gatts_start();

    // Config server and primary health models are required in the primary element
    // create them here and add them as the first models.
    m_configSrvModel = new bt_mesh_model{{BT_MESH_MODEL_ID_CFG_SRV},0,0,0,nullptr,{0},{0},bt_mesh_cfg_srv_op,&m_serverConfig};
    for(int i = 0; i < CONFIG_BT_MESH_MODEL_KEY_COUNT; i++) {
        m_configSrvModel->keys[i] = BT_MESH_KEY_UNUSED;
    }

    for(int i = 0; i < CONFIG_BT_MESH_MODEL_GROUP_COUNT; i++) {
        m_configSrvModel->groups[i] = BT_MESH_ADDR_UNASSIGNED;
    }

    m_configHthModel = new bt_mesh_model{{BT_MESH_MODEL_ID_HEALTH_SRV},0,0,0,&m_healthPub,{0},{0},bt_mesh_health_srv_op,&m_healthSrv};

    m_elemVec[0]->addModel(m_configSrvModel);
    m_elemVec[0]->addModel(m_configHthModel);

    // setup node composition
    m_comp.cid = CID_VENDOR;
    m_comp.elem = (bt_mesh_elem*)calloc(m_elemVec.size(), sizeof(bt_mesh_elem));

    if(m_comp.elem == nullptr) {
        NIMBLE_LOGE(LOG_TAG, "Error: No Mem");
        return false;
    }

    for(size_t i = 0; i < m_elemVec.size(); i++) {
        memcpy((void*)&m_comp.elem[i], (void*)m_elemVec[i]->start(),sizeof(bt_mesh_elem));
    }
    m_comp.elem_count = (uint8_t)m_elemVec.size();

    // Use random address
    ble_addr_t addr;
    int err = ble_hs_id_gen_rnd(1, &addr);
    assert(err == 0);
    err = ble_hs_id_set_rnd(addr.val);
    assert(err == 0);

    err = bt_mesh_init(addr.type, &m_prov, &m_comp);
    if (err) {
        NIMBLE_LOGE(LOG_TAG, "Initializing mesh failed (err %d)", err);
        return false;
    }
    if (IS_ENABLED(CONFIG_SETTINGS)) {
        settings_load();
    }

    if (bt_mesh_is_provisioned()) {
        NIMBLE_LOGI(LOG_TAG, "Mesh network restored from flash");
    }

    return true;
}


/**
 * @brief Health server callbacks
 */
int NimBLEHealthSrvCallbacks::faultGetCurrent(bt_mesh_model *model, uint8_t *test_id,
			                                  uint16_t *company_id, uint8_t *faults,
			                                  uint8_t *fault_count)
{
    NIMBLE_LOGD(LOG_TAG, "faultGetCurrent - default");
    return 0;
}

int NimBLEHealthSrvCallbacks::faultGetRegistered(bt_mesh_model *model, uint16_t company_id,
			                                     uint8_t *test_id, uint8_t *faults,
			                                     uint8_t *fault_count)
{
    NIMBLE_LOGD(LOG_TAG, "faultGetRegistered - default");
    return 0;
}

int NimBLEHealthSrvCallbacks::faultClear(bt_mesh_model *model, uint16_t company_id)
{
    NIMBLE_LOGD(LOG_TAG, "faultClear - default");
    return 0;
}

int NimBLEHealthSrvCallbacks::faultTest(bt_mesh_model *model, uint8_t test_id, uint16_t company_id)
{
    NIMBLE_LOGD(LOG_TAG, "faultTest - default");
    return 0;
}

void NimBLEHealthSrvCallbacks::attentionOn(bt_mesh_model *model)
{
    NIMBLE_LOGD(LOG_TAG, "attentionOn - default");
}

void NimBLEHealthSrvCallbacks::attentionOff(bt_mesh_model *model)
{
    NIMBLE_LOGD(LOG_TAG, "attentionOff - default");
}


#endif // CONFIG_BT_ENABLED