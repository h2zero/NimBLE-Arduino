/*
 * NimBLEMeshModel.cpp
 *
 *  Created: on Aug 25 2020
 *      Author H2zero
 *
 */

#include "NimBLEMeshModel.h"
#include "NimBLELog.h"

static const char* LOG_TAG = "NimBLEMeshModel";

static NimBLEMeshModelCallbacks defaultCallbacks;

/**
 * @brief base model constructor
 * @param [in] pCallbacks, a pointer to a callback instance for model operations
 */
NimBLEMeshModel::NimBLEMeshModel(NimBLEMeshModelCallbacks* pCallbacks) {
    if(pCallbacks == nullptr) {
        m_callbacks = &defaultCallbacks;
    } else {
        m_callbacks = pCallbacks;
    }

    opList = nullptr;
    opPub  = nullptr;
}


/**
 * @brief destructor
 */
NimBLEMeshModel::~NimBLEMeshModel(){
    if(opList != nullptr) {
        delete[] opList;
    }

    if(opPub != nullptr) {
        delete[] opPub;
    }
}


/**
 * @brief Generic on/off server model constructor
 * @param [in] pCallbacks, a pointer to a callback instance for model operations
 */
NimBLEGenOnOffSrvModel::NimBLEGenOnOffSrvModel(NimBLEMeshModelCallbacks* pCallbacks)
:NimBLEMeshModel(pCallbacks)
{
    // Register the opcodes for this model with the required callbacks
    opList = new bt_mesh_model_op[4]{
    { BT_MESH_MODEL_OP_2(0x82, 0x01), 0, NimBLEGenOnOffSrvModel::getOnOff },
    { BT_MESH_MODEL_OP_2(0x82, 0x02), 2, NimBLEGenOnOffSrvModel::setOnOff },
    { BT_MESH_MODEL_OP_2(0x82, 0x03), 2, NimBLEGenOnOffSrvModel::setOnOffUnack },
    BT_MESH_MODEL_OP_END};
}


/**
 * @brief Called by the NimBLE stack to get the on/off status of the model
 */
void NimBLEGenOnOffSrvModel::getOnOff(bt_mesh_model *model,
                                      bt_mesh_msg_ctx *ctx,
                                      os_mbuf *buf)
{
    NimBLEGenOnOffSrvModel *pModel = (NimBLEGenOnOffSrvModel*)model->user_data;
    struct os_mbuf *msg = NET_BUF_SIMPLE(3);
    uint8_t *status;

    bt_mesh_model_msg_init(msg, BT_MESH_MODEL_OP_2(0x82, 0x04));
    status = (uint8_t*)net_buf_simple_add(msg, 1);
    *status = pModel->m_callbacks->getOnOff();

    if (bt_mesh_model_send(model, ctx, msg, NULL, NULL)) {
        NIMBLE_LOGE(LOG_TAG, "Send status failed");
    }

    os_mbuf_free_chain(msg);
}


/**
 * @brief Called by the NimBLE stack to set the status of the model with acknowledgement.
 */
void NimBLEGenOnOffSrvModel::setOnOff(bt_mesh_model *model,
                                      bt_mesh_msg_ctx *ctx,
                                      os_mbuf *buf)
{
    NimBLEGenOnOffSrvModel *pModel = (NimBLEGenOnOffSrvModel*)model->user_data;
    pModel->m_callbacks->setOnOff(buf->om_data[0]);

    // send the status update
    NimBLEGenOnOffSrvModel::getOnOff(model,ctx,buf);
}


/**
 * @brief Called by the NimBLE stack to set the status of the model without acknowledgement.
 */
void NimBLEGenOnOffSrvModel::setOnOffUnack(bt_mesh_model *model,
                                           bt_mesh_msg_ctx *ctx,
                                           os_mbuf *buf)
{
    NimBLEGenOnOffSrvModel *pModel = (NimBLEGenOnOffSrvModel*)model->user_data;
    pModel->m_callbacks->setOnOff(buf->om_data[0]);
}


/**
 * @brief Generic level server model constructor
 * @param [in] pCallbacks, a pointer to a callback instance for model operations
 */
NimBLEGenLevelSrvModel::NimBLEGenLevelSrvModel(NimBLEMeshModelCallbacks* pCallbacks)
:NimBLEMeshModel(pCallbacks)
{
    // Register the opcodes for this model with the required callbacks
    opList = new bt_mesh_model_op[8]{
    { BT_MESH_MODEL_OP_2(0x82, 0x05), 0, NimBLEGenLevelSrvModel::getLevel },
    { BT_MESH_MODEL_OP_2(0x82, 0x06), 3, NimBLEGenLevelSrvModel::setLevel },
    { BT_MESH_MODEL_OP_2(0x82, 0x07), 3, NimBLEGenLevelSrvModel::setLevelUnack },
    { BT_MESH_MODEL_OP_2(0x82, 0x09), 5, NimBLEGenLevelSrvModel::setDelta },
    { BT_MESH_MODEL_OP_2(0x82, 0x0a), 5, NimBLEGenLevelSrvModel::setDeltaUnack },
    { BT_MESH_MODEL_OP_2(0x82, 0x0b), 3, NimBLEGenLevelSrvModel::setMove },
    { BT_MESH_MODEL_OP_2(0x82, 0x0c), 3, NimBLEGenLevelSrvModel::setMoveUnack },
    BT_MESH_MODEL_OP_END};
}


/**
 * @brief Called by the NimBLE stack to get the level value of the model.
 */
void NimBLEGenLevelSrvModel::getLevel(bt_mesh_model *model,
                                      bt_mesh_msg_ctx *ctx,
                                      os_mbuf *buf)
{
    NimBLEMeshModel *pModel = (NimBLEMeshModel*)model->user_data;

    struct os_mbuf *msg = NET_BUF_SIMPLE(4);

    bt_mesh_model_msg_init(msg, BT_MESH_MODEL_OP_2(0x82, 0x08));
    net_buf_simple_add_le16(msg, pModel->m_callbacks->getLevel());

    if (bt_mesh_model_send(model, ctx, msg, NULL, NULL)) {
        NIMBLE_LOGE(LOG_TAG, "Send status failed");
    }

    os_mbuf_free_chain(msg);
}


/**
 * @brief Called by the NimBLE stack to set the level value of the model.
 */
void NimBLEGenLevelSrvModel::setLevel(bt_mesh_model *model,
                                      bt_mesh_msg_ctx *ctx,
                                      os_mbuf *buf)
{
    NimBLEMeshModel *pModel = (NimBLEMeshModel*)model->user_data;
    pModel->m_callbacks->setLevel((int16_t) net_buf_simple_pull_le16(buf));

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
    pModel->m_callbacks->setLevel((int16_t) net_buf_simple_pull_le16(buf));
}


/**
 * @brief Called by the NimBLE stack to set the level value by delta of the model.
 */
void NimBLEGenLevelSrvModel::setDelta(bt_mesh_model *model,
                                      bt_mesh_msg_ctx *ctx,
                                      os_mbuf *buf)
{
    NimBLEMeshModel *pModel = (NimBLEMeshModel*)model->user_data;
    pModel->m_callbacks->setDelta((int16_t) net_buf_simple_pull_le16(buf));

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
    pModel->m_callbacks->setDelta((int16_t) net_buf_simple_pull_le16(buf));
}


void NimBLEGenLevelSrvModel::setMove(bt_mesh_model *model,
                                     bt_mesh_msg_ctx *ctx,
                                     os_mbuf *buf)
{
}

void NimBLEGenLevelSrvModel::setMoveUnack(bt_mesh_model *model,
                                          bt_mesh_msg_ctx *ctx,
                                          os_mbuf *buf)
{
}


/**
 * Default model callbacks
 */
NimBLEMeshModelCallbacks::~NimBLEMeshModelCallbacks() {}

void NimBLEMeshModelCallbacks::setOnOff(uint8_t val) {
    NIMBLE_LOGD(LOG_TAG, "Gen On/Off set val: %d", val);
}

uint8_t NimBLEMeshModelCallbacks::getOnOff() {
    NIMBLE_LOGD(LOG_TAG, "Gen On/Off get");
    return 0;
}

void NimBLEMeshModelCallbacks::setLevel(int16_t val) {
    NIMBLE_LOGD(LOG_TAG, "Gen Level set val: %d", val);
}

int16_t NimBLEMeshModelCallbacks::getLevel() {
    NIMBLE_LOGD(LOG_TAG, "Gen Level get");
    return 0;
}

void NimBLEMeshModelCallbacks::setDelta(int16_t val) {
    NIMBLE_LOGD(LOG_TAG, "Gen Delta set val: %d", val);
}