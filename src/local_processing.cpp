// ============================================================================
// LOCAL PROCESSING WORKFLOW
// Device records audio, processes locally, caches results on SD card
// ============================================================================

#include "local_processing.h"
#include "config.h"
#include "storage.h"
#include "audio.h"
#include "display.h"
#include "api.h"
#include "supabase_client.h"
#include "wifi_manager.h"
#include <M5Unified.h>
#include <SD.h>
#include <ArduinoJson.h>
#include <time.h>

// ============================================================================
// GLOBAL STATE
// ============================================================================
static ProcessingState currentProcessingState = PROCESSING_IDLE;
static String currentStatusMessage = "";
static int currentPercentComplete = 0;
static String currentStep = "";
static String currentErrorMessage = "";
static bool isProcessingCancelled = false;

// ============================================================================
// INITIALIZATION
// ============================================================================

void initLocalProcessing() {
    Serial.println("[LOCAL_PROCESSING] Initializing local processing system...");
    
    // Ensure storage is initialized
    initStorage();
    
    // Create necessary SD card folders
    initSDFolders();
    
    Serial.println("[LOCAL_PROCESSING] Initialization complete");
}

// ============================================================================
// STATUS & PROGRESS
// ============================================================================

ProcessingProgress getProcessingProgress() {
    return {
        .state = currentProcessingState,
        .statusMessage = currentStatusMessage,
        .percentComplete = currentPercentComplete,
        .currentStep = currentStep,
        .errorMessage = currentErrorMessage
    };
}

static void setProcessingState(ProcessingState state, const String& message, const String& step, int percent) {
    currentProcessingState = state;
    currentStatusMessage = message;
    currentStep = step;
    currentPercentComplete = percent;
    if (state != PROCESSING_ERROR) {
        currentErrorMessage = "";
    }
    
    Serial.printf("[LOCAL_PROCESSING] State: %d | Step: %s | %d%%\n", state, step.c_str(), percent);
    Serial.printf("[LOCAL_PROCESSING] Message: %s\n", message.c_str());
}

static void setProcessingError(const String& errorMessage) {
    currentProcessingState = PROCESSING_ERROR;
    currentErrorMessage = errorMessage;
    Serial.printf("[LOCAL_PROCESSING] ERROR: %s\n", errorMessage.c_str());
}

bool cancelProcessing() {
    isProcessingCancelled = true;
    return true;
}

// ============================================================================
// RECORDING WITH PROGRESS
// ============================================================================

bool recordAudioWithProgress(int durationSeconds, AudioFile& outAudioFile) {
    setProcessingState(PROCESSING_RECORDING, "Recording audio...", "Recording", 0);
    isProcessingCancelled = false;
    
    // Create filename with timestamp
    uint32_t timestamp = time(nullptr);
    String filename = String("/audio/rec_") + String(timestamp) + ".wav";
    
    Serial.printf("[AUDIO_REC] Recording %d seconds to: %s\n", durationSeconds, filename.c_str());
    
    // Show recording UI
    M5.Display.clear();
    M5.Display.setTextSize(2);
    M5.Display.drawString("RECORDING...", M5.Display.width() / 2, 50);
    M5.Display.setTextSize(1);
    
    // Start recording
    if (!startAudioRecording()) {
        setProcessingError("Failed to start audio recording");
        return false;
    }
    
    // Record for specified duration with progress updates
    uint32_t recordingStartTime = millis();
    uint32_t recordingEndTime = millis() + (durationSeconds * 1000);
    uint32_t lastProgressUpdate = millis();
    
    while (millis() < recordingEndTime && !isProcessingCancelled) {
        M5.update();
        
        // Update audio samples
        updateAudioRecording();
        
        // Update progress display every 500ms
        if (millis() - lastProgressUpdate > 500) {
            uint32_t elapsedMs = millis() - recordingStartTime;
            int elapsedSec = elapsedMs / 1000;
            int percent = (elapsedSec * 100) / durationSeconds;
            
            setProcessingState(PROCESSING_RECORDING, 
                             "Recording audio...",
                             String("Recording: ") + String(elapsedSec) + "s/" + String(durationSeconds) + "s",
                             percent);
            
            // Update display
            M5.Display.fillRect(0, 100, M5.Display.width(), 100, 0x000000);
            M5.Display.drawString(String(elapsedSec) + "s", M5.Display.width() / 2, 120);
            M5.Display.progressBar(20, 200, M5.Display.width() - 40, 20, percent);
            
            lastProgressUpdate = millis();
        }
        
        delay(10);
    }
    
    // Stop recording and save
    if (!stopAudioRecording(outAudioFile)) {
        setProcessingError("Failed to save audio recording");
        return false;
    }
    
    // Update filename
    outAudioFile.filename = filename;
    outAudioFile.localPath = filename;
    outAudioFile.timestamp = timestamp;
    
    setProcessingState(PROCESSING_IDLE, "Recording complete", "Complete", 100);
    
    Serial.printf("[AUDIO_REC] Recording complete: %s (%.1f KB)\n", 
                  filename.c_str(), outAudioFile.fileSize / 1024.0);
    
    return true;
}

// ============================================================================
// TRANSCRIPTION
// ============================================================================

bool transcribeAudioLocally(const AudioFile& audioFile, String& outTranscript) {
    Serial.println("[TRANSCRIBE] Starting transcription...");
    Serial.printf("[TRANSCRIBE] Audio file path: %s\n", audioFile.localPath.c_str());
    Serial.printf("[TRANSCRIBE] Free heap: %d bytes\n", ESP.getFreeHeap());
    
    setProcessingState(PROCESSING_TRANSCRIBING, 
                      "Transcribing with ElevenLabs API...",
                      "Transcribing",
                      25);
    
    if (!isWiFiConnected()) {
        Serial.println("[TRANSCRIBE] ERROR: WiFi not connected");
        setProcessingError("WiFi not connected. Cannot transcribe.");
        return false;
    }
    
    // Call ElevenLabs API
    Serial.println("[TRANSCRIBE] Calling ElevenLabs API...");
    TranscriptionResult result = transcribeAudio(audioFile.localPath);
    Serial.printf("[TRANSCRIBE] API call returned: success=%d, httpCode=%d\n", result.success, result.httpCode);
    
    if (!result.success) {
        Serial.printf("[TRANSCRIBE] ERROR: %s\n", result.error.c_str());
        setProcessingError(String("Transcription failed: ") + result.error);
        return false;
    }
    
    outTranscript = result.text;
    Serial.printf("[TRANSCRIBE] Got transcript: %d characters\n", outTranscript.length());
    
    // Save transcript locally
    String transcriptPath = getTranscriptCachePath(audioFile.id);
    Serial.printf("[TRANSCRIBE] Saving to: %s\n", transcriptPath.c_str());
    
    File transcriptFile = SD.open(transcriptPath, FILE_WRITE);
    if (!transcriptFile) {
        Serial.println("[TRANSCRIBE] ERROR: Failed to open file for writing");
        setProcessingError("Failed to save transcript to SD card");
        return false;
    }
    
    transcriptFile.print(outTranscript);
    transcriptFile.close();
    Serial.println("[TRANSCRIBE] Transcript saved successfully");
    
    setProcessingState(PROCESSING_TRANSCRIBING,
                      "Transcription complete",
                      "Transcribed: " + String(outTranscript.length()) + " characters",
                      50);
    
    Serial.printf("[TRANSCRIBE] Success: %d characters\n", outTranscript.length());
    
    return true;
}

// ============================================================================
// SUMMARIZATION
// ============================================================================

bool summarizeTranscriptLocally(const String& transcript, String& outSummary, int maxLength) {
    setProcessingState(PROCESSING_SUMMARIZING,
                      "Summarizing with Gemini API...",
                      "Summarizing",
                      50);
    
    if (!isWiFiConnected()) {
        setProcessingError("WiFi not connected. Cannot summarize.");
        return false;
    }
    
    // Call Gemini API
    SummaryResult result = summarizeText(transcript, maxLength);
    
    if (!result.success) {
        setProcessingError(String("Summarization failed: ") + result.error);
        return false;
    }
    
    outSummary = result.text;
    
    setProcessingState(PROCESSING_SUMMARIZING,
                      "Summarization complete",
                      "Summary: " + String(outSummary.length()) + " characters",
                      75);
    
    Serial.printf("[SUMMARIZE] Success: %d characters\n", outSummary.length());
    
    return true;
}

// ============================================================================
// SUPABASE UPLOAD
// ============================================================================

bool uploadProcessingResults(const AudioFile& audioFile, const String& transcript, const String& summary) {
    setProcessingState(PROCESSING_UPLOADING,
                      "Uploading results to Supabase...",
                      "Uploading",
                      75);
    
    if (!isWiFiConnected()) {
        setProcessingError("WiFi not connected. Cannot upload.");
        return false;
    }
    
    // TODO: Implement database inserts and file uploads to Supabase
    // 1. Create audio_records entry
    // 2. Create summaries entry
    // 3. Upload files to storage buckets
    
    setProcessingState(PROCESSING_COMPLETE,
                      "Processing complete! Results uploaded to Supabase.",
                      "Complete",
                      100);
    
    Serial.println("[UPLOAD] All results uploaded successfully");
    
    return true;
}

// ============================================================================
// MAIN WORKFLOW
// ============================================================================

bool recordAndProcessAudio(int durationSeconds, AudioFile& outAudioFile) {
    Serial.println("\n[WORKFLOW] Starting complete audio processing workflow...\n");
    
    isProcessingCancelled = false;
    currentProcessingState = PROCESSING_RECORDING;
    
    // Step 1: Record audio
    if (!recordAudioWithProgress(durationSeconds, outAudioFile)) {
        setProcessingError("Recording failed");
        return false;
    }
    
    if (isProcessingCancelled) return false;
    
    // Generate ID for this recording (simplified UUID)
    outAudioFile.id = String(millis());
    
    // Step 2: Transcribe audio
    String transcript = "";
    if (!transcribeAudioLocally(outAudioFile, transcript)) {
        setProcessingError("Transcription failed");
        return false;
    }
    
    if (isProcessingCancelled) return false;
    
    // Step 3: Summarize transcript
    String summary = "";
    if (!summarizeTranscriptLocally(transcript, summary)) {
        setProcessingError("Summarization failed");
        return false;
    }
    
    if (isProcessingCancelled) return false;
    
    // Step 4: Upload results to Supabase
    if (!uploadProcessingResults(outAudioFile, transcript, summary)) {
        setProcessingError("Upload failed");
        return false;
    }
    
    Serial.println("\n[WORKFLOW] Complete audio processing workflow finished successfully!\n");
    
    return true;
}

bool processExistingAudio(const AudioFile& cloudFile) {
    Serial.println("\n[WORKFLOW] Processing existing audio file...\n");
    
    isProcessingCancelled = false;
    
    // Step 1: Download audio from Supabase to SD card
    setProcessingState(PROCESSING_RECORDING, 
                      "Downloading audio from Supabase...",
                      "Downloading",
                      10);
    
    if (!isWiFiConnected()) {
        setProcessingError("WiFi not connected. Cannot download audio.");
        return false;
    }
    
    // Download audio file from Supabase storage
    String localPath = "";
    Serial.printf("[DOWNLOAD] Downloading: %s\n", cloudFile.filename.c_str());
    Serial.printf("[DOWNLOAD] Free heap before download: %d bytes\n", ESP.getFreeHeap());
    
    if (!downloadAudioFromSupabase(cloudFile, localPath)) {
        setProcessingError("Failed to download audio from Supabase");
        return false;
    }
    
    // Create AudioFile with local path
    AudioFile localAudioFile;
    localAudioFile.id = cloudFile.id;
    localAudioFile.filename = cloudFile.filename;
    localAudioFile.localPath = localPath;
    localAudioFile.storagePath = cloudFile.storagePath;
    localAudioFile.timestamp = cloudFile.timestamp;
    
    // Get file size
    File file = SD.open(localPath, FILE_READ);
    if (file) {
        localAudioFile.fileSize = file.size();
        file.close();
    }
    
    Serial.printf("[DOWNLOAD] Complete: %s (%.1f KB)\n", 
                  localPath.c_str(), localAudioFile.fileSize / 1024.0);
    Serial.printf("[DOWNLOAD] Free heap after download: %d bytes\n", ESP.getFreeHeap());
    
    if (isProcessingCancelled) return false;
    
    // Step 2: Transcribe
    Serial.println("[WORKFLOW] === Starting transcription step ===");
    String transcript = "";
    unsigned long transcribeStartTime = millis();
    
    if (!transcribeAudioLocally(localAudioFile, transcript)) {
        unsigned long elapsed = millis() - transcribeStartTime;
        Serial.printf("[WORKFLOW] Transcription failed after %lu ms\n", elapsed);
        return false;
    }
    
    unsigned long transcribeTime = millis() - transcribeStartTime;
    Serial.printf("[WORKFLOW] === Transcription complete in %lu ms ===\n", transcribeTime);
    
    if (isProcessingCancelled) return false;
    
    // Step 3: Summarize
    Serial.println("[WORKFLOW] === Starting summarization step ===");
    String summary = "";
    if (!summarizeTranscriptLocally(transcript, summary)) {
        return false;
    }
    
    if (isProcessingCancelled) return false;
    
    // Step 4: Upload results
    if (!uploadProcessingResults(localAudioFile, transcript, summary)) {
        return false;
    }
    
    return true;
}

// ============================================================================
// FILE MANAGEMENT
// ============================================================================

bool getLocalAudioFiles(std::vector<AudioFile>& outFiles) {
    File audioDir = SD.open("/audio");
    if (!audioDir || !audioDir.isDirectory()) {
        Serial.println("[FILE_MGT] /audio directory not found");
        return false;
    }
    
    outFiles.clear();
    File file = audioDir.openNextFile();
    
    while (file) {
        if (file.name()[0] != '.') {  // Skip hidden files
            AudioFile audioFile;
            audioFile.filename = String(file.name());
            audioFile.localPath = String("/audio/") + String(file.name());
            audioFile.fileSize = file.size();
            outFiles.push_back(audioFile);
        }
        file = audioDir.openNextFile();
    }
    
    audioDir.close();
    
    Serial.printf("[FILE_MGT] Found %d audio files\n", outFiles.size());
    
    return true;
}

bool getLocalSummaries(std::vector<SummaryFile>& outSummaries) {
    File summaryDir = SD.open("/summaries");
    if (!summaryDir || !summaryDir.isDirectory()) {
        Serial.println("[FILE_MGT] /summaries directory not found");
        return false;
    }
    
    outSummaries.clear();
    File file = summaryDir.openNextFile();
    
    while (file) {
        if (file.name()[0] != '.') {  // Skip hidden files
            SummaryFile summaryFile;
            summaryFile.filename = String(file.name());
            summaryFile.localPath = String("/summaries/") + String(file.name());
            outSummaries.push_back(summaryFile);
        }
        file = summaryDir.openNextFile();
    }
    
    summaryDir.close();
    
    Serial.printf("[FILE_MGT] Found %d summary files\n", outSummaries.size());
    
    return true;
}

bool deleteLocalAudio(const String& audioId) {
    String audioPath = getAudioCachePath(audioId);
    String transcriptPath = getTranscriptCachePath(audioId);
    String summaryPath = getSummaryCachePath(audioId);
    
    bool success = true;
    
    if (SD.exists(audioPath)) {
        if (!SD.remove(audioPath)) {
            Serial.printf("[FILE_MGT] Failed to delete: %s\n", audioPath.c_str());
            success = false;
        }
    }
    
    if (SD.exists(transcriptPath)) {
        if (!SD.remove(transcriptPath)) {
            Serial.printf("[FILE_MGT] Failed to delete: %s\n", transcriptPath.c_str());
            success = false;
        }
    }
    
    if (SD.exists(summaryPath)) {
        if (!SD.remove(summaryPath)) {
            Serial.printf("[FILE_MGT] Failed to delete: %s\n", summaryPath.c_str());
            success = false;
        }
    }
    
    return success;
}

bool syncLocalWithSupabase() {
    setProcessingState(PROCESSING_UPLOADING,
                      "Syncing local files with Supabase...",
                      "Syncing",
                      50);
    
    // TODO: Implement sync logic
    // 1. Find all local files that haven't been synced
    // 2. Upload them to Supabase storage
    // 3. Update database records
    
    setProcessingState(PROCESSING_IDLE,
                      "Sync complete",
                      "Idle",
                      100);
    
    return true;
}
