#ifdef ENABLE_WIFI
#include <M5CardputerOS_core.h>
#include <ui.h>
#include <ota.h>
#include "wifi.h"

#include <Preferences.h>

void saveWifiCredentials(const String& ssid, const String& password) {
    Preferences prefs;
    prefs.begin("wifi", false); // read/write
    prefs.putString("ssid", ssid);
    prefs.putString("pass", password);
    prefs.end();
}

void wifiAutoConnect(bool returnToWifiMenu) {
    debugMessage("DEBUG:", "wifiAutoConnect() called");
    // displayMessage("Checking WiFi...", "", 500);
    Preferences prefs;
    prefs.begin("wifi", true); // read-only
    String ssid = prefs.getString("ssid", "");
    String password = prefs.getString("pass", "");
    prefs.end();

    if (ssid.length() > 0) {
    // displayMessage("Connecting to:", ssid, 1000);
        WiFi.begin(ssid.c_str(), password.c_str());

        int try_count = 0;
        while (WiFi.status() != WL_CONNECTED && try_count < 20) {
            M5Cardputer.update();
            String dots = String((try_count % 4) + 1, '.');
            // displayMessage("Connecting to:", ssid + dots, 250);
            delay(250);
            try_count++;
        }

        if (WiFi.status() == WL_CONNECTED) {
            // displayMessage("Connected to:", ssid, 1000);
            if (returnToWifiMenu) {
                currentState = STATE_WIFI_SETTINGS_MENU;
                drawScreen();
            }
        } else {
            displayMessage("Connection Failed", "", 1500);
        }
    } else {
        displayMessage("No saved WiFi network.", "", 1500);
    }
}

void showWifiStatus() {
    debugMessage("DEBUG:", "showWifiStatus() called");
    if (WiFi.status() == WL_CONNECTED) {
    displayMessage("Connected to: " + WiFi.SSID(), "IP: " + WiFi.localIP().toString());
    } else {
        displayMessage("WiFi Disconnected");
    }
}

void scanWifiNetworks() {
    debugMessage("DEBUG:", "scanWifiNetworks() called");
    displayMessage("Scanning...", "", 100);
    debugMessage("DEBUG:", "Scanning for WiFi networks...");
    int n = WiFi.scanNetworks();
    scanned_networks.clear();
    
    if (n == 0) {
        displayMessage("No networks found");
    debugMessage("DEBUG:", "No networks found.");
    } else {
    debugMessage("DEBUG:", String(n) + " networks found:");
        for (int i = 0; i < n; ++i) {
            String ssid = WiFi.SSID(i);
            debugMessage("DEBUG:", String(i + 1) + ": " + ssid);
            if (ssid != "" && std::find(scanned_networks.begin(), scanned_networks.end(), ssid) == scanned_networks.end()) {
                scanned_networks.push_back(ssid);
            }
        }
        currentState = STATE_WIFI_SCAN_RESULTS;
        selected_network_index = 0;
        drawScreen();
    }
}

void disconnectWifi() {
    debugMessage("DEBUG:", "disconnectWifi() called");
    displayMessage("Disconnecting...", "", 1000);
    WiFi.disconnect(true);
    preferences.begin("wifi-creds", false);
    preferences.clear();
    preferences.end();
    displayMessage("Disconnected & Forgotten");
}
#endif