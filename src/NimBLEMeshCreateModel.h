/*
 * NimBLEMeshCreateModel.h
 *
 *  Created: on April 27 2022
 *      Author H2zero
 *
 */

#ifndef __NIMBLE_MESH_CREATE_MODEL_H
#define __NIMBLE_MESH_CREATE_MODEL_H

#include "nimconfig.h"
#if defined(CONFIG_NIMBLE_CPP_IDF)
#  include "mesh/mesh.h"
#  include "mesh/cfg_srv.h"
#else
#  include "nimble/nimble/host/mesh/include/mesh/mesh.h"
#  include "nimble/nimble/host/mesh/include/mesh/cfg_srv.h"
#endif


#ifdef __cplusplus
extern "C" {
#endif
//int modelInitCallback(struct bt_mesh_model *model);
struct bt_mesh_model createConfigSrvModel(struct bt_mesh_cfg_srv* cfg);
struct bt_mesh_model createHealthModel(struct bt_mesh_health_srv* hsrv,
                                       struct bt_mesh_model_pub* hpub);
struct bt_mesh_model createGenModel(int16_t _id, struct bt_mesh_model_op* op,
                                    struct bt_mesh_model_pub* pub, void* udata);

#ifdef __cplusplus
}
#endif
#endif
