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
        // Fallback: smaller buffer in SRAM (5 seconds ~ 160KB)
        // ESP32-S3 has ~512KB SRAM but much is used by system/WiFi/Bluetooth
        audioBufferSize = AUDIO_SAMPLE_RATE * AUDIO_CHANNELS * (AUDIO_BITS_PER_SAMPLE / 8) * 5;
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
    
    // IMPORTANT: Ensure speaker is disabled before initializing microphone
    // Microphone and speaker cannot be used simultaneously
    M5.Speaker.end();
    
    // Pre-allocate audio buffer for recording
    if (!allocateAudioBuffer()) {
        Serial.println("[AUDIO] ERROR: Could not allocate audio buffer!");
        // Do not return here, allow system to continue without audio
    }
    
    // Configure microphone settings via M5.Mic
    // The PaperS3 uses PDM microphone (pin_data_in only, no BCK/WS needed)
    auto mic_cfg = M5.Mic.config();
    mic_cfg.sample_rate = AUDIO_SAMPLE_RATE;
    mic_cfg.over_sampling = 2;  // Oversample for better quality
    mic_cfg.magnification = 16;  // Gain amplification
    mic_cfg.noise_filter_level = 32;  // Light noise filtering
    M5.Mic.config(mic_cfg);
    
    Serial.println("[AUDIO] M5Unified audio initialized");
    Serial.printf("[AUDIO] Sample rate: %d Hz\n", AUDIO_SAMPLE_RATE);
    Serial.printf("[AUDIO] Channels: %d\n", AUDIO_CHANNELS);
    Serial.printf("[AUDIO] Bit depth: %d bits\n", AUDIO_BITS_PER_SAMPLE);
    if (audioBuffer) {
        Serial.printf("[AUDIO] Buffer capacity: %.1f seconds\n", (float)audioBufferSize / (AUDIO_SAMPLE_RATE * AUDIO_CHANNELS * (AUDIO_BITS_PER_SAMPLE / 8)));
    } else {
        Serial.println("[AUDIO] Buffer capacity: 0 seconds (Allocation Failed)");
    }
}

// ============================================================================
// RECORDING FUNCTIONS
// ============================================================================

bool startAudioRecording() {
    if (isRecording) {
        Serial.println("[AUDIO] Already recording!");
        return false;
    }
    
    if (audioBuffer == nullptr) {
        Serial.println("[AUDIO] Cannot start recording: No buffer allocated");
        return false;
    }
    
    // Ensure speaker is stopped (microphone/speaker mutual exclusion)
    M5.Speaker.end();
    delay(100);  // Give speaker time to stop
    
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

/// Call this in the main loop to collect audio samples
/// M5Unified Microphone task runs in background and buffers samples
void updateAudioRecording() {
    if (!isRecording || !M5.Mic.isRecording()) {
        return;
    }
    
    if (audioBuffer == nullptr) {
        isRecording = false;
        return;
    }
    
    // M5.Mic.isRecording() returns:
    // 0 = not recording
    // 1 = recording (has room in buffer)
    // 2 = recording (buffer full - might lose samples if not reading)
    
    // Collect buffered audio from M5.Mic DMA
    // We need to read samples into our recording buffer
    // Use M5.Mic.record() to pull data from the microphone's internal DMA buffer
    
    // Create a temporary buffer for reading samples
    static const size_t CHUNK_SIZE = 512;  // Read 512 samples (1ms at 16kHz) at a time
    static int16_t temp_buffer[CHUNK_SIZE];
    
    // Try to read samples from microphone
    if (M5.Mic.record((int16_t*)temp_buffer, CHUNK_SIZE, AUDIO_SAMPLE_RATE, false)) {
        // Calculate bytes to write
        size_t bytesToWrite = CHUNK_SIZE * sizeof(int16_t);
        
        // Check if we have room in our buffer
        if (audioBufferIndex + bytesToWrite <= audioBufferSize) {
            // Copy to recording buffer
            memcpy(&audioBuffer[audioBufferIndex], temp_buffer, bytesToWrite);
            audioBufferIndex += bytesToWrite;
            recordingByteCount += bytesToWrite;
        } else {
            // Buffer full - log warning
            // Only log once per second to avoid spamming
            static uint32_t lastLog = 0;
            if (millis() - lastLog > 1000) {
                Serial.println("[AUDIO] WARNING: Recording buffer full - some audio may be lost!");
                lastLog = millis();
            }
            isRecording = false;  // Stop recording to prevent buffer overflow
        }
    }
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

// ============================================================================
// SPEAKER/PLAYBACK IMPLEMENTATION
// ============================================================================

/// WAV header structure for file parsing
struct WAVFileHeader {
    uint32_t riff;              // "RIFF"
    uint32_t fileSize;
    uint32_t wave;              // "WAVE"
    uint32_t fmt;               // "fmt "
    uint32_t fmtSize;
    uint16_t audioFormat;       // 1 for PCM
    uint16_t numChannels;
    uint32_t sampleRate;
    uint32_t byteRate;
    uint16_t blockAlign;
    uint16_t bitsPerSample;
    uint32_t data;              // "data"
    uint32_t dataSize;
};

// Playback state
File playbackFile;
bool isPlayingFile = false;
bool isPlaybackPaused = false;
WAVFileHeader currentWavHeader;
uint32_t playbackBytesRead = 0;

bool initSpeaker() {
    // Ensure microphone is stopped (microphone/speaker mutual exclusion)
    M5.Mic.end();
    delay(100);  // Give microphone time to stop
    
    // Configure and start speaker
    if (!M5.Speaker.begin()) {
        Serial.println("[AUDIO] Failed to initialize speaker!");
        return false;
    }
    
    // Set default volume (moderate level)
    M5.Speaker.setVolume(96);  // 96/255 = 37.6% volume (safe default)
    
    Serial.println("[AUDIO] Speaker initialized successfully");
    Serial.printf("[AUDIO] Speaker volume: %d/255\n", M5.Speaker.getVolume());
    
    return true;
}

void stopSpeaker() {
    if (M5.Speaker.isRunning()) {
        M5.Speaker.stop();
        M5.Speaker.end();
        Serial.println("[AUDIO] Speaker stopped");
    }
}

bool playAudioData(const int16_t* data, size_t samples, uint32_t sampleRate, bool stereo) {
    if (!M5.Speaker.isRunning()) {
        Serial.println("[AUDIO] Speaker not initialized!");
        return false;
    }
    
    if (data == nullptr || samples == 0) {
        Serial.println("[AUDIO] Invalid audio data!");
        return false;
    }
    
    Serial.printf("[AUDIO] Playing %d samples at %d Hz (%s)\n", 
                  samples, sampleRate, stereo ? "stereo" : "mono");
    
    // Use M5Speaker's playRaw for int16_t audio
    bool result = M5.Speaker.playRaw(data, samples, sampleRate, stereo, 1, -1, false);
    
    if (!result) {
        Serial.println("[AUDIO] Failed to start audio playback");
        return false;
    }
    
    return true;
}

bool startAudioPlayback(const String& filename) {
    if (isPlayingFile) {
        stopPlayback();
    }

    if (!SD.exists(filename)) {
        Serial.printf("[AUDIO] File not found: %s\n", filename.c_str());
        return false;
    }
    
    if (!M5.Speaker.isRunning()) {
        initSpeaker();
    }
    
    playbackFile = SD.open(filename, FILE_READ);
    if (!playbackFile) {
        Serial.printf("[AUDIO] Failed to open file: %s\n", filename.c_str());
        return false;
    }
    
    // Read and parse WAV header
    if (playbackFile.read((uint8_t*)&currentWavHeader, sizeof(WAVFileHeader)) != sizeof(WAVFileHeader)) {
        Serial.println("[AUDIO] Failed to read WAV header");
        playbackFile.close();
        return false;
    }
    
    // Validate WAV file
    if (currentWavHeader.audioFormat != 1 || currentWavHeader.bitsPerSample != 16) {
        Serial.printf("[AUDIO] Unsupported WAV format (fmt=%d, bps=%d). Only PCM 16-bit supported.\n",
                      currentWavHeader.audioFormat, currentWavHeader.bitsPerSample);
        playbackFile.close();
        return false;
    }
    
    isPlayingFile = true;
    isPlaybackPaused = false;
    playbackBytesRead = 0;
    
    Serial.printf("[AUDIO] Started playback: %s (%d Hz)\n", filename.c_str(), currentWavHeader.sampleRate);
    return true;
}

void updateAudioPlayback() {
    if (!isPlayingFile || isPlaybackPaused) {
        return;
    }
    
    if (playbackBytesRead >= currentWavHeader.dataSize) {
        stopPlayback();
        return;
    }

    // Read a chunk
    static const size_t CHUNK_SIZE = 2048;
    uint8_t buffer[CHUNK_SIZE];
    
    size_t toRead = (currentWavHeader.dataSize - playbackBytesRead) > CHUNK_SIZE ? CHUNK_SIZE : (currentWavHeader.dataSize - playbackBytesRead);
    
    size_t numRead = playbackFile.read(buffer, toRead);
    if (numRead > 0) {
        bool isStereo = (currentWavHeader.numChannels > 1);
        size_t numSamples = numRead / sizeof(int16_t);
        
        if (M5.Speaker.playRaw((const int16_t*)buffer, numSamples, currentWavHeader.sampleRate, isStereo, 1, -1, false)) {
            playbackBytesRead += numRead;
        } else {
            // Buffer full, seek back
            playbackFile.seek(playbackFile.position() - numRead);
        }
    } else {
        stopPlayback();
    }
}

void pauseAudioPlayback() {
    if (isPlayingFile) {
        isPlaybackPaused = true;
        M5.Speaker.stop(); 
        Serial.println("[AUDIO] Playback paused");
    }
}

void resumeAudioPlayback() {
    if (isPlayingFile && isPlaybackPaused) {
        isPlaybackPaused = false;
        Serial.println("[AUDIO] Playback resumed");
    }
}

void stopPlayback() {
    if (isPlayingFile) {
        playbackFile.close();
        isPlayingFile = false;
        isPlaybackPaused = false;
        M5.Speaker.stop();
        Serial.println("[AUDIO] Playback stopped");
    }
}

bool isAudioPlaying() {
    return M5.Speaker.isPlaying() || isPlayingFile;
}

void setAudioVolume(uint8_t volume) {
    M5.Speaker.setVolume(volume);
    Serial.printf("[AUDIO] Volume set to %d/255\n", volume);
}