#ifndef SUPABASE_CLIENT_H
#define SUPABASE_CLIENT_H

#include <vector>
#include <WString.h>
#include "storage.h"

// Fetch list of audio records from Supabase REST
bool fetchSupabaseAudio(std::vector<AudioFile>& out);

// Fetch summary row linked to audio id
bool fetchSupabaseSummary(const String& audioId, SummaryFile& summary);

// Download audio via signed URL to SD
bool downloadAudioFromSupabase(const AudioFile& remote, String& localPath);

// Download summary via signed URL to SD
bool downloadSummaryFromSupabase(const String& summaryId, String& localPath);

// Trigger processing pipeline (Edge Function)
bool triggerProcessAudio(const String& audioId);

#endif // SUPABASE_CLIENT_H
