#ifdef ENABLE_SD_CARD
#include "globals.h"
#include "ui.h"

#define SD_SPI_SCK_PIN  40
#define SD_SPI_MISO_PIN 39
#define SD_SPI_MOSI_PIN 14
#define SD_SPI_CS_PIN   12

void mountSD() {
    debugMessage("DEBUG:", "mountSD() called");
    if (!isSdCardMounted) {
        SPI.begin(SD_SPI_SCK_PIN, SD_SPI_MISO_PIN, SD_SPI_MOSI_PIN);
        bool mounted = false;
        long freqs[] = {4000000, 10000000, 25000000};
        for (int i = 0; i < 3; ++i) {
            debugMessage("DEBUG:", "Trying SD.begin() freq " + String(freqs[i]));
            if (SD.begin(SD_SPI_CS_PIN, SPI, freqs[i])) {
                mounted = true;
                break;
            }
        }
        if (!mounted) {
            displayMessage("SD Card Mount Failed!");
            debugMessage("DEBUG:", "SD Card mount failed at all freqs");
            isSdCardMounted = false;
        } else {
            isSdCardMounted = true;
            // displayMessage("SD Card Mounted!");
            debugMessage("DEBUG:", "SD Card mounted successfully");
        }
    } else {
        displayMessage("Already Mounted");
    }
}

void unmountSD() {
    debugMessage("DEBUG:", "unmountSD() called");
    if (isSdCardMounted) {
        isSdCardMounted = false;
        displayMessage("SD Card Unmounted");
    } else {
        displayMessage("Already Unmounted");
    }
}

void showSDCardInfo() {
    debugMessage("DEBUG:", "showSDCardInfo() called");
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