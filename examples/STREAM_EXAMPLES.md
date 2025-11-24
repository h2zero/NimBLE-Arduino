# NimBLE Stream Examples

This document describes the new Stream-based examples that demonstrate using BLE characteristics with the familiar Arduino Stream interface.

## Overview

The NimBLE Stream classes (`NimBLEStreamServer` and `NimBLEStreamClient`) provide a simple way to use BLE characteristics like serial ports. You can use familiar methods like `print()`, `println()`, `read()`, and `available()` just like you would with `Serial`.

## Available Examples

### 1. NimBLE_Stream_Echo
**Difficulty:** Beginner  
**Purpose:** Introduction to the Stream API

This is the simplest example, perfect for getting started. It creates a BLE server that echoes back any data it receives. Uses default UUIDs for quick setup.

**Key Features:**
- Minimal setup code
- Simple echo functionality
- Good starting point for learning

**Use Case:** Testing BLE connectivity, learning the basics

---

### 2. NimBLE_Stream_Server
**Difficulty:** Intermediate  
**Purpose:** Full-featured server implementation

A complete BLE server example that demonstrates all the major features of `NimBLEStreamServer`. Shows connection management, bidirectional communication, and proper server setup.

**Key Features:**
- Custom service and characteristic UUIDs
- Connection callbacks
- Periodic message sending
- Echo functionality
- MTU negotiation
- Connection parameter updates

**Use Case:** Building custom BLE services, wireless data logging, remote monitoring

---

### 3. NimBLE_Stream_Client
**Difficulty:** Intermediate  
**Purpose:** Full-featured client implementation

Demonstrates how to scan for, connect to, and communicate with a BLE server using `NimBLEStreamClient`. Pairs with the NimBLE_Stream_Server example.

**Key Features:**
- Automatic server discovery
- Connection management
- Reconnection on disconnect
- Bidirectional communication
- Serial passthrough (type in Serial monitor to send via BLE)

**Use Case:** Building BLE client applications, data collection from BLE devices

## Quick Start

### Running the Echo Example

1. Upload `NimBLE_Stream_Echo` to your ESP32
2. Open Serial monitor (115200 baud)
3. Use a BLE app (like nRF Connect) to connect
4. Find service UUID `0xc0de`, characteristic `0xfeed`
5. Subscribe to notifications
6. Write data to see it echoed back

### Running Server + Client Examples

1. Upload `NimBLE_Stream_Server` to one ESP32
2. Upload `NimBLE_Stream_Client` to another ESP32
3. Open Serial monitors for both (115200 baud)
4. Client will automatically find and connect to server
5. Observe bidirectional communication
6. Type in either Serial monitor to send data

## Common Use Cases

- **Wireless Serial Communication:** Replace physical serial connections with BLE
- **Data Logging:** Stream sensor data to a mobile device or another microcontroller
- **Remote Control:** Send commands between devices using familiar print/println
- **Debugging:** Use BLE as a wireless alternative to USB serial debugging
- **IoT Applications:** Simple data exchange between ESP32 devices

## Stream Interface Methods

The examples demonstrate these familiar methods:

**Output (Sending Data):**
- `print(value)` - Print data without newline
- `println(value)` - Print data with newline
- `printf(format, ...)` - Formatted output
- `write(data)` - Write raw bytes

**Input (Receiving Data):**
- `available()` - Check if data is available
- `read()` - Read a single byte
- `peek()` - Preview next byte without removing it

**Status:**
- `hasSubscriber()` - Check if a client is connected (server only)
- `ready()` or `operator bool()` - Check if stream is ready

## Configuration

Both server and client support configuration before calling `begin()`:

```cpp
bleStream.setTxBufSize(2048);       // TX buffer size
bleStream.setRxBufSize(2048);       // RX buffer size
bleStream.setTxTaskStackSize(4096); // Task stack size
bleStream.setTxTaskPriority(1);     // Task priority
```

## Compatibility

These examples work with:
- ESP32 (all variants: ESP32, ESP32-C3, ESP32-S3, etc.)
- Nordic chips (with n-able Arduino core)
- Any BLE client supporting GATT characteristics with notifications

## Additional Resources

- [Arduino Stream Reference](https://www.arduino.cc/reference/en/language/functions/communication/stream/)
- [NimBLE-Arduino Documentation](https://h2zero.github.io/NimBLE-Arduino/)
- Each example includes a detailed README.md file

## Troubleshooting

**Client can't find server:**
- Check that both devices are powered on
- Verify UUIDs match between server and client
- Ensure server advertising is started

**Data not received:**
- Verify client has subscribed to notifications
- Check that buffers aren't full (increase buffer sizes)
- Ensure both `init()` and `begin()` were called

**Connection drops:**
- Check for interference
- Reduce connection interval if needed
- Ensure devices are within BLE range
