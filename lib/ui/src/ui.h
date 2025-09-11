#ifndef UI_H
#define UI_H

#include <M5CardputerOS_core.h>

void drawStartupScreen(String serialStatus, String sdStatus, String wifiStatus, String ip, bool showWelcome);
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
void debugMessage(String line1, String line2 = "");

#endif