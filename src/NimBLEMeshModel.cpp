/*
 * NimBLEMeshModel.cpp
 *
 *  Created: on Aug 25 2020
 *      Author H2zero
 *
 */

#include "NimBLEMeshModel.h"
#include "NimBLEUtils.h"
#include "NimBLELog.h"

#include "nimble/nimble_port.h"

static const char* LOG_TAG = "NimBLEMeshModel";

static NimBLEMeshModelCallbacks defaultCallbacks;

/**
 * @brief base model constructor
 * @param [in] pCallbacks, a pointer to a callback instance for model operations
 */
NimBLEMeshModel::NimBLEMeshModel(NimBLEMeshModelCallbacks *pCallbacks) {
    if(pCallbacks == nullptr) {
        m_callbacks = &defaultCallbacks;
    } else {
        m_callbacks = pCallbacks;
    }

    m_opList        = nullptr;
    m_lastTid     = 0;
    m_lastSrcAddr = 0;
    m_lastDstAddr = 0;
    m_lastMsgTime = 0;
    m_transTime   = 0;
    m_delayTime   = 0;
    m_transStep   = 0;

    memset(&m_opPub, 0, sizeof(m_opPub));
}


/**
 * @brief destructor
 */
NimBLEMeshModel::~NimBLEMeshModel() {
    if(m_opList != nullptr) {
        delete[] m_opList;
    }
}


int NimBLEMeshModel::extractTransTimeDelay(os_mbuf *buf)
{
    switch(buf->om_len) {
        case 0x00:
             m_transTime = 0;
             m_delayTime = 0;
             return 0;
        case 0x02:
            m_transTime = buf->om_data[0];
            if((m_transTime & 0x3F) == 0x3F) {
                // unknown transition time
                m_transTime = 0;
                m_delayTime = 0;
                return BLE_HS_EINVAL;
            }
            m_delayTime = buf->om_data[1];
            return 0;
        default:
            return BLE_HS_EMSGSIZE;
    }
}


bool NimBLEMeshModel::checkRetransmit(uint8_t tid, bt_mesh_msg_ctx *ctx) {
    time_t now = time(nullptr);

    if(m_lastTid == tid &&
       m_lastSrcAddr == ctx->addr &&
       m_lastDstAddr == ctx->recv_dst &&
        (now - m_lastMsgTime <= 6)) {
        NIMBLE_LOGD(LOG_TAG, "Ignoring retransmit");
        return true;
    }

    m_lastTid = tid;
    m_lastSrcAddr = ctx->addr;
    m_lastDstAddr = ctx->recv_dst;
    m_lastMsgTime = now;

    return false;
}


void NimBLEMeshModel::sendMessage(bt_mesh_model *model, bt_mesh_msg_ctx *ctx, os_mbuf *msg) {
    if (bt_mesh_model_send(model, ctx, msg, NULL, NULL)) {
        NIMBLE_LOGE(LOG_TAG, "Send status failed");
    }

    os_mbuf_free_chain(msg);
}


void NimBLEMeshModel::startTdTimer(ble_npl_time_t timerMs) {
    ble_npl_time_t ticks;
    ble_npl_time_ms_to_ticks(timerMs, &ticks);
    ble_npl_callout_reset(&m_tdTimer, ticks);
}


void NimBLEMeshModel::publish() {
    ble_npl_callout_reset(&m_pubTimer, 1);
}

uint32_t NimBLEMeshModel::getTransTime() {
    return (m_transTime & 0x3F) * NimBLEUtils::meshTransTimeMs(m_transTime);
}

uint16_t NimBLEMeshModel::getDelayTime() {
    return m_delayTime * 5;
}


/**
 * @brief Generic on/off server model constructor
 * @param [in] pCallbacks, a pointer to a callback instance for model operations
 */
NimBLEGenOnOffSrvModel::NimBLEGenOnOffSrvModel(NimBLEMeshModelCallbacks *pCallbacks)
:NimBLEMeshModel(pCallbacks)
{
    // Register the opcodes for this model with the required callbacks
    m_opList = new bt_mesh_model_op[4]{
    { BT_MESH_MODEL_OP_2(0x82, 0x01), 0, NimBLEGenOnOffSrvModel::getOnOff },
    { BT_MESH_MODEL_OP_2(0x82, 0x02), 2, NimBLEGenOnOffSrvModel::setOnOff },
    { BT_MESH_MODEL_OP_2(0x82, 0x03), 2, NimBLEGenOnOffSrvModel::setOnOffUnack },
    BT_MESH_MODEL_OP_END};

    ble_npl_callout_init(&m_tdTimer, nimble_port_get_dflt_eventq(),
                         NimBLEGenOnOffSrvModel::tdTimerCb, this);
    ble_npl_callout_init(&m_pubTimer, nimble_port_get_dflt_eventq(),
                         NimBLEGenOnOffSrvModel::pubTimerCb, this);

    m_opPub.msg = NET_BUF_SIMPLE(2 + 3);
    
    m_value.push_back(0);
    m_targetValue.push_back(0);
}


/**
 * @brief Called by the NimBLE stack to get the on/off status of the model
 */
void NimBLEGenOnOffSrvModel::getOnOff(bt_mesh_model *model,
                                      bt_mesh_msg_ctx *ctx,
                                      os_mbuf *buf)
{
    NimBLEMeshModel *pModel = (NimBLEMeshModel*)model->user_data;
    if(pModel->m_callbacks != &defaultCallbacks) {
        pModel->setValue(pModel->m_callbacks->getOnOff(pModel));
    }
    pModel->setPubMsg();

    if (bt_mesh_model_send(model, ctx, pModel->m_opPub.msg, NULL, NULL)) {
        NIMBLE_LOGE(LOG_TAG, "Send status failed");
    }
}


/**
 * @brief Called by the NimBLE stack to set the status of the model with acknowledgement.
 */
void NimBLEGenOnOffSrvModel::setOnOff(bt_mesh_model *model,
                                      bt_mesh_msg_ctx *ctx,
                                      os_mbuf *buf)
{
    // Rather than duplicate code just call the unack function then send the status
    NimBLEGenOnOffSrvModel::setOnOffUnack(model,ctx,buf);
    NimBLEGenOnOffSrvModel::getOnOff(model,ctx,buf);
}


/**
 * @brief Called by the NimBLE stack to set the status of the model without acknowledgement.
 */
void NimBLEGenOnOffSrvModel::setOnOffUnack(bt_mesh_model *model,
                                           bt_mesh_msg_ctx *ctx,
                                           os_mbuf *buf)
{
    NimBLEMeshModel *pModel = (NimBLEMeshModel*)model->user_data;
    uint8_t newval = net_buf_simple_pull_u8(buf);
    uint8_t tid = net_buf_simple_pull_u8(buf);

    if(pModel->checkRetransmit(tid, ctx)) {
        return;
    }

    if(pModel->extractTransTimeDelay(buf) != 0) {
        NIMBLE_LOGI(LOG_TAG, "Transition time / delay data error");
        return;
    }

    // stop the transition timer to handle the new input
    ble_npl_callout_stop(&pModel->m_tdTimer);

    // Mesh spec says transition to "ON state" happens immediately
    // after delay, so ignore the transition time.
    if(newval == 1) {
        pModel->m_transTime = 0;
    }

    ble_npl_time_t timerMs = 0;

    if(newval != pModel->m_value[0]) {
       pModel->m_targetValue[0] = newval;

        if(pModel->m_delayTime > 0) {
            timerMs = 5 * pModel->m_delayTime;
        } else if(pModel->m_transTime & 0x3F) {
            timerMs = NimBLEUtils::meshTransTimeMs(pModel->m_transTime);
            pModel->m_transTime -= 1;
        }
    }

    if(timerMs > 0) {
        pModel->startTdTimer(timerMs);
    } else {
        pModel->m_value = pModel->m_targetValue;
        pModel->m_callbacks->setOnOff(pModel, pModel->m_value[0]);
    }
}

void NimBLEGenOnOffSrvModel::tdTimerCb(ble_npl_event *event) {
    NimBLEMeshModel *pModel = (NimBLEMeshModel*)event->arg;
    if(pModel->m_delayTime > 0) {
        pModel->m_delayTime = 0;
    }

    if((pModel->m_transTime & 0x3F) && pModel->m_targetValue[0]/*m_onOffTarget*/ == 0) {
        pModel->startTdTimer(NimBLEUtils::meshTransTimeMs(pModel->m_transTime));
        pModel->m_transTime -= 1;
        pModel->publish();
        return;
    }

    pModel->m_transTime = 0;
    pModel->m_value = pModel->m_targetValue;
    pModel->m_callbacks->setOnOff(pModel, pModel->m_value[0]);
}


void NimBLEGenOnOffSrvModel::pubTimerCb(ble_npl_event *event) {
    NimBLEMeshModel *pModel = (NimBLEMeshModel*)event->arg;
    pModel->setPubMsg();

    int err = bt_mesh_model_publish(pModel->m_opPub.mod);
    if(err != 0) {
        NIMBLE_LOGD(LOG_TAG, "Publish rc: %d",err);
    }
}


void NimBLEGenOnOffSrvModel::setPubMsg() {
    bt_mesh_model_msg_init(m_opPub.msg, BT_MESH_MODEL_OP_2(0x82, 0x04));
    net_buf_simple_add_u8(m_opPub.msg, m_value[0]);
    if(m_transTime > 0) {
        net_buf_simple_add_u8(m_opPub.msg, m_targetValue[0]);
        // If we started the transition timer in setOnOff we need to correct the reported remaining time.
        net_buf_simple_add_u8(m_opPub.msg, (m_delayTime > 0) ?
                                   m_transTime : m_transTime + 1);
    }
}

void NimBLEGenOnOffSrvModel::setValue(uint8_t *val, size_t len) {
    if(len != sizeof(uint8_t)) {
        NIMBLE_LOGE(LOG_TAG, "NimBLEGenOnOffSrvModel: Incorrect value length");
        return;
    }
    m_value[0] = *val;
}

void NimBLEGenOnOffSrvModel::setTargetValue(uint8_t *val, size_t len) {
    if(len != sizeof(uint8_t)) {
        NIMBLE_LOGE(LOG_TAG, "NimBLEGenOnOffSrvModel: Incorrect target value length");
        return;
    }
    m_targetValue[0] = *val;
}

/**
 * @brief Generic level server model constructor
 * @param [in] pCallbacks, a pointer to a callback instance for model operations
 */
NimBLEGenLevelSrvModel::NimBLEGenLevelSrvModel(NimBLEMeshModelCallbacks *pCallbacks)
:NimBLEMeshModel(pCallbacks)
{
    // Register the opcodes for this model with the required callbacks
    m_opList = new bt_mesh_model_op[8]{
    { BT_MESH_MODEL_OP_2(0x82, 0x05), 0, NimBLEGenLevelSrvModel::getLevel },
    { BT_MESH_MODEL_OP_2(0x82, 0x06), 3, NimBLEGenLevelSrvModel::setLevel },
    { BT_MESH_MODEL_OP_2(0x82, 0x07), 3, NimBLEGenLevelSrvModel::setLevelUnack },
    { BT_MESH_MODEL_OP_2(0x82, 0x09), 5, NimBLEGenLevelSrvModel::setDelta },
    { BT_MESH_MODEL_OP_2(0x82, 0x0a), 5, NimBLEGenLevelSrvModel::setDeltaUnack },
    { BT_MESH_MODEL_OP_2(0x82, 0x0b), 3, NimBLEGenLevelSrvModel::setMove },
    { BT_MESH_MODEL_OP_2(0x82, 0x0c), 3, NimBLEGenLevelSrvModel::setMoveUnack },
    BT_MESH_MODEL_OP_END};

    ble_npl_callout_init(&m_tdTimer, nimble_port_get_dflt_eventq(),
                         NimBLEGenLevelSrvModel::tdTimerCb, this);
    ble_npl_callout_init(&m_pubTimer, nimble_port_get_dflt_eventq(),
                         NimBLEGenLevelSrvModel::pubTimerCb, this);
    m_opPub.msg = NET_BUF_SIMPLE(2 + 5);
    
    m_value.assign(2, 0);
    m_targetValue.assign(2, 0);
}


/**
 * @brief Called by the NimBLE stack to get the level value of the model.
 */
void NimBLEGenLevelSrvModel::getLevel(bt_mesh_model *model,
                                      bt_mesh_msg_ctx *ctx,
                                      os_mbuf *buf)
{
    NimBLEMeshModel *pModel = (NimBLEMeshModel*)model->user_data;
    if(pModel->m_callbacks != &defaultCallbacks) {
        pModel->setValue(pModel->m_callbacks->getLevel(pModel));
    }
    pModel->setPubMsg();

    if (bt_mesh_model_send(model, ctx, pModel->m_opPub.msg, NULL, NULL)) {
        NIMBLE_LOGE(LOG_TAG, "Send status failed");
    }
}


/**
 * @brief Called by the NimBLE stack to set the level value of the model.
 */
void NimBLEGenLevelSrvModel::setLevel(bt_mesh_model *model,
                                      bt_mesh_msg_ctx *ctx,
                                      os_mbuf *buf)
{
    NimBLEGenLevelSrvModel::setLevelUnack(model, ctx, buf);
    NimBLEGenLevelSrvModel::getLevel(model, ctx, buf);
}


/**
 * @brief Called by the NimBLE stack to set the level value of the model without acknowledgement.
 */
void NimBLEGenLevelSrvModel::setLevelUnack(bt_mesh_model *model,
                                           bt_mesh_msg_ctx *ctx,
                                           os_mbuf *buf)
{
    NimBLEMeshModel *pModel = (NimBLEMeshModel*)model->user_data;
    int16_t newval = (int16_t) net_buf_simple_pull_le16(buf);
    uint8_t tid = net_buf_simple_pull_u8(buf);

    if(pModel->checkRetransmit(tid, ctx)) {
        return;
    }

    if(pModel->extractTransTimeDelay(buf) != 0) {
        NIMBLE_LOGI(LOG_TAG, "Transition time / delay data error");
        return;
    }

    // stop the transition timer to handle the new input
    ble_npl_callout_stop(&pModel->m_tdTimer);

    ble_npl_time_t timerMs = 0;
     
    int16_t curval = *(int16_t*)&pModel->m_value[0];
    
    if(newval != curval) {
        pModel->m_targetValue.assign({(uint8_t)newval, (uint8_t)(newval >> 8)});

        if(pModel->m_delayTime > 0) {
            timerMs = 5 * pModel->m_delayTime;
        }

        if(pModel->m_transTime & 0x3F) {
            pModel->m_transStep = -1 * ((curval - newval) / (pModel->m_transTime & 0x3F));
            if(timerMs == 0) {
                timerMs = NimBLEUtils::meshTransTimeMs(pModel->m_transTime);
                pModel->m_transTime -= 1;
            }
        }
    }

    if(timerMs > 0) {
        pModel->startTdTimer(timerMs);
    } else {
        pModel->m_value = pModel->m_targetValue;
        pModel->m_callbacks->setLevel(pModel, newval);
    }
}


/**
 * @brief Called by the NimBLE stack to set the level value by delta of the model.
 */
void NimBLEGenLevelSrvModel::setDelta(bt_mesh_model *model,
                                      bt_mesh_msg_ctx *ctx,
                                      os_mbuf *buf)
{
    NimBLEGenLevelSrvModel::setDeltaUnack(model, ctx, buf);
    NimBLEGenLevelSrvModel::getLevel(model, ctx, buf);
}


/**
 * @brief Called by the NimBLE stack to set the level value by delta without acknowledgement.
 */
void NimBLEGenLevelSrvModel::setDeltaUnack(bt_mesh_model *model,
                                           bt_mesh_msg_ctx *ctx,
                                           os_mbuf *buf)
{
    NimBLEMeshModel *pModel = (NimBLEMeshModel*)model->user_data;
    int32_t delta = (int32_t) net_buf_simple_pull_le32(buf);

    int32_t temp32 = *(int16_t*)&pModel->m_value[0] + delta;
    if (temp32 < INT16_MIN) {
        temp32 = INT16_MIN;
    } else if (temp32 > INT16_MAX) {
        temp32 = INT16_MAX;
    }

    net_buf_simple_push_le16(buf, (uint16_t)temp32);
    NimBLEGenLevelSrvModel::setLevelUnack(model, ctx, buf);
}


void NimBLEGenLevelSrvModel::setMove(bt_mesh_model *model,
                                     bt_mesh_msg_ctx *ctx,
                                     os_mbuf *buf)
{
    NimBLEGenLevelSrvModel::setMoveUnack(model, ctx, buf);
    NimBLEGenLevelSrvModel::getLevel(model, ctx, buf);
}


void NimBLEGenLevelSrvModel::setMoveUnack(bt_mesh_model *model,
                                          bt_mesh_msg_ctx *ctx,
                                          os_mbuf *buf)
{
    int16_t delta = (int16_t) net_buf_simple_pull_le16(buf);
    // Check if a transition time is present, if not then ignore this message.
    // See: bluetooth mesh specifcation
    if(buf->om_len < 3) {
        return;
    }
    put_le32(net_buf_simple_push(buf, 4), (int32_t)delta);
    NimBLEGenLevelSrvModel::setDeltaUnack(model, ctx, buf);
}


void NimBLEGenLevelSrvModel::tdTimerCb(ble_npl_event *event) {
    NimBLEMeshModel *pModel = (NimBLEMeshModel*)event->arg;
    if(pModel->m_delayTime > 0) {
        pModel->m_delayTime = 0;
    }

    if(pModel->m_transTime & 0x3F) {
        int16_t newval = *(int16_t*)&pModel->m_value[0] + pModel->m_transStep;
        pModel->m_value.assign({(uint8_t)newval, (uint8_t)(newval >> 8)});
        pModel->m_callbacks->setLevel(pModel, newval);
        pModel->startTdTimer(NimBLEUtils::meshTransTimeMs(pModel->m_transTime));
        pModel->m_transTime -= 1;
        return;
    }

    pModel->m_transTime = 0;
    pModel->m_value = pModel->m_targetValue;
    pModel->m_callbacks->setLevel(pModel, *(int16_t*)&pModel->m_value[0]);
}


void NimBLEGenLevelSrvModel::pubTimerCb(ble_npl_event *event) {
    NimBLEMeshModel *pModel = (NimBLEMeshModel*)event->arg;
    pModel->setPubMsg();

    int err = bt_mesh_model_publish(pModel->m_opPub.mod);
    if(err != 0) {
        NIMBLE_LOGD(LOG_TAG, "Publish rc: %d",err);
    }
}

void NimBLEGenLevelSrvModel::setPubMsg() {
    bt_mesh_model_msg_init(m_opPub.msg, BT_MESH_MODEL_OP_2(0x82, 0x08));
    net_buf_simple_add_le16(m_opPub.msg, *(int16_t*)&m_value[0]);
    if(m_transTime > 0) {
        net_buf_simple_add_le16(m_opPub.msg, *(int16_t*)&m_targetValue[0]);
        // If we started the transition timer in setOnOff we need to correct the reported remaining time.
        net_buf_simple_add_u8(m_opPub.msg, (m_delayTime > 0) ?
                                   m_transTime : m_transTime + 1);
    }
}

void NimBLEGenLevelSrvModel::setValue(uint8_t *val, size_t len) {
    if(len != sizeof(int16_t)) {
        NIMBLE_LOGE(LOG_TAG, "NimBLEGenLevelSrvModel: Incorrect value length");
        return;
    }
    m_value.assign({*val, val[1]});
}

void NimBLEGenLevelSrvModel::setTargetValue(uint8_t *val, size_t len) {
    if(len != sizeof(int16_t)) {
        NIMBLE_LOGE(LOG_TAG, "NimBLEGenLevelSrvModel: Incorrect target value length");
        return;
    }
    m_targetValue.assign({*val, val[1]});
}

/**
 * Default model callbacks
 */
NimBLEMeshModelCallbacks::~NimBLEMeshModelCallbacks() {}

void NimBLEMeshModelCallbacks::setOnOff(NimBLEMeshModel *pModel, uint8_t val) {
    NIMBLE_LOGD(LOG_TAG, "Gen On/Off set val: %d", val);
}

uint8_t NimBLEMeshModelCallbacks::getOnOff(NimBLEMeshModel *pModel) {
    NIMBLE_LOGD(LOG_TAG, "Gen On/Off get");
    return 0;
}

void NimBLEMeshModelCallbacks::setLevel(NimBLEMeshModel *pModel, int16_t val) {
    NIMBLE_LOGD(LOG_TAG, "Gen Level set val: %d", val);
}

int16_t NimBLEMeshModelCallbacks::getLevel(NimBLEMeshModel *pModel) {
    NIMBLE_LOGD(LOG_TAG, "Gen Level get");
    return 0;
}
