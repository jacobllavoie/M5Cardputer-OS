#include "globals.h"
#include "ui.h"
#include "web_server.h"
#include "ota.h"
#include <ArduinoOTA.h>

void setupOTA() {
  // Set a hostname for the device
  ArduinoOTA.setHostname("M5Cardputer-OS");

  // Set a password for security. You will be prompted for this in the Arduino IDE.
  ArduinoOTA.setPassword("m5stack");

  ArduinoOTA
    .onStart([]() {
      String type;
      if (ArduinoOTA.getCommand() == U_FLASH)
        type = "sketch";
      else
        type = "filesystem";
      
      stopWebServer();
      displayMessage("Starting OTA Update", type);
    })
    .onEnd([]() {
      displayMessage("Update Complete!");
    })
    .onProgress([](unsigned int progress, unsigned int total) {
      M5Cardputer.Display.clear();
      M5Cardputer.Display.setTextDatum(middle_center);
      M5Cardputer.Display.drawString("Updating...", M5Cardputer.Display.width() / 2, M5Cardputer.Display.height() / 2 - 15);
      M5Cardputer.Display.drawString(String(progress / (total / 100)) + "%", M5Cardputer.Display.width() / 2, M5Cardputer.Display.height() / 2 + 15);
    })
    .onError([](ota_error_t error) {
      String errorMsg = "Error[" + String(error) + "]: ";
      if (error == OTA_AUTH_ERROR) errorMsg += "Auth Failed";
      else if (error == OTA_BEGIN_ERROR) errorMsg += "Begin Failed";
      else if (error == OTA_CONNECT_ERROR) errorMsg += "Connect Failed";
      else if (error == OTA_RECEIVE_ERROR) errorMsg += "Receive Failed";
      else if (error == OTA_END_ERROR) errorMsg += "End Failed";
      displayMessage("OTA Error!", errorMsg, 5000);
    });

  ArduinoOTA.begin();
  Serial.println("OTA Ready");
}

void stopOTA() {
    Serial.println("OTA Stopped.");
}

void handleOTA() {
  ArduinoOTA.handle();
}