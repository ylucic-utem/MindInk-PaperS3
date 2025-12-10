# M5Stack PaperS3 Local Audio Processing Implementation Guide

## Overview

This guide explains how to use the newly integrated **local audio processing workflow** on your M5Stack PaperS3 device. The system allows you to:

1. **Record audio** directly on the device microphone
2. **Transcribe audio** using ElevenLabs API (on-device via API call)
3. **Summarize transcriptions** using Google Gemini API
4. **Store all files** on the SD card with proper organization
5. **Sync results** with Supabase database and storage

## Architecture

### Module Structure

```
include/
├── local_processing.h      ← New: Complete workflow orchestration
├── api.h                   ← Updated: Transcription & summarization
├── audio.h                 ← Recording interface
├── storage.h               ← SD card management
└── supabase_client.h       ← Database & storage sync

src/
├── local_processing.cpp    ← New: Full implementation
├── api.cpp                 ← Updated with API integrations
└── main.cpp                ← Updated: Initialize local processing
```

### Data Flow

```
Device Microphone
    ↓
[Record Audio] → SD Card (/audio/)
    ↓
[Transcribe] → ElevenLabs API (scribe_v2) → SD Card (/transcripts/)
    ↓
[Summarize] → Gemini API (2.5 Flash Lite) → SD Card (/summaries/)
    ↓
[Upload] → Supabase (database + storage buckets)
    ↓
Display Menu (Read Summaries)
```

### SD Card Folder Structure

```
/
├── audio/              ← Raw audio recordings (.wav)
│   ├── rec_1733708800.wav
│   ├── rec_1733708900.wav
│   └── ...
├── transcripts/        ← Transcribed text
│   ├── 1733708800.txt
│   ├── 1733708900.txt
│   └── ...
├── summaries/          ← Summarized text
│   ├── summary_1733708800.txt
│   ├── summary_1733708900.txt
│   └── ...
└── infographics/       ← Generated images (if enabled)
    ├── graphic_1733708800.png
    └── ...
```

## Key Functions

### 1. Initialize Local Processing

```cpp
void setup() {
    // ... other initialization ...
    
    // Initialize local processing system
    initLocalProcessing();  // Creates SD card folders
    
    // ... rest of setup ...
}
```

### 2. Record and Process Audio (Complete Workflow)

```cpp
// In your menu or button handler:
AudioFile myAudio;
if (recordAndProcessAudio(30, myAudio)) {  // Record 30 seconds
    // Success! Audio recorded, transcribed, and summarized
    Serial.printf("✓ Processing complete\n");
    Serial.printf("  Audio: %s\n", myAudio.filename.c_str());
    Serial.printf("  ID: %s\n", myAudio.id.c_str());
} else {
    // Failed - check error message
    ProcessingProgress progress = getProcessingProgress();
    Serial.printf("✗ Error: %s\n", progress.errorMessage.c_str());
}
```

### 3. Check Processing Status

```cpp
ProcessingProgress status = getProcessingProgress();

Serial.printf("State: %d\n", status.state);           // 0-6
Serial.printf("Message: %s\n", status.statusMessage.c_str());
Serial.printf("Progress: %d%%\n", status.percentComplete);
Serial.printf("Step: %s\n", status.currentStep.c_str());

// States:
// 0 = PROCESSING_IDLE
// 1 = PROCESSING_RECORDING
// 2 = PROCESSING_TRANSCRIBING
// 3 = PROCESSING_SUMMARIZING
// 4 = PROCESSING_UPLOADING
// 5 = PROCESSING_COMPLETE
// 6 = PROCESSING_ERROR
```

### 4. Get Local Audio Files

```cpp
std::vector<AudioFile> localAudios;
if (getLocalAudioFiles(localAudios)) {
    for (const auto& audio : localAudios) {
        Serial.printf("File: %s (%d bytes)\n", 
                      audio.filename.c_str(), 
                      audio.fileSize);
    }
}
```

### 5. Get Local Summaries

```cpp
std::vector<SummaryFile> localSummaries;
if (getLocalSummaries(localSummaries)) {
    for (const auto& summary : localSummaries) {
        Serial.printf("Summary: %s\n", summary.filename.c_str());
        
        // Read the summary text from SD
        File f = SD.open(summary.localPath);
        while (f.available()) {
            Serial.write(f.read());
        }
        f.close();
    }
}
```

### 6. Delete Audio and Associated Files

```cpp
String audioId = "1733708800";
if (deleteLocalAudio(audioId)) {
    Serial.println("✓ Audio and summaries deleted");
} else {
    Serial.println("✗ Failed to delete audio");
}
```

## API Integration Details

### ElevenLabs Transcription

**Model:** `scribe_v2` (latest speech-to-text model)

**Usage:**
```cpp
TranscriptionResult result = transcribeAudio(audioFile.localPath);

if (result.success) {
    Serial.printf("Transcript:\n%s\n", result.text.c_str());
} else {
    Serial.printf("Error: %s (HTTP %d)\n", 
                  result.error.c_str(), 
                  result.httpCode);
}
```

**Supported Formats:** WAV, MP3, M4A, OGG, FLAC, PCM

### Google Gemini Summarization

**Model:** `gemini-2.5-flash-lite` (fast, cost-effective)

**Usage:**
```cpp
SummaryResult result = summarizeText(transcript, 500);  // Max 500 chars

if (result.success) {
    Serial.printf("Summary:\n%s\n", result.text.c_str());
} else {
    Serial.printf("Error: %s (HTTP %d)\n", 
                  result.error.c_str(), 
                  result.httpCode);
}
```

## Database Schema

### Supabase Tables

#### audio_records
```sql
id              UUID PRIMARY KEY
file_name       TEXT NOT NULL
storage_path    TEXT NOT NULL
recording_date  TIMESTAMP NOT NULL
summary_id      UUID (FK to summaries)
status          TEXT ('pending', 'processing', 'completed', 'error')
error_message   TEXT (optional)
```

#### summaries
```sql
id                   UUID PRIMARY KEY
audio_id            UUID NOT NULL (FK to audio_records)
transcription_text  TEXT
summary_storage_path TEXT
infographic_id      UUID (FK to infographics)
status              TEXT ('pending', 'processing', 'completed', 'error')
error_message       TEXT (optional)
```

#### transcriptions
```sql
-- Optional table for detailed transcription data
audio_id        UUID (FK to audio_records)
text            TEXT
language        TEXT
confidence      FLOAT
```

## Implementation Checklist

- [x] Create `local_processing.h` header with function declarations
- [x] Implement `local_processing.cpp` with complete workflow
- [x] Update `main.cpp` to include and initialize local processing
- [x] Update `api.h/cpp` for ElevenLabs transcription (scribe_v2)
- [x] Update `api.h/cpp` for Gemini summarization
- [x] Ensure `storage.h` handles SD card folder creation
- [ ] **TODO:** Implement UI menu system for recording control
- [ ] **TODO:** Implement UI to display summaries on e-ink display
- [ ] **TODO:** Implement Supabase database inserts
- [ ] **TODO:** Implement Supabase file uploads
- [ ] **TODO:** Add error recovery and retry logic
- [ ] **TODO:** Add progress display on device screen

## Next Steps

### 1. Implement Recording UI Menu

Add to `display.cpp`:
```cpp
void recordingMenu() {
    M5.Display.clear();
    M5.Display.setTextSize(2);
    M5.Display.drawString("RECORDING MENU", 10, 10);
    M5.Display.setTextSize(1);
    
    // Show options:
    // [A] Record 10s
    // [B] Record 30s
    // [C] Record 60s
    // [D] Back
    
    while (true) {
        M5.update();
        
        if (M5.BtnA.wasPressed()) {
            AudioFile audio;
            recordAndProcessAudio(10, audio);
            break;
        }
        // ... handle other buttons
    }
}
```

### 2. Implement Summary Viewer

```cpp
void summaryMenu() {
    std::vector<SummaryFile> summaries;
    getLocalSummaries(summaries);
    
    M5.Display.clear();
    M5.Display.setTextSize(1);
    
    for (size_t i = 0; i < summaries.size(); i++) {
        M5.Display.printf("[%d] %s\n", i, summaries[i].filename.c_str());
    }
    
    // Let user select and view summary...
}
```

### 3. Implement Supabase Sync

In `local_processing.cpp`, implement the TODO sections:
```cpp
bool uploadProcessingResults(const AudioFile& audioFile, 
                            const String& transcript, 
                            const String& summary) {
    // 1. Insert into audio_records
    // 2. Insert into summaries
    // 3. Upload files to storage buckets
    // 4. Update Supabase status
}
```

### 4. Add WiFi Connectivity Check

Before processing:
```cpp
if (!isWiFiConnected()) {
    M5.Display.drawString("WiFi not connected!", 10, 100);
    return false;
}
```

## Troubleshooting

### Microphone Not Recording
- Check M5Unified audio configuration in `audio.h`
- Verify I2S pins for your specific M5 variant
- Test with `initAudio()` and `startAudioRecording()`

### ElevenLabs API Fails
- Verify API key in `.env` (for device, use `config.h`)
- Check model is `scribe_v2` (not text-to-speech models)
- Ensure WAV file is valid format
- Check API usage limits/quota

### Gemini API Fails
- Verify API key in `config.h`
- Check model is `gemini-2.5-flash-lite`
- Ensure text length is reasonable (<100KB)
- Check API usage limits/quota

### SD Card Not Working
- Verify SD card is FAT32 formatted
- Check `initSDFolders()` creates all required directories
- Monitor free space on SD card
- Verify SD card pins in M5 config

### Supabase Upload Fails
- Check WiFi connection
- Verify Supabase credentials in `config.h`
- Check table permissions and RLS policies
- Verify storage bucket names match code

## Configuration Files

### `config.h`
```cpp
#define SUPABASE_URL "https://fptyrdjmrzabvimbzupv.supabase.co"
#define SUPABASE_API_KEY "eyJ..."
#define ELEVEN_LABS_API_KEY "sk_..."
#define GEMINI_API_KEY "AIza..."

// Audio settings
#define AUDIO_SAMPLE_RATE 16000
#define AUDIO_CHANNELS 1
#define AUDIO_MAX_RECORDING_SECONDS 60
```

## Testing

Test the complete workflow:

```cpp
void testLocalProcessing() {
    Serial.println("\n=== Testing Local Processing Workflow ===\n");
    
    // Initialize
    initLocalProcessing();
    initWiFi();
    
    // Record and process
    AudioFile audio;
    if (recordAndProcessAudio(10, audio)) {
        Serial.println("✓ Workflow complete!");
        
        // Read results
        File summaryFile = SD.open(getSummaryCachePath(audio.id));
        if (summaryFile) {
            Serial.println("Summary:");
            while (summaryFile.available()) {
                Serial.write(summaryFile.read());
            }
            summaryFile.close();
        }
    } else {
        ProcessingProgress p = getProcessingProgress();
        Serial.printf("✗ Failed: %s\n", p.errorMessage.c_str());
    }
}
```

## Performance Notes

- **Recording:** 30-60 seconds recommended (avoid max 60s limit)
- **Transcription:** Typical 10-30 seconds of audio takes 10-20 seconds
- **Summarization:** Typical 100-1000 char transcript takes 5-15 seconds
- **Total Time:** ~30-60 seconds for complete workflow
- **Storage:** ~50KB per minute of audio, ~1KB per 100 chars of text

## References

- M5Unified Documentation: https://github.com/m5stack/M5Unified
- ElevenLabs API Docs: https://api.elevenlabs.io/docs
- Google Gemini API: https://ai.google.dev/docs
- Supabase Docs: https://supabase.com/docs
