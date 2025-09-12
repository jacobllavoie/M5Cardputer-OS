#include <M5Cardputer.h>
#include <esp_ota_ops.h>
#include <esp_system.h>
#include <M5CardputerOS_core.h>
#include <settings_manager.h>

// App-specific global to hold the loaded font size
float appTextSize = 1.0f;

// --- UI Scrolling ---
int scrollOffset = 0;
const int VISIBLE_ITEMS = 3; // Correctly show 3 items to allow for scrolling

// Define variables for Ohm's Law
float voltage = 0.0;
float current = 0.0;
float resistance = 0.0;
float wattage = 0.0;

// Define which variable to calculate
enum CalculationTarget {
  VOLTAGE,
  CURRENT,
  RESISTANCE,
  WATTAGE,
  NUM_TARGETS // Helper to get the count
};
CalculationTarget target = VOLTAGE;

// Define input strings and create arrays for UI
String voltage_str = "0.0";
String current_str = "0.0";
String resistance_str = "0.0";
String wattage_str = "0.0";
const char* fieldNames[] = {"Voltage (V)", "Current (A)", "Resistance (O)", "Wattage (W)"};
String* fieldValues[] = {&voltage_str, &current_str, &resistance_str, &wattage_str};


// Define which input is currently active
enum ActiveInput {
  NONE,
  VOLTAGE_INPUT,
  CURRENT_INPUT,
  RESISTANCE_INPUT,
  WATTAGE_INPUT
};
ActiveInput active_input = NONE;

// Help screen state
bool help_screen_active = false;

void drawHelpScreen() {
  M5Cardputer.Display.fillScreen(BLACK);
  M5Cardputer.Display.setTextColor(WHITE);
  M5Cardputer.Display.setTextSize(1);
  
  M5Cardputer.Display.setTextDatum(top_center);
  M5Cardputer.Display.drawString("--- Help ---", M5Cardputer.Display.width() / 2, 10);

  M5Cardputer.Display.setTextDatum(top_left);
  int y = 30;
  int lineHeight = 18;
  M5Cardputer.Display.drawString("Up/Down (;/.) : Select field", 10, y); y += lineHeight;
  M5Cardputer.Display.drawString("Enter         : Edit field", 10, y); y += lineHeight;
  M5Cardputer.Display.drawString("Calculate (c) : Solve for selected", 10, y); y += lineHeight;
  M5Cardputer.Display.drawString("Exit (fn+`)   : To launcher", 10, y); y += lineHeight;

  M5Cardputer.Display.setTextDatum(bottom_center);
  M5Cardputer.Display.drawString("Press any key to close", M5Cardputer.Display.width() / 2, M5Cardputer.Display.height() - 10);
}

void drawScreen() {
  M5Cardputer.Display.fillScreen(BLACK);
  M5Cardputer.Display.setTextColor(WHITE);
  M5Cardputer.Display.setFont(availableFonts[currentFontSelection].font);
  M5Cardputer.Display.setTextSize(appTextSize);
  M5Cardputer.Display.setTextDatum(top_left);

  int lineHeight = 35; // Height for each item block

  // Loop through and draw each visible calculator field
  for (int i = scrollOffset; i < (scrollOffset + VISIBLE_ITEMS) && i < NUM_TARGETS; i++) {
      int y = 10 + (i - scrollOffset) * lineHeight;

      // Highlight the main selection (calculation target)
      if (target == (CalculationTarget)i) {
          M5Cardputer.Display.drawRect(8, y - 2, 224, lineHeight, GREEN);
      }

      // Highlight the field currently being edited
      if (active_input == (ActiveInput)(i + 1)) { // ActiveInput enum is 1-based
          M5Cardputer.Display.drawRect(8, y + 13, 224, 18, BLUE);
      }

      M5Cardputer.Display.drawString(fieldNames[i], 10, y);
      M5Cardputer.Display.drawString(*fieldValues[i], 10, y + 15);
  }

  // Add hint at the bottom right
  M5Cardputer.Display.setTextDatum(bottom_right);
  M5Cardputer.Display.setTextColor(TFT_DARKGREY);
  M5Cardputer.Display.drawString("h for help", M5Cardputer.Display.width() - 5, M5Cardputer.Display.height() - 5);
  M5Cardputer.Display.setTextDatum(top_left); // Reset datum
}

void setup() {
  M5Cardputer.begin();
  M5Cardputer.Display.setRotation(1);

  // --- Load Font Settings ---
  settings_init();
  appTextSize = (float)settings_get_font_size() / 10.0f;
  String savedFontName = settings_get_font_name();
  for (int i = 0; i < numAvailableFonts; ++i) {
      if (savedFontName == availableFonts[i].name) {
          currentFontSelection = i;
          break;
      }
  }
  // -----------------------------

  drawScreen();
}

void loop() {
  M5Cardputer.update();

  if (M5Cardputer.Keyboard.isChange() && M5Cardputer.Keyboard.isPressed()) {

    if (help_screen_active) {
        help_screen_active = false;
        drawScreen();
        return;
    }

    Keyboard_Class::KeysState status = M5Cardputer.Keyboard.keysState();

    if (active_input != NONE) {
      if (!status.word.empty()) {
        char key = status.word[0];
        if ((key >= '0' && key <= '9') || key == '.') {
          *fieldValues[active_input - 1] += key;
        }
      } else if (status.del) {
        String* current_str = fieldValues[active_input - 1];
        if (current_str->length() > 0) {
            current_str->remove(current_str->length() - 1);
        }
      } else if (status.enter) {
        active_input = NONE;
      }
    } else {
        if (!status.word.empty() && status.word[0] == 'h') {
            help_screen_active = true;
            drawHelpScreen();
            return;
        }
      if (M5Cardputer.Keyboard.isKeyPressed(';')) {
        target = (CalculationTarget)((target - 1 + NUM_TARGETS) % NUM_TARGETS);
      } else if (M5Cardputer.Keyboard.isKeyPressed('.')) {
        target = (CalculationTarget)((target + 1) % NUM_TARGETS);
      }
      
      // Adjust scroll offset to keep selection in view
      if (target < scrollOffset) {
          scrollOffset = target;
      }
      if (target >= scrollOffset + VISIBLE_ITEMS) {
          scrollOffset = target - VISIBLE_ITEMS + 1;
      }
      
      if (status.enter) {
        active_input = (ActiveInput)(target + 1);
        // Clear the field when editing starts
        *fieldValues[active_input - 1] = "";
      } else if (status.fn && !status.word.empty() && status.word[0] == '`') {
        const esp_partition_t *launcher_partition = esp_partition_find_first(ESP_PARTITION_TYPE_APP, ESP_PARTITION_SUBTYPE_APP_OTA_0, NULL);
        if (launcher_partition != NULL) {
          esp_ota_set_boot_partition(launcher_partition);
          esp_restart();
        }
      } else if (!status.word.empty() && status.word[0] == 'c') {
        voltage = voltage_str.toFloat();
        current = current_str.toFloat();
        resistance = resistance_str.toFloat();

        if (target == VOLTAGE) {
          voltage = current * resistance;
          voltage_str = String(voltage);
        } else if (target == CURRENT) {
          if (resistance == 0) current = 0;
          else current = voltage / resistance;
          current_str = String(current);
        } else if (target == RESISTANCE) {
          if (current == 0) resistance = 0;
          else resistance = voltage / current;
          resistance_str = String(resistance);
        }
        
        // Always recalculate wattage based on the new values
        wattage = voltage * current;
        wattage_str = String(wattage);
      }
    }
    
    if (!help_screen_active) {
        drawScreen();
    }
  }
}

