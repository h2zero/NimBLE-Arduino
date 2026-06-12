# NimBLE Stream Server Example

This example demonstrates how to use the `NimBLEStreamServer` class to create a BLE GATT server that behaves like a serial port using the familiar Arduino Stream interface.

## Features

- Uses Arduino Stream interface (print, println, read, available, etc.)
- Automatic connection management
- Bidirectional communication using the Nordic UART Service (NUS)
- Buffered TX/RX using ring buffers
- Compatible with NUS terminal apps (nRF UART, Serial Bluetooth Terminal, etc.)
- Similar usage to Serial communication

## How it Works

1. Creates the NUS service with two characteristics (TX and RX)
2. Initializes two `NimBLEStreamServer` instances — one for TX (notifications) and one for RX (writes)
3. `NimBLEStreamServer::begin()` automatically enables only the direction supported by the characteristic's properties: the TX characteristic (NOTIFY-only) enables the TX buffer; the RX characteristic (WRITE-only) enables the RX buffer
4. Uses familiar Stream methods like `print()`, `println()`, `read()`, and `available()`
5. Automatically handles connection state and MTU negotiation

## Usage

1. Upload this sketch to your ESP32
2. The device will advertise as "NimBLE-Stream"
3. Connect with a BLE client (such as the NimBLE_Stream_Client example or a NUS terminal app)
4. Once connected, the server will:
   - Send periodic messages to the client via the TX characteristic
   - Echo back any data received from the client on the RX characteristic
   - Display all communication on the Serial monitor

## Service UUIDs (Nordic UART Service)

- Service: `6E400001-B5A3-F393-E0A9-E50E24DCCA9E`
- TX Characteristic (server notifies → client subscribes): `6E400003-B5A3-F393-E0A9-E50E24DCCA9E`
- RX Characteristic (client writes → server receives): `6E400002-B5A3-F393-E0A9-E50E24DCCA9E`

## Compatible With

- NimBLE_Stream_Client example
- nRF Connect mobile app
- nRF UART app
- Serial Bluetooth Terminal app
- Any BLE client that supports the Nordic UART Service (NUS)
