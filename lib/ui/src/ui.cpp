#include <M5CardputerOS_core.h>
#include <vector>
#define COLOR_BLUE 0x001F
#define COLOR_RED 0xF800
#define COLOR_GREEN 0x07E0
#define COLOR_CYAN 0x07FF
#define COLOR_YELLOW 0xFFE0

void drawStartupScreen(String serialStatus, String sdStatus, String wifiStatus, String ip, bool showWelcome) {
    M5Cardputer.Display.fillScreen(BACKGROUND_COLOR);
    M5Cardputer.Display.setFont(&fonts::Orbitron_Light_24);
    M5Cardputer.Display.setTextSize(0.5f);
    M5Cardputer.Display.setTextDatum(top_left);
    int y = 20;
    int lineSpacing = 18;
    int statusX = 140;

    auto statusColor = [](const String& status) {
        if (status == "[OK]" || status == "OK" || status == "[ok]" || status == "ok" || status == "connected" || status == "mounted") return COLOR_GREEN;
        if (status == "[FAIL]" || status == "FAIL" || status == "error" || status == "[ERROR]") return COLOR_RED;
        return COLOR_YELLOW;
    };
    M5Cardputer.Display.setTextColor(COLOR_BLUE, BACKGROUND_COLOR);
    M5Cardputer.Display.drawString("M5Cardputer-OS Booting...", 10, y); y += lineSpacing;
    M5Cardputer.Display.setTextColor(COLOR_RED, BACKGROUND_COLOR);
    M5Cardputer.Display.drawString("Loading Serial...", 10, y);
    M5Cardputer.Display.setTextColor(statusColor(serialStatus), BACKGROUND_COLOR);
    M5Cardputer.Display.drawString(serialStatus, statusX, y); y += lineSpacing;
    M5Cardputer.Display.setTextColor(COLOR_RED, BACKGROUND_COLOR);
    M5Cardputer.Display.drawString("SD Card Status...", 10, y);
    M5Cardputer.Display.setTextColor(statusColor(sdStatus), BACKGROUND_COLOR);
    M5Cardputer.Display.drawString(sdStatus, statusX, y); y += lineSpacing;
    M5Cardputer.Display.setTextColor(COLOR_RED, BACKGROUND_COLOR);
    M5Cardputer.Display.drawString("WiFi Status...", 10, y);
    M5Cardputer.Display.setTextColor(statusColor(wifiStatus), BACKGROUND_COLOR);
    M5Cardputer.Display.drawString(wifiStatus, statusX, y); y += lineSpacing;
    M5Cardputer.Display.setTextColor(COLOR_RED, BACKGROUND_COLOR);
    M5Cardputer.Display.drawString("IP address:", 10, y);
    M5Cardputer.Display.setTextColor(ip.length() > 3 ? COLOR_GREEN : COLOR_YELLOW, BACKGROUND_COLOR);
    M5Cardputer.Display.drawString(ip, statusX, y); y += lineSpacing;
    M5Cardputer.Display.setTextColor(COLOR_RED, BACKGROUND_COLOR);
    M5Cardputer.Display.drawString("Loading Main Menu", 10, y); y += lineSpacing;

// WELCOME is now shown on its own screen
}
// Menu scroll offsets
int mainMenuScrollOffset = 0;
int appsMenuScrollOffset = 0;
int settingsMenuScrollOffset = 0;
int displayMenuScrollOffset = 0;
#ifdef ENABLE_SD_CARD
int sdCardMenuScrollOffset = 0;
#endif
#ifdef ENABLE_WIFI
int wifiMenuScrollOffset = 0;
int wifiScanScrollOffset = 0;
#endif

const int MENU_VISIBLE_COUNT = 5;
const int MENU_LINE_HEIGHT = 22;
const float MENU_FONT_SIZE = 0.8;

#include <M5CardputerOS_core.h>
#include "ui.h"




// --- NEW FUNCTION TO BUILD MENUS ---
void initializeMenus() {
    debugMessage("DEBUG:", "initializeMenus() called");
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
    debugMessage("DEBUG:", "drawScreen() called");
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
    int percentage = M5.Power.getBatteryLevel();
    float voltage = M5.Power.getBatteryVoltage() / 1000.0;
    String status = String(percentage) + "% " + String(voltage, 2) + "V";
    M5Cardputer.Display.setTextDatum(top_right);
    M5Cardputer.Display.setFont(&fonts::Orbitron_Light_24);
    M5Cardputer.Display.setTextSize(0.7);
    M5Cardputer.Display.setTextSize(menuTextSize);
    M5Cardputer.Display.setTextColor(TEXT_COLOR, BACKGROUND_COLOR);
    M5Cardputer.Display.drawString(status, M5Cardputer.Display.width() - 10, 5);
}

void drawMainMenu() {
    #ifdef DEBUG_MODE
    debugMessage("DEBUG:", "drawMainMenu() called");
    #endif
    M5Cardputer.Display.fillScreen(BACKGROUND_COLOR);
    drawBatteryStatus(); 
    
    M5Cardputer.Display.setTextDatum(middle_left);
    M5Cardputer.Display.setFont(&fonts::Orbitron_Light_24);
    M5Cardputer.Display.setTextSize(MENU_FONT_SIZE);
    M5Cardputer.Display.setTextSize(menuTextSize);
    // Scroll logic
    if (currentMainMenuSelection < mainMenuScrollOffset) mainMenuScrollOffset = currentMainMenuSelection;
    if (currentMainMenuSelection >= mainMenuScrollOffset + MENU_VISIBLE_COUNT) mainMenuScrollOffset = currentMainMenuSelection - MENU_VISIBLE_COUNT + 1;
    for (int i = mainMenuScrollOffset; i < numMainMenuItems && i < mainMenuScrollOffset + MENU_VISIBLE_COUNT; i++) {
        int y = 30 + (i - mainMenuScrollOffset) * MENU_LINE_HEIGHT;
        if (i == currentMainMenuSelection) M5Cardputer.Display.setTextColor(HIGHLIGHT_TEXT_COLOR, HIGHLIGHT_COLOR);
        else M5Cardputer.Display.setTextColor(TEXT_COLOR);
        M5Cardputer.Display.drawString(mainMenuItems[i], 20, y);
    }
    M5Cardputer.Display.setTextColor(TEXT_COLOR);
}

void drawAppsMenu() {
    #ifdef DEBUG_MODE
    debugMessage("DEBUG:", "drawAppsMenu() called");
    #endif
    M5Cardputer.Display.fillScreen(BACKGROUND_COLOR);
    M5Cardputer.Display.setTextDatum(middle_left);
    M5Cardputer.Display.setFont(&fonts::Orbitron_Light_24);
    M5Cardputer.Display.setTextSize(MENU_FONT_SIZE);
    M5Cardputer.Display.setTextSize(menuTextSize);
    M5Cardputer.Display.drawString("Select App:", 20, 15);
    if (app_list.empty()) {
        M5Cardputer.Display.drawString("No apps found in /apps", 20, 60);
    } else {
        if (currentAppSelection < appsMenuScrollOffset) appsMenuScrollOffset = currentAppSelection;
        if (currentAppSelection >= appsMenuScrollOffset + MENU_VISIBLE_COUNT) appsMenuScrollOffset = currentAppSelection - MENU_VISIBLE_COUNT + 1;
        for (size_t i = appsMenuScrollOffset; i < app_list.size() && i < appsMenuScrollOffset + MENU_VISIBLE_COUNT; i++) {
            int y = 40 + (i - appsMenuScrollOffset) * MENU_LINE_HEIGHT;
            if (i == currentAppSelection) M5Cardputer.Display.setTextColor(HIGHLIGHT_TEXT_COLOR, HIGHLIGHT_COLOR);
            else M5Cardputer.Display.setTextColor(TEXT_COLOR);
            M5Cardputer.Display.drawString(app_list[i], 20, y);
        }
    }
    M5Cardputer.Display.setTextColor(TEXT_COLOR);
}

void drawSettingsMenu() {
    #ifdef DEBUG_MODE
    debugMessage("DEBUG:", "drawSettingsMenu() called");
    #endif
    M5Cardputer.Display.fillScreen(BACKGROUND_COLOR);
    M5Cardputer.Display.setTextDatum(middle_left);
    M5Cardputer.Display.setFont(&fonts::Orbitron_Light_24);
    M5Cardputer.Display.setTextSize(MENU_FONT_SIZE);
    M5Cardputer.Display.setTextSize(menuTextSize);
    if (currentSettingsSelection < settingsMenuScrollOffset) settingsMenuScrollOffset = currentSettingsSelection;
    if (currentSettingsSelection >= settingsMenuScrollOffset + MENU_VISIBLE_COUNT) settingsMenuScrollOffset = currentSettingsSelection - MENU_VISIBLE_COUNT + 1;
    for (int i = settingsMenuScrollOffset; i < numSettingsMenuItems && i < settingsMenuScrollOffset + MENU_VISIBLE_COUNT; i++) {
        int y = 30 + (i - settingsMenuScrollOffset) * MENU_LINE_HEIGHT;
        if (i == currentSettingsSelection) M5Cardputer.Display.setTextColor(HIGHLIGHT_TEXT_COLOR, HIGHLIGHT_COLOR);
        else M5Cardputer.Display.setTextColor(TEXT_COLOR);
        M5Cardputer.Display.drawString(settingsMenuItems[i], 20, y);
    }
    M5Cardputer.Display.setTextColor(TEXT_COLOR);
}
void drawDisplaySettingsMenu() {
    #ifdef DEBUG_MODE
    debugMessage("DEBUG:", "drawDisplaySettingsMenu() called");
    #endif
    M5Cardputer.Display.fillScreen(BACKGROUND_COLOR);
    M5Cardputer.Display.setTextDatum(middle_left);
    M5Cardputer.Display.setFont(&fonts::Orbitron_Light_24);
    M5Cardputer.Display.setTextSize(MENU_FONT_SIZE);
    M5Cardputer.Display.setTextSize(menuTextSize);
    if (currentDisplaySelection < displayMenuScrollOffset) displayMenuScrollOffset = currentDisplaySelection;
    if (currentDisplaySelection >= displayMenuScrollOffset + MENU_VISIBLE_COUNT) displayMenuScrollOffset = currentDisplaySelection - MENU_VISIBLE_COUNT + 1;
    for (int i = displayMenuScrollOffset; i < numDisplayMenuItems && i < displayMenuScrollOffset + MENU_VISIBLE_COUNT; i++) {
        int y = 30 + (i - displayMenuScrollOffset) * MENU_LINE_HEIGHT;
        String itemText = displayMenuItems[i];
        if (itemText == "Text Size") {
            itemText = String("Text Size: < ") + String(menuTextSize, 1) + String(" >");
        }
        if (i == currentDisplaySelection) M5Cardputer.Display.setTextColor(HIGHLIGHT_TEXT_COLOR, HIGHLIGHT_COLOR);
        else M5Cardputer.Display.setTextColor(TEXT_COLOR);
        M5Cardputer.Display.drawString(itemText, 20, y);
    }
    M5Cardputer.Display.setTextColor(TEXT_COLOR);
}
#ifdef ENABLE_SD_CARD
void drawSdCardMenu() {
    #ifdef DEBUG_MODE
    debugMessage("DEBUG:", "drawSdCardMenu() called");
    #endif
    M5Cardputer.Display.fillScreen(BACKGROUND_COLOR);
    M5Cardputer.Display.setTextDatum(middle_left);
    M5Cardputer.Display.setFont(&fonts::Orbitron_Light_24);
    M5Cardputer.Display.setTextSize(MENU_FONT_SIZE);
    M5Cardputer.Display.setTextSize(menuTextSize);
    if (currentSdCardSelection < sdCardMenuScrollOffset) sdCardMenuScrollOffset = currentSdCardSelection;
    if (currentSdCardSelection >= sdCardMenuScrollOffset + MENU_VISIBLE_COUNT) sdCardMenuScrollOffset = currentSdCardSelection - MENU_VISIBLE_COUNT + 1;
    for (int i = sdCardMenuScrollOffset; i < numSdCardMenuItems && i < sdCardMenuScrollOffset + MENU_VISIBLE_COUNT; i++) {
        int y = 30 + (i - sdCardMenuScrollOffset) * MENU_LINE_HEIGHT;
        String itemText = sdCardMenuItems[i];
        if (itemText == "Mount/Unmount SD") {
            itemText = isSdCardMounted ? "Unmount SD Card" : "Mount SD Card";
        }
        if (i == currentSdCardSelection) M5Cardputer.Display.setTextColor(HIGHLIGHT_TEXT_COLOR, HIGHLIGHT_COLOR);
        else M5Cardputer.Display.setTextColor(TEXT_COLOR);
        M5Cardputer.Display.drawString(itemText, 20, y);
    }
    M5Cardputer.Display.setTextColor(TEXT_COLOR);
}
#endif
#ifdef ENABLE_WIFI
void drawWifiSettingsMenu() {
    #ifdef DEBUG_MODE
    debugMessage("DEBUG:", "drawWifiSettingsMenu() called");
    #endif
    M5Cardputer.Display.fillScreen(BACKGROUND_COLOR);
    M5Cardputer.Display.setTextDatum(middle_left);
    M5Cardputer.Display.setFont(&fonts::Orbitron_Light_24);
    M5Cardputer.Display.setTextSize(MENU_FONT_SIZE);
    M5Cardputer.Display.setTextSize(menuTextSize);
    if (currentWifiSelection < wifiMenuScrollOffset) wifiMenuScrollOffset = currentWifiSelection;
    if (currentWifiSelection >= wifiMenuScrollOffset + MENU_VISIBLE_COUNT) wifiMenuScrollOffset = currentWifiSelection - MENU_VISIBLE_COUNT + 1;
    for (int i = wifiMenuScrollOffset; i < numWifiMenuItems && i < wifiMenuScrollOffset + MENU_VISIBLE_COUNT; i++) {
        int y = 30 + (i - wifiMenuScrollOffset) * MENU_LINE_HEIGHT;
        if (i == currentWifiSelection) M5Cardputer.Display.setTextColor(HIGHLIGHT_TEXT_COLOR, HIGHLIGHT_COLOR);
        else M5Cardputer.Display.setTextColor(TEXT_COLOR);
        M5Cardputer.Display.drawString(wifiMenuItems[i], 20, y);
    }
    M5Cardputer.Display.setTextColor(TEXT_COLOR);
}
void drawWifiScanResults() {
    #ifdef DEBUG_MODE
    debugMessage("DEBUG:", "drawWifiScanResults() called");
    #endif
    M5Cardputer.Display.fillScreen(BACKGROUND_COLOR);
    M5Cardputer.Display.setTextDatum(middle_left);
    M5Cardputer.Display.setFont(&fonts::Orbitron_Light_24);
    M5Cardputer.Display.setTextSize(MENU_FONT_SIZE);
    M5Cardputer.Display.setTextSize(menuTextSize);
    M5Cardputer.Display.drawString("Select a Network:", 10, 5);
    if (selected_network_index < wifiScanScrollOffset) wifiScanScrollOffset = selected_network_index;
    if (selected_network_index >= wifiScanScrollOffset + MENU_VISIBLE_COUNT) wifiScanScrollOffset = selected_network_index - MENU_VISIBLE_COUNT + 1;
    for (size_t i = wifiScanScrollOffset; i < scanned_networks.size() && i < wifiScanScrollOffset + MENU_VISIBLE_COUNT; i++) {
        int y = 30 + (i - wifiScanScrollOffset) * MENU_LINE_HEIGHT;
        if (i == selected_network_index) M5Cardputer.Display.setTextColor(HIGHLIGHT_TEXT_COLOR, HIGHLIGHT_COLOR);
        else M5Cardputer.Display.setTextColor(TEXT_COLOR);
        M5Cardputer.Display.drawString(scanned_networks[i], 20, y);
    }
    M5Cardputer.Display.setTextColor(TEXT_COLOR);
}
void drawPasswordInputScreen() {
    #ifdef DEBUG_MODE
    debugMessage("DEBUG:", "drawPasswordInputScreen() called");
    #endif
    M5Cardputer.Display.fillScreen(BACKGROUND_COLOR); M5Cardputer.Display.setTextDatum(top_left); M5Cardputer.Display.setFont(&fonts::Orbitron_Light_24); M5Cardputer.Display.setTextSize(menuTextSize);
    M5Cardputer.Display.drawString("Password for:", 10, 20); M5Cardputer.Display.drawString(selected_ssid, 10, 50); String stars = ""; for (size_t i = 0; i < password_buffer.length(); i++) { stars += "*"; }
    M5Cardputer.Display.drawString("> " + stars + "_", 10, 80);
}
#endif
void drawFactoryResetConfirmScreen() {
    #ifdef DEBUG_MODE
    debugMessage("DEBUG:", "drawFactoryResetConfirmScreen() called");
    #endif
    M5Cardputer.Display.fillScreen(BACKGROUND_COLOR); M5Cardputer.Display.setTextDatum(top_center); M5Cardputer.Display.setFont(&fonts::Orbitron_Light_24); M5Cardputer.Display.setTextSize(menuTextSize);
    M5Cardputer.Display.setTextColor(WARNING_COLOR); M5Cardputer.Display.drawString("FACTORY RESET?", M5Cardputer.Display.width() / 2, 20); M5Cardputer.Display.drawString("ALL SAVED DATA WILL BE ERASED!", M5Cardputer.Display.width() / 2, 50);
    M5Cardputer.Display.setTextColor(WHITE); M5Cardputer.Display.drawString("Press 'Y' to confirm", M5Cardputer.Display.width() / 2, 90); M5Cardputer.Display.drawString("Any other key to cancel", M5Cardputer.Display.width() / 2, 115);
}
#ifdef ENABLE_WEB_SERVER
void drawWebServerScreen() {
    #ifdef DEBUG_MODE
    debugMessage("DEBUG:", "drawWebServerScreen() called");
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
    M5Cardputer.Display.clear();
    M5Cardputer.Display.setTextDatum(middle_center);
    M5Cardputer.Display.drawString(line1, M5Cardputer.Display.width() / 2, M5Cardputer.Display.height() / 2 - 15);
    if (line2 != "") {
        M5Cardputer.Display.drawString(line2, M5Cardputer.Display.width() / 2, M5Cardputer.Display.height() / 2 + 15);
    }
    delay(delay_ms);
}

void debugMessage(String line1, String line2) {
#ifdef DEBUG_MODE
    Serial.println("DEBUG: " + line1 + (line2.length() ? (" " + line2) : ""));
#endif
}
#ifdef ENABLE_OTA
void drawOtaScreen() {
    #ifdef DEBUG_MODE
    debugMessage("DEBUG:", "drawOtaScreen() called");
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
    debugMessage("DEBUG:", "drawKeyboardTestScreen() called");
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