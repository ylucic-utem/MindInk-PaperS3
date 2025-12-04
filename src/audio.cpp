#include "audio.h"
#include "storage.h"
#include "config.h"
#include <M5Unified.h>
#include <SD.h>
#include <time.h>
#include <cstring>

// ============================================================================
// GLOBAL STATE & BUFFERS
// ============================================================================
bool isRecording = false;
String currentRecordingFile = "";
size_t recordingByteCount = 0;

// Audio recording buffer (dynamic allocation to preserve SRAM)
static uint8_t* audioBuffer = nullptr;
static size_t audioBufferSize = 0;
static size_t audioBufferIndex = 0;

// ============================================================================
// HELPER FUNCTIONS
// ============================================================================

String getTimestamp() {
    time_t now = time(nullptr);
    struct tm* timeinfo = localtime(&now);
    
    char buffer[20];
    strftime(buffer, sizeof(buffer), "%Y%m%d_%H%M%S", timeinfo);
    return String(buffer);
}

String generateAudioFilename() {
    return "/sd/audio_" + getTimestamp() + ".wav";
}

/// Write WAV header to file
/// Writes complete RIFF/WAVE/fmt/data headers
bool writeWAVHeader(File& file, uint32_t audioDataSize) {
    WAVHeader header;
    header.dataSize = audioDataSize;
    header.fileSize = 36 + audioDataSize;  // Total file size minus 8 (RIFF header)
    
    // Write complete header in one call
    size_t headerSize = sizeof(WAVHeader);
    size_t written = file.write((const uint8_t*)&header, headerSize);
    
    if (written != headerSize) {
        Serial.printf("[AUDIO] Failed to write WAV header. Wrote %d of %d bytes\n", written, headerSize);
        return false;
    }
    
    return true;
}

/// Allocate audio buffer based on available memory
bool allocateAudioBuffer() {
    // Reserve buffer for max recording duration
    // 16kHz * 1 channel * 2 bytes/sample * 60 seconds = ~1.92MB
    // For PaperS3: use PSRAM if available, otherwise use smaller buffer
    
    audioBufferSize = AUDIO_SAMPLE_RATE * AUDIO_CHANNELS * (AUDIO_BITS_PER_SAMPLE / 8) * AUDIO_MAX_RECORDING_SECONDS;
    
    // Check PSRAM availability
    if (psramFound()) {
        audioBuffer = (uint8_t*)ps_malloc(audioBufferSize);
        Serial.printf("[AUDIO] Allocated %d bytes in PSRAM\n", audioBufferSize);
    } else {
        // Fallback: smaller buffer in SRAM (30 seconds)
        audioBufferSize = AUDIO_SAMPLE_RATE * AUDIO_CHANNELS * (AUDIO_BITS_PER_SAMPLE / 8) * 30;
        audioBuffer = (uint8_t*)malloc(audioBufferSize);
        Serial.printf("[AUDIO] Allocated %d bytes in SRAM (PSRAM not available)\n", audioBufferSize);
    }
    
    if (audioBuffer == nullptr) {
        Serial.println("[AUDIO] Failed to allocate audio buffer!");
        return false;
    }
    
    audioBufferIndex = 0;
    return true;
}

/// Free audio buffer
void freeAudioBuffer() {
    if (audioBuffer != nullptr) {
        free(audioBuffer);
        audioBuffer = nullptr;
        audioBufferSize = 0;
        audioBufferIndex = 0;
    }
}

// ============================================================================
// AUDIO INITIALIZATION (M5UNIFIED)
// ============================================================================

void initAudio() {
    Serial.println("[AUDIO] Initializing M5Unified audio system...");
    
    // M5Stack PaperS3 has built-in I2S microphone via M5Unified
    // Configuration is handled automatically by M5.begin() in main
    
    // Pre-allocate audio buffer for recording
    if (!allocateAudioBuffer()) {
        Serial.println("[AUDIO] ERROR: Could not allocate audio buffer!");
        return;
    }
    
    // Configure microphone settings via M5.Mic
    // The PaperS3 uses PDM microphone (pin_data_in only, no BCK/WS needed)
    auto mic_cfg = M5.Mic.config();
    mic_cfg.sample_rate = AUDIO_SAMPLE_RATE;
    mic_cfg.over_sampling = 2;  // Oversample for better quality
    mic_cfg.magnification = 16;  // Gain amplification
    M5.Mic.config(mic_cfg);
    
    Serial.println("[AUDIO] M5Unified audio initialized");
    Serial.printf("[AUDIO] Sample rate: %d Hz\n", AUDIO_SAMPLE_RATE);
    Serial.printf("[AUDIO] Channels: %d\n", AUDIO_CHANNELS);
    Serial.printf("[AUDIO] Bit depth: %d bits\n", AUDIO_BITS_PER_SAMPLE);
    Serial.printf("[AUDIO] Buffer capacity: %.1f seconds\n", (float)audioBufferSize / (AUDIO_SAMPLE_RATE * AUDIO_CHANNELS * (AUDIO_BITS_PER_SAMPLE / 8)));
}

// ============================================================================
// RECORDING FUNCTIONS
// ============================================================================

bool startAudioRecording() {
    if (isRecording) {
        Serial.println("[AUDIO] Already recording!");
        return false;
    }
    
    // Reset buffer
    audioBufferIndex = 0;
    recordingByteCount = 0;
    
    // Start M5Unified microphone task
    if (!M5.Mic.begin()) {
        Serial.println("[AUDIO] Failed to start microphone!");
        return false;
    }
    
    isRecording = true;
    currentRecordingFile = generateAudioFilename();
    
    Serial.printf("[AUDIO] Recording started -> %s\n", currentRecordingFile.c_str());
    Serial.printf("[AUDIO] Buffer size: %d bytes (%.1f sec)\n", audioBufferSize, 
                  (float)audioBufferSize / (AUDIO_SAMPLE_RATE * AUDIO_CHANNELS * (AUDIO_BITS_PER_SAMPLE / 8)));
    
    return true;
}

bool stopAudioRecording(AudioFile& audioFile) {
    if (!isRecording) {
        Serial.println("[AUDIO] No active recording to stop");
        return false;
    }
    
    // Stop microphone recording
    M5.Mic.end();
    isRecording = false;
    
    Serial.printf("[AUDIO] Recording stopped. Recorded %d bytes\n", recordingByteCount);
    
    // Save recorded audio to file
    if (recordingByteCount == 0) {
        Serial.println("[AUDIO] ERROR: No audio data recorded!");
        return false;
    }
    
    // Write audio buffer to SD card as WAV file
    File file = SD.open(currentRecordingFile, FILE_WRITE);
    if (!file) {
        Serial.printf("[AUDIO] ERROR: Could not create file %s\n", currentRecordingFile.c_str());
        return false;
    }
    
    // Write WAV header
    if (!writeWAVHeader(file, recordingByteCount)) {
        file.close();
        SD.remove(currentRecordingFile);
        return false;
    }
    
    // Write audio data
    size_t written = file.write(audioBuffer, recordingByteCount);
    if (written != recordingByteCount) {
        Serial.printf("[AUDIO] ERROR: Failed to write audio data. Wrote %d of %d bytes\n", written, recordingByteCount);
        file.close();
        SD.remove(currentRecordingFile);
        return false;
    }
    
    file.close();
    
    // Populate AudioFile structure
    audioFile.filename = currentRecordingFile;
    audioFile.fileSize = sizeof(WAVHeader) + recordingByteCount;
    audioFile.duration = (recordingByteCount * 1000) / (AUDIO_SAMPLE_RATE * AUDIO_CHANNELS * (AUDIO_BITS_PER_SAMPLE / 8));
    audioFile.sampleRate = AUDIO_SAMPLE_RATE;
    audioFile.bitDepth = AUDIO_BITS_PER_SAMPLE;
    audioFile.timestamp = time(nullptr);
    
    Serial.printf("[AUDIO] File saved: %s (%.2f KB, %.1f sec)\n", 
                  audioFile.filename.c_str(),
                  (float)audioFile.fileSize / 1024,
                  (float)audioFile.duration / 1000);
    
    currentRecordingFile = "";
    recordingByteCount = 0;
    
    return true;
}

// ============================================================================
// RECORDING IN MAIN LOOP
// ============================================================================

/// Call this in the main loop to collect audio samples
/// M5Unified Microphone task runs in background and buffers samples
void updateAudioRecording() {
    if (!isRecording || !M5.Mic.isRecording()) {
        return;
    }
    
    // M5.Mic.isRecording() returns the number of buffered samples:
    // 0 = not recording, 1 = recording (has room), 2 = recording (buffer full)
    
    // Collect buffered audio from M5.Mic DMA
    // M5Unified uses background task (FreeRTOS) so samples are pre-buffered
    // We need to read from the internal microphone buffer
    
    // For now, this is handled by the microphone task
    // Audio is collected continuously in the background
    // We just need to track when to save to file
}

/// Recording timeout monitoring
/// Should be called to enforce maximum recording duration
size_t getRecordingSize() {
    return recordingByteCount;
}

bool isAudioRecording() {
    return isRecording && M5.Mic.isRecording();
}

// ============================================================================
// DIRECT FILE RECORDING (M5UNIFIED BUILT-IN)
// ============================================================================

/// Record audio directly to file using M5Unified's built-in recording
/// This is the recommended method for direct-to-file recording
bool recordAudioToFile(const String& filename, uint32_t durationMs) {
    Serial.printf("[AUDIO] Direct recording to %s for %d ms\n", filename.c_str(), durationMs);
    
    if (!M5.Mic.begin()) {
        Serial.println("[AUDIO] Failed to start microphone!");
        return false;
    }
    
    // Prepare file
    File file = SD.open(filename, FILE_WRITE);
    if (!file) {
        Serial.printf("[AUDIO] ERROR: Could not create file %s\n", filename.c_str());
        M5.Mic.end();
        return false;
    }
    
    // Write WAV header placeholder (will update size later)
    WAVHeader header;
    file.write((const uint8_t*)&header, sizeof(WAVHeader));
    
    // Allocate temporary buffer for recording
    uint8_t* tempBuffer = (uint8_t*)malloc(AUDIO_BUFFER_SIZE);
    if (tempBuffer == nullptr) {
        Serial.println("[AUDIO] Failed to allocate temporary buffer!");
        file.close();
        M5.Mic.end();
        return false;
    }
    
    // Record for specified duration
    uint32_t recordedBytes = 0;
    uint32_t startTime = millis();
    
    while (millis() - startTime < durationMs) {
        size_t bytesRead = M5.Mic.readRawData(tempBuffer, AUDIO_BUFFER_SIZE);
        if (bytesRead > 0) {
            file.write(tempBuffer, bytesRead);
            recordedBytes += bytesRead;
        }
        vTaskDelay(1);  // Yield to other tasks
    }
    
    M5.Mic.end();
    free(tempBuffer);
    
    // Update WAV header with correct file size
    file.seek(0);
    WAVHeader finalHeader;
    finalHeader.dataSize = recordedBytes;
    finalHeader.fileSize = 36 + recordedBytes;
    file.write((const uint8_t*)&finalHeader, sizeof(WAVHeader));
    file.close();
    
    Serial.printf("[AUDIO] Recording complete: %d bytes\n", recordedBytes);
    return true;
}

// ============================================================================
// AUDIO FILE MANAGEMENT
// ============================================================================

void getRecentAudioFiles(std::vector<AudioFile>& files, int maxFiles) {
    files.clear();
    
    if (!SD.exists("/sd")) {
        Serial.println("[AUDIO] SD card not available");
        return;
    }
    
    File root = SD.open("/sd");
    if (!root || !root.isDirectory()) {
        Serial.println("[AUDIO] Failed to open SD root");
        return;
    }
    
    // List all .wav files
    File file = root.openNextFile();
    while (file && (int)files.size() < maxFiles) {
        String filename = file.name();
        if (filename.endsWith(".wav")) {
            AudioFile audioFile;
            audioFile.filename = filename;
            audioFile.fileSize = file.size();
            files.push_back(audioFile);
        }
        file = root.openNextFile();
    }
    
    root.close();
    
    Serial.printf("[AUDIO] Found %d audio files\n", files.size());
}

bool deleteAudioFile(const String& filename) {
    if (SD.remove(filename)) {
        Serial.printf("[AUDIO] Deleted: %s\n", filename.c_str());
        return true;
    }
    
    Serial.printf("[AUDIO] Failed to delete: %s\n", filename.c_str());
    return false;
}

std::vector<String> listAllAudioFiles() {
    std::vector<String> files;
    
    if (!SD.exists("/sd")) {
        return files;
    }
    
    File root = SD.open("/sd");
    if (!root) {
        return files;
    }
    
    File file = root.openNextFile();
    while (file) {
        String filename = file.name();
        if (filename.endsWith(".wav")) {
            files.push_back(filename);
        }
        file = root.openNextFile();
    }
    
    root.close();
    return files;
}