/*
 * NimBLEMeshNode.h
 *
 *  Created: on July 22 2020
 *      Author H2zero
 *
 */

#ifndef MAIN_NIMBLE_MESH_NODE_H_
#define MAIN_NIMBLE_MESH_NODE_H_

#include "sdkconfig.h"
#if defined(CONFIG_BT_ENABLED)
#include "nimconfig.h"

#include "mesh/glue.h"
#include "mesh/mesh.h"

/****  FIX COMPILATION ****/
#undef min
#undef max
/**************************/

#include "NimBLEUUID.h"
#include "NimBLEMeshElement.h"

#include <vector>

typedef enum {
    RELAY        =  0x01 << 0,
    BEACON       =  0x01 << 1,
    FRIEND       =  0x01 << 2,
    PROXY        =  0x01 << 3,
} NIMBLE_MESH;

class NimBLEMeshElement;

class NimBLEMeshNode {
public:
    bool                   start();
    NimBLEMeshElement*     createElement();
    NimBLEMeshElement*     getElement(uint8_t index = 0);

private:
    friend class NimBLEDevice;
    friend class NimBLEMeshElement;

    NimBLEMeshNode(const NimBLEUUID &uuid, uint8_t type);
    ~NimBLEMeshNode();
    static void            provComplete(uint16_t netIdx, uint16_t addr);
    static void            provReset();
    void                   setProvData(uint16_t netIdx, uint16_t addr);

    bt_mesh_cfg_srv        m_serverConfig;
    bt_mesh_prov           m_prov;
    bt_mesh_comp           m_comp;
    uint16_t               m_primAddr;
    uint16_t               m_primNetIdx;
    bt_mesh_model*         m_configSrvModel;
    bt_mesh_model*         m_configHthModel;
    bt_mesh_health_srv     m_healthSrv;
    bt_mesh_model_pub      m_healthPub;
    NimBLEUUID             m_uuid;

    std::vector<NimBLEMeshElement*> m_elemVec;
};


class NimBLEHealthSrvCallbacks {
public:
    static int faultGetCurrent(bt_mesh_model *model, uint8_t *test_id,
			                   uint16_t *company_id, uint8_t *faults,
			                   uint8_t *fault_count);

    static int faultGetRegistered(bt_mesh_model *model, uint16_t company_id,
			                      uint8_t *test_id, uint8_t *faults,
			                      uint8_t *fault_count);

    static int faultClear(bt_mesh_model *model, uint16_t company_id);

    static int faultTest(bt_mesh_model *model, uint8_t test_id, uint16_t company_id);

    static void attentionOn(bt_mesh_model *model);

    static void attentionOff(bt_mesh_model *model);
};

#endif // CONFIG_BT_ENABLED
#endif // MAIN_NIMBLE_MESH_NODE_H_