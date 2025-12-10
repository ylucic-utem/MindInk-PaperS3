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

void EbookReader::increaseFontSize() {
    if (fontSize < 3) {
        fontSize++;
        recalculateLayout();
    }
}

void EbookReader::decreaseFontSize() {
    if (fontSize > 0) {
        fontSize--;
        recalculateLayout();
    }
}

void EbookReader::recalculateLayout() {
    // Adjust lines per page and chars per line based on font size
    // fontSize: 0=16pt, 1=20pt, 2=24pt, 3=28pt
    switch (fontSize) {
        case 0: linesPerPage = 14; charsPerLine = 42; break;  // 16pt - most text
        case 1: linesPerPage = 11; charsPerLine = 35; break;  // 20pt - default
        case 2: linesPerPage = 9;  charsPerLine = 30; break;  // 24pt - larger
        case 3: linesPerPage = 7;  charsPerLine = 25; break;  // 28pt - very large
    }
    
    // Remember current position and re-paginate
    int oldPage = currentPage;
    paginateText();
    
    // Try to stay at approximately the same position
    if (oldPage >= totalPages) {
        currentPage = totalPages > 0 ? totalPages - 1 : 0;
    } else {
        currentPage = oldPage;
    }
}

// ============================================================================
// UI SETUP
// ============================================================================

void setupButtons() {
    menuButtons.clear();
    // 5-button layout with larger touch targets (MENU_BTN_WIDTH x MENU_BTN_HEIGHT)
    // Row 1: y = 200
    Button btn1 = {40, 200, MENU_BTN_WIDTH, MENU_BTN_HEIGHT, "AUDIO FILES"};
    menuButtons.push_back(btn1);
    Button btn2 = {40 + MENU_BTN_WIDTH + 20, 200, MENU_BTN_WIDTH, MENU_BTN_HEIGHT, "SUMMARIES"};
    menuButtons.push_back(btn2);
    // Row 2: y = 200 + MENU_BTN_HEIGHT + 30 = 350
    Button btn3 = {40, 350, MENU_BTN_WIDTH, MENU_BTN_HEIGHT, "GALLERY"};
    menuButtons.push_back(btn3);
    Button btn4 = {40 + MENU_BTN_WIDTH + 20, 350, MENU_BTN_WIDTH, MENU_BTN_HEIGHT, "POWER OFF"};
    menuButtons.push_back(btn4);
    // Row 3: Deep Sleep button centered, y = 500
    Button btn5 = {(SCREEN_WIDTH - MENU_BTN_WIDTH) / 2, 500, MENU_BTN_WIDTH, MENU_BTN_HEIGHT, "DEEP SLEEP"};
    menuButtons.push_back(btn5);
}

bool checkButtonPress(int x, int y, const Button& btn) {
    return (x >= btn.x && x <= btn.x + btn.w && y >= btn.y && y <= btn.y + btn.h);
}

void drawButton(const Button& btn) {
    // Draw rounded rectangle border (no background fill, e-ink optimized)
    M5.Display.drawRoundRect(btn.x, btn.y, btn.w, btn.h, MENU_BTN_RADIUS, TFT_BLACK);
    M5.Display.drawRoundRect(btn.x + 2, btn.y + 2, btn.w - 4, btn.h - 4, MENU_BTN_RADIUS - 2, TFT_BLACK);
    
    // Draw text centered with larger font
    M5.Display.setTextColor(TFT_BLACK, TFT_WHITE);
    M5.Display.setFont(&fonts::lgfxJapanGothic_16);
    M5.Display.drawCenterString(btn.label, btn.x + btn.w/2, btn.y + btn.h/2 - 8);
    M5.Display.setFont(nullptr); // Reset to default
}

void displayStatusBar() {
    // Draw status bar background (50px height)
    M5.Display.fillRect(0, 0, SCREEN_WIDTH, STATUS_BAR_HEIGHT, TFT_BLACK);
    M5.Display.setTextColor(TFT_WHITE, TFT_BLACK);
    M5.Display.setFont(&fonts::lgfxJapanGothic_16);
    
    // WiFi status indicator (left side)
    bool wifiConnected = (WiFi.status() == WL_CONNECTED);
    String wifiStr = wifiConnected ? "WiFi" : "No WiFi";
    M5.Display.drawString(wifiStr, PADDING_MEDIUM, 15);
    
    // WiFi signal bars (simple representation)
    int barX = 80;
    int barY = 35;
    if (wifiConnected) {
        int rssi = WiFi.RSSI();
        int bars = (rssi > -50) ? 4 : (rssi > -60) ? 3 : (rssi > -70) ? 2 : 1;
        for (int i = 0; i < bars; i++) {
            M5.Display.fillRect(barX + i * 8, barY - (i + 1) * 4, 5, (i + 1) * 4, TFT_WHITE);
        }
    }
    
    // Battery percentage (center-right)
    int batteryLevel = M5.Power.getBatteryLevel();
    String batteryStr = String(batteryLevel) + "%";
    M5.Display.drawString(batteryStr, SCREEN_WIDTH / 2 + 60, 15);
    
    // Battery icon
    int batX = SCREEN_WIDTH / 2 + 110;
    int batY = 15;
    M5.Display.drawRect(batX, batY, 30, 16, TFT_WHITE);
    M5.Display.fillRect(batX + 30, batY + 4, 4, 8, TFT_WHITE);
    int fillWidth = (batteryLevel * 26) / 100;
    M5.Display.fillRect(batX + 2, batY + 2, fillWidth, 12, TFT_WHITE);
    
    // Storage indicator (right side)
    String storageStr = storage.activeStorage == STORAGE_SD ? "SD" : 
                        (storage.activeStorage == STORAGE_PSRAM ? "RAM" : "--");
    M5.Display.drawRightString(storageStr, SCREEN_WIDTH - PADDING_MEDIUM, 15);
    
    // Bottom separator line
    M5.Display.drawLine(0, STATUS_BAR_HEIGHT - 1, SCREEN_WIDTH, STATUS_BAR_HEIGHT - 1, TFT_DARKGREY);
    
    M5.Display.setFont(nullptr); // Reset font
}

// ============================================================================
// DRAWING FUNCTIONS
// ============================================================================

void drawMenu() {
    M5.Display.startWrite();
    M5.Display.fillScreen(TFT_WHITE);
    displayStatusBar();
    
    // Title with large font
    M5.Display.setTextColor(TFT_BLACK, TFT_WHITE);
    M5.Display.setFont(&fonts::lgfxJapanGothic_24);
    M5.Display.drawCenterString("MindInk", SCREEN_WIDTH / 2, STATUS_BAR_HEIGHT + 40);
    M5.Display.setFont(nullptr);
    
    // Draw all menu buttons
    for (const auto& btn : menuButtons) {
        drawButton(btn);
    }
    
    // Footer info
    M5.Display.setFont(&fonts::lgfxJapanGothic_12);
    M5.Display.setTextColor(TFT_DARKGREY, TFT_WHITE);
    String info = storage.activeStorage == STORAGE_SD ? "Storage: SD Card" : 
                  (storage.activeStorage == STORAGE_PSRAM ? "Storage: PSRAM" : "Storage: None");
    M5.Display.drawCenterString(info, SCREEN_WIDTH / 2, SCREEN_HEIGHT - 30);
    M5.Display.setFont(nullptr);
    M5.Display.endWrite();
}

void drawList(String title, const std::vector<String>& items) {
    M5.Display.startWrite();
    M5.Display.fillScreen(TFT_WHITE);
    displayStatusBar();
    
    // Title
    M5.Display.setTextColor(TFT_BLACK, TFT_WHITE);
    M5.Display.setFont(&fonts::lgfxJapanGothic_24);
    M5.Display.drawCenterString(title, SCREEN_WIDTH / 2, STATUS_BAR_HEIGHT + 20);
    
    // List items with larger height and font
    M5.Display.setFont(&fonts::lgfxJapanGothic_16);
    int startY = STATUS_BAR_HEIGHT + 70;
    int maxItems = (SCREEN_HEIGHT - startY - 80) / (LIST_ITEM_HEIGHT + PADDING_SMALL);
    
    for (size_t i = 0; i < items.size() && (int)i < maxItems; i++) {
        int y = startY + i * (LIST_ITEM_HEIGHT + PADDING_SMALL);
        
        // Rounded rectangle for list item
        M5.Display.drawRoundRect(PADDING_MEDIUM, y, SCREEN_WIDTH - 2 * PADDING_MEDIUM, LIST_ITEM_HEIGHT, LIST_ITEM_RADIUS, TFT_BLACK);
        
        // Item text vertically centered
        M5.Display.setTextColor(TFT_BLACK, TFT_WHITE);
        M5.Display.drawString(items[i], PADDING_LARGE, y + (LIST_ITEM_HEIGHT - 16) / 2);
    }
    
    // Back button (larger)
    int backBtnY = SCREEN_HEIGHT - 70;
    M5.Display.fillRoundRect(PADDING_MEDIUM, backBtnY, NAV_BTN_WIDTH, NAV_BTN_HEIGHT, LIST_ITEM_RADIUS, TFT_LIGHTGREY);
    M5.Display.drawRoundRect(PADDING_MEDIUM, backBtnY, NAV_BTN_WIDTH, NAV_BTN_HEIGHT, LIST_ITEM_RADIUS, TFT_BLACK);
    M5.Display.setTextColor(TFT_BLACK, TFT_LIGHTGREY);
    M5.Display.drawCenterString("BACK", PADDING_MEDIUM + NAV_BTN_WIDTH / 2, backBtnY + (NAV_BTN_HEIGHT - 16) / 2);
    
    M5.Display.setFont(nullptr);
    M5.Display.endWrite();
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
    
    // Title bar with larger font
    M5.Display.setTextColor(TFT_BLACK, TFT_WHITE);
    M5.Display.setFont(&fonts::lgfxJapanGothic_16);
    M5.Display.drawString("Resumen", PADDING_MEDIUM, STATUS_BAR_HEIGHT + 15);
    
    // Page indicator and font size indicator
    String pageInfo = String(ebookReader.currentPage + 1) + "/" + String(ebookReader.totalPages);
    M5.Display.drawRightString(pageInfo, SCREEN_WIDTH - PADDING_MEDIUM, STATUS_BAR_HEIGHT + 15);
    
    // Draw horizontal line separator
    int separatorY = STATUS_BAR_HEIGHT + 45;
    M5.Display.drawLine(PADDING_MEDIUM, separatorY, SCREEN_WIDTH - PADDING_MEDIUM, separatorY, TFT_BLACK);
    
    // Text content area
    int textStartY = separatorY + 15;
    int textAreaHeight = SCREEN_HEIGHT - textStartY - 80; // Leave room for nav buttons
    
    M5.Display.setTextColor(TFT_BLACK, TFT_WHITE);
    
    // Select font based on fontSize level (0=16pt, 1=20pt, 2=24pt, 3=28pt)
    int lineHeight;
    switch (ebookReader.fontSize) {
        case 0:
            M5.Display.setFont(&fonts::lgfxJapanGothic_16);
            lineHeight = 24;
            break;
        case 1:
            M5.Display.setFont(&fonts::lgfxJapanGothic_20);
            lineHeight = 30;
            break;
        case 2:
            M5.Display.setFont(&fonts::lgfxJapanGothic_24);
            lineHeight = 36;
            break;
        case 3:
        default:
            M5.Display.setFont(&fonts::lgfxJapanGothic_28);
            lineHeight = 42;
            break;
    }
    
    // Get page text and render with line wrapping
    String pageText = ebookReader.getCurrentPageText();
    
    // Render text line by line with proper spacing
    int cursorY = textStartY;
    int lineStart = 0;
    
    for (int i = 0; i <= (int)pageText.length(); i++) {
        if (i == (int)pageText.length() || pageText.charAt(i) == '\n') {
            String line = pageText.substring(lineStart, i);
            M5.Display.drawString(line, PADDING_LARGE, cursorY);
            cursorY += lineHeight;
            lineStart = i + 1;
            
            if (cursorY > textStartY + textAreaHeight) break;
        }
    }
    
    // Navigation buttons at bottom
    int btnY = SCREEN_HEIGHT - 70;
    int smallBtnW = 60;  // Smaller width for A+/A- buttons
    
    // Previous button (far left)
    if (ebookReader.hasPrevPage()) {
        M5.Display.fillRoundRect(PADDING_MEDIUM, btnY, NAV_BTN_WIDTH - 20, NAV_BTN_HEIGHT, LIST_ITEM_RADIUS, TFT_LIGHTGREY);
        M5.Display.drawRoundRect(PADDING_MEDIUM, btnY, NAV_BTN_WIDTH - 20, NAV_BTN_HEIGHT, LIST_ITEM_RADIUS, TFT_BLACK);
        M5.Display.setTextColor(TFT_BLACK, TFT_LIGHTGREY);
        M5.Display.setFont(&fonts::lgfxJapanGothic_16);
        M5.Display.drawCenterString("< PREV", PADDING_MEDIUM + (NAV_BTN_WIDTH - 20) / 2, btnY + (NAV_BTN_HEIGHT - 16) / 2);
    }
    
    // A- button (decrease font)
    int aMinusBtnX = PADDING_MEDIUM + NAV_BTN_WIDTH - 10;
    M5.Display.fillRoundRect(aMinusBtnX, btnY, smallBtnW, NAV_BTN_HEIGHT, LIST_ITEM_RADIUS, TFT_LIGHTGREY);
    M5.Display.drawRoundRect(aMinusBtnX, btnY, smallBtnW, NAV_BTN_HEIGHT, LIST_ITEM_RADIUS, TFT_BLACK);
    M5.Display.setTextColor(TFT_BLACK, TFT_LIGHTGREY);
    M5.Display.setFont(&fonts::lgfxJapanGothic_16);
    M5.Display.drawCenterString("A-", aMinusBtnX + smallBtnW / 2, btnY + (NAV_BTN_HEIGHT - 16) / 2);
    
    // Back button (center)
    int backBtnX = (SCREEN_WIDTH - 80) / 2;
    M5.Display.fillRoundRect(backBtnX, btnY, 80, NAV_BTN_HEIGHT, LIST_ITEM_RADIUS, TFT_DARKGREY);
    M5.Display.drawRoundRect(backBtnX, btnY, 80, NAV_BTN_HEIGHT, LIST_ITEM_RADIUS, TFT_BLACK);
    M5.Display.setTextColor(TFT_WHITE, TFT_DARKGREY);
    M5.Display.drawCenterString("BACK", backBtnX + 40, btnY + (NAV_BTN_HEIGHT - 16) / 2);
    
    // A+ button (increase font)
    int aPlusBtnX = backBtnX + 80 + 10;
    M5.Display.fillRoundRect(aPlusBtnX, btnY, smallBtnW, NAV_BTN_HEIGHT, LIST_ITEM_RADIUS, TFT_LIGHTGREY);
    M5.Display.drawRoundRect(aPlusBtnX, btnY, smallBtnW, NAV_BTN_HEIGHT, LIST_ITEM_RADIUS, TFT_BLACK);
    M5.Display.setTextColor(TFT_BLACK, TFT_LIGHTGREY);
    M5.Display.drawCenterString("A+", aPlusBtnX + smallBtnW / 2, btnY + (NAV_BTN_HEIGHT - 16) / 2);
    
    // Next button (far right)
    if (ebookReader.hasNextPage()) {
        int nextBtnX = SCREEN_WIDTH - PADDING_MEDIUM - (NAV_BTN_WIDTH - 20);
        M5.Display.fillRoundRect(nextBtnX, btnY, NAV_BTN_WIDTH - 20, NAV_BTN_HEIGHT, LIST_ITEM_RADIUS, TFT_LIGHTGREY);
        M5.Display.drawRoundRect(nextBtnX, btnY, NAV_BTN_WIDTH - 20, NAV_BTN_HEIGHT, LIST_ITEM_RADIUS, TFT_BLACK);
        M5.Display.setTextColor(TFT_BLACK, TFT_LIGHTGREY);
        M5.Display.drawCenterString("NEXT >", nextBtnX + (NAV_BTN_WIDTH - 20) / 2, btnY + (NAV_BTN_HEIGHT - 16) / 2);
    }
    
    M5.Display.setFont(nullptr);
    M5.Display.endWrite();
}

bool handleEbookTouch(int x, int y) {
    int btnY = SCREEN_HEIGHT - 70;
    int smallBtnW = 60;
    int prevBtnW = NAV_BTN_WIDTH - 20;
    int backBtnW = 80;
    int nextBtnW = NAV_BTN_WIDTH - 20;
    
    // Check Previous button (far left)
    if (ebookReader.hasPrevPage() && x >= PADDING_MEDIUM && x <= PADDING_MEDIUM + prevBtnW && y >= btnY && y <= btnY + NAV_BTN_HEIGHT) {
        ebookReader.prevPage();
        drawEbookPage();
        return true;
    }
    
    // A- button (decrease font)
    int aMinusBtnX = PADDING_MEDIUM + NAV_BTN_WIDTH - 10;
    if (x >= aMinusBtnX && x <= aMinusBtnX + smallBtnW && y >= btnY && y <= btnY + NAV_BTN_HEIGHT) {
        ebookReader.decreaseFontSize();
        drawEbookPage();
        return true;
    }
    
    // Check Back button (center)
    int backBtnX = (SCREEN_WIDTH - backBtnW) / 2;
    if (x >= backBtnX && x <= backBtnX + backBtnW && y >= btnY && y <= btnY + NAV_BTN_HEIGHT) {
        return false; // Signal to exit ebook view
    }
    
    // A+ button (increase font)
    int aPlusBtnX = backBtnX + backBtnW + 10;
    if (x >= aPlusBtnX && x <= aPlusBtnX + smallBtnW && y >= btnY && y <= btnY + NAV_BTN_HEIGHT) {
        ebookReader.increaseFontSize();
        drawEbookPage();
        return true;
    }
    
    // Check Next button (far right)
    int nextBtnX = SCREEN_WIDTH - PADDING_MEDIUM - nextBtnW;
    if (ebookReader.hasNextPage() && x >= nextBtnX && x <= nextBtnX + nextBtnW && y >= btnY && y <= btnY + NAV_BTN_HEIGHT) {
        ebookReader.nextPage();
        drawEbookPage();
        return true;
    }
    
    return true; // Stay in ebook view
}

// ============================================================================
// DEEP SLEEP SCREENS
// ============================================================================

void drawDeepSleepConfirm() {
    M5.Display.startWrite();
    M5.Display.fillScreen(TFT_WHITE);
    displayStatusBar();
    
    // Title
    M5.Display.setTextColor(TFT_BLACK, TFT_WHITE);
    M5.Display.setFont(&fonts::lgfxJapanGothic_24);
    M5.Display.drawCenterString("Enter Deep Sleep?", SCREEN_WIDTH / 2, SCREEN_HEIGHT / 2 - 100);
    
    // Description
    M5.Display.setFont(&fonts::lgfxJapanGothic_16);
    M5.Display.setTextColor(TFT_DARKGREY, TFT_WHITE);
    M5.Display.drawCenterString("Press physical button to wake", SCREEN_WIDTH / 2, SCREEN_HEIGHT / 2 - 40);
    
    // YES button
    int btnY = SCREEN_HEIGHT / 2 + 30;
    int btnSpacing = 30;
    
    M5.Display.fillRoundRect(SCREEN_WIDTH / 2 - MENU_BTN_WIDTH - btnSpacing / 2, btnY, MENU_BTN_WIDTH, 80, MENU_BTN_RADIUS, TFT_LIGHTGREY);
    M5.Display.drawRoundRect(SCREEN_WIDTH / 2 - MENU_BTN_WIDTH - btnSpacing / 2, btnY, MENU_BTN_WIDTH, 80, MENU_BTN_RADIUS, TFT_BLACK);
    M5.Display.setTextColor(TFT_BLACK, TFT_LIGHTGREY);
    M5.Display.drawCenterString("YES", SCREEN_WIDTH / 2 - MENU_BTN_WIDTH / 2 - btnSpacing / 2, btnY + 32);
    
    // CANCEL button
    M5.Display.fillRoundRect(SCREEN_WIDTH / 2 + btnSpacing / 2, btnY, MENU_BTN_WIDTH, 80, MENU_BTN_RADIUS, TFT_DARKGREY);
    M5.Display.drawRoundRect(SCREEN_WIDTH / 2 + btnSpacing / 2, btnY, MENU_BTN_WIDTH, 80, MENU_BTN_RADIUS, TFT_BLACK);
    M5.Display.setTextColor(TFT_WHITE, TFT_DARKGREY);
    M5.Display.drawCenterString("CANCEL", SCREEN_WIDTH / 2 + MENU_BTN_WIDTH / 2 + btnSpacing / 2, btnY + 32);
    
    M5.Display.setFont(nullptr);
    M5.Display.endWrite();
}

void drawDeepSleepScreen() {
    // Full refresh before sleep (anti-ghosting)
    M5.Display.fillScreen(TFT_BLACK);
    delay(100);
    M5.Display.fillScreen(TFT_WHITE);
    
    // Display "MindInk" centered
    M5.Display.setTextColor(TFT_BLACK, TFT_WHITE);
    M5.Display.setFont(&fonts::lgfxJapanGothic_24);
    M5.Display.drawCenterString("MindInk", SCREEN_WIDTH / 2, SCREEN_HEIGHT / 2 - 20);
    M5.Display.setFont(nullptr);
}

// ============================================================================
// ANTI-GHOSTING
// ============================================================================

// Global variables for anti-ghosting
static unsigned long lastFullRefresh = 0;

void forceFullRefresh() {
    // Flash screen black then white to clear ghosting
    M5.Display.fillScreen(TFT_BLACK);
    delay(50);
    M5.Display.fillScreen(TFT_WHITE);
    delay(50);
    lastFullRefresh = millis();
}

void checkAndRefresh() {
    unsigned long now = millis();
    if (now - lastFullRefresh > GHOST_REFRESH_INTERVAL) {
        // Perform anti-ghosting refresh
        forceFullRefresh();
        
        // Redraw current screen based on state
        switch (currentState) {
            case STATE_MENU:
                drawMenu();
                break;
            case STATE_VIEW_SUMMARY:
                drawEbookPage();
                break;
            // Other states will be redrawn when next action occurs
            default:
                break;
        }
    }
}
