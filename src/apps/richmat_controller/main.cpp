#include <M5Cardputer.h>
#include <esp_ota_ops.h>
#include <esp_system.h>
#include <BLEDevice.h>
#include <BLEUtils.h>
#include <vector>
#include <string>

#include <M5CardputerOS_core.h>
#include <settings_manager.h>

// Optional: Define your bed's MAC address in a separate secrets file
#include "secrets.h"

// --- BLE Service and Characteristic UUIDs ---
#define RICHMAT_SERVICE_UUID        "0000fee9-0000-1000-8000-00805f9b34fb"
#define RICHMAT_CHARACTERISTIC_UUID "d44bc439-abfd-45a2-b575-925416129600"

// --- App-Specific State Machine ---
enum ControllerState {
    STATE_SCANNING,
    STATE_CONNECTING,
    STATE_CONNECTED,
    STATE_DISCONNECTED
};
ControllerState currentAppState = STATE_SCANNING; // Use our local state

// --- Global State ---
float appTextSize = 1.0f;
bool isSending = false;
int currentCommand = 0;
int scrollOffset = 0;
const int VISIBLE_COMMANDS = 5;
unsigned long lastSendTime = 0;
const int SEND_INTERVAL = 150;

BLEAdvertisedDevice* myDevice = nullptr;
BLEClient* pClient = nullptr;
BLERemoteCharacteristic* pCharacteristic = nullptr;

// --- Bed Commands ---
const uint8_t CMD_STOP[]          = {0x6E, 0x01, 0x00, 0x6E, 0xDD};
const uint8_t CMD_HEAD_UP[]       = {0x6E, 0x01, 0x00, 0x24, 0x93};
const uint8_t CMD_HEAD_DOWN[]     = {0x6E, 0x01, 0x00, 0x25, 0x94};
// ... (rest of commands are the same)
const uint8_t CMD_FOOT_UP[]       = {0x6E, 0x01, 0x00, 0x26, 0x95};
const uint8_t CMD_FOOT_DOWN[]     = {0x6E, 0x01, 0x00, 0x27, 0x96};
const uint8_t CMD_BOTH_UP[]       = {0x6E, 0x01, 0x00, 0x29, 0x98};
const uint8_t CMD_BOTH_DOWN[]     = {0x6E, 0x01, 0x00, 0x2A, 0x99};
const uint8_t CMD_FLAT[]          = {0x6E, 0x01, 0x00, 0x31, 0xA0};
const uint8_t CMD_ZERO_G[]        = {0x6E, 0x01, 0x00, 0x45, 0xB4};
const uint8_t CMD_TV[]            = {0x6E, 0x01, 0x00, 0x58, 0xC7};
const uint8_t CMD_LIGHT[]         = {0x6E, 0x01, 0x00, 0x3C, 0xAB};
const uint8_t CMD_MEMORY_RECALL[] = {0x6E, 0x01, 0x00, 0x2E, 0x9D};
const uint8_t CMD_MEMORY_STORE[]  = {0x6E, 0x01, 0x00, 0x2B, 0x9A};

std::vector<std::pair<String, const uint8_t*>> commands = {
    {"Head Up", CMD_HEAD_UP}, {"Head Down", CMD_HEAD_DOWN},
    {"Foot Up", CMD_FOOT_UP}, {"Foot Down", CMD_FOOT_DOWN},
    {"Both Up", CMD_BOTH_UP}, {"Both Down", CMD_BOTH_DOWN},
    {"Go Flat", CMD_FLAT}, {"Zero-G", CMD_ZERO_G},
    {"TV Position", CMD_TV}, {"Toggle Light", CMD_LIGHT},
    {"Recall Memory", CMD_MEMORY_RECALL}, {"Store Memory", CMD_MEMORY_STORE}
};

// --- Function Prototypes ---
void drawUI();
bool connectToServer();
void sendCommand(const uint8_t* cmd, size_t len);
bool isMotorCommand(int commandIndex);

// --- BLE Callbacks ---
class MyClientCallback : public BLEClientCallbacks {
    void onConnect(BLEClient* pclient) {
        currentAppState = STATE_CONNECTED;
    }
    void onDisconnect(BLEClient* pclient) {
        currentAppState = STATE_DISCONNECTED;
    }
};

class MyAdvertisedDeviceCallbacks: public BLEAdvertisedDeviceCallbacks {
    void onResult(BLEAdvertisedDevice advertisedDevice) {
        if (advertisedDevice.haveServiceUUID() && advertisedDevice.isAdvertisingService(BLEUUID(RICHMAT_SERVICE_UUID))) {
            BLEDevice::getScan()->stop();
            myDevice = new BLEAdvertisedDevice(advertisedDevice);
            currentAppState = STATE_CONNECTING;
        }
    }
};

// --- UI Drawing ---
void drawUI() {
    M5Cardputer.Display.fillScreen(BLACK);
    M5Cardputer.Display.setTextDatum(top_left);

    String statusText;
    uint16_t statusColor;

    switch(currentAppState) {
        case STATE_SCANNING:
            statusText = "Scanning..."; statusColor = YELLOW; break;
        case STATE_CONNECTING:
            statusText = "Bed Found. Connecting..."; statusColor = CYAN; break;
        case STATE_CONNECTED:
            statusText = "Connected"; statusColor = GREEN; break;
        case STATE_DISCONNECTED:
            statusText = "Disconnected"; statusColor = RED; break;
    }

    M5Cardputer.Display.setTextColor(statusColor);
    M5Cardputer.Display.drawString("Status: " + statusText, 10, 5);
    M5Cardputer.Display.setTextColor(WHITE);
    M5Cardputer.Display.drawFastHLine(0, 20, M5Cardputer.Display.width(), DARKGREY);

    if (currentAppState == STATE_CONNECTED) {
        int lineHeight = 18;
        for (int i = scrollOffset; i < (scrollOffset + VISIBLE_COMMANDS) && i < commands.size(); i++) {
            int y_pos = 25 + (i - scrollOffset) * lineHeight;
            bool isSelected = (i == currentCommand);
            M5Cardputer.Display.setTextColor(isSelected ? BLACK : WHITE, isSelected ? WHITE : BLACK);
            M5Cardputer.Display.drawString(commands[i].first, 10, y_pos);
        }
        M5Cardputer.Display.setTextColor(WHITE, BLACK);
        M5Cardputer.Display.drawString("Up/Down: Select | Hold Enter: Activate", 5, M5Cardputer.Display.height() - 12);
        if(isSending) {
             M5Cardputer.Display.fillCircle(M5Cardputer.Display.width() - 15, M5Cardputer.Display.height() / 2, 5, RED);
        }
    }
}

// --- Setup ---
void setup() {
    M5Cardputer.begin();
    M5Cardputer.Display.setRotation(1);

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

    BLEDevice::init("");

    #ifdef BED_ADDRESS
        myDevice = new BLEAdvertisedDevice();
        myDevice->setAddress(BLEAddress(BED_ADDRESS));
        currentAppState = STATE_CONNECTING;
    #else
        BLEScan* pBLEScan = BLEDevice::getScan();
        pBLEScan->setAdvertisedDeviceCallbacks(new MyAdvertisedDeviceCallbacks());
        pBLEScan->setActiveScan(true);
        pBLEScan->start(10, false); // Non-blocking 10-second scan
    #endif

    drawUI();
}

// --- Main Loop ---
void loop() {
    M5Cardputer.update();

    if (currentAppState == STATE_CONNECTING) {
        if (connectToServer()) {
            // Success
        } else {
            currentAppState = STATE_DISCONNECTED; // Failed to connect
        }
        drawUI();
    }

    if (currentAppState == STATE_CONNECTED) {
        bool needsRedraw = false;
        if (M5Cardputer.Keyboard.isChange()) {
            if (M5Cardputer.Keyboard.isKeyPressed(';')) { // Up
                currentCommand = (currentCommand - 1 + commands.size()) % commands.size();
                needsRedraw = true;
            }
            if (M5Cardputer.Keyboard.isKeyPressed('.')) { // Down
                currentCommand = (currentCommand + 1) % commands.size();
                needsRedraw = true;
            }
            if (needsRedraw) {
                if (currentCommand < scrollOffset) scrollOffset = currentCommand;
                if (currentCommand >= scrollOffset + VISIBLE_COMMANDS) scrollOffset = currentCommand - VISIBLE_COMMANDS + 1;
            }
        }
        
        Keyboard_Class::KeysState keys = M5Cardputer.Keyboard.keysState();
        if (keys.enter) {
            if (!isSending) {
                isSending = true; needsRedraw = true;
                if (!isMotorCommand(currentCommand)) {
                    sendCommand(commands[currentCommand].second, 5);
                }
            }
            if (isMotorCommand(currentCommand) && millis() - lastSendTime > SEND_INTERVAL) {
                sendCommand(commands[currentCommand].second, 5);
                lastSendTime = millis();
            }
        } else {
            if (isSending) {
                isSending = false; needsRedraw = true;
                if(isMotorCommand(currentCommand)) {
                    sendCommand(CMD_STOP, 5);
                }
            }
        }

        if (needsRedraw) drawUI();
    }

    // Exit to launcher
    if (M5Cardputer.Keyboard.isPressed()) {
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

// --- Helper Functions ---
bool connectToServer() {
    if (myDevice == nullptr) return false;

    pClient = BLEDevice::createClient();
    pClient->setClientCallbacks(new MyClientCallback());
    if (!pClient->connect(myDevice)) return false;

    BLERemoteService* pRemoteService = pClient->getService(RICHMAT_SERVICE_UUID);
    if (pRemoteService == nullptr) { pClient->disconnect(); return false; }

    pCharacteristic = pRemoteService->getCharacteristic(RICHMAT_CHARACTERISTIC_UUID);
    if (pCharacteristic == nullptr) { pClient->disconnect(); return false; }
    
    return true;
}

void sendCommand(const uint8_t* cmd, size_t len) {
    if (currentAppState == STATE_CONNECTED && pCharacteristic != nullptr) {
        pCharacteristic->writeValue((uint8_t*)cmd, len, false);
    }
}

bool isMotorCommand(int commandIndex) {
    String name = commands[commandIndex].first;
    return (name.indexOf("Up") != -1 || name.indexOf("Down") != -1);
}

