#include <M5Cardputer.h>
#include <esp_ota_ops.h>
#include <esp_system.h>
#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLE2902.h>
#include <vector>
#include <SD.h>
#include <SPI.h>

#include <M5CardputerOS_core.h>
#include <settings_manager.h>
#include <sd_card.h> 

// --- BLE Service and Characteristic UUIDs (from NRF Connect log) ---
#define SERVICE_UUID        "0000fee9-0000-1000-8000-00805f9b34fb"
#define CHARACTERISTIC_UUID "d44bc439-abfd-45a2-b575-925416129600"

// --- Global Variables ---
float appTextSize = 1.0f;
bool deviceConnected = false;
std::vector<String> receivedCommands;
const int MAX_COMMANDS_DISPLAY = 6; 

BLEServer *pServer = NULL;
BLECharacteristic *pCharacteristic = NULL;


// --- Function Prototypes ---
void drawUI();

// --- BLE Server Callbacks ---
class MyServerCallbacks: public BLEServerCallbacks {
    void onConnect(BLEServer* pServer) {
      deviceConnected = true;
      drawUI();
    }

    void onDisconnect(BLEServer* pServer) {
      deviceConnected = false;
      drawUI();
      // Restart advertising after disconnect
      pServer->getAdvertising()->start();
    }
};

// --- BLE Characteristic Callbacks ---
class MyCharacteristicCallbacks: public BLECharacteristicCallbacks {
    void onWrite(BLECharacteristic *pCharacteristic) {
        std::string value = pCharacteristic->getValue();
        String commandStr = "";

        for (int i = 0; i < value.length(); i++) {
            char hex[4];
            sprintf(hex, "%02X ", (unsigned char)value[i]);
            commandStr += hex;
        }

        receivedCommands.insert(receivedCommands.begin(), commandStr);
        if (receivedCommands.size() > MAX_COMMANDS_DISPLAY) {
            receivedCommands.pop_back();
        }

        if (isSdCardMounted) {
            File logFile = SD.open("/ble_log.txt", FILE_APPEND);
            if (logFile) {
                logFile.println(commandStr);
                logFile.close();
            }
        }
        
        drawUI();
    }
};

// --- UI Drawing Function ---
void drawUI() {
    M5Cardputer.Display.fillScreen(BLACK);
    M5Cardputer.Display.setTextDatum(top_left);

    M5Cardputer.Display.drawString("BLE Command Sniffer", 10, 5);
    
    M5Cardputer.Display.setTextDatum(top_right);
    M5Cardputer.Display.drawString(isSdCardMounted ? "SD: OK" : "SD: FAIL", M5Cardputer.Display.width() - 10, 5);
    M5Cardputer.Display.setTextDatum(top_left);


    String statusText = deviceConnected ? "App Connected" : "Waiting for App...";
    M5Cardputer.Display.setTextColor(deviceConnected ? GREEN : YELLOW);
    M5Cardputer.Display.drawString("Status: " + statusText, 10, 25);
    M5Cardputer.Display.setTextColor(WHITE);

    M5Cardputer.Display.drawFastHLine(0, 45, M5Cardputer.Display.width(), DARKGREY);
    M5Cardputer.Display.drawString("Received Commands:", 10, 50);

    int y_pos = 70;
    int lineHeight = 15;
    for (const auto& cmd : receivedCommands) {
        M5Cardputer.Display.drawString(cmd, 10, y_pos);
        y_pos += lineHeight;
    }
}

void setup() {
    M5Cardputer.begin();
    M5Cardputer.Display.setRotation(1);

    // --- Load Global Font Settings ---
    settings_init();
    appTextSize = (float)settings_get_font_size() / 10.0f;
    String savedFontName = settings_get_font_name();
    for (int i = 0; i < numAvailableFonts; ++i) {
        if (savedFontName == availableFonts[i].name) {
            currentFontSelection = i;
            break;
        }
    }
    M5Cardputer.Display.setFont(availableFonts[currentFontSelection].font);
    M5Cardputer.Display.setTextSize(appTextSize);

    mountSD();

    drawUI();

    // --- Create the BLE Device (Spoofing the Bed) ---
    BLEDevice::init("QRRM100514"); // Use the exact name from your log
    pServer = BLEDevice::createServer();
    pServer->setCallbacks(new MyServerCallbacks());
    BLEService *pService = pServer->createService(SERVICE_UUID);

    pCharacteristic = pService->createCharacteristic(
                        CHARACTERISTIC_UUID,
                        BLECharacteristic::PROPERTY_WRITE | BLECharacteristic::PROPERTY_WRITE_NR
                      );

    // Set our callback to catch the commands
    pCharacteristic->setCallbacks(new MyCharacteristicCallbacks());

    pService->start();
    pServer->getAdvertising()->start();
}

void loop() {
    M5Cardputer.update();
    
    // Exit to launcher
    if (M5Cardputer.Keyboard.isChange() && M5Cardputer.Keyboard.isPressed()) {
        Keyboard_Class::KeysState status = M5Cardputer.Keyboard.keysState();
        if (status.fn && !status.word.empty() && status.word[0] == '`') {
            const esp_partition_t *launcher_partition = esp_partition_find_first(ESP_PARTITION_TYPE_APP, ESP_PARTITION_SUBTYPE_APP_OTA_0, NULL);
            if (launcher_partition != NULL) {
                esp_ota_set_boot_partition(launcher_partition);
                esp_restart();
            }
        }
    }
}

