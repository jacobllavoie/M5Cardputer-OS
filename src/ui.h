#ifndef UI_H
#define UI_H

#include <M5Cardputer.h>

void initializeMenus();

void drawScreen();
void drawMainMenu();
void drawAppsMenu();
void drawSettingsMenu();
void drawDisplaySettingsMenu();
void drawSdCardMenu();
void drawWifiSettingsMenu();
void drawWifiScanResults();
void drawPasswordInputScreen();
void drawFactoryResetConfirmScreen();
void drawWebServerScreen();
void drawOtaScreen();
void drawKeyboardTestScreen();
void drawBatteryStatus();
void displayMessage(String line1, String line2 = "", int delay_ms = 2000);

#endif