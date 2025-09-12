#ifndef SETTINGS_MANAGER_H
#define SETTINGS_MANAGER_H

#include <Arduino.h>

void settings_init();
void settings_save_font_size(int size);
int settings_get_font_size();
void settings_save_font_name(const String& name);
String settings_get_font_name();

void settings_save_wifi_credentials(const String& ssid, const String& password);
String settings_get_wifi_ssid();
String settings_get_wifi_password();

#endif