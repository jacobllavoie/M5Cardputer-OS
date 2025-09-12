// This radio application is adapted from the M5Cardputer_WebRadio project.
// Original author: cyberwisk
// Original repository: https://github.com/cyberwisk/m5cardputer_webradio

// Converted from WebRadio_with_ESP8266Audio.ino for PlatformIO

// Dependencies: M5Unified, ESP8266Audio
// Place your WiFi credentials below unless using NVS storage
#define WIFI_SSID "SET YOUR WIFI SSID"
#define WIFI_PASS "SET YOUR WIFI PASS"

#include <WiFi.h>
#include <HTTPClient.h>
#include <math.h>
#include <M5Cardputer.h>
#include <esp_system.h>
#include <SD.h>
#include <Update.h>
#include <SPI.h> // Include SPI for SD card
#include <Preferences.h> // For NVS
#include <settings_manager.h>

#include <Audio.h> // ESP32-audioI2S version
#include <Adafruit_NeoPixel.h>
#include <sd_card.h> // Include SD card functions from M5Cardputer-OS
#include <load_launcher.h> // Include loadLauncher function


#ifdef ENABLE_FFT
#include <arduinoFFT.h>
#endif

Adafruit_NeoPixel led(1, 21, NEO_GRB + NEO_KHZ800);

#define I2S_BCK 41
#define I2S_WS 43
#define I2S_DOUT 42

#define MAX_STATIONS 25
#define MAX_NAME_LENGTH 30
#define MAX_URL_LENGTH 100

#ifdef ENABLE_FFT
// FFT Constants
#define FFT_SIZE 256
#define WAVE_SIZE 320 // This might not be directly used by arduinoFFT, but keeping for consistency

static double vReal[FFT_SIZE];
static double vImag[FFT_SIZE];
static ArduinoFFT<double>* FFT_ptr = nullptr;

static uint16_t prev_y[(FFT_SIZE / 2) + 1];
static uint16_t peak_y[(FFT_SIZE / 2) + 1];
static int header_height = 51; 
static bool fft_enabled = false;

static int16_t fft_audio_buffer[FFT_SIZE];
static volatile uint16_t fft_buffer_idx = 0;

static uint32_t bgcolor(int y) {
  auto h = M5Cardputer.Display.height();
  auto dh = h - header_height;
  int v = ((h - y) << 5) / dh;
  if (dh > header_height) {
    int v2 = ((h - y - 1) << 5) / dh;
    if ((v >> 2) != (v2 >> 2)) {
      return 0x666666u;
    }
  }
  return M5Cardputer.Display.color888(v + 2, v, v + 6);
}

void audio_process_i2s(uint32_t* sample, bool *continueI2S) {
  Serial.println("audio_process_i2s called");
  // Extract left channel (assuming 16-bit stereo, left channel is lower 16 bits)
  int16_t left_sample = (int16_t)(*sample & 0xFFFF);

  if (fft_buffer_idx < FFT_SIZE) {
    fft_audio_buffer[fft_buffer_idx++] = left_sample;
  }
}

void setupFFT() {
  if (!fft_enabled) return;
  // Initialize FFT arrays
  for (int x = 0; x < (FFT_SIZE / 2) + 1; ++x) {
    prev_y[x] = INT16_MAX;
    peak_y[x] = INT16_MAX;
  }
  // Draw the gradient background
  int display_height = M5Cardputer.Display.height();
  for (int y = header_height; y < display_height; ++y) {
    M5Cardputer.Display.drawFastHLine(0, y, M5Cardputer.Display.width(), bgcolor(y));
  }
}

void updateFFT() {
  if (!fft_enabled) return;

  static unsigned long lastFFTUpdate = 0;
  if (millis() - lastFFTUpdate < 50) return; // Update every 50ms
  lastFFTUpdate = millis();

  // Check if we have enough samples for FFT
  if (fft_buffer_idx >= FFT_SIZE) {
    // Copy samples to vReal and clear imaginary part
    for (int i = 0; i < FFT_SIZE; i++) {
      vReal[i] = fft_audio_buffer[i];
      vImag[i] = 0;
    }
    fft_buffer_idx = 0; // Reset buffer index

        FFT_ptr->compute(FFTDirection::Forward); // Perform FFT
        FFT_ptr->complexToMagnitude(); // Convert to magnitude

    // Parameters for drawing
    size_t bw = M5Cardputer.Display.width() / 30;
    if (bw < 3) bw = 3;
    int32_t dsp_height = M5Cardputer.Display.height();
    int32_t fft_height = dsp_height - header_height - 1;
    size_t xe = M5Cardputer.Display.width() / bw;
    if (xe > (FFT_SIZE / 2)) xe = (FFT_SIZE / 2);
    uint32_t bar_color[2] = {0x000033u, 0x99AAFFu};

    M5Cardputer.Display.startWrite();
    for (size_t bx = 0; bx <= xe; ++bx) {
      size_t x = bx * bw;
      int32_t f = vReal[bx] * 3; // Scale magnitude for display
      int32_t y = (f * fft_height) / (FFT_SIZE * 100); // Adjust scaling as needed
      if (y > fft_height) y = fft_height;
      y = dsp_height - y;

      int32_t py = prev_y[bx];
      if (y != py) {
        M5Cardputer.Display.fillRect(x, y, bw - 1, py - y, bar_color[(y < py)]);
        prev_y[bx] = y;
      }
      py = peak_y[bx] + ((peak_y[bx] - y) > 5 ? 2 : 1);
      if (py < y) {
        M5Cardputer.Display.writeFastHLine(x, py - 1, bw - 1, bgcolor(py - 1));
      } else {
        py = y - 1;
      }
      if (peak_y[bx] != py) {
        peak_y[bx] = py;
        M5Cardputer.Display.writeFastHLine(x, py, bw - 1, TFT_WHITE);
      }
    }
    M5Cardputer.Display.endWrite();
  }
}

void toggleFFT() {
  fft_enabled = !fft_enabled;
  M5Cardputer.Display.fillRect(0, 51, 240, 89, TFT_BLACK); // Clear FFT area
  if (fft_enabled) {
    setupFFT();
  }
}
#endif

Audio audio;


struct RadioStation {
  char name[MAX_NAME_LENGTH];
  char url[MAX_URL_LENGTH];
};

const PROGMEM RadioStation defaultStations[] = {
  {"Groove Salad", "http://ice5.somafm.com/groovesalad-128-mp3"},
  {"Lush", "http://ice5.somafm.com/lush-128-mp3"},
  {"Drone Zone", "http://ice5.somafm.com/dronezone-128-mp3"},
  {"The Trip", "http://ice5.somafm.com/thetrip-128-mp3"},
  {"Seven Inch Soul", "http://ice5.somafm.com/7soul-128-mp3"},
  {"Beat Blender", "http://ice5.somafm.com/beatblender-128-mp3"},
  {"Boot Liquor", "http://ice5.somafm.com/bootliquor-128-mp3"},
  {"Suburbs of Goa", "http://ice5.somafm.com/suburbsofgoa-128-mp3"},
  {"Digitalis", "http://ice1.somafm.com/digitalis-128-mp3"},
  {"Doomed", "http://ice5.somafm.com/doomed-128-mp3"},
  {"Fluid", "http://ice5.somafm.com/fluid-128-mp3"},
  {"Mission Control", "http://ice5.somafm.com/missioncontrol-128-mp3"},
  {"SF 10-33", "http://ice5.somafm.com/sf1033-128-mp3"},
  {"Black Rock FM", "http://ice5.somafm.com/brfm-128-mp3"},
  {"Bossa Beyond", "http://ice5.somafm.com/bossa-128-mp3"},
  {"Chillits Radio", "http://ice5.somafm.com/chillits-128-mp3"},
  {"cliqhop idm", "http://ice5.somafm.com/cliqhop-128-mp3"},
  {"Covers", "http://ice5.somafm.com/covers-128-mp3"},
  {"The Dark Zone", "http://ice5.somafm.com/darkzone-128-mp3"},
  {"Deep Space One", "http://ice5.somafm.com/deepspaceone-128-mp3"},
  {"Dub Step Beyond", "http://ice5.somafm.com/dubstep-128-mp3"}
};


RadioStation stations[MAX_STATIONS];
size_t numStations = 0;
size_t curStation = 3; // which radio to start with
uint16_t curVolume = 115;

unsigned long lastButtonPress = 0;
const unsigned long DEBOUNCE_DELAY = 200;

void showVolume() {
  static uint8_t lastVolume = 0;
  uint8_t currentVolume = curVolume;

  if (currentVolume != lastVolume) {
    lastVolume = currentVolume;

    int barHeight = 4; // Bar height
    M5Cardputer.Display.fillRect(0, 6, 200, 6, TFT_BLACK);
    int barWidth = map(currentVolume, 0, 200, 0, M5Cardputer.Display.width());
    if (barWidth < 200) {
      M5Cardputer.Display.fillRect(0, 6, barWidth, barHeight, 0xAAFFAA);
    }
  }
}

void showStation() {
  M5Cardputer.Display.fillRect(0, 15, 240, 35, TFT_BLACK);
  M5Cardputer.Display.drawString(stations[curStation].name, 0, 15);
  showVolume();
}

void Playfile() {
  led.setPixelColor(0, led.Color(255, 0, 0));
  led.show();
  audio.stopSong();

    String url = stations[curStation].url; // Stores the URL for easy access

    if (url.indexOf("http") != -1) {
        audio.connecttohost(stations[curStation].url);
    }
    else if (url.indexOf("/mp3") != -1) {
        M5Cardputer.Display.drawString("Play MP3 no SD /mp3    ", 0, 15);
        delay(4000);
        audio.connecttoFS(SD,stations[curStation].url);
    }
    else {
        audio.connecttospeech("And now, for the sound of silence.", "en");
    }
  showStation();
}

void volumeUp() {
  if (curVolume < 255) {
    curVolume = std::min(static_cast<uint16_t>(curVolume + 10), static_cast<uint16_t>(255));
    audio.setVolume(map(curVolume, 0, 255, 0, 21));
    showVolume();
  }
}

void volumeDown() {
  if (curVolume > 0) {
    curVolume = std::max(static_cast<uint16_t>(curVolume - 10), static_cast<uint16_t>(0));
    audio.setVolume(map(curVolume, 0, 255, 0, 21));
    showVolume();
  }
}

bool isMuted = false;
uint16_t prevVolume = 0; // To store the volume before muting

void volumeMute() {
  if (!isMuted) {
    prevVolume = curVolume;
    curVolume = 0;
    isMuted = true;
  } else {
    curVolume = prevVolume;
    isMuted = false;
  }
  audio.setVolume(map(curVolume, 0, 255, 0, 21));
  showVolume();
}

void stationUp() {
  if (numStations > 0) {
    curStation = (curStation + 1) % numStations;
    audio.stopSong();
    Playfile();
    showStation();
  }
  showVolume();
}

void stationDown() {
  if (numStations > 0) {
    curStation = (curStation - 1 + numStations) % numStations;
    audio.stopSong();
    Playfile();
    showStation();
  }
  showVolume();
}

void loadDefaultStations() {
  numStations = std::min(sizeof(defaultStations)/sizeof(defaultStations[0]), static_cast<size_t>(MAX_STATIONS));
  memcpy(stations, defaultStations, sizeof(RadioStation) * numStations);
}

void mergeRadioStations() {
  if (!isSdCardMounted) {
    led.setPixelColor(0, led.Color(255, 0, 0));
    led.show();
    M5Cardputer.Display.drawString("/station_list.txt ", 20, 30);
    M5Cardputer.Display.drawString("Not found on the SD", 20, 50);
    delay(4000);
    loadDefaultStations();
    M5Cardputer.Display.fillScreen(BLACK);
    return;
  }

  File file = SD.open("/station_list.txt");
  if (!file) {
    loadDefaultStations();
    return;
  }

  numStations = 0;

  String line;
  while (file.available() && numStations < MAX_STATIONS) {
    line = file.readStringUntil('\n');
    int commaIndex = line.indexOf(',');

    if (commaIndex > 0) {
      String name = line.substring(0, commaIndex);
      String url = line.substring(commaIndex + 1);

      name.trim();
      url.trim();

      if (name.length() > 0 && url.length() > 0) {
        strncpy(stations[numStations].name, name.c_str(), MAX_NAME_LENGTH - 1);
        strncpy(stations[numStations].url, url.c_str(), MAX_URL_LENGTH - 1);
        stations[numStations].name[MAX_NAME_LENGTH - 1] = '\0';
        stations[numStations].url[MAX_URL_LENGTH - 1] = '\0';
        numStations++;
      }
    }
  }

  file.close();
  if (numStations == 0) {
    loadDefaultStations();
  }
    led.setPixelColor(0, led.Color(0, 0, 0));
    led.show();
}

void updateBatteryDisplay(unsigned long updateInterval) {
  static unsigned long lastUpdate = 0;

  if (millis() - lastUpdate >= updateInterval) {
    lastUpdate = millis();

    int batteryLevel = M5.Power.getBatteryLevel();
    uint16_t batteryColor = batteryLevel < 30 ? TFT_RED : TFT_GREEN;

    M5Cardputer.Display.fillRect(215, 5, 40, 12, TFT_BLACK);

    M5Cardputer.Display.fillRect(215, 5, 20, 10, TFT_DARKGREY);
    M5Cardputer.Display.fillRect(235, 7, 3, 6, TFT_DARKGREY);
    M5Cardputer.Display.fillRect(217, 7, (batteryLevel * 16) / 100, 6, batteryColor);
  }
}

void audio_id3data(const char *info){M5Cardputer.Display.drawString(info, 0, 33);}

void audio_showstation(const char *showstation) {
    if (showstation && *showstation) { // Check if showstation is not null and not empty
        char limitedInfo[241];  // Buffer to hold limited string
        strncpy(limitedInfo, showstation, 24);  // Copy up to 24 characters
        limitedInfo[24] = '\0';  // Ensure null-termination
        M5Cardputer.Display.fillRect(0, 15, 240, 15, TFT_BLACK);
        M5Cardputer.Display.drawString(limitedInfo, 0, 15);
       
    }
}

void audio_showstreamtitle(const char *info) {
  static int xOffset = 0;                  // Current X position of the text
  static unsigned long lastUpdate = 0;     // Last update time
  const int updateInterval = 100;          // Update interval in milliseconds

  if (info && *info) {
    int textWidth = M5Cardputer.Display.textWidth(info);  // Get the width of the text

    // Scroll the text if it's wider than the display area
    if (textWidth > (240 - 21)) { // 240 is display width, 21 is starting X
      static unsigned long lastScrollStart = 0;
      static bool scrolling = false;

      if (!scrolling) {
        // Initial pause before scrolling starts
        if (millis() - lastUpdate > 2000) { // PAUSE_DURATION_MS
          scrolling = true;
          lastUpdate = millis();
          lastScrollStart = millis();
        }
      } else {
        if (millis() - lastUpdate > updateInterval) {
          lastUpdate = millis();
          xOffset--;  // Move text to the left

          // If text has scrolled completely out of view
          if (xOffset < -textWidth) {
            // Pause after scrolling off-screen
            if (millis() - lastScrollStart > (textWidth * updateInterval) + 2000) { // PAUSE_DURATION_MS
              xOffset = 240; // Start from right edge of display
              scrolling = false; // Reset for next cycle
            }
          }
        }
      }
    } else {
      xOffset = 21;  // Position text at SCROLL_START_X if it fits
    }

    // Clears the text area
    M5Cardputer.Display.fillRect(0, 33, 240, 15, TFT_BLACK);
    // Draws the text at the current position
    M5Cardputer.Display.drawString(info, xOffset, 33);
    // Decorative red line under the text
    M5Cardputer.Display.fillRect(0, 50, 240, 1, TFT_RED);
  }
}

// Function to generate a random color
uint32_t generateRandomColor() {
  return led.Color(random(256), random(256), random(256));
}

void setup() {

  auto cfg = M5.config();
  auto spk_cfg = M5Cardputer.Speaker.config();
    /// Increasing the sample_rate will improve the sound quality instead of increasing the CPU load.
    spk_cfg.sample_rate = 128000; // default:64000 (64kHz)  e.g. 48000 , 50000 , 80000 , 96000 , 100000 , 128000 , 144000 , 192000 , 200000
    spk_cfg.task_pinned_core = APP_CPU_NUM;
    M5Cardputer.Speaker.config(spk_cfg);

  M5Cardputer.begin(cfg, true);

  Serial.println("DEBUG: After M5.begin()"); // Keep debug statement
  led.begin();
  led.setBrightness(255);  // Brightness (0-255)
  led.show();  // Initialize off

  // WiFi connection logic (from your original code)
  M5Cardputer.Display.println("Connecting to WiFi");
  WiFi.disconnect();
  WiFi.softAPdisconnect(true);
  WiFi.mode(WIFI_STA);

  // --- Load Font Settings ---
  settings_init();
  String ssid = settings_get_wifi_ssid();
  String pass = settings_get_wifi_password();
  float menuTextSize = (float)settings_get_font_size() / 10.0f;
  String savedFontName = settings_get_font_name();
  for (int i = 0; i < numAvailableFonts; ++i) {
      if (savedFontName == availableFonts[i].name) {
          currentFontSelection = i;
          break;
      }
  }
  // -----------------------------

  M5Cardputer.Display.setRotation(1);
  M5Cardputer.Display.setFont(availableFonts[currentFontSelection].font);
  M5Cardputer.Display.setTextSize(menuTextSize);

  if (ssid.length() > 0) {
      WiFi.begin(ssid.c_str(), pass.c_str());
  }
  else {
      // Fallback to defines if NVS is empty
      WiFi.begin(WIFI_SSID, WIFI_PASS);
  }
  int wifiTries = 0;
  while (WiFi.status() != WL_CONNECTED && wifiTries < 60) {
      M5Cardputer.Display.print(".");
      delay(100);
      wifiTries++;
  }
  if (WiFi.status() == WL_CONNECTED) {
      M5Cardputer.Display.clear();
      M5Cardputer.Display.setTextColor(0x00FF00);
      M5Cardputer.Display.setTextSize(2);
      M5Cardputer.Display.setCursor(10, 10);
      M5Cardputer.Display.println("WiFi Connected!");
      M5Cardputer.Display.println("IP: " + WiFi.localIP().toString());
      Serial.println("WiFi connected, IP: " + WiFi.localIP().toString());
  }
  else {
      M5Cardputer.Display.clear();
      M5Cardputer.Display.setTextColor(0xFF0000);
      M5Cardputer.Display.setTextSize(2);
      M5Cardputer.Display.setCursor(10, 10);
      M5Cardputer.Display.println("WiFi Failed!");
      Serial.println("WiFi failed to connect.");
      delay(2000);
      return;
  }
  M5Cardputer.Display.clear();
  M5Cardputer.Display.setTextSize(menuTextSize);

  audio.setPinout(I2S_BCK, I2S_WS, I2S_DOUT);
  audio.setVolume(map(curVolume, 0, 255, 0, 21));
  audio.setBalance(0);

  M5Cardputer.Display.fillScreen(BLACK);

  if (!mountSD()) {
    M5Cardputer.Display.clear();
    M5Cardputer.Display.setTextColor(0xFF0000);
    M5Cardputer.Display.setTextSize(2);
    M5Cardputer.Display.setCursor(10, 10);
    M5Cardputer.Display.println("SD Card Mount Failed!");
    delay(2000);
    // Decide what to do here. Maybe just continue without SD functionality.
  }

  audio.stopSong();
  mergeRadioStations();
  Playfile();
  showStation();
  Serial.print("Audio Sample Rate: ");
  Serial.println(audio.getSampleRate());
#ifdef ENABLE_FFT
  FFT_ptr = new ArduinoFFT<double>(vReal, vImag, FFT_SIZE, audio.getSampleRate()); // Initialize FFT object
  if (FFT_ptr == nullptr) {
    Serial.println("FFT_ptr allocation failed!");
  } else {
    Serial.println("FFT_ptr allocated successfully.");
  }
  toggleFFT();
#endif
}

void loop() {
  audio.loop();
  M5Cardputer.update();
  updateBatteryDisplay(5000);

  static unsigned long lastLedChange = 0;
  const unsigned long LED_CHANGE_INTERVAL = 500; // Change color every 500ms

  if (millis() - lastLedChange > LED_CHANGE_INTERVAL) {
    lastLedChange = millis();
    led.setPixelColor(0, generateRandomColor());
    led.show();
  }

  if (M5Cardputer.Keyboard.isChange() && (millis() - lastButtonPress > DEBOUNCE_DELAY)) {
    led.setPixelColor(0, led.Color(120, 0, 255));
    led.show();

    Keyboard_Class::KeysState status = M5Cardputer.Keyboard.keysState();
    char key = 0; // Initialize key to null character

    if (status.fn && !status.word.empty() && status.word[0] == '`') {
        const esp_partition_t *launcher_partition = esp_partition_find_first(ESP_PARTITION_TYPE_APP, ESP_PARTITION_SUBTYPE_APP_OTA_0, NULL);
        if (launcher_partition != NULL) {
          esp_ota_set_boot_partition(launcher_partition);
          esp_restart();
        }
    }

    if (!status.word.empty()) {
      key = status.word[0]; // Get the first character from the word vector
    }

    Serial.print("Key pressed: ");
    Serial.println(key); // Debugging output

    switch (key) {
      case ';':
        volumeUp();
        break;
      case '.':
        volumeDown();
        break;
      case 'm':
        volumeMute();
        break;
      case '/':
        stationUp();
        break;
      case ',':
        stationDown();
        break;
      case 'r':
        audio.stopSong();
        audio.connecttohost(stations[curStation].url);
        break;
      case 'p':
        M5Cardputer.Display.fillRect(0, 15, 240, 49, TFT_BLACK);
        M5Cardputer.Display.drawString("Play MP3 no SD /mp3    ", 0, 15);
        audio.stopSong();
        audio.connecttoFS(SD,"/mp3/Default.mp3");
        break;
      case 'o':
        M5Cardputer.Display.fillRect(0, 15, 240, 49, TFT_BLACK);
        M5Cardputer.Display.drawString("PlayFile", 0, 15);
        Playfile();
        break;
      case 's':
        audio.stopSong();
        audio.connecttospeech("And now, for the sound of silence.", "en");
        break;
#ifdef ENABLE_FFT
      case 'f':
        toggleFFT();
        break;
#endif
      default:
        // Handle other keys or do nothing
        break;
    }

    lastButtonPress = millis();
   }

#ifdef ENABLE_FFT
  if (fft_enabled) {
    updateFFT();
  }
#endif

  delay(1);
    led.setPixelColor(0, led.Color(0, 0, 0)); // Azul
    led.show();  // Initializes off
    led.show();  // Updates the LED
}
