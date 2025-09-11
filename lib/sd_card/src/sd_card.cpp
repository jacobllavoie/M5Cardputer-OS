#ifdef ENABLE_SD_CARD
#include <Arduino.h>
#include "sd_card.h"
#include <SPI.h>
#include <SD.h>



#define SD_SPI_SCK_PIN  40
#define SD_SPI_MISO_PIN 39
#define SD_SPI_MOSI_PIN 14
#define SD_SPI_CS_PIN   12

bool mountSD() {
    if (isSdCardMounted) {
        return true;
    }

    SPI.begin(SD_SPI_SCK_PIN, SD_SPI_MISO_PIN, SD_SPI_MOSI_PIN);
    long freqs[] = {4000000, 10000000, 25000000};
    for (int i = 0; i < 3; ++i) {
        if (SD.begin(SD_SPI_CS_PIN, SPI, freqs[i])) {
            isSdCardMounted = true;
            return true;
        }
    }
    isSdCardMounted = false;
    return false;
}

void unmountSD() {
    if (isSdCardMounted) {
        SD.end();
        isSdCardMounted = false;
    }
}

String getSDCardInfo() {
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
        return "Type: " + cardType + ", Size: " + String(cardSize) + "MB";
    } else {
        return "SD Card Not Mounted";
    }
}

bool mountSDforMSC() {
    if (isSdCardMounted) {
        return true;
    }
    SPI.begin(SD_SPI_SCK_PIN, SD_SPI_MISO_PIN, SD_SPI_MOSI_PIN);
    if (SD.begin(SD_SPI_CS_PIN, SPI, 4000000)) {
        isSdCardMounted = true;
        return true;
    } else {
        isSdCardMounted = false;
        return false;
    }
}
#endif