#ifdef ENABLE_SD_CARD
#include "globals.h"
#include "ui.h"

#define SD_SPI_SCK_PIN  40
#define SD_SPI_MISO_PIN 39
#define SD_SPI_MOSI_PIN 14
#define SD_SPI_CS_PIN   12

void mountSD() {
    #ifdef DEBUG_MODE
    Serial.println("DEBUG: mountSD() called");
    #endif
    if (!isSdCardMounted) {
        SPI.begin(SD_SPI_SCK_PIN, SD_SPI_MISO_PIN, SD_SPI_MOSI_PIN);

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
    #ifdef DEBUG_MODE
    Serial.println("DEBUG: unmountSD() called");
    #endif
    if (isSdCardMounted) {
        isSdCardMounted = false;
        displayMessage("SD Card Unmounted");
    } else {
        displayMessage("Already Unmounted");
    }
}

void showSDCardInfo() {
    #ifdef DEBUG_MODE
    Serial.println("DEBUG: showSDCardInfo() called");
    #endif
    if (isSdCardMounted) {
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