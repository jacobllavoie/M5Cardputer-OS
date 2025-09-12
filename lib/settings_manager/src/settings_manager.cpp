#include "settings_manager.h"
#include <Preferences.h>

Preferences preferences;

void settings_init() {
    preferences.begin("cardputer-os", false); // 'cardputer-os' is the namespace
}

void settings_save_font_size(int size) {
    preferences.putInt("font_size", size);
    preferences.end();
}

int settings_get_font_size() {
    preferences.begin("cardputer-os", true);
    int fontSize = preferences.getInt("font_size", 1); // Default font size is 1
    preferences.end();
    return fontSize;
}

String settings_get_font_name() {
    return preferences.getString("font_name", "Orbitron"); // Default font name
}

void settings_save_font_name(const String& name) {
    preferences.putString("font_name", name);
}

void settings_save_wifi_credentials(const String& ssid, const String& password) {
    preferences.begin("wifi-creds", false);
    preferences.putString("ssid", ssid);
    preferences.putString("password", password);
    preferences.end();
}

String settings_get_wifi_ssid() {
    preferences.begin("wifi-creds", true);
    String ssid = preferences.getString("ssid", "");
    preferences.end();
    return ssid;
}

String settings_get_wifi_password() {
    preferences.begin("wifi-creds", true);
    String password = preferences.getString("password", "");
    preferences.end();
    return password;
}