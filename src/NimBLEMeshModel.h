/*
 * NimBLEMeshModel.h
 *
 *  Created: on Aug 25 2020
 *      Author H2zero
 *
 */

#ifndef MAIN_NIMBLE_MESH_MODEL_H_
#define MAIN_NIMBLE_MESH_MODEL_H_

#include "sdkconfig.h"
#if defined(CONFIG_BT_ENABLED)
#include "nimconfig.h"

#include "NimBLEMeshElement.h"

class NimBLEMeshModelCallbacks;

class NimBLEMeshModel {
public:
    NimBLEMeshModel(NimBLEMeshModelCallbacks* pCallbacks);
    ~NimBLEMeshModel();

    bt_mesh_model_op* opList;

    bt_mesh_model_pub* opPub;
    NimBLEMeshModelCallbacks* m_callbacks;
};

class NimBLEGenOnOffSrvModel : NimBLEMeshModel {
    friend class NimBLEMeshElement;
    friend class NimBLEMeshNode;

    NimBLEGenOnOffSrvModel(NimBLEMeshModelCallbacks* pCallbacks);
    ~NimBLEGenOnOffSrvModel();

    static void getOnOff(bt_mesh_model *model,
                         bt_mesh_msg_ctx *ctx,
                         os_mbuf *buf);
    static void setOnOff(bt_mesh_model *model,
                         bt_mesh_msg_ctx *ctx,
                         os_mbuf *buf);
    static void setOnOffUnack(bt_mesh_model *model,
                              bt_mesh_msg_ctx *ctx,
                              os_mbuf *buf);
};

class NimBLEGenLevelSrvModel : NimBLEMeshModel {
    friend class NimBLEMeshElement;
    friend class NimBLEMeshNode;

    NimBLEGenLevelSrvModel(NimBLEMeshModelCallbacks* pCallbacks);
    ~NimBLEGenLevelSrvModel();

    static void getLevel(bt_mesh_model *model,
                         bt_mesh_msg_ctx *ctx,
                         os_mbuf *buf);
    static void setLevel(bt_mesh_model *model,
                         bt_mesh_msg_ctx *ctx,
                         os_mbuf *buf);
    static void setLevelUnack(bt_mesh_model *model,
                              bt_mesh_msg_ctx *ctx,
                              os_mbuf *buf);
    static void setDelta(bt_mesh_model *model,
                         bt_mesh_msg_ctx *ctx,
                         os_mbuf *buf);
    static void setDeltaUnack(bt_mesh_model *model,
                              bt_mesh_msg_ctx *ctx,
                              os_mbuf *buf);
    static void setMove(bt_mesh_model *model,
                        bt_mesh_msg_ctx *ctx,
                        os_mbuf *buf);
    static void setMoveUnack(bt_mesh_model *model,
                             bt_mesh_msg_ctx *ctx,
                             os_mbuf *buf);
};

class NimBLEMeshModelCallbacks {
public:
    virtual ~NimBLEMeshModelCallbacks();
    virtual void    setOnOff(uint8_t);
    virtual uint8_t getOnOff();
    virtual void    setLevel(int16_t);
    virtual int16_t getLevel();
    virtual void    setDelta(int16_t);

};

#endif // CONFIG_BT_ENABLED
#endif // MAIN_NIMBLE_MESH_MODEL_H_