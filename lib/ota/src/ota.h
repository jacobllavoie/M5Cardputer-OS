#ifdef ENABLE_OTA
#ifndef OTA_H
#define OTA_H

#include <M5CardputerOS_core.h>

void setupOTA();
void stopOTA();
void handleOTA();

#endif
#endif