/*
 * NimBLEMeshElement.cpp
 *
 *  Created: on Aug 23 2020
 *      Author H2zero
 *
 */

#include "NimBLEMeshElement.h"
#include "NimBLELog.h"

static const char* LOG_TAG = "NimBLEMeshElement";

NimBLEMeshElement::NimBLEMeshElement() {
    m_pElem = nullptr;
}
NimBLEMeshElement::~NimBLEMeshElement() {
    if(m_pElem != nullptr) {
        delete m_pElem;
    }

    for(auto &it : m_modelsVec) {
        delete (NimBLEMeshModel*)it.user_data;
    }
}

/**
 * @brief Creates a model and adds it the the elements model vector.
 * @param [in] type The type of model to create.
 * @param [in] pCallbacks a pointer to a callback instance for this model.
 */
void NimBLEMeshElement::createModel(uint16_t type, NimBLEMeshModelCallbacks *pCallbacks) {
    for(auto &it : m_modelsVec) {
        if(it.id == type) {
            NIMBLE_LOGE(LOG_TAG, "Error: element already has a type %04x model", type);
            return;
        }
    }

    NIMBLE_LOGD(LOG_TAG, "Creating model type: %04x", type);

    NimBLEMeshModel* pModel = nullptr;

    switch(type)
    {
        case BT_MESH_MODEL_ID_GEN_ONOFF_SRV:
            pModel = new NimBLEGenOnOffSrvModel(pCallbacks);
            break;

        case BT_MESH_MODEL_ID_GEN_LEVEL_SRV:
            pModel = new NimBLEGenLevelSrvModel(pCallbacks);
            break;

        default:
            NIMBLE_LOGE(LOG_TAG, "Error: model type %04x not supported", type);
            return;
    }

    m_modelsVec.push_back(bt_mesh_model{{type},0,0,0, pModel->opPub,{0},{0},pModel->opList, pModel});
}

/**
 * @brief Adds a model created outside of element context to the elements model vector.
 * @param [in] model A pointer to the model instance to add.
 */
void NimBLEMeshElement::addModel(bt_mesh_model* model) {
    m_modelsVec.push_back(*model);
}

/**
 * @brief Creates a bt_mesh_elem for registering with the nimble stack.
 * @returns A pointer to the bt_mesh_elem created.
 * @details Must not be called until all models have been added.
 */
bt_mesh_elem* NimBLEMeshElement::start() {
    m_pElem = new bt_mesh_elem{0, 0, uint8_t(m_modelsVec.size()), 0, &m_modelsVec[0], NULL};
    return m_pElem;
}


