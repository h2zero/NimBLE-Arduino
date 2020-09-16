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

#include <vector>

class NimBLEMeshModelCallbacks;

class NimBLEMeshModel {
public:
    NimBLEMeshModel(NimBLEMeshModelCallbacks* pCallbacks);
    virtual ~NimBLEMeshModel();
    int extractTransTimeDelay(os_mbuf *buf);
    bool checkRetransmit(uint8_t tid, bt_mesh_msg_ctx *ctx);
    void sendMessage(bt_mesh_model *model, bt_mesh_msg_ctx *ctx, os_mbuf *msg);
    void startTdTimer(ble_npl_time_t timerMs);
    void publish();
    uint32_t getTransTime();
    uint16_t getDelayTime();
    virtual void setPubMsg(){};
    virtual void setValue(uint8_t *val, size_t len){};
    virtual void setTargetValue(uint8_t *val, size_t len){};
    virtual bt_mesh_health_srv* getHealth_t();

    template<typename T>
    void setValue(const T &s) {
        setValue((uint8_t*)&s, sizeof(T));
    }

    template<typename T>
    void setTargetValue(const T &s) {
        setTargetValue((uint8_t*)&s, sizeof(T));
    }

    template<typename T>
    void getValue(T &s) {
        s = (T)m_value[0];
    }

    template<typename T>
    void getTargetValue(T &s) {
        s = (T)m_targetValue[0];
    }

    bt_mesh_model_op*         m_opList;
    bt_mesh_model_pub         m_opPub;
    NimBLEMeshModelCallbacks* m_callbacks;
    uint8_t                   m_lastTid;
    uint16_t                  m_lastSrcAddr;
    uint16_t                  m_lastDstAddr;
    time_t                    m_lastMsgTime;
    uint8_t                   m_transTime;
    uint8_t                   m_delayTime;
    std::vector<uint8_t>      m_value;
    std::vector<uint8_t>      m_targetValue;
    int16_t                   m_transStep;

    ble_npl_callout           m_tdTimer;
    ble_npl_callout           m_pubTimer;
};


class NimBLEGenOnOffSrvModel : NimBLEMeshModel {
    friend class NimBLEMeshElement;
    friend class NimBLEMeshNode;

    NimBLEGenOnOffSrvModel(NimBLEMeshModelCallbacks *pCallbacks);
    ~NimBLEGenOnOffSrvModel(){};

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
    static void pubTimerCb(ble_npl_event *event);

    void setPubMsg() override;
    void setValue(uint8_t *val, size_t len) override;
    void setTargetValue(uint8_t *val, size_t len) override;
};


class NimBLEGenLevelSrvModel : NimBLEMeshModel {
    friend class NimBLEMeshElement;
    friend class NimBLEMeshNode;

    NimBLEGenLevelSrvModel(NimBLEMeshModelCallbacks *pCallbacks);
    ~NimBLEGenLevelSrvModel(){};

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
    static void pubTimerCb(ble_npl_event *event);

    void setPubMsg() override;
    void setValue(uint8_t *val, size_t len) override;
    void setTargetValue(uint8_t *val, size_t len) override;
};


class NimBLEHealthSrvModel : NimBLEMeshModel {
    friend class NimBLEMeshElement;
    friend class NimBLEMeshNode;

    NimBLEHealthSrvModel(NimBLEMeshModelCallbacks *pCallbacks);
    ~NimBLEHealthSrvModel(){};

    bt_mesh_health_srv*   getHealth_t() override;

    bt_mesh_health_srv    m_healthSrv;
};


class NimBLEMeshModelCallbacks {
public:
    virtual ~NimBLEMeshModelCallbacks();
    virtual void    setOnOff(NimBLEMeshModel *pModel, uint8_t val);
    virtual uint8_t getOnOff(NimBLEMeshModel *pModel);
    virtual void    setLevel(NimBLEMeshModel *pModel, int16_t val);
    virtual int16_t getLevel(NimBLEMeshModel *pModel);
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
#endif // MAIN_NIMBLE_MESH_MODEL_H_