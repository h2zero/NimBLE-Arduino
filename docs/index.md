# Overview

This is a C++ BLE library for the ESP32 that uses the NimBLE host stack instead of bluedroid.  
The aim is to maintain, as much as reasonable, the original bluedroid C++ API while adding new features  
and making improvements in performance, resource use and stability.  

**Testing shows a nearly 50% reduction in flash use and approx. 100kB less ram consumed vs the original!**  
*Your results may vary*  
<br/>

### What is NimBLE?
NimBLE is a completely open source Bluetooth Low Energy stack produced by [Apache](https://github.com/apache/mynewt-nimble).  
It is more suited to resource constrained devices than bluedroid and has now been ported to the ESP32 by Espressif.  
<br/>

# Arduino Installation
Download as .zip and extract to Arduino/libraries folder, or in Arduino IDE from Sketch menu -> Include library -> Add .Zip library.

`#include "NimBLEDevice.h"` at the beginning of your sketch.

Tested and working with esp32-arduino v1.0.2 and 1.0.4 in Arduino IDE v1.8.12 and platform IO.  
<br/>  

# ESP-IDF Installation
### v4.0+
Download as .zip and extract or clone into the components folder in your esp-idf project.

Run menuconfig, go to `Component config->Bluetooth` enable Bluetooth and in `Bluetooth host` NimBLE.  
Configure settings in `NimBLE Options`.  
`#include "NimBLEDevice.h"` in main.cpp.  
Call `NimBLEDevice::init("");` in `app_main`.  
<br/>

### v3.2 & v3.3
The NimBLE component does not come with these versions of IDF.  
A backport that works in these versions has been created and is [available here](https://github.com/h2zero/esp-nimble-component).  
Download or clone that repo into your project/components folder and run menuconfig.
Configure settings in `main menu -> NimBLE Options`.  

`#include "NimBLEDevice.h"` in main.cpp.  
Call `NimBLEDevice::init("");` in `app_main`.  
<br/>  

# Using
This library is intended to be compatible with the original ESP32 BLE functions and types with minor changes.  

See: [Breaking API Changes vs Original](docs/BREAKING_API_CHANGES.md) for details.  

Also see [Improvements_and_updates](docs/Improvements_and_updates.md) for information about non-breaking changes.  

### Arduino
    See the Refactored_original_examples in the examples folder for highlights of the differences with the original library.  

    More advanced examples highlighting many available features are in examples/NimBLE_Server, NimBLE_Client.  
    
    Beacon examples provided by [beegee-tokyo](https://github.com/beegee-tokyo) are in examples/BLE_Beacon_Scanner, BLE_EddystoneTLM_Beacon, BLE_EddystoneURL_Beacon.  

    Change the settings in the nimconfig.h file to customize NimBLE to your project, such as increasing max connections (default == 3).  
<br/>  

# Acknowledgments

* [nkolban](https://github.com/nkolban) and [chegewara](https://github.com/chegewara) for the [original esp32 BLE library](https://github.com/nkolban/esp32-snippets/tree/master/cpp_utils) this project was derived from.
* [beegee-tokyo](https://github.com/beegee-tokyo) for contributing your time to test/debug and contributing the beacon examples.
* [Jeroen88](https://github.com/Jeroen88) for the amazing help debugging and improving the client code.
<br/>  

