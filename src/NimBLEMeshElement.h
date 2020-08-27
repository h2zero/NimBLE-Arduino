/*
 * NimBLEMeshElement.h
 *
 *  Created: on Aug 23 2020
 *      Author H2zero
 *
 */

#ifndef MAIN_NIMBLE_MESH_ELEMENT_H_
#define MAIN_NIMBLE_MESH_ELEMENT_H_

#include "sdkconfig.h"
#if defined(CONFIG_BT_ENABLED)
#include "nimconfig.h"

#include "NimBLEMeshNode.h"
#include "NimBLEMeshModel.h"

#include <vector>

class NimBLEMeshModelCallbacks;

class NimBLEMeshElement {
public:
    void createModel(uint16_t type, NimBLEMeshModelCallbacks* pCallbacks=nullptr);

private:
    friend class NimBLEMeshNode;

    NimBLEMeshElement();
    ~NimBLEMeshElement();
    void addModel(bt_mesh_model* model);
    bt_mesh_elem* start();

    bt_mesh_elem *m_pElem;
    std::vector<bt_mesh_model> m_modelsVec;
};


#endif // CONFIG_BT_ENABLED
#endif // MAIN_NIMBLE_MESH_ELEMENT_H_