#ifdef ENABLE_WIFI
#ifndef WIFI_H
#define WIFI_H

void wifiAutoConnect(bool returnToWifiMenu = false);
void showWifiStatus();
void scanWifiNetworks();
void disconnectWifi();

void saveWifiCredentials(const String& ssid, const String& password);

#endif
#endif