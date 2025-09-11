# Arduino command line and platformio config options

Below are the configuration macros for NimBLE-Arduino, updated and reordered to match those found in `nimconfig.h`.  
Uncomment or define these in your build system or `sdkconfig.h`/`platformio.ini` as needed.

---

`MYNEWT_VAL_BLE_MAX_CONNECTIONS`  
Sets the number of simultaneous BLE connections (ESP32 controller max is 9)  
- Default: 3

`MYNEWT_VAL_BLE_ATT_PREFERRED_MTU`  
Sets the default MTU size.  
- Default: 517

`MYNEWT_VAL_BLE_SVC_GAP_DEVICE_NAME`  
Set the default BLE device name  
- Default: "nimble"

`MYNEWT_VAL_BLE_SVC_GAP_APPEARANCE`  
Set the default GAP appearance  
- Default: 0x0

`MYNEWT_VAL_BLE_HS_LOG_LVL`  
Set the debug log messages level from the NimBLE host stack.  
Values: 0 = DEBUG, 1 = INFO, 2 = WARNING, 3 = ERROR, 4 = CRITICAL, 5+ = NONE  
- Default: 5

`MYNEWT_VAL_BLE_ROLE_CENTRAL`  
If 0, NimBLE Client functions will not be included.  
- Reduces flash size by approx. 7kB

`MYNEWT_VAL_BLE_ROLE_OBSERVER`  
If 0, NimBLE Scan functions will not be included.  
- Reduces flash size by approx. 26kB

`MYNEWT_VAL_BLE_ROLE_PERIPHERAL`  
If 0, NimBLE Server functions will not be included.  
- Reduces flash size by approx. 16kB

`MYNEWT_VAL_BLE_ROLE_BROADCASTER`  
If 0, NimBLE Advertising functions will not be included.  
- Reduces flash size by approx. 5kB

`MYNEWT_VAL_BLE_STORE_MAX_BONDS`  
Sets the number of devices allowed to store/bond with  
- Default: 3

`MYNEWT_VAL_BLE_STORE_MAX_CCCDS`  
Sets the maximum number of CCCD subscriptions to store  
- Default: 8

`MYNEWT_VAL_BLE_RPA_TIMEOUT`  
Sets the random address refresh time in seconds  
- Default: 900

`MYNEWT_VAL_MSYS_1_BLOCK_COUNT`  
Set the number of msys blocks for prepare write & prepare responses  
- Default: 12

`MYNEWT_VAL_NIMBLE_MEM_ALLOC_MODE_EXTERNAL`  
Set to 1 to use external PSRAM for NimBLE host  
- Default: 0 (internal)

`MYNEWT_VAL_NIMBLE_PINNED_TO_CORE`  
Sets the core the NimBLE host stack will run on  
- Options: 0 or 1

`MYNEWT_VAL_NIMBLE_HOST_TASK_STACK_SIZE`  
Set the task stack size for the NimBLE host  
- Default: 4096

`MYNEWT_VAL_NIMBLE_CPP_FREERTOS_TASK_BLOCK_BIT`  
Set the bit used to block tasks during BLE operations  
- Default: 31

`MYNEWT_VAL_NIMBLE_CPP_ADDR_FMT_EXCLUDE_DELIMITER`  
Disable showing colon characters when printing address

`MYNEWT_VAL_NIMBLE_CPP_ADDR_FMT_UPPERCASE`  
Use uppercase letters when printing address

`MYNEWT_VAL_NIMBLE_CPP_ATT_VALUE_TIMESTAMP_ENABLED`  
Enable/disable storing the timestamp when an attribute value is updated  
- 1 = Enabled, 0 = Disabled; Default = Disabled

`MYNEWT_VAL_NIMBLE_CPP_ATT_VALUE_INIT_LENGTH`  
Set the default allocation size (bytes) for each attribute  
- Default: 20; Range: 1–512

`MYNEWT_VAL_NIMBLE_CPP_LOG_LEVEL`  
Set the debug log message level from the NimBLE CPP Wrapper  
Values: 0 = NONE, 1 = ERROR, 2 = WARNING, 3 = INFO, 4+ = DEBUG  
- Default: Uses Arduino core debug level

`MYNEWT_VAL_NIMBLE_CPP_DEBUG_ASSERT_ENABLED`  
Enable debug asserts in NimBLE CPP wrapper  
- Default: 0 (disabled)

`MYNEWT_VALNIMBLE_CPP_ENABLE_RETURN_CODE_TEXT`  
If defined, NimBLE host return codes will be printed as text in debug log messages  
- Uses approx. 7kB of flash memory

`MYNEWT_VAL_NIMBLE_CPP_ENABLE_GAP_EVENT_CODE_TEXT`  
If defined, GAP event codes will be printed as text in debug log messages  
- Uses approx. 1kB of flash memory

`MYNEWT_VAL_NIMBLE_CPP_ENABLE_ADVERTISEMENT_TYPE_TEXT`  
If defined, advertisement types will be printed as text while scanning in debug log messages  
- Uses approx. 250 bytes of flash memory

---

## Extended advertising settings (ESP32C3, ESP32S3, ESP32H2 ONLY)

`MYNEWT_VAL_BLE_EXT_ADV`  
Set to 1 to enable extended advertising features.
For Platformio this may cause a redefinition warning with the latest arduino cores,
to avoid this `CONFIG_BT_NIMBLE_EXT_ADV` can be used instead.

`MYNEWT_VAL_BLE_MULTI_ADV_INSTANCES`  
Sets the max number of extended advertising instances  
- Range: 0–4; Default: 0

`MYNEWT_VAL_BLE_EXT_ADV_MAX_SIZE`  
Set the max extended advertising data size  
- Range: 31–1650; Default: 1650

---

**Note:**  
All macro names and descriptions above are matched to those found in `nimconfig.h`.  
Uncomment or define them in your build system as needed.
