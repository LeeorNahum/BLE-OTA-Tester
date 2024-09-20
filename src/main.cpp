#include <BLEDevice.h>
#include <Update.h>
#include <UMS3.h>

BLECharacteristic *pCharacteristic;
bool newDataAvailable = false;
size_t expectedSize = 0;  // The expected size of the firmware
size_t receivedSize = 0;  // The size of data received so far
bool sizeReceived = false; // Flag to check if the size metadata is received

// Create an instance of the UMS3
UMS3 ums3;

bool deviceConnected = false;

// Callback class for BLE server events
class MyServerCallbacks: public BLEServerCallbacks {
    void onConnect(BLEServer* pServer) {
        Serial.println("Client connected");
        deviceConnected = true;
    };

    void onDisconnect(BLEServer* pServer) {
        Serial.println("Client disconnected, restarting advertising");
        deviceConnected = false;
        sizeReceived = false;
        pServer->startAdvertising();
    }
};

class MyCallbacks : public BLECharacteristicCallbacks {
    void onWrite(BLECharacteristic *pCharacteristic) {
        unsigned long long hi = micros();
        // ums3.setPixelColor(UMS3::color(0, 255, 0));  // LED color indicator (commented out for now)
        std::string value = pCharacteristic->getValue();

        if (!sizeReceived) {
            expectedSize = *((uint32_t*)value.c_str());
            sizeReceived = true;
            Serial.printf("Expected firmware size: %d bytes\n", expectedSize);

            // Start the update process
            if (!Update.begin(expectedSize)) {
                Serial.println("Failed to start update");
                return;
            }
        } else {
            // Write directly to flash
            size_t chunkSize = value.length();
            if (Update.write((uint8_t*)value.c_str(), chunkSize) != chunkSize) {
                Serial.println("Failed to write firmware chunk");
                return;
            }

            receivedSize += chunkSize;
            // Serial.printf("Received %d/%d bytes\n", receivedSize, expectedSize);

            if (receivedSize == expectedSize) {
                newDataAvailable = true;
            }
        }

        // ums3.setPixelColor(UMS3::color(0, 0, 255));  // LED color indicator (commented out for now)
        /////////        Serial.println(micros()-hi);
    }
};

void setup() {
    Serial.begin(115200);

    BLEDevice::init("ESP32_BLE_OTA");

    // Increase MTU dynamically for faster transmission
    // uint16_t mtu = (1024 * 4);  // Old MTU (commented out)
    BLEDevice::setMTU(517);  // Increased MTU size (dynamically adjustable)
    
    BLEDevice::setPower(ESP_PWR_LVL_P9);  // BLE power level

    BLEServer *pServer = BLEDevice::createServer();
    pServer->setCallbacks(new MyServerCallbacks());
    
    BLESecurity *pSecurity = new BLESecurity();
    pSecurity->setAuthenticationMode(ESP_LE_AUTH_BOND);  // ESP_LE_AUTH_REQ_SC_MITM_BOND

    BLEService *pService = pServer->createService(BLEUUID("4e8cbb5e-bc0f-4aab-a6e8-55e662418bef"));

    pCharacteristic = pService->createCharacteristic(
        BLEUUID("513fcda9-f46d-4e41-ac4f-42b768495a85"),
        BLECharacteristic::PROPERTY_READ | BLECharacteristic::PROPERTY_WRITE | BLECharacteristic::PROPERTY_WRITE_NR  // Added write without response
    );

    pCharacteristic->setCallbacks(new MyCallbacks());
    pService->start();

    BLEAdvertising *pAdvertising = pServer->getAdvertising();
    pAdvertising->addServiceUUID(pService->getUUID());
    pAdvertising->setScanResponse(true);
    pAdvertising->setMinPreferred(0x00);  // Minimum preferred connection interval (0x06)
    pAdvertising->setMaxPreferred(0x10);  // Maximum preferred connection interval (0x12)
    pAdvertising->setAppearance(ESP_BLE_APPEARANCE_HID_MOUSE);
    pAdvertising->start();

    ums3.begin();
    ums3.setPixelPower(true);
    ums3.setPixelBrightness(255 / 3);
    ums3.setPixelColor(UMS3::color(0, 0, 255));
    
    setCpuFrequencyMhz(240);  // Set CPU frequency to 240 MHz for faster processing
}

void loop() {
    if (newDataAvailable) {
        newDataAvailable = false;

        if (receivedSize == expectedSize && expectedSize > 0) {
            Serial.println("All firmware data received, finalizing update...");

            if (Update.end(true)) {  // True to set the flag for reboot
                Serial.println("Update complete. Rebooting...");
                ums3.setPixelColor(UMS3::color(255, 0, 0));
                ESP.restart();
            } else {
                Serial.println("Failed to end update");
            }
        }
    }

    // if (!deviceConnected) {
    //     ums3.setPixelColor(UMS3::color(0, 0, random(0, 255)));  // LED color animation if not connected (commented out for now)
    // }
}
