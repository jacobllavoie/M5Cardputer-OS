#include <M5Cardputer.h>
#include <Adafruit_NeoPixel.h>
#include <esp_ota_ops.h>
#include <esp_system.h>
#include <M5CardputerOS_core.h>
#include <settings_manager.h>

// --- LED Strip Configuration ---
#define LED_PIN    1
#define LED_COUNT 65
Adafruit_NeoPixel strip(LED_COUNT, LED_PIN, NEO_GRBW + NEO_KHZ800);

// --- App State ---
enum AnimationMode {
    MODE_RAINBOW,
    MODE_COLOR_WIPE,
    MODE_SOLID_COLOR,
    MODE_OFF
};
AnimationMode currentMode = MODE_RAINBOW;
const int NUM_MODES = 4;
const char* modeNames[] = {"Rainbow", "Color Wipe", "Solid Color", "Off"};

enum SelectedParam {
    PARAM_MODE,
    PARAM_RED,
    PARAM_GREEN,
    PARAM_BLUE,
    PARAM_BRIGHTNESS,
    PARAM_SPEED
};
SelectedParam currentParam = PARAM_MODE;
// --- UI Scrolling ---
int paramScrollOffset = 0;
const int VISIBLE_PARAMS = 5; // How many parameters to show on screen at once
const int NUM_PARAMS = 6;

// --- Animation Parameters ---
int red = 255;
int green = 0;
int blue = 0;
int brightness = 1; // 0-255
int animationSpeed = 50; // Delay in ms

// --- Helper Functions for Animations ---
uint32_t wheel(byte WheelPos) {
    WheelPos = 255 - WheelPos;
    if (WheelPos < 85) {
        return strip.Color(255 - WheelPos * 3, 0, WheelPos * 3);
    }
    if (WheelPos < 170) {
        WheelPos -= 85;
        return strip.Color(0, WheelPos * 3, 255 - WheelPos * 3);
    }
    WheelPos -= 170;
    return strip.Color(WheelPos * 3, 255 - WheelPos * 3, 0);
}

void rainbow() {
    static uint16_t j = 0;
    for (uint16_t i = 0; i < strip.numPixels(); i++) {
        strip.setPixelColor(i, wheel((i + j) & 255));
    }
    strip.show();
    j++;
    delay(animationSpeed);
}

void colorWipe(uint32_t color) {
    for (uint16_t i = 0; i < strip.numPixels(); i++) {
        strip.setPixelColor(i, color);
        strip.show();
        delay(animationSpeed);
    }
}

void setSolidColor() {
    uint32_t color = strip.Color(red, green, blue);
    for (uint16_t i = 0; i < strip.numPixels(); i++) {
        strip.setPixelColor(i, color);
    }
    strip.show();
}

void turnOff() {
    strip.clear();
    strip.show();
}

// --- UI Drawing ---
void drawUI() {
    M5Cardputer.Display.fillScreen(BLACK);
    M5Cardputer.Display.setTextDatum(top_left);

    // Title
    M5Cardputer.Display.drawString("LED Controller", 10, 5);
    M5Cardputer.Display.drawFastHLine(0, 20, M5Cardputer.Display.width(), DARKGREY);

    // --- Create arrays for parameter names and values ---
    const char* paramNames[] = {"Mode", "Red", "Green", "Blue", "Brightness", "Speed"};
    String paramValues[NUM_PARAMS];
    paramValues[0] = String(modeNames[currentMode]);
    paramValues[1] = String(red);
    paramValues[2] = String(green);
    paramValues[3] = String(blue);
    paramValues[4] = String(brightness);
    paramValues[5] = String(animationSpeed);
    
    // --- Loop and draw only the visible parameters ---
    int lineHeight = 18;
    for (int i = paramScrollOffset; i < (paramScrollOffset + VISIBLE_PARAMS) && i < NUM_PARAMS; i++) {
        int y_pos = 25 + (i - paramScrollOffset) * lineHeight;
        SelectedParam param = (SelectedParam)i;

        M5Cardputer.Display.setTextColor(currentParam == param ? BLACK : WHITE, currentParam == param ? WHITE : BLACK);
        M5Cardputer.Display.drawString(String(paramNames[i]) + ": " + paramValues[i], 10, y_pos);
    }

    // Instructions
    M5Cardputer.Display.setTextColor(WHITE, BLACK);
    M5Cardputer.Display.drawString("Up/Down: Select | L/R: Adjust", 5, M5Cardputer.Display.height() - 15);
}

// --- Input Handling ---
void handleInput() {
    if (M5Cardputer.Keyboard.isChange() && M5Cardputer.Keyboard.isPressed()) {
        Keyboard_Class::KeysState status = M5Cardputer.Keyboard.keysState();

        // Exit to launcher
        if (status.fn && !status.word.empty() && status.word[0] == '`') {
            turnOff();
            const esp_partition_t *launcher_partition = esp_partition_find_first(ESP_PARTITION_TYPE_APP, ESP_PARTITION_SUBTYPE_APP_OTA_0, NULL);
            if (launcher_partition != NULL) {
                esp_ota_set_boot_partition(launcher_partition);
                esp_restart();
            }
        }

        // Navigate parameters
        if (M5Cardputer.Keyboard.isKeyPressed(';')) { // Up
            currentParam = (SelectedParam)((currentParam - 1 + NUM_PARAMS) % NUM_PARAMS);
        }
        if (M5Cardputer.Keyboard.isKeyPressed('.')) { // Down
            currentParam = (SelectedParam)((currentParam + 1) % NUM_PARAMS);
        }
        // Adjust scroll offset to keep the selection visible
        if (currentParam < paramScrollOffset) {
            paramScrollOffset = currentParam;
        }
        if (currentParam >= paramScrollOffset + VISIBLE_PARAMS) {
            paramScrollOffset = currentParam - VISIBLE_PARAMS + 1;
        }

        // Adjust values
        int change = 0;
        if (M5Cardputer.Keyboard.isKeyPressed(',')) change = -5; // Left
        if (M5Cardputer.Keyboard.isKeyPressed('/')) change = 5;  // Right
        if (M5Cardputer.Keyboard.isKeyPressed('o')) change = -50; // Shift + Left
        if (M5Cardputer.Keyboard.isKeyPressed('p')) change = 50;  // Shift + Right


        switch (currentParam) {
            case PARAM_MODE:
                if (change != 0) {
                     currentMode = (AnimationMode)((currentMode + (change > 0 ? 1 : -1) + NUM_MODES) % NUM_MODES);
                }
                break;
            case PARAM_RED:
                red = constrain(red + change, 0, 255);
                break;
            case PARAM_GREEN:
                green = constrain(green + change, 0, 255);
                break;
            case PARAM_BLUE:
                blue = constrain(blue + change, 0, 255);
                break;
            case PARAM_BRIGHTNESS:
                brightness = constrain(brightness + change, 0, 255);
                strip.setBrightness(brightness);
                break;
            case PARAM_SPEED:
                animationSpeed = constrain(animationSpeed + change, 1, 200);
                break;
        }

        drawUI();
    }
}

// --- Main Setup and Loop ---
void setup() {
    M5Cardputer.begin();
    M5Cardputer.Display.setRotation(1);

    // Load global font settings
    settings_init();
    float textSize = (float)settings_get_font_size() / 10.0f;
    String fontName = settings_get_font_name();
    for (int i = 0; i < numAvailableFonts; ++i) {
        if (fontName == availableFonts[i].name) {
            currentFontSelection = i;
            break;
        }
    }
    M5Cardputer.Display.setFont(availableFonts[currentFontSelection].font);
    M5Cardputer.Display.setTextSize(textSize);

    // Initialize LED strip
    strip.begin();
    strip.setBrightness(brightness);
    strip.show(); // Initialize all pixels to 'off'

    drawUI();
}

void loop() {
    M5Cardputer.update();
    handleInput();

    // Run the current animation
    switch (currentMode) {
        case MODE_RAINBOW:
            rainbow();
            break;
        case MODE_COLOR_WIPE:
            colorWipe(strip.Color(red, green, blue));
            break;
        case MODE_SOLID_COLOR:
            setSolidColor();
            break;
        case MODE_OFF:
            turnOff();
            break;
    }
}
