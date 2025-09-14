#include <M5Cardputer.h>
#include <esp_ota_ops.h>
#include <esp_system.h>
#include <WiFi.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>
#include <vector>
#include <string>

#include "secrets.h" // For your MQTT configuration
#include <M5CardputerOS_core.h>
#include <settings_manager.h>

// --- Globals ---
float appTextSize = 1.0f;
WiFiClient espClient;
PubSubClient client(espClient);

std::vector<String> messages;
const int MAX_MESSAGES = 20;
int messageScrollOffset = 0;
String inputBuffer = "";
unsigned long lastReconnectAttempt = 0;

// --- Function Prototypes ---
void drawUI();
void callback(char* topic, byte* payload, unsigned int length);
void try_reconnect();
void addMessage(String message);

// --- UI Drawing ---
void drawUI() {
    M5Cardputer.Display.fillScreen(BLACK);
    M5Cardputer.Display.setTextDatum(top_left);

    // Header
    String status = client.connected() ? "Connected" : "Disconnected";
    uint16_t color = client.connected() ? GREEN : RED;
    M5Cardputer.Display.setTextColor(color);
    M5Cardputer.Display.drawString("Meshtastic MQTT Chat - " + status, 5, 5);
    M5Cardputer.Display.setTextColor(WHITE);
    M5Cardputer.Display.drawFastHLine(0, 20, M5Cardputer.Display.width(), DARKGREY);

    // Message Area
    int lineHeight = 12;
    int visibleMessages = 8;
    for (int i = 0; i < visibleMessages && (i + messageScrollOffset) < messages.size(); ++i) {
        M5Cardputer.Display.drawString(messages[i + messageScrollOffset], 5, 25 + i * lineHeight);
    }

    // Input Area
    M5Cardputer.Display.drawFastHLine(0, M5Cardputer.Display.height() - 20, M5Cardputer.Display.width(), DARKGREY);
    M5Cardputer.Display.drawString("> " + inputBuffer, 5, M5Cardputer.Display.height() - 15);
}

// --- App Logic ---
void setup() {
    M5Cardputer.begin();
    M5Cardputer.Display.setRotation(1);

    settings_init();
    appTextSize = (float)settings_get_font_size() / 10.0f;
    String savedFontName = settings_get_font_name();
    for (int i = 0; i < numAvailableFonts; ++i) {
        if (savedFontName == availableFonts[i].name) {
            currentFontSelection = i;
            break;
        }
    }
    M5Cardputer.Display.setFont(availableFonts[currentFontSelection].font);
    M5Cardputer.Display.setTextSize(appTextSize);
    
    // --- WiFi Connection using NVS credentials ---
    addMessage("Connecting to WiFi...");
    drawUI();
    String ssid = settings_get_wifi_ssid();
    String password = settings_get_wifi_password();

    if (ssid.length() > 0) {
        WiFi.begin(ssid.c_str(), password.c_str());
        int retries = 0;
        while (WiFi.status() != WL_CONNECTED && retries < 20) {
            addMessage("Connecting... " + String(retries));
            drawUI();
            delay(500);
            retries++;
            M5Cardputer.update(); // Keep keyboard responsive
        }
    }

    if (WiFi.status() == WL_CONNECTED) {
        addMessage("WiFi Connected!");
    } else {
        addMessage("WiFi connection failed.");
    }
    
    addMessage("Setting up MQTT...");
    drawUI();
    client.setServer(MQTT_SERVER, MQTT_PORT);
    client.setCallback(callback);
}

void loop() {
    M5Cardputer.update();

    if (!client.connected() && WiFi.status() == WL_CONNECTED) {
        // Try to reconnect every 5 seconds
        if (millis() - lastReconnectAttempt > 5000) {
            try_reconnect();
        }
    } else {
        client.loop(); // Only process messages if connected
    }

    if (M5Cardputer.Keyboard.isChange() && M5Cardputer.Keyboard.isPressed()) {
        Keyboard_Class::KeysState status = M5Cardputer.Keyboard.keysState();

        if (!status.word.empty()) {
            for (char c : status.word) {
                inputBuffer += c;
            }
        } else if (status.del && inputBuffer.length() > 0) {
            inputBuffer.remove(inputBuffer.length() - 1);
        } else if (status.enter && inputBuffer.length() > 0) {
            if (client.connected()) {
                JsonDocument doc;
                doc["from"] = MQTT_NICKNAME;
                doc["text"] = inputBuffer;
                
                char buffer[256];
                serializeJson(doc, buffer);
                
                String topic = String(MQTT_TOPIC_PREFIX) + "/" + MQTT_NICKNAME;
                client.publish(topic.c_str(), buffer);
                
                inputBuffer = "";
            } else {
                addMessage("Not connected. Cannot send.");
            }
        }

        // Scrolling message history
        if (M5Cardputer.Keyboard.isKeyPressed(';')) { // Up
            if (messageScrollOffset > 0) messageScrollOffset--;
        }
        if (M5Cardputer.Keyboard.isKeyPressed('.')) { // Down
            if (messageScrollOffset < (int)messages.size() - 8) messageScrollOffset++;
        }

        // Exit to launcher
        if (status.fn && !status.word.empty() && status.word[0] == '`') {
            const esp_partition_t *launcher_partition = esp_partition_find_first(ESP_PARTITION_TYPE_APP, ESP_PARTITION_SUBTYPE_APP_OTA_0, NULL);
            if (launcher_partition != NULL) {
                pixel.clear(); pixel.show(); // Turn off LED
                esp_ota_set_boot_partition(launcher_partition);
                esp_restart();
            }
        }
        drawUI();
    }
}

// --- Helper Functions ---
void callback(char* topic, byte* payload, unsigned int length) {
    String message;
    for (int i = 0; i < length; i++) {
        message += (char)payload[i];
    }
    
    JsonDocument doc;
    deserializeJson(doc, message);
    
    String from = doc["from"] | "unknown";
    String text = doc["text"] | "";
    
    addMessage(from + ": " + text);
    drawUI();
}

void try_reconnect() {
    lastReconnectAttempt = millis();
    addMessage("Connecting to MQTT...");
    drawUI();
    String clientId = "M5Cardputer-";
    clientId += String(random(0xffff), HEX);
    if (client.connect(clientId.c_str(), MQTT_USER, MQTT_PASSWORD)) {
        addMessage("MQTT Connected!");
        String topic = String(MQTT_TOPIC_PREFIX) + "/#";
        client.subscribe(topic.c_str());
    } else {
        addMessage("MQTT failed, rc=" + String(client.state()));
    }
    drawUI();
}

void addMessage(String message) {
    if (messages.size() >= MAX_MESSAGES) {
        messages.pop_back();
    }
    messages.insert(messages.begin(), message);
}

