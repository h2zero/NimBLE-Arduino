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
#ifndef __SYS_CONFIG_H_
#define __SYS_CONFIG_H_

#include "../../nimconfig.h"
#if defined(CONFIG_NIMBLE_CPP_IDF)
#  include <os/queue.h>
#else
#  include "nimble/porting/nimble/include/os/queue.h"
#endif

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

#define CONF_MAX_DIR_DEPTH	8	/* max depth of config tree */
#define CONF_MAX_NAME_LEN	(8 * CONF_MAX_DIR_DEPTH)

/**
 * Type of configuration value.
 */
typedef enum conf_type {
    CONF_NONE = 0,
    CONF_DIR,
    /** 8-bit signed integer */
    CONF_INT8,
    /** 16-bit signed integer */
    CONF_INT16,
    /** 32-bit signed integer */
    CONF_INT32,
    /** 64-bit signed integer */
    CONF_INT64,
    /** String */
    CONF_STRING,
    /** Bytes */
    CONF_BYTES,
    /** Floating point */
    CONF_FLOAT,
    /** Double precision */
    CONF_DOUBLE,
    /** Boolean */
    CONF_BOOL,
    /** 8-bit unsigned integer */
    CONF_UINT8,
    /** 16-bit unsigned integer */
    CONF_UINT16,
    /** 32-bit unsigned integer */
    CONF_UINT32,
    /** 64-bit unsigned integer */
    CONF_UINT64,
} __attribute__((__packed__)) conf_type_t;

/**
 * Parameter to commit handler describing where data is going to.
 */
enum conf_export_tgt {
    /** Value is to be persisted */
    CONF_EXPORT_PERSIST,
    /** Value is to be display */
    CONF_EXPORT_SHOW
};

typedef enum conf_export_tgt conf_export_tgt_t;

/**
 * Handler for getting configuration items, this handler is called
 * per-configuration section.  Configuration sections are delimited
 * by '/', for example:
 *
 *  - section/name/value
 *
 * Would be passed as:
 *
 *  - argc = 3
 *  - argv[0] = section
 *  - argv[1] = name
 *  - argv[2] = value
 *
 * The handler returns the value into val, null terminated, up to
 * val_len_max.
 *
 * @param argc          The number of sections in the configuration variable
 * @param argv          The array of configuration sections
 * @param val           A pointer to the buffer to return the configuration
 *                      value into.
 * @param val_len_max   The maximum length of the val buffer to copy into.
 *
 * @return A pointer to val or NULL if error.
 */
typedef char *(*conf_get_handler_t)(int argc, char **argv, char *val, int val_len_max);
typedef char *(*conf_get_handler_ext_t)(int argc, char **argv, char *val, int val_len_max, void *arg);

/**
 * Set the configuration variable pointed to by argc and argv.  See
 * description of ch_get_handler_t for format of these variables.  This sets the
 * configuration variable to the shadow value, but does not apply the configuration
 * change.  In order to apply the change, call the ch_commit() handler.
 *
 * @param argc   The number of sections in the configuration variable.
 * @param argv   The array of configuration sections
 * @param val    The value to configure that variable to
 *
 * @return 0 on success, non-zero error code on failure.
 */
typedef int (*conf_set_handler_t)(int argc, char **argv, char *val);
typedef int (*conf_set_handler_ext_t)(int argc, char **argv, char *val, void *arg);

/**
 * Commit shadow configuration state to the active configuration.
 *
 * @return 0 on success, non-zero error code on failure.
 */
typedef int (*conf_commit_handler_t)(void);
typedef int (*conf_commit_handler_ext_t)(void *arg);

/**
 * Called per-configuration variable being exported.
 *
 * @param name The name of the variable to export
 * @param val  The value of the variable to export
 */
typedef void (*conf_export_func_t)(char *name, char *val);

/**
 * Export all of the configuration variables, calling the export_func
 * per variable being exported.
 *
 * @param export_func  The export function to call.
 * @param tgt          The target of the export, either for persistence or display.
 *
 * @return 0 on success, non-zero error code on failure.
 */
typedef int (*conf_export_handler_t)(conf_export_func_t export_func,
                                     conf_export_tgt_t tgt);
typedef int (*conf_export_handler_ext_t)(conf_export_func_t export_func,
                                         conf_export_tgt_t tgt, void *arg);

/**
 * Configuration handler, used to register a config item/subtree.
 */
struct conf_handler {
    SLIST_ENTRY(conf_handler) ch_list;
    /**
     * The name of the conifguration item/subtree
     */
    char *ch_name;

    /**
     * Whether to use the extended callbacks.
     * false: standard
     * true:  extended
     */
    bool ch_ext;

    /** Get configuration value */
    union {
        conf_get_handler_t ch_get;
        conf_get_handler_ext_t ch_get_ext;
    };

    /** Set configuration value */
    union {
        conf_set_handler_t ch_set;
        conf_set_handler_ext_t ch_set_ext;
    };

    /** Commit configuration value */
    union {
        conf_commit_handler_t ch_commit;
        conf_commit_handler_ext_t ch_commit_ext;
    };

    /** Export configuration value */
    union {
        conf_export_handler_t ch_export;
        conf_export_handler_ext_t ch_export_ext;
    };

    /** Custom argument that gets passed to the extended callbacks */
    void *ch_arg;
};

/**
 * Register a handler for configurations items.
 *
 * @param cf Structure containing registration info.
 *
 * @return 0 on success, non-zero on failure.
 */
int conf_register(struct conf_handler *cf);

/**
 * Load configuration from registered persistence sources. Handlers for
 * configuration subtrees registered earlier will be called for encountered
 * values.
 *
 * @return 0 on success, non-zero on failure.
 */
int conf_load(void);

/**
 * Write a single configuration value to persisted storage (if it has
 * changed value).
 *
 * @param name Name/key of the configuration item.
 * @param var Value of the configuration item.
 *
 * @return 0 on success, non-zero on failure.
 */
int conf_save_one(const char *name, char *var);

#ifdef __cplusplus
}
#endif

#define SYSINIT_PANIC_ASSERT_MSG(rc, msg) assert(rc)

#endif /* __SYS_CONFIG_H_ */