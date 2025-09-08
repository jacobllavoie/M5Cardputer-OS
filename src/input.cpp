#include "globals.h"
#include "ui.h"
#include "input.h"

// --- Conditional Includes ---
#ifdef ENABLE_WIFI
#include "wifi.h"
#endif
#ifdef ENABLE_SD_CARD
#include "sd_card.h"
#endif
#ifdef ENABLE_WEB_SERVER
#include "web_server.h"
#endif
#ifdef ENABLE_OTA
#include "ota.h"
#endif

void factoryReset() {
    displayMessage("Resetting...", "Erasing all settings.", 1500);
    #ifdef ENABLE_WIFI
    preferences.begin("wifi-creds", false);
    preferences.clear();
    preferences.end();
    #endif
    displayMessage("Reset Complete.", "Rebooting...", 2000);
    ESP.restart();
}

#ifdef ENABLE_SD_CARD
void loadApp(String appName) {
    String appPath = "/apps/" + appName;
    if (!sd.exists(appPath.c_str())) {
        displayMessage("Error:", "App file not found!");
        return;
    }

    FsFile appFile = sd.open(appPath.c_str(), O_RDONLY);
    if (!appFile) {
        displayMessage("Error:", "Failed to open app!");
        return;
    }

    size_t appSize = appFile.size();
    if (appSize == 0) {
        displayMessage("Error:", "App file is empty!");
        appFile.close();
        return;
    }

    displayMessage("Loading App:", appName, 1000);
    
    const esp_partition_t* update_partition = esp_ota_get_next_update_partition(NULL);
    if (update_partition == NULL) {
        displayMessage("Error:", "No OTA partition!");
        appFile.close();
        return;
    }
    
    Serial.printf("Flashing to partition: %s\n", update_partition->label);

    if (!Update.begin(appSize, 0, -1, -1, update_partition->label)) {
        displayMessage("Update.begin failed!", "Error: " + String(Update.getError()));
        appFile.close();
        return;
    }
    
    uint8_t buffer[4096];
    size_t bytesRead = 0;
    while ((bytesRead = appFile.read(buffer, sizeof(buffer))) > 0) {
        if (Update.write(buffer, bytesRead) != bytesRead) {
            displayMessage("Flash write failed!", "Error: " + String(Update.getError()));
            appFile.close();
            Update.abort();
            return;
        }
        yield(); 
    }
    appFile.close();

    if (!Update.end()) {
        displayMessage("Update.end failed!", "Error: " + String(Update.getError()));
        return;
    }
    
    if (esp_ota_set_boot_partition(update_partition) != ESP_OK) {
        displayMessage("Error:", "Cannot set boot part!");
        return;
    }

    displayMessage("Load complete.", "Rebooting into app...");
    ESP.restart();
}
#endif

void handleInput() {
    #ifdef ENABLE_WIFI
    if (currentState == STATE_WIFI_PASSWORD_INPUT) {
        handlePasswordInput();
        return;
    }
    #endif

    if (M5Cardputer.Keyboard.isChange() && M5Cardputer.Keyboard.isPressed()) {
        bool stateHandled = false;
        #ifdef ENABLE_WEB_SERVER
        if (currentState == STATE_WEB_SERVER_ACTIVE) {
             Keyboard_Class::KeysState status = M5Cardputer.Keyboard.keysState();
             if (status.enter) {
                 stopWebServer();
                 currentState = STATE_WIFI_SETTINGS_MENU;
                 drawScreen();
             }
             stateHandled = true;
        }
        #endif
        #ifdef ENABLE_OTA
        if (currentState == STATE_OTA_MODE) {
             Keyboard_Class::KeysState status = M5Cardputer.Keyboard.keysState();
             if (status.enter) {
                 stopOTA();
                 currentState = STATE_SETTINGS_MENU;
                 drawScreen();
             }
             stateHandled = true;
        }
        #endif

        if (!stateHandled) {
            if (currentState == STATE_MAIN_MENU) handleMainMenuInput();
            else if (currentState == STATE_KEYBOARD_TEST) handleKeyboardTestInput();
            else if (currentState == STATE_APPS_MENU) handleAppsMenuInput();
            else if (currentState == STATE_SETTINGS_MENU) handleSettingsMenuInput();
            else if (currentState == STATE_DISPLAY_SETTINGS_MENU) handleDisplaySettingsInput();
            else if (currentState == STATE_FACTORY_RESET_CONFIRM) handleFactoryResetConfirmInput();
            #ifdef ENABLE_SD_CARD
            else if (currentState == STATE_SDCARD_SETTINGS_MENU) handleSdCardMenuInput();
            #endif
            #ifdef ENABLE_WIFI
            else if (currentState == STATE_WIFI_SETTINGS_MENU) handleWifiSettingsInput();
            else if (currentState == STATE_WIFI_SCAN_RESULTS) handleWifiScanResultsInput();
            #endif
        }
    }
}


void handleMainMenuInput() {
    Keyboard_Class::KeysState status = M5Cardputer.Keyboard.keysState();
    if (M5Cardputer.Keyboard.isKeyPressed(';')) { currentMainMenuSelection = (currentMainMenuSelection - 1 + numMainMenuItems) % numMainMenuItems; }
    if (M5Cardputer.Keyboard.isKeyPressed('.')) { currentMainMenuSelection = (currentMainMenuSelection + 1) % numMainMenuItems; }
    if (status.enter) { 
        if (strcmp(mainMenuItems[currentMainMenuSelection], "Apps") == 0) {
            #ifdef ENABLE_SD_CARD
            app_list.clear();
            FsFile root = sd.open("/apps");
            if (root && root.isDirectory()) {
                FsFile file = root.openNextFile();
                char fileName[256];
                while(file) {
                    file.getName(fileName, sizeof(fileName));
                    if (String(fileName).endsWith(".bin")) {
                        app_list.push_back(fileName);
                    }
                    file.close();
                    file = root.openNextFile();
                }
            }
            root.close();
            currentState = STATE_APPS_MENU;
            #else
            displayMessage("SD Card support disabled.");
            #endif
        } 
        else if (strcmp(mainMenuItems[currentMainMenuSelection], "Keyboard Test") == 0) {
            lastKeyPressed = "None";
            currentState = STATE_KEYBOARD_TEST;
        } 
        else if (strcmp(mainMenuItems[currentMainMenuSelection], "Settings") == 0) { 
            currentState = STATE_SETTINGS_MENU; 
        } 
        else { 
            displayMessage("You selected:", mainMenuItems[currentMainMenuSelection]); 
        } 
    }
    drawScreen();
}

void handleAppsMenuInput() {
    Keyboard_Class::KeysState status = M5Cardputer.Keyboard.keysState();
    if (M5Cardputer.Keyboard.isKeyPressed(';')) { 
        if (!app_list.empty())
            currentAppSelection = (currentAppSelection - 1 + app_list.size()) % app_list.size();
    }
    if (M5Cardputer.Keyboard.isKeyPressed('.')) { 
        if (!app_list.empty())
            currentAppSelection = (currentAppSelection + 1) % app_list.size();
    }
    if (status.enter) {
        #ifdef ENABLE_SD_CARD
        if (!app_list.empty()) {
            loadApp(app_list[currentAppSelection]);
        } else {
            currentState = STATE_MAIN_MENU;
        }
        #else
        currentState = STATE_MAIN_MENU;
        #endif
    }
    if (status.del) {
        currentState = STATE_MAIN_MENU;
    }
    drawScreen();
}

void handleSettingsMenuInput() {
    Keyboard_Class::KeysState status = M5Cardputer.Keyboard.keysState();
    if (M5Cardputer.Keyboard.isKeyPressed(';')) { currentSettingsSelection = (currentSettingsSelection - 1 + numSettingsMenuItems) % numSettingsMenuItems; }
    if (M5Cardputer.Keyboard.isKeyPressed('.')) { currentSettingsSelection = (currentSettingsSelection + 1) % numSettingsMenuItems; }
    if (status.enter) { 
        const char* selected = settingsMenuItems[currentSettingsSelection]; 
        if (strcmp(selected, "Display") == 0) currentState = STATE_DISPLAY_SETTINGS_MENU; 
        #ifdef ENABLE_SD_CARD
        else if (strcmp(selected, "SD Card") == 0) currentState = STATE_SDCARD_SETTINGS_MENU; 
        #endif
        #ifdef ENABLE_WIFI
        else if (strcmp(selected, "WiFi") == 0) currentState = STATE_WIFI_SETTINGS_MENU; 
        #endif
        #ifdef ENABLE_OTA
        else if (strcmp(selected, "OTA Update") == 0) { 
            #ifdef ENABLE_WIFI
            if (WiFi.status() == WL_CONNECTED) { 
                currentState = STATE_OTA_MODE; 
                setupOTA(); 
            } else { 
                displayMessage("WiFi is not connected."); 
            }
            #else
            displayMessage("WiFi support disabled.");
            #endif
        }
        #endif
        else if (strcmp(selected, "Factory Reset") == 0) currentState = STATE_FACTORY_RESET_CONFIRM; 
        else if (strcmp(selected, "Back") == 0) currentState = STATE_MAIN_MENU; 
    }
    drawScreen();
}

void handleDisplaySettingsInput() {
    Keyboard_Class::KeysState status = M5Cardputer.Keyboard.keysState();
    bool sizeChanged = false;

    if (M5Cardputer.Keyboard.isKeyPressed(';')) { currentDisplaySelection = (currentDisplaySelection - 1 + numDisplayMenuItems) % numDisplayMenuItems; }
    if (M5Cardputer.Keyboard.isKeyPressed('.')) { currentDisplaySelection = (currentDisplaySelection + 1) % numDisplayMenuItems; }
    
    if (strcmp(displayMenuItems[currentDisplaySelection], "Text Size") == 0) { 
        if (M5Cardputer.Keyboard.isKeyPressed('a')) { 
            menuTextSize = max(0.5f, menuTextSize - 0.1f); 
            sizeChanged = true;
        } 
        if (M5Cardputer.Keyboard.isKeyPressed('d')) { 
            menuTextSize = min(2.0f, menuTextSize + 0.1f); 
            sizeChanged = true;
        } 
    }
    
    if (status.enter) { 
        if (strcmp(displayMenuItems[currentDisplaySelection], "Back") == 0) { 
            currentState = STATE_SETTINGS_MENU; 
        } 
    }

    if (sizeChanged) {
        preferences.begin("display-settings", false);
        preferences.putFloat("fontSize", menuTextSize);
        preferences.end();
    }

    drawScreen();
}

#ifdef ENABLE_SD_CARD
void handleSdCardMenuInput() {
    Keyboard_Class::KeysState status = M5Cardputer.Keyboard.keysState();
    if (M5Cardputer.Keyboard.isKeyPressed(';')) { currentSdCardSelection = (currentSdCardSelection - 1 + numSdCardMenuItems) % numSdCardMenuItems; }
    if (M5Cardputer.Keyboard.isKeyPressed('.')) { currentSdCardSelection = (currentSdCardSelection + 1) % numSdCardMenuItems; }
    if (status.enter) { 
        const char* selected = sdCardMenuItems[currentSdCardSelection]; 
        if (strcmp(selected, "SD Card Info") == 0) { showSDCardInfo(); } 
        else if (strcmp(selected, "Mount/Unmount SD") == 0) { if (isSdCardMounted) unmountSD(); else mountSD(); } 
        else if (strcmp(selected, "Back") == 0) { currentState = STATE_SETTINGS_MENU; } 
    }
    drawScreen();
}
#endif

#ifdef ENABLE_WIFI
void handleWifiSettingsInput() {
    Keyboard_Class::KeysState status = M5Cardputer.Keyboard.keysState();
    if (M5Cardputer.Keyboard.isKeyPressed(';')) { currentWifiSelection = (currentWifiSelection - 1 + numWifiMenuItems) % numWifiMenuItems; }
    if (M5Cardputer.Keyboard.isKeyPressed('.')) { currentWifiSelection = (currentWifiSelection + 1) % numWifiMenuItems; }
    if (status.enter) { 
        const char* selected = wifiMenuItems[currentWifiSelection]; 
        if (strcmp(selected, "Status") == 0) showWifiStatus(); 
        else if (strcmp(selected, "Scan for Networks") == 0) scanWifiNetworks(); 
        #ifdef ENABLE_WEB_SERVER
        else if (strcmp(selected, "Web Server") == 0) { 
            if (WiFi.status() == WL_CONNECTED) { 
                currentState = STATE_WEB_SERVER_ACTIVE; 
                startWebServer(); 
            } else { 
                displayMessage("WiFi is not connected."); 
            } 
        }
        #endif
        else if (strcmp(selected, "Disconnect") == 0) disconnectWifi(); 
        else if (strcmp(selected, "Back") == 0) currentState = STATE_SETTINGS_MENU; 
    }
    drawScreen();
}

void handleWifiScanResultsInput() {
    Keyboard_Class::KeysState status = M5Cardputer.Keyboard.keysState();
    if (M5Cardputer.Keyboard.isKeyPressed(';')) { 
        if (!scanned_networks.empty())
            selected_network_index = (selected_network_index - 1 + scanned_networks.size()) % scanned_networks.size(); 
    }
    if (M5Cardputer.Keyboard.isKeyPressed('.')) { 
        if (!scanned_networks.empty())
            selected_network_index = (selected_network_index + 1) % scanned_networks.size(); 
    }
    if (status.enter) { 
        if (!scanned_networks.empty()) {
            selected_ssid = scanned_networks[selected_network_index]; 
            password_buffer = ""; 
            currentState = STATE_WIFI_PASSWORD_INPUT; 
        }
    }
    drawScreen();
}

void handlePasswordInput() {
    if (M5Cardputer.Keyboard.isChange() && M5Cardputer.Keyboard.isPressed()) {
        Keyboard_Class::KeysState status = M5Cardputer.Keyboard.keysState();
        if (!status.word.empty()) { password_buffer += status.word[0]; }
        if (status.del && password_buffer.length() > 0) { password_buffer.remove(password_buffer.length() - 1); }
        if (status.enter) {
            displayMessage("Saving & Connecting...", "", 1500);
            preferences.begin("wifi-creds", false);
            preferences.putString("ssid", selected_ssid);
            preferences.putString("password", password_buffer);
            preferences.end();
            currentState = STATE_WIFI_SETTINGS_MENU;
            wifiAutoConnect();
            return;
        }
        drawPasswordInputScreen();
    }
}
#endif

void handleFactoryResetConfirmInput() {
    if (M5Cardputer.Keyboard.isChange() && M5Cardputer.Keyboard.isPressed()) {
        Keyboard_Class::KeysState status = M5Cardputer.Keyboard.keysState();
        char key = 0;
        if (!status.word.empty()) key = status.word[0];
        if (key == 'y' || key == 'Y') {
            factoryReset();
        }
        currentState = STATE_SETTINGS_MENU;
        drawScreen();
    }
}

void handleKeyboardTestInput() {
    Keyboard_Class::KeysState status = M5Cardputer.Keyboard.keysState();

    if (!status.word.empty()) {
        lastKeyPressed = "";
        for(char c : status.word) {
            lastKeyPressed += c;
        }
    } else if (status.enter) {
        lastKeyPressed = "Enter";
    } else if (status.del) {
        currentState = STATE_MAIN_MENU; 
    } else if (status.tab) {
        lastKeyPressed = "Tab";
    } else if (status.fn) {
        lastKeyPressed = "Fn";
    } else if (status.opt) {
        lastKeyPressed = "Opt";
    } else if (status.ctrl) {
        lastKeyPressed = "Ctrl";
    }
    
    drawScreen();
}