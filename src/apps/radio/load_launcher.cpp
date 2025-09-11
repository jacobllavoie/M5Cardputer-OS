#include <Arduino.h>
#include <SD.h>
#include <Update.h>
#include <esp_partition.h>
#include "esp_ota_ops.h" // <-- ADD THIS LINE

#if defined(ENABLE_SD_CARD)
void loadLauncher() {
    const char* appPath = "/launcher.bin"; // Hardcoded path to the launcher
    if (!SD.exists(appPath)) {
        return;
    }

    File appFile = SD.open(appPath);
    if (!appFile) {
        return;
    }

    size_t appSize = appFile.size();
    if (appSize == 0) {
        appFile.close();
        return;
    }

    const esp_partition_t* update_partition = esp_ota_get_next_update_partition(NULL);
    if (update_partition == NULL) {
        appFile.close();
        return;
    }
    
    Serial.printf("Flashing to partition: %s\n", update_partition->label);

    if (!Update.begin(appSize, 0, -1, -1, update_partition->label)) {
        appFile.close();
        return;
    }
    
    uint8_t buffer[4096];
    size_t bytesRead = 0;
    while ((bytesRead = appFile.read(buffer, sizeof(buffer))) > 0) {
        if (Update.write(buffer, bytesRead) != bytesRead) {
            appFile.close();
            Update.abort();
            return;
        }
        yield(); 
    }
    appFile.close();

    if (!Update.end()) {
        return;
    }
    
    if (esp_ota_set_boot_partition(update_partition) != ESP_OK) {
        return;
    }

    ESP.restart();
}
#endif