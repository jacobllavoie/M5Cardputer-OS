#include <M5Cardputer.h>
#include <esp_ota_ops.h>
#include <esp_system.h>
#include <M5CardputerOS_core.h>
#include <settings_manager.h>
#include <BLEDevice.h>
#include <vector>
#include <map>
#include "secrets.h" // Include the new secrets file

// --- BLE Device Configuration ---
static BLEUUID serviceUUID("0000fee9-0000-1000-8000-00805f9b34fb");
static BLEUUID charUUID("d44bc439-abfd-45a2-b575-925416129600");

// --- BLE State Variables ---
static boolean doConnect = false;
static boolean connected = false;
static boolean doScan = false;
static BLERemoteCharacteristic* pRemoteCharacteristic;
static BLEClient* pClient;
static BLEAdvertisedDevice* myDevice;
static BLEAddress* pServerAddress = nullptr;

// --- Bed Commands (from richmat.py) ---
const uint8_t CMD_STOP[] =      {0x01, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
const uint8_t CMD_HEAD_UP[] =   {0x01, 0x01, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00};
const uint8_t CMD_HEAD_DOWN[] = {0x01, 0x01, 0x02, 0x00, 0x00, 0x00, 0x00, 0x00};
const uint8_t CMD_FEET_UP[] =   {0x01, 0x01, 0x04, 0x00, 0x00, 0x00, 0x00, 0x00};
const uint8_t CMD_FEET_DOWN[] = {0x01, 0x01, 0x08, 0x00, 0x00, 0x00, 0x00, 0x00};
const uint8_t CMD_FLAT[] =      {0x01, 0x01, 0x20, 0x00, 0x00, 0x00, 0x00, 0x00};

// Map to associate names with commands
std::vector<std::pair<String, const uint8_t*>> commands = {
    {"Head Up", CMD_HEAD_UP},
    {"Head Down", CMD_HEAD_DOWN},
    {"Feet Up", CMD_FEET_UP},
    {"Feet Down", CMD_FEET_DOWN},
    {"Go Flat", CMD_FLAT}
};

// --- UI State ---
int selectedCommandIndex = 0;
int scrollOffset = 0;
const int VISIBLE_ITEMS = 5;
String statusMessage = "Initializing...";

// --- BLE Callbacks ---
class MyClientCallback : public BLEClientCallbacks {
    void onConnect(BLEClient* pclient) {
        connected = true;
        statusMessage = "Connected!";
    }
    void onDisconnect(BLEClient* pclient) {
        connected = false;
        statusMessage = "Disconnected";
        #ifdef BED_MAC_ADDRESS
        doConnect = true; // Reconnect to the saved MAC
        #else
        doScan = true; // Scan again if disconnected
        #endif
    }
};

// Callback for BLE Scan results
class MyAdvertisedDeviceCallbacks: public BLEAdvertisedDeviceCallbacks {
    void onResult(BLEAdvertisedDevice advertisedDevice) {
        // We have found a device, see if it contains the serviceUUID
        if (advertisedDevice.haveServiceUUID() && advertisedDevice.isAdvertisingService(serviceUUID)) {
            BLEDevice::getScan()->stop();
            myDevice = new BLEAdvertisedDevice(advertisedDevice);
            doConnect = true;
            doScan = false;
        }
    }
};

// --- Function to Connect to the Bed ---
bool connectToServer() {
    statusMessage = "Connecting...";
    pClient = BLEDevice::createClient();
    pClient->setClientCallbacks(new MyClientCallback());
    
    bool success = false;
    if (pServerAddress != nullptr) {
        success = pClient->connect(*pServerAddress);
    } else if (myDevice != nullptr) {
        success = pClient->connect(myDevice);
    }

    if (!success) {
        return false;
    }

    BLERemoteService* pRemoteService = pClient->getService(serviceUUID);
    if (pRemoteService == nullptr) {
        statusMessage = "Service not found";
        return false;
    }

    pRemoteCharacteristic = pRemoteService->getCharacteristic(charUUID);
    if (pRemoteCharacteristic == nullptr) {
        statusMessage = "Characteristic not found";
        return false;
    }
    return true;
}

// --- Function to Send a Command ---
void sendCommand(const uint8_t* command) {
    if (connected && pRemoteCharacteristic->canWrite()) {
        pRemoteCharacteristic->writeValue((uint8_t*)command, 8, false);
    }
}

// --- UI Drawing ---
void drawUI() {
    M5Cardputer.Display.fillScreen(BLACK);
    M5Cardputer.Display.setTextDatum(top_left);

    // Title and Status
    M5Cardputer.Display.drawString("Smart Bed Remote", 10, 5);
    M5Cardputer.Display.drawString("Status: " + statusMessage, 10, M5Cardputer.Display.height() - 30);
    M5Cardputer.Display.drawFastHLine(0, 20, M5Cardputer.Display.width(), DARKGREY);

    if (connected) {
        // Draw command list only when connected
        int lineHeight = 18;
        for (int i = scrollOffset; i < (scrollOffset + VISIBLE_ITEMS) && i < commands.size(); i++) {
            int y_pos = 25 + (i - scrollOffset) * lineHeight;
            if (i == selectedCommandIndex) {
                M5Cardputer.Display.setTextColor(BLACK, WHITE);
            } else {
                M5Cardputer.Display.setTextColor(WHITE, BLACK);
            }
            M5Cardputer.Display.drawString(commands[i].first, 10, y_pos);
        }
    }
    
    M5Cardputer.Display.setTextColor(WHITE, BLACK);
    M5Cardputer.Display.setTextDatum(bottom_center);
    if(connected) {
      M5Cardputer.Display.drawString("Hold Enter to Activate", M5Cardputer.Display.width()/2, M5Cardputer.Display.height() - 5);
    } else {
      M5Cardputer.Display.drawString(statusMessage, M5Cardputer.Display.width()/2, M5Cardputer.Display.height() - 5);
    }
    M5Cardputer.Display.setTextDatum(top_left);
}

// --- Main Setup and Loop ---
void setup() {
    M5Cardputer.begin();
    M5Cardputer.Display.setRotation(1);

    // Load global font settings
    settings_init();
    float textSize = (float)settings_get_font_size() / 10.0f;
    String fontName = settings_get_font_name();
    for (int i = 0; i < numAvailableFonts; ++i) {
        if (fontName == availableFonts[i].name) {
            M5Cardputer.Display.setFont(availableFonts[i].font);
            break;
        }
    }
    M5Cardputer.Display.setTextSize(textSize);

    BLEDevice::init("");

    #ifdef BED_MAC_ADDRESS
    pServerAddress = new BLEAddress(BED_MAC_ADDRESS);
    doConnect = true;
    statusMessage = "Using saved MAC...";
    #else
    doScan = true;
    statusMessage = "Scanning...";
    #endif

    drawUI();
}

void loop() {
    M5Cardputer.update();

    if (doScan) {
        statusMessage = "Scanning...";
        drawUI();
        BLEScan* pBLEScan = BLEDevice::getScan();
        pBLEScan->setAdvertisedDeviceCallbacks(new MyAdvertisedDeviceCallbacks());
        pBLEScan->setActiveScan(true);
        pBLEScan->start(5, false); // Scan for 5 seconds
    }
    
    if (doConnect) {
        if (!connectToServer()) {
            #ifdef BED_MAC_ADDRESS
            statusMessage = "Connection failed. Retrying...";
            delay(2000);
            doConnect = true; // Retry connection
            #else
            statusMessage = "Connection failed. Rescanning...";
            doScan = true; // Scan again if connection fails
            #endif
        }
        doConnect = false;
        drawUI();
    }
    
    // --- Handle Keyboard Input ---
    if (M5Cardputer.Keyboard.isChange()) {
        Keyboard_Class::KeysState keys = M5Cardputer.Keyboard.keysState();

         if (keys.enter) { // Key is PRESSED
            if (connected) {
                sendCommand(commands[selectedCommandIndex].second);
                statusMessage = "Moving...";
            }
        } else { // Key is RELEASED
            if (connected) {
                sendCommand(CMD_STOP);
                statusMessage = "Connected";
            }
        }

        if (M5Cardputer.Keyboard.isKeyPressed(';')) { // Up
            selectedCommandIndex = (selectedCommandIndex - 1 + commands.size()) % commands.size();
        }
        if (M5Cardputer.Keyboard.isKeyPressed('.')) { // Down
            selectedCommandIndex = (selectedCommandIndex + 1) % commands.size();
        }
        
        // Adjust scroll offset
        if (selectedCommandIndex < scrollOffset) {
            scrollOffset = selectedCommandIndex;
        }
        if (selectedCommandIndex >= scrollOffset + VISIBLE_ITEMS) {
            scrollOffset = selectedCommandIndex - VISIBLE_ITEMS + 1;
        }

        if (keys.fn && !keys.word.empty() && keys.word[0] == '`') {
            const esp_partition_t *launcher_partition = esp_partition_find_first(ESP_PARTITION_TYPE_APP, ESP_PARTITION_SUBTYPE_APP_OTA_0, NULL);
            if (launcher_partition != NULL) {
                esp_ota_set_boot_partition(launcher_partition);
                esp_restart();
            }
        }
        drawUI();
    }
}

