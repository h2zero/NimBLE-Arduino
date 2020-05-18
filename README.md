# *** UPDATE ***
**Breaking change:** Client and scan now use `std::vector` instead of `std::map` for storing the remote attribute database.   
   
This change will affect your application code if you use `NimBLEClient::getServices()` or `NimBLERemoteService::getCharacteristics()`   
in your application as they now return a pointer to `std::vector` of the respective attributes.   

In addition `NimBLERemoteService::getCharacteristicsByHandle()` has been removed as it is no longer maintained in the library.

These changes were necessary due to the amount of resources required to use `std::map`, it was not justifed by any benfit it provided.   
   
It is expected that there will be minimal impact on most applications, if you need help adjusting your code please create an issue.   

# NimBLE-Arduino
A fork of the NimBLE stack restructured for compilation in the Ardruino IDE with a CPP library for use with ESP32.

Why? Because the Bluedroid library is too bulky. 

Initial client code testing has resulted in code size reduction of ~115k and reduced ram consumption of ~37k.

Server code testing results from @beegee-toyo [from the project here](https://github.com/beegee-tokyo/ESP32WiFiBLE-NimBLE):


### Memory usage (compilation output)
#### Arduino BLE library
```log
RAM:   [==        ]  17.7% (used 58156 bytes from 327680 bytes)    
Flash: [========  ]  76.0% (used 1345630 bytes from 1769472 bytes)    
```
#### NimBLE-Arduino library
```log
RAM:   [=         ]  14.5% (used 47476 bytes from 327680 bytes)    
Flash: [=======   ]  69.5% (used 911378 bytes from 1310720 bytes)    
```
### Memory usage after **`setup()`** function
#### Arduino BLE library
**`Internal Total heap 259104, internal Free Heap 91660`**    
#### NimBLE-Arduino library
**`Internal Total heap 290288, internal Free Heap 182344`** 
  
  
# Installation:

Download as .zip and extract to Arduino/libraries folder, or in Arduino IDE from Sketch menu -> Include library -> Add .Zip library.

`#include "NimBLEDevice.h"` at the beginning of your sketch.

Tested and working with esp32-arduino v1.0.2 and 1.0.4 in Arduino IDE v1.8.12 and platform IO.


# Usage: 

This library is intended to be compatible with the original ESP32 BLE functions and types with minor changes.

Check the Refactored_original_examples in the examples folder for highlights of the differences with the original library.

More advanced examples highlighting many available features are in examples/ NimBLE_Server, NimBLE_Client.

Beacon examples provided by @beegee-tokyo are in examples/ BLE_Beacon_Scanner, BLE_EddystoneTLM_Beacon, BLE_EddystoneURL_Beacon.   

Change the settings in the `nimconfig.h` file to customize NimBLE to your project, such as increasing max connections, default is 3.


# Continuing development:

This Library is tracking the esp-nimble repo, nimble-1.2.0-idf master branch, currently [@fead24e.](https://github.com/espressif/esp-nimble)

Also tracking the NimBLE related changes in esp-idf, master branch, currently [@2bc28bb.](https://github.com/espressif/esp-idf/tree/master/components/bt/host/nimble)

# Acknowledgments:

* @nkolban and @chegewara for the [original esp32 BLE library](https://github.com/nkolban/esp32-snippets) this project was derived from.
* @beegee-tokyo for contributing your time to test/debug and contributing the beacon examples.


# Todo:

1. Code cleanup.
2. Create documentation.
3. Expose more NimBLE features.
4. Add BLE Mesh code.

