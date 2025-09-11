#ifdef ENABLE_SD_CARD
#ifndef SD_CARD_H
#define SD_CARD_H

#include <Arduino.h> // For String type

extern bool isSdCardMounted;

void mountSD();
void unmountSD();
void showSDCardInfo();

#endif
#endif