#include <NimBLEDevice.h>
#include <FastBLEOTA.h>
#include <UMS3.h>
#include <SimpleDebug.h>
#include <esp_sleep.h>
#include "esp_err.h"
#include "esp_pm.h"

UMS3 ums3;

bool deviceConnected = false;

class ServerCallbacks : public NimBLEServerCallbacks {
  void onConnect(NimBLEServer* pServer) {
    Serial.println("Client connected, stopping advertising");
    NimBLEDevice::stopAdvertising();
    ums3.setPixelColor(UMS3::color(0, 0, 255));  // Turn LED blue when connected
  }

  void onDisconnect(NimBLEServer* pServer) {
    Serial.println("Client disconnected, starting advertising");
    NimBLEDevice::startAdvertising();
    ums3.setPixelColor(UMS3::color(255, 0, 0));  // Turn LED red when disconnected
  }
};

class OTACallbacks : public FastBLEOTACallbacks {
  void onOTAStart(size_t expectedSize) override {
    Serial.println("MyFastBLEOTA: OTA Update Started");
    ums3.setPixelColor(UMS3::color(255, 255, 0));  // Yellow
  }

  void onOTAProgress(size_t receivedSize, size_t expectedSize) override {
    float progress = (float)receivedSize / (float)expectedSize * 100.0;
    //Serial.printf("MyFastBLEOTA: OTA Progress: %.2f%%\n", progress);
    Serial.println(progress, 2);
    // Optionally update LED brightness or color
  }

  void onOTAComplete() override {
    Serial.println("MyFastBLEOTA: OTA Update Complete");
    ums3.setPixelColor(UMS3::color(0, 255, 0));  // Green
    delay(1000);
    ESP.restart();
  }

  void onOTAError(fastbleota_error_t errorCode) override {
    Serial.printf("MyFastBLEOTA: OTA Error: %d\n", errorCode);
    ums3.setPixelColor(UMS3::color(255, 0, 0));  // Red
  }
};

void setup() {
  Serial.begin(115200);

  setCpuFrequencyMhz(80);
  
  ums3.begin();
  ums3.setPixelPower(true);
  ums3.setPixelBrightness(85);
  ums3.setPixelColor(UMS3::color(0, 255, 255));

  NimBLEDevice::init("ESP32_BLE_OTA");
  NimBLEDevice::setMTU(512);
  NimBLEDevice::setPower(ESP_PWR_LVL_P9);  
  NimBLEDevice::setSecurityAuth(true, true, true);
  NimBLEDevice::createServer()->setCallbacks(new ServerCallbacks());

  FastBLEOTA::setCallbacks(new OTACallbacks());
  FastBLEOTA::begin(NimBLEDevice::getServer());
  
  NimBLEDevice::getAdvertising()->start();
  esp_pm_config_t pm_config = {
    .max_freq_mhz = 80,
    .min_freq_mhz = 40,
    .light_sleep_enable = false
  };
}

void loop() {
  if (!NimBLEDevice::getServer()->getConnectedCount()) {
    int brightness = (int)(sin(millis() / 500.0) * 128 + 128);
    ums3.setPixelColor(UMS3::color(0, brightness, 0));  // Fade in and out
  }
  delay(100);
}