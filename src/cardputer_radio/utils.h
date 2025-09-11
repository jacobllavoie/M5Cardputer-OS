#ifndef UTILS_H
#define UTILS_H

#include <Arduino.h> // For String type
#include <M5Cardputer.h>

void debugMessage(const char* type, const char* message);
void displayMessage(String title, String message = "", int delay_ms = 0);

#endif
