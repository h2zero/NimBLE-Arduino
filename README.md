# NimBLE-Arduino
A fork of the NimBLE stack restructured for compilation in the Ardruino IDE with a CPP library for use with ESP32.  

**Note for IDF users: This repo will not compile correctly in ESP-IDF. An ESP-IDF component version of this library can be [found here.](https://github.com/h2zero/esp-nimble-cpp)**

This library **significantly** reduces resource usage and improves performance for ESP32 BLE applications as compared    
with the bluedroid based library. The goal is to maintain, as much as reasonable, compatibility with the original   
library but refactored to use the NimBLE stack. In addition, this library will be more actively developed and maintained   
to provide improved capabilites and stability over the original.  
<br/>

## Resource use improvement

### (Original) BLE_client example comparison (Debug):
#### Arduino BLE Library   
Sketch uses **1216377** bytes (58%) of program storage space.   
Memory after connection: Free Heap: **171548**  

#### NimBLE-Arduino library
Sketch uses **617256** bytes (29%) of program storage space.   
Memory after connection: Free Heap: **270336**   
***
### (Original) BLE_notify example comparison (Debug):   
#### Arduino BLE Library
Sketch uses **1208409** bytes (57%) of program storage space.   
Memory after connection: Free Heap: **173300**   

#### NimBLE-Arduino library   
Sketch uses **603432** bytes (28%) of program storage space.   
Memory after connection: Free Heap: **269792**  

**As shown: there is nearly a 50% reduction in flash use and approx. 100kB less ram consumed!**  
<br/>

# Installation
Download as .zip and extract to Arduino/libraries folder, or in Arduino IDE from Sketch menu -> Include library -> Add .Zip library.

`#include "NimBLEDevice.h"` at the beginning of your sketch.

Tested and working with esp32-arduino in Arduino IDE and platform IO.  
<br/>

# Using
This library is intended to be compatible with the original ESP32 BLE functions and types with minor changes.  
See: [The migration guide](docs/Migration_guide.md) for details. 

Also see [Improvements_and_updates](docs/Improvements_and_updates.md) for information about non-breaking changes.

[Full API documentation and class list can be found here.](https://h2zero.github.io/esp-nimble-cpp/)  

Check the Refactored_original_examples in the examples folder for highlights of the differences with the original library.

More advanced examples highlighting many available features are in examples/ NimBLE_Server, NimBLE_Client.

Beacon examples provided by @beegee-tokyo are in examples/ BLE_Beacon_Scanner, BLE_EddystoneTLM_Beacon, BLE_EddystoneURL_Beacon.   

Change the settings in the `nimconfig.h` file to customize NimBLE to your project, such as increasing max connections, default is 3.  

**Note To increase max connections in Arduino it is also required to change the controller max connections defined in sdkconfig.h.**  

This is located in your Arduino/hardware/espressif/esp32/tools/sdk/include/config folder.

The values in `sdkconfig.h` you will need to change are:  
```
#define CONFIG_BTDM_CONTROLLER_BLE_MAX_CONN 3
#define CONFIG_BTDM_CONTROLLER_BLE_MAX_CONN_EFF 3
```
In `nimconfig.h` the value is:  
```
#define CONFIG_BT_NIMBLE_MAX_CONNECTIONS 3
```
Espressif has stated the hard maximum connections is 9.  
<br/>

# Continuing development
This Library is tracking the esp-nimble repo, nimble-1.2.0-idf master branch, currently [@95bd864.](https://github.com/espressif/esp-nimble)  

Also tracking the NimBLE related changes in ESP-IDF, master branch, currently [@2ef4890.](https://github.com/espressif/esp-idf/tree/master/components/bt/host/nimble)  
<br/>

# Acknowledgments
* [nkolban](https://github.com/nkolban) and [chegewara](https://github.com/chegewara) for the [original esp32 BLE library](https://github.com/nkolban/esp32-snippets/tree/master/cpp_utils) this project was derived from.
* [beegee-tokyo](https://github.com/beegee-tokyo) for contributing your time to test/debug and contributing the beacon examples.
* [Jeroen88](https://github.com/Jeroen88) for the amazing help debugging and improving the client code.  
<br/>  

# Todo
1. Document nimconfig.
2. Examples.
3. Improve documentation. 
<br/>  
