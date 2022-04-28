/*
 * NimBLEMeshCreateModel.cpp
 *
 *  Created: on April 27 2022
 *      Author H2zero
 *
 */

#include "NimBLEMeshCreateModel.h"

static struct bt_mesh_model_cb mod_cb = {
    //.init = modelInitCallback
};

struct bt_mesh_model createConfigSrvModel(struct bt_mesh_cfg_srv* cfg) {
    struct bt_mesh_model cmod = BT_MESH_MODEL_CFG_SRV(cfg);
    return cmod;
}

struct bt_mesh_model createHealthModel(struct bt_mesh_health_srv* hsrv,
                                       struct bt_mesh_model_pub* hpub) {
    struct bt_mesh_model hmod = BT_MESH_MODEL_HEALTH_SRV(hsrv, hpub);
    return hmod;
}

struct bt_mesh_model createGenModel(int16_t _id, struct bt_mesh_model_op* op,
                                    struct bt_mesh_model_pub* pub, void* udata) {
    struct bt_mesh_model mod = BT_MESH_MODEL_CB(_id, op, pub, udata, &mod_cb);
    return mod;
}
