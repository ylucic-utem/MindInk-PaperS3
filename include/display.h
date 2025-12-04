#ifndef DISPLAY_H
#define DISPLAY_H

#include <vector>
#include <String.h>
#include "storage.h"

// ============================================================================
// UI DATA STRUCTURES
// ============================================================================
struct Button {
    int x, y, w, h;
    String label;
};

enum AppState {
    STATE_MENU,
    STATE_RECORDING,
    STATE_LIST_AUDIO,
    STATE_LIST_SUMMARIES,
    STATE_VIEW_SUMMARY,
    STATE_GALLERY,
    STATE_PROCESSING,
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
void drawList(String title, const std::vector<String>& items);
void drawTextView(String text);
void drawGallery(const std::vector<ImageFile>& images);
void drawProcessing(const String& message = "Processing...");
void drawError(const String& message);

// ============================================================================
// DISPLAY STATE
// ============================================================================
extern std::vector<Button> menuButtons;
extern AppState currentState;
extern String currentTextContent;

#endif // DISPLAY_H
