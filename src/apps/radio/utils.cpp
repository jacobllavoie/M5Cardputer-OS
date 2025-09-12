#include "utils.h"

void debugMessage(const char* type, const char* message) {
  Serial.print(type);
  Serial.println(message);
}

void displayMessage(String title, String message, int delay_ms) {
  M5Cardputer.Display.drawString(title, 0, 15);
  if (message != "") {
    M5Cardputer.Display.drawString(message, 0, 30);
  }
  if (delay_ms > 0) {
    delay(delay_ms);
  }
}
