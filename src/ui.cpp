#include "globals.h"
#include "ui.h"

SdFs sd;
Preferences preferences;
float menuTextSize = 0.8;
AppState currentState = STATE_MAIN_MENU;
bool isSdCardMounted = false;
std::vector<String> scanned_networks;
int selected_network_index = 0;
String selected_ssid = "";
String password_buffer = "";
String lastKeyPressed = "None";
const char* mainMenuItems[] = { "Apps", "Keyboard Test", "Option C", "Settings" }; // <-- EDIT THIS LINE
const int numMainMenuItems = sizeof(mainMenuItems) / sizeof(mainMenuItems[0]);
int currentMainMenuSelection = 0;
const char* settingsMenuItems[] = { "Display", "SD Card", "WiFi", "OTA Update", "Factory Reset", "Back" };
const int numSettingsMenuItems = sizeof(settingsMenuItems) / sizeof(settingsMenuItems[0]);
int currentSettingsSelection = 0;
const char* displayMenuItems[] = { "Text Size", "Back" };
const int numDisplayMenuItems = sizeof(displayMenuItems) / sizeof(displayMenuItems[0]);
int currentDisplaySelection = 0;
const char* sdCardMenuItems[] = { "SD Card Info", "Mount/Unmount SD", "Back" };
const int numSdCardMenuItems = sizeof(sdCardMenuItems) / sizeof(sdCardMenuItems[0]);
int currentSdCardSelection = 0;
const char* wifiMenuItems[] = { "Status", "Scan for Networks", "Web Server", "Disconnect", "Back" };
const int numWifiMenuItems = sizeof(wifiMenuItems) / sizeof(wifiMenuItems[0]);
int currentWifiSelection = 0;
std::vector<String> app_list;
int currentAppSelection = 0;

void drawScreen() {
    if (currentState == STATE_MAIN_MENU)             drawMainMenu();
    else if (currentState == STATE_APPS_MENU)        drawAppsMenu();
    else if (currentState == STATE_SETTINGS_MENU)    drawSettingsMenu();
    else if (currentState == STATE_DISPLAY_SETTINGS_MENU) drawDisplaySettingsMenu();
    else if (currentState == STATE_SDCARD_SETTINGS_MENU)  drawSdCardMenu();
    else if (currentState == STATE_WIFI_SETTINGS_MENU)   drawWifiSettingsMenu();
    else if (currentState == STATE_WIFI_SCAN_RESULTS)  drawWifiScanResults();
    else if (currentState == STATE_WIFI_PASSWORD_INPUT) drawPasswordInputScreen();
    else if (currentState == STATE_FACTORY_RESET_CONFIRM) drawFactoryResetConfirmScreen();
    else if (currentState == STATE_WEB_SERVER_ACTIVE)   drawWebServerScreen();
    else if (currentState == STATE_OTA_MODE)            drawOtaScreen();
    else if (currentState == STATE_KEYBOARD_TEST)    drawKeyboardTestScreen();
}

void drawBatteryStatus() {
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
    M5Cardputer.Display.fillScreen(BACKGROUND_COLOR);
    M5Cardputer.Display.setTextDatum(middle_left);
    M5Cardputer.Display.setFont(&fonts::Orbitron_Light_24);
    M5Cardputer.Display.setTextSize(menuTextSize);

    M5Cardputer.Display.drawString("Select App:", 20, 15);

    if (app_list.empty()) {
        M5Cardputer.Display.drawString("No apps found in /apps", 20, 60);
    } else {
        for (int i = 0; i < app_list.size(); i++) {
            if (i == currentAppSelection) M5Cardputer.Display.setTextColor(HIGHLIGHT_TEXT_COLOR, HIGHLIGHT_COLOR);
            else M5Cardputer.Display.setTextColor(TEXT_COLOR);
            M5Cardputer.Display.drawString(app_list[i], 20, 40 + i * 25);
        }
    }
    M5Cardputer.Display.setTextColor(TEXT_COLOR);
}

void drawSettingsMenu() {
    M5Cardputer.Display.fillScreen(BACKGROUND_COLOR); M5Cardputer.Display.setTextDatum(middle_left); M5Cardputer.Display.setFont(&fonts::Orbitron_Light_24); M5Cardputer.Display.setTextSize(menuTextSize);
    for (int i = 0; i < numSettingsMenuItems; i++) { if (i == currentSettingsSelection) M5Cardputer.Display.setTextColor(HIGHLIGHT_TEXT_COLOR, HIGHLIGHT_COLOR); else M5Cardputer.Display.setTextColor(TEXT_COLOR); M5Cardputer.Display.drawString(settingsMenuItems[i], 20, 30 + i * 30); }
    M5Cardputer.Display.setTextColor(TEXT_COLOR);
}
void drawDisplaySettingsMenu() {
    M5Cardputer.Display.fillScreen(BACKGROUND_COLOR); M5Cardputer.Display.setTextDatum(middle_left); M5Cardputer.Display.setFont(&fonts::Orbitron_Light_24); M5Cardputer.Display.setTextSize(menuTextSize);
    for (int i = 0; i < numDisplayMenuItems; i++) { if (i == currentDisplaySelection) M5Cardputer.Display.setTextColor(HIGHLIGHT_TEXT_COLOR, HIGHLIGHT_COLOR); else M5Cardputer.Display.setTextColor(TEXT_COLOR); String itemText = displayMenuItems[i]; if (itemText == "Text Size") { itemText = String("Text Size: < ") + String(menuTextSize, 1) + String(" >"); } M5Cardputer.Display.drawString(itemText, 20, 30 + i * 30); }
    M5Cardputer.Display.setTextColor(TEXT_COLOR);
}
void drawSdCardMenu() {
    M5Cardputer.Display.fillScreen(BACKGROUND_COLOR); M5Cardputer.Display.setTextDatum(middle_left); M5Cardputer.Display.setFont(&fonts::Orbitron_Light_24); M5Cardputer.Display.setTextSize(menuTextSize);
    for (int i = 0; i < numSdCardMenuItems; i++) { if (i == currentSdCardSelection) M5Cardputer.Display.setTextColor(HIGHLIGHT_TEXT_COLOR, HIGHLIGHT_COLOR); else M5Cardputer.Display.setTextColor(TEXT_COLOR); String itemText = sdCardMenuItems[i]; if (itemText == "Mount/Unmount SD") { itemText = isSdCardMounted ? "Unmount SD Card" : "Mount SD Card"; } M5Cardputer.Display.drawString(itemText, 20, 30 + i * 30); }
    M5Cardputer.Display.setTextColor(TEXT_COLOR);
}
void drawWifiSettingsMenu() {
    M5Cardputer.Display.fillScreen(BACKGROUND_COLOR); M5Cardputer.Display.setTextDatum(middle_left); M5Cardputer.Display.setFont(&fonts::Orbitron_Light_24); M5Cardputer.Display.setTextSize(menuTextSize);
    for (int i = 0; i < numWifiMenuItems; i++) { if (i == currentWifiSelection) M5Cardputer.Display.setTextColor(HIGHLIGHT_TEXT_COLOR, HIGHLIGHT_COLOR); else M5Cardputer.Display.setTextColor(TEXT_COLOR); M5Cardputer.Display.drawString(wifiMenuItems[i], 20, 30 + i * 30); }
    M5Cardputer.Display.setTextColor(TEXT_COLOR);
}
void drawWifiScanResults() {
    M5Cardputer.Display.fillScreen(BACKGROUND_COLOR); M5Cardputer.Display.setTextDatum(middle_left); M5Cardputer.Display.setFont(&fonts::Orbitron_Light_24); M5Cardputer.Display.setTextSize(menuTextSize);
    M5Cardputer.Display.drawString("Select a Network:", 10, 5);
    for (int i = 0; i < scanned_networks.size(); i++) { if (i == selected_network_index) M5Cardputer.Display.setTextColor(HIGHLIGHT_TEXT_COLOR, HIGHLIGHT_COLOR); else M5Cardputer.Display.setTextColor(TEXT_COLOR); M5Cardputer.Display.drawString(scanned_networks[i], 20, 30 + i * 25); }
    M5Cardputer.Display.setTextColor(TEXT_COLOR);
}
void drawPasswordInputScreen() {
    M5Cardputer.Display.fillScreen(BACKGROUND_COLOR); M5Cardputer.Display.setTextDatum(top_left); M5Cardputer.Display.setFont(&fonts::Orbitron_Light_24); M5Cardputer.Display.setTextSize(menuTextSize);
    M5Cardputer.Display.drawString("Password for:", 10, 20); M5Cardputer.Display.drawString(selected_ssid, 10, 50); String stars = ""; for (int i = 0; i < password_buffer.length(); i++) { stars += "*"; }
    M5Cardputer.Display.drawString("> " + stars + "_", 10, 80);
}
void drawFactoryResetConfirmScreen() {
    M5Cardputer.Display.fillScreen(BACKGROUND_COLOR); M5Cardputer.Display.setTextDatum(top_center); M5Cardputer.Display.setFont(&fonts::Orbitron_Light_24); M5Cardputer.Display.setTextSize(menuTextSize);
    M5Cardputer.Display.setTextColor(WARNING_COLOR); M5Cardputer.Display.drawString("FACTORY RESET?", M5Cardputer.Display.width() / 2, 20); M5Cardputer.Display.drawString("ALL SAVED DATA WILL BE ERASED!", M5Cardputer.Display.width() / 2, 50);
    M5Cardputer.Display.setTextColor(WHITE); M5Cardputer.Display.drawString("Press 'Y' to confirm", M5Cardputer.Display.width() / 2, 90); M5Cardputer.Display.drawString("Any other key to cancel", M5Cardputer.Display.width() / 2, 115);
}
void drawWebServerScreen() {
    M5Cardputer.Display.fillScreen(BACKGROUND_COLOR); M5Cardputer.Display.setTextDatum(top_center); M5Cardputer.Display.setFont(&fonts::Orbitron_Light_24); M5Cardputer.Display.setTextSize(menuTextSize);
    M5Cardputer.Display.setTextColor(TEXT_COLOR);
    if (WiFi.status() == WL_CONNECTED) {
        M5Cardputer.Display.drawString("Web Server Active", M5Cardputer.Display.width() / 2, 20);
        M5Cardputer.Display.drawString("http://" + WiFi.localIP().toString(), M5Cardputer.Display.width() / 2, 50);
        M5Cardputer.Display.drawString("Press Enter to Stop", M5Cardputer.Display.width() / 2, 90);
    } else {
        M5Cardputer.Display.drawString("WiFi Disconnected", M5Cardputer.Display.width() / 2, 50);
         M5Cardputer.Display.drawString("Press Enter to Exit", M5Cardputer.Display.width() / 2, 90);
    }
}
void displayMessage(String line1, String line2, int delay_ms) {
    M5Cardputer.Display.clear(); M5Cardputer.Display.setTextDatum(middle_center); M5Cardputer.Display.drawString(line1, M5Cardputer.Display.width() / 2, M5Cardputer.Display.height() / 2 - 15); if (line2 != "") { M5Cardputer.Display.drawString(line2, M5Cardputer.Display.width() / 2, M5Cardputer.Display.height() / 2 + 15); } delay(delay_ms);
}
void drawOtaScreen() {
    M5Cardputer.Display.fillScreen(BACKGROUND_COLOR);
    M5Cardputer.Display.setTextDatum(top_center);
    M5Cardputer.Display.setFont(&fonts::Orbitron_Light_24);
    M5Cardputer.Display.setTextSize(menuTextSize);
    M5Cardputer.Display.setTextColor(TEXT_COLOR);

    if (WiFi.status() == WL_CONNECTED) {
        M5Cardputer.Display.drawString("OTA Update Mode", M5Cardputer.Display.width() / 2, 20);
        M5Cardputer.Display.drawString("Host: M5Cardputer-OS", M5Cardputer.Display.width() / 2, 50);
        M5Cardputer.Display.drawString("IP: " + WiFi.localIP().toString(), M5Cardputer.Display.width() / 2, 75);
        M5Cardputer.Display.drawString("Press Enter to Exit", M5Cardputer.Display.width() / 2, 110);
    } else {
        M5Cardputer.Display.drawString("WiFi Disconnected", M5Cardputer.Display.width() / 2, 50);
        M5Cardputer.Display.drawString("Press Enter to Exit", M5Cardputer.Display.width() / 2, 90);
    }
}
void drawKeyboardTestScreen() {
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