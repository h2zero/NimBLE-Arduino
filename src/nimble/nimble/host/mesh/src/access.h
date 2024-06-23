/*  Bluetooth Mesh */

/*
 * Copyright (c) 2017 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef __ACCESS_H__
#define __ACCESS_H__

#include "../include/mesh/mesh.h"

/* Tree walk return codes */
enum bt_mesh_walk {
	BT_MESH_WALK_STOP,
	BT_MESH_WALK_CONTINUE,
};

void bt_mesh_elem_register(struct bt_mesh_elem *elem, uint8_t count);

uint8_t bt_mesh_elem_count(void);

/* Find local element based on unicast address */
struct bt_mesh_elem *bt_mesh_elem_find(uint16_t addr);

bool bt_mesh_has_addr(uint16_t addr);
bool bt_mesh_model_has_key(struct bt_mesh_model *mod, uint16_t key);

void bt_mesh_model_extensions_walk(struct bt_mesh_model *root,
				   enum bt_mesh_walk (*cb)(struct bt_mesh_model *mod,
							   void *user_data),
				   void *user_data);

uint16_t *bt_mesh_model_find_group(struct bt_mesh_model **mod, uint16_t addr);

void bt_mesh_model_foreach(void (*func)(struct bt_mesh_model *mod,
					struct bt_mesh_elem *elem,
					bool vnd, bool primary,
					void *user_data),
			   void *user_data);

int32_t bt_mesh_model_pub_period_get(struct bt_mesh_model *mod);

void bt_mesh_comp_provision(uint16_t addr);
void bt_mesh_comp_unprovision(void);

uint16_t bt_mesh_primary_addr(void);

const struct bt_mesh_comp *bt_mesh_comp_get(void);

struct bt_mesh_model *bt_mesh_model_get(bool vnd, uint8_t elem_idx, uint8_t mod_idx);

void bt_mesh_model_recv(struct bt_mesh_net_rx *rx, struct os_mbuf *buf);

int bt_mesh_comp_register(const struct bt_mesh_comp *comp);

void bt_mesh_model_pending_store(void);
void bt_mesh_model_bind_store(struct bt_mesh_model *mod);
void bt_mesh_model_sub_store(struct bt_mesh_model *mod);
void bt_mesh_model_pub_store(struct bt_mesh_model *mod);
void bt_mesh_model_settings_commit(void);

/** @brief Register a callback function hook for mesh model messages.
 *
 * Register a callback function to act as a hook for recieving mesh model layer messages
 * directly to the application without having instantiated the relevant models.
 *
 * @param cb A pointer to the callback function.
 */
void bt_mesh_msg_cb_set(void (*cb)(uint32_t opcode, struct bt_mesh_msg_ctx *ctx,
	struct os_mbuf *buf));

/** @brief Send a mesh model message.
 *
 * Send a mesh model layer message out into the mesh network without having instantiated
 * the relevant mesh models.
 *
 * @param ctx The Bluetooth mesh message context.
 * @param buf The message payload.
 *
 * @return 0 on success or negative error code on failure.
 */
int bt_mesh_msg_send(struct bt_mesh_msg_ctx *ctx, struct os_mbuf *buf, uint16_t src_addr,
	const struct bt_mesh_send_cb *cb, void *cb_data);

void bt_mesh_access_init(void);
#endif
