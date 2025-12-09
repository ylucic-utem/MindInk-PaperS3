#ifndef LOCAL_PROCESSING_H
#define LOCAL_PROCESSING_H

#include <WString.h>
#include "storage.h"
#include "api.h"

// ============================================================================
// PROCESSING STATE & WORKFLOW
// ============================================================================

enum ProcessingState {
    PROCESSING_IDLE = 0,
    PROCESSING_RECORDING = 1,
    PROCESSING_TRANSCRIBING = 2,
    PROCESSING_SUMMARIZING = 3,
    PROCESSING_UPLOADING = 4,
    PROCESSING_COMPLETE = 5,
    PROCESSING_ERROR = 6
};

struct ProcessingProgress {
    ProcessingState state;
    String statusMessage;
    int percentComplete;    // 0-100
    String currentStep;     // "Recording", "Transcribing", etc.
    String errorMessage;    // If state == PROCESSING_ERROR
};

// ============================================================================
// LOCAL PROCESSING WORKFLOW
// ============================================================================

/// Initialize local processing system
/// Ensures SD card folders exist for audio, transcripts, summaries
void initLocalProcessing();

/// Complete local audio processing workflow:
/// 1. Record audio from microphone and save to SD card
/// 2. Transcribe audio using ElevenLabs API
/// 3. Save transcript to SD card
/// 4. Summarize transcript using Gemini API
/// 5. Save summary to SD card
/// 6. Create audio_records entry in Supabase
/// 7. Create summaries entry in Supabase
/// 8. Upload audio file to Supabase storage
/// 9. Upload transcript to Supabase storage (optional)
/// 10. Upload summary to Supabase storage (optional)
///
/// @param durationSeconds How long to record (e.g., 30 seconds)
/// @param outAudioFile AudioFile struct with metadata
/// @return true if entire workflow completed successfully
bool recordAndProcessAudio(int durationSeconds, AudioFile& outAudioFile);

/// Process existing audio file (skip recording step)
/// Useful for reprocessing cached audio or audio from SD card
/// @param audioFile Audio file to process
/// @return true if transcription and summarization completed
bool processExistingAudio(const AudioFile& audioFile);

/// Record audio to SD card with visual progress on display
/// @param durationSeconds Recording duration in seconds
/// @param outAudioFile Populated with recording metadata
/// @return true if recording completed successfully
bool recordAudioWithProgress(int durationSeconds, AudioFile& outAudioFile);

/// Transcribe audio file locally using ElevenLabs
/// @param audioFile Audio file to transcribe
/// @param outTranscript Transcribed text
/// @return true if transcription successful
bool transcribeAudioLocally(const AudioFile& audioFile, String& outTranscript);

/// Summarize transcribed text using Gemini API
/// @param transcript Text to summarize
/// @param outSummary Summarized text
/// @param maxLength Maximum summary length (0 = default)
/// @return true if summarization successful
bool summarizeTranscriptLocally(const String& transcript, String& outSummary, int maxLength = 0);

/// Save processing results to Supabase
/// Creates/updates audio_records, summaries, and transcriptions tables
/// Uploads files to storage buckets
/// @param audioFile Audio file metadata
/// @param transcript Transcribed text
/// @param summary Summarized text
/// @return true if all Supabase operations successful
bool uploadProcessingResults(const AudioFile& audioFile, const String& transcript, const String& summary);

/// Get current processing status
/// @return ProcessingProgress with current state and message
ProcessingProgress getProcessingProgress();

/// Cancel current processing operation
/// @return true if cancelled successfully
bool cancelProcessing();

/// Get list of local audio files on SD card
/// @param outFiles Vector to populate with AudioFile entries
/// @return true if directory read successful
bool getLocalAudioFiles(std::vector<AudioFile>& outFiles);

/// Get list of local summaries on SD card
/// @param outSummaries Vector to populate with SummaryFile entries
/// @return true if directory read successful
bool getLocalSummaries(std::vector<SummaryFile>& outSummaries);

/// Delete local audio file and associated transcript/summary
/// @param audioId UUID of audio file
/// @return true if deletion successful
bool deleteLocalAudio(const String& audioId);

/// Sync local processing results with Supabase
/// Uploads any local files that haven't been synced yet
/// @return true if sync completed successfully
bool syncLocalWithSupabase();

#endif // LOCAL_PROCESSING_H
