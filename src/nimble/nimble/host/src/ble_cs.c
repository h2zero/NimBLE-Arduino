/*
 * Licensed to the Apache Software Foundation (ASF) under one
 * or more contributor license agreements.  See the NOTICE file
 * distributed with this work for additional information
 * regarding copyright ownership.  The ASF licenses this file
 * to you under the Apache License, Version 2.0 (the
 * "License"); you may not use this file except in compliance
 * with the License.  You may obtain a copy of the License at
 *
 *  http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing,
 * software distributed under the License is distributed on an
 * "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
 * KIND, either express or implied.  See the License for the
 * specific language governing permissions and limitations
 * under the License.
 */

#include <inttypes.h>
#include "syscfg/syscfg.h"
#include "ble_hs_mbuf_priv.h"

#if MYNEWT_VAL(BLE_CHANNEL_SOUNDING)

#include "nimble/porting/nimble/include/os/os_mbuf.h"
#include "nimble/nimble/host/include/host/ble_hs_log.h"
#include "nimble/nimble/host/include/host/ble_hs.h"
#include "nimble/nimble/host/include/host/ble_cs.h"
#include "nimble/nimble/include/nimble/hci_common.h"
#include "sys/queue.h"
#include "ble_hs_hci_priv.h"

struct ble_cs_rd_rem_supp_cap_cp {
    uint16_t conn_handle;
} __attribute__((packed));

struct ble_cs_wr_cached_rem_supp_cap_cp {
    uint16_t conn_handle;
    uint8_t num_config_supported;
    uint16_t max_consecutive_procedures_supported;
    uint8_t num_antennas_supported;
    uint8_t max_antenna_paths_supported;
    uint8_t roles_supported;
    uint8_t optional_modes_supported;
    uint8_t rtt_capability;
    uint8_t rtt_aa_only_n;
    uint8_t rtt_sounding_n;
    uint8_t rtt_random_payload_n;
    uint16_t optional_nadm_sounding_capability;
    uint16_t optional_nadm_random_capability;
    uint8_t optional_cs_sync_phys_supported;
    uint16_t optional_subfeatures_supported;
    uint16_t optional_t_ip1_times_supported;
    uint16_t optional_t_ip2_times_supported;
    uint16_t optional_t_fcs_times_supported;
    uint16_t optional_t_pm_times_supported;
    uint8_t t_sw_time_supported;
} __attribute__((packed));
struct ble_cs_wr_cached_rem_supp_cap_rp {
    uint16_t conn_handle;
} __attribute__((packed));

struct ble_cs_sec_enable_cp {
    uint16_t conn_handle;
} __attribute__((packed));

struct ble_cs_set_def_settings_cp {
    uint16_t conn_handle;
    uint8_t role_enable;
    uint8_t cs_sync_antenna_selection;
    uint8_t max_tx_power;
} __attribute__((packed));
struct ble_cs_set_def_settings_rp {
    uint16_t conn_handle;
} __attribute__((packed));

struct ble_cs_rd_rem_fae_cp {
    uint16_t conn_handle;
} __attribute__((packed));

struct ble_cs_wr_cached_rem_fae_cp {
    uint16_t conn_handle;
    uint8_t remote_fae_table[72];
} __attribute__((packed));
struct ble_cs_wr_cached_rem_fae_rp {
    uint16_t conn_handle;
} __attribute__((packed));

struct ble_cs_create_config_cp {
    uint16_t conn_handle;
    uint8_t config_id;
    /* If the config should be created on the remote controller too */
    uint8_t create_context;
    /* The main mode to be used in the CS procedures */
    uint8_t main_mode_type;
    /* The sub mode to be used in the CS procedures */
    uint8_t sub_mode_type;
    /* Minimum/maximum number of CS main mode steps to be executed before
     * a submode step.
     */
    uint8_t min_main_mode_steps;
    uint8_t max_main_mode_steps;
    /* The number of main mode steps taken from the end of the last
     * CS subevent to be repeated at the beginning of the current CS subevent
     * directly after the last mode 0 step of that event
     */
    uint8_t main_mode_repetition;
    uint8_t mode_0_steps;
    uint8_t role;
    uint8_t rtt_type;
    uint8_t cs_sync_phy;
    uint8_t channel_map[10];
    uint8_t channel_map_repetition;
    uint8_t channel_selection_type;
    uint8_t ch3c_shape;
    uint8_t ch3c_jump;
    uint8_t companion_signal_enable;
} __attribute__((packed));

struct ble_cs_remove_config_cp {
    uint16_t conn_handle;
    uint8_t config_id;
} __attribute__((packed));

struct ble_cs_set_chan_class_cp {
    uint8_t channel_classification[10];
} __attribute__((packed));

struct ble_cs_set_proc_params_cp {
    uint16_t conn_handle;
    uint8_t config_id;
    /* The maximum duration of each CS procedure (time = N × 0.625 ms) */
    uint16_t max_procedure_len;
    /* The minimum and maximum number of connection events between
     * consecutive CS procedures. Ignored if only one CS procedure. */
    uint16_t min_procedure_interval;
    uint16_t max_procedure_interval;
    /* The maximum number of consecutive CS procedures to be scheduled */
    uint16_t max_procedure_count;
    /* Minimum/maximum suggested durations for each CS subevent in microseconds.
     * Only 3 bytes meaningful. */
    uint32_t min_subevent_len;
    uint32_t max_subevent_len;
    /* Antenna Configuration Index (ACI) for swithing during phase measuement */
    uint8_t tone_antenna_config_selection;
    /* The remote device’s Tx PHY. A 4 bits bitmap. */
    uint8_t phy;
    /* How many more or fewer of transmit power levels should the remote device’s
     * Tx PHY use during the CS tones and RTT transmission */
    uint8_t tx_power_delta;
    /* Preferred peer-ordered antenna elements to be used by the peer for
     * the selected antenna configuration (ACI). A 4 bits bitmap. */
    uint8_t preferred_peer_antenna;
    /* SNR Output Index (SOI) for SNR control adjustment. */
    uint8_t snr_control_initiator;
    uint8_t snr_control_reflector;
} __attribute__((packed));
struct ble_cs_set_proc_params_rp {
    uint16_t conn_handle;
} __attribute__((packed));

struct ble_cs_proc_enable_cp {
    uint16_t conn_handle;
    uint8_t config_id;
    uint8_t enable;
} __attribute__((packed));

struct ble_cs_state {
    uint8_t op;
    ble_cs_event_fn *cb;
    void *cb_arg;
};

static struct ble_cs_state cs_state;

static int
ble_cs_call_event_cb(struct ble_cs_event *event)
{
    int rc;

    if (cs_state.cb != NULL) {
        rc = cs_state.cb(event, cs_state.cb_arg);
    } else {
        rc = 0;
    }

    return rc;
}

static void
ble_cs_call_procedure_complete_cb(uint16_t conn_handle, uint8_t status)
{
    struct ble_cs_event event;

    memset(&event, 0, sizeof event);
    event.type = BLE_CS_EVENT_CS_PROCEDURE_COMPLETE;
    event.procedure_complete.conn_handle = conn_handle;
    event.procedure_complete.status = status;
    ble_cs_call_event_cb(&event);
}

static int
ble_cs_rd_loc_supp_cap(void)
{
    int rc;
    struct ble_hci_le_cs_rd_loc_supp_cap_rp rp;

    rc = ble_hs_hci_cmd_tx(BLE_HCI_OP(BLE_HCI_OGF_LE,
                                      BLE_HCI_OCF_LE_CS_RD_LOC_SUPP_CAP),
                           NULL, 0, &rp, sizeof(rp));

    rp.max_consecutive_procedures_supported = le16toh(rp.max_consecutive_procedures_supported);
    rp.optional_nadm_sounding_capability = le16toh(rp.optional_nadm_sounding_capability);
    rp.optional_nadm_random_capability = le16toh(rp.optional_nadm_random_capability);
    rp.optional_subfeatures_supported = le16toh(rp.optional_subfeatures_supported);
    rp.optional_t_ip1_times_supported = le16toh(rp.optional_t_ip1_times_supported);
    rp.optional_t_ip2_times_supported = le16toh(rp.optional_t_ip2_times_supported);
    rp.optional_t_fcs_times_supported = le16toh(rp.optional_t_fcs_times_supported);
    rp.optional_t_pm_times_supported = le16toh(rp.optional_t_pm_times_supported);
    (void) rp;

    return rc;
}

static int
ble_cs_rd_rem_supp_cap(const struct ble_cs_rd_rem_supp_cap_cp *cmd)
{
    struct ble_hci_le_cs_rd_rem_supp_cap_cp cp;

    cp.conn_handle = htole16(cmd->conn_handle);

    return ble_hs_hci_cmd_tx(BLE_HCI_OP(BLE_HCI_OGF_LE,
                                        BLE_HCI_OCF_LE_CS_RD_REM_SUPP_CAP),
                             &cp, sizeof(cp), NULL, 0);
}

static int
ble_cs_wr_cached_rem_supp_cap(const struct ble_cs_wr_cached_rem_supp_cap_cp *cmd,
                              struct ble_cs_wr_cached_rem_supp_cap_rp *rsp)
{
    struct ble_hci_le_cs_wr_cached_rem_supp_cap_cp cp;
    struct ble_hci_le_cs_wr_cached_rem_supp_cap_rp rp;
    int rc;

    cp.conn_handle = htole16(cmd->conn_handle);
    cp.num_config_supported = cmd->num_config_supported;
    cp.max_consecutive_procedures_supported = htole16(cmd->max_consecutive_procedures_supported);
    cp.num_antennas_supported = cmd->num_antennas_supported;
    cp.max_antenna_paths_supported = cmd->max_antenna_paths_supported;
    cp.roles_supported = cmd->roles_supported;
    cp.optional_modes_supported = cmd->optional_modes_supported;
    cp.rtt_capability = cmd->rtt_capability;
    cp.rtt_aa_only_n = cmd->rtt_aa_only_n;
    cp.rtt_sounding_n = cmd->rtt_sounding_n;
    cp.rtt_random_payload_n = cmd->rtt_random_payload_n;
    cp.optional_nadm_sounding_capability = htole16(cmd->optional_nadm_sounding_capability);
    cp.optional_nadm_random_capability = htole16(cmd->optional_nadm_random_capability);
    cp.optional_cs_sync_phys_supported = cmd->optional_cs_sync_phys_supported;
    cp.optional_subfeatures_supported = htole16(cmd->optional_subfeatures_supported);
    cp.optional_t_ip1_times_supported = htole16(cmd->optional_t_ip1_times_supported);
    cp.optional_t_ip2_times_supported = htole16(cmd->optional_t_ip2_times_supported);
    cp.optional_t_fcs_times_supported = htole16(cmd->optional_t_fcs_times_supported);
    cp.optional_t_pm_times_supported = htole16(cmd->optional_t_pm_times_supported);
    cp.t_sw_time_supported = cmd->t_sw_time_supported;

    rc = ble_hs_hci_cmd_tx(BLE_HCI_OP(BLE_HCI_OGF_LE,
                                      BLE_HCI_OCF_LE_CS_WR_CACHED_REM_SUPP_CAP),
                           &cp, sizeof(cp), &rp, sizeof(rp));

    rsp->conn_handle = le16toh(rp.conn_handle);

    return rc;
}

static int
ble_cs_sec_enable(const struct ble_cs_sec_enable_cp *cmd)
{
    struct ble_hci_le_cs_sec_enable_cp cp;

    cp.conn_handle = htole16(cmd->conn_handle);

    return ble_hs_hci_cmd_tx(BLE_HCI_OP(BLE_HCI_OGF_LE,
                                        BLE_HCI_OCF_LE_CS_SEC_ENABLE),
                             &cp, sizeof(cp), NULL, 0);
}

static int
ble_cs_set_def_settings(const struct ble_cs_set_def_settings_cp *cmd,
                        struct ble_cs_set_def_settings_rp *rsp)
{
    struct ble_hci_le_cs_set_def_settings_cp cp;
    struct ble_hci_le_cs_set_def_settings_rp rp;
    int rc;

    cp.conn_handle = htole16(cmd->conn_handle);
    cp.role_enable = cmd->role_enable;
    cp.cs_sync_antenna_selection = cmd->cs_sync_antenna_selection;
    cp.max_tx_power = cmd->max_tx_power;

    rc = ble_hs_hci_cmd_tx(BLE_HCI_OP(BLE_HCI_OGF_LE,
                                      BLE_HCI_OCF_LE_CS_SET_DEF_SETTINGS),
                           &cp, sizeof(cp), &rp, sizeof(rp));

    rsp->conn_handle = le16toh(rp.conn_handle);

    return rc;
}

static int
ble_cs_rd_rem_fae(const struct ble_cs_rd_rem_fae_cp *cmd)
{
    struct ble_hci_le_cs_rd_rem_fae_cp cp;

    cp.conn_handle = htole16(cmd->conn_handle);

    return ble_hs_hci_cmd_tx(BLE_HCI_OP(BLE_HCI_OGF_LE,
                                        BLE_HCI_OCF_LE_CS_RD_REM_FAE),
                             &cp, sizeof(cp), NULL, 0);
}

static int
ble_cs_wr_cached_rem_fae(const struct ble_cs_wr_cached_rem_fae_cp *cmd,
                         struct ble_cs_wr_cached_rem_fae_rp *rsp)
{
    struct ble_hci_le_cs_wr_cached_rem_fae_cp cp;
    struct ble_hci_le_cs_wr_cached_rem_fae_rp rp;
    int rc;

    cp.conn_handle = htole16(cmd->conn_handle);
    memcpy(cp.remote_fae_table, cmd->remote_fae_table, 72);

    rc = ble_hs_hci_cmd_tx(BLE_HCI_OP(BLE_HCI_OGF_LE,
                                      BLE_HCI_OCF_LE_CS_WR_CACHED_REM_FAE),
                           &cp, sizeof(cp), &rp, sizeof(rp));

    rsp->conn_handle = le16toh(rp.conn_handle);

    return rc;
}

static int
ble_cs_create_config(const struct ble_cs_create_config_cp *cmd)
{
    struct ble_hci_le_cs_create_config_cp cp;

    cp.conn_handle = htole16(cmd->conn_handle);
    cp.config_id = cmd->config_id;
    cp.create_context = cmd->create_context;
    cp.main_mode_type = cmd->main_mode_type;
    cp.sub_mode_type = cmd->sub_mode_type;
    cp.min_main_mode_steps = cmd->min_main_mode_steps;
    cp.max_main_mode_steps = cmd->max_main_mode_steps;
    cp.main_mode_repetition = cmd->main_mode_repetition;
    cp.mode_0_steps = cmd->mode_0_steps;
    cp.role = cmd->role;
    cp.rtt_type = cmd->rtt_type;
    cp.cs_sync_phy = cmd->cs_sync_phy;
    memcpy(cp.channel_map, cmd->channel_map, 10);
    cp.channel_map_repetition = cmd->channel_map_repetition;
    cp.channel_selection_type = cmd->channel_selection_type;
    cp.ch3c_shape = cmd->ch3c_shape;
    cp.ch3c_jump = cmd->ch3c_jump;
    cp.companion_signal_enable = cmd->companion_signal_enable;

    return ble_hs_hci_cmd_tx(BLE_HCI_OP(BLE_HCI_OGF_LE,
                                        BLE_HCI_OCF_LE_CS_CREATE_CONFIG),
                             &cp, sizeof(cp), NULL, 0);
}

static int
ble_cs_remove_config(const struct ble_cs_remove_config_cp *cmd)
{
    struct ble_hci_le_cs_remove_config_cp cp;

    cp.conn_handle = htole16(cmd->conn_handle);
    cp.config_id = cmd->config_id;

    return ble_hs_hci_cmd_tx(BLE_HCI_OP(BLE_HCI_OGF_LE,
                                        BLE_HCI_OCF_LE_CS_REMOVE_CONFIG),
                             &cp, sizeof(cp), NULL, 0);
}

static int
ble_cs_set_chan_class(const struct ble_cs_set_chan_class_cp *cmd)
{
    struct ble_hci_le_cs_set_chan_class_cp cp;
    int rc;

    memcpy(cp.channel_classification, cmd->channel_classification, 10);

    rc = ble_hs_hci_cmd_tx(BLE_HCI_OP(BLE_HCI_OGF_LE,
                                      BLE_HCI_OCF_LE_CS_SET_CHAN_CLASS),
                           &cp, sizeof(cp), NULL, 0);

    return rc;
}

static int
ble_cs_set_proc_params(const struct ble_cs_set_proc_params_cp *cmd,
                       struct ble_cs_set_proc_params_rp *rsp)
{
    struct ble_hci_le_cs_set_proc_params_cp cp;
    struct ble_hci_le_cs_set_proc_params_rp rp;
    int rc;

    cp.conn_handle = htole16(cmd->conn_handle);
    cp.config_id = cmd->config_id;
    cp.max_procedure_len = htole16(cmd->max_procedure_len);
    cp.min_procedure_interval = htole16(cmd->min_procedure_interval);
    cp.max_procedure_interval = htole16(cmd->max_procedure_interval);
    cp.max_procedure_count = htole16(cmd->max_procedure_count);
    put_le24(cp.min_subevent_len, cmd->min_subevent_len);
    put_le24(cp.max_subevent_len, cmd->max_subevent_len);
    cp.tone_antenna_config_selection = cmd->tone_antenna_config_selection;
    cp.phy = cmd->phy;
    cp.tx_power_delta = cmd->tx_power_delta;
    cp.preferred_peer_antenna = cmd->preferred_peer_antenna;
    cp.snr_control_initiator = cmd->snr_control_initiator;
    cp.snr_control_reflector = cmd->snr_control_reflector;

    rc = ble_hs_hci_cmd_tx(BLE_HCI_OP(BLE_HCI_OGF_LE,
                                      BLE_HCI_OCF_LE_CS_SET_PROC_PARAMS),
                           &cp, sizeof(cp), &rp, sizeof(rp));

    rsp->conn_handle = le16toh(rp.conn_handle);

    return rc;
}

static int
ble_cs_proc_enable(const struct ble_cs_proc_enable_cp *cmd)
{
    struct ble_hci_le_cs_proc_enable_cp cp;

    cp.conn_handle = htole16(cmd->conn_handle);
    cp.config_id = cmd->config_id;
    cp.enable = cmd->enable;

    return ble_hs_hci_cmd_tx(BLE_HCI_OP(BLE_HCI_OGF_LE,
                                        BLE_HCI_OCF_LE_CS_PROC_ENABLE),
                             &cp, sizeof(cp), NULL, 0);
}

int
ble_hs_hci_evt_le_cs_rd_rem_supp_cap_complete(uint8_t subevent, const void *data,
                                              unsigned int len)
{
    int rc;
    const struct ble_hci_ev_le_subev_cs_rd_rem_supp_cap_complete *ev = data;
    struct ble_cs_set_def_settings_cp set_cmd;
    struct ble_cs_set_def_settings_rp set_rsp;
    struct ble_cs_rd_rem_fae_cp fae_cmd;

    if (len != sizeof(*ev) || ev->status) {
        return BLE_HS_ECONTROLLER;
    }

    BLE_HS_LOG(DEBUG, "CS capabilities exchanged");

    /* TODO: Save the remote capabilities somewhere */

    set_cmd.conn_handle = le16toh(ev->conn_handle);
    /* Only initiator role is enabled */
    set_cmd.role_enable = 0x01;
    /* Use antenna with ID 0x01 */
    set_cmd.cs_sync_antenna_selection = 0x01;
    /* Set max TX power to the max supported */
    set_cmd.max_tx_power = 0x7F;

    rc = ble_cs_set_def_settings(&set_cmd, &set_rsp);
    if (rc) {
        BLE_HS_LOG(DEBUG, "Failed to set the default CS settings, err %dt", rc);

        return rc;
    }

    /* Read the mode 0 Frequency Actuation Error table */
    fae_cmd.conn_handle = le16toh(ev->conn_handle);
    rc = ble_cs_rd_rem_fae(&fae_cmd);
    if (rc) {
        BLE_HS_LOG(DEBUG, "Failed to read FAE table");
    }

    return rc;
}

int
ble_hs_hci_evt_le_cs_rd_rem_fae_complete(uint8_t subevent, const void *data,
                                         unsigned int len)
{
    const struct ble_hci_ev_le_subev_cs_rd_rem_fae_complete *ev = data;
    struct ble_cs_create_config_cp cmd;
    int rc;

    if (len != sizeof(*ev) || ev->status) {
        return BLE_HS_ECONTROLLER;
    }

    cmd.conn_handle = le16toh(ev->conn_handle);
    /* The config will use ID 0x00 */
    cmd.config_id = 0x00;
    /* Create the config on the remote controller too */
    cmd.create_context = 0x01;
    /* Measure phase rotations in main mode */
    cmd.main_mode_type = 0x01;
    /* Do not use sub mode for now. */
    cmd.sub_mode_type = 0xFF;
    /* Range from which the number of CS main mode steps to execute
     * will be randomly selected.
     */
    cmd.min_main_mode_steps = 0x02;
    cmd.max_main_mode_steps = 0x06;
    /* The number of main mode steps to be repeated at the beginning of
     * the current CS, irrespectively if there are some overlapping main
     * mode steps from previous CS subevent or not.
     */
    cmd.main_mode_repetition = 0x00;
    /* Number of CS mode 0 steps to be included at the beginning of
     * each CS subevent
     */
    cmd.mode_0_steps = 0x03;
    /* Take the Initiator role */
    cmd.role = 0x00;
    cmd.rtt_type = 0x01;
    cmd.cs_sync_phy = 0x01;
    memcpy(cmd.channel_map, (uint8_t[10]) {0x0a, 0xfa, 0xcf, 0xac, 0xfa, 0xc0}, 10);
    cmd.channel_map_repetition = 0x01;
    /* Use Channel Selection Algorithm #3b */
    cmd.channel_selection_type = 0x00;
    /* Ignore these as used only with #3c algorithm */
    cmd.ch3c_shape = 0x00;
    cmd.ch3c_jump = 0x00;
    /* EDLC/ECLD attack protection not supported */
    cmd.companion_signal_enable = 0x00;

    /* Create CS config */
    rc = ble_cs_create_config(&cmd);
    if (rc) {
        BLE_HS_LOG(DEBUG, "Failed to create CS config");
    }

    return rc;
}

int
ble_hs_hci_evt_le_cs_sec_enable_complete(uint8_t subevent, const void *data,
                                         unsigned int len)
{
    int rc;
    struct ble_cs_set_proc_params_cp cmd;
    struct ble_cs_set_proc_params_rp rsp;
    struct ble_cs_proc_enable_cp enable_cmd;
    const struct ble_hci_ev_le_subev_cs_sec_enable_complete *ev = data;

    if (len != sizeof(*ev)) {
        BLE_HS_LOG(DEBUG, "Failed to enable CS security");

        return BLE_HS_ECONTROLLER;
    }

    BLE_HS_LOG(DEBUG, "CS setup phase completed");

    cmd.conn_handle = le16toh(ev->conn_handle);
    cmd.config_id = 0x00;
    /* The maximum duration of each CS procedure (time = N × 0.625 ms) */
    cmd.max_procedure_len = 8;
    /* The maximum number of consecutive CS procedures to be scheduled
     * as part of this measurement
     */
    cmd.max_procedure_count = 0x0001;
    /* The minimum and maximum number of connection events between
     * consecutive CS procedures. Ignored if only one CS procedure.
     */
    cmd.min_procedure_interval = 0x0000;
    cmd.max_procedure_interval = 0x0000;
    /* Minimum/maximum suggested durations for each CS subevent in microseconds.
     * 1250us and 5000us selected.
     */
    cmd.min_subevent_len = 1250;
    cmd.max_subevent_len = 5000;
    /* Use ACI 0 as we have only one antenna on each side */
    cmd.tone_antenna_config_selection = 0x00;
    /* Use LE 1M PHY for CS procedures */
    cmd.phy = 0x01;
    /* Transmit power delta set to 0x80 means Host does not have a recommendation. */
    cmd.tx_power_delta = 0x80;
    /* Preferred antenna array elements to use. We have only a single antenna here. */
    cmd.preferred_peer_antenna = 0x01;
    /* SNR Output Index (SOI) for SNR control adjustment. 0xFF means SNR control
     * is not to be applied.
     */
    cmd.snr_control_initiator = 0xff;
    cmd.snr_control_reflector = 0xff;

    rc = ble_cs_set_proc_params(&cmd, &rsp);
    if (rc) {
        BLE_HS_LOG(DEBUG, "Failed to set CS procedure parameters");
    }

    enable_cmd.conn_handle = le16toh(ev->conn_handle);
    enable_cmd.config_id = 0x00;
    enable_cmd.enable = 0x01;

    rc = ble_cs_proc_enable(&enable_cmd);
    if (rc) {
        BLE_HS_LOG(DEBUG, "Failed to enable CS procedure");
    }

    return rc;
}

int
ble_hs_hci_evt_le_cs_config_complete(uint8_t subevent, const void *data,
                                     unsigned int len)
{
    int rc;
    const struct ble_hci_ev_le_subev_cs_config_complete *ev = data;
    struct ble_cs_sec_enable_cp cmd;

    if (len != sizeof(*ev) || ev->status) {
        return BLE_HS_ECONTROLLER;
    }

    cmd.conn_handle = le16toh(ev->conn_handle);

    /* Exchange CS security keys */
    rc = ble_cs_sec_enable(&cmd);
    if (rc) {
        BLE_HS_LOG(DEBUG, "Failed to enable CS security");
        ble_cs_call_procedure_complete_cb(le16toh(ev->conn_handle), ev->status);
    }

    return 0;
}

int
ble_hs_hci_evt_le_cs_proc_enable_complete(uint8_t subevent, const void *data,
                                          unsigned int len)
{
    const struct ble_hci_ev_le_subev_cs_proc_enable_complete *ev = data;

    if (len != sizeof(*ev) || ev->status) {
        return BLE_HS_ECONTROLLER;
    }

    return 0;
}

int
ble_hs_hci_evt_le_cs_subevent_result(uint8_t subevent, const void *data,
                                     unsigned int len)
{
    const struct ble_hci_ev_le_subev_cs_subevent_result *ev = data;

    if (len != sizeof(*ev)) {
        return BLE_HS_ECONTROLLER;
    }

    return 0;
}

int
ble_hs_hci_evt_le_cs_subevent_result_continue(uint8_t subevent, const void *data,
                                              unsigned int len)
{
    const struct ble_hci_ev_le_subev_cs_subevent_result_continue *ev = data;

    if (len != sizeof(*ev)) {
        return BLE_HS_ECONTROLLER;
    }

    return 0;
}

int
ble_hs_hci_evt_le_cs_test_end_complete(uint8_t subevent, const void *data,
                                       unsigned int len)
{
    const struct ble_hci_ev_le_subev_cs_test_end_complete *ev = data;

    if (len != sizeof(*ev)) {
        return BLE_HS_ECONTROLLER;
    }

    return 0;
}

int
ble_cs_initiator_procedure_start(const struct ble_cs_initiator_procedure_start_params *params)
{
    struct ble_cs_rd_rem_supp_cap_cp cmd;
    int rc;

    /* Channel Sounding setup phase:
     * 1. Set local default CS settings
     * 2. Exchange CS capabilities with the remote
     * 3. Read or write the mode 0 Frequency Actuation Error table
     * 4. Create CS configurations
     * 5. Start the CS Security Start procedure
     */

    (void)ble_cs_set_chan_class;
    (void)ble_cs_remove_config;
    (void)ble_cs_wr_cached_rem_fae;
    (void)ble_cs_wr_cached_rem_supp_cap;
    (void)ble_cs_rd_loc_supp_cap;

    cs_state.cb = params->cb;
    cs_state.cb_arg = params->cb_arg;

    cmd.conn_handle = params->conn_handle;
    rc = ble_cs_rd_rem_supp_cap(&cmd);
    if (rc) {
        BLE_HS_LOG(DEBUG, "Failed to read local supported CS capabilities,"
                   "err %dt", rc);
    }

    return rc;
}

int
ble_cs_initiator_procedure_terminate(uint16_t conn_handle)
{
    return 0;
}

int
ble_cs_reflector_setup(struct ble_cs_reflector_setup_params *params)
{
    cs_state.cb = params->cb;
    cs_state.cb_arg = params->cb_arg;

    return 0;
}
#endif
