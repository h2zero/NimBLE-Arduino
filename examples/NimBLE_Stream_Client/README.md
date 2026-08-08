# NimBLE Stream Client Example

This example demonstrates how to use the `NimBLEStreamClient` class to connect to a BLE GATT server and communicate using the familiar Arduino Stream interface.

## Features

- Uses Arduino Stream interface (print, println, write, etc.)
- Automatic server discovery and connection
- Bidirectional communication using the Nordic UART Service (NUS)
- TX: `NimBLEStreamClient` writes to the server's RX characteristic (6E400002)
- RX: direct notification callback subscribed to the server's TX characteristic (6E400003)
- Automatic reconnection on disconnect
- Compatible with the NimBLE_Stream_Server example and NUS terminal apps
- Similar usage to Serial communication

## How it Works

1. Scans for BLE devices advertising the NUS service UUID
2. Connects to the server and discovers the TX and RX characteristics
3. Initializes `NimBLEStreamClient` with the server's RX characteristic for writing (our TX path)
4. Subscribes directly to the server's TX characteristic to receive notifications (our RX path)
5. Uses familiar Stream methods like `print()`, `println()`, and `write()` to send data

## Usage

1. First, upload the NimBLE_Stream_Server example to one ESP32
2. Upload this client sketch to another ESP32
3. The client will automatically:
   - Scan for the server
   - Connect when found
   - Set up the stream interface
   - Begin bidirectional communication
4. You can also type in the Serial monitor to send data to the server

## Service UUIDs (Nordic UART Service)

Must match the server:
- Service: `6E400001-B5A3-F393-E0A9-E50E24DCCA9E`
- TX Characteristic (server → client, client subscribes): `6E400003-B5A3-F393-E0A9-E50E24DCCA9E`
- RX Characteristic (client → server, client writes): `6E400002-B5A3-F393-E0A9-E50E24DCCA9E`

## Serial Monitor Output

The example displays:
- Server discovery progress
- Connection status
- All data received from the server (via notification callback)
- Confirmation of data sent to the server

## Testing

Run with NimBLE_Stream_Server to see bidirectional communication:
- Server sends periodic status messages
- Client sends periodic uptime messages
- Server echoes data back to the client
- You can send data from the Serial monitor
