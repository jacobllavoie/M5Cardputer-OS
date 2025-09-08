#include "globals.h"
#include "ui.h"

#ifdef ENABLE_SETTINGS_PERSISTENCE
Preferences preferences;
#endif
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
const char* mainMenuItems[] = { "Apps", "Keyboard Test", "Option C", "Settings" };
const int numMainMenuItems = sizeof(mainMenuItems) / sizeof(mainMenuItems[0]);
int currentMainMenuSelection = 0;

// --- CORRECTED MENU DEFINITION ---
// We now use a vector that we will build dynamically.
std::vector<const char*> settingsMenuItems;
int numSettingsMenuItems = 0;
int currentSettingsSelection = 0;
// --- END OF CORRECTION ---

const char* displayMenuItems[] = { "Text Size", "Back" };
const int numDisplayMenuItems = sizeof(displayMenuItems) / sizeof(displayMenuItems[0]);
int currentDisplaySelection = 0;
#ifdef ENABLE_SD_CARD
const char* sdCardMenuItems[] = { "SD Card Info", "Mount/Unmount SD", "Back" };
const int numSdCardMenuItems = sizeof(sdCardMenuItems) / sizeof(sdCardMenuItems[0]);
int currentSdCardSelection = 0;
#endif
#ifdef ENABLE_WIFI
const char* wifiMenuItems[] = { "Status", "Scan for Networks", "Web Server", "Disconnect", "Back" };
const int numWifiMenuItems = sizeof(wifiMenuItems) / sizeof(wifiMenuItems[0]);
int currentWifiSelection = 0;
#endif
std::vector<String> app_list;
int currentAppSelection = 0;


// --- NEW FUNCTION TO BUILD MENUS ---
void initializeMenus() {
    #ifdef DEBUG_MODE
    Serial.println("DEBUG: initializeMenus() called");
    #endif
    settingsMenuItems.clear(); // Clear any previous items
    settingsMenuItems.push_back("Display");
    #ifdef ENABLE_SD_CARD
    settingsMenuItems.push_back("SD Card");
    #endif
    #ifdef ENABLE_WIFI
    settingsMenuItems.push_back("WiFi");
    #endif
    #ifdef ENABLE_OTA
    settingsMenuItems.push_back("OTA Update");
    #endif
    settingsMenuItems.push_back("Factory Reset");
    settingsMenuItems.push_back("Back");

    numSettingsMenuItems = settingsMenuItems.size();
}
// --- END OF NEW FUNCTION ---

// The rest of the file remains the same...
void drawScreen() {
    #ifdef DEBUG_MODE
    Serial.println("DEBUG: drawScreen() called");
    #endif
    if (currentState == STATE_MAIN_MENU) drawMainMenu();
    else if (currentState == STATE_APPS_MENU) drawAppsMenu();
    else if (currentState == STATE_SETTINGS_MENU) drawSettingsMenu();
    else if (currentState == STATE_DISPLAY_SETTINGS_MENU) drawDisplaySettingsMenu();
    else if (currentState == STATE_FACTORY_RESET_CONFIRM) drawFactoryResetConfirmScreen();
    else if (currentState == STATE_KEYBOARD_TEST) drawKeyboardTestScreen();
    #ifdef ENABLE_SD_CARD
    else if (currentState == STATE_SDCARD_SETTINGS_MENU) drawSdCardMenu();
    #endif
    #ifdef ENABLE_WIFI
    else if (currentState == STATE_WIFI_SETTINGS_MENU) drawWifiSettingsMenu();
    else if (currentState == STATE_WIFI_SCAN_RESULTS) drawWifiScanResults();
    else if (currentState == STATE_WIFI_PASSWORD_INPUT) drawPasswordInputScreen();
    #endif
    #ifdef ENABLE_WEB_SERVER
    else if (currentState == STATE_WEB_SERVER_ACTIVE) drawWebServerScreen();
    #endif
    #ifdef ENABLE_OTA
    else if (currentState == STATE_OTA_MODE) drawOtaScreen();
    #endif
}

void drawBatteryStatus() {
    #ifdef DEBUG_MODE
    Serial.println("DEBUG: drawBatteryStatus() called");
    #endif
    int percentage = M5.Power.getBatteryLevel();
    float voltage = M5.Power.getBatteryVoltage() / 1000.0;
    String status = String(percentage) + "% " + String(voltage, 2) + "V";
    M5Cardputer.Display.setTextDatum(top_right);
    M5Cardputer.Display.setFont(&fonts::Orbitron_Light_24);
    M5Cardputer.Display.setTextSize(0.7);
    M5Cardputer.Display.setTextColor(TEXT_COLOR, BACKGROUND_COLOR);
    M5Cardputer.Display.drawString(status, M5Cardputer.Display.width() - 10, 5);
}

void drawMainMenu() {
    #ifdef DEBUG_MODE
    Serial.println("DEBUG: drawMainMenu() called");
    #endif
    M5Cardputer.Display.fillScreen(BACKGROUND_COLOR);
    drawBatteryStatus(); 
    
    M5Cardputer.Display.setTextDatum(middle_left);
    M5Cardputer.Display.setFont(&fonts::Orbitron_Light_24);
    M5Cardputer.Display.setTextSize(menuTextSize);

    for (int i = 0; i < numMainMenuItems; i++) {
        if (i == currentMainMenuSelection) M5Cardputer.Display.setTextColor(HIGHLIGHT_TEXT_COLOR, HIGHLIGHT_COLOR);
        else M5Cardputer.Display.setTextColor(TEXT_COLOR);
        M5Cardputer.Display.drawString(mainMenuItems[i], 20, 30 + i * 30);
    }
    M5Cardputer.Display.setTextColor(TEXT_COLOR);
}

void drawAppsMenu() {
    #ifdef DEBUG_MODE
    Serial.println("DEBUG: drawAppsMenu() called");
    #endif
    M5Cardputer.Display.fillScreen(BACKGROUND_COLOR);
    M5Cardputer.Display.setTextDatum(middle_left);
    M5Cardputer.Display.setFont(&fonts::Orbitron_Light_24);
    M5Cardputer.Display.setTextSize(menuTextSize);

    M5Cardputer.Display.drawString("Select App:", 20, 15);

    if (app_list.empty()) {
        M5Cardputer.Display.drawString("No apps found in /apps", 20, 60);
    } else {
        for (size_t i = 0; i < app_list.size(); i++) {
            if (i == currentAppSelection) M5Cardputer.Display.setTextColor(HIGHLIGHT_TEXT_COLOR, HIGHLIGHT_COLOR);
            else M5Cardputer.Display.setTextColor(TEXT_COLOR);
            M5Cardputer.Display.drawString(app_list[i], 20, 40 + i * 25);
        }
    }
    M5Cardputer.Display.setTextColor(TEXT_COLOR);
}

void drawSettingsMenu() {
    #ifdef DEBUG_MODE
    Serial.println("DEBUG: drawSettingsMenu() called");
    #endif
    M5Cardputer.Display.fillScreen(BACKGROUND_COLOR); M5Cardputer.Display.setTextDatum(middle_left); M5Cardputer.Display.setFont(&fonts::Orbitron_Light_24); M5Cardputer.Display.setTextSize(menuTextSize);
    for (int i = 0; i < numSettingsMenuItems; i++) { if (i == currentSettingsSelection) M5Cardputer.Display.setTextColor(HIGHLIGHT_TEXT_COLOR, HIGHLIGHT_COLOR); else M5Cardputer.Display.setTextColor(TEXT_COLOR); M5Cardputer.Display.drawString(settingsMenuItems[i], 20, 30 + i * 30); }
    M5Cardputer.Display.setTextColor(TEXT_COLOR);
}
void drawDisplaySettingsMenu() {
    #ifdef DEBUG_MODE
    Serial.println("DEBUG: drawDisplaySettingsMenu() called");
    #endif
    M5Cardputer.Display.fillScreen(BACKGROUND_COLOR); M5Cardputer.Display.setTextDatum(middle_left); M5Cardputer.Display.setFont(&fonts::Orbitron_Light_24); M5Cardputer.Display.setTextSize(menuTextSize);
    for (int i = 0; i < numDisplayMenuItems; i++) { if (i == currentDisplaySelection) M5Cardputer.Display.setTextColor(HIGHLIGHT_TEXT_COLOR, HIGHLIGHT_COLOR); else M5Cardputer.Display.setTextColor(TEXT_COLOR); String itemText = displayMenuItems[i]; if (itemText == "Text Size") { itemText = String("Text Size: < ") + String(menuTextSize, 1) + String(" >"); } M5Cardputer.Display.drawString(itemText, 20, 30 + i * 30); }
    M5Cardputer.Display.setTextColor(TEXT_COLOR);
}
#ifdef ENABLE_SD_CARD
void drawSdCardMenu() {
    #ifdef DEBUG_MODE
    Serial.println("DEBUG: drawSdCardMenu() called");
    #endif
    M5Cardputer.Display.fillScreen(BACKGROUND_COLOR); M5Cardputer.Display.setTextDatum(middle_left); M5Cardputer.Display.setFont(&fonts::Orbitron_Light_24); M5Cardputer.Display.setTextSize(menuTextSize);
    for (int i = 0; i < numSdCardMenuItems; i++) { if (i == currentSdCardSelection) M5Cardputer.Display.setTextColor(HIGHLIGHT_TEXT_COLOR, HIGHLIGHT_COLOR); else M5Cardputer.Display.setTextColor(TEXT_COLOR); String itemText = sdCardMenuItems[i]; if (itemText == "Mount/Unmount SD") { itemText = isSdCardMounted ? "Unmount SD Card" : "Mount SD Card"; } M5Cardputer.Display.drawString(itemText, 20, 30 + i * 30); }
    M5Cardputer.Display.setTextColor(TEXT_COLOR);
}
#endif
#ifdef ENABLE_WIFI
void drawWifiSettingsMenu() {
    #ifdef DEBUG_MODE
    Serial.println("DEBUG: drawWifiSettingsMenu() called");
    #endif
    M5Cardputer.Display.fillScreen(BACKGROUND_COLOR); M5Cardputer.Display.setTextDatum(middle_left); M5Cardputer.Display.setFont(&fonts::Orbitron_Light_24); M5Cardputer.Display.setTextSize(menuTextSize);
    for (int i = 0; i < numWifiMenuItems; i++) { if (i == currentWifiSelection) M5Cardputer.Display.setTextColor(HIGHLIGHT_TEXT_COLOR, HIGHLIGHT_COLOR); else M5Cardputer.Display.setTextColor(TEXT_COLOR); M5Cardputer.Display.drawString(wifiMenuItems[i], 20, 30 + i * 30); }
    M5Cardputer.Display.setTextColor(TEXT_COLOR);
}
void drawWifiScanResults() {
    #ifdef DEBUG_MODE
    Serial.println("DEBUG: drawWifiScanResults() called");
    #endif
    M5Cardputer.Display.fillScreen(BACKGROUND_COLOR); M5Cardputer.Display.setTextDatum(middle_left); M5Cardputer.Display.setFont(&fonts::Orbitron_Light_24); M5Cardputer.Display.setTextSize(menuTextSize);
    M5Cardputer.Display.drawString("Select a Network:", 10, 5);
    for (size_t i = 0; i < scanned_networks.size(); i++) { if (i == selected_network_index) M5Cardputer.Display.setTextColor(HIGHLIGHT_TEXT_COLOR, HIGHLIGHT_COLOR); else M5Cardputer.Display.setTextColor(TEXT_COLOR); M5Cardputer.Display.drawString(scanned_networks[i], 20, 30 + i * 25); }
    M5Cardputer.Display.setTextColor(TEXT_COLOR);
}
void drawPasswordInputScreen() {
    #ifdef DEBUG_MODE
    Serial.println("DEBUG: drawPasswordInputScreen() called");
    #endif
    M5Cardputer.Display.fillScreen(BACKGROUND_COLOR); M5Cardputer.Display.setTextDatum(top_left); M5Cardputer.Display.setFont(&fonts::Orbitron_Light_24); M5Cardputer.Display.setTextSize(menuTextSize);
    M5Cardputer.Display.drawString("Password for:", 10, 20); M5Cardputer.Display.drawString(selected_ssid, 10, 50); String stars = ""; for (size_t i = 0; i < password_buffer.length(); i++) { stars += "*"; }
    M5Cardputer.Display.drawString("> " + stars + "_", 10, 80);
}
#endif
void drawFactoryResetConfirmScreen() {
    #ifdef DEBUG_MODE
    Serial.println("DEBUG: drawFactoryResetConfirmScreen() called");
    #endif
    M5Cardputer.Display.fillScreen(BACKGROUND_COLOR); M5Cardputer.Display.setTextDatum(top_center); M5Cardputer.Display.setFont(&fonts::Orbitron_Light_24); M5Cardputer.Display.setTextSize(menuTextSize);
    M5Cardputer.Display.setTextColor(WARNING_COLOR); M5Cardputer.Display.drawString("FACTORY RESET?", M5Cardputer.Display.width() / 2, 20); M5Cardputer.Display.drawString("ALL SAVED DATA WILL BE ERASED!", M5Cardputer.Display.width() / 2, 50);
    M5Cardputer.Display.setTextColor(WHITE); M5Cardputer.Display.drawString("Press 'Y' to confirm", M5Cardputer.Display.width() / 2, 90); M5Cardputer.Display.drawString("Any other key to cancel", M5Cardputer.Display.width() / 2, 115);
}
#ifdef ENABLE_WEB_SERVER
void drawWebServerScreen() {
    #ifdef DEBUG_MODE
    Serial.println("DEBUG: drawWebServerScreen() called");
    #endif
    M5Cardputer.Display.fillScreen(BACKGROUND_COLOR); M5Cardputer.Display.setTextDatum(top_center); M5Cardputer.Display.setFont(&fonts::Orbitron_Light_24); M5Cardputer.Display.setTextSize(menuTextSize);
    M5Cardputer.Display.setTextColor(TEXT_COLOR);
    #ifdef ENABLE_WIFI
    if (WiFi.status() == WL_CONNECTED) {
        M5Cardputer.Display.drawString("Web Server Active", M5Cardputer.Display.width() / 2, 20);
        M5Cardputer.Display.drawString("http://" + WiFi.localIP().toString(), M5Cardputer.Display.width() / 2, 50);
        M5Cardputer.Display.drawString("Press Enter to Stop", M5Cardputer.Display.width() / 2, 90);
    } else {
        M5Cardputer.Display.drawString("WiFi Disconnected", M5Cardputer.Display.width() / 2, 50);
         M5Cardputer.Display.drawString("Press Enter to Exit", M5Cardputer.Display.width() / 2, 90);
    }
    #endif
}
#endif
void displayMessage(String line1, String line2, int delay_ms) {
    #ifdef DEBUG_MODE
    Serial.println("DEBUG: displayMessage() called with line1: " + line1 + ", line2: " + line2);
    #endif
    M5Cardputer.Display.clear(); M5Cardputer.Display.setTextDatum(middle_center); M5Cardputer.Display.drawString(line1, M5Cardputer.Display.width() / 2, M5Cardputer.Display.height() / 2 - 15); if (line2 != "") { M5Cardputer.Display.drawString(line2, M5Cardputer.Display.width() / 2, M5Cardputer.Display.height() / 2 + 15); } delay(delay_ms);
}
#ifdef ENABLE_OTA
void drawOtaScreen() {
    #ifdef DEBUG_MODE
    Serial.println("DEBUG: drawOtaScreen() called");
    #endif
    M5Cardputer.Display.fillScreen(BACKGROUND_COLOR);
    M5Cardputer.Display.setTextDatum(top_center);
    M5Cardputer.Display.setFont(&fonts::Orbitron_Light_24);
    M5Cardputer.Display.setTextSize(menuTextSize);
    M5Cardputer.Display.setTextColor(TEXT_COLOR);

    #ifdef ENABLE_WIFI
    if (WiFi.status() == WL_CONNECTED) {
        M5Cardputer.Display.drawString("OTA Update Mode", M5Cardputer.Display.width() / 2, 20);
        M5Cardputer.Display.drawString("Host: M5Cardputer-OS", M5Cardputer.Display.width() / 2, 50);
        M5Cardputer.Display.drawString("IP: " + WiFi.localIP().toString(), M5Cardputer.Display.width() / 2, 75);
        M5Cardputer.Display.drawString("Press Enter to Exit", M5Cardputer.Display.width() / 2, 110);
    } else {
        M5Cardputer.Display.drawString("WiFi Disconnected", M5Cardputer.Display.width() / 2, 50);
        M5Cardputer.Display.drawString("Press Enter to Exit", M5Cardputer.Display.width() / 2, 90);
    }
    #endif
}
#endif
void drawKeyboardTestScreen() {
    #ifdef DEBUG_MODE
    Serial.println("DEBUG: drawKeyboardTestScreen() called");
    #endif
    M5Cardputer.Display.fillScreen(BACKGROUND_COLOR);
    M5Cardputer.Display.setTextDatum(top_center);
    M5Cardputer.Display.setFont(&fonts::Orbitron_Light_24);
    M5Cardputer.Display.setTextSize(1.0);
    M5Cardputer.Display.drawString("Keyboard Test", M5Cardputer.Display.width() / 2, 20);
    M5Cardputer.Display.drawString("Last Key Pressed:", M5Cardputer.Display.width() / 2, 50);
    
    M5Cardputer.Display.setTextSize(1.5);
    M5Cardputer.Display.drawString(lastKeyPressed, M5Cardputer.Display.width() / 2, 80);
    
    M5Cardputer.Display.setTextSize(0.8);
    M5Cardputer.Display.drawString("Press Del to exit", M5Cardputer.Display.width() / 2, 120);
}