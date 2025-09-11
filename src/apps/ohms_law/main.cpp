#include <M5Cardputer.h>
#include <esp_ota_ops.h>
#include <esp_system.h>

// Define variables for Ohm's Law
float voltage = 0.0;
float current = 0.0;
float resistance = 0.0;

// Define which variable to calculate
enum CalculationTarget {
  VOLTAGE,
  CURRENT,
  RESISTANCE
};
CalculationTarget target = VOLTAGE;

// Define input strings
String voltage_str = "0.0";
String current_str = "0.0";
String resistance_str = "0.0";

// Define which input is currently active
enum ActiveInput {
  NONE,
  VOLTAGE_INPUT,
  CURRENT_INPUT,
  RESISTANCE_INPUT
};
ActiveInput active_input = NONE;

void drawScreen() {
  M5Cardputer.Display.fillScreen(BLACK);
  M5Cardputer.Display.setTextColor(WHITE);
  M5Cardputer.Display.setTextSize(2);

  // Display titles
  M5Cardputer.Display.drawString("Voltage (V):", 10, 10);
  M5Cardputer.Display.drawString("Current (A):", 10, 50);
  M5Cardputer.Display.drawString("Resistance (O):", 10, 90);

  // Display values
  M5Cardputer.Display.drawString(voltage_str, 10, 30);
  M5Cardputer.Display.drawString(current_str, 10, 70);
  M5Cardputer.Display.drawString(resistance_str, 10, 110);

  // Highlight the calculation target
  switch (target) {
    case VOLTAGE:
      M5Cardputer.Display.drawRect(8, 8, 224, 40, GREEN);
      break;
    case CURRENT:
      M5Cardputer.Display.drawRect(8, 48, 224, 40, GREEN);
      break;
    case RESISTANCE:
      M5Cardputer.Display.drawRect(8, 88, 224, 40, GREEN);
      break;
  }
    // Highlight the active input
  switch (active_input) {
    case VOLTAGE_INPUT:
      M5Cardputer.Display.drawRect(8, 28, 224, 20, BLUE);
      break;
    case CURRENT_INPUT:
      M5Cardputer.Display.drawRect(8, 68, 224, 20, BLUE);
      break;
    case RESISTANCE_INPUT:
      M5Cardputer.Display.drawRect(8, 108, 224, 20, BLUE);
      break;
    default:
      break;
  }
}

void setup() {
  M5Cardputer.begin();
  M5Cardputer.Display.setRotation(1);
  drawScreen();
}

void loop() {
  M5Cardputer.update();

  if (M5Cardputer.Keyboard.isChange() && M5Cardputer.Keyboard.isPressed()) {
    Keyboard_Class::KeysState status = M5Cardputer.Keyboard.keysState();

    if (active_input != NONE) {
      // Handle numerical input
      if (!status.word.empty()) {
        char key = status.word[0];
        if ((key >= '0' && key <= '9') || key == '.') {
          switch (active_input) {
            case VOLTAGE_INPUT:
              voltage_str += key;
              break;
            case CURRENT_INPUT:
              current_str += key;
              break;
            case RESISTANCE_INPUT:
              resistance_str += key;
              break;
            default:
              break;
          }
        }
      } else if (status.del) {
        // Handle backspace
         switch (active_input) {
            case VOLTAGE_INPUT:
              if (voltage_str.length() > 0) {
                voltage_str.remove(voltage_str.length() - 1);
              }
              break;
            case CURRENT_INPUT:
              if (current_str.length() > 0) {
                current_str.remove(current_str.length() - 1);
              }
              break;
            case RESISTANCE_INPUT:
              if (resistance_str.length() > 0) {
                resistance_str.remove(resistance_str.length() - 1);
              }
              break;
            default:
              break;
          }
      } else if (status.enter) {
        active_input = NONE; // Deactivate input on enter
      }
    } else {
        // Handle other keys
      if (M5Cardputer.Keyboard.isKeyPressed(';')) {
        // Move selection up
        target = (CalculationTarget)((target - 1 + 3) % 3);
      } else if (M5Cardputer.Keyboard.isKeyPressed('.')) {
        // Move selection down
        target = (CalculationTarget)((target + 1) % 3);
      } else if (status.enter) {
        // Activate the selected input
        if (target == VOLTAGE) active_input = VOLTAGE_INPUT;
        else if (target == CURRENT) active_input = CURRENT_INPUT;
        else if (target == RESISTANCE) active_input = RESISTANCE_INPUT;
      } else if (M5Cardputer.Keyboard.isKeyPressed(KEY_ESC)) {
        // Return to launcher
        const esp_partition_t *launcher_partition = esp_partition_find_first(ESP_PARTITION_TYPE_APP, ESP_PARTITION_SUBTYPE_APP_OTA_0, NULL);
        if (launcher_partition != NULL) {
          esp_ota_set_boot_partition(launcher_partition);
          esp_restart();
        }
      } else if (M5Cardputer.Keyboard.isKeyPressed('c')) {
         // Calculate
        voltage = voltage_str.toFloat();
        current = current_str.toFloat();
        resistance = resistance_str.toFloat();

        if (target == VOLTAGE) {
          voltage = current * resistance;
          voltage_str = String(voltage);
        } else if (target == CURRENT) {
          current = voltage / resistance;
          current_str = String(current);
        } else if (target == RESISTANCE) {
          resistance = voltage / current;
          resistance_str = String(resistance);
        }
      }
    }
    drawScreen();
  }
}