/**
 * @file ble_ota.cpp
 * @brief OTA service wrapper implementation
 */

#include "ble_ota.h"
#include "../ble.h"

// Forward declarations of LED callbacks (defined in main.cpp)
extern void onOTAStateChanged(const char* state, uint32_t color);
extern void onOTAProgressUpdate(float percent);

// OTA callbacks with LED feedback
class OTACallbacks : public FastBLEOTACallbacks {
public:
  void onStart(size_t expectedSize, uint32_t expectedCRC) override {
    Serial.println("[OTA] Update started");
    Serial.printf("[OTA]   Size: %u bytes\n", expectedSize);
    Serial.printf("[OTA]   CRC: 0x%08X\n", expectedCRC);
    onOTAStateChanged("started", 0xFFFF00);  // Yellow
  }
  
  void onProgress(size_t bytesReceived, size_t bytesExpected, float percent) override {
    static int lastTen = -1;
    int currentTen = (int)percent / 10;
    if (currentTen != lastTen) {
      lastTen = currentTen;
      Serial.printf("[OTA] Progress: %.0f%% (%u/%u)\n", percent, bytesReceived, bytesExpected);
    }
    onOTAProgressUpdate(percent);
  }
  
  void onComplete() override {
    Serial.println("[OTA] Update complete! Restarting...");
    onOTAStateChanged("complete", 0x00FF00);  // Green
    delay(500);  // Brief pause to show green LED
  }
  
  void onError(fbo_error_t error, const char* errorString) override {
    Serial.printf("[OTA] Error: %s\n", errorString);
    onOTAStateChanged("error", 0xFF0000);  // Red
  }
  
  void onAbort() override {
    Serial.println("[OTA] Update aborted");
    onOTAStateChanged("aborted", 0xFF0000);  // Red
  }
};

void bleStartOTA() {
  FastBLEOTA.setCallbacks(new OTACallbacks());
  FastBLEOTA.begin(bleGetServer());
  
  Serial.printf("[OTA] Service started (UUID: %s)\n", FastBLEOTA.getServiceUUID().toString().c_str());
}

const NimBLEUUID& bleGetOTAServiceUUID() {
  return FastBLEOTA.getServiceUUID();
}