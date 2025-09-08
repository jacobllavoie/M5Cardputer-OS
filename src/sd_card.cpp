#ifdef ENABLE_SD_CARD
#include "globals.h"
#include "ui.h"

#define SD_SPI_SCK_PIN  40
#define SD_SPI_MISO_PIN 39
#define SD_SPI_MOSI_PIN 14
#define SD_SPI_CS_PIN   12

void mountSD() {
    if (!isSdCardMounted) {
        if (!sd.begin(SD_SPI_CS_PIN, SD_SCK_MHZ(25))) {
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
        sd.end();
        isSdCardMounted = false;
        displayMessage("SD Card Unmounted");
    } else {
        displayMessage("Already Unmounted");
    }
}

void showSDCardInfo() {
    if (isSdCardMounted) {
        uint32_t cardSizeMB = sd.card()->sectorCount() * 512E-6;
        displayMessage("Type: SDHC/SDXC", "Size: " + String(cardSizeMB) + "MB");
    } else {
        displayMessage("SD Card Not Mounted");
    }
}
#endif