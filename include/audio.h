#ifndef AUDIO_H
#define AUDIO_H

#include <cstdint>
#include <vector>
#include <String.h>
#include <M5Unified.h>
#include "storage.h"

// ============================================================================
// AUDIO CONFIGURATION (M5Stack PaperS3 via M5Unified)
// ============================================================================
#define AUDIO_BUFFER_SIZE 8192          // DMA buffer size
#define AUDIO_SAMPLE_RATE 16000         // 16kHz sampling
#define AUDIO_CHANNELS 1                // Mono
#define AUDIO_BITS_PER_SAMPLE 16        // 16-bit PCM
#define AUDIO_MAX_RECORDING_SECONDS 60  // Max 1 minute recording
#define AUDIO_BUFFER_SAMPLES 16000      // 1 second at 16kHz

// ============================================================================
// WAV FILE FORMAT STRUCTURES
// ============================================================================
struct WAVHeader {
    uint32_t riff = 0x46464952;  // "RIFF"
    uint32_t fileSize;
    uint32_t wave = 0x45564157;  // "WAVE"
    uint32_t fmt = 0x20746d66;   // "fmt "
    uint32_t fmtSize = 16;
    uint16_t audioFormat = 1;    // PCM
    uint16_t numChannels = AUDIO_CHANNELS;
    uint32_t sampleRate = AUDIO_SAMPLE_RATE;
    uint32_t byteRate;
    uint16_t blockAlign;
    uint16_t bitsPerSample = AUDIO_BITS_PER_SAMPLE;
    uint32_t data = 0x61746164;  // "data"
    uint32_t dataSize = 0;

    WAVHeader() {
        byteRate = sampleRate * numChannels * (bitsPerSample / 8);
        blockAlign = numChannels * (bitsPerSample / 8);
    }
};

// ============================================================================
// AUDIO STATE & RECORDING BUFFERS
// ============================================================================
extern bool isRecording;
extern String currentRecordingFile;
extern size_t recordingByteCount;

// ============================================================================
// M5UNIFIED MICROPHONE INTERFACE FUNCTIONS
// ============================================================================

/// Initialize audio system using M5Unified microphone
/// Configures I2S microphone with PDM mode for PaperS3
void initAudio();

/// Start recording audio from microphone to internal buffer
/// M5Unified handles I2S DMA automatically
/// Returns true if recording started successfully
bool startAudioRecording();

/// Stop recording and save buffered audio to WAV file
/// Writes WAV header + audio data to SD card
/// Returns true if recording stopped and file saved successfully
bool stopAudioRecording(AudioFile& audioFile);

/// Get current recording progress (bytes recorded)
size_t getRecordingSize();

/// Check if microphone recording is active
bool isAudioRecording();

/// Record audio directly to file (blocking call)
/// Uses M5Unified built-in recording with automatic WAV header generation
/// @param filename Output WAV filename (e.g., "/sd/rec_001.wav")
/// @param durationMs Recording duration in milliseconds
/// @return true if recording completed successfully
bool recordAudioToFile(const String& filename, uint32_t durationMs);

// ============================================================================
// AUDIO FILE MANAGEMENT
// ============================================================================

/// Get list of recent audio files from storage
void getRecentAudioFiles(std::vector<AudioFile>& files, int maxFiles = 10);

/// Delete an audio file by filename
bool deleteAudioFile(const String& filename);

/// Get list of all audio files on SD card
std::vector<String> listAllAudioFiles();

#endif // AUDIO_H
