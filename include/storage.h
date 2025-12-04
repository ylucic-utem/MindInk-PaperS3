#ifndef STORAGE_H
#define STORAGE_H

#include <vector>
#include <WString.h>

// ============================================================================
// DATA STRUCTURES
// ============================================================================
enum StorageType {
    STORAGE_SD,
    STORAGE_PSRAM,
    STORAGE_NONE
};

struct StorageInfo {
    bool sdAvailable;
    bool psramAvailable;
    size_t psramTotal;
    size_t psramFree;
    StorageType activeStorage;
};

struct AudioFile {
    String filename;
    uint32_t fileSize = 0;
    uint32_t duration = 0;  // milliseconds
    uint32_t sampleRate = 0;
    uint16_t bitDepth = 0;
    uint32_t timestamp = 0;
};

struct SummaryFile {
    String filename;
    String relatedAudioFilename;
    uint32_t duration = 0;
};

struct ImageFile {
    String filename;
    String summaryFilename;
    uint32_t timestamp = 0;
};

// ============================================================================
// STORAGE INITIALIZATION
// ============================================================================
extern StorageInfo storage;

void initStorage();
StorageInfo getStorageInfo();

#endif // STORAGE_H
