#include "display.h"
#include "storage.h"
#include <M5Unified.h>
#include <WiFi.h>

// ============================================================================
// DISPLAY STATE
// ============================================================================
std::vector<Button> menuButtons;
AppState currentState = STATE_MENU;
String currentTextContent = "";
EbookReader ebookReader;

// ============================================================================
// EBOOK READER IMPLEMENTATION
// ============================================================================

void EbookReader::setText(const String& text) {
    fullText = text;
    currentPage = 0;
    pages.clear();
    paginateText();
}

void EbookReader::paginateText() {
    pages.clear();
    if (fullText.length() == 0) {
        totalPages = 0;
        return;
    }
    
    String remaining = fullText;
    remaining.trim();
    
    while (remaining.length() > 0) {
        String pageText = "";
        int lineCount = 0;
        
        // Build page line by line
        while (lineCount < linesPerPage && remaining.length() > 0) {
            String line = "";
            
            // Word wrapping: take words until we reach charsPerLine
            while (remaining.length() > 0 && line.length() < charsPerLine) {
                // Find next space or newline
                int spaceIdx = remaining.indexOf(' ');
                int newlineIdx = remaining.indexOf('\n');
                
                // Handle explicit newline
                if (newlineIdx >= 0 && (newlineIdx < spaceIdx || spaceIdx < 0)) {
                    String word = remaining.substring(0, newlineIdx);
                    if (line.length() + word.length() <= charsPerLine) {
                        if (line.length() > 0) line += " ";
                        line += word;
                        remaining = remaining.substring(newlineIdx + 1);
                        break; // Force new line
                    } else if (line.length() == 0) {
                        // Word too long for line, take what fits
                        line = word.substring(0, charsPerLine);
                        remaining = word.substring(charsPerLine) + remaining.substring(newlineIdx);
                    }
                    break;
                }
                
                // Handle space-separated word
                if (spaceIdx >= 0) {
                    String word = remaining.substring(0, spaceIdx);
                    if (line.length() == 0) {
                        // First word on line
                        if (word.length() <= charsPerLine) {
                            line = word;
                            remaining = remaining.substring(spaceIdx + 1);
                        } else {
                            // Single word too long, break it
                            line = word.substring(0, charsPerLine);
                            remaining = word.substring(charsPerLine) + remaining.substring(spaceIdx);
                            break;
                        }
                    } else {
                        // Not first word
                        if (line.length() + 1 + word.length() <= charsPerLine) {
                            line += " " + word;
                            remaining = remaining.substring(spaceIdx + 1);
                        } else {
                            break; // Word doesn't fit, move to next line
                        }
                    }
                } else {
                    // Last word in text
                    if (line.length() == 0) {
                        if (remaining.length() <= charsPerLine) {
                            line = remaining;
                            remaining = "";
                        } else {
                            line = remaining.substring(0, charsPerLine);
                            remaining = remaining.substring(charsPerLine);
                        }
                    } else {
                        if (line.length() + 1 + remaining.length() <= charsPerLine) {
                            line += " " + remaining;
                            remaining = "";
                        } else {
                            break;
                        }
                    }
                }
            }
            
            if (line.length() > 0) {
                if (pageText.length() > 0) pageText += "\n";
                pageText += line;
                lineCount++;
            }
            
            remaining.trim();
        }
        
        if (pageText.length() > 0) {
            pages.push_back(pageText);
        }
    }
    
    totalPages = pages.size();
}

void EbookReader::nextPage() {
    if (currentPage < totalPages - 1) {
        currentPage++;
    }
}

void EbookReader::prevPage() {
    if (currentPage > 0) {
        currentPage--;
    }
}

bool EbookReader::hasNextPage() {
    return currentPage < totalPages - 1;
}

bool EbookReader::hasPrevPage() {
    return currentPage > 0;
}

String EbookReader::getCurrentPageText() {
    if (currentPage >= 0 && currentPage < pages.size()) {
        return pages[currentPage];
    }
    return "";
}

// ============================================================================
// UI SETUP
// ============================================================================

void setupButtons() {
    menuButtons.clear();
    // New 4-button layout for Cloud Remote & Viewer
    Button btn1 = {50, 100, 200, 80, "AUDIO FILES"};      // Remote Trigger
    menuButtons.push_back(btn1);
    Button btn2 = {300, 100, 200, 80, "SUMMARIES"};       // View Text
    menuButtons.push_back(btn2);
    Button btn3 = {50, 220, 200, 80, "GALLERY"};          // View Images
    menuButtons.push_back(btn3);
    Button btn4 = {300, 220, 200, 80, "POWER"};           // Power Options
    menuButtons.push_back(btn4);
}

bool checkButtonPress(int x, int y, const Button& btn) {
    return (x >= btn.x && x <= btn.x + btn.w && y >= btn.y && y <= btn.y + btn.h);
}

void drawButton(const Button& btn) {
    // Draw double border (no background fill)
    M5.Display.drawRect(btn.x, btn.y, btn.w, btn.h, TFT_BLACK);
    M5.Display.drawRect(btn.x + 2, btn.y + 2, btn.w - 4, btn.h - 4, TFT_BLACK);
    
    // Draw text centered on white background
    M5.Display.setTextColor(TFT_BLACK, TFT_WHITE);
    M5.Display.setTextSize(1);
    M5.Display.drawCenterString(btn.label, btn.x + btn.w/2, btn.y + btn.h/2 - 8);
}

void displayStatusBar() {
    M5.Display.fillRect(0, 0, M5.Display.width(), 30, TFT_BLACK);
    M5.Display.setTextColor(TFT_WHITE, TFT_BLACK);
    M5.Display.setTextSize(1);
    
    String wifiStatus = WiFi.status() == WL_CONNECTED ? "WiFi: ON" : "WiFi: OFF";
    M5.Display.drawString(wifiStatus, 10, 8);
    
    String storageStatus = storage.activeStorage == STORAGE_SD ? "SD" : 
                          (storage.activeStorage == STORAGE_PSRAM ? "PSRAM" : "NONE");
    M5.Display.drawString("Storage: " + storageStatus, 150, 8);
}

// ============================================================================
// DRAWING FUNCTIONS
// ============================================================================

void drawMenu() {
    M5.Display.startWrite();
    M5.Display.fillScreen(TFT_WHITE);
    displayStatusBar();
    
    M5.Display.setTextColor(TFT_BLACK, TFT_WHITE);
    M5.Display.setTextSize(2);
    M5.Display.drawCenterString("MindInk", M5.Display.width()/2, 50);
    
    for (const auto& btn : menuButtons) {
        drawButton(btn);
    }
    
    M5.Display.setTextSize(1);
    M5.Display.setTextColor(TFT_DARKGREY, TFT_WHITE);
    String info = storage.activeStorage == STORAGE_SD ? "Storage: SD Card" : 
                  (storage.activeStorage == STORAGE_PSRAM ? "Storage: PSRAM" : "Storage: None");
    M5.Display.drawCenterString(info, M5.Display.width()/2, M5.Display.height() - 20);
    M5.Display.endWrite();
}

void drawList(String title, const std::vector<String>& items) {
    M5.Display.fillScreen(TFT_WHITE);
    displayStatusBar();
    
    M5.Display.setTextColor(TFT_BLACK, TFT_WHITE);
    M5.Display.setTextSize(2);
    M5.Display.drawCenterString(title, M5.Display.width()/2, 50);
    
    M5.Display.setTextSize(1);
    int y = 100;
    for (size_t i = 0; i < items.size(); i++) {
        M5.Display.drawRect(20, y, M5.Display.width() - 40, 50, TFT_BLACK);
        M5.Display.drawString(items[i], 30, y + 15);
        y += 60;
    }
    
    M5.Display.fillRect(20, M5.Display.height() - 60, 100, 40, TFT_LIGHTGREY);
    M5.Display.setTextColor(TFT_BLACK, TFT_LIGHTGREY);
    M5.Display.drawCenterString("BACK", 70, M5.Display.height() - 45);
}

void drawTextView(String text) {
    M5.Display.fillScreen(TFT_WHITE);
    displayStatusBar();
    
    M5.Display.setTextColor(TFT_BLACK, TFT_WHITE);
    M5.Display.setTextSize(1);
    M5.Display.drawString("Summary", 20, 50);
    M5.Display.setCursor(20, 80);
    M5.Display.print(text);
    
    M5.Display.fillRect(20, M5.Display.height() - 60, 100, 40, TFT_LIGHTGREY);
    M5.Display.setTextColor(TFT_BLACK, TFT_LIGHTGREY);
    M5.Display.drawCenterString("BACK", 70, M5.Display.height() - 45);
}

void drawGallery(const std::vector<ImageFile>& images) {
    M5.Display.fillScreen(TFT_WHITE);
    displayStatusBar();
    
    M5.Display.setTextColor(TFT_BLACK, TFT_WHITE);
    M5.Display.setTextSize(2);
    M5.Display.drawCenterString("Image Gallery", M5.Display.width()/2, 50);
    
    int x = 20, y = 100;
    for(const auto& img : images) {
        M5.Display.drawRect(x, y, 150, 150, TFT_BLACK);
        M5.Display.setTextSize(1);
        M5.Display.drawString(img.filename.substring(0, 15), x + 5, y + 70);
        x += 160;
        if(x > M5.Display.width() - 160) { x = 20; y += 160; }
    }
    
    M5.Display.fillRect(20, M5.Display.height() - 60, 100, 40, TFT_LIGHTGREY);
    M5.Display.setTextColor(TFT_BLACK, TFT_LIGHTGREY);
    M5.Display.drawCenterString("BACK", 70, M5.Display.height() - 45);
}

void drawImageMessage(const String& title, const String& message) {
    M5.Display.fillScreen(TFT_WHITE);
    displayStatusBar();
    M5.Display.setTextColor(TFT_BLACK, TFT_WHITE);
    M5.Display.setTextSize(2);
    M5.Display.drawCenterString(title, M5.Display.width()/2, 50);
    M5.Display.setTextSize(1);
    M5.Display.drawCenterString(message, M5.Display.width()/2, M5.Display.height()/2);
    M5.Display.fillRect(20, M5.Display.height() - 60, 100, 40, TFT_LIGHTGREY);
    M5.Display.setTextColor(TFT_BLACK, TFT_LIGHTGREY);
    M5.Display.drawCenterString("BACK", 70, M5.Display.height() - 45);
}

void drawImageFromFile(const String& label, const String& path) {
    M5.Display.fillScreen(TFT_WHITE);
    displayStatusBar();
    M5.Display.setTextColor(TFT_BLACK, TFT_WHITE);
    M5.Display.setTextSize(2);
    M5.Display.drawCenterString(label, M5.Display.width()/2, 40);
    M5.Display.setTextSize(1);

    bool drawn = false;
    if (path.endsWith(".png")) {
        M5.Display.drawPngFile(path.c_str(), 0, 60);
        drawn = true;
    } else if (path.endsWith(".jpg") || path.endsWith(".jpeg")) {
        M5.Display.drawJpgFile(path.c_str(), 0, 60);
        drawn = true;
    }

    if (!drawn) {
        M5.Display.drawCenterString("Unsupported image format", M5.Display.width()/2, M5.Display.height()/2);
    }

    M5.Display.fillRect(20, M5.Display.height() - 60, 100, 40, TFT_LIGHTGREY);
    M5.Display.setTextColor(TFT_BLACK, TFT_LIGHTGREY);
    M5.Display.drawCenterString("BACK", 70, M5.Display.height() - 45);
}

void drawImageFromBuffer(const String& label, const uint8_t* data, size_t len) {
    M5.Display.fillScreen(TFT_WHITE);
    displayStatusBar();
    M5.Display.setTextColor(TFT_BLACK, TFT_WHITE);
    M5.Display.setTextSize(2);
    M5.Display.drawCenterString(label, M5.Display.width()/2, 40);
    M5.Display.setTextSize(1);

    // Attempt PNG first; fall back to JPG.
    bool drawn = false;
    if (M5.Display.drawPng(data, len, 0, 60) == 0) {
        drawn = true;
    } else if (M5.Display.drawJpg(data, len, 0, 60) == 0) {
        drawn = true;
    }

    if (!drawn) {
        M5.Display.drawCenterString("Unable to render image", M5.Display.width()/2, M5.Display.height()/2);
    }

    M5.Display.fillRect(20, M5.Display.height() - 60, 100, 40, TFT_LIGHTGREY);
    M5.Display.setTextColor(TFT_BLACK, TFT_LIGHTGREY);
    M5.Display.drawCenterString("BACK", 70, M5.Display.height() - 45);
}

void drawProcessing(const String& message) {
    M5.Display.fillScreen(TFT_WHITE);
    displayStatusBar();
    
    M5.Display.setTextColor(TFT_BLACK, TFT_WHITE);
    M5.Display.setTextSize(2);
    M5.Display.drawCenterString(message, M5.Display.width()/2, M5.Display.height()/2 - 20);
    
    M5.Display.setTextSize(1);
    M5.Display.drawCenterString("Please wait...", M5.Display.width()/2, M5.Display.height()/2 + 40);
}

void drawError(const String& message) {
    M5.Display.fillScreen(TFT_WHITE);
    displayStatusBar();
    
    M5.Display.setTextColor(TFT_RED, TFT_WHITE);
    M5.Display.setTextSize(2);
    M5.Display.drawCenterString("ERROR", M5.Display.width()/2, 50);
    
    M5.Display.setTextSize(1);
    M5.Display.setTextColor(TFT_BLACK, TFT_WHITE);
    M5.Display.drawString(message, 20, M5.Display.height()/2);
    
    M5.Display.fillRect(20, M5.Display.height() - 60, 100, 40, TFT_LIGHTGREY);
    M5.Display.setTextColor(TFT_BLACK, TFT_LIGHTGREY);
    M5.Display.drawCenterString("BACK", 70, M5.Display.height() - 45);
}

// Helper to draw a standard control button
void drawControlBtn(int x, int y, int w, int h, const char* label, bool active = false) {
    M5.Display.fillRect(x, y, w, h, active ? TFT_DARKGREY : TFT_LIGHTGREY);
    M5.Display.drawRect(x, y, w, h, TFT_BLACK);
    M5.Display.setTextColor(active ? TFT_WHITE : TFT_BLACK, active ? TFT_DARKGREY : TFT_LIGHTGREY);
    M5.Display.setTextSize(1);
    M5.Display.drawCenterString(label, x + w/2, y + h/2 - 5);
}

// ============================================================================
// MESSAGE DISPLAY (for confirmation dialogs)
// ============================================================================

void drawMessage(const String& title, const String& msg) {
    M5.Display.fillScreen(TFT_WHITE);
    displayStatusBar();
    
    // Title
    M5.Display.setTextColor(TFT_BLACK, TFT_WHITE);
    M5.Display.setTextSize(2);
    M5.Display.drawCenterString(title, M5.Display.width()/2, 80);
    
    // Message body
    M5.Display.setTextSize(1);
    M5.Display.setTextColor(TFT_DARKGREY, TFT_WHITE);
    
    // Word wrap the message across multiple lines
    int lineY = 150;
    int maxWidth = M5.Display.width() - 40;
    String remaining = msg;
    
    while (remaining.length() > 0 && lineY < M5.Display.height() - 80) {
        String line = "";
        int charIdx = 0;
        
        // Build line that fits within maxWidth
        while (charIdx < remaining.length()) {
            String testLine = line + remaining.charAt(charIdx);
            if (M5.Display.textWidth(testLine) > maxWidth) {
                break;
            }
            line = testLine;
            charIdx++;
        }
        
        if (line.length() == 0 && remaining.length() > 0) {
            // Single word too long, just take characters that fit
            line = remaining.substring(0, 1);
            charIdx = 1;
        }
        
        M5.Display.drawCenterString(line, M5.Display.width()/2, lineY);
        remaining = remaining.substring(charIdx);
        remaining.trim();
        lineY += 25;
    }
    
    // OK button to dismiss
    int btnW = 100;
    int btnH = 40;
    int btnX = (M5.Display.width() - btnW) / 2;
    int btnY = M5.Display.height() - 60;
    M5.Display.fillRect(btnX, btnY, btnW, btnH, TFT_LIGHTGREY);
    M5.Display.drawRect(btnX, btnY, btnW, btnH, TFT_BLACK);
    M5.Display.setTextColor(TFT_BLACK, TFT_LIGHTGREY);
    M5.Display.drawCenterString("OK", btnX + btnW/2, btnY + btnH/2 - 5);
}

// ============================================================================
// POWER OFF SCREEN
// ============================================================================

void drawPowerOffScreen() {
    M5.Display.fillScreen(TFT_WHITE);
    M5.Display.setTextColor(TFT_BLACK, TFT_WHITE);
    M5.Display.setTextSize(4);
    M5.Display.drawCenterString("MindInk", M5.Display.width()/2, M5.Display.height()/2 - 20);
}

// ============================================================================
// EBOOK READER VIEW
// ============================================================================

void drawEbookPage() {
    M5.Display.startWrite();
    M5.Display.fillScreen(TFT_WHITE);
    displayStatusBar();
    
    // Title bar
    M5.Display.setTextColor(TFT_BLACK, TFT_WHITE);
    M5.Display.setTextSize(1);
    M5.Display.drawString("Summary", 20, 40);
    
    // Page indicator
    String pageInfo = "Page " + String(ebookReader.currentPage + 1) + " / " + String(ebookReader.totalPages);
    M5.Display.drawRightString(pageInfo, M5.Display.width() - 20, 40);
    
    // Draw horizontal line separator
    M5.Display.drawLine(20, 60, M5.Display.width() - 20, 60, TFT_BLACK);
    
    // Text content area (monospace font for consistent wrapping)
    M5.Display.setTextColor(TFT_BLACK, TFT_WHITE);
    M5.Display.setFont(&fonts::Font0); // Use default monospace font
    M5.Display.setCursor(20, 75);
    
    String pageText = ebookReader.getCurrentPageText();
    M5.Display.print(pageText);
    
    // Navigation buttons at bottom
    int btnY = M5.Display.height() - 60;
    int btnH = 45;
    int btnW = 100;
    int centerBtnW = 100;
    
    // Previous button (left)
    if (ebookReader.hasPrevPage()) {
        M5.Display.fillRect(20, btnY, btnW, btnH, TFT_LIGHTGREY);
        M5.Display.drawRect(20, btnY, btnW, btnH, TFT_BLACK);
        M5.Display.setTextColor(TFT_BLACK, TFT_LIGHTGREY);
        M5.Display.drawCenterString("< PREV", 20 + btnW/2, btnY + btnH/2 - 5);
    }
    
    // Back button (center)
    int backBtnX = (M5.Display.width() - centerBtnW) / 2;
    M5.Display.fillRect(backBtnX, btnY, centerBtnW, btnH, TFT_DARKGREY);
    M5.Display.drawRect(backBtnX, btnY, centerBtnW, btnH, TFT_BLACK);
    M5.Display.setTextColor(TFT_WHITE, TFT_DARKGREY);
    M5.Display.drawCenterString("BACK", backBtnX + centerBtnW/2, btnY + btnH/2 - 5);
    
    // Next button (right)
    if (ebookReader.hasNextPage()) {
        int nextBtnX = M5.Display.width() - 20 - btnW;
        M5.Display.fillRect(nextBtnX, btnY, btnW, btnH, TFT_LIGHTGREY);
        M5.Display.drawRect(nextBtnX, btnY, btnW, btnH, TFT_BLACK);
        M5.Display.setTextColor(TFT_BLACK, TFT_LIGHTGREY);
        M5.Display.drawCenterString("NEXT >", nextBtnX + btnW/2, btnY + btnH/2 - 5);
    }
    
    M5.Display.endWrite();
}

bool handleEbookTouch(int x, int y) {
    int btnY = M5.Display.height() - 60;
    int btnH = 45;
    int btnW = 100;
    int centerBtnW = 100;
    
    // Check Previous button
    if (ebookReader.hasPrevPage() && x >= 20 && x <= 20 + btnW && y >= btnY && y <= btnY + btnH) {
        ebookReader.prevPage();
        drawEbookPage();
        return true;
    }
    
    // Check Back button
    int backBtnX = (M5.Display.width() - centerBtnW) / 2;
    if (x >= backBtnX && x <= backBtnX + centerBtnW && y >= btnY && y <= btnY + btnH) {
        return false; // Signal to exit ebook view
    }
    
    // Check Next button
    int nextBtnX = M5.Display.width() - 20 - btnW;
    if (ebookReader.hasNextPage() && x >= nextBtnX && x <= nextBtnX + btnW && y >= btnY && y <= btnY + btnH) {
        ebookReader.nextPage();
        drawEbookPage();
        return true;
    }
    
    return true; // Stay in ebook view
}
