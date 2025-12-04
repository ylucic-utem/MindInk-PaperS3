#ifndef API_H
#define API_H

#include <String.h>

// ============================================================================
// API RESPONSES
// ============================================================================
struct TranscriptionResult {
    bool success;
    String text;
    String error;
};

struct SummaryResult {
    bool success;
    String text;
    String error;
};

struct InfographicResult {
    bool success;
    String imageData;
    String error;
};

// ============================================================================
// API FUNCTIONS
// ============================================================================
TranscriptionResult transcribeAudio(const String& audioFilePath);
SummaryResult summarizeText(const String& text);
InfographicResult generateInfographic(const String& summaryText);

#endif // API_H
