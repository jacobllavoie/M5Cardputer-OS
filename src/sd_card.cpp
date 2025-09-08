#ifdef ENABLE_SD_CARD
#include "globals.h"
#include "ui.h"

#define SD_SPI_SCK_PIN  40
#define SD_SPI_MISO_PIN 39
#define SD_SPI_MOSI_PIN 14
#define SD_SPI_CS_PIN   12

void mountSD() {
    if (!isSdCardMounted) {
        // Updated sd.begin() call with frequency in Hz
        if (!SD.begin(SD_SPI_CS_PIN, SPI, 25000000)) {
            displayMessage("SD Card Mount Failed!");
            isSdCardMounted = false;
        } else {
            isSdCardMounted = true;
            displayMessage("SD Card Mounted!");
        }
    } else {
        displayMessage("Already Mounted");
    }
}

void unmountSD() {
    if (isSdCardMounted) {
        // SD.end() doesn't exist in the standard library for ESP32,
        // so we just update the flag.
        isSdCardMounted = false;
        displayMessage("SD Card Unmounted");
    } else {
        displayMessage("Already Unmounted");
    }
}

void showSDCardInfo() {
    if (isSdCardMounted) {
        // Updated function calls for card size and type
        uint64_t cardSize = SD.cardSize() / (1024 * 1024);
        String cardType = "";
        switch(SD.cardType()){
            case CARD_MMC: cardType = "MMC"; break;
            case CARD_SD: cardType = "SDSC"; break;
            case CARD_SDHC: cardType = "SDHC"; break;
            case CARD_UNKNOWN:
            default: cardType = "Unknown"; break;
        }
        displayMessage("Type: " + cardType, "Size: " + String(cardSize) + "MB");
    } else {
        displayMessage("SD Card Not Mounted");
    }
}
#endif