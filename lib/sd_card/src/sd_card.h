#ifdef ENABLE_SD_CARD
#ifndef SD_CARD_H
#define SD_CARD_H

#include <Arduino.h> // For String type

extern bool isSdCardMounted;

bool mountSD();
void unmountSD();
String getSDCardInfo(); // Returns a string with info, or an error message.
bool mountSDforMSC();

#endif
#endif