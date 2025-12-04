#ifndef STORAGE_H
#define STORAGE_H

#include <vector>
#include <String.h>

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
    String dateTime;
    StorageType storage;
};

struct SummaryFile {
    String filename;
    String relatedAudioFilename;
    String dateTime;
    StorageType storage;
};

struct ImageFile {
    String originalFilename;
    String summaryFilename;
    String dateTime;
    StorageType storage;
};

// ============================================================================
// STORAGE INITIALIZATION
// ============================================================================
extern StorageInfo storage;

void initStorage();
StorageInfo getStorageInfo();

#endif // STORAGE_H
