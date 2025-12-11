#ifndef DISPLAY_H
#define DISPLAY_H

#include <vector>
#include <WString.h>
#include "storage.h"

// Include modular components
#include "ebook_reader.h"
#include "image_viewer.h"
#include "display_refresh.h"

// ============================================================================
// DESIGN SYSTEM CONSTANTS
// ============================================================================
// Screen dimensions (PaperS3: 540x960 in portrait rotation 2)
constexpr int SCREEN_WIDTH = 540;
constexpr int SCREEN_HEIGHT = 960;

// Spacing
constexpr int PADDING_SMALL = 10;
constexpr int PADDING_MEDIUM = 20;
constexpr int PADDING_LARGE = 30;

// Status Bar
constexpr int STATUS_BAR_HEIGHT = 50;

// Menu Buttons (larger touch targets)
constexpr int MENU_BTN_WIDTH = 230;
constexpr int MENU_BTN_HEIGHT = 120;
constexpr int MENU_BTN_RADIUS = 10;

// List Items
constexpr int LIST_ITEM_HEIGHT = 80;
constexpr int LIST_ITEM_RADIUS = 8;

// Navigation Buttons
constexpr int NAV_BTN_WIDTH = 140;
constexpr int NAV_BTN_HEIGHT = 50;

// Anti-ghosting refresh interval (5 minutes in ms)
constexpr unsigned long GHOST_REFRESH_INTERVAL = 300000;

// =============================================================================
// POWER MANAGEMENT CONSTANTS
// =============================================================================
// Auto-sleep timeout (3 minutes in ms for battery saving)
constexpr unsigned long AUTO_SLEEP_TIMEOUT = 180000;

// Idle warning (30 seconds before auto-sleep)
constexpr unsigned long IDLE_WARNING_TIME = 30000;

// Battery check interval (60 seconds)
constexpr unsigned long BATTERY_CHECK_INTERVAL = 60000;

// Battery thresholds
constexpr int BATTERY_LOW_THRESHOLD = 15;
constexpr int BATTERY_CRITICAL_THRESHOLD = 5;

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
    STATE_LIST_AUDIO,
    STATE_LIST_SUMMARIES,
    STATE_VIEW_SUMMARY,
    STATE_GALLERY,
    STATE_GALLERY_SUMMARIES,
    STATE_GALLERY_SAVED,
    STATE_VIEW_IMAGE,
    STATE_PROCESSING,
    STATE_ERROR,
    STATE_DEEP_SLEEP_CONFIRM
};

// ============================================================================
// UI FUNCTIONS - Core Display
// ============================================================================
void setupButtons();
bool checkButtonPress(int x, int y, const Button& btn);
void drawButton(const Button& btn);
void drawButtonPressed(const Button& btn);   // Visual press feedback
void showButtonFeedback(const Button& btn);  // Brief flash feedback
void displayStatusBar();
void drawMenu();
void drawMessage(const String& title, const String& msg);

// Touch debounce threshold (ms)
constexpr unsigned long TOUCH_DEBOUNCE_MS = 100;

// ============================================================================
// UI FUNCTIONS - Lists and Views
// ============================================================================
void drawList(String title, const std::vector<String>& items);
void drawTextView(String text);
void drawProcessing(const String& message = "Processing...");
void drawError(const String& message);
void drawPowerOffScreen();
void drawDeepSleepConfirm();
void drawDeepSleepScreen();

// ============================================================================
// DISPLAY STATE (extern declarations)
// ============================================================================
extern std::vector<Button> menuButtons;
extern AppState currentState;
extern String currentTextContent;

#endif // DISPLAY_H
