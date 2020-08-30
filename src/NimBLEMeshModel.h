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
    int extractTransTimeDelay(os_mbuf *buf);
    bool checkRetransmit(uint8_t tid, bt_mesh_msg_ctx *ctx);
    void sendMessage(bt_mesh_model *model, bt_mesh_msg_ctx *ctx, os_mbuf *msg);
    void startTdTimer(ble_npl_time_t timerMs);

    uint8_t   m_lastTid;
    uint16_t  m_lastSrcAddr;
    uint16_t  m_lastDstAddr;
    time_t    m_lastMsgTime;
    uint8_t   m_transTime;
    uint8_t   m_delayTime;
    uint8_t   m_onOffValue;
    uint8_t   m_onOffTarget;
    int16_t   m_levelValue;
    int16_t   m_levelTarget;
    int16_t   m_transStep;
    ble_npl_callout m_tdTimer;
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
    static void tdTimerCb(ble_npl_event *event);
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
    static void tdTimerCb(ble_npl_event *event);
};

class NimBLEMeshModelCallbacks {
public:
    virtual ~NimBLEMeshModelCallbacks();
    virtual void    setOnOff(uint8_t);
    virtual uint8_t getOnOff();
    virtual void    setLevel(int16_t);
    virtual int16_t getLevel();
};

#endif // CONFIG_BT_ENABLED
#endif // MAIN_NIMBLE_MESH_MODEL_H_