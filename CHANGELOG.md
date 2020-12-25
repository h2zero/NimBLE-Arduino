# Changelog

All notable changes to this project will be documented in this file.  

## [Unreleased]

### Added
- Conditional checks added for command line config options in `nimconfig.h` to support custom configuration in platformio.  

- `NimBLEClient::setValue` Now takes an extra bool parameter `response` to enable the use of write with response (default = false).  

- `NimBLEClient::getCharacteristic(uint16_t handle)` Enabling the use of the characteristic handle to be used to find 
the NimBLERemoteCharacteristic object.  

- `NimBLEHIDDevice` class added by wakwak-koba.  

- `NimBLEServerCallbacks::onDisconnect` overloaded callback added to provide a ble_gap_conn_desc parameter for the application  
to obtain information about the disconnected client.  

- Conditional checks in `nimconfig.h` for command line defined macros to support platformio config settings.  

### Changed
- `NimBLERemoteCharacteristic::subscribe` and `NimBLERemoteCharacteristic::registerForNotify` will now set the callback
  regardless of the existance of the CCCD and return true unless the descriptor write operation failed.  

- Advertising tx power level is now sent in the advertisement packet instead of scan response.  

- `NimBLEScan` When the scan ends the scan stopped flag is now set before calling the scan complete callback (if used)  
this allows the starting of a new scan from the callback function.  

### Fixed
- `FreeRTOS` compile errors resolved in latest Ardruino core and IDF v3.3.  

- Multiple instances of `time()` called inside critical sections caused sporadic crashes, these have been moved out of critical regions.  

- Advertisement type now correctly set when using non-connectable (advertiser only) mode.  

- Advertising payload length correction, now accounts for appearance.  

- (Arduino) Ensure controller mode is set to BLE Only.  

## [1.0.2] - 2020-09-13

### Changed

- `NimBLEAdvertising::start` Now takes 2 optional parameters, the first is the duration to advertise for (in seconds), the second is a  
callback that is invoked when advertsing ends and takes a pointer to a `NimBLEAdvertising` object (similar to the `NimBLEScan::start` API).

- (Arduino) Maximum BLE connections can now be altered by only changing the value of `CONFIG_BT_NIMBLE_MAX_CONNECTIONS` in `nimconfig.h`.
Any changes to the controller max connection settings in `sdkconfig.h` will now have no effect when using this library.

- (Arduino) Revert the previous change to fix the advertising start delay. Instead a replacement fix that routes all BLE controller commands from  
a task running on core 0 (same as the controller) has been implemented. This improves response times and reliability for all BLE functions.

## [1.0.1] - 2020-09-02

### Added

- Empty `NimBLEAddress` constructor: `NimBLEAddress()` produces an address of 00:00:00:00:00:00 type 0.
- Documentation of the difference of NimBLEAddress::getNative vs the original bluedroid library.

### Changed

- notify_callback typedef is now defined as std::function to enable the use of std::bind to call a class member function.

### Fixed

- Fix advertising start delay when first called.


## [1.0.0] - 2020-08-22

First stable release.

All the original library functionality is complete and many extras added with full documentation.
