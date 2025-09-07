#include "globals.h"
#include "serial.h"
#include "wifi.h"
#include "sd_card.h"
#include "ui.h" 

String serial_buffer = "";

void handleSerial() {
    while (Serial.available() > 0) {
        char incomingChar = Serial.read();
        if (incomingChar == '\n' || incomingChar == '\r') {
            if (serial_buffer.length() > 0) {
                processSerialCommand(serial_buffer);
                serial_buffer = "";
            }
        } else {
            serial_buffer += incomingChar;
        }
    }
}

void processSerialCommand(String command) {
    command.trim();
    Serial.println("# Processing: " + command);

    String lowerCaseCommand = command;
    lowerCaseCommand.toLowerCase();

    if (lowerCaseCommand.equalsIgnoreCase("help")) {
        Serial.println("--- Available Commands ---");
        Serial.println("help - Shows this message");
        Serial.println("status - Displays WiFi, SD, and battery status");
        Serial.println("scan - Scans for WiFi networks");
        Serial.println("connect <ssid> <password> - Connects to WiFi");
        Serial.println("reboot - Reboots the device");
        Serial.println("-------------------------");
    } 
    else if (lowerCaseCommand.equalsIgnoreCase("status")) {
        Serial.println("--- System Status ---");
        if (WiFi.status() == WL_CONNECTED) {
            Serial.println("WiFi: Connected to " + WiFi.SSID());
            Serial.println("IP Address: " + WiFi.localIP().toString());
        } else {
            Serial.println("WiFi: Disconnected");
        }
        Serial.println("SD Card: " + String(isSdCardMounted ? "Mounted" : "Not Mounted"));
        Serial.println("Battery: " + String(M5.Power.getBatteryLevel()) + "% (" + String(M5.Power.getBatteryVoltage() / 1000.0, 2) + "V)");
        Serial.println("---------------------");
    }
    else if (lowerCaseCommand.equalsIgnoreCase("scan")) {
        scanWifiNetworks();
    }
    else if (lowerCaseCommand.startsWith("connect ")) {
        int first_space = command.indexOf(' ');
        int second_space = command.indexOf(' ', first_space + 1);

        if (second_space > first_space) {
            String ssid = command.substring(first_space + 1, second_space);
            String password = command.substring(second_space + 1);
            Serial.println("Attempting to connect to SSID: " + ssid);
            
            displayMessage("Connecting via Serial...", ssid, 1500);
            WiFi.begin(ssid.c_str(), password.c_str());

        } else {
            Serial.println("Invalid format. Use: connect <ssid> <password>");
        }
    }
    else if (lowerCaseCommand.equalsIgnoreCase("reboot")) {
        Serial.println("Rebooting now...");
        delay(100);
        ESP.restart();
    }
    else {
        Serial.println("Unknown command. Type 'help' for a list of commands.");
    }
}