#include <M5Cardputer.h>
#include <esp_ota_ops.h>
#include <esp_system.h>
#include <BLEDevice.h>
#include <vector>
#include <string>

#include <M5CardputerOS_core.h>
#include <settings_manager.h>

// --- Govee BLE Definitions ---
#define GOVEE_SERVICE_UUID           "00010203-0405-0607-0809-0a0b0c0d1910"
#define GOVEE_CHARACTERISTIC_UUID_CONTROL "00010203-0405-0607-0809-0a0b0c0d2b11"

// --- App-Specific State Machine ---
enum ControllerState {
    STATE_SCANNING,
    STATE_DEVICE_LIST,
    STATE_CONNECTING,
    STATE_CONNECTED,
    STATE_DISCONNECTED
};
ControllerState currentGoveeState = STATE_SCANNING; // Renamed to avoid conflict

// --- Data Structures ---
struct GoveeDevice {
    std::string name;
    BLEAddress* address;
};
std::vector<GoveeDevice> foundDevices;
int selectedDeviceIndex = 0;
int deviceScrollOffset = 0;

// --- Globals ---
float appTextSize = 1.0f;
BLEClient* pClient = nullptr;
BLERemoteCharacteristic* pCharacteristic = nullptr;

// --- Control Parameters ---
enum SelectedParam { PARAM_STATE, PARAM_RED, PARAM_GREEN, PARAM_BLUE, PARAM_BRIGHTNESS, PARAM_DISCONNECT, NUM_PARAMS };
SelectedParam currentParam = PARAM_STATE;
bool powerState = true;
int red = 255, green = 0, blue = 0;
int brightness = 100;

// --- Function Prototypes ---
void drawUI();
bool connectToServer(BLEAddress* pAddress);
void sendCommand(const uint8_t* payload, size_t len);
void setPower(bool on);
void setColor(int r, int g, int b);
void setBrightness(int level);

// --- BLE Callbacks ---
class MyClientCallback : public BLEClientCallbacks {
    void onConnect(BLEClient* pclient) { currentGoveeState = STATE_CONNECTED; }
    void onDisconnect(BLEClient* pclient) { 
        currentGoveeState = STATE_DEVICE_LIST; // Go back to list after disconnect
        pClient = nullptr; // Clear the client
    }
};

class MyAdvertisedDeviceCallbacks: public BLEAdvertisedDeviceCallbacks {
    void onResult(BLEAdvertisedDevice advertisedDevice) {
        if (advertisedDevice.haveServiceUUID() && advertisedDevice.isAdvertisingService(BLEUUID(GOVEE_SERVICE_UUID))) {
            if (advertisedDevice.haveName()) {
                std::string name = advertisedDevice.getName();
                // Check if the device name contains any of the known model numbers
                if (name.find("H6184") != std::string::npos || 
                    name.find("H7090") != std::string::npos || 
                    name.find("B7080") != std::string::npos) 
                {
                    // Check if we already have this device
                    bool found = false;
                    for (auto const& dev : foundDevices) {
                        if (dev.address->equals(advertisedDevice.getAddress())) { // Corrected this line
                            found = true;
                            break;
                        }
                    }
                    if (!found) {
                        foundDevices.push_back({advertisedDevice.getName(), new BLEAddress(advertisedDevice.getAddress())});
                    }
                }
            }
        }
    }
};

// --- UI Drawing ---
void drawUI() {
    M5Cardputer.Display.fillScreen(BLACK);
    M5Cardputer.Display.setTextDatum(top_left);

    String statusText;
    uint16_t statusColor;

    switch(currentGoveeState) {
        case STATE_SCANNING: statusText = "Scanning for Govee..."; statusColor = YELLOW; break;
        case STATE_DEVICE_LIST: statusText = "Select a Device"; statusColor = CYAN; break;
        case STATE_CONNECTING: statusText = "Connecting..."; statusColor = CYAN; break;
        case STATE_CONNECTED: statusText = "Connected to " + String(foundDevices[selectedDeviceIndex].name.c_str()); statusColor = GREEN; break;
        case STATE_DISCONNECTED: statusText = "Disconnected"; statusColor = RED; break;
    }

    M5Cardputer.Display.setTextColor(statusColor);
    M5Cardputer.Display.drawString("Status: " + statusText, 10, 5);
    M5Cardputer.Display.setTextColor(WHITE);
    M5Cardputer.Display.drawFastHLine(0, 20, M5Cardputer.Display.width(), DARKGREY);

    if (currentGoveeState == STATE_DEVICE_LIST) {
        if (foundDevices.empty()) {
            M5Cardputer.Display.drawString("No devices found.", 10, 30);
        } else {
            int lineHeight = 18;
            for (int i = deviceScrollOffset; i < (deviceScrollOffset + 6) && i < foundDevices.size(); i++) {
                int y_pos = 25 + (i-deviceScrollOffset) * lineHeight;
                bool isSelected = (i == selectedDeviceIndex);
                M5Cardputer.Display.setTextColor(isSelected ? BLACK : WHITE, isSelected ? WHITE : BLACK);
                M5Cardputer.Display.drawString(foundDevices[i].name.c_str(), 10, y_pos);
            }
        }
    } else if (currentGoveeState == STATE_CONNECTED) {
        const char* paramNames[] = {"State", "Red", "Green", "Blue", "Brightness", "Disconnect"};
        String paramValues[] = {
            powerState ? "ON" : "OFF",
            String(red), String(green), String(blue), String(brightness) + "%", ""
        };

        int lineHeight = 18;
        for (int i = 0; i < NUM_PARAMS; i++) {
            int y_pos = 25 + i * lineHeight;
            bool isSelected = (i == currentParam);
            M5Cardputer.Display.setTextColor(isSelected ? BLACK : WHITE, isSelected ? WHITE : BLACK);
            M5Cardputer.Display.drawString(String(paramNames[i]) + ": < " + paramValues[i] + " >", 10, y_pos);
        }
    }
}

// --- App Logic ---
void setup() {
    M5Cardputer.begin();
    M5Cardputer.Display.setRotation(1);

    settings_init();
    appTextSize = (float)settings_get_font_size() / 10.0f;
    String savedFontName = settings_get_font_name();
    for (int i = 0; i < numAvailableFonts; ++i) {
        if (savedFontName == availableFonts[i].name) { currentFontSelection = i; break; }
    }
    M5Cardputer.Display.setFont(availableFonts[currentFontSelection].font);
    M5Cardputer.Display.setTextSize(appTextSize);

    BLEDevice::init("");
    BLEScan* pBLEScan = BLEDevice::getScan();
    pBLEScan->setAdvertisedDeviceCallbacks(new MyAdvertisedDeviceCallbacks());
    pBLEScan->setActiveScan(true);
    pBLEScan->start(10, [](BLEScanResults results){
        currentGoveeState = STATE_DEVICE_LIST;
    });

    drawUI();
}

void loop() {
    M5Cardputer.update();

    // --- Redraw UI on state change from scanning ---
    static ControllerState previousState = STATE_SCANNING;
    if (previousState != currentGoveeState) {
        drawUI();
        previousState = currentGoveeState;
    }
    // ---------------------------------------------

    // --- State: Device List ---
    if (currentGoveeState == STATE_DEVICE_LIST) {
        if (M5Cardputer.Keyboard.isChange() && M5Cardputer.Keyboard.isPressed() && !foundDevices.empty()) {
            if (M5Cardputer.Keyboard.isKeyPressed(';')) selectedDeviceIndex = (selectedDeviceIndex - 1 + foundDevices.size()) % foundDevices.size();
            if (M5Cardputer.Keyboard.isKeyPressed('.')) selectedDeviceIndex = (selectedDeviceIndex + 1) % foundDevices.size();
            
            if (M5Cardputer.Keyboard.keysState().enter) {
                currentGoveeState = STATE_CONNECTING;
                drawUI();
                if (!connectToServer(foundDevices[selectedDeviceIndex].address)) {
                     currentGoveeState = STATE_DEVICE_LIST; // Connection failed, return to list
                }
            }

            if (selectedDeviceIndex < deviceScrollOffset) deviceScrollOffset = selectedDeviceIndex;
            if (selectedDeviceIndex >= deviceScrollOffset + 6) deviceScrollOffset = selectedDeviceIndex - 5;
            
            drawUI();
        }
    }

    // --- State: Connected ---
    else if (currentGoveeState == STATE_CONNECTED) {
        if (M5Cardputer.Keyboard.isChange() && M5Cardputer.Keyboard.isPressed()) {
            int change = 0;
            if (M5Cardputer.Keyboard.isKeyPressed(';')) currentParam = (SelectedParam)((currentParam - 1 + NUM_PARAMS) % NUM_PARAMS);
            if (M5Cardputer.Keyboard.isKeyPressed('.')) currentParam = (SelectedParam)((currentParam + 1) % NUM_PARAMS);
            if (M5Cardputer.Keyboard.isKeyPressed(',')) change = -1;
            if (M5Cardputer.Keyboard.isKeyPressed('/')) change = 1;

            if (M5Cardputer.Keyboard.keysState().enter && currentParam == PARAM_DISCONNECT) {
                if(pClient) pClient->disconnect();
            }

            if (change != 0) {
                switch(currentParam) {
                    case PARAM_STATE: powerState = !powerState; setPower(powerState); break;
                    case PARAM_RED: red = max(0, min(255, red + change * 15)); break;
                    case PARAM_GREEN: green = max(0, min(255, green + change * 15)); break;
                    case PARAM_BLUE: blue = max(0, min(255, blue + change * 15)); break;
                    case PARAM_BRIGHTNESS: brightness = max(0, min(100, brightness + change * 5)); setBrightness(brightness); break;
                }
                if (currentParam >= PARAM_RED && currentParam <= PARAM_BLUE) {
                    setColor(red, green, blue);
                }
            }
             drawUI();
        }
    }
    
    // Exit to launcher
    if (M5Cardputer.Keyboard.isPressed()) {
        if (M5Cardputer.Keyboard.keysState().fn && M5Cardputer.Keyboard.isKeyPressed('`')) {
            pixel.clear(); pixel.show();
            esp_ota_set_boot_partition(esp_partition_find_first(ESP_PARTITION_TYPE_APP, ESP_PARTITION_SUBTYPE_APP_OTA_0, NULL));
            esp_restart();
        }
    }
}

// --- Helper Functions ---
bool connectToServer(BLEAddress* pAddress) {
    if(pClient) {
        pClient->disconnect();
        delete pClient;
        pClient = nullptr;
    }
    pClient = BLEDevice::createClient();
    pClient->setClientCallbacks(new MyClientCallback());
    if (!pClient->connect(*pAddress)) return false;

    BLERemoteService* pRemoteService = pClient->getService(GOVEE_SERVICE_UUID);
    if (pRemoteService == nullptr) { pClient->disconnect(); return false; }

    pCharacteristic = pRemoteService->getCharacteristic(GOVEE_CHARACTERISTIC_UUID_CONTROL);
    if (pCharacteristic == nullptr) { pClient->disconnect(); return false; }
    
    return true;
}

void sendCommand(const uint8_t* payload, size_t len) {
    if (currentGoveeState != STATE_CONNECTED || pCharacteristic == nullptr) return;

    uint8_t command[20] = {0};
    memcpy(command, payload, len);
    uint8_t checksum = 0;
    for (int i = 0; i < 19; i++) { checksum ^= command[i]; }
    command[19] = checksum;

    pCharacteristic->writeValue(command, 20, false);
}

void setPower(bool on) {
    uint8_t payload[3] = {0x33, 0x01, (uint8_t)(on ? 0x01 : 0x00)};
    sendCommand(payload, sizeof(payload));
}

void setColor(int r, int g, int b) {
    uint8_t payload[6] = {0x33, 0x05, 0x02, (uint8_t)r, (uint8_t)g, (uint8_t)b};
    sendCommand(payload, sizeof(payload));
}

void setBrightness(int level) {
    uint8_t payload[3] = {0x33, 0x04, (uint8_t)map(level, 0, 100, 0, 254)};
    sendCommand(payload, sizeof(payload));
}

