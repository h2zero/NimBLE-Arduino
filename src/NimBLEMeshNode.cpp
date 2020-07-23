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

static const struct bt_mesh_health_srv_cb health_srv_cb = {
    NimBLEHealthSrvCallbacks::faultGetCurrent,
    NimBLEHealthSrvCallbacks::faultGetRegistered,
    NimBLEHealthSrvCallbacks::faultClear,
    NimBLEHealthSrvCallbacks::faultTest,
    NimBLEHealthSrvCallbacks::attentionOn,
    NimBLEHealthSrvCallbacks::attentionOff
};

extern "C" {
    
static struct bt_mesh_model_pub gen_onoff_pub;
static uint8_t gen_on_off_state;

static void gen_onoff_status(struct bt_mesh_model *model,
                             struct bt_mesh_msg_ctx *ctx)
{
    struct os_mbuf *msg = NET_BUF_SIMPLE(3);
    uint8_t *status;

    NIMBLE_LOGI(LOG_TAG, "#mesh-onoff STATUS\n");

    bt_mesh_model_msg_init(msg, BT_MESH_MODEL_OP_2(0x82, 0x04));
    status = (uint8_t*)net_buf_simple_add(msg, 1);
    *status = gen_on_off_state;

    if (bt_mesh_model_send(model, ctx, msg, NULL, NULL)) {
        NIMBLE_LOGI(LOG_TAG, "#mesh-onoff STATUS: send status failed\n");
    }

    os_mbuf_free_chain(msg);
}

static void gen_onoff_get(struct bt_mesh_model *model,
                          struct bt_mesh_msg_ctx *ctx,
                          struct os_mbuf *buf)
{
    NIMBLE_LOGI(LOG_TAG, "#mesh-onoff GET\n");

    gen_onoff_status(model, ctx);
}

static void gen_onoff_set(struct bt_mesh_model *model,
                          struct bt_mesh_msg_ctx *ctx,
                          struct os_mbuf *buf)
{
    NIMBLE_LOGI(LOG_TAG, "#mesh-onoff SET\n");

    gen_on_off_state = buf->om_data[0];

    gen_onoff_status(model, ctx);
}

static void gen_onoff_set_unack(struct bt_mesh_model *model,
                                struct bt_mesh_msg_ctx *ctx,
                                struct os_mbuf *buf)
{
    NIMBLE_LOGI(LOG_TAG, "#mesh-onoff SET-UNACK\n");

    gen_on_off_state = buf->om_data[0];
}

static const struct bt_mesh_model_op gen_onoff_op[] = {
    { BT_MESH_MODEL_OP_2(0x82, 0x01), 0, gen_onoff_get },
    { BT_MESH_MODEL_OP_2(0x82, 0x02), 2, gen_onoff_set },
    { BT_MESH_MODEL_OP_2(0x82, 0x03), 2, gen_onoff_set_unack },
    BT_MESH_MODEL_OP_END,
};

}


NimBLEMeshNode::NimBLEMeshNode(const NimBLEUUID &uuid, uint8_t type):
m_configSrvModel{{BT_MESH_MODEL_ID_CFG_SRV},0,0,0,nullptr,{0},{0},bt_mesh_cfg_srv_op,nullptr}
{   
    assert(uuid.bitSize() == 128);

    memset(&m_serverConfig, 0, sizeof(m_serverConfig));
    memset(&m_prov, 0, sizeof(m_prov));
    memset(&m_comp, 0, sizeof(m_comp));
    memset(&m_healthPub, 0, sizeof(m_healthPub));
    
    m_serverConfig.relay = BT_MESH_RELAY_DISABLED;/*(type & NIMBLE_MESH::RELAY) ?
                           BT_MESH_RELAY_ENABLED :
                           BT_MESH_RELAY_DISABLED;*/

    m_serverConfig.beacon = BT_MESH_BEACON_ENABLED;
    m_serverConfig.frnd = BT_MESH_FRIEND_DISABLED;/*(type & NIMBLE_MESH::FRIEND) ?
                          BT_MESH_FRIEND_ENABLED :
                          BT_MESH_FRIEND_DISABLED;*/

    m_serverConfig.gatt_proxy = BT_MESH_GATT_PROXY_ENABLED; /*(type & NIMBLE_MESH::RELAY) ?
                                BT_MESH_GATT_PROXY_ENABLED :
                                BT_MESH_GATT_PROXY_NOT_SUPPORTED;*/

    m_serverConfig.default_ttl = 7;
    m_serverConfig.net_transmit = BT_MESH_TRANSMIT(2, 20);
    m_serverConfig.relay_retransmit = BT_MESH_TRANSMIT(2, 20);

    m_configSrvModel.user_data = &m_serverConfig;

    for(int i = 0; i < CONFIG_BT_MESH_MODEL_KEY_COUNT; i++) {
        m_configSrvModel.keys[i] = BT_MESH_KEY_UNUSED;
    }
    
    for(int i = 0; i < CONFIG_BT_MESH_MODEL_GROUP_COUNT; i++) {
        m_configSrvModel.groups[i] = BT_MESH_ADDR_UNASSIGNED;
    }

    m_healthSrv     = {0};
    m_healthSrv.cb  = &health_srv_cb;
    
    m_healthPub.msg = BT_MESH_HEALTH_FAULT_MSG(0);
    m_uuid          = uuid;

    m_prov.uuid     = m_uuid.getNative()->u128.value;
    m_prov.complete = NimBLEMeshNode::provComplete;
    m_prov.reset    = NimBLEMeshNode::provReset;
    
    m_rootModels    = nullptr;
    m_elem          = nullptr;
}


void NimBLEMeshNode::setProvData(uint16_t netIdx, uint16_t addr) {
    m_primAddr = addr;
    m_primNetIdx = netIdx;
}


void NimBLEMeshNode::provComplete(uint16_t netIdx, uint16_t addr) {
    NIMBLE_LOGE(LOG_TAG,
                "provisioning complete for netIdx 0x%04x addr 0x%04x",
                netIdx, addr);
    NimBLEDevice::getMeshNode()->setProvData(netIdx, addr);
}


void NimBLEMeshNode::provReset() {
        NIMBLE_LOGE(LOG_TAG, "provisioning reset");
}


bool NimBLEMeshNode::start() {
    ble_gatts_reset();
    ble_svc_gap_init();
    ble_svc_gatt_init();
    bt_mesh_register_gatt();
    ble_gatts_start();
    
    uint8_t numModels = 2 + m_modelsVec.size();
    
    if(m_rootModels == nullptr) {
        m_configHthModel = new bt_mesh_model{{BT_MESH_MODEL_ID_HEALTH_SRV},0,0,0,&m_healthPub,{0},{0},bt_mesh_health_srv_op,&m_healthSrv};
    
        m_rootModels = (bt_mesh_model*) calloc(numModels, sizeof(bt_mesh_model));
        memcpy(&m_rootModels[0], &m_configSrvModel, sizeof(bt_mesh_model));
        memcpy(&m_rootModels[1], m_configHthModel, sizeof(bt_mesh_model));
        for(size_t i = 0; i < m_modelsVec.size(); i++) {
            memcpy(&m_rootModels[i+2], m_modelsVec[i], sizeof(bt_mesh_model));
        }
    }
    
    if(m_elem == nullptr) {
        m_elem = new bt_mesh_elem{0, 0, numModels, 0, m_rootModels, NULL};
    }
    
    m_comp.cid = CID_VENDOR;
    m_comp.elem = m_elem;
    m_comp.elem_count = 1;
    
    ble_addr_t addr;

    /* Use NRPA */
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


void NimBLEMeshNode::createModel() {
    m_modelsVec.push_back(new bt_mesh_model{{BT_MESH_MODEL_ID_GEN_ONOFF_SRV},0,0,0, &gen_onoff_pub,{0},{0},gen_onoff_op, NULL});
}
    
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