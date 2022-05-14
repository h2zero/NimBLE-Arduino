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

#include "nimconfig.h"
#ifdef ESP_PLATFORM
#if CONFIG_BT_NIMBLE_MESH && CONFIG_NIMBLE_CPP_PERSIST_MESH_SETTINGS

#include "config.h"
#include "nvs.h"

#include <string.h>

static struct conf_handler* config_handler;

int conf_parse_name(char *name, int *name_argc, char *name_argv[])
{
    char *tok;
    char *tok_ptr;
    const char *sep = "/";
    int i;

    tok = strtok_r(name, sep, &tok_ptr);

    i = 0;
    while (tok) {
        name_argv[i++] = tok;
        tok = strtok_r(NULL, sep, &tok_ptr);
    }
    *name_argc = i;

    return 0;
}

int conf_load(void)
{
    esp_err_t err;
    nvs_handle handle;

    err = nvs_open(config_handler->ch_name, NVS_READONLY, &handle);
    if (err != ESP_OK) return err;

    nvs_iterator_t it = nvs_entry_find("nvs", config_handler->ch_name, NVS_TYPE_ANY);

    while (it != NULL) {
        nvs_entry_info_t info;
        nvs_entry_info(it, &info);
        it = nvs_entry_next(it);

        size_t required_size = 0;
        err = nvs_get_str(handle, info.key, NULL, &required_size);
        if (err != ESP_OK && err != ESP_ERR_NVS_NOT_FOUND) return err;

        char* val = malloc(required_size);
        if (required_size > 0) {
            err = nvs_get_str(handle, info.key, val, &required_size);
            if (err != ESP_OK) {
                free(val);
                return err;
            }
        }

        int name_argc;
        char *name_argv[8];
        conf_parse_name(info.key, &name_argc, name_argv);

        config_handler->ch_set(name_argc, &name_argv[0], val);
        free(val);
    }

    nvs_close(handle);
    config_handler->ch_commit();
    return ESP_OK;
}

int conf_save_one(const char *name, char *var)
{
    esp_err_t err;
    nvs_handle_t handle;
    int name_argc;
    char *name_argv[CONF_MAX_DIR_DEPTH];
    char n[CONF_MAX_NAME_LEN];

    strcpy(n, name);
    conf_parse_name(n, &name_argc, name_argv);

    err = nvs_open(name_argv[0], NVS_READWRITE, &handle);
    if (err != ESP_OK) return err;

    const char* key = name_argv[1];
    if (name_argc > 2) {
        key = name;
        while (*key != '/') {
            key++;
        }
        key++;
    }

    if (var) {
        err = nvs_set_str(handle, key, var);
        if (err != ESP_OK) return err;
    } else {
        err = nvs_erase_key(handle, key);
        if (err != ESP_OK && err != ESP_ERR_NVS_NOT_FOUND) return err;
    }

    err = nvs_commit(handle);
    if (err != ESP_OK) return err;

    nvs_close(handle);
    return ESP_OK;
}

int conf_register(struct conf_handler *cf)
{
    config_handler = cf;
    return 0;
}

#endif // CONFIG_BT_NIMBLE_MESH && MYNEWT_VAL_BLE_MESH_SETTINGS
#endif // ESP_PLATFORM
