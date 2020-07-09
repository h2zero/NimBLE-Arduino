# Breaking API Changes vs Original  
<br/>

# Server API differnces

### Characteristics
When creating a characteristic the properties are now set with `NIMBLE_PROPERTY::XXXX` instead of `BLECharacteristic::XXXX`.

#### Originally
> BLECharacteristic::PROPERTY_READ |  
> BLECharacteristic::PROPERTY_WRITE  

#### Is Now
> NIMBLE_PROPERTY::READ |  
> NIMBLE_PROPERTY::WRITE  
<br/>

#### The full list of properties
> NIMBLE_PROPERTY::READ  
> NIMBLE_PROPERTY::READ_ENC  
> NIMBLE_PROPERTY::READ_AUTHEN  
> NIMBLE_PROPERTY::READ_AUTHOR  
> NIMBLE_PROPERTY::WRITE  
> NIMBLE_PROPERTY::WRITE_NR  
> NIMBLE_PROPERTY::WRITE_ENC  
> NIMBLE_PROPERTY::WRITE_AUTHEN  
> NIMBLE_PROPERTY::WRITE_AUTHOR  
> NIMBLE_PROPERTY::BROADCAST  
> NIMBLE_PROPERTY::NOTIFY   
> NIMBLE_PROPERTY::INDICATE  
<br/>

### Descriptors
Descriptors are now created using the `NimBLECharacteristic::createDescriptor()` method.
 
The previous method `BLECharacteristic::addDescriptor()` is now a private function in the library.

This was done because the NimBLE host automatically creates a 0x2902 descriptor if a characteristic has NOTIFY or INDICATE properties applied.   
Due to this fact, the library also creates one automatically for your application.    
The only reason to manually create this descriptor now is to assign callback functions.   
If you do not require this functionality you can safely exclude the manual creation of the 0x2902 descriptor.   

For any other descriptor, (except 0x2904, see below) it should now be created just as characteristics are    
by using the `NimBLECharacteristic::createDescriptor` method.   
Which are defined as:
```
NimBLEDescriptor* createDescriptor(const char* uuid,
                                   uint32_t properties = 
                                   NIMBLE_PROPERTY::READ |
                                   NIMBLE_PROPERTY::WRITE,
                                   uint16_t max_len = 100);
                                     
NimBLEDescriptor* createDescriptor(NimBLEUUID uuid,
                                   uint32_t properties = 
                                   NIMBLE_PROPERTY::READ |
                                   NIMBLE_PROPERTY::WRITE,
                                   uint16_t max_len = 100);
```
##### Example
```
pDescriptor = pCharacteristic->createDescriptor("ABCD", 
                                                NIMBLE_PROPERTY::READ | 
                                                NIMBLE_PROPERTY::WRITE |
                                                NIMBLE_PROPERTY::WRITE_ENC,
                                                25);
```
Would create a descriptor with the UUID 0xABCD, publicly readable but only writable if paired/bonded (encrypted) and has a max value length of 25 bytes.  
<br/>

For the 0x2904 and 0x2902 descriptor, there is a special class that is created when you call `createDescriptor("2904")`or `createDescriptor("2902")`.

The pointer returned is of the base class `NimBLEDescriptor` but the call will create the derived class of `NimBLE2904` or `NimBLE2902` so you must cast the returned pointer to  
`NimBLE2904` or `NimBLE2902` to access the specific class methods.

##### Example
```
p2904 = (NimBLE2904*)pCharacteristic->createDescriptor("2904");
p2902 = (NimBLE2902*)pCharacteristic->createDescriptor("2902");
```
<br/>

### Server Security
Security is set on the characteristic or descriptor properties by applying one of the following:
> NIMBLE_PROPERTY::READ_ENC  
> NIMBLE_PROPERTY::READ_AUTHEN  
> NIMBLE_PROPERTY::READ_AUTHOR  
> NIMBLE_PROPERTY::WRITE_ENC  
> NIMBLE_PROPERTY::WRITE_AUTHEN  
> NIMBLE_PROPERTY::WRITE_AUTHOR  

When a peer wants to read or write a characteristic or descriptor with any of these properties applied    
it will trigger the pairing process. By default the "just-works" pairing will be performed automatically.   
This can be changed to use passkey authentication or numeric comparison. See [Security Differences](#security-differences) for details.  

<br/>

# Client API Differences
The `NimBLEAdvertisedDeviceCallbacks::onResult()` method now receives a pointer to the    
`NimBLEAdvertisedDevice` object instead of a copy.

`NimBLEClient::connect()` now takes an extra parameter to indicate if the client should download the services   
 database from the peripheral, default value is true.

Defined as:
> NimBLEClient::connect(NimBLEAdvertisedDevice\* device, bool refreshServices = true);  
> NimBLEClient::connect(NimBLEAddress address, uint8_t type = BLE_ADDR_PUBLIC, bool refreshServices = true);  

If set to false the client will use the services database it retrieved from the peripheral last time it connected.  
This allows for faster connections and power saving if the devices just dropped connection and want to reconnect.  
<br/>

> NimBLERemoteCharacteristic::writeValue();  
> NimBLERemoteCharacteristic::registerForNotify();  

Now return true or false to indicate success or failure so you can choose to disconnect or try again.  
<br/>

> NimBLERemoteCharacteristic::registerForNotify();  
Is now **deprecated**.  
> NimBLERemoteCharacteristic::subscribe()  
> NimBLERemoteCharacteristic::unsubscribe()  

Are the new methods added to replace it.  
<br/>

> NimBLERemoteCharacteristic::readUInt8()  
> NimBLERemoteCharacteristic::readUInt16()  
> NimBLERemoteCharacteristic::readUInt32()  
> NimBLERemoteCharacteristic::readFloat()  

Are **deprecated** and NimBLERemoteCharacteristic::readValue(time_t\*, bool) template added to replace them.  
<br/>

> NimBLERemoteService::getCharacteristicsByHandle()  

Has been removed from the API as it is no longer maintained in the library.  
<br/>

> NimBLERemoteCharacteristic::readRawData()  

Has been removed from the API as it stored an unnecessary copy of the data.  
The user application should use NimBLERemoteCharacteristic::readValue or NimBLERemoteCharacteristic::getValue.  
Then cast the returned std::string to the type they wish such as:  
```
uint8_t *val = (uint8_t*)pChr->readValue().data();
```
<br/>

> NimBLEClient::getServices(bool refresh = false)   
> NimBLERemoteCharacteristic::getDescriptors(bool refresh = false)   
> NimBLERemoteService::getCharacteristics(bool refresh = false)    

These methods now take an optional (bool) parameter and return a pointer to `std::vector` instead of `std::map`.   
If passed true it will clear the respective vector and retrieve all the respective attributes from the peripheral.   
If false(default) it will return the respective vector with the currently stored attributes. 

**Removed:** the automatic discovery of all peripheral attributes as they consumed time and resources for data   
the user may not be interested in.  
   
**Added:** NimBLEClient::discoverAttributes() for the user to discover all the peripheral attributes   
to replace the the removed functionality.  
<br/>

> NimBLEClient::getService()  
> NimBLERemoteService::getCharacteristic()  
> NimBLERemoteCharacteristic::getDescriptor()  

These methods will now check the respective vectors for the attribute object and, if not found, will retrieve (only)  
the specified attribute from the peripheral.  

These changes allow more control for the user to manage the resources used for the attributes.    
<br/>

### Client Security
The client will automatically initiate security when the peripheral responds that it's required.  
The default configuration will use "just-works" pairing with no bonding, if you wish to enable bonding see below.  
<br/>

# Security Differences
Security callback functions are now incorporated in the NimBLEServerCallbacks / NimBLEClientCallbacks classes.   
However backward compatibility with the original `BLESecurity` class is retained to minimize app code changes.  

The callback methods are:

> bool     onConfirmPIN(uint32_t pin);  

Receives the pin when using numeric comparison authentication, `return true;` to accept.  
<br/>

> uint32_t onPassKeyRequest();  

For server callback; return the passkey expected from the client.  
For client callback; return the passkey to send to the server.  
<br/>

> void     onAuthenticationComplete(ble_gap_conn_desc\* desc);  

Authentication complete, success or failed information is in `desc`.  
<br/>

Security settings and IO capabilities are now set by the following methods of NimBLEDevice.
> NimBLEDevice::setSecurityAuth(bool bonding, bool mitm, bool sc)  
> NimBLEDevice::setSecurityAuth(uint8_t auth_req)  

Sets the authorization mode for this device.  
<br/>

> NimBLEDevice::setSecurityIOCap(uint8_t iocap)  

Sets the Input/Output capabilities of this device.  
<br/>

> NimBLEDevice::setSecurityInitKey(uint8_t init_key)  

If we are the initiator of the security procedure this sets the keys we will distribute.  
<br/>

> NimBLEDevice::setSecurityRespKey(uint8_t resp_key)  

Sets the keys we are willing to accept from the peer during pairing.  
<br/>

