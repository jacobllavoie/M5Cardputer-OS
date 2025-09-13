#ifdef ENABLE_WEB_SERVER
#include <M5CardputerOS_core.h>
#include <WiFi.h>
#include <ESPAsyncWebServer.h>
#include <ArduinoJson.h>
#include "web_server.h"
#include "sd_card.h"

// --- Web Server and File Upload Variables ---
AsyncWebServer server(80);
File uploadFile;

// --- API Endpoint Handlers ---
void handleStats(AsyncWebServerRequest *request) {
    JsonDocument doc;
    doc["ip"] = WiFi.localIP().toString();
    doc["cpuUtilization"] = random(10, 30); // Placeholder
    doc["batteryPercentage"] = M5.Power.getBatteryLevel();
    if (isSdCardMounted) {
        doc["sdTotalBytes"] = SD.cardSize();
        doc["sdUsedBytes"] = SD.usedBytes();
    } else {
        doc["sdTotalBytes"] = 0;
        doc["sdUsedBytes"] = 0;
    }
    String json;
    serializeJson(doc, json);
    request->send(200, "application/json", json);
}

void handleFileList(AsyncWebServerRequest *request) {
    if (!isSdCardMounted) {
        request->send(500, "application/json", "{\"error\":\"SD Card not mounted\"}");
        return;
    }

    String path = "/";
    if (request->hasParam("dir")) {
        path = request->getParam("dir")->value();
    }

    File root = SD.open(path);
    if (!root || !root.isDirectory()) {
        request->send(500, "application/json", "{\"error\":\"Directory not found\"}");
        return;
    }

    String json = "[";
    File file = root.openNextFile();
    while(file){
        if (json != "[") json += ",";
        json += "{\"name\":\"" + String(file.name()) + "\",\"size\":" + String(file.size()) + ",\"isDir\":" + (file.isDirectory() ? "true" : "false") + "}";
        file.close();
        file = root.openNextFile();
    }
    root.close();
    json += "]";
    request->send(200, "application/json", json);
}

void handleDelete(AsyncWebServerRequest *request) {
    if (request->hasParam("path")) {
        if (SD.remove(request->getParam("path")->value()) || SD.rmdir(request->getParam("path")->value())) {
            request->send(200, "text/plain", "Deleted");
        } else {
            request->send(500, "text/plain", "Delete failed");
        }
    } else {
        request->send(400, "text/plain", "Bad Request");
    }
}

void handleMove(AsyncWebServerRequest *request) {
    if (request->hasParam("from") && request->hasParam("to")) {
        if (SD.rename(request->getParam("from")->value(), request->getParam("to")->value())) {
            request->send(200, "text/plain", "Moved/Renamed");
        } else {
            request->send(500, "text/plain", "Move/Rename failed");
        }
    } else {
        request->send(400, "text/plain", "Bad Request");
    }
}


void handleUpload(AsyncWebServerRequest *request, String filename, size_t index, uint8_t *data, size_t len, bool final) {
    String path = "/";
     if (request->hasParam("dir")) {
        path = request->getParam("dir")->value();
    }
    if (!path.endsWith("/")) {
        path += "/";
    }

    if (!index) {
        String filePath = path + filename;
        if(SD.exists(filePath)){
            SD.remove(filePath);
        }
        uploadFile = SD.open(filePath, FILE_WRITE);
    }
    if (uploadFile) {
        uploadFile.write(data, len);
    }
    if (final) {
        if (uploadFile) {
            uploadFile.close();
        }
        request->redirect("/");
    }
}

// --- Web Server Control ---
void startWebServer() {
    // Serve static files from SD card
    server.serveStatic("/", SD, "/").setDefaultFile("index.html");

    server.on("/stats", HTTP_GET, handleStats);
    server.on("/files", HTTP_GET, handleFileList);
    server.on("/delete", HTTP_POST, handleDelete);
    server.on("/move", HTTP_POST, handleMove); // Also used for rename
    server.on(
        "/upload", HTTP_POST,
        [](AsyncWebServerRequest *request) {
            request->send(200);
        },
        handleUpload
    );
    
    server.begin();
}

void stopWebServer() {
    server.end();
}

void handleWebServerClient() {
    // Not needed for ESPAsyncWebServer
}
#endif

