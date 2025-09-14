#include "M5CardputerOS_core.h"

// --- NeoPixel Definition ---
#define NEOPIXEL_PIN 21
Adafruit_NeoPixel pixel(1, NEOPIXEL_PIN, NEO_GRB + NEO_KHZ800);
// --------------------

// Removed Preferences preferences; definition as it's now managed by settings_manager

// --- Font Definitions ---
const FontInfo availableFonts[] = {
    {"Orbitron", &fonts::Orbitron_Light_24},
    {"Mono Oblique", &fonts::FreeMonoOblique9pt7b},
    {"FreeMono", &fonts::FreeMono9pt7b},
    {"FreeSans", &fonts::FreeSans9pt7b},
    {"TomThumb", &fonts::TomThumb}
};
const int numAvailableFonts = sizeof(availableFonts) / sizeof(availableFonts[0]);
int currentFontSelection = 0; // Default to the first font
// --------------------

float menuTextSize = 0.8;
AppState currentState = STATE_MAIN_MENU;
#ifdef ENABLE_SD_CARD
bool isSdCardMounted = false;
#endif
#ifdef ENABLE_WIFI
std::vector<String> scanned_networks;
int selected_network_index = 0;
String selected_ssid = "";
String password_buffer = "";
#endif
String lastKeyPressed = "None";
const char* mainMenuItems[] = { "Apps", "Keyboard Test", "Meshtastic Chat", "Settings" };
const int numMainMenuItems = sizeof(mainMenuItems) / sizeof(mainMenuItems[0]);
int currentMainMenuSelection = 0;

std::vector<const char*> settingsMenuItems;
int numSettingsMenuItems = 0;
int currentSettingsSelection = 0;

const char* displayMenuItems[] = { "Text Size", "Font", "Back" };
const int numDisplayMenuItems = sizeof(displayMenuItems) / sizeof(displayMenuItems[0]);
int currentDisplaySelection = 0;
#ifdef ENABLE_SD_CARD
const char* sdCardMenuItems[] = { "SD Card Info", "Mount/Unmount SD", "Back" };
const int numSdCardMenuItems = sizeof(sdCardMenuItems) / sizeof(sdCardMenuItems[0]);
int currentSdCardSelection = 0;
#endif
#ifdef ENABLE_WIFI
const char* wifiMenuItems[] = { "Status", "Scan for Networks", "Disconnect", "Back" };
const int numWifiMenuItems = sizeof(wifiMenuItems) / sizeof(wifiMenuItems[0]);
int currentWifiSelection = 0;
#endif
std::vector<String> app_list;
int currentAppSelection = 0;

// --- New function to load and run applications ---
#include <SD.h>
#include <Update.h>
#include <esp_partition.h>
#include "esp_ota_ops.h"

void loadAndRunApp(const String& appPath) {
    Serial.printf("Attempting to load app: %s\n", appPath.c_str());

    if (!SD.exists(appPath)) {
        Serial.println("App file not found on SD card.");
        return;
    }

    File appFile = SD.open(appPath);
    if (!appFile) {
        Serial.println("Failed to open app file.");
        return;
    }

    size_t appSize = appFile.size();
    if (appSize == 0) {
        Serial.println("App file is empty.");
        appFile.close();
        return;
    }

    // Get the current running partition
    const esp_partition_t *running_partition = esp_ota_get_running_partition();
    if (running_partition == NULL) {
        Serial.println("Failed to get running partition.");
        appFile.close();
        return;
    }

    // Find the other OTA partition
    const esp_partition_t *update_partition = NULL;
    if (running_partition->subtype == ESP_PARTITION_SUBTYPE_APP_OTA_0) {
        update_partition = esp_partition_find_first(ESP_PARTITION_TYPE_APP, ESP_PARTITION_SUBTYPE_APP_OTA_1, NULL);
    } else if (running_partition->subtype == ESP_PARTITION_SUBTYPE_APP_OTA_1) {
        update_partition = esp_partition_find_first(ESP_PARTITION_TYPE_APP, ESP_PARTITION_SUBTYPE_APP_OTA_0, NULL);
    }

    if (update_partition == NULL) {
        Serial.println("No other OTA partition found.");
        appFile.close();
        return;
    }

    Serial.printf("Flashing app to partition: %s\n", update_partition->label);

    esp_ota_handle_t ota_handle;
    if (esp_ota_begin(update_partition, OTA_SIZE_UNKNOWN, &ota_handle) != ESP_OK) {
        Serial.println("esp_ota_begin failed.");
        appFile.close();
        return;
    }

    uint8_t buffer[4096];
    size_t bytesRead = 0;
    while ((bytesRead = appFile.read(buffer, sizeof(buffer))) > 0) {
        if (esp_ota_write(ota_handle, buffer, bytesRead) != ESP_OK) {
            Serial.println("esp_ota_write failed.");
            esp_ota_end(ota_handle);
            appFile.close();
            return;
        }
        yield(); 
    }
    appFile.close();

    if (esp_ota_end(ota_handle) != ESP_OK) {
        Serial.println("esp_ota_end failed.");
        return;
    }
    
    if (esp_ota_set_boot_partition(update_partition) != ESP_OK) {
        Serial.println("esp_ota_set_boot_partition failed.");
        return;
    }

    Serial.println("App loaded successfully. Rebooting...");
    esp_restart();
}
