#ifndef AUDIO_H
#define AUDIO_H

#include <String.h>
#include "storage.h"

// ============================================================================
// AUDIO RECORDING
// ============================================================================
extern bool isRecording;

void startAudioRecording(AudioFile& audioFile);
void stopAudioRecording();
void getRecentAudioFiles(std::vector<AudioFile>& files);

#endif // AUDIO_H
