/**
 * @file ble.cpp
 * @brief BLE management implementation for OTA Tester
 */

#include "ble.h"

// Forward declaration of LED callback (defined in main.cpp)
extern void onBLEConnectionChanged(bool connected);

// BLE server instance
static NimBLEServer* ble_server = nullptr;

// Server callbacks for connection tracking (NimBLE 2.x signatures)
class ServerCallbacks : public NimBLEServerCallbacks {
public:
  void onConnect(NimBLEServer* pServer, NimBLEConnInfo& connInfo) override {
    Serial.println("[BLE] Client connected");
    onBLEConnectionChanged(true);
    
    // Continue advertising for additional connections
    NimBLEDevice::getAdvertising()->start();
  }
  
  void onDisconnect(NimBLEServer* pServer, NimBLEConnInfo& connInfo, int reason) override {
    Serial.printf("[BLE] Client disconnected (reason: %d)\n", reason);
    
    if (pServer->getConnectedCount() == 0) {
      onBLEConnectionChanged(false);
    }
    
    // Resume advertising
    NimBLEDevice::getAdvertising()->start();
  }
};

void bleStart(const char* deviceName) {
  Serial.println("[BLE] Initializing...");
  
  // Initialize NimBLE
  NimBLEDevice::init(deviceName);
  NimBLEDevice::setMTU(256);  // Larger MTU for faster OTA
  
  // Create server
  ble_server = NimBLEDevice::createServer();
  ble_server->setCallbacks(new ServerCallbacks());
  
  // Start OTA service
  bleStartOTA();
  
  // Configure advertising
  NimBLEAdvertising* pAdvertising = NimBLEDevice::getAdvertising();
  pAdvertising->setName(deviceName);
  pAdvertising->addServiceUUID(bleGetOTAServiceUUID());
  pAdvertising->start();
  
  Serial.printf("[BLE] Started. Address: %s\n", 
                NimBLEDevice::getAddress().toString().c_str());
}

bool bleIsDeviceConnected() {
  return ble_server && ble_server->getConnectedCount() > 0;
}

NimBLEServer* bleGetServer() {
  return ble_server;
}