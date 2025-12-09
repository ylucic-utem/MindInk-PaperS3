#include "storage.h"
#include "config.h"
#include <SD.h>
#include <SPI.h>
#include "FS.h"
#include <SD_MMC.h>

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
        
        // Check if SD card is formatted
        if (!checkSDFormatted()) {
            Serial.println("[STORAGE] WARNING: SD Card appears unformatted!");
            Serial.println("[STORAGE] Please format SD card as FAT32");
            Serial.println("[STORAGE] You can format via: Settings > Format SD Card");
        } else {
            // Initialize folder structure
            if (initSDFolders()) {
                Serial.println("[STORAGE] SD folder structure ready");
            } else {
                Serial.println("[STORAGE] WARNING: Could not create folder structure");
            }
        }
        
        storage.activeStorage = STORAGE_SD;
    } else {
        Serial.println("[STORAGE] SD Card not found - using PSRAM");
        storage.activeStorage = storage.psramAvailable ? STORAGE_PSRAM : STORAGE_NONE;
    }
}

StorageInfo getStorageInfo() {
    return storage;
}

// ============================================================================
// SD CARD FOLDER MANAGEMENT
// ============================================================================

bool initSDFolders() {
    if (!storage.sdAvailable) return false;
    
    const char* folders[] = {
        "/audio",
        "/summaries", 
        "/transcripts",
        "/infographics",
        "/recordings"
    };
    
    bool allSuccess = true;
    for (const char* folder : folders) {
        if (!SD.exists(folder)) {
            Serial.printf("[STORAGE] Creating folder: %s\n", folder);
            if (!SD.mkdir(folder)) {
                Serial.printf("[STORAGE] ERROR: Failed to create %s\n", folder);
                allSuccess = false;
            }
        } else {
            Serial.printf("[STORAGE] Folder exists: %s\n", folder);
        }
    }
    
    return allSuccess;
}

bool checkSDFormatted() {
    if (!storage.sdAvailable) return false;
    
    // Try to open root directory
    File root = SD.open("/");
    if (!root) {
        Serial.println("[STORAGE] Cannot open root directory");
        return false;
    }
    
    if (!root.isDirectory()) {
        root.close();
        Serial.println("[STORAGE] Root is not a directory");
        return false;
    }
    
    root.close();
    
    // Try to create a test file
    File test = SD.open("/.format_test", FILE_WRITE);
    if (!test) {
        Serial.println("[STORAGE] Cannot create test file - likely unformatted");
        return false;
    }
    test.write((uint8_t*)"test", 4);
    test.close();
    
    // Try to read it back
    test = SD.open("/.format_test");
    if (!test) {
        Serial.println("[STORAGE] Cannot read test file");
        SD.remove("/.format_test");
        return false;
    }
    test.close();
    SD.remove("/.format_test");
    
    Serial.println("[STORAGE] SD Card is properly formatted");
    return true;
}

bool formatSDCard() {
    Serial.println("[STORAGE] WARNING: Formatting SD card will erase ALL data!");
    Serial.println("[STORAGE] This operation is not implemented for safety.");
    Serial.println("[STORAGE] Please format SD card using a computer (FAT32 format).");
    return false;
}

// ============================================================================
// CACHE PATH HELPERS
// ============================================================================

String getAudioCachePath(const String& audioId) {
    return "/audio/" + audioId + ".webm";
}

String getTranscriptCachePath(const String& audioId) {
    return "/transcripts/" + audioId + ".txt";
}

String getSummaryCachePath(const String& summaryId) {
    return "/summaries/" + summaryId + ".txt";
}

String getInfographicCachePath(const String& infographicId) {
    return "/infographics/" + infographicId + ".png";
}

bool isAudioCached(const String& audioId) {
    if (!storage.sdAvailable) return false;
    return SD.exists(getAudioCachePath(audioId));
}

bool isTranscriptCached(const String& audioId) {
    if (!storage.sdAvailable) return false;
    return SD.exists(getTranscriptCachePath(audioId));
}

bool isSummaryCached(const String& summaryId) {
    if (!storage.sdAvailable) return false;
    return SD.exists(getSummaryCachePath(summaryId));
}
