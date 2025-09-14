#include <M5Cardputer.h>
#include <esp_ota_ops.h>
#include <esp_system.h>
#include <BLEDevice.h>
#include <vector>
#include <string>
#include <map>
#include <SD.h>

#include <M5CardputerOS_core.h>
#include <settings_manager.h>
#include <sd_card.h>

// --- App-Specific State Machine ---
enum ScannerState {
    STATE_SCANNING,
    STATE_DEVICE_LIST,
    STATE_CONNECTING,
    STATE_DEVICE_DETAILS,
    STATE_SAVING
};
ScannerState currentScannerState = STATE_SCANNING;

// --- Data Structures ---
struct DiscoveredDevice {
    std::string name;
    BLEAddress address;
    int rssi;
    std::string rawAdvertisement;
};
struct ServiceInfo {
    std::string uuid;
    std::vector<std::string> characteristics;
};

std::vector<DiscoveredDevice> foundDevices;
std::vector<ServiceInfo> capturedServices;
int listScrollOffset = 0;
int selectedDeviceIndex = 0;

// --- Globals ---
float appTextSize = 1.0f;
BLEClient* pClient = nullptr;

// --- Function Prototypes ---
void drawUI();
void connectAndCapture(DiscoveredDevice device);
std::string getPropertiesString(BLERemoteCharacteristic* pChar);
void saveDeviceProfile();

// --- BLE Scanner Callback ---
class MyAdvertisedDeviceCallbacks: public BLEAdvertisedDeviceCallbacks {
    void onResult(BLEAdvertisedDevice advertisedDevice) {
        for (int i = 0; i < foundDevices.size(); i++) {
            if (foundDevices[i].address.equals(advertisedDevice.getAddress())) {
                foundDevices[i].rssi = advertisedDevice.getRSSI();
                return;
            }
        }
        foundDevices.push_back({
            advertisedDevice.haveName() ? advertisedDevice.getName() : "Unknown",
            advertisedDevice.getAddress(),
            advertisedDevice.getRSSI(),
            advertisedDevice.getPayload() != nullptr ? BLEUtils::buildHexData(nullptr, (uint8_t*)advertisedDevice.getPayload(), advertisedDevice.getPayloadLength()) : ""
        });
    }
};

// --- UI Drawing ---
void drawUI() {
    M5Cardputer.Display.fillScreen(BLACK);
    M5Cardputer.Display.setTextDatum(top_left);

    String statusText;
    uint16_t statusColor;

    switch(currentScannerState) {
        case STATE_SCANNING: statusText = "Scanning..."; statusColor = YELLOW; break;
        case STATE_DEVICE_LIST: statusText = "Scan Complete. Select Device."; statusColor = GREEN; break;
        case STATE_CONNECTING: statusText = "Connecting & Capturing..."; statusColor = CYAN; break;
        case STATE_DEVICE_DETAILS: statusText = "Device Details"; statusColor = GREEN; break;
        case STATE_SAVING: statusText = "Saving to SD Card..."; statusColor = CYAN; break;
    }

    M5Cardputer.Display.setTextColor(statusColor);
    M5Cardputer.Display.drawString("Status: " + statusText, 10, 5);
    M5Cardputer.Display.setTextColor(WHITE);
    M5Cardputer.Display.drawFastHLine(0, 20, M5Cardputer.Display.width(), DARKGREY);

    if (currentScannerState == STATE_DEVICE_LIST || currentScannerState == STATE_SCANNING) {
        if (foundDevices.empty() && currentScannerState == STATE_DEVICE_LIST) {
            M5Cardputer.Display.drawString("No devices found.", 10, 30);
        } else {
            int lineHeight = 18;
            for (int i = listScrollOffset; i < (listScrollOffset + 6) && i < foundDevices.size(); i++) {
                int y_pos = 25 + (i - listScrollOffset) * lineHeight;
                String line = String(foundDevices[i].name.c_str()) + " (" + String(foundDevices[i].rssi) + "dBm)";
                bool isSelected = (i == selectedDeviceIndex);
                M5Cardputer.Display.setTextColor(isSelected ? BLACK : WHITE, isSelected ? WHITE : BLACK);
                M5Cardputer.Display.drawString(line, 5, y_pos);
                M5Cardputer.Display.drawString(foundDevices[i].address.toString().c_str(), 5, y_pos + 8);
            }
        }
        M5Cardputer.Display.setTextColor(WHITE, BLACK);
        M5Cardputer.Display.drawString("Up/Down: Scroll | Enter: Details", 5, M5Cardputer.Display.height() - 15);
    } else if (currentScannerState == STATE_DEVICE_DETAILS) {
        int y_pos = 25;
        int lineHeight = 10;
        M5Cardputer.Display.drawString(foundDevices[selectedDeviceIndex].name.c_str(), 5, y_pos); y_pos += lineHeight;
        M5Cardputer.Display.drawString(foundDevices[selectedDeviceIndex].address.toString().c_str(), 5, y_pos); y_pos += lineHeight;
        M5Cardputer.Display.drawString("Raw Adv:", 5, y_pos); y_pos += lineHeight;
        M5Cardputer.Display.drawString(foundDevices[selectedDeviceIndex].rawAdvertisement.c_str(), 5, y_pos); y_pos += (lineHeight + 3);
        M5Cardputer.Display.drawFastHLine(0, y_pos-2, M5Cardputer.Display.width(), DARKGREY);
        
        for(int i = 0; i < capturedServices.size(); i++){
            M5Cardputer.Display.setTextColor(CYAN);
            M5Cardputer.Display.drawString(capturedServices[i].uuid.c_str(), 5, y_pos); y_pos += lineHeight;
            M5Cardputer.Display.setTextColor(WHITE);
            for(int j = 0; j < capturedServices[i].characteristics.size(); j++){
                 if (y_pos < M5Cardputer.Display.height() - 20) { // Prevent drawing off screen
                    M5Cardputer.Display.drawString(capturedServices[i].characteristics[j].c_str(), 15, y_pos); y_pos += lineHeight;
                 }
            }
        }
        M5Cardputer.Display.setTextColor(WHITE, BLACK);
        M5Cardputer.Display.drawString("Enter: Save | DEL: Back", 5, M5Cardputer.Display.height() - 15);
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
    
    mountSD(); // Mount the SD card

    BLEDevice::init("");
    BLEScan* pBLEScan = BLEDevice::getScan();
    pBLEScan->setAdvertisedDeviceCallbacks(new MyAdvertisedDeviceCallbacks());
    pBLEScan->setActiveScan(true);
    pBLEScan->start(10, [](BLEScanResults results){
        currentScannerState = STATE_DEVICE_LIST;
    });

    drawUI();
}

void loop() {
    M5Cardputer.update();

    static ScannerState previousState = STATE_SCANNING;
    if (previousState != currentScannerState) {
        drawUI();
        previousState = currentScannerState;
    }

    if (currentScannerState == STATE_DEVICE_LIST && M5Cardputer.Keyboard.isChange() && M5Cardputer.Keyboard.isPressed()) {
        if (M5Cardputer.Keyboard.isKeyPressed(';')) { // Up
            if(!foundDevices.empty()) selectedDeviceIndex = (selectedDeviceIndex - 1 + foundDevices.size()) % foundDevices.size();
        }
        if (M5Cardputer.Keyboard.isKeyPressed('.')) { // Down
            if(!foundDevices.empty()) selectedDeviceIndex = (selectedDeviceIndex + 1) % foundDevices.size();
        }

        if (M5Cardputer.Keyboard.keysState().enter) {
            if (!foundDevices.empty()) {
                connectAndCapture(foundDevices[selectedDeviceIndex]);
            }
        }
        
        if (selectedDeviceIndex < listScrollOffset) listScrollOffset = selectedDeviceIndex;
        if (selectedDeviceIndex >= listScrollOffset + 6) listScrollOffset = selectedDeviceIndex - 5;

        drawUI();
    } else if (currentScannerState == STATE_DEVICE_DETAILS && M5Cardputer.Keyboard.isChange() && M5Cardputer.Keyboard.isPressed()) {
        if(M5Cardputer.Keyboard.keysState().del) {
            if(pClient && pClient->isConnected()) pClient->disconnect();
            capturedServices.clear();
            currentScannerState = STATE_DEVICE_LIST;
            drawUI();
        }
        if (M5Cardputer.Keyboard.keysState().enter) {
            saveDeviceProfile();
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
void connectAndCapture(DiscoveredDevice device) {
    currentScannerState = STATE_CONNECTING;
    drawUI();

    capturedServices.clear();

    if(pClient) {
        delete pClient;
        pClient = nullptr;
    }

    pClient = BLEDevice::createClient();
    if (!pClient->connect(device.address)) {
        currentScannerState = STATE_DEVICE_LIST;
        return;
    }

    std::map<std::string, BLERemoteService*>* services = pClient->getServices();
    // Use a compatible iterator loop
    for (std::map<std::string, BLERemoteService*>::iterator it = services->begin(); it != services->end(); ++it) {
        ServiceInfo service;
        service.uuid = it->second->getUUID().toString();
        
        std::map<std::string, BLERemoteCharacteristic*>* characteristics = it->second->getCharacteristics();
        for (std::map<std::string, BLERemoteCharacteristic*>::iterator char_it = characteristics->begin(); char_it != characteristics->end(); ++char_it) {
            std::string props = getPropertiesString(char_it->second);
            service.characteristics.push_back(char_it->second->getUUID().toString() + " [" + props + "]");
        }
        capturedServices.push_back(service);
    }
    
    currentScannerState = STATE_DEVICE_DETAILS;
}

std::string getPropertiesString(BLERemoteCharacteristic* pChar) {
    std::string props = "";
    if (pChar->canRead()) props += "R ";
    if (pChar->canWrite()) props += "W ";
    if (pChar->canWriteNoResponse()) props += "WNR ";
    if (pChar->canNotify()) props += "N ";
    if (pChar->canIndicate()) props += "I ";
    return props;
}

void saveDeviceProfile() {
    if (!isSdCardMounted || foundDevices.empty()) return;

    currentScannerState = STATE_SAVING;
    drawUI();

    DiscoveredDevice device = foundDevices[selectedDeviceIndex];
    String fileName = "/" + String(device.name.c_str()) + ".txt";
    fileName.replace(":", ""); // Sanitize filename

    File file = SD.open(fileName, FILE_WRITE);
    if (file) {
        file.println("--- BLE Device Profile ---");
        file.println("Name: " + String(device.name.c_str()));
        file.println("Address: " + String(device.address.toString().c_str()));
        file.println("RSSI: " + String(device.rssi) + " dBm");
        file.println("Raw Advertisement: " + String(device.rawAdvertisement.c_str()));
        file.println("\n--- Services & Characteristics ---");
        for(const auto& service : capturedServices) {
            file.println("Service: " + String(service.uuid.c_str()));
            for(const auto& characteristic : service.characteristics) {
                file.println("  - Char: " + String(characteristic.c_str()));
            }
        }
        file.close();
    }
    
    delay(1000); // Show "Saving..." message
    currentScannerState = STATE_DEVICE_DETAILS;
    drawUI();
}

