#ifndef NIMCONFIG_H
#define NIMCONFIG_H

/***********************************************
 *            Arduino User Options             *
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
 * @brief Un-comment to change the bit used to block tasks during BLE operations
 * that call NimBLEUtils::taskWait. This should be different than any other
 * task notification flag used in the system.
 */
// #define MYNEWT_VAL_NIMBLE_CPP_FREERTOS_TASK_BLOCK_BIT 31

/**
 * @brief Un-comment to disable showing colon characters when printing address.
 */
// #define MYNEWT_VAL_NIMBLE_CPP_ADDR_FMT_EXCLUDE_DELIMITER 1

/**
 * @brief Un-comment to use uppercase letters when printing address.
 */
// #define MYNEWT_VAL_NIMBLE_CPP_ADDR_FMT_UPPERCASE 1

/** @brief Un-comment to enable storing the timestamp when an attribute value is updated\n
 *  This allows for checking the last update time using getTimeStamp() or getValue(time_t*)\n
 *  If disabled, the timestamp returned from these functions will be 0.\n
 *  Disabling timestamps will reduce the memory used for each value.\n
 *  1 = Enabled, 0 = Disabled; Default = Disabled
 */
// #define MYNEWT_VAL_NIMBLE_CPP_ATT_VALUE_TIMESTAMP_ENABLED 0

/** @brief Uncomment to set the default allocation size (bytes) for each attribute if\n
 *  not specified when the constructor is called. This is also the size used when a remote\n
 *  characteristic or descriptor is constructed before a value is read/notified.\n
 *  Increasing this will reduce reallocations but increase memory footprint.\n
 *  Default value is 20. Range: 1 : 512 (BLE_ATT_ATTR_MAX_LEN)
 */
// #define MYNEWT_VAL_NIMBLE_CPP_ATT_VALUE_INIT_LENGTH 20

/** @brief Un-comment to set the debug log messages level from the NimBLE CPP Wrapper.\n
 *  Values: 0 = NONE, 1 = ERROR, 2 = WARNING, 3 = INFO, 4+ = DEBUG\n
 *  Uses approx. 32kB of flash memory.
 */
// #define MYNEWT_VAL_NIMBLE_CPP_LOG_LEVEL 0

/** @brief Un-comment to enable the debug asserts in NimBLE CPP wrapper.*/
// #define MYNEWT_VAL_NIMBLE_CPP_DEBUG_ASSERT_ENABLED 1

/** @brief Un-comment to see NimBLE host return codes as text debug log messages.
 *  Uses approx. 7kB of flash memory.
 */
// #define MYNEWT_VAL_NIMBLE_CPP_ENABLE_RETURN_CODE_TEXT

/** @brief Un-comment to see GAP event codes as text in debug log messages.
 *  Uses approx. 1kB of flash memory.
 */
// #define MYNEWT_VAL_NIMBLE_CPP_ENABLE_GAP_EVENT_CODE_TEXT

/** @brief Un-comment to see advertisement types as text while scanning in debug log messages.
 *  Uses approx. 250 bytes of flash memory.
 */
// #define MYNEWT_VAL_NIMBLE_CPP_ENABLE_ADVERTISEMENT_TYPE_TEXT

/****************************************************
 *         Extended advertising settings            *
 *        NOT FOR USE WITH ORIGINAL ESP32           *
 ***************************************************/

/** @brief Un-comment to enable extended advertising */
// #define MYNEWT_VAL_BLE_EXT_ADV 1

/** @brief Un-comment to set the max number of extended advertising instances (Range: 0 - 4) */
// #define MYNEWT_VAL_BLE_MULTI_ADV_INSTANCES 0

/** @brief Un-comment to set the max extended advertising data size (Range: 31 - 1650) */
// #define MYNEWT_VAL_BLE_EXT_ADV_MAX_SIZE 1650

/***********************************************
 *          End Arduino User Options           *
 **********************************************/

/* This section should not be altered */

#ifdef ESP_PLATFORM
# include "sdkconfig.h"

# ifndef CONFIG_BTDM_SCAN_DUPL_TYPE_DEVICE
#  define CONFIG_BTDM_SCAN_DUPL_TYPE_DEVICE (0)
# endif

# ifndef CONFIG_BTDM_SCAN_DUPL_TYPE_DATA
#  define CONFIG_BTDM_SCAN_DUPL_TYPE_DATA (1)
# endif

# ifndef CONFIG_BTDM_SCAN_DUPL_TYPE_DATA_DEVICE
#  define CONFIG_BTDM_SCAN_DUPL_TYPE_DATA_DEVICE (2)
# endif

# if !defined(CONFIG_BT_NIMBLE_LEGACY_VHCI_ENABLE) && \
     (defined(CONFIG_IDF_TARGET_ESP32) || defined(CONFIG_IDF_TARGET_ESP32C3) || defined(CONFIG_IDF_TARGET_ESP32S3))
#  define CONFIG_BT_NIMBLE_LEGACY_VHCI_ENABLE (1)
# endif

# if !defined(CONFIG_BT_CONTROLLER_DISABLED)
#  define CONFIG_BT_CONTROLLER_DISABLED (0)
# endif

# if CONFIG_IDF_TARGET_ESP32 || CONFIG_IDF_TARGET_ESP32C3 || CONFIG_IDF_TARGET_ESP32S3
#  define NIMBLE_CFG_CONTROLLER 0
# else
#  define NIMBLE_CFG_CONTROLLER CONFIG_BT_CONTROLLER_ENABLED
# endif

# ifndef CONFIG_BT_NIMBLE_USE_ESP_TIMER
#  define CONFIG_BT_NIMBLE_USE_ESP_TIMER (1)
# endif

#define MYNEWT_VAL_BLE_USE_ESP_TIMER (CONFIG_BT_NIMBLE_USE_ESP_TIMER)

#ifdef CONFIG_BT_NIMBLE_EXT_ADV // Workaround for PlatformIO build flags causing redefinition warnings
# undef MYNEWT_VAL_BLE_EXT_ADV
# define MYNEWT_VAL_BLE_EXT_ADV (CONFIG_BT_NIMBLE_EXT_ADV)
#endif

#else // !ESP_PLATFORM
# if defined(NRF51)
#  include "syscfg/devcfg/nrf51cfg.h"
# elif defined(NRF52810_XXAA)
#  include "syscfg/devcfg/nrf52810cfg.h"
# elif defined(NRF52832_XXAA) || defined(NRF52832_XXAB)
#  include "syscfg/devcfg/nrf52832cfg.h"
# elif defined(NRF52833_XXAA)
#  include "syscfg/devcfg/nrf52833cfg.h"
# elif defined(NRF52840_XXAA)
#  include "syscfg/devcfg/nrf52840cfg.h"
# else
#  error No supported mcu config specified
# endif

# ifdef USE_LFRC
#  define MYNEWT_VAL_BLE_LL_SCA (500)
# endif

/* Required definitions for NimBLE */
# define NIMBLE_CFG_CONTROLLER               (1)
# define MYNEWT_VAL_BLE_CONTROLLER           (1)
# define CONFIG_BT_NIMBLE_LEGACY_VHCI_ENABLE (1)
#endif // ESP_PLATFORM

/* Enables the use of Arduino String class for attribute values */
#if defined __has_include
# if __has_include(<Arduino.h>)
#  define NIMBLE_CPP_ARDUINO_STRING_AVAILABLE (1)
# endif
#endif

/* Required macros for all supported devices */

#ifndef CONFIG_BT_NIMBLE_ENABLED
# define CONFIG_BT_NIMBLE_ENABLED (1)
#endif

#ifndef CONFIG_BT_CONTROLLER_ENABLED
# define CONFIG_BT_CONTROLLER_ENABLED (1)
#endif

#endif // NIMCONFIG_H
