#ifndef DISPLAY_H
#define DISPLAY_H

#include <vector>
#include <WString.h>
#include "storage.h"

// ============================================================================
// UI DATA STRUCTURES
// ============================================================================
struct Button {
    int x, y, w, h;
    const char* label;
    
    Button() : x(0), y(0), w(0), h(0), label(nullptr) {}
    Button(int x_, int y_, int w_, int h_, const char* label_) 
        : x(x_), y(y_), w(w_), h(h_), label(label_) {}
};

enum AppState {
    STATE_MENU,
    STATE_RECORDING,
    STATE_LIST_AUDIO,
    STATE_LIST_SUMMARIES,
    STATE_VIEW_SUMMARY,
    STATE_GALLERY,
    STATE_PROCESSING,
    STATE_AUDIO_PLAYER,
    STATE_ERROR
};

// ============================================================================
// UI FUNCTIONS
// ============================================================================
void setupButtons();
bool checkButtonPress(int x, int y, const Button& btn);
void drawButton(const Button& btn);
void displayStatusBar();
void drawMenu();
void drawAudioPlayer(String filename, bool isPlaying, bool isPaused);
void drawRecordingPage(bool isRecording, bool isPaused, bool hasRecordedData);

void drawList(String title, const std::vector<String>& items);
void drawTextView(String text);
void drawGallery(const std::vector<ImageFile>& images);
void drawProcessing(const String& message = "Processing...");
void drawError(const String& message);
void drawPowerOffScreen();

// ============================================================================
// DISPLAY STATE
// ============================================================================
extern std::vector<Button> menuButtons;
extern AppState currentState;
extern String currentTextContent;

#endif // DISPLAY_H
