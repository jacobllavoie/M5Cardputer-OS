#include <M5Cardputer.h>
#include <esp_ota_ops.h>
#include <esp_system.h>
#include <Preferences.h>

// App-specific global to hold the loaded font size
float appTextSize = 1.0f;

// App-specific global to hold the font index
int currentFontSelection = 0; // <-- ADD THIS LINE

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
  WATTAGE
};
CalculationTarget target = VOLTAGE;

// Define input strings
String voltage_str = "0.0";
String current_str = "0.0";
String resistance_str = "0.0";
String wattage_str = "0.0";

// Define which input is currently active
enum ActiveInput {
  NONE,
  VOLTAGE_INPUT,
  CURRENT_INPUT,
  RESISTANCE_INPUT,
  WATTAGE_INPUT
};
ActiveInput active_input = NONE;

// New state for help screen
bool help_screen_active = false;

// New function to draw the help screen
void drawHelpScreen() {
  M5Cardputer.Display.fillScreen(BLACK);
  M5Cardputer.Display.setTextColor(WHITE);
  M5Cardputer.Display.setTextSize(1);
  M5Cardputer.Display.setTextDatum(top_left);

  M5Cardputer.Display.drawString("--- Help ---", 70, 5);
  M5Cardputer.Display.drawString("Up/Down (;/.)  : Select field", 10, 25);
  M5Cardputer.Display.drawString("Enter           : Edit field", 10, 45);
  M5Cardputer.Display.drawString("Calculate (c)   : Solve for", 10, 65);
  M5Cardputer.Display.drawString("                  selected field", 10, 78);
  M5Cardputer.Display.drawString("Exit (fn+`)     : To launcher", 10, 98);
  M5Cardputer.Display.setTextSize(1);
  M5Cardputer.Display.drawString("Press any key to close", 40, 120);
}

void drawScreen() {
  M5Cardputer.Display.fillScreen(BLACK);
  M5Cardputer.Display.setTextColor(WHITE);
  M5Cardputer.Display.setFont(availableFonts[currentFontSelection].font);
  M5Cardputer.Display.setTextSize(appTextSize);

  // Display titles and values
  M5Cardputer.Display.drawString("Voltage (V):", 10, 10);
  M5Cardputer.Display.drawString(voltage_str, 10, 25);

  M5Cardputer.Display.drawString("Current (A):", 10, 45);
  M5Cardputer.Display.drawString(current_str, 10, 60);

  M5Cardputer.Display.drawString("Resistance (O):", 10, 80);
  M5Cardputer.Display.drawString(resistance_str, 10, 95);

  M5Cardputer.Display.drawString("Wattage (W):", 10, 115);
  M5Cardputer.Display.drawString(wattage_str, 10, 130);

  // Highlight the calculation target
  switch (target) {
    case VOLTAGE:
      M5Cardputer.Display.drawRect(8, 8, 224, 35, GREEN);
      break;
    case CURRENT:
      M5Cardputer.Display.drawRect(8, 43, 224, 35, GREEN);
      break;
    case RESISTANCE:
      M5Cardputer.Display.drawRect(8, 78, 224, 35, GREEN);
      break;
    case WATTAGE:
      M5Cardputer.Display.drawRect(8, 113, 224, 35, GREEN);
      break;
  }
    // Highlight the active input
  switch (active_input) {
    case VOLTAGE_INPUT:
      M5Cardputer.Display.drawRect(8, 23, 224, 18, BLUE);
      break;
    case CURRENT_INPUT:
      M5Cardputer.Display.drawRect(8, 58, 224, 18, BLUE);
      break;
    case RESISTANCE_INPUT:
      M5Cardputer.Display.drawRect(8, 93, 224, 18, BLUE);
      break;
    case WATTAGE_INPUT:
      M5Cardputer.Display.drawRect(8, 128, 224, 18, BLUE);
      break;
    default:
      break;
  }
  
  // Add hint at the bottom right
  M5Cardputer.Display.setTextColor(TFT_DARKGREY);
  M5Cardputer.Display.drawString("h for help", 170, 125);
}

void setup() {
  M5Cardputer.begin();
  M5Cardputer.Display.setRotation(1);
  // --- Load Font Size from NVS ---
  Preferences prefs;
  prefs.begin("disp-settings", true); // Open preferences in read-only mode
  currentFontSelection = prefs.getInt("fontIndex", 0);
  float menuTextSize = prefs.getFloat("fontSize", 1.0f); // Default to 1.0 if not found
  prefs.end();
  M5Cardputer.Display.setTextSize(menuTextSize);
  // -----------------------------
  drawScreen();
}

void loop() {
  M5Cardputer.update();

  if (M5Cardputer.Keyboard.isChange() && M5Cardputer.Keyboard.isPressed()) {

    // If help screen is active, any key press will close it
    if (help_screen_active) {
        help_screen_active = false;
        drawScreen();
        return; // Skip the rest of the input handling
    }

    Keyboard_Class::KeysState status = M5Cardputer.Keyboard.keysState();

    if (active_input != NONE) {
      // Handle numerical input
      if (!status.word.empty()) {
        char key = status.word[0];
        if ((key >= '0' && key <= '9') || key == '.') {
          switch (active_input) {
            case VOLTAGE_INPUT: voltage_str += key; break;
            case CURRENT_INPUT: current_str += key; break;
            case RESISTANCE_INPUT: resistance_str += key; break;
            case WATTAGE_INPUT: wattage_str += key; break;
            default: break;
          }
        }
      } else if (status.del) {
        // Handle backspace
         switch (active_input) {
            case VOLTAGE_INPUT: if (voltage_str.length() > 0) { voltage_str.remove(voltage_str.length() - 1); } break;
            case CURRENT_INPUT: if (current_str.length() > 0) { current_str.remove(current_str.length() - 1); } break;
            case RESISTANCE_INPUT: if (resistance_str.length() > 0) { resistance_str.remove(resistance_str.length() - 1); } break;
            case WATTAGE_INPUT: if (wattage_str.length() > 0) { wattage_str.remove(wattage_str.length() - 1); } break;
            default: break;
          }
      } else if (status.enter) {
        active_input = NONE; // Deactivate input on enter
      }
    } else {
        // Handle other keys when not in input mode
        if (!status.word.empty() && status.word[0] == 'h') {
            help_screen_active = true;
            drawHelpScreen();
            return; // Show help and skip other actions
        }
      if (M5Cardputer.Keyboard.isKeyPressed(';')) {
        // Move selection up
        target = (CalculationTarget)((target - 1 + 4) % 4);
      } else if (M5Cardputer.Keyboard.isKeyPressed('.')) {
        // Move selection down
        target = (CalculationTarget)((target + 1) % 4);
      } else if (status.enter) {
        // Activate the selected input
        if (target == VOLTAGE) active_input = VOLTAGE_INPUT;
        else if (target == CURRENT) active_input = CURRENT_INPUT;
        else if (target == RESISTANCE) active_input = RESISTANCE_INPUT;
        else if (target == WATTAGE) active_input = WATTAGE_INPUT;
      } else if (status.fn && !status.word.empty() && status.word[0] == '`') { 
        // Return to launcher
        const esp_partition_t *launcher_partition = esp_partition_find_first(ESP_PARTITION_TYPE_APP, ESP_PARTITION_SUBTYPE_APP_OTA_0, NULL);
        if (launcher_partition != NULL) {
          esp_ota_set_boot_partition(launcher_partition);
          esp_restart();
        }
      } else if (!status.word.empty() && status.word[0] == 'c') { // Calculation key
        voltage = voltage_str.toFloat();
        current = current_str.toFloat();
        resistance = resistance_str.toFloat();
        wattage = wattage_str.toFloat();

        if (target == VOLTAGE) {
          voltage = current * resistance;
          voltage_str = String(voltage);
        } else if (target == CURRENT) {
          current = voltage / resistance;
          current_str = String(current);
        } else if (target == RESISTANCE) {
          resistance = voltage / current;
          resistance_str = String(resistance);
        } else if (target == WATTAGE) {
          wattage = voltage * current;
          wattage_str = String(wattage);
        }

        if (target != WATTAGE) {
          wattage = voltage * current;
          wattage_str = String(wattage);
        }
      }
    }
    // Only draw the main screen if help is not active
    if (!help_screen_active) {
        drawScreen();
    }
  }
}