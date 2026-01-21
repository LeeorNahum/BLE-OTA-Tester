/**
 * @file ble.h
 * @brief BLE management module for OTA Tester
 * 
 * Modular BLE architecture - keeps main.cpp clean.
 */

#ifndef BLE_H
#define BLE_H

#include <NimBLEDevice.h>
#include "ble_ota/ble_ota.h"

/**
 * @brief Initialize and start BLE
 * @param deviceName Name for BLE advertising
 */
void bleStart(const char* deviceName);

/**
 * @brief Check if a device is connected
 * @return true if at least one device connected
 */
bool bleIsDeviceConnected();

/**
 * @brief Get the BLE server instance
 * @return Pointer to NimBLE server
 */
NimBLEServer* bleGetServer();

#endif // BLE_H