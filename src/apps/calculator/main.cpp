#include <M5Cardputer.h>
#include <esp_ota_ops.h>
#include <esp_system.h>
#include <M5CardputerOS_core.h>
#include <settings_manager.h>
#include <stack>
#include <string>
#include <vector>
#include <cctype>

// --- App State ---
String inputExpression = "";
String resultString = "Result: ";
long last_cursor_blink = 0;
bool cursor_visible = true;

// --- Helper function for operator precedence ---
int getPrecedence(char op) {
    if (op == '+' || op == '-') return 1;
    if (op == '*' || op == '/') return 2;
    return 0;
}

// --- Helper function to apply an operator ---
double applyOp(double a, double b, char op) {
    switch (op) {
        case '+': return a + b;
        case '-': return a - b;
        case '*': return a * b;
        case '/': 
            if (b == 0) return NAN; // Handle division by zero
            return a / b;
    }
    return 0;
}

// --- Main evaluation function ---
double evaluate(String expression) {
    std::stack<double> values;
    std::stack<char> ops;
    
    for (int i = 0; i < expression.length(); ++i) {
        if (isspace(expression[i])) continue;

        if (isdigit(expression[i]) || expression[i] == '.') {
            String s;
            while (i < expression.length() && (isdigit(expression[i]) || expression[i] == '.')) {
                s += expression[i++];
            }
            values.push(s.toFloat());
            i--;
        } else if (expression[i] == '(') {
            ops.push(expression[i]);
        } else if (expression[i] == ')') {
            while (!ops.empty() && ops.top() != '(') {
                double val2 = values.top(); values.pop();
                double val1 = values.top(); values.pop();
                char op = ops.top(); ops.pop();
                values.push(applyOp(val1, val2, op));
            }
            if (!ops.empty()) ops.pop(); // Pop '('
        } else { // Operator
            while (!ops.empty() && getPrecedence(ops.top()) >= getPrecedence(expression[i])) {
                double val2 = values.top(); values.pop();
                double val1 = values.top(); values.pop();
                char op = ops.top(); ops.pop();
                values.push(applyOp(val1, val2, op));
            }
            ops.push(expression[i]);
        }
    }

    while (!ops.empty()) {
        double val2 = values.top(); values.pop();
        double val1 = values.top(); values.pop();
        char op = ops.top(); ops.pop();
        values.push(applyOp(val1, val2, op));
    }
    
    return values.top();
}


// --- UI Drawing ---
void drawUI() {
    M5Cardputer.Display.fillScreen(BLACK);
    M5Cardputer.Display.setTextDatum(top_left);

    // Draw the expression with a blinking cursor
    String displayStr = "> " + inputExpression;
    if (cursor_visible) {
        displayStr += "_";
    }
    M5Cardputer.Display.drawString(displayStr, 10, 30);

    // Draw the result
    M5Cardputer.Display.drawString(resultString, 10, 70);
    
    // Instructions
    M5Cardputer.Display.drawString("C: Clear | Enter: Calc", 10, M5Cardputer.Display.height() - 20);
}

// --- Input Handling ---
void handleInput() {
    if (M5Cardputer.Keyboard.isChange() && M5Cardputer.Keyboard.isPressed()) {
        Keyboard_Class::KeysState status = M5Cardputer.Keyboard.keysState();

        // Exit to launcher
        if (status.fn && !status.word.empty() && status.word[0] == '`') {
            const esp_partition_t *launcher_partition = esp_partition_find_first(ESP_PARTITION_TYPE_APP, ESP_PARTITION_SUBTYPE_APP_OTA_0, NULL);
            if (launcher_partition != NULL) {
                esp_ota_set_boot_partition(launcher_partition);
                esp_restart();
            }
            return;
        }

        // Handle character input
        if (!status.word.empty()) {
            char key = status.word[0];
            if (isdigit(key) || key == '.' || key == '+' || key == '-' || key == '*' || key == '/' || key == '(' || key == ')') {
                inputExpression += key;
            } else if (key == 'c' || key == 'C') {
                inputExpression = "";
                resultString = "Result: ";
            }
        }
        
        // Handle special keys
        if (status.del && inputExpression.length() > 0) {
            inputExpression.remove(inputExpression.length() - 1);
        }
        
        if (status.enter) {
            if (inputExpression.length() > 0) {
                double result = evaluate(inputExpression);
                if (isnan(result)) {
                    resultString = "Error";
                } else {
                    resultString = "Result: " + String(result);
                }
            }
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
            M5Cardputer.Display.setFont(availableFonts[i].font);
            break;
        }
    }
    M5Cardputer.Display.setTextSize(textSize);

    drawUI();
}

void loop() {
    M5Cardputer.update();
    handleInput();

    // Handle cursor blinking
    if (millis() - last_cursor_blink > 500) {
        last_cursor_blink = millis();
        cursor_visible = !cursor_visible;
        drawUI();
    }
}
