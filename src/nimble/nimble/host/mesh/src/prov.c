/*  Bluetooth Mesh */

/*
 * Copyright (c) 2017 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "nimble/porting/nimble/include/syscfg/syscfg.h"
#if MYNEWT_VAL(BLE_MESH)

#define MESH_LOG_MODULE BLE_MESH_PROV_LOG

#include <errno.h>

#include "../include/mesh/mesh.h"
#include "mesh_priv.h"

#include "crypto.h"
#include "atomic.h"
#include "net.h"
#include "access.h"
#include "foundation.h"
#include "prov.h"
#include "../include/mesh/testing.h"

struct bt_mesh_prov_link bt_mesh_prov_link;
const struct bt_mesh_prov *bt_mesh_prov;

/* Verify specification defined length: */
BUILD_ASSERT(sizeof(bt_mesh_prov_link.conf_inputs) == 145,
	     "Confirmation inputs shall be 145 bytes");

static void pub_key_ready(const uint8_t *pkey)
{
	if (!pkey) {
		BT_WARN("Public key not available");
		return;
	}
	BT_DBG("Local public key ready");
}

int bt_mesh_prov_reset_state(void (*func)(const uint8_t key[BT_PUB_KEY_LEN]))
{
	BT_DBG("bt_mesh_prov_reset_state");

	int err;
	static struct bt_pub_key_cb pub_key_cb;
	const size_t offset = offsetof(struct bt_mesh_prov_link, auth);

	pub_key_cb.func = func ? func : pub_key_ready;

	/* Disable Attention Timer if it was set */
	if (bt_mesh_prov_link.conf_inputs.invite[0]) {
		bt_mesh_attention(NULL, 0);
	}

	atomic_clear(bt_mesh_prov_link.flags);
	(void)memset((uint8_t *)&bt_mesh_prov_link + offset, 0,
		     sizeof(bt_mesh_prov_link) - offset);

	err = bt_pub_key_gen(&pub_key_cb);
	if (err) {
		BT_ERR("Failed to generate public key (%d)", err);
		return err;
	}
	return 0;
}

static bt_mesh_output_action_t output_action(uint8_t action)
{
	switch (action) {
	case OUTPUT_OOB_BLINK:
		return BT_MESH_BLINK;
	case OUTPUT_OOB_BEEP:
		return BT_MESH_BEEP;
	case OUTPUT_OOB_VIBRATE:
		return BT_MESH_VIBRATE;
	case OUTPUT_OOB_NUMBER:
		return BT_MESH_DISPLAY_NUMBER;
	case OUTPUT_OOB_STRING:
		return BT_MESH_DISPLAY_STRING;
	default:
		return BT_MESH_NO_OUTPUT;
	}
}

static bt_mesh_input_action_t input_action(uint8_t action)
{
	switch (action) {
	case INPUT_OOB_PUSH:
		return BT_MESH_PUSH;
	case INPUT_OOB_TWIST:
		return BT_MESH_TWIST;
	case INPUT_OOB_NUMBER:
		return BT_MESH_ENTER_NUMBER;
	case INPUT_OOB_STRING:
		return BT_MESH_ENTER_STRING;
	default:
		return BT_MESH_NO_INPUT;
	}
}

static int check_output_auth(bt_mesh_output_action_t output, uint8_t size)
{
	if (!output) {
		return -EINVAL;
	}

	if (!(bt_mesh_prov->output_actions & output)) {
		return -EINVAL;
	}

	if (size > bt_mesh_prov->output_size) {
		return -EINVAL;
	}

	return 0;
}

static int check_input_auth(bt_mesh_input_action_t input, uint8_t size)
{
	if (!input) {
		return -EINVAL;
	}

	if (!(bt_mesh_prov->input_actions & input)) {
		return -EINVAL;
	}

	if (size > bt_mesh_prov->input_size) {
		return -EINVAL;
	}

	return 0;
}

static void get_auth_string(char *str, uint8_t size)
{
	uint64_t value;

	bt_rand(&value, sizeof(value));

	static const char characters[36] = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ";

	for (int i = 0; i < size; i++) {
		/* pull base-36 digits: */
		int idx = value % 36;

		value = value / 36;
		str[i] = characters[idx];
	}

	str[size] = '\0';

	memcpy(bt_mesh_prov_link.auth, str, size);
	memset(bt_mesh_prov_link.auth + size, 0,
	       sizeof(bt_mesh_prov_link.auth) - size);
}

static uint32_t get_auth_number(bt_mesh_output_action_t output,
				bt_mesh_input_action_t input, uint8_t size)
				{
	const uint32_t divider[PROV_IO_OOB_SIZE_MAX] = { 10, 100, 1000, 10000,
							 100000, 1000000, 10000000, 100000000 };
	uint32_t num = 0;

	bt_rand(&num, sizeof(num));

	if (output == BT_MESH_BLINK ||
	output == BT_MESH_BEEP ||
	output == BT_MESH_VIBRATE ||
	input == BT_MESH_PUSH ||
	input == BT_MESH_TWIST) {
		/* According to the Bluetooth Mesh Profile
		 * Specification Section 5.4.2.4, blink, beep
		 * vibrate, push and twist should be a random integer
		 * between 0 and 10^size, *exclusive*:
		 */
		num = (num % (divider[size - 1] - 1)) + 1;
	} else {
		num %= divider[size - 1];
	}

	sys_put_be32(num, &bt_mesh_prov_link.auth[12]);
	memset(bt_mesh_prov_link.auth, 0, 12);

	return num;
				}

int bt_mesh_prov_auth(bool is_provisioner, uint8_t method, uint8_t action, uint8_t size)
{
	bt_mesh_output_action_t output;
	bt_mesh_input_action_t input;
	int err;

	switch (method) {
	case AUTH_METHOD_NO_OOB:
		if (action || size) {
			return -EINVAL;
		}

		(void)memset(bt_mesh_prov_link.auth, 0, sizeof(bt_mesh_prov_link.auth));
		return 0;
	case AUTH_METHOD_STATIC:
		if (action || size) {
			return -EINVAL;
		}

		atomic_set_bit(bt_mesh_prov_link.flags, OOB_STATIC_KEY);

		return 0;

	case AUTH_METHOD_OUTPUT:
		output = output_action(action);
		if (is_provisioner) {
			if (output == BT_MESH_DISPLAY_STRING) {
				input = BT_MESH_ENTER_STRING;
				atomic_set_bit(bt_mesh_prov_link.flags, WAIT_STRING);
			} else {
				input = BT_MESH_ENTER_NUMBER;
				atomic_set_bit(bt_mesh_prov_link.flags, WAIT_NUMBER);
			}

			return bt_mesh_prov->input(input, size);
		}

		err = check_output_auth(output, size);
		if (err) {
			return err;
		}

		if (output == BT_MESH_DISPLAY_STRING) {
			char str[9];

			atomic_set_bit(bt_mesh_prov_link.flags, NOTIFY_INPUT_COMPLETE);
			get_auth_string(str, size);
			return bt_mesh_prov->output_string(str);
		}

		atomic_set_bit(bt_mesh_prov_link.flags, NOTIFY_INPUT_COMPLETE);
		return bt_mesh_prov->output_number(output,
						   get_auth_number(output, BT_MESH_NO_INPUT, size));
	case AUTH_METHOD_INPUT:
		input = input_action(action);
		if (!is_provisioner) {
			err = check_input_auth(input, size);
			if (err) {
				return err;
			}

			if (input == BT_MESH_ENTER_STRING) {
				atomic_set_bit(bt_mesh_prov_link.flags, WAIT_STRING);
			} else {
				atomic_set_bit(bt_mesh_prov_link.flags, WAIT_NUMBER);
			}

			return bt_mesh_prov->input(input, size);
		}

		if (input == BT_MESH_ENTER_STRING) {
			char str[9];

			atomic_set_bit(bt_mesh_prov_link.flags, NOTIFY_INPUT_COMPLETE);
			get_auth_string(str, size);
			return bt_mesh_prov->output_string(str);
		}

		atomic_set_bit(bt_mesh_prov_link.flags, NOTIFY_INPUT_COMPLETE);
		output = BT_MESH_DISPLAY_NUMBER;
		return bt_mesh_prov->output_number(output,
						   get_auth_number(BT_MESH_NO_OUTPUT, input, size));

	default:
		return -EINVAL;
	}
}

int bt_mesh_input_number(uint32_t num)
{
	BT_DBG("%u", (unsigned) num);

	if (!atomic_test_and_clear_bit(bt_mesh_prov_link.flags, WAIT_NUMBER)) {
		return -EINVAL;
	}

	sys_put_be32(num, &bt_mesh_prov_link.auth[12]);

	bt_mesh_prov_link.role->input_complete();

	return 0;
}

int bt_mesh_input_string(const char *str)
{
	BT_DBG("%s", str);

	if (strlen(str) > PROV_IO_OOB_SIZE_MAX ||
	    strlen(str) > bt_mesh_prov_link.oob_size) {
		return -ENOTSUP;
	}

	if (!atomic_test_and_clear_bit(bt_mesh_prov_link.flags, WAIT_STRING)) {
		return -EINVAL;
	}

	strcpy((char *)bt_mesh_prov_link.auth, str);

	bt_mesh_prov_link.role->input_complete();

	return 0;
}

const struct bt_mesh_prov *bt_mesh_prov_get(void)
{
	return bt_mesh_prov;
}

bool bt_mesh_prov_active(void)
{
	return atomic_test_bit(bt_mesh_prov_link.flags, LINK_ACTIVE);
}

static void prov_recv(const struct prov_bearer *bearer, void *cb_data,
		      		  struct os_mbuf *buf)
{
	static const uint8_t op_len[10] = {
		[PROV_INVITE]         = PDU_LEN_INVITE,
		[PROV_CAPABILITIES]   = PDU_LEN_CAPABILITIES,
		[PROV_START]          = PDU_LEN_START,
		[PROV_PUB_KEY]        = PDU_LEN_PUB_KEY,
		[PROV_INPUT_COMPLETE] = PDU_LEN_INPUT_COMPLETE,
		[PROV_CONFIRM]        = PDU_LEN_CONFIRM,
		[PROV_RANDOM]         = PDU_LEN_RANDOM,
		[PROV_DATA]           = PDU_LEN_DATA,
		[PROV_COMPLETE]       = PDU_LEN_COMPLETE,
		[PROV_FAILED]         = PDU_LEN_FAILED,
	};

	uint8_t type = buf->om_data[0];
	BT_DBG("type 0x%02x len %u", type, buf->om_len);

	if (type >= ARRAY_SIZE(bt_mesh_prov_link.role->op)) {
		BT_ERR("Unknown provisioning PDU type 0x%02x", type);
		bt_mesh_prov_link.role->error(PROV_ERR_NVAL_PDU);
		return;
	}

	if ((type != PROV_FAILED && type != bt_mesh_prov_link.expect) ||
	    !bt_mesh_prov_link.role->op[type]) {
		BT_WARN("Unexpected msg 0x%02x != 0x%02x", type, bt_mesh_prov_link.expect);
		bt_mesh_prov_link.role->error(PROV_ERR_UNEXP_PDU);
		return;
	}

	if (1 + op_len[type] != buf->om_len) {
		BT_ERR("Invalid length %u for type 0x%02x", buf->om_len, type);
		bt_mesh_prov_link.role->error(PROV_ERR_NVAL_FMT);
		return;
	}

	bt_mesh_prov_link.role->op[type](&buf->om_data[1]);
}

static void prov_link_opened(const struct prov_bearer *bearer, void *cb_data)
{
	atomic_set_bit(bt_mesh_prov_link.flags, LINK_ACTIVE);

	BT_ERR("bt_mesh_prov->link_open");
	if (bt_mesh_prov->link_open) {
		bt_mesh_prov->link_open(bearer->type);
	}

	BT_ERR("bt_mesh_prov_link.bearer");
	bt_mesh_prov_link.bearer = bearer;

	BT_ERR("bt_mesh_prov_link.role->link_opened");
	BT_ERR("%p", bt_mesh_prov_link.role);
	BT_ERR("%p", bt_mesh_prov_link.role->link_opened);
	if (bt_mesh_prov_link.role->link_opened) {
		bt_mesh_prov_link.role->link_opened();
	}
	BT_ERR("done");
}

static void prov_link_closed(const struct prov_bearer *bearer, void *cb_data,
			     enum prov_bearer_link_status reason)
{
	if (bt_mesh_prov_link.role->link_closed) {
		bt_mesh_prov_link.role->link_closed();
	}

	if (bt_mesh_prov->link_close) {
		bt_mesh_prov->link_close(bearer->type);
	}
}

static void prov_bearer_error(const struct prov_bearer *bearer, void *cb_data,
			      uint8_t err)
{
	if (bt_mesh_prov_link.role->error) {
		bt_mesh_prov_link.role->error(err);
	}
}

static const struct prov_bearer_cb prov_bearer_cb = {
	.link_opened = prov_link_opened,
	.link_closed = prov_link_closed,
	.error = prov_bearer_error,
	.recv = prov_recv,
};

const struct prov_bearer_cb *bt_mesh_prov_bearer_cb_get(void)
{
	return &prov_bearer_cb;
}

void bt_mesh_prov_complete(uint16_t net_idx, uint16_t addr)
{
	if (bt_mesh_prov->complete) {
		bt_mesh_prov->complete(net_idx, addr);
	}
}

void bt_mesh_prov_reset(void)
{
	BT_DBG("bt_mesh_prov_reset");

	if (IS_ENABLED(CONFIG_BT_MESH_PB_ADV)) {
		pb_adv_reset();
	}

	if (IS_ENABLED(CONFIG_BT_MESH_PB_GATT)) {
		pb_gatt_reset();
	}

	bt_mesh_prov_reset_state(NULL);

	if (bt_mesh_prov->reset) {
		bt_mesh_prov->reset();
	}
}

int bt_mesh_prov_init(const struct bt_mesh_prov *prov_info)
{
	if (!prov_info) {
		BT_ERR("No provisioning context provided");
		return -EINVAL;
	}

	bt_mesh_prov = prov_info;

	if (IS_ENABLED(CONFIG_BT_MESH_PB_ADV)) {
		pb_adv_init();
	}
	if (IS_ENABLED(CONFIG_BT_MESH_PB_GATT)) {
		pb_gatt_init();
	}

	return bt_mesh_prov_reset_state(NULL);
}

#endif /* MYNEWT_VAL(BLE_MESH) */
