# Fairsight Luminary Alert Radiance Emitter
*Expanding original codebase for long distance lights.*

## What does this repository solve?
+ [x] Wifi provisioning via BLE.
+ [x] Adding the possibility of one-to-many connection instead of just one-to-one (Any number of devices, scalable logic)
+ [x] Fixed duplicated codebase for each light, by provisioning value via BLE preference.

## Installation Guide
*This guide is specific to current implementation on my side (Adafruit HUZZAH32 - ESP32)  and can be changed for specific purposes.*

1. Create Adafruit IO account, to get the necessary username and API key.
2. In the same place where the config sketch is, create a file called `secrets.h`.
3. Paste this block of code inside and change all the necessary variables:
```
#ifndef SECRETS_H
#define SECRETS_H

#define IO_USERNAME  "your_username"
#define IO_KEY       "your_IO_key"

#endif
```
4. Verify and upload the program to your device. If there is a problem with uploading, make sure to use **No OTA** Partition scheme
5. When program uploads for the first time, the device will start with BLE provisioning process, since it doesnt know current Wifi name, password and device role.
6. Download nRF Connect app (or any other similar BLE connection app). Under **Devices** select **FLARE** device and connect to it.
7. Under service *0xFFB0*, you have 3 characteristics: *0XFFB1, 0XFFB2 and 0xFFB3*. To each characteristic, upload as a text:
  + **0XFFB1**: Wifi name
  + **0XFFB2**: Wifi password
  + **0xFFB3**: Device Role (1,2,3,...) - unique integer for a device
8. When the device connects to the Wifi, it blinks, restarts and after a minute is ready to use.
9. With each press of the button, the device will send a signal to other devices connected to the current IO_KEY. 
