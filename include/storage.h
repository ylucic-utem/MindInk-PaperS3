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
    // Cloud metadata (Supabase)
    String id;             // audio_records.id
    String storagePath;    // storage_path in bucket
    String summaryId;      // summaries.id linked
    String status;         // status column if present
    // Local cache info
    String localPath;      // Path on SD card
    bool isCached = false; // Whether file is downloaded locally
};

struct SummaryFile {
    String filename;
    String relatedAudioFilename;
    uint32_t duration = 0;
    // Cloud metadata
    String id;           // summaries.id
    String storagePath;  // summary_storage_path
    String status;       // status column if present
    // Local cache info
    String localPath;      // Path on SD card
    bool isCached = false; // Whether file is downloaded locally
};

struct ImageFile {
    String filename;
    String summaryFilename;
    uint32_t timestamp = 0;
    // Cloud metadata
    String id;           // infographics.id
    String storagePath;  // storage_path in bucket
    String summaryId;    // summaries.id link
    String status;       // optional status if added later
};

// ============================================================================
// STORAGE INITIALIZATION
// ============================================================================
extern StorageInfo storage;

void initStorage();
StorageInfo getStorageInfo();

/// Initialize SD card folder structure
/// Creates /audio, /summaries, /transcripts, /infographics folders
/// @return true if successful or folders already exist
bool initSDFolders();

/// Check if SD card is formatted (FAT32)
/// @return true if formatted, false if needs formatting
bool checkSDFormatted();

/// Format SD card (WARNING: erases all data)
/// @return true if successful
bool formatSDCard();

/// Get local cache path for audio file
/// @param audioId UUID from Supabase
/// @return Full path like "/audio/uuid.webm"
String getAudioCachePath(const String& audioId);

/// Get local cache path for transcript
/// @param audioId UUID from Supabase
/// @return Full path like "/transcripts/uuid.txt"
String getTranscriptCachePath(const String& audioId);

/// Get local cache path for summary
/// @param summaryId UUID from Supabase
/// @return Full path like "/summaries/uuid.txt"
String getSummaryCachePath(const String& summaryId);

/// Get local cache path for infographic
/// @param infographicId UUID from Supabase
/// @return Full path like "/infographics/uuid.png"
String getInfographicCachePath(const String& infographicId);

/// Check if audio is cached locally
bool isAudioCached(const String& audioId);

/// Check if transcript is cached locally
bool isTranscriptCached(const String& audioId);

/// Check if summary is cached locally
bool isSummaryCached(const String& summaryId);

/// List infographic files on SD card
/// @param infographics Vector to populate with found files
void listInfographicsOnSD(std::vector<String>& infographics);

#endif // STORAGE_H
