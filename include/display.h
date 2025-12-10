#ifndef DISPLAY_H
#define DISPLAY_H

#include <vector>
#include <WString.h>
#include "storage.h"

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

// Auto-sleep timeout (10 minutes in ms)
constexpr unsigned long AUTO_SLEEP_TIMEOUT = 600000;

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
// UI FUNCTIONS
// ============================================================================
void setupButtons();
bool checkButtonPress(int x, int y, const Button& btn);
void drawButton(const Button& btn);
void displayStatusBar();
void drawMenu();
void drawMessage(const String& title, const String& msg);

void drawList(String title, const std::vector<String>& items);
void drawTextView(String text);
void drawGallery(const std::vector<ImageFile>& images);
void drawSummariesForInfographic(const std::vector<SummaryFile>& summaries);
void drawInfographicsOnSD(const std::vector<String>& infographics);
void drawImageMessage(const String& title, const String& message);
void drawImageFromFile(const String& label, const String& path);
void drawImageFromBuffer(const String& label, const uint8_t* data, size_t len);
void drawProcessing(const String& message = "Processing...");
void drawError(const String& message);
void drawPowerOffScreen();
void drawDeepSleepConfirm();
void drawDeepSleepScreen();

// Anti-ghosting
void checkAndRefresh();
void forceFullRefresh();

// ============================================================================
// EBOOK READER STATE
// ============================================================================
struct EbookReader {
    String fullText;
    std::vector<String> pages;
    int currentPage;
    int totalPages;
    int linesPerPage;
    int charsPerLine;
    int fontSize;  // Font size level: 0=16pt, 1=20pt, 2=24pt, 3=28pt
    
    EbookReader() : currentPage(0), totalPages(0), linesPerPage(12), charsPerLine(40), fontSize(1) {}
    
    void setText(const String& text);
    void paginateText();
    void nextPage();
    void prevPage();
    bool hasNextPage();
    bool hasPrevPage();
    String getCurrentPageText();
    void increaseFontSize();
    void decreaseFontSize();
    void recalculateLayout();
};

// ============================================================================
// DISPLAY STATE
// ============================================================================
extern std::vector<Button> menuButtons;
extern AppState currentState;
extern String currentTextContent;
extern EbookReader ebookReader;

void drawEbookPage();
bool handleEbookTouch(int x, int y);

#endif // DISPLAY_H
