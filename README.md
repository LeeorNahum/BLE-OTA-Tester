# BLE-OTA-Tester

Hardware tester for the [FastBLEOTA](https://github.com/LeeorNahum/FastBLEOTA) library using the ESP32-S3.

## Features

- **Visual OTA Verification**: LED color changes each build based on compile timestamp
- **Modular Architecture**: Clean separation of BLE and OTA code
- **One-Click Upload**: Batch script for easy OTA uploads

## Hardware

- [Unexpected Maker NanoS3](https://unexpectedmaker.com/shop/nanos3) (ESP32-S3 with RGB LED)

## LED Behavior

| State | LED Color |
| ----- | --------- |
| Idle | Breathing animation in build-unique color |
| Connected | Solid blue |
| OTA Progress | Yellow blink (slow → fast as progress increases) |
| OTA Complete | Green flash → reboot |
| OTA Error | Red pulse |

The idle color is generated from the build timestamp - each compile produces a different color, making it easy to verify that OTA actually changed the firmware.

## Quick Start

### 1. Clone & Setup

```bash
git clone https://github.com/LeeorNahum/BLE-OTA-Tester.git
cd BLE-OTA-Tester
```

### 2. Configure Device Address

Copy the secrets template and set your device's MAC address:

```bash
cp secrets.bat.example secrets.bat
# Edit secrets.bat and set DEVICE_ADDRESS
```

Find your device's address by scanning:

```bash
python path/to/FastBLEOTA/BLE_OTA.py --scan
```

### 3. Build & Flash (First Time)

```bash
pio run -t upload
pio device monitor
```

Note the LED color and `Build Timestamp` in the serial output.

### 4. OTA Upload (Subsequent Updates)

After making changes, build and upload via OTA:

```bash
pio run
ota_upload.bat
```

The LED color and serial timestamp will change after successful OTA.

## Project Structure

```text
BLE-OTA-Tester/
├── src/
│   ├── main.cpp              # Main application with LED control
│   └── ble/
│       ├── ble.h/cpp         # BLE management
│       └── ble_ota/
│           └── ble_ota.h/cpp # OTA service wrapper
├── platformio.ini            # Build configuration
├── ota_upload.bat            # OTA upload script
├── secrets.bat.example       # Template for device address
└── secrets.bat               # Your device address (gitignored)
```

## Dependencies

- [FastBLEOTA](https://github.com/LeeorNahum/FastBLEOTA) - BLE OTA library
- [UMS3 Helper](https://github.com/UnexpectedMaker/esp32s3-arduino-helper) - NanoS3 RGB LED control
- [NimBLE-Arduino](https://github.com/h2zero/NimBLE-Arduino) - BLE stack (via FastBLEOTA)

## License

Copyright (c) 2024-2026 Leeor Nahum
