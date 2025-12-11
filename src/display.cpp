#include "display.h"
#include "storage.h"
#include "config_manager.h"
#include <M5Unified.h>
#include <WiFi.h>
#include <FS.h>
#include <SD.h>

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
        configSetFontSize(fontSize);  // Persist to NVS
    }
}

void EbookReader::decreaseFontSize() {
    if (fontSize > 0) {
        fontSize--;
        recalculateLayout();
        configSetFontSize(fontSize);  // Persist to NVS
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

// Touch feedback: draw button in pressed (inverted) state
void drawButtonPressed(const Button& btn) {
    // Fill with black background for pressed state
    M5.Display.fillRoundRect(btn.x, btn.y, btn.w, btn.h, MENU_BTN_RADIUS, TFT_BLACK);
    
    // Draw text in white (inverted)
    M5.Display.setTextColor(TFT_WHITE, TFT_BLACK);
    M5.Display.setFont(&fonts::lgfxJapanGothic_16);
    M5.Display.drawCenterString(btn.label, btn.x + btn.w/2, btn.y + btn.h/2 - 8);
    M5.Display.setFont(nullptr);
    
    // Partial update for quick feedback
    M5.Display.display();
}

// Show brief visual feedback then restore button
void showButtonFeedback(const Button& btn) {
    drawButtonPressed(btn);
    delay(100); // Brief flash
    drawButton(btn);
    M5.Display.display();
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
    M5.Display.display();
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
    M5.Display.display();
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

void drawSummariesForInfographic(const std::vector<SummaryFile>& summaries) {
    M5.Display.startWrite();
    M5.Display.fillScreen(TFT_WHITE);
    displayStatusBar();
    
    // Title
    M5.Display.setTextColor(TFT_BLACK, TFT_WHITE);
    M5.Display.setFont(&fonts::lgfxJapanGothic_20);
    M5.Display.drawCenterString("Create Infographic", SCREEN_WIDTH / 2, STATUS_BAR_HEIGHT + 15);
    M5.Display.setFont(&fonts::lgfxJapanGothic_16);
    M5.Display.setTextColor(TFT_DARKGREY, TFT_WHITE);
    M5.Display.drawString("Select a summary:", PADDING_MEDIUM, STATUS_BAR_HEIGHT + 45);
    
    // List items
    int startY = STATUS_BAR_HEIGHT + 70;
    int maxItems = (SCREEN_HEIGHT - startY - 80) / (LIST_ITEM_HEIGHT + PADDING_SMALL);
    
    if (summaries.empty()) {
        M5.Display.setTextColor(TFT_DARKGREY, TFT_WHITE);
        M5.Display.drawCenterString("(No summaries available)", SCREEN_WIDTH / 2, SCREEN_HEIGHT / 2);
    } else {
        for (size_t i = 0; i < summaries.size() && (int)i < maxItems; i++) {
            int y = startY + i * (LIST_ITEM_HEIGHT + PADDING_SMALL);
            M5.Display.drawRoundRect(PADDING_MEDIUM, y, SCREEN_WIDTH - 2 * PADDING_MEDIUM, LIST_ITEM_HEIGHT, LIST_ITEM_RADIUS, TFT_BLACK);
            M5.Display.setTextColor(TFT_BLACK, TFT_WHITE);
            M5.Display.drawString(summaries[i].filename, PADDING_LARGE, y + (LIST_ITEM_HEIGHT - 16) / 2);
        }
    }
    
    // Back button
    int backBtnY = SCREEN_HEIGHT - 70;
    M5.Display.fillRoundRect(PADDING_MEDIUM, backBtnY, NAV_BTN_WIDTH, NAV_BTN_HEIGHT, LIST_ITEM_RADIUS, TFT_LIGHTGREY);
    M5.Display.drawRoundRect(PADDING_MEDIUM, backBtnY, NAV_BTN_WIDTH, NAV_BTN_HEIGHT, LIST_ITEM_RADIUS, TFT_BLACK);
    M5.Display.setTextColor(TFT_BLACK, TFT_LIGHTGREY);
    M5.Display.drawCenterString("BACK", PADDING_MEDIUM + NAV_BTN_WIDTH / 2, backBtnY + 15);
    
    M5.Display.setFont(nullptr);
    M5.Display.endWrite();
    M5.Display.display();
}

void drawInfographicsOnSD(const std::vector<String>& infographics) {
    M5.Display.startWrite();
    M5.Display.fillScreen(TFT_WHITE);
    displayStatusBar();
    
    M5.Display.setTextColor(TFT_BLACK, TFT_WHITE);
    M5.Display.setFont(&fonts::lgfxJapanGothic_20);
    M5.Display.drawCenterString("Saved Infographics", SCREEN_WIDTH / 2, STATUS_BAR_HEIGHT + 15);
    
    if (infographics.empty()) {
        M5.Display.setFont(&fonts::lgfxJapanGothic_16);
        M5.Display.setTextColor(TFT_DARKGREY, TFT_WHITE);
        M5.Display.drawCenterString("No saved infographics", SCREEN_WIDTH / 2, SCREEN_HEIGHT / 2);
        
        // Back button
        int backBtnY = SCREEN_HEIGHT - 70;
        M5.Display.fillRoundRect(PADDING_MEDIUM, backBtnY, NAV_BTN_WIDTH, NAV_BTN_HEIGHT, LIST_ITEM_RADIUS, TFT_LIGHTGREY);
        M5.Display.drawRoundRect(PADDING_MEDIUM, backBtnY, NAV_BTN_WIDTH, NAV_BTN_HEIGHT, LIST_ITEM_RADIUS, TFT_BLACK);
        M5.Display.setTextColor(TFT_BLACK, TFT_LIGHTGREY);
        M5.Display.drawCenterString("BACK", PADDING_MEDIUM + NAV_BTN_WIDTH / 2, backBtnY + 15);
    } else {
        // Grid layout: 3 columns
        int gridStartY = STATUS_BAR_HEIGHT + 55;
        int cellWidth = (SCREEN_WIDTH - 4 * PADDING_SMALL) / 3;  // ~160px each
        int cellHeight = 180;  // Height including filename
        int thumbHeight = 140;
        int maxRows = (SCREEN_HEIGHT - gridStartY - 80) / cellHeight;  // Rows that fit on screen
        int maxItems = maxRows * 3;
        
        M5.Display.setFont(&fonts::lgfxJapanGothic_12);
        
        for (size_t i = 0; i < infographics.size() && (int)i < maxItems; i++) {
            int col = i % 3;
            int row = i / 3;
            int cellX = PADDING_SMALL + col * (cellWidth + PADDING_SMALL);
            int cellY = gridStartY + row * cellHeight;
            
            // Draw thumbnail placeholder (bordered rectangle with image icon)
            M5.Display.drawRect(cellX, cellY, cellWidth, thumbHeight, TFT_BLACK);
            M5.Display.drawRect(cellX + 1, cellY + 1, cellWidth - 2, thumbHeight - 2, TFT_DARKGREY);
            
            // Draw simple image icon in center
            int iconCenterX = cellX + cellWidth / 2;
            int iconCenterY = cellY + thumbHeight / 2;
            // Mountain-like icon
            M5.Display.drawLine(iconCenterX - 30, iconCenterY + 20, iconCenterX - 10, iconCenterY - 10, TFT_DARKGREY);
            M5.Display.drawLine(iconCenterX - 10, iconCenterY - 10, iconCenterX, iconCenterY + 5, TFT_DARKGREY);
            M5.Display.drawLine(iconCenterX, iconCenterY + 5, iconCenterX + 15, iconCenterY - 20, TFT_DARKGREY);
            M5.Display.drawLine(iconCenterX + 15, iconCenterY - 20, iconCenterX + 30, iconCenterY + 20, TFT_DARKGREY);
            
            // Draw filename below thumbnail (truncated if too long)
            String displayName = infographics[i];
            // Remove path prefix if present
            int lastSlash = displayName.lastIndexOf('/');
            if (lastSlash >= 0) displayName = displayName.substring(lastSlash + 1);
            // Truncate to fit cell width
            if (displayName.length() > 14) {
                displayName = displayName.substring(0, 11) + "...";
            }
            
            M5.Display.setTextColor(TFT_BLACK, TFT_WHITE);
            M5.Display.drawCenterString(displayName, cellX + cellWidth / 2, cellY + thumbHeight + 8);
            
            // Item number for selection
            M5.Display.setTextColor(TFT_WHITE, TFT_BLACK);
            M5.Display.fillRect(cellX + 2, cellY + 2, 20, 16, TFT_BLACK);
            M5.Display.drawCenterString(String(i + 1), cellX + 12, cellY + 4);
        }
        
        // Show scroll indicator if more items
        if ((int)infographics.size() > maxItems) {
            M5.Display.setFont(&fonts::lgfxJapanGothic_12);
            M5.Display.setTextColor(TFT_DARKGREY, TFT_WHITE);
            String moreText = "+" + String(infographics.size() - maxItems) + " more";
            M5.Display.drawCenterString(moreText, SCREEN_WIDTH / 2, SCREEN_HEIGHT - 90);
        }
        
        // Back button
        int backBtnY = SCREEN_HEIGHT - 70;
        M5.Display.fillRoundRect(PADDING_MEDIUM, backBtnY, NAV_BTN_WIDTH, NAV_BTN_HEIGHT, LIST_ITEM_RADIUS, TFT_LIGHTGREY);
        M5.Display.drawRoundRect(PADDING_MEDIUM, backBtnY, NAV_BTN_WIDTH, NAV_BTN_HEIGHT, LIST_ITEM_RADIUS, TFT_BLACK);
        M5.Display.setTextColor(TFT_BLACK, TFT_LIGHTGREY);
        M5.Display.setFont(&fonts::lgfxJapanGothic_16);
        M5.Display.drawCenterString("BACK", PADDING_MEDIUM + NAV_BTN_WIDTH / 2, backBtnY + 15);
    }
    
    M5.Display.setFont(nullptr);
    M5.Display.endWrite();
    M5.Display.display();
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
    Serial.printf("[DISPLAY] drawImageFromFile: label='%s', path='%s'\n", label.c_str(), path.c_str());
    
    // Set EPD mode for quality refresh (important for images on e-ink)
    // epd_quality gives the best image quality for photos
    M5.Display.setEpdMode(epd_mode_t::epd_quality);
    
    // Clear display first with anti-ghosting
    M5.Display.startWrite();
    M5.Display.fillScreen(TFT_WHITE);
    M5.Display.endWrite();
    M5.Display.display();
    M5.Display.waitDisplay();
    
    bool drawn = false;
    int rc = -1;
    
    // Check if file exists first
    if (!SD.exists(path)) {
        Serial.printf("[DISPLAY] File does NOT exist: %s\n", path.c_str());
        M5.Display.startWrite();
        M5.Display.fillScreen(TFT_WHITE);
        displayStatusBar();
        M5.Display.setFont(&fonts::lgfxJapanGothic_20);
        M5.Display.setTextColor(TFT_BLACK, TFT_WHITE);
        M5.Display.drawCenterString("File not found!", SCREEN_WIDTH/2, SCREEN_HEIGHT/2 - 20);
        M5.Display.drawCenterString(path, SCREEN_WIDTH/2, SCREEN_HEIGHT/2 + 20);
        
        // Draw BACK button
        int backBtnY = SCREEN_HEIGHT - 70;
        M5.Display.fillRoundRect(PADDING_MEDIUM, backBtnY, NAV_BTN_WIDTH, NAV_BTN_HEIGHT, LIST_ITEM_RADIUS, TFT_LIGHTGREY);
        M5.Display.drawRoundRect(PADDING_MEDIUM, backBtnY, NAV_BTN_WIDTH, NAV_BTN_HEIGHT, LIST_ITEM_RADIUS, TFT_BLACK);
        M5.Display.setTextColor(TFT_BLACK, TFT_LIGHTGREY);
        M5.Display.setFont(&fonts::lgfxJapanGothic_16);
        M5.Display.drawCenterString("BACK", PADDING_MEDIUM + NAV_BTN_WIDTH / 2, backBtnY + 15);
        M5.Display.setFont(nullptr);
        M5.Display.endWrite();
        M5.Display.display();
        M5.Display.waitDisplay();
        return;
    }
    
    // Get file info
    File imgFile = SD.open(path, FILE_READ);
    size_t fileSize = 0;
    if (imgFile) {
        fileSize = imgFile.size();
        imgFile.close();
        Serial.printf("[DISPLAY] File found, size: %lu bytes\n", fileSize);
    }
    
    // Calculate display area
    int displayAreaY = STATUS_BAR_HEIGHT;
    int displayAreaHeight = SCREEN_HEIGHT - STATUS_BAR_HEIGHT - 80;
    int displayAreaWidth = SCREEN_WIDTH;
    
    Serial.printf("[DISPLAY] Display area: %dx%d at y=%d\n", displayAreaWidth, displayAreaHeight, displayAreaY);
    
    // Start drawing
    M5.Display.startWrite();
    M5.Display.fillScreen(TFT_WHITE);
    displayStatusBar();
    
    // Draw the image using M5GFX
    // For e-ink, try drawing centered with proper scaling
    if (path.endsWith(".png")) {
        Serial.println("[DISPLAY] Attempting to draw PNG...");
        
        // Try with explicit maxWidth/maxHeight for scaling
        rc = M5.Display.drawPngFile(
            path.c_str(),
            0,                    // x - left aligned
            displayAreaY,         // y - below status bar
            displayAreaWidth,     // maxWidth
            displayAreaHeight,    // maxHeight
            0, 0,                 // offX, offY
            1.0f, 1.0f,           // scaleX, scaleY (auto-scale within max bounds)
            datum_t::top_left
        );
        
        Serial.printf("[DISPLAY] drawPngFile result: rc=%d\n", rc);
        drawn = (rc == 0);
        
    } else if (path.endsWith(".jpg") || path.endsWith(".jpeg")) {
        Serial.println("[DISPLAY] Attempting to draw JPEG...");
        
        rc = M5.Display.drawJpgFile(
            path.c_str(),
            0, displayAreaY,
            displayAreaWidth, displayAreaHeight,
            0, 0, 1.0f, 1.0f,
            datum_t::top_left
        );
        
        Serial.printf("[DISPLAY] drawJpgFile result: rc=%d\n", rc);
        drawn = (rc == 0);
    }

    if (drawn) {
        Serial.println("[DISPLAY] Image drawn successfully to framebuffer");
    } else {
        Serial.printf("[DISPLAY] Failed to draw image. RC=%d\n", rc);
        
        M5.Display.setFont(&fonts::lgfxJapanGothic_16);
        M5.Display.setTextColor(TFT_BLACK, TFT_WHITE);
        M5.Display.drawCenterString("Error loading image", SCREEN_WIDTH/2, SCREEN_HEIGHT/2 - 40);
        M5.Display.drawCenterString("File: " + path.substring(path.lastIndexOf('/') + 1), SCREEN_WIDTH/2, SCREEN_HEIGHT/2 - 10);
        M5.Display.drawCenterString("Size: " + String(fileSize) + " bytes", SCREEN_WIDTH/2, SCREEN_HEIGHT/2 + 20);
        M5.Display.drawCenterString("Error code: " + String(rc), SCREEN_WIDTH/2, SCREEN_HEIGHT/2 + 50);
        M5.Display.setFont(nullptr);
    }

    // Draw BACK button
    int backBtnY = SCREEN_HEIGHT - 70;
    M5.Display.fillRoundRect(PADDING_MEDIUM, backBtnY, NAV_BTN_WIDTH, NAV_BTN_HEIGHT, LIST_ITEM_RADIUS, TFT_LIGHTGREY);
    M5.Display.drawRoundRect(PADDING_MEDIUM, backBtnY, NAV_BTN_WIDTH, NAV_BTN_HEIGHT, LIST_ITEM_RADIUS, TFT_BLACK);
    M5.Display.setTextColor(TFT_BLACK, TFT_LIGHTGREY);
    M5.Display.setFont(&fonts::lgfxJapanGothic_16);
    M5.Display.drawCenterString("BACK", PADDING_MEDIUM + NAV_BTN_WIDTH / 2, backBtnY + 15);
    M5.Display.setFont(nullptr);

    M5.Display.endWrite();
    
    // Force full EPD refresh for e-ink display
    Serial.println("[DISPLAY] Triggering display refresh...");
    M5.Display.display();
    M5.Display.waitDisplay();
    
    // Reset EPD mode to faster mode for UI interactions
    M5.Display.setEpdMode(epd_mode_t::epd_fast);
    
    Serial.println("[DISPLAY] Display refresh complete");
}

void drawImageFromBuffer(const String& label, const uint8_t* data, size_t len) {
    M5.Display.startWrite();
    M5.Display.fillScreen(TFT_WHITE);
    displayStatusBar();
    
    Serial.printf("[DISPLAY] drawImageFromBuffer: label='%s', len=%d bytes\n", label.c_str(), len);
    
    M5.Display.setTextColor(TFT_BLACK, TFT_WHITE);
    M5.Display.setFont(&fonts::lgfxJapanGothic_16);
    M5.Display.drawCenterString(label, M5.Display.width()/2, STATUS_BAR_HEIGHT + 10);

    int displayAreaY = STATUS_BAR_HEIGHT + 30;

    // Attempt PNG first; fall back to JPG.
    // Use simple version without scaling to avoid M5GFX template issues
    bool drawn = false;
    int rc = M5.Display.drawPng(data, len, 0, displayAreaY);
    Serial.printf("[DISPLAY] drawPng(buffer) rc=%d\n", rc);
    
    if (rc != 0) {
        // Try JPG
        rc = M5.Display.drawJpg(data, len, 0, displayAreaY);
        Serial.printf("[DISPLAY] drawJpg(buffer) rc=%d\n", rc);
    }
    drawn = (rc == 0);

    if (!drawn) {
        M5.Display.setTextSize(1);
        M5.Display.setTextColor(TFT_BLACK, TFT_WHITE);
        M5.Display.drawCenterString("Unable to render image", M5.Display.width()/2, M5.Display.height()/2 - 20);
        M5.Display.drawCenterString("Size: " + String(len) + " bytes", M5.Display.width()/2, M5.Display.height()/2);
        M5.Display.drawCenterString("RC: " + String(rc), M5.Display.width()/2, M5.Display.height()/2 + 20);
        Serial.printf("[DISPLAY] Failed to render buffer image. RC=%d\n", rc);
    }

    // Draw BACK button
    int backBtnY = SCREEN_HEIGHT - 60;
    M5.Display.fillRoundRect(PADDING_MEDIUM, backBtnY, NAV_BTN_WIDTH, 45, LIST_ITEM_RADIUS, TFT_LIGHTGREY);
    M5.Display.drawRoundRect(PADDING_MEDIUM, backBtnY, NAV_BTN_WIDTH, 45, LIST_ITEM_RADIUS, TFT_BLACK);
    M5.Display.setTextColor(TFT_BLACK, TFT_LIGHTGREY);
    M5.Display.drawCenterString("BACK", PADDING_MEDIUM + NAV_BTN_WIDTH / 2, backBtnY + 14);
    
    M5.Display.setFont(nullptr);
    M5.Display.endWrite();
    M5.Display.display();
}

void drawProcessing(const String& message) {
    M5.Display.startWrite();
    M5.Display.fillScreen(TFT_WHITE);
    displayStatusBar();
    
    M5.Display.setTextColor(TFT_BLACK, TFT_WHITE);
    M5.Display.setFont(&fonts::lgfxJapanGothic_24);
    M5.Display.drawCenterString(message, M5.Display.width()/2, M5.Display.height()/2 - 30);
    
    M5.Display.setFont(&fonts::lgfxJapanGothic_16);
    M5.Display.setTextColor(TFT_DARKGREY, TFT_WHITE);
    M5.Display.drawCenterString("Please wait...", M5.Display.width()/2, M5.Display.height()/2 + 30);
    
    M5.Display.setFont(nullptr);
    M5.Display.endWrite();
    M5.Display.display();
}

void drawError(const String& message) {
    M5.Display.startWrite();
    M5.Display.fillScreen(TFT_WHITE);
    displayStatusBar();
    
    M5.Display.setTextColor(TFT_RED, TFT_WHITE);
    M5.Display.setFont(&fonts::lgfxJapanGothic_24);
    M5.Display.drawCenterString("ERROR", M5.Display.width()/2, STATUS_BAR_HEIGHT + 40);
    
    M5.Display.setFont(&fonts::lgfxJapanGothic_16);
    M5.Display.setTextColor(TFT_BLACK, TFT_WHITE);
    
    // Word wrap error message
    int maxWidth = SCREEN_WIDTH - 2 * PADDING_LARGE;
    int lineY = SCREEN_HEIGHT / 2 - 40;
    String remaining = message;
    while (remaining.length() > 0 && lineY < SCREEN_HEIGHT - 100) {
        int charCount = min((int)remaining.length(), 50);  // Max chars per line
        while (charCount > 0 && M5.Display.textWidth(remaining.substring(0, charCount)) > maxWidth) {
            charCount--;
        }
        if (charCount == 0) charCount = 1;
        M5.Display.drawCenterString(remaining.substring(0, charCount), SCREEN_WIDTH / 2, lineY);
        remaining = remaining.substring(charCount);
        remaining.trim();
        lineY += 25;
    }
    
    // Back button (consistent with touch handler)
    int backBtnY = SCREEN_HEIGHT - 70;
    M5.Display.fillRoundRect(PADDING_MEDIUM, backBtnY, NAV_BTN_WIDTH, NAV_BTN_HEIGHT, LIST_ITEM_RADIUS, TFT_LIGHTGREY);
    M5.Display.drawRoundRect(PADDING_MEDIUM, backBtnY, NAV_BTN_WIDTH, NAV_BTN_HEIGHT, LIST_ITEM_RADIUS, TFT_BLACK);
    M5.Display.setTextColor(TFT_BLACK, TFT_LIGHTGREY);
    M5.Display.drawCenterString("BACK", PADDING_MEDIUM + NAV_BTN_WIDTH / 2, backBtnY + 15);
    
    M5.Display.setFont(nullptr);
    M5.Display.endWrite();
    M5.Display.display();
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
    M5.Display.startWrite();
    M5.Display.fillScreen(TFT_WHITE);
    M5.Display.setTextColor(TFT_BLACK, TFT_WHITE);
    M5.Display.setFont(&fonts::lgfxJapanGothic_28);
    M5.Display.drawCenterString("MindInk", M5.Display.width()/2, M5.Display.height()/2 - 20);
    M5.Display.setFont(nullptr);
    M5.Display.endWrite();
    M5.Display.display();  // Force e-ink refresh
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
    
    // ==========================================================================
    // PROGRESS BAR
    // ==========================================================================
    int progressBarY = SCREEN_HEIGHT - 15;
    int progressBarWidth = SCREEN_WIDTH - 2 * PADDING_MEDIUM;
    int progressBarHeight = 8;
    
    // Calculate progress percentage
    float progress = (ebookReader.totalPages > 0) 
        ? (float)(ebookReader.currentPage + 1) / (float)ebookReader.totalPages 
        : 0.0f;
    int filledWidth = (int)(progressBarWidth * progress);
    
    // Draw progress bar background
    M5.Display.fillRect(PADDING_MEDIUM, progressBarY, progressBarWidth, progressBarHeight, TFT_LIGHTGREY);
    M5.Display.drawRect(PADDING_MEDIUM, progressBarY, progressBarWidth, progressBarHeight, TFT_BLACK);
    
    // Draw filled portion
    if (filledWidth > 0) {
        M5.Display.fillRect(PADDING_MEDIUM + 1, progressBarY + 1, filledWidth - 2, progressBarHeight - 2, TFT_BLACK);
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
    
    // ==========================================================================
    // TAP-TO-NAVIGATE ZONES (in reading area, not on buttons)
    // ==========================================================================
    // Reading area: from separator to button bar
    int readingAreaTop = STATUS_BAR_HEIGHT + 50;
    int readingAreaBottom = btnY - 10;
    
    if (y >= readingAreaTop && y < readingAreaBottom) {
        // Left 1/3 of screen: previous page
        if (x < SCREEN_WIDTH / 3 && ebookReader.hasPrevPage()) {
            ebookReader.prevPage();
            drawEbookPage();
            return true;
        }
        // Right 1/3 of screen: next page
        if (x > 2 * SCREEN_WIDTH / 3 && ebookReader.hasNextPage()) {
            ebookReader.nextPage();
            drawEbookPage();
            return true;
        }
        // Center area: do nothing (could add bookmark later)
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
    M5.Display.startWrite();
    // Full refresh before sleep (anti-ghosting)
    M5.Display.fillScreen(TFT_BLACK);
    M5.Display.display();  // Push black screen
    delay(200);
    M5.Display.fillScreen(TFT_WHITE);
    
    // Display "MindInk" centered
    M5.Display.setTextColor(TFT_BLACK, TFT_WHITE);
    M5.Display.setFont(&fonts::lgfxJapanGothic_28);
    M5.Display.drawCenterString("MindInk", SCREEN_WIDTH / 2, SCREEN_HEIGHT / 2 - 20);
    M5.Display.setFont(nullptr);
    M5.Display.endWrite();
    M5.Display.display();  // Force e-ink refresh to show message
}

// ============================================================================
// ANTI-GHOSTING & DISPLAY REFRESH OPTIMIZATION
// ============================================================================

// Global variables for anti-ghosting
static unsigned long lastFullRefresh = 0;
static int partialRefreshCount = 0;

// Force full refresh every N partial updates (prevents ghost buildup)
constexpr int PARTIAL_REFRESH_THRESHOLD = 5;

void forceFullRefresh() {
    Serial.println("[DISPLAY] Performing full refresh (anti-ghosting)");
    
    // Flash screen black then white to clear ghosting
    M5.Display.fillScreen(TFT_BLACK);
    M5.Display.display();
    delay(100);
    M5.Display.fillScreen(TFT_WHITE);
    M5.Display.display();
    delay(100);
    
    lastFullRefresh = millis();
    partialRefreshCount = 0;
}

void incrementPartialRefresh() {
    partialRefreshCount++;
    if (partialRefreshCount >= PARTIAL_REFRESH_THRESHOLD) {
        Serial.printf("[DISPLAY] Partial refresh count %d reached threshold\n", partialRefreshCount);
        forceFullRefresh();
    }
}

void checkAndRefresh() {
    unsigned long now = millis();
    
    // Time-based refresh (every 5 minutes)
    if (now - lastFullRefresh > GHOST_REFRESH_INTERVAL) {
        Serial.println("[DISPLAY] Time-based anti-ghosting refresh");
        forceFullRefresh();
        
        // Redraw current screen based on state
        switch (currentState) {
            case STATE_MENU:
                drawMenu();
                break;
            case STATE_VIEW_SUMMARY:
                drawEbookPage();
                break;
            case STATE_LIST_AUDIO:
            case STATE_LIST_SUMMARIES:
                // These will be redrawn on next user action
                break;
            case STATE_GALLERY_SAVED:
                // Will redraw on next action
                break;
            default:
                // Other states will be redrawn when next action occurs
                break;
        }
    }
}

// Request full refresh before next draw (useful for image display)
void requestFullRefresh() {
    partialRefreshCount = PARTIAL_REFRESH_THRESHOLD;
}

// ============================================================================
// DIAGNOSTICS SCREEN
// ============================================================================

void drawDiagnosticsScreen() {
    M5.Display.startWrite();
    M5.Display.fillScreen(TFT_WHITE);
    displayStatusBar();
    
    M5.Display.setTextColor(TFT_BLACK, TFT_WHITE);
    M5.Display.setFont(&fonts::lgfxJapanGothic_20);
    M5.Display.drawCenterString("System Diagnostics", SCREEN_WIDTH / 2, STATUS_BAR_HEIGHT + 20);
    
    M5.Display.setFont(&fonts::lgfxJapanGothic_16);
    int y = STATUS_BAR_HEIGHT + 70;
    int lineHeight = 35;
    
    // Memory info
    M5.Display.drawString("Heap Free:", PADDING_MEDIUM, y);
    M5.Display.drawRightString(String(ESP.getFreeHeap() / 1024) + " KB", SCREEN_WIDTH - PADDING_MEDIUM, y);
    y += lineHeight;
    
    M5.Display.drawString("Heap Total:", PADDING_MEDIUM, y);
    M5.Display.drawRightString(String(ESP.getHeapSize() / 1024) + " KB", SCREEN_WIDTH - PADDING_MEDIUM, y);
    y += lineHeight;
    
    M5.Display.drawString("PSRAM Free:", PADDING_MEDIUM, y);
    M5.Display.drawRightString(String(ESP.getFreePsram() / 1024) + " KB", SCREEN_WIDTH - PADDING_MEDIUM, y);
    y += lineHeight;
    
    M5.Display.drawString("PSRAM Total:", PADDING_MEDIUM, y);
    M5.Display.drawRightString(String(ESP.getPsramSize() / 1024) + " KB", SCREEN_WIDTH - PADDING_MEDIUM, y);
    y += lineHeight + 10;
    
    // Divider
    M5.Display.drawLine(PADDING_MEDIUM, y, SCREEN_WIDTH - PADDING_MEDIUM, y, TFT_BLACK);
    y += 15;
    
    // Power info
    M5.Display.drawString("Battery:", PADDING_MEDIUM, y);
    M5.Display.drawRightString(String(M5.Power.getBatteryLevel()) + "%", SCREEN_WIDTH - PADDING_MEDIUM, y);
    y += lineHeight;
    
    M5.Display.drawString("Charging:", PADDING_MEDIUM, y);
    M5.Display.drawRightString(M5.Power.isCharging() ? "Yes" : "No", SCREEN_WIDTH - PADDING_MEDIUM, y);
    y += lineHeight + 10;
    
    // Divider
    M5.Display.drawLine(PADDING_MEDIUM, y, SCREEN_WIDTH - PADDING_MEDIUM, y, TFT_BLACK);
    y += 15;
    
    // WiFi info
    M5.Display.drawString("WiFi Status:", PADDING_MEDIUM, y);
    M5.Display.drawRightString(WiFi.status() == WL_CONNECTED ? "Connected" : "Disconnected", SCREEN_WIDTH - PADDING_MEDIUM, y);
    y += lineHeight;
    
    if (WiFi.status() == WL_CONNECTED) {
        M5.Display.drawString("RSSI:", PADDING_MEDIUM, y);
        M5.Display.drawRightString(String(WiFi.RSSI()) + " dBm", SCREEN_WIDTH - PADDING_MEDIUM, y);
        y += lineHeight;
        
        M5.Display.drawString("IP:", PADDING_MEDIUM, y);
        M5.Display.drawRightString(WiFi.localIP().toString(), SCREEN_WIDTH - PADDING_MEDIUM, y);
        y += lineHeight;
    }
    
    y += 10;
    // Divider
    M5.Display.drawLine(PADDING_MEDIUM, y, SCREEN_WIDTH - PADDING_MEDIUM, y, TFT_BLACK);
    y += 15;
    
    // Storage info
    M5.Display.drawString("Storage:", PADDING_MEDIUM, y);
    String storageType = storage.activeStorage == STORAGE_SD ? "SD Card" : 
                         (storage.activeStorage == STORAGE_PSRAM ? "PSRAM" : "None");
    M5.Display.drawRightString(storageType, SCREEN_WIDTH - PADDING_MEDIUM, y);
    y += lineHeight;
    
    M5.Display.drawString("SD Available:", PADDING_MEDIUM, y);
    M5.Display.drawRightString(storage.sdAvailable ? "Yes" : "No", SCREEN_WIDTH - PADDING_MEDIUM, y);
    
    // Back button
    int backBtnY = SCREEN_HEIGHT - 70;
    M5.Display.fillRoundRect(PADDING_MEDIUM, backBtnY, NAV_BTN_WIDTH, NAV_BTN_HEIGHT, LIST_ITEM_RADIUS, TFT_LIGHTGREY);
    M5.Display.drawRoundRect(PADDING_MEDIUM, backBtnY, NAV_BTN_WIDTH, NAV_BTN_HEIGHT, LIST_ITEM_RADIUS, TFT_BLACK);
    M5.Display.setTextColor(TFT_BLACK, TFT_LIGHTGREY);
    M5.Display.drawCenterString("BACK", PADDING_MEDIUM + NAV_BTN_WIDTH / 2, backBtnY + 15);
    
    // Refresh button
    int refreshBtnX = SCREEN_WIDTH - PADDING_MEDIUM - NAV_BTN_WIDTH;
    M5.Display.fillRoundRect(refreshBtnX, backBtnY, NAV_BTN_WIDTH, NAV_BTN_HEIGHT, LIST_ITEM_RADIUS, TFT_DARKGREY);
    M5.Display.drawRoundRect(refreshBtnX, backBtnY, NAV_BTN_WIDTH, NAV_BTN_HEIGHT, LIST_ITEM_RADIUS, TFT_BLACK);
    M5.Display.setTextColor(TFT_WHITE, TFT_DARKGREY);
    M5.Display.drawCenterString("REFRESH", refreshBtnX + NAV_BTN_WIDTH / 2, backBtnY + 15);
    
    M5.Display.setFont(nullptr);
    M5.Display.endWrite();
    M5.Display.display();
}
