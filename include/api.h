#ifndef API_H
#define API_H

#include <WString.h>

// ============================================================================
// API CONFIGURATION
// ============================================================================
#define API_TIMEOUT_MS 30000
#define API_MAX_RETRIES 3
#define API_RETRY_DELAY_MS 1000

// ============================================================================
// API RESPONSE STRUCTURES
// ============================================================================

struct TranscriptionResult {
    bool success;
    String text;
    String error;
    int httpCode;
    
    TranscriptionResult() : success(false), httpCode(0) {}
};

struct SummaryResult {
    bool success;
    String text;
    String error;
    int httpCode;
    
    SummaryResult() : success(false), httpCode(0) {}
};

struct InfographicResult {
    bool success;
    String imageData;  // Base64 encoded image data
    String error;
    int httpCode;
    
    InfographicResult() : success(false), httpCode(0) {}
};

// ============================================================================
// API FUNCTIONS
// ============================================================================

/// Transcribe audio file using Eleven Labs API
/// @param audioFilePath Path to audio file (.wav)
/// @return TranscriptionResult with transcribed text or error
TranscriptionResult transcribeAudio(const String& audioFilePath);

/// Summarize text using Google Gemini 1.5 Flash
/// @param text Text to summarize
/// @param maxLength Maximum length of summary (0 = default)
/// @return SummaryResult with summarized text or error
SummaryResult summarizeText(const String& text, int maxLength = 0);

/// Generate an infographic image using Google Gemini 3 Pro
/// @param summaryText Text to visualize as infographic
/// @param style Style prompt for the image (optional)
/// @return InfographicResult with base64 encoded image or error
InfographicResult generateInfographic(const String& summaryText, const String& style = "");

// ============================================================================
// UTILITY FUNCTIONS
// ============================================================================



/// Get HTTP response code description
String getHttpCodeDescription(int code);

#endif // API_H
