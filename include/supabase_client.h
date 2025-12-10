#ifndef SUPABASE_CLIENT_H
#define SUPABASE_CLIENT_H

#include <vector>
#include <WString.h>
#include "storage.h"

// Fetch list of audio records from Supabase REST
bool fetchSupabaseAudio(std::vector<AudioFile>& out);

// Fetch summary row linked to audio id
bool fetchSupabaseSummary(const String& audioId, SummaryFile& summary);

// Fetch list of summaries (recent)
bool fetchSupabaseSummaries(std::vector<SummaryFile>& out);

// Fetch list of infographics
bool fetchSupabaseImages(std::vector<ImageFile>& out);

// Download audio via signed URL to SD
bool downloadAudioFromSupabase(const AudioFile& remote, String& localPath);

// Download summary via signed URL to SD
bool downloadSummaryFromSupabase(const String& summaryId, String& localPath);

// Download infographic via signed URL to SD
bool downloadImageFromSupabase(const String& imageId, String& localPath, const String& filename);

// Fetch summary text into memory (when no SD)
bool fetchSummaryTextFromSupabase(const String& summaryId, String& outText);

// Fetch infographic bytes into memory (when no SD)
bool fetchImageToBuffer(const String& imageId, std::vector<uint8_t>& outBuf);

// Trigger processing pipeline (Edge Function)
bool triggerProcessAudio(const String& audioId);

// Trigger infographic generation for a summary
bool triggerGenerateInfographic(const String& summaryId);

// Fetch infographic for a summary and save to SD card
bool fetchAndSaveInfographicToSD(const String& summaryId, String& localPath);

#endif // SUPABASE_CLIENT_H
