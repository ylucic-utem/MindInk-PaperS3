# MindInk Local Processing Architecture

## Overview

The MindInk PaperS3 device now supports **local processing** using the SD card for caching and intermediate storage. This eliminates the need for Supabase Edge Functions to handle API calls, as the device directly communicates with ElevenLabs and Gemini APIs.

## Architecture Comparison

### OLD: Cloud-Based Processing (❌ Had Issues)
```
Device → Upload Audio → Supabase Storage
                    ↓
            Edge Function (Supabase)
                    ↓
              ElevenLabs API ← (Required env vars in Supabase)
                    ↓
              Gemini API
                    ↓
            Save to Database/Storage
                    ↓
Device ← Poll for Results ← Supabase
```

**Problems:**
- Edge Function environment variables not set properly
- No visibility into background processing failures
- Device behind NAT can't receive webhooks
- Complex debugging across multiple systems

### NEW: Local Processing (✅ Recommended)
```
Device → Fetch Audio List → Supabase REST API
   ↓
Download Audio to SD Card (/audio/uuid.webm)
   ↓
Read Audio from SD Card
   ↓
Send Audio → ElevenLabs API (Direct from device!)
   ↓
Save Transcript to SD Card (/transcripts/uuid.txt)
   ↓
Read Transcript from SD Card
   ↓
Send Transcript → Gemini API (Direct from device!)
   ↓
Save Summary to SD Card (/summaries/uuid.txt)
   ↓
(Optional) Upload Results → Supabase Storage/Database
   ↓
Display Summary on E-Ink Screen
```

**Advantages:**
- ✅ Device has full control over processing
- ✅ All results cached locally on SD card
- ✅ Works offline after initial download
- ✅ Easy debugging via Serial Monitor
- ✅ No Edge Function env var issues
- ✅ Faster response (no Edge Function overhead)
- ✅ Local files can be accessed for playback/display

## SD Card Folder Structure

```
/
├── audio/                   # Downloaded audio from Supabase
│   ├── uuid1.webm
│   ├── uuid2.webm
│   └── uuid3.webm
│
├── transcripts/             # Transcripts from ElevenLabs
│   ├── uuid1.txt
│   ├── uuid2.txt
│   └── uuid3.txt
│
├── summaries/               # Summaries from Gemini
│   ├── uuid1_summary.txt
│   ├── uuid2_summary.txt
│   └── uuid3_summary.txt
│
├── infographics/            # Generated images (future)
│   ├── uuid1.png
│   └── uuid2.png
│
└── recordings/              # Audio recorded on device
    ├── recording_20251208_143000.wav
    └── recording_20251208_143500.wav
```

## Processing Workflow

### Step 1: Fetch Audio List
```cpp
std::vector<AudioFile> audioList;
if (fetchSupabaseAudio(audioList)) {
    // Display list to user
    // User selects audio to process
}
```

### Step 2: Download Audio (if not cached)
```cpp
String audioId = "62cd9c95-51d2-434e-8baf-1eeef165a6a0";

if (!isAudioCached(audioId)) {
    AudioFile targetAudio = findAudioById(audioList, audioId);
    String localPath;
    downloadAudioFromSupabase(targetAudio, localPath);
    // Now localPath = "/audio/62cd9c95-51d2-434e-8baf-1eeef165a6a0.webm"
}
```

### Step 3: Transcribe Audio
```cpp
String localAudioPath = getAudioCachePath(audioId);
TranscriptionResult result = transcribeAudio(localAudioPath);

if (result.success) {
    Serial.println(result.text);
    saveTranscriptLocally(audioId, result.text);
}
```

### Step 4: Summarize Transcript
```cpp
String transcript;
readTranscriptLocally(audioId, transcript);

SummaryResult summary = summarizeText(transcript);

if (summary.success) {
    Serial.println(summary.text);
    saveSummaryLocally(audioId + "_summary", summary.text);
}
```

### Step 5: Display Summary
```cpp
String summaryText;
readSummaryLocally(audioId + "_summary", summaryText);

// Display on E-Ink screen
M5.Display.clear();
M5.Display.setCursor(10, 10);
M5.Display.println(summaryText);
```

## Complete Workflow (Single Function Call)

```cpp
// Process everything in one call
bool success = processAudioLocally(audioId);

if (success) {
    // Audio downloaded ✓
    // Transcript saved ✓
    // Summary saved ✓
    // All cached on SD card ✓
}
```

## API Requirements

### ElevenLabs API Key
- Get from: https://elevenlabs.io/app/settings/api-keys
- Format: `sk_...`
- Set in: `src/config.cpp` → `ELEVEN_LABS_API_KEY`

### Google Gemini API Key
- Get from: https://aistudio.google.com/apikey
- Format: `AIza...`
- Set in: `src/config.cpp` → `GEMINI_API_KEY`

### Supabase Keys (for audio download)
- Anon Key: Already configured
- URL: `https://fptyrdjmrzabvimbzupv.supabase.co`

## SD Card Setup

### Formatting
1. Format SD card as **FAT32** on your computer
2. Insert into PaperS3 device
3. Device will auto-create folder structure on first boot

### Manual Formatting Check
```cpp
if (!checkSDFormatted()) {
    Serial.println("SD card needs formatting!");
    // Format on computer, not on device
}
```

### Folder Initialization
```cpp
// Automatically called in setup()
initStorage();
// Creates: /audio, /transcripts, /summaries, /infographics, /recordings
```

## Cache Management

### Checking Cache Status
```cpp
if (isAudioCached(audioId)) {
    Serial.println("Audio already downloaded");
}

if (isTranscriptCached(audioId)) {
    Serial.println("Transcript already exists");
}

if (isSummaryCached(summaryId)) {
    Serial.println("Summary already exists");
}
```

### Cache Benefits
- **Offline playback**: Play audio without WiFi
- **Faster display**: Show summaries instantly
- **Bandwidth savings**: Don't re-download
- **Background sync**: Upload results later when WiFi available

## Error Handling

### WiFi Connection Lost
```cpp
if (!isWiFiConnected()) {
    // Try to use cached data
    if (isAudioCached(audioId) && isTranscriptCached(audioId)) {
        // Can still show cached results
    } else {
        // Need WiFi for download/processing
    }
}
```

### SD Card Full
```cpp
StorageInfo info = getStorageInfo();
if (info.sdAvailable) {
    // Check free space (TODO: implement)
    // Clean old cache files if needed
}
```

### API Errors
```cpp
TranscriptionResult result = transcribeAudio(path);

if (!result.success) {
    Serial.printf("Error: %s (HTTP %d)\n", result.error.c_str(), result.httpCode);
    
    if (result.httpCode == 401) {
        // Check API key
    } else if (result.httpCode == 429) {
        // Rate limit - wait and retry
    }
}
```

## Integration with Main UI

### Menu Option: "Process Audio"
```cpp
// In main.cpp, add menu button
if (checkButtonPress(tx, ty, menuButtons[4])) { // PROCESS AUDIO
    currentState = STATE_PROCESS_AUDIO;
    
    // Show audio list
    std::vector<AudioFile> audioList;
    fetchSupabaseAudio(audioList);
    
    // User selects audio
    String selectedAudioId = audioList[selectedIndex].id;
    
    // Show processing status
    drawProcessingScreen("Processing...");
    
    // Process locally
    if (processAudioLocally(selectedAudioId)) {
        drawProcessingScreen("Complete!");
        delay(2000);
        
        // Show summary
        String summary;
        readSummaryLocally(selectedAudioId + "_summary", summary);
        drawTextScreen("Summary", summary);
    } else {
        drawProcessingScreen("Error!");
    }
}
```

### Display Processing Progress
```cpp
void drawProcessingScreen(const String& status) {
    M5.Display.clear();
    M5.Display.setTextSize(2);
    M5.Display.setCursor(50, 100);
    M5.Display.println(status);
    
    // Show steps
    M5.Display.setTextSize(1);
    M5.Display.setCursor(20, 150);
    M5.Display.println("[1/5] Downloading audio...");
    M5.Display.println("[2/5] Transcribing...");
    M5.Display.println("[3/5] Saving transcript...");
    M5.Display.println("[4/5] Generating summary...");
    M5.Display.println("[5/5] Saving summary...");
}
```

## Testing Workflow

### 1. Test SD Card
```
Serial Monitor Output:
[STORAGE] SD Card initialized
[STORAGE] SD Card is properly formatted
[STORAGE] Folder exists: /audio
[STORAGE] Folder exists: /transcripts
[STORAGE] Folder exists: /summaries
```

### 2. Test Audio Download
```cpp
String audioId = "62cd9c95-51d2-434e-8baf-1eeef165a6a0";
std::vector<AudioFile> audioList;
fetchSupabaseAudio(audioList);

AudioFile* audio = findAudioById(audioList, audioId);
String localPath;
downloadAudioFromSupabase(*audio, localPath);

// Check file exists
if (SD.exists(localPath)) {
    Serial.println("Download successful!");
}
```

### 3. Test Transcription
```cpp
TranscriptionResult result = transcribeAudio("/audio/uuid.webm");

Serial.printf("Success: %d\n", result.success);
Serial.printf("Text: %s\n", result.text.c_str());
Serial.printf("Error: %s\n", result.error.c_str());
```

### 4. Test Summary
```cpp
SummaryResult result = summarizeText("This is test text to summarize");

Serial.printf("Success: %d\n", result.success);
Serial.printf("Summary: %s\n", result.text.c_str());
```

### 5. Test Full Workflow
```cpp
bool success = processAudioLocally("62cd9c95-51d2-434e-8baf-1eeef165a6a0");

Serial.printf("Workflow complete: %d\n", success);
```

## Performance Considerations

### Memory Usage
- Audio files: Stream from SD, don't load into RAM
- Transcripts: ~1-5 KB per file
- Summaries: ~500 bytes per file
- PSRAM fallback: Available if SD fails

### Processing Time
- Audio download: ~5-10 seconds (depends on file size)
- Transcription: ~10-30 seconds (ElevenLabs API)
- Summarization: ~5-10 seconds (Gemini API)
- **Total: ~30-60 seconds per audio**

### Network Usage
- Audio download: 100-500 KB per file
- API calls: ~10-50 KB per request
- Much less than re-downloading every time!

## Future Enhancements

1. **Background sync**: Upload results to Supabase after processing
2. **Queue system**: Process multiple audio files in batch
3. **Progress callbacks**: Update UI during processing
4. **Cache expiration**: Auto-delete old files
5. **Compression**: Compress transcripts/summaries
6. **Infographic generation**: Use Gemini to create images
7. **Voice playback**: TTS for summaries

## Migration from Old System

### What to Keep
- ✅ Supabase REST API for audio list
- ✅ Signed URLs for downloads
- ✅ Database schema for metadata

### What to Replace
- ❌ Edge Function process-audio (no longer needed!)
- ❌ Edge Function environment variables
- ❌ Polling for processing status

### What to Add
- ✅ SD card folder structure
- ✅ Local processing functions
- ✅ Cache management
- ✅ Direct API calls from device

## Summary

The new local processing architecture:
1. **Simplifies** the system (no Edge Functions needed)
2. **Improves** reliability (device has full control)
3. **Enhances** performance (local caching)
4. **Reduces** complexity (easier debugging)
5. **Enables** offline mode (cached results)

The device now acts as a **standalone processing unit** that:
- Downloads audio from cloud
- Processes locally with external APIs
- Caches everything on SD card
- Displays results on E-Ink screen
- Optionally syncs back to cloud

This is the **recommended architecture** going forward! 🎉
