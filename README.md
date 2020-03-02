# NimBLE-Arduino
A fork of the NimBLE library structured for compilation with Ardruino, designed for use with ESP32.

Why? Because the Bluedroid library is too bulky, In testing I have found an initial code size reduction of ~115k and reduced ram usage by ~37k.

# Installation:

Download .zip 
Extract to Arduino/libraries folder or in Arduino IDE from Sketch menu -> Include library -> Add .Zip library.


# Use: 

This library is intended to be compatible with the current BLE library classes, functions and types with minor changes. 

At this time only the client code has been (nearly) fully implemented and work has started on the server code.


# New Features:

Multiple clients are supported, up to 3 presently.

# Todo:

1. Implement server code.
2. Create documentation.
3. Examples.

