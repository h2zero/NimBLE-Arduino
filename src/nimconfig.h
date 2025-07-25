#pragma once

#ifdef ESP_PLATFORM
#include "sdkconfig.h"
#else
#include "ext_nimble_config.h"
#endif

#include "nimconfig_rename.h"

/***********************************************
 * Arduino user-config options start here
 **********************************************/

/** @brief Un-comment to change the number of simultaneous connections (esp controller max is 9) */
// #define MYNEWT_VAL_BLE_MAX_CONNECTIONS 3

/** @brief Un-comment to change the default MTU size */
// #define MYNEWT_VAL_BLE_ATT_PREFERRED_MTU 517

/** @brief Un-comment to change default device name */
// #define MYNEWT_VAL_BLE_SVC_GAP_DEVICE_NAME "nimble"

/** @brief Un-comment to set the debug log messages level from the NimBLE host stack.\n
 *  Values: 0 = DEBUG, 1 = INFO, 2 = WARNING, 3 = ERROR, 4 = CRITICAL, 5+ = NONE\n
 *  Uses approx. 32kB of flash memory.
 */
 // #define MYNEWT_VAL_BLE_HS_LOG_LVL 5

/** @brief Un-comment to change the default GAP appearance */
// #define  MYNEWT_VAL_BLE_SVC_GAP_APPEARANCE 0x0

 /** @brief Un-comment if not using NimBLE Client functions \n
 *  Reduces flash size by approx. 7kB.
 */
// #define MYNEWT_VAL_BLE_ROLE_CENTRAL 0

/** @brief Un-comment if not using NimBLE Scan functions \n
 *  Reduces flash size by approx. 26kB.
 */
// #define MYNEWT_VAL_BLE_ROLE_OBSERVER 0

/** @brief Un-comment if not using NimBLE Server functions \n
 *  Reduces flash size by approx. 16kB.
 */
// #define MYNEWT_VAL_BLE_ROLE_PERIPHERAL 0

/** @brief Un-comment if not using NimBLE Advertising functions \n
 *  Reduces flash size by approx. 5kB.
 */
// #define MYNEWT_VAL_BLE_ROLE_BROADCASTER 0

/** @brief Un-comment to change the number of devices allowed to store/bond with */
// #define MYNEWT_VAL_BLE_STORE_MAX_BONDS 3

/** @brief Un-comment to change the maximum number of CCCD subscriptions to store */
// #define MYNEWT_VAL_BLE_STORE_MAX_CCCDS 8

/** @brief Un-comment to change the random address refresh time (in seconds) */
// #define MYNEWT_VAL_BLE_RPA_TIMEOUT 900

/**
 * @brief Un-comment to change the number of MSYS buffers available.
 * @details MSYS is a system level mbuf registry. For prepare write & prepare \n
 * responses MBUFs are allocated out of msys_1 pool. This may need to be increased if\n
 * you are sending large blocks of data with a low MTU. E.g: 512 bytes with 23 MTU will fail.
 */
// #define MYNEWT_VAL_MSYS_1_BLOCK_COUNT 12

/** @brief Un-comment to use external PSRAM for the NimBLE host */
// #define MYNEWT_VAL_NIMBLE_MEM_ALLOC_MODE_EXTERNAL 1

/** @brief Un-comment to change the core NimBLE host runs on */
// #define MYNEWT_VAL_NIMBLE_PINNED_TO_CORE 0

/** @brief Un-comment to change the stack size for the NimBLE host task */
// #define MYNEWT_VAL_NIMBLE_HOST_TASK_STACK_SIZE 4096

/**
 * @brief Un-comment to use mbedtls instead of tinycrypt.
 * @details This could save approximately 8k of flash if already using mbedtls for other functionality.
 */
// #define MYNEWT_VAL_BLE_CRYPTO_STACK_MBEDTLS 1

/****************************************************
 *         Extended advertising settings            *
 *        NOT FOR USE WITH ORIGINAL ESP32           *
 ***************************************************/

/** @brief Un-comment to enable extended advertising */
// #define MYNEWT_VAL_BLE_EXT_ADV 1

/** @brief Un-comment to set the max number of extended advertising instances (Range: 0 - 4) */
// #define MYNEWT_VAL_BLE_MULTI_ADV_INSTANCES 1

/** @brief Un-comment to set the max extended advertising data size (Range: 31 - 1650) */
// #define MYNEWT_VAL_BLE_EXT_ADV_MAX_SIZE 251

/****************************************************
 *        END Extended advertising settings         *
 ***************************************************/

/**
 * @brief Un-comment to change the bit used to block tasks during BLE operations
 * that call NimBLEUtils::taskWait. This should be different than any other
 * task notification flag used in the system.
 */
// #define CONFIG_NIMBLE_CPP_FREERTOS_TASK_BLOCK_BIT 31

/**
 * @brief Un-comment to disable showing colon characters when printing address.
 */
// #define CONFIG_NIMBLE_CPP_ADDR_FMT_EXCLUDE_DELIMITER 1

/**
 * @brief Un-comment to use uppercase letters when printing address.
 */
// #define CONFIG_NIMBLE_CPP_ADDR_FMT_UPPERCASE 1

/** @brief Un-comment to enable storing the timestamp when an attribute value is updated\n
 *  This allows for checking the last update time using getTimeStamp() or getValue(time_t*)\n
 *  If disabled, the timestamp returned from these functions will be 0.\n
 *  Disabling timestamps will reduce the memory used for each value.\n
 *  1 = Enabled, 0 = Disabled; Default = Disabled
 */
// #define CONFIG_NIMBLE_CPP_ATT_VALUE_TIMESTAMP_ENABLED 0

/** @brief Uncomment to set the default allocation size (bytes) for each attribute if\n
 *  not specified when the constructor is called. This is also the size used when a remote\n
 *  characteristic or descriptor is constructed before a value is read/notified.\n
 *  Increasing this will reduce reallocations but increase memory footprint.\n
 *  Default value is 20. Range: 1 : 512 (BLE_ATT_ATTR_MAX_LEN)
 */
// #define CONFIG_NIMBLE_CPP_ATT_VALUE_INIT_LENGTH 20

 /** @brief Un-comment to set the debug log messages level from the NimBLE CPP Wrapper.\n
 *  Values: 0 = NONE, 1 = ERROR, 2 = WARNING, 3 = INFO, 4+ = DEBUG\n
 *  Uses approx. 32kB of flash memory.
 */
 // #define CONFIG_NIMBLE_CPP_LOG_LEVEL 0

/** @brief Un-comment to enable the debug asserts in NimBLE CPP wrapper.*/
// #define CONFIG_NIMBLE_CPP_DEBUG_ASSERT_ENABLED 1

/** @brief Un-comment to see NimBLE host return codes as text debug log messages.
 *  Uses approx. 7kB of flash memory.
 */
// #define CONFIG_NIMBLE_CPP_ENABLE_RETURN_CODE_TEXT

/** @brief Un-comment to see GAP event codes as text in debug log messages.
 *  Uses approx. 1kB of flash memory.
 */
// #define CONFIG_NIMBLE_CPP_ENABLE_GAP_EVENT_CODE_TEXT

/** @brief Un-comment to see advertisement types as text while scanning in debug log messages.
 *  Uses approx. 250 bytes of flash memory.
 */
// #define CONFIG_NIMBLE_CPP_ENABLE_ADVERTISEMENT_TYPE_TEXT

/**********************************
 End Arduino user-config
**********************************/

/* This section should not be altered */
#ifndef MYNEWT_VAL_BLE_ROLE_BROADCASTER
#define MYNEWT_VAL_BLE_ROLE_BROADCASTER (1)
#endif

#ifndef MYNEWT_VAL_BLE_ROLE_CENTRAL
#define MYNEWT_VAL_BLE_ROLE_CENTRAL (1)
#endif

#ifndef MYNEWT_VAL_BLE_ROLE_OBSERVER
#define MYNEWT_VAL_BLE_ROLE_OBSERVER (1)
#endif

#ifndef MYNEWT_VAL_BLE_ROLE_PERIPHERAL
#define MYNEWT_VAL_BLE_ROLE_PERIPHERAL (1)
#endif

#ifndef MYNEWT_VAL_NIMBLE_PINNED_TO_CORE
#define MYNEWT_VAL_NIMBLE_PINNED_TO_CORE (0)
#endif

#ifndef MYNEWT_VAL_NIMBLE_HOST_TASK_STACK_SIZE
#define MYNEWT_VAL_NIMBLE_HOST_TASK_STACK_SIZE (4096)
#endif

#ifndef MYNEWT_VAL_NIMBLE_CONTROLLER_TASK_STACK_SIZE
#define MYNEWT_VAL_NIMBLE_CONTROLLER_TASK_STACK_SIZE (4096)
#endif

#ifndef MYNEWT_VAL_NIMBLE_MEM_ALLOC_MODE_EXTERNAL
#define MYNEWT_VAL_NIMBLE_MEM_ALLOC_MODE_INTERNAL (1)
#endif

#ifndef MYNEWT_VAL_BLE_MAX_CONNECTIONS
#define MYNEWT_VAL_BLE_MAX_CONNECTIONS (3)
#endif

#ifndef MYNEWT_VAL_BLE_STORE_MAX_BONDS
#define MYNEWT_VAL_BLE_STORE_MAX_BONDS (3)
#endif

#ifndef MYNEWT_VAL_BLE_STORE_MAX_CCCDS
#define MYNEWT_VAL_BLE_STORE_MAX_CCCDS (8)
#endif

#ifndef MYNEWT_VAL_BLE_ATT_PREFERRED_MTU
#define MYNEWT_VAL_BLE_ATT_PREFERRED_MTU (255)
#endif

#ifndef MYNEWT_VAL_BLE_SVC_GAP_APPEARANCE
#define MYNEWT_VAL_BLE_SVC_GAP_APPEARANCE (0x0)
#endif

#ifndef MYNEWT_VAL_MSYS_1_BLOCK_COUNT
#define MYNEWT_VAL_MSYS_1_BLOCK_COUNT (12)
#endif

#ifndef MYNEWT_VAL_MSYS_1_BLOCK_SIZE
#define MYNEWT_VAL_MSYS_1_BLOCK_SIZE (256)
#endif

#ifndef MYNEWT_VAL_MSYS_2_BLOCK_COUNT
#define MYNEWT_VAL_MSYS_2_BLOCK_COUNT (0)
#endif

#ifndef MYNEWT_VAL_MSYS_2_BLOCK_SIZE
#define MYNEWT_VAL_MSYS_2_BLOCK_SIZE (0)
#endif

#ifndef MYNEWT_VAL_BLE_TRANSPORT_EVT_DISCARDABLE_COUNT
#define MYNEWT_VAL_BLE_TRANSPORT_EVT_DISCARDABLE_COUNT (16)
#endif

#ifndef MYNEWT_VAL_BLE_RPA_TIMEOUT
#define MYNEWT_VAL_BLE_RPA_TIMEOUT (900)
#endif

#ifndef MYNEWT_VAL_BLE_HS_LOG_LVL
#define MYNEWT_VAL_BLE_HS_LOG_LVL (5)
#endif

#ifndef MYNEWT_VAL_BLE_MESH_DEVICE_NAME
#define MYNEWT_VAL_BLE_SVC_GAP_DEVICE_NAME "nimble"
#endif

#ifndef MYNEWT_VAL_BLE_LL_WHITELIST_SIZE
#define MYNEWT_VAL_BLE_LL_WHITELIST_SIZE (12)
#endif

/** @brief Set if CCCD's and bond data should be stored in NVS */
#define MYNEWT_VAL_BLE_STORE_CONFIG_PERSIST (1)

/** @brief Allow legacy paring */
#define MYNEWT_VAL_BLE_SM_LEGACY (1)

/** @brief Allow BLE secure connections */
#define MYNEWT_VAL_BLE_SM_SC (1)

/** @brief Max device name length (bytes) */
#define MYNEWT_VAL_BLE_SVC_GAP_DEVICE_NAME_MAX_LENGTH (31)

/** @brief ACL Buffer count */
#define MYNEWT_VAL_BLE_TRANSPORT_ACL_FROM_LL_COUNT (12)

/** @brief ACL Buffer size */
#define MYNEWT_VAL_BLE_TRANSPORT_ACL_SIZE (255)

/** @brief Transport (HCI) Event Buffer size */
#if MYNEWT_VAL_BLE_EXT_ADV
#  define MYNEWT_VAL_BLE_TRANSPORT_EVT_SIZE (257)
#else
#  define MYNEWT_VAL_BLE_TRANSPORT_EVT_SIZE (70)
#endif

/** @brief Number of high priority HCI event buffers */
#define MYNEWT_VAL_BLE_TRANSPORT_EVT_COUNT (30)

#define MYNEWT_VAL_BLE_L2CAP_COC_SDU_BUFF_COUNT (1)
#define MYNEWT_VAL_BLE_EATT_CHAN_NUM (0)
#define MYNEWT_VAL_BLE_SVC_GAP_CENTRAL_ADDRESS_RESOLUTION (-1)
#define MYNEWT_VAL_BLE_GATT_MAX_PROCS (4)
#define MYNEWT_VAL_BLE_HS_STOP_ON_SHUTDOWN_TIMEOUT (2000)

#ifndef CONFIG_BT_NIMBLE_ENABLED
#define CONFIG_BT_NIMBLE_ENABLED (1)
#endif

#ifndef CONFIG_BT_CONTROLLER_ENABLED
#define CONFIG_BT_CONTROLLER_ENABLED (1)
#endif

#ifndef MYNEWT_VAL_BLE_CRYPTO_STACK_MBEDTLS
#define MYNEWT_VAL_BLE_CRYPTO_STACK_MBEDTLS (0)
#endif

#ifdef ESP_PLATFORM
#ifndef CONFIG_BTDM_SCAN_DUPL_TYPE_DEVICE
#define CONFIG_BTDM_SCAN_DUPL_TYPE_DEVICE (0)
#endif

#ifndef CONFIG_BTDM_SCAN_DUPL_TYPE_DATA
#define CONFIG_BTDM_SCAN_DUPL_TYPE_DATA (1)
#endif

#ifndef CONFIG_BTDM_SCAN_DUPL_TYPE_DATA_DEVICE
#define CONFIG_BTDM_SCAN_DUPL_TYPE_DATA_DEVICE (2)
#endif

#if !defined(CONFIG_BT_NIMBLE_LEGACY_VHCI_ENABLE) && (defined(CONFIG_IDF_TARGET_ESP32) || \
defined(CONFIG_IDF_TARGET_ESP32C3) || defined(CONFIG_IDF_TARGET_ESP32S3))
#define CONFIG_BT_NIMBLE_LEGACY_VHCI_ENABLE (1)
#endif

#ifdef CONFIG_IDF_TARGET_ESP32
#define MYNEWT_VAL_BLE_HS_FLOW_CTRL_TX_ON_DISCONNECT (1)
#endif

#if !defined(CONFIG_BT_CONTROLLER_DISABLED)
#define CONFIG_BT_CONTROLLER_DISABLED (0)
#endif

#if MYNEWT_VAL_BLE_EXT_ADV
#  if defined(CONFIG_IDF_TARGET_ESP32)
#    error Extended advertising is not supported on ESP32.
#  endif
#endif

#ifndef CONFIG_BT_NIMBLE_USE_ESP_TIMER
#define CONFIG_BT_NIMBLE_USE_ESP_TIMER (1)
#endif

#endif // ESP_PLATFORM

#if !defined(MYNEWT_VAL_BLE_MULTI_ADV_INSTANCES)
#  define MYNEWT_VAL_BLE_MULTI_ADV_INSTANCES (1)
#endif

#if !defined(MYNEWT_VAL_BLE_EXT_ADV_MAX_SIZE)
#  define MYNEWT_VAL_BLE_EXT_ADV_MAX_SIZE (251)
#endif

/* Enables the use of Arduino String class for attribute values */
#if defined __has_include
#  if __has_include (<Arduino.h>)
#    define NIMBLE_CPP_ARDUINO_STRING_AVAILABLE (1)
#  endif
#endif

#ifndef CONFIG_NIMBLE_CPP_DEBUG_ASSERT_ENABLED
#define CONFIG_NIMBLE_CPP_DEBUG_ASSERT_ENABLED (0)
#endif

#ifndef CONFIG_NIMBLE_CPP_FREERTOS_TASK_BLOCK_BIT
#define CONFIG_NIMBLE_CPP_FREERTOS_TASK_BLOCK_BIT (31)
#endif
