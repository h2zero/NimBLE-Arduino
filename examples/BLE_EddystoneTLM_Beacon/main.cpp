/*
* main.cpp
 *
 * Purpose: This file exists ONLY to satisfy PlatformIO's build system.
 * The NimBLE Server example normally compiles fine in the Arduino IDE
 * without needing a separate main.cpp. PlatformIO requires a main entry
 * (setup() and loop()) for any build environment, so this file exists
 * purely to make PIO happy.
 */

#ifdef PLATFORMIO_BUILD
    #include "BLE_EddystoneTLM_Beacon.ino"
#endif
