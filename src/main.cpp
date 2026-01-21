/**
 * @file main.cpp
 * @brief BLE OTA Tester for Unexpected Maker NanoS3
 * 
 * Hardware tester for FastBLEOTA library using modular architecture.
 * 
 * LED Color Behavior:
 * - Startup:   Unique color based on build timestamp (changes each compile!)
 * - Idle:      Fast breathing animation (25%-100%) in build color
 * - Connected: Solid blue
 * - OTA Start: Yellow blink (slow)
 * - OTA Progress: Yellow blink (speeds up as progress increases)
 * - OTA Complete: Green flash before restart
 * - OTA Error: Red pulse
 * 
 * Serial Output:
 * - Prints BUILD_TIMESTAMP on startup for verification
 * - Logs all BLE and OTA events
 * 
 * Copyright (c) 2024-2026 Leeor Nahum
 */

#include <Arduino.h>
#include <UMS3.h>
#include "ble/ble.h"

// Build identification - set via platformio.ini build_flags
// These change each compile, making it easy to verify OTA worked
#ifndef BUILD_TIMESTAMP
  #define BUILD_TIMESTAMP 0
#endif

#ifndef BUILD_MESSAGE
  #define BUILD_MESSAGE "Build: " __DATE__ " " __TIME__
#endif

// Device name for BLE advertising
#define DEVICE_NAME "BLE-OTA-Tester"

// LED brightness (0-255)
#define LED_BRIGHTNESS 100

// UMS3 instance for NeoPixel LED
UMS3 ums3;

// Current state
enum TesterState {
  STATE_IDLE,
  STATE_CONNECTED,
  STATE_OTA_ACTIVE,
  STATE_OTA_ERROR
};

static TesterState currentState = STATE_IDLE;
static uint32_t buildColor = 0;
static float otaProgress = 0;

// -----------------------------------------------------------------------------
// Build-specific color generation
// -----------------------------------------------------------------------------

/**
 * @brief Generate a unique LED color from build timestamp
 * 
 * Uses simple hash to create a visually distinct color each build.
 * The color is consistent for the same timestamp, so you can
 * remember "this build was cyan" etc.
 */
uint32_t generateBuildColor(uint32_t timestamp) {
  // Simple hash to spread values across color space
  uint32_t hash = timestamp;
  hash ^= (hash >> 16);
  hash *= 0x85ebca6b;
  hash ^= (hash >> 13);
  hash *= 0xc2b2ae35;
  hash ^= (hash >> 16);
  
  // Extract RGB (using HSV->RGB for more vibrant colors)
  // Hue: 0-360 from hash
  // Saturation: Always high (200-255)
  // Value: Always high (200-255)
  uint16_t hue = hash % 360;
  uint8_t sat = 200 + (hash >> 8) % 56;  // 200-255
  uint8_t val = 200 + (hash >> 16) % 56; // 200-255
  
  // HSV to RGB conversion
  uint8_t r, g, b;
  uint8_t region = hue / 60;
  uint8_t remainder = (hue - (region * 60)) * 255 / 60;
  
  uint8_t p = (val * (255 - sat)) / 255;
  uint8_t q = (val * (255 - ((sat * remainder) / 255))) / 255;
  uint8_t t = (val * (255 - ((sat * (255 - remainder)) / 255))) / 255;
  
  switch (region) {
    case 0:  r = val; g = t;   b = p;   break;
    case 1:  r = q;   g = val; b = p;   break;
    case 2:  r = p;   g = val; b = t;   break;
    case 3:  r = p;   g = q;   b = val; break;
    case 4:  r = t;   g = p;   b = val; break;
    default: r = val; g = p;   b = q;   break;
  }
  
  return UMS3::color(r, g, b);
}

// -----------------------------------------------------------------------------
// LED Control
// -----------------------------------------------------------------------------

// Scale a color component by LED_BRIGHTNESS
inline uint8_t scaleBrightness(uint8_t value) {
  return (uint8_t)((value * LED_BRIGHTNESS) / 255);
}

void setLED(uint32_t color) {
  ums3.setPixelColor(color);
}

void updateLED() {
  unsigned long now = millis();
  
  switch (currentState) {
    case STATE_IDLE: {
      // Breathing: 25%-100% intensity, faster cycle (~1.5s full cycle)
      float breath = 0.25f + 0.75f * ((sin(now / 250.0f) + 1.0f) / 2.0f);
      uint8_t r = scaleBrightness(((buildColor >> 16) & 0xFF) * breath);
      uint8_t g = scaleBrightness(((buildColor >> 8) & 0xFF) * breath);
      uint8_t b = scaleBrightness((buildColor & 0xFF) * breath);
      setLED(UMS3::color(r, g, b));
      break;
    }
    
    case STATE_CONNECTED: {
      // Solid blue when connected
      uint8_t blue = scaleBrightness(255);
      setLED(UMS3::color(0, 0, blue));
      break;
    }
    
    case STATE_OTA_ACTIVE: {
      // Yellow with blink rate increasing as progress increases
      // At 0%: blink period ~800ms (slow), at 100%: period ~50ms (fast)
      float blinkPeriod = 800.0f - (otaProgress * 7.5f);  // 800ms -> 50ms
      bool ledOn = ((now % (int)blinkPeriod) < (blinkPeriod / 2));
      
      uint8_t intensity = ledOn ? scaleBrightness(255) : 0;
      setLED(UMS3::color(intensity, intensity, 0));
      break;
    }
    
    case STATE_OTA_ERROR: {
      // Red pulse
      bool ledOn = ((now / 250) % 2 == 0);
      uint8_t high = scaleBrightness(255);
      uint8_t low = scaleBrightness(80);
      setLED(UMS3::color(ledOn ? high : low, 0, 0));
      break;
    }
  }
}

// -----------------------------------------------------------------------------
// Callbacks from BLE module
// -----------------------------------------------------------------------------

void onBLEConnectionChanged(bool connected) {
  if (currentState != STATE_OTA_ACTIVE && currentState != STATE_OTA_ERROR) {
    currentState = connected ? STATE_CONNECTED : STATE_IDLE;
    Serial.printf("[LED] State: %s\n", connected ? "CONNECTED" : "IDLE");
  }
}

void onOTAStateChanged(const char* state, uint32_t color) {
  Serial.printf("[LED] OTA State: %s\n", state);
  
  if (strcmp(state, "started") == 0) {
    currentState = STATE_OTA_ACTIVE;
    otaProgress = 0;
  } else if (strcmp(state, "complete") == 0) {
    // Solid green (scaled to LED_BRIGHTNESS)
    setLED(UMS3::color(0, scaleBrightness(255), 0));
    // Device will restart after this
  } else if (strcmp(state, "error") == 0 || strcmp(state, "aborted") == 0) {
    currentState = STATE_OTA_ERROR;
  }
}

void onOTAProgressUpdate(float percent) {
  otaProgress = percent;
}

// -----------------------------------------------------------------------------
// Setup & Loop
// -----------------------------------------------------------------------------

void setup() {
  Serial.begin(115200);
  delay(1000);  // Allow USB CDC to initialize
  
  // Initialize UMS3 and LED
  ums3.begin();
  ums3.setPixelPower(true);
  ums3.setPixelBrightness(LED_BRIGHTNESS);
  
  // Generate unique color from build timestamp
  buildColor = generateBuildColor(BUILD_TIMESTAMP);
  
  // Print build info (this is the key verification!)
  Serial.println();
  Serial.println("============================================");
  Serial.println("    BLE OTA Tester - FastBLEOTA v3.0.0");
  Serial.println("============================================");
  Serial.printf("Build Timestamp: %lu\n", (unsigned long)BUILD_TIMESTAMP);
  Serial.printf("Build Message:   %s\n", BUILD_MESSAGE);
  Serial.printf("Build Color:     #%06X\n", buildColor);
  Serial.println("============================================");
  Serial.println();
  Serial.println("LED will show a unique color per build.");
  Serial.println("After OTA, the color and timestamp change!");
  Serial.println();
  
  // Show build color immediately
  setLED(buildColor);
  delay(1000);
  
  // Initialize BLE
  bleStart(DEVICE_NAME);
  
  Serial.println();
  Serial.println("Ready! Use BLE_OTA.py to upload new firmware.");
  Serial.println("Watch the LED color and serial timestamp to verify OTA.");
  Serial.println();
}

void loop() {
  updateLED();
  delay(16);  // ~60 FPS for smooth LED animations
}