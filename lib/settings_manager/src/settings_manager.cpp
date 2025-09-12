#include "settings_manager.h"
#include <Preferences.h>

Preferences preferences;

void settings_init() {
    preferences.begin("cardputer-os", false);
}

void settings_save_font_size(int size) {
    preferences.putInt("font_size", size);
}

int settings_get_font_size() {
    return preferences.getInt("font_size", 10);
}

void settings_save_font_name(const String& name) {
    preferences.putString("font_name", name);
}

String settings_get_font_name() {
    return preferences.getString("font_name", "Orbitron");
}

void settings_save_wifi_credentials(const String& ssid, const String& password) {
    preferences.putString("ssid", ssid);
    preferences.putString("password", password);
}

String settings_get_wifi_ssid() {
    return preferences.getString("ssid", "");
}

String settings_get_wifi_password() {
    return preferences.getString("password", "");
}