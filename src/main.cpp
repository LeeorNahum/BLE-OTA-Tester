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
        ums3.setPixelColor(UMS3::color(0, 255, 0));  // Turn LED green when connected
    };

    void onDisconnect(BLEServer* pServer) {
        Serial.println("Client disconnected, restarting advertising");
        deviceConnected = false;
        sizeReceived = false;
        receivedSize = 0;
        Update.abort();  // Abort any ongoing update
        pServer->startAdvertising();
        ums3.setPixelColor(UMS3::color(255, 0, 0));  // Turn LED red when disconnected
    }
};

class MyCallbacks : public BLECharacteristicCallbacks {
    void onWrite(BLECharacteristic *pCharacteristic) {
        std::string value = pCharacteristic->getValue();

        if (!sizeReceived) {
            if (value.length() != sizeof(uint32_t)) {
                Serial.println("Received size data of incorrect length");
                return;
            }
            expectedSize = *((uint32_t*)value.c_str());
            sizeReceived = true;
            Serial.printf("Expected firmware size: %d bytes\n", expectedSize);

            // Start the update process
            if (!Update.begin(expectedSize)) {
                Serial.println("Failed to start update");
                Update.printError(Serial);
                return;
            }
        } else {
            // Write directly to flash
            size_t chunkSize = value.length();
            if (Update.write((uint8_t*)value.c_str(), chunkSize) != chunkSize) {
                Serial.println("Failed to write firmware chunk");
                Update.printError(Serial);
                return;
            }

            receivedSize += chunkSize;

            if (receivedSize > expectedSize) {
                Serial.println("Received more data than expected");
                Update.end();
                return;
            }

            if (receivedSize == expectedSize) {
                newDataAvailable = true;
            }
        }
    }
};

void setup() {
    Serial.begin(115200);

    BLEDevice::init("ESP32_BLE_OTA");

    BLEDevice::setMTU(517);  // Set MTU size to maximum

    BLEDevice::setPower(ESP_PWR_LVL_P9);  // Max BLE power level

    BLEServer *pServer = BLEDevice::createServer();
    pServer->setCallbacks(new MyServerCallbacks());

    BLESecurity *pSecurity = new BLESecurity();
    pSecurity->setAuthenticationMode(ESP_LE_AUTH_BOND);

    BLEService *pService = pServer->createService(BLEUUID("4e8cbb5e-bc0f-4aab-a6e8-55e662418bef"));

    pCharacteristic = pService->createCharacteristic(
        BLEUUID("513fcda9-f46d-4e41-ac4f-42b768495a85"),
        BLECharacteristic::PROPERTY_READ | BLECharacteristic::PROPERTY_WRITE | BLECharacteristic::PROPERTY_WRITE_NR
    );

    pCharacteristic->setCallbacks(new MyCallbacks());
    pService->start();

    BLEAdvertising *pAdvertising = pServer->getAdvertising();
    pAdvertising->addServiceUUID(pService->getUUID());
    pAdvertising->setScanResponse(true);
    pAdvertising->setMinPreferred(0x06);
    pAdvertising->setMaxPreferred(0x12);
    pAdvertising->setAppearance(ESP_BLE_APPEARANCE_GENERIC_TAG);
    pAdvertising->start();

    ums3.begin();
    ums3.setPixelPower(true);
    ums3.setPixelBrightness(85);  // Set brightness to a third
    ums3.setPixelColor(UMS3::color(0, 0, 255));  // Initial color

    setCpuFrequencyMhz(240);  // Max CPU frequency
}

void loop() {
    if (newDataAvailable) {
        newDataAvailable = false;

        if (receivedSize == expectedSize && expectedSize > 0) {
            Serial.println("All firmware data received, finalizing update...");

            if (Update.end(true)) {  // True to reboot
                Serial.println("Update complete. Rebooting...");
                ums3.setPixelColor(UMS3::color(255, 0, 0));  // Indicate completion
                delay(1000);
                ESP.restart();
            } else {
                Serial.println("Failed to finalize update");
                Update.printError(Serial);
            }
        }
    }

    if (!deviceConnected) {
        ums3.setPixelColor(UMS3::color(0, 0, (millis() / 10) % 255));  // Animate LED
    }

    delay(100);
}
