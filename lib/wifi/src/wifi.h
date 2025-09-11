#ifdef ENABLE_WIFI
#ifndef WIFI_H
#define WIFI_H

#include <M5CardputerOS_core.h>

void wifiAutoConnect(bool returnToWifiMenu = false);
void showWifiStatus();
void scanWifiNetworks();
void disconnectWifi();

void saveWifiCredentials(const String& ssid, const String& password);

#endif
#endif