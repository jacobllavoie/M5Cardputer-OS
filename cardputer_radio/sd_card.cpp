#ifdef ENABLE_SD_CARD
#include <Arduino.h> // Must be first for String and other Arduino types
#include "sd_card.h" // Include its own header
#include <SPI.h> // For SPI object
#include <SD.h>  // For SD object

#define SD_SPI_SCK_PIN  40
#define SD_SPI_MISO_PIN 39
#define SD_SPI_MOSI_PIN 14
#define SD_SPI_CS_PIN   12

void mountSD() {
    if (!isSdCardMounted) {
        SPI.begin(SD_SPI_SCK_PIN, SD_SPI_MISO_PIN, SD_SPI_MOSI_PIN);
        bool mounted = false;
        long freqs[] = {4000000, 10000000, 25000000};
        for (int i = 0; i < 3; ++i) {
            if (SD.begin(SD_SPI_CS_PIN, SPI, freqs[i])) {
                mounted = true;
                break;
            }
        }
        if (!mounted) {
            isSdCardMounted = false;
        } else {
            isSdCardMounted = true;
        }
    }
}

void unmountSD() {
    if (isSdCardMounted) {
        isSdCardMounted = false;
    }
}

void showSDCardInfo() {
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