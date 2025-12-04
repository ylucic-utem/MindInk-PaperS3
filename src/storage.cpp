#include "storage.h"
#include "config.h"
#include <SD.h>
#include <SPI.h>
#include "FS.h"

// ============================================================================
// GLOBAL STORAGE INFO
// ============================================================================
StorageInfo storage;

// ============================================================================
// STORAGE INITIALIZATION
// ============================================================================

void initStorage() {
    Serial.println("[STORAGE] Initializing storage systems...");
    
    storage.psramAvailable = psramFound();
    if (storage.psramAvailable) {
        storage.psramTotal = ESP.getPsramSize();
        storage.psramFree = ESP.getFreePsram();
        Serial.printf("[STORAGE] PSRAM: %d bytes total, %d free\n", storage.psramTotal, storage.psramFree);
    }
    
    SPI.begin(SD_SCK, SD_MISO, SD_MOSI, SD_CS);
    delay(300);
    
    storage.sdAvailable = SD.begin(SD_CS, SPI, 10000000);
    if (storage.sdAvailable) {
        Serial.println("[STORAGE] SD Card initialized");
        storage.activeStorage = STORAGE_SD;
    } else {
        Serial.println("[STORAGE] SD Card not found - using PSRAM");
        storage.activeStorage = storage.psramAvailable ? STORAGE_PSRAM : STORAGE_NONE;
    }
}

StorageInfo getStorageInfo() {
    return storage;
}
