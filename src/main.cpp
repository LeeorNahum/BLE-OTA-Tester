#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEServer.h>
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
        //ums3.setPixelColor(UMS3::color(0, 255, 0));
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
            //Serial.printf("Received %d/%d bytes\n", receivedSize, expectedSize);

            if (receivedSize == expectedSize) {
                newDataAvailable = true;
            }
        }

        //ums3.setPixelColor(UMS3::color(0, 0, 255));
        Serial.println(micros()-hi);
    }
};

void setup() {
    Serial.begin(115200);

    BLEDevice::init("ESP32_BLE_OTA");
    uint16_t mtu = (1024 * 4);
    BLEDevice::setMTU(mtu);

    BLEServer *pServer = BLEDevice::createServer();
    pServer->setCallbacks(new MyServerCallbacks());
    
    BLESecurity *pSecurity = new BLESecurity();
    pSecurity->setAuthenticationMode(ESP_LE_AUTH_BOND); // ESP_LE_AUTH_REQ_SC_MITM_BOND

    BLEService *pService = pServer->createService(BLEUUID("4e8cbb5e-bc0f-4aab-a6e8-55e662418bef"));

    pCharacteristic = pService->createCharacteristic(
        BLEUUID("513fcda9-f46d-4e41-ac4f-42b768495a85"),
        BLECharacteristic::PROPERTY_WRITE | BLECharacteristic::PROPERTY_READ
    );

    pCharacteristic->setCallbacks(new MyCallbacks());
    pService->start();

    BLEAdvertising *pAdvertising = pServer->getAdvertising();
    pAdvertising->addServiceUUID(pService->getUUID());
    pAdvertising->setScanResponse(true);
    pAdvertising->setMinPreferred(0x0C);  // Minimum preferred connection interval (15ms - 0x0C)
    pAdvertising->setMaxPreferred(0x18);  // Maximum preferred connection interval (30ms - 0x18)
    pAdvertising->setAppearance(ESP_BLE_APPEARANCE_HID_MOUSE);
    pAdvertising->start();

    ums3.begin();
    ums3.setPixelPower(true);
    ums3.setPixelBrightness(255 / 3);
    ums3.setPixelColor(UMS3::color(0, 0, 255));
    
    setCpuFrequencyMhz(240); // 80 160 240
}

void loop() {
    if (newDataAvailable) {
        newDataAvailable = false;

        if (receivedSize == expectedSize && expectedSize > 0) {
            Serial.println("All firmware data received, finalizing update...");

            if (Update.end(true)) { // True to set the flag for reboot
                Serial.println("Update complete. Rebooting...");
                ums3.setPixelColor(UMS3::color(255, 0, 0));
                ESP.restart();
            } else {
                Serial.println("Failed to end update");
                // You can handle the error here, like reverting or retrying
            }
        }
    }

    //if (!deviceConnected) {
    //    ums3.setPixelColor(UMS3::color(0, 0, random(0, 255)));
    //}
}
