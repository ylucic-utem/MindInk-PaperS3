#include "audio.h"
#include "storage.h"
#include "config.h"

// ============================================================================
// GLOBAL STATE
// ============================================================================
bool isRecording = false;
File recordingFile;

// ============================================================================
// AUDIO RECORDING FUNCTIONS
// ============================================================================

void startAudioRecording(AudioFile& audioFile) {
    isRecording = true;
    Serial.println("[AUDIO] Starting audio recording...");
    
    // TODO: Implement actual audio recording logic
    // This would use the I2S audio input on the M5Stack PaperS3
}

void stopAudioRecording() {
    isRecording = false;
    Serial.println("[AUDIO] Stopped audio recording");
    
    // TODO: Implement recording stop and file save logic
}

void getRecentAudioFiles(std::vector<AudioFile>& files) {
    // TODO: List recent audio files from storage
    files.clear();
}
