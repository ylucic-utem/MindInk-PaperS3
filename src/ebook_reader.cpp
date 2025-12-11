/**
 * @file ebook_reader.cpp
 * @brief Ebook/Text reader component for MindInk PaperS3
 * 
 * Extracted from display.cpp for modularity.
 */

#include "ebook_reader.h"
#include "display.h"
#include "display_refresh.h"
#include "config_manager.h"
#include <M5Unified.h>

// =============================================================================
// GLOBAL INSTANCE
// =============================================================================
EbookReader ebookReader;

// =============================================================================
// EBOOK READER IMPLEMENTATION
// =============================================================================

void EbookReader::setText(const String& text) {
    fullText = text;
    currentPage = 0;
    paginateText();
}

void EbookReader::paginateText() {
    pages.clear();
    
    if (fullText.length() == 0) {
        pages.push_back("(Empty)");
        totalPages = 1;
        return;
    }
    
    // Calculate layout based on font size
    recalculateLayout();
    
    // Smart word-wrapping pagination
    String currentPageContent = "";
    int currentLineCount = 0;
    
    int textIndex = 0;
    int textLen = fullText.length();
    
    while (textIndex < textLen) {
        // Build one line at a time
        String currentLine = "";
        int lineChars = 0;
        
        while (textIndex < textLen && lineChars < charsPerLine) {
            char c = fullText[textIndex];
            
            // Handle newlines
            if (c == '\n') {
                textIndex++;
                break;
            }
            
            // Handle carriage return (skip it)
            if (c == '\r') {
                textIndex++;
                continue;
            }
            
            currentLine += c;
            lineChars++;
            textIndex++;
        }
        
        // Word-wrap: if we're not at end and didn't hit newline, back up to last space
        if (textIndex < textLen && lineChars >= charsPerLine) {
            int lastSpace = currentLine.lastIndexOf(' ');
            if (lastSpace > charsPerLine / 2) {  // Only wrap if space is in second half
                // Back up textIndex
                int backUp = currentLine.length() - lastSpace - 1;
                textIndex -= backUp;
                currentLine = currentLine.substring(0, lastSpace + 1);
            }
        }
        
        // Add line to current page
        currentPageContent += currentLine;
        currentLineCount++;
        
        if (currentLineCount < linesPerPage && textIndex < textLen) {
            currentPageContent += "\n";
        }
        
        // Check if page is full
        if (currentLineCount >= linesPerPage) {
            pages.push_back(currentPageContent);
            currentPageContent = "";
            currentLineCount = 0;
        }
    }
    
    // Add remaining content as final page
    if (currentPageContent.length() > 0) {
        pages.push_back(currentPageContent);
    }
    
    totalPages = pages.size();
    if (totalPages == 0) {
        pages.push_back("(Empty)");
        totalPages = 1;
    }
    
    // Ensure currentPage is valid
    if (currentPage >= totalPages) {
        currentPage = totalPages - 1;
    }
    if (currentPage < 0) {
        currentPage = 0;
    }
    
    Serial.printf("[EBOOK] Paginated: %d pages, %d lines/page, %d chars/line\n", 
                  totalPages, linesPerPage, charsPerLine);
}

void EbookReader::nextPage() {
    if (hasNextPage()) {
        currentPage++;
    }
}

void EbookReader::prevPage() {
    if (hasPrevPage()) {
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
    if (currentPage >= 0 && currentPage < (int)pages.size()) {
        return pages[currentPage];
    }
    return "(No content)";
}

void EbookReader::increaseFontSize() {
    if (fontSize < 3) {
        fontSize++;
        paginateText();  // Re-paginate with new font size
    }
}

void EbookReader::decreaseFontSize() {
    if (fontSize > 0) {
        fontSize--;
        paginateText();  // Re-paginate with new font size
    }
}

void EbookReader::recalculateLayout() {
    // Font sizes: 0=16pt, 1=20pt, 2=24pt, 3=28pt
    // Screen: 540x960, content area after status bar and controls
    
    int contentHeight = SCREEN_HEIGHT - STATUS_BAR_HEIGHT - 120;  // Status bar + nav area
    int contentWidth = SCREEN_WIDTH - 2 * PADDING_MEDIUM;
    
    switch (fontSize) {
        case 0:  // 16pt - smallest, most text
            linesPerPage = contentHeight / 22;  // ~22px per line
            charsPerLine = contentWidth / 9;    // ~9px per char
            break;
        case 1:  // 20pt - default
            linesPerPage = contentHeight / 28;
            charsPerLine = contentWidth / 11;
            break;
        case 2:  // 24pt
            linesPerPage = contentHeight / 32;
            charsPerLine = contentWidth / 13;
            break;
        case 3:  // 28pt - largest, least text
            linesPerPage = contentHeight / 38;
            charsPerLine = contentWidth / 15;
            break;
        default:
            linesPerPage = 12;
            charsPerLine = 40;
    }
    
    // Ensure minimum values
    if (linesPerPage < 5) linesPerPage = 5;
    if (charsPerLine < 20) charsPerLine = 20;
}

// =============================================================================
// EBOOK PAGE DRAWING
// =============================================================================

void drawEbookPage() {
    M5.Display.fillScreen(TFT_WHITE);
    
    // Status bar
    displayStatusBar();
    
    // Content area
    int contentY = STATUS_BAR_HEIGHT + PADDING_MEDIUM;
    int contentWidth = SCREEN_WIDTH - 2 * PADDING_MEDIUM;
    int contentHeight = SCREEN_HEIGHT - STATUS_BAR_HEIGHT - 120;
    
    // Draw page content
    M5.Display.setTextColor(TFT_BLACK, TFT_WHITE);
    
    // Set font based on fontSize
    int fontHeight;
    switch (ebookReader.fontSize) {
        case 0:
            M5.Display.setTextSize(1);
            fontHeight = 16;
            break;
        case 1:
            M5.Display.setTextSize(1.25);
            fontHeight = 20;
            break;
        case 2:
            M5.Display.setTextSize(1.5);
            fontHeight = 24;
            break;
        case 3:
            M5.Display.setTextSize(1.75);
            fontHeight = 28;
            break;
        default:
            M5.Display.setTextSize(1.25);
            fontHeight = 20;
    }
    
    String pageText = ebookReader.getCurrentPageText();
    
    // Draw text line by line
    int lineY = contentY;
    int lineStart = 0;
    
    for (int i = 0; i <= (int)pageText.length(); i++) {
        if (i == (int)pageText.length() || pageText[i] == '\n') {
            String line = pageText.substring(lineStart, i);
            M5.Display.drawString(line, PADDING_MEDIUM, lineY);
            lineY += fontHeight + 4;
            lineStart = i + 1;
            
            if (lineY > contentY + contentHeight) break;
        }
    }
    
    // Navigation area
    int navY = SCREEN_HEIGHT - 100;
    
    // Page indicator
    M5.Display.setTextSize(1);
    String pageInfo = "Page " + String(ebookReader.currentPage + 1) + " / " + String(ebookReader.totalPages);
    M5.Display.setTextColor(TFT_BLACK);
    M5.Display.drawCentreString(pageInfo, SCREEN_WIDTH / 2, navY, 1);
    
    // Navigation buttons
    int btnY = SCREEN_HEIGHT - 70;
    int btnW = 100;
    int btnH = 50;
    int btnSpacing = 20;
    
    // BACK button (left)
    M5.Display.drawRoundRect(PADDING_MEDIUM, btnY, btnW, btnH, 8, TFT_BLACK);
    M5.Display.drawCentreString("BACK", PADDING_MEDIUM + btnW/2, btnY + 15, 1);
    
    // PREV button
    if (ebookReader.hasPrevPage()) {
        int prevX = PADDING_MEDIUM + btnW + btnSpacing;
        M5.Display.fillRoundRect(prevX, btnY, btnW, btnH, 8, TFT_BLACK);
        M5.Display.setTextColor(TFT_WHITE);
        M5.Display.drawCentreString("< PREV", prevX + btnW/2, btnY + 15, 1);
        M5.Display.setTextColor(TFT_BLACK);
    }
    
    // NEXT button
    if (ebookReader.hasNextPage()) {
        int nextX = SCREEN_WIDTH - PADDING_MEDIUM - btnW;
        M5.Display.fillRoundRect(nextX, btnY, btnW, btnH, 8, TFT_BLACK);
        M5.Display.setTextColor(TFT_WHITE);
        M5.Display.drawCentreString("NEXT >", nextX + btnW/2, btnY + 15, 1);
        M5.Display.setTextColor(TFT_BLACK);
    }
    
    // Font size buttons (A- and A+)
    int fontBtnW = 60;
    int fontBtnX = SCREEN_WIDTH / 2 - fontBtnW - 10;
    
    // A- button
    M5.Display.drawRoundRect(fontBtnX, btnY, fontBtnW, btnH, 8, TFT_BLACK);
    M5.Display.drawCentreString("A-", fontBtnX + fontBtnW/2, btnY + 15, 1);
    
    // A+ button
    M5.Display.drawRoundRect(fontBtnX + fontBtnW + 20, btnY, fontBtnW, btnH, 8, TFT_BLACK);
    M5.Display.drawCentreString("A+", fontBtnX + fontBtnW + 20 + fontBtnW/2, btnY + 15, 1);
    
    incrementPartialRefresh();
}

// =============================================================================
// EBOOK TOUCH HANDLING
// =============================================================================

bool handleEbookTouch(int x, int y) {
    int btnY = SCREEN_HEIGHT - 70;
    int btnW = 100;
    int btnH = 50;
    int btnSpacing = 20;
    
    // Check if touch is in button area
    if (y >= btnY && y <= btnY + btnH) {
        // BACK button
        if (x >= PADDING_MEDIUM && x <= PADDING_MEDIUM + btnW) {
            return false;  // Signal to go back
        }
        
        // PREV button
        int prevX = PADDING_MEDIUM + btnW + btnSpacing;
        if (x >= prevX && x <= prevX + btnW && ebookReader.hasPrevPage()) {
            ebookReader.prevPage();
            drawEbookPage();
            return true;
        }
        
        // NEXT button
        int nextX = SCREEN_WIDTH - PADDING_MEDIUM - btnW;
        if (x >= nextX && x <= nextX + btnW && ebookReader.hasNextPage()) {
            ebookReader.nextPage();
            drawEbookPage();
            return true;
        }
        
        // Font size buttons
        int fontBtnW = 60;
        int fontBtnX = SCREEN_WIDTH / 2 - fontBtnW - 10;
        
        // A- button
        if (x >= fontBtnX && x <= fontBtnX + fontBtnW) {
            ebookReader.decreaseFontSize();
            // Save preference
            appConfig.fontSize = ebookReader.fontSize;
            configSave();
            drawEbookPage();
            return true;
        }
        
        // A+ button
        if (x >= fontBtnX + fontBtnW + 20 && x <= fontBtnX + fontBtnW * 2 + 20) {
            ebookReader.increaseFontSize();
            // Save preference
            appConfig.fontSize = ebookReader.fontSize;
            configSave();
            drawEbookPage();
            return true;
        }
    }
    
    // Tap left half for prev, right half for next (gesture navigation)
    if (y < btnY - 50) {  // Above button area
        if (x < SCREEN_WIDTH / 2 && ebookReader.hasPrevPage()) {
            ebookReader.prevPage();
            drawEbookPage();
            return true;
        } else if (x >= SCREEN_WIDTH / 2 && ebookReader.hasNextPage()) {
            ebookReader.nextPage();
            drawEbookPage();
            return true;
        }
    }
    
    return true;  // Touch handled
}
