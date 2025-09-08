#ifdef ENABLE_USB_MSC
#include "globals.h"
#include "ui.h"
#include "USB.h"
#include "USBMSC.h"
#include "SD.h"

USBMSC MSC;

// Callback for when the host reads from the device
int32_t onRead(uint32_t lba, uint32_t offset, void* buffer, uint32_t bufsize) {
    return SD.readRAW((uint8_t*)buffer, lba) ? bufsize : -1;
}

// Callback for when the host writes to the device
int32_t onWrite(uint32_t lba, uint32_t offset, uint8_t* buffer, uint32_t bufsize) {
    return SD.writeRAW((uint8_t*)buffer, lba) ? bufsize : -1;
}

// Callback for when the host connects or disconnects from the device
bool onStartStop(uint8_t power_condition, bool start, bool load_eject) {
    if (start) {
        debugMessage("MSC", "MSC Start");
    } else {
        debugMessage("MSC", "MSC Stop");
    }
    return true;
}

void startMSC() {
    if (isSdCardMounted) {
        MSC.onRead(onRead);
        MSC.onWrite(onWrite);
        MSC.onStartStop(onStartStop);
        MSC.begin(SD.cardSize() / 512, 512);
        USB.begin();
    } else {
        displayMessage("SD Card Not Mounted", "Mount SD first");
    }
}

void stopMSC() {
    MSC.end();
}

#endif
