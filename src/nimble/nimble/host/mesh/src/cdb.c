/*
 * Copyright (c) 2019 Tobias Svehagen
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "nimble/porting/nimble/include/syscfg/syscfg.h"
#if MYNEWT_VAL(BLE_MESH)

#define MESH_LOG_MODULE BLE_MESH_LOG
#include "nimble/porting/nimble/include/log/log.h"
#include <stdlib.h>

#include "cdb_priv.h"
#include "net.h"
#include "rpl.h"
#include "settings.h"
#include "mesh_priv.h"
#include "../include/mesh/glue.h"

#if MYNEWT_VAL(BLE_MESH_CDB)
/* Tracking of what storage changes are pending for App and Net Keys. We
 * track this in a separate array here instead of within the respective
 * bt_mesh_app_key and bt_mesh_subnet structs themselves, since once a key
 * gets deleted its struct becomes invalid and may be reused for other keys.
 */
struct key_update {
	uint16_t key_idx:12,    /* AppKey or NetKey Index */
		 valid:1,       /* 1 if this entry is valid, 0 if not */
		 app_key:1,     /* 1 if this is an AppKey, 0 if a NetKey */
		 clear:1;       /* 1 if key needs clearing, 0 if storing */
};

/* Tracking of what storage changes are pending for node settings. */
struct node_update {
	uint16_t addr;
	bool clear;
};

/* Node information for persistent storage. */
struct node_val {
	uint16_t net_idx;
	uint8_t  num_elem;
	uint8_t  flags;
#define F_NODE_CONFIGURED 0x01
	uint8_t  uuid[16];
	uint8_t  dev_key[16];
} __packed;

/* NetKey storage information */
struct net_key_val {
	uint8_t  kr_flag:1,
	kr_phase:7;
	uint8_t  val[2][16];
} __packed;

/* AppKey information for persistent storage. */
struct app_key_val {
	uint16_t net_idx;
	bool     updated;
	uint8_t  val[2][16];
} __packed;

/* IV Index & IV Update information for persistent storage. */
struct net_val {
	uint32_t iv_index;
	bool  iv_update;
} __packed;

static struct node_update cdb_node_updates[MYNEWT_VAL(BLE_MESH_CDB_NODE_COUNT)];
static struct key_update cdb_key_updates[MYNEWT_VAL(BLE_MESH_CDB_SUBNET_COUNT) +
					 MYNEWT_VAL(BLE_MESH_CDB_APP_KEY_COUNT)];

struct bt_mesh_cdb bt_mesh_cdb = {
	.nodes = {
		[0 ... (CONFIG_BT_MESH_NODE_COUNT - 1)] = {
			.addr = BT_MESH_ADDR_UNASSIGNED,
		}
	},
	.subnets = {
		[0 ... (CONFIG_BT_MESH_SUBNET_COUNT - 1)] = {
			.net_idx = BT_MESH_KEY_UNUSED,
		}
	},
	.app_keys = {
		[0 ... (CONFIG_BT_MESH_APP_KEY_COUNT - 1)] = {
			.net_idx = BT_MESH_KEY_UNUSED,
		}
	},
};

/*
 * Check if an address range from addr_start for addr_start + num_elem - 1 is
 * free for use. When a conflict is found, next will be set to the next address
 * available after the conflicting range and -EAGAIN will be returned.
 */
static int addr_is_free(uint16_t addr_start, uint8_t num_elem, uint16_t *next)
{
	uint16_t addr_end = addr_start + num_elem - 1;
	uint16_t other_start, other_end;
	int i;

	if (!BT_MESH_ADDR_IS_UNICAST(addr_start) ||
	    !BT_MESH_ADDR_IS_UNICAST(addr_end) ||
	    num_elem == 0) {
		return -EINVAL;
	}

	for (i = 0; i < ARRAY_SIZE(bt_mesh_cdb.nodes); i++) {
		struct bt_mesh_cdb_node *node = &bt_mesh_cdb.nodes[i];

		if (node->addr == BT_MESH_ADDR_UNASSIGNED) {
			continue;
		}

		other_start = node->addr;
		other_end = other_start + node->num_elem - 1;

		if (!(addr_end < other_start || addr_start > other_end)) {
			if (next) {
				*next = other_end + 1;
			}

			return -EAGAIN;
		}
	}

	return 0;
}

/*
 * Find the lowest possible starting address that can fit num_elem elements. If
 * a free address range cannot be found, BT_MESH_ADDR_UNASSIGNED will be
 * returned. Otherwise the first address in the range is returned.
 *
 * NOTE: This is quite an ineffective algorithm as it might need to look
 *       through the array of nodes N+2 times. A more effective algorithm
 *       could be used if the nodes were stored in a sorted list.
 */
static uint16_t find_lowest_free_addr(uint8_t num_elem)
{
	uint16_t addr = 1, next = 0;
	int err, i;

	/*
	 * It takes a maximum of node count + 2 to find a free address if there
	 * is any. +1 for our own address and +1 for making sure that the
	 * address range is valid.
	 */
	for (i = 0; i < ARRAY_SIZE(bt_mesh_cdb.nodes) + 2; ++i) {
		err = addr_is_free(addr, num_elem, &next);
		if (err == 0) {
			break;
		} else if (err != -EAGAIN) {
			addr = BT_MESH_ADDR_UNASSIGNED;
			break;
		}

		addr = next;
	}

	return addr;
}

#if MYNEWT_VAL(BLE_MESH_SETTINGS)
static int cdb_net_set(int argc, char *val)
{
	struct net_val net;
	int len, err;

	len = sizeof(net);
	err = settings_bytes_from_str(val, &net, &len);
	if (err) {
		BT_ERR("Failed to set \'cdb_net\'");
		return err;
	}

	bt_mesh_cdb.iv_index = net.iv_index;

	if (net.iv_update) {
		atomic_set_bit(bt_mesh_cdb.flags, BT_MESH_CDB_IVU_IN_PROGRESS);
	}

	atomic_set_bit(bt_mesh_cdb.flags, BT_MESH_CDB_VALID);

	return 0;
}

static int cdb_node_set(int argc, char *str)
{
	struct bt_mesh_cdb_node *node;
	struct node_val val;
	uint16_t addr;
	int len, err;

	if (argc < 1) {
		BT_ERR("Insufficient number of arguments");
		return -ENOENT;
	}

	addr = strtol(str, NULL, 16);
	len = sizeof(str);

	if (argc < 1) {
		BT_DBG("val (null)");
		BT_DBG("Deleting node 0x%04x", addr);

		node = bt_mesh_cdb_node_get(addr);
		if (node) {
			bt_mesh_cdb_node_del(node, false);
		}

		return 0;
	}

	err = settings_bytes_from_str(str, &val, &len);
	if (err) {
		BT_ERR("Failed to decode value %s (err %d)", val, err);
		return -EINVAL;
	}

	if (len != sizeof(struct node_val)) {
		BT_ERR("Invalid length for node_val");
		return -EINVAL;
	}

	node = bt_mesh_cdb_node_get(addr);
	if (!node) {
		node = bt_mesh_cdb_node_alloc(val.uuid, addr, val.num_elem,
					      			  val.net_idx);
	}

	if (!node) {
		BT_ERR("No space for a new node");
		return -ENOMEM;
	}

	if (val.flags & F_NODE_CONFIGURED) {
		atomic_set_bit(node->flags, BT_MESH_CDB_NODE_CONFIGURED);
	}

	memcpy(node->uuid, val.uuid, 16);
	memcpy(node->dev_key, val.dev_key, 16);

	BT_DBG("Node 0x%04x recovered from storage", addr);

	return 0;
}

static int cdb_subnet_set(int argc, char *name)
{
	struct bt_mesh_cdb_subnet *sub;
	struct net_key_val key;
	uint16_t net_idx;
	int len, len_rd, err;

	if (!name) {
		BT_ERR("Insufficient number of arguments");
		return -ENOENT;
	}

	len_rd = sizeof(sub);
	net_idx = strtol(name, NULL, 16);
	sub = bt_mesh_cdb_subnet_get(net_idx);

	if (len_rd == 0) {
		BT_DBG("val (null)");
		if (!sub) {
			BT_ERR("No subnet with NetKeyIndex 0x%03x", net_idx);
			return -ENOENT;
		}

		BT_DBG("Deleting NetKeyIndex 0x%03x", net_idx);
		bt_mesh_cdb_subnet_del(sub, false);
		return 0;
	}

	len = sizeof(key);
	err = settings_bytes_from_str(name, &key, &len);
	if (err) {
		BT_ERR("Failed to set \'net-key\'");
		return err;
	}

	if (sub) {
		BT_DBG("Updating existing NetKeyIndex 0x%03x", net_idx);

		sub->kr_phase = key.kr_phase;
		memcpy(sub->keys[0].net_key, &key.val[0], 16);
		memcpy(sub->keys[1].net_key, &key.val[1], 16);

		return 0;
	}

	sub = bt_mesh_cdb_subnet_alloc(net_idx);
	if (!sub) {
		BT_ERR("No space to allocate a new subnet");
		return -ENOMEM;
	}

	sub->kr_phase = key.kr_phase;
	memcpy(sub->keys[0].net_key, &key.val[0], 16);
	memcpy(sub->keys[1].net_key, &key.val[1], 16);

	BT_DBG("NetKeyIndex 0x%03x recovered from storage", net_idx);

	return 0;
}

static int cdb_app_key_set(int argc, char *name)
{
	struct bt_mesh_cdb_app_key *app;
	struct app_key_val key;
	uint16_t app_idx;
	int len_rd, err;

	app_idx = strtol(name, NULL, 16);
	len_rd = sizeof(key);

	if (len_rd == 0) {
		BT_DBG("val (null)");
		BT_DBG("Deleting AppKeyIndex 0x%03x", app_idx);

		app = bt_mesh_cdb_app_key_get(app_idx);
		if (app) {
			bt_mesh_cdb_app_key_del(app, false);
		}

		return 0;
	}

	err = settings_bytes_from_str(name, &key, &len_rd);
	if (err) {
		BT_ERR("Failed to set \'app-key\'");
		return err;
	}

	app = bt_mesh_cdb_app_key_get(app_idx);
	if (!app) {
		app = bt_mesh_cdb_app_key_alloc(key.net_idx, app_idx);
	}

	if (!app) {
		BT_ERR("No space for a new app key");
		return -ENOMEM;
	}

	memcpy(app->keys[0].app_key, key.val[0], 16);
	memcpy(app->keys[1].app_key, key.val[1], 16);

	BT_DBG("AppKeyIndex 0x%03x recovered from storage", app_idx);

	return 0;
}

static int cdb_set(int argc, char **argv, char *name)
{
	int len;
	char *next;

	if (argc < 1) {
		BT_ERR("Insufficient number of arguments");
		return -ENOENT;
	}

	if (!strcmp(name, "Net")) {
		return cdb_net_set(1, name);
	}

	len = settings_name_next(name, &next);

	if (!next) {
		BT_ERR("Insufficient number of arguments");
		return -ENOENT;
	}

	if (!strncmp(name, "Node", len)) {
		return cdb_node_set(1, next);
	}

	if (!strncmp(name, "Subnet", len)) {
		return cdb_subnet_set(1, next);
	}

	if (!strncmp(name, "AppKey", len)) {
		return cdb_app_key_set(1, next);
	}

	BT_WARN("Unknown module key %s", name);
	return -ENOENT;
}
#endif

static void store_cdb_node(const struct bt_mesh_cdb_node *node)
{
#if MYNEWT_VAL(BLE_MESH_SETTINGS)
	char buf[BT_SETTINGS_SIZE(sizeof(struct node_val))];
	struct node_val val;
	char path[30];
	char *str;
	int err;

	val.net_idx = node->net_idx;
	val.num_elem = node->num_elem;
	val.flags = 0;

	if (atomic_test_bit(node->flags, BT_MESH_CDB_NODE_CONFIGURED)) {
		val.flags |= F_NODE_CONFIGURED;
	}

	memcpy(val.uuid, node->uuid, 16);
	memcpy(val.dev_key, node->dev_key, 16);

	snprintk(path, sizeof(path), "bt_mesh/cdb/Node/%x", node->addr);

	str = settings_str_from_bytes(&val, sizeof(val), buf, sizeof(buf));
	if (!str) {
		BT_ERR("Unable to encode Node as value");
		return;
	}


	err = settings_save_one(path, str);
	if (err) {
		BT_ERR("Failed to store Node %s value", path);
	} else {
		BT_DBG("Stored Node %s value", path);
	}
#endif
}

static void clear_cdb_node(uint16_t addr)
{
#if MYNEWT_VAL(BLE_MESH_SETTINGS)
	char path[30];
	int err;

	BT_DBG("Node 0x%04x", addr);

	snprintk(path, sizeof(path), "bt_mesh/cdb/Node/%x", addr);
	err = settings_save_one(path, NULL);
	if (err) {
		BT_ERR("Failed to clear Node 0x%04x", addr);
	} else {
		BT_DBG("Cleared Node 0x%04x", addr);
	}
#endif
}

static void store_cdb_subnet(const struct bt_mesh_cdb_subnet *sub)
{
#if MYNEWT_VAL(BLE_MESH_SETTINGS)
	char buf[BT_SETTINGS_SIZE(sizeof(struct net_key_val))];
	struct net_key_val key;
	char path[30];
	int err;
	char *str;

	BT_DBG("NetKeyIndex 0x%03x NetKey %s", sub->net_idx,
	       bt_hex(sub->keys[0].net_key, 16));

	memcpy(&key.val[0], sub->keys[0].net_key, 16);
	memcpy(&key.val[1], sub->keys[1].net_key, 16);
	key.kr_flag = 0U; /* Deprecated */
	key.kr_phase = sub->kr_phase;

	snprintk(path, sizeof(path), "bt_mesh/cdb/Subnet/%x", sub->net_idx);


	str = settings_str_from_bytes(&key, sizeof(key), buf, sizeof(buf));
	if (!str) {
		BT_ERR("Unable to encode Subnet as value");
		return;
	}
	err = settings_save_one(path, str);
	if (err) {
		BT_ERR("Failed to store Subnet value");
	} else {
		BT_DBG("Stored Subnet value");
	}
#endif
}

static void clear_cdb_subnet(uint16_t net_idx)
{
#if MYNEWT_VAL(BLE_MESH_SETTINGS)
	char path[30];
	int err;

	BT_DBG("NetKeyIndex 0x%03x", net_idx);

	snprintk(path, sizeof(path), "bt_mesh/cdb/Subnet/%x", net_idx);
	err = settings_save_one(path, NULL);
	if (err) {
		BT_ERR("Failed to clear NetKeyIndex 0x%03x", net_idx);
	} else {
		BT_DBG("Cleared NetKeyIndex 0x%03x", net_idx);
	}
#endif
}

static void store_cdb_app_key(const struct bt_mesh_cdb_app_key *app)
{
#if MYNEWT_VAL(BLE_MESH_SETTINGS)
	char buf[BT_SETTINGS_SIZE(sizeof(struct app_key_val))];
	struct app_key_val key;
	char path[30];
	int err;
	char *str;

	key.net_idx = app->net_idx;
	key.updated = false;
	memcpy(key.val[0], app->keys[0].app_key, 16);
	memcpy(key.val[1], app->keys[1].app_key, 16);

	snprintk(path, sizeof(path), "bt_mesh/cdb/AppKey/%x", app->app_idx);

	str = settings_str_from_bytes(&key, sizeof(key), buf, sizeof(buf));
	err = settings_save_one(path, str);
	if (err) {
		BT_ERR("Failed to store AppKey");
	} else {
		BT_DBG("Stored AppKey");
	}
#endif
}

static void clear_cdb_app_key(uint16_t app_idx)
{
#if MYNEWT_VAL(BLE_MESH_SETTINGS)
	char path[30];
	int err;

	snprintk(path, sizeof(path), "bt_mesh/cdb/AppKey/%x", app_idx);
	err = settings_save_one(path, NULL);
	if (err) {
		BT_ERR("Failed to clear AppKeyIndex 0x%03x", app_idx);
	} else {
		BT_DBG("Cleared AppKeyIndex 0x%03x", app_idx);
	}
#endif
}

static void schedule_cdb_store(int flag)
{
	atomic_set_bit(bt_mesh_cdb.flags, flag);
	bt_mesh_settings_store_schedule(BT_MESH_SETTINGS_CDB_PENDING);
}

static void update_cdb_net_settings(void)
{
	schedule_cdb_store(BT_MESH_CDB_SUBNET_PENDING);
}

static struct node_update *cdb_node_update_find(uint16_t addr,
						struct node_update **free_slot)
{
	struct node_update *match;
	int i;

	match = NULL;
	*free_slot = NULL;

	for (i = 0; i < ARRAY_SIZE(cdb_node_updates); i++) {
		struct node_update *update = &cdb_node_updates[i];

		if (update->addr == BT_MESH_ADDR_UNASSIGNED) {
			*free_slot = update;
			continue;
		}

		if (update->addr == addr) {
			match = update;
		}
	}

	return match;
}

static void update_cdb_node_settings(const struct bt_mesh_cdb_node *node,
				     bool store)
{
	struct node_update *update, *free_slot;

	BT_DBG("Node 0x%04x", node->addr);

	update = cdb_node_update_find(node->addr, &free_slot);
	if (update) {
		update->clear = !store;
		schedule_cdb_store(BT_MESH_CDB_NODES_PENDING);
		return;
	}

	if (!free_slot) {
		if (store) {
			store_cdb_node(node);
		} else {
			clear_cdb_node(node->addr);
		}
		return;
	}

	free_slot->addr = node->addr;
	free_slot->clear = !store;

	schedule_cdb_store(BT_MESH_CDB_NODES_PENDING);
}

static struct key_update *cdb_key_update_find(bool app_key, uint16_t key_idx,
					      struct key_update **free_slot)
{
	struct key_update *match;
	int i;

	match = NULL;
	*free_slot = NULL;

	for (i = 0; i < ARRAY_SIZE(cdb_key_updates); i++) {
		struct key_update *update = &cdb_key_updates[i];

		if (!update->valid) {
			*free_slot = update;
			continue;
		}

		if (update->app_key != app_key) {
			continue;
		}

		if (update->key_idx == key_idx) {
			match = update;
		}
	}

	return match;
}

static void update_cdb_subnet_settings(const struct bt_mesh_cdb_subnet *sub,
				       bool store)
{
	struct key_update *update, *free_slot;
	uint8_t clear = store ? 0U : 1U;

	BT_DBG("NetKeyIndex 0x%03x", sub->net_idx);

	update = cdb_key_update_find(false, sub->net_idx, &free_slot);
	if (update) {
		update->clear = clear;
		schedule_cdb_store(BT_MESH_CDB_KEYS_PENDING);
		return;
	}

	if (!free_slot) {
		if (store) {
			store_cdb_subnet(sub);
		} else {
			clear_cdb_subnet(sub->net_idx);
		}
		return;
	}

	free_slot->valid = 1U;
	free_slot->key_idx = sub->net_idx;
	free_slot->app_key = 0U;
	free_slot->clear = clear;

	schedule_cdb_store(BT_MESH_CDB_KEYS_PENDING);
}

static void update_cdb_app_key_settings(const struct bt_mesh_cdb_app_key *key,
					bool store)
{
	struct key_update *update, *free_slot;
	uint8_t clear = store ? 0U : 1U;

	BT_DBG("AppKeyIndex 0x%03x", key->app_idx);

	update = cdb_key_update_find(true, key->app_idx, &free_slot);
	if (update) {
		update->clear = clear;
		schedule_cdb_store(BT_MESH_CDB_KEYS_PENDING);
		return;
	}

	if (!free_slot) {
		if (store) {
			store_cdb_app_key(key);
		} else {
			clear_cdb_app_key(key->app_idx);
		}

		return;
	}

	free_slot->valid = 1U;
	free_slot->key_idx = key->app_idx;
	free_slot->app_key = 1U;
	free_slot->clear = clear;

	schedule_cdb_store(BT_MESH_CDB_KEYS_PENDING);
}

int bt_mesh_cdb_create(const uint8_t key[16])
{
	struct bt_mesh_cdb_subnet *sub;

	if (atomic_test_and_set_bit(bt_mesh_cdb.flags,
				    BT_MESH_CDB_VALID)) {
		return -EALREADY;
	}

	sub = bt_mesh_cdb_subnet_alloc(BT_MESH_KEY_PRIMARY);
	if (sub == NULL) {
		return -ENOMEM;
	}

	memcpy(sub->keys[0].net_key, key, 16);
	bt_mesh_cdb.iv_index = 0;

	if (IS_ENABLED(CONFIG_BT_SETTINGS)) {
		update_cdb_net_settings();
		update_cdb_subnet_settings(sub, true);
	}

	return 0;
}

void bt_mesh_cdb_clear(void)
{
	int i;

	atomic_clear_bit(bt_mesh_cdb.flags, BT_MESH_CDB_VALID);

	for (i = 0; i < ARRAY_SIZE(bt_mesh_cdb.nodes); ++i) {
		if (bt_mesh_cdb.nodes[i].addr != BT_MESH_ADDR_UNASSIGNED) {
			bt_mesh_cdb_node_del(&bt_mesh_cdb.nodes[i], true);
		}
	}

	for (i = 0; i < ARRAY_SIZE(bt_mesh_cdb.subnets); ++i) {
		if (bt_mesh_cdb.subnets[i].net_idx != BT_MESH_KEY_UNUSED) {
			bt_mesh_cdb_subnet_del(&bt_mesh_cdb.subnets[i], true);
		}
	}

	for (i = 0; i < ARRAY_SIZE(bt_mesh_cdb.app_keys); ++i) {
		if (bt_mesh_cdb.app_keys[i].net_idx != BT_MESH_KEY_UNUSED) {
			bt_mesh_cdb_app_key_del(&bt_mesh_cdb.app_keys[i], true);
		}
	}

	if (IS_ENABLED(CONFIG_BT_SETTINGS)) {
		update_cdb_net_settings();
	}
}

void bt_mesh_cdb_iv_update(uint32_t iv_index, bool iv_update)
{
	BT_DBG("Updating IV index to %d\n", iv_index);

	bt_mesh_cdb.iv_index = iv_index;

	atomic_set_bit_to(bt_mesh_cdb.flags, BT_MESH_CDB_IVU_IN_PROGRESS,
			  iv_update);

	if (IS_ENABLED(CONFIG_BT_SETTINGS)) {
		update_cdb_net_settings();
	}
}

struct bt_mesh_cdb_subnet *bt_mesh_cdb_subnet_alloc(uint16_t net_idx)
{
	struct bt_mesh_cdb_subnet *sub;
	int i;

	if (bt_mesh_cdb_subnet_get(net_idx) != NULL) {
		return NULL;
	}

	for (i = 0; i < ARRAY_SIZE(bt_mesh_cdb.subnets); ++i) {
		sub = &bt_mesh_cdb.subnets[i];

		if (sub->net_idx != BT_MESH_KEY_UNUSED) {
			continue;
		}

		sub->net_idx = net_idx;

		return sub;
	}

	return NULL;
}

void bt_mesh_cdb_subnet_del(struct bt_mesh_cdb_subnet *sub, bool store)
{
	BT_DBG("NetIdx 0x%03x store %u", sub->net_idx, store);

	if (IS_ENABLED(CONFIG_BT_SETTINGS) && store) {
		update_cdb_subnet_settings(sub, false);
	}

	sub->net_idx = BT_MESH_KEY_UNUSED;
	memset(sub->keys, 0, sizeof(sub->keys));
}

struct bt_mesh_cdb_subnet *bt_mesh_cdb_subnet_get(uint16_t net_idx)
{
	int i;

	for (i = 0; i < ARRAY_SIZE(bt_mesh_cdb.subnets); ++i) {
		if (bt_mesh_cdb.subnets[i].net_idx == net_idx) {
			return &bt_mesh_cdb.subnets[i];
		}
	}

	return NULL;
}

void bt_mesh_cdb_subnet_store(const struct bt_mesh_cdb_subnet *sub)
{
	if (IS_ENABLED(CONFIG_BT_SETTINGS)) {
		update_cdb_subnet_settings(sub, true);
	}
}

uint8_t bt_mesh_cdb_subnet_flags(const struct bt_mesh_cdb_subnet *sub)
{
	uint8_t flags = 0x00;

	if (sub && SUBNET_KEY_TX_IDX(sub)) {
		flags |= BT_MESH_NET_FLAG_KR;
	}

	if (atomic_test_bit(bt_mesh_cdb.flags, BT_MESH_CDB_IVU_IN_PROGRESS)) {
		flags |= BT_MESH_NET_FLAG_IVU;
	}

	return flags;
}

struct bt_mesh_cdb_node *bt_mesh_cdb_node_alloc(const uint8_t uuid[16], uint16_t addr,
						uint8_t num_elem, uint16_t net_idx)
{
	int i;

	if (addr == BT_MESH_ADDR_UNASSIGNED) {
        addr = find_lowest_free_addr(num_elem);
		if (addr == BT_MESH_ADDR_UNASSIGNED) {
			return NULL;
		}
	} else if (addr_is_free(addr, num_elem, NULL) < 0) {
		BT_DBG("Address range 0x%04x-0x%04x is not free", addr,
		       addr + num_elem - 1);
		return NULL;
	}

	for (i = 0; i < ARRAY_SIZE(bt_mesh_cdb.nodes); i++) {
		struct bt_mesh_cdb_node *node = &bt_mesh_cdb.nodes[i];

		if (node->addr == BT_MESH_ADDR_UNASSIGNED) {
			memcpy(node->uuid, uuid, 16);
			node->addr = addr;
			node->num_elem = num_elem;
			node->net_idx = net_idx;
			atomic_set(node->flags, 0);
			return node;
		}
	}

	return NULL;
}

void bt_mesh_cdb_node_del(struct bt_mesh_cdb_node *node, bool store)
{
	BT_DBG("Node addr 0x%04x store %u", node->addr, store);

	if (IS_ENABLED(CONFIG_BT_SETTINGS) && store) {
		update_cdb_node_settings(node, false);
	}

	node->addr = BT_MESH_ADDR_UNASSIGNED;
	memset(node->dev_key, 0, sizeof(node->dev_key));
}

struct bt_mesh_cdb_node *bt_mesh_cdb_node_get(uint16_t addr)
{
	int i;

	for (i = 0; i < ARRAY_SIZE(bt_mesh_cdb.nodes); i++) {
		struct bt_mesh_cdb_node *node = &bt_mesh_cdb.nodes[i];

		if (addr >= node->addr &&
		    addr <= node->addr + node->num_elem - 1) {
			return node;
		}
	}

	return NULL;
}

void bt_mesh_cdb_node_store(const struct bt_mesh_cdb_node *node)
{
	if (IS_ENABLED(CONFIG_BT_SETTINGS)) {
		update_cdb_node_settings(node, true);
	}
}

void bt_mesh_cdb_node_foreach(bt_mesh_cdb_node_func_t func, void *user_data)
{
	int i;

	for (i = 0; i < ARRAY_SIZE(bt_mesh_cdb.nodes); ++i) {
		if (bt_mesh_cdb.nodes[i].addr == BT_MESH_ADDR_UNASSIGNED) {
			continue;
		}

		if (func(&bt_mesh_cdb.nodes[i], user_data) ==
		    BT_MESH_CDB_ITER_STOP) {
			break;
		}
	}
}

struct bt_mesh_cdb_app_key *bt_mesh_cdb_app_key_alloc(uint16_t net_idx,
						      uint16_t app_idx)
{
	struct bt_mesh_cdb_app_key *key;
	int i;

	for (i = 0; i < ARRAY_SIZE(bt_mesh_cdb.app_keys); ++i) {
		key = &bt_mesh_cdb.app_keys[i];

		if (key->net_idx != BT_MESH_KEY_UNUSED) {
			continue;
		}

		key->net_idx = net_idx;
		key->app_idx = app_idx;

		return key;
	}

	return NULL;
}

void bt_mesh_cdb_app_key_del(struct bt_mesh_cdb_app_key *key, bool store)
{
	BT_DBG("AppIdx 0x%03x store %u", key->app_idx, store);

	if (IS_ENABLED(CONFIG_BT_SETTINGS) && store) {
		update_cdb_app_key_settings(key, false);
	}

	key->net_idx = BT_MESH_KEY_UNUSED;
	memset(key->keys, 0, sizeof(key->keys));
}

struct bt_mesh_cdb_app_key *bt_mesh_cdb_app_key_get(uint16_t app_idx)
{
	int i;

	for (i = 0; i < ARRAY_SIZE(bt_mesh_cdb.app_keys); i++) {
		struct bt_mesh_cdb_app_key *key = &bt_mesh_cdb.app_keys[i];

		if (key->net_idx != BT_MESH_KEY_UNUSED &&
		    key->app_idx == app_idx) {
			return key;
		}
	}

	return NULL;
}

void bt_mesh_cdb_app_key_store(const struct bt_mesh_cdb_app_key *key)
{
	if (IS_ENABLED(CONFIG_BT_SETTINGS)) {
		update_cdb_app_key_settings(key, true);
	}
}

static void clear_cdb_net(void)
{
#if MYNEWT_VAL(BLE_MESH_SETTINGS)
	char path[30];
	int err;

	snprintk(path, sizeof(path), "bt_mesh/cdb/Net");
	err = settings_save_one(path, NULL);
	if (err) {
		BT_ERR("Failed to clear Net");
	} else {
		BT_DBG("Cleared NetKeyIndex 0x%03x");
	}
#endif
}

static void store_cdb_pending_net(void)
{
#if MYNEWT_VAL(BLE_MESH_SETTINGS)
	struct net_val net;
	int err;
	char buf[BT_SETTINGS_SIZE(sizeof(struct net_val))];
	char *str;

	BT_DBG("");

	net.iv_index = bt_mesh_cdb.iv_index;
	net.iv_update = atomic_test_bit(bt_mesh_cdb.flags,
					BT_MESH_CDB_IVU_IN_PROGRESS);

	str = settings_str_from_bytes(&net, sizeof(net), buf, sizeof(buf));
	if (!str) {
		BT_ERR("Unable to encode Network as value");
		return;
	}
	err = settings_save_one("bt_mesh/cdb/Net", str);
	if (err) {
		BT_ERR("Failed to store Network value");
	} else {
		BT_DBG("Stored Network value");
	}
#endif
}

static void store_cdb_pending_nodes(void)
{
	int i;

	for (i = 0; i < ARRAY_SIZE(cdb_node_updates); ++i) {
		struct node_update *update = &cdb_node_updates[i];

		if (update->addr == BT_MESH_ADDR_UNASSIGNED) {
			continue;
		}

		BT_DBG("addr: 0x%04x, clear: %d", update->addr, update->clear);

		if (update->clear) {
			clear_cdb_node(update->addr);
		} else {
			struct bt_mesh_cdb_node *node;

			node = bt_mesh_cdb_node_get(update->addr);
			if (node) {
				store_cdb_node(node);
			} else {
				BT_WARN("Node 0x%04x not found", update->addr);
			}
		}

		update->addr = BT_MESH_ADDR_UNASSIGNED;
	}
}

static void store_cdb_pending_keys(void)
{
	int i;

	for (i = 0; i < ARRAY_SIZE(cdb_key_updates); i++) {
		struct key_update *update = &cdb_key_updates[i];

		if (!update->valid) {
			continue;
		}

		if (update->clear) {
			if (update->app_key) {
				clear_cdb_app_key(update->key_idx);
			} else {
				clear_cdb_subnet(update->key_idx);
			}
		} else {
			if (update->app_key) {
				struct bt_mesh_cdb_app_key *key;

				key = bt_mesh_cdb_app_key_get(update->key_idx);
				if (key) {
					store_cdb_app_key(key);
				} else {
					BT_WARN("AppKeyIndex 0x%03x not found",
						update->key_idx);
				}
			} else {
				struct bt_mesh_cdb_subnet *sub;

				sub = bt_mesh_cdb_subnet_get(update->key_idx);
				if (sub) {
					store_cdb_subnet(sub);
				} else {
					BT_WARN("NetKeyIndex 0x%03x not found",
						update->key_idx);
				}
			}
		}

		update->valid = 0U;
	}
}

void bt_mesh_cdb_pending_store(void)
{
	if (atomic_test_and_clear_bit(bt_mesh_cdb.flags,
				      BT_MESH_CDB_SUBNET_PENDING)) {
		if (atomic_test_bit(bt_mesh_cdb.flags,
				    BT_MESH_CDB_VALID)) {
			store_cdb_pending_net();
		} else {
			clear_cdb_net();
		}
	}

	if (atomic_test_and_clear_bit(bt_mesh_cdb.flags,
				      BT_MESH_CDB_NODES_PENDING)) {
		store_cdb_pending_nodes();
	}

	if (atomic_test_and_clear_bit(bt_mesh_cdb.flags,
				      BT_MESH_CDB_KEYS_PENDING)) {
		store_cdb_pending_keys();
	}
}

#if MYNEWT_VAL(BLE_MESH_SETTINGS)
static struct conf_handler bt_mesh_cdb_conf_handler = {
	.ch_name = "bt_mesh",
	.ch_get = NULL,
	.ch_set = cdb_set,
	.ch_commit = NULL,
	.ch_export = NULL,
};
#endif

void bt_mesh_cdb_init(void)
{
#if MYNEWT_VAL(BLE_MESH_SETTINGS)
	int rc;

	rc = conf_register(&bt_mesh_cdb_conf_handler);

	SYSINIT_PANIC_ASSERT_MSG(rc == 0,
				 "Failed to register bt_mesh_net conf");
#endif
}
#endif

#endif // MYNEWT_VAL(BLE_MESH)