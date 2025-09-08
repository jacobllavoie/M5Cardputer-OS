#ifdef ENABLE_WIFI
#include "globals.h"
#include "ui.h"
#include "ota.h"

void wifiAutoConnect() {
    #ifdef DEBUG_MODE
    Serial.println("DEBUG: wifiAutoConnect() called");
    #endif
    displayMessage("Checking WiFi...", "", 500);
    preferences.begin("wifi-creds", true);
    String ssid = preferences.getString("ssid", "");
    String password = preferences.getString("password", "");
    preferences.end();

    if (ssid.length() > 0) {
        displayMessage("Connecting to:", ssid, 1000);
        WiFi.begin(ssid.c_str(), password.c_str());

        int try_count = 0;
        while (WiFi.status() != WL_CONNECTED && try_count < 20) {
            delay(500);
            try_count++;
        }

        if (WiFi.status() == WL_CONNECTED) {
        } else {
            displayMessage("Connection Failed", "", 1500);
        }
    } else {
        displayMessage("No saved WiFi network.", "", 1500);
    }
}

void showWifiStatus() {
    #ifdef DEBUG_MODE
    Serial.println("DEBUG: showWifiStatus() called");
    #endif
    if (WiFi.status() == WL_CONNECTED) {
        displayMessage("Connected to: " + WiFi.SSID(), "IP: " + WiFi.localIP().toString());
    } else {
        displayMessage("WiFi Disconnected");
    }
}

void scanWifiNetworks() {
    #ifdef DEBUG_MODE
    Serial.println("DEBUG: scanWifiNetworks() called");
    #endif
    displayMessage("Scanning...", "", 100);
    Serial.println("Scanning for WiFi networks...");
    int n = WiFi.scanNetworks();
    scanned_networks.clear();
    
    if (n == 0) {
        displayMessage("No networks found");
        Serial.println("No networks found.");
    } else {
        Serial.println(String(n) + " networks found:");
        for (int i = 0; i < n; ++i) {
            String ssid = WiFi.SSID(i);
            Serial.println("  " + String(i + 1) + ": " + ssid);
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
    #ifdef DEBUG_MODE
    Serial.println("DEBUG: disconnectWifi() called");
    #endif
    displayMessage("Disconnecting...", "", 1000);
    WiFi.disconnect(true);
    preferences.begin("wifi-creds", false);
    preferences.clear();
    preferences.end();
    displayMessage("Disconnected & Forgotten");
}
#endif