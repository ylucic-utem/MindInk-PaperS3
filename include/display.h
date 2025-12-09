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
    STATE_LIST_AUDIO,
    STATE_LIST_SUMMARIES,
    STATE_VIEW_SUMMARY,
    STATE_GALLERY,
    STATE_VIEW_IMAGE,
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
void drawMessage(const String& title, const String& msg);

void drawList(String title, const std::vector<String>& items);
void drawTextView(String text);
void drawGallery(const std::vector<ImageFile>& images);
void drawImageMessage(const String& title, const String& message);
void drawImageFromFile(const String& label, const String& path);
void drawImageFromBuffer(const String& label, const uint8_t* data, size_t len);
void drawProcessing(const String& message = "Processing...");
void drawError(const String& message);
void drawPowerOffScreen();

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
    
    EbookReader() : currentPage(0), totalPages(0), linesPerPage(20), charsPerLine(60) {}
    
    void setText(const String& text);
    void paginateText();
    void nextPage();
    void prevPage();
    bool hasNextPage();
    bool hasPrevPage();
    String getCurrentPageText();
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
