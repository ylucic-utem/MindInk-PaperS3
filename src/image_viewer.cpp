/**
 * @file image_viewer.cpp
 * @brief Image viewing and gallery functions for MindInk PaperS3
 * 
 * Extracted from display.cpp for modularity.
 * Uses M5GFX built-in image decoding with buffer-based approach.
 */

#include "image_viewer.h"
#include "display.h"
#include "display_refresh.h"
#include <M5Unified.h>
#include <SD.h>
#include <FS.h>

// =============================================================================
// HELPER: Load file to buffer and decode
// =============================================================================

static bool drawImageFromSDPath(const String& path, int x, int y, int maxW, int maxH) {
    File imgFile = SD.open(path, FILE_READ);
    if (!imgFile) {
        Serial.printf("[IMAGE] Cannot open: %s\n", path.c_str());
        return false;
    }
    
    size_t fileSize = imgFile.size();
    if (fileSize == 0 || fileSize > 2000000) {  // Max 2MB
        imgFile.close();
        Serial.printf("[IMAGE] File too large or empty: %d bytes\n", fileSize);
        return false;
    }
    
    uint8_t* buffer = (uint8_t*)malloc(fileSize);
    if (!buffer) {
        imgFile.close();
        Serial.println("[IMAGE] Failed to allocate buffer");
        return false;
    }
    
    size_t bytesRead = imgFile.read(buffer, fileSize);
    imgFile.close();
    
    if (bytesRead != fileSize) {
        free(buffer);
        Serial.println("[IMAGE] Failed to read file");
        return false;
    }
    
    bool success = false;
    
    // Try PNG first, then JPEG
    if (path.endsWith(".png") || path.endsWith(".PNG")) {
        success = M5.Display.drawPng(buffer, fileSize, x, y, maxW, maxH);
    } else if (path.endsWith(".jpg") || path.endsWith(".JPG") ||
               path.endsWith(".jpeg") || path.endsWith(".JPEG")) {
        success = M5.Display.drawJpg(buffer, fileSize, x, y, maxW, maxH);
    } else {
        // Try both
        success = M5.Display.drawPng(buffer, fileSize, x, y, maxW, maxH);
        if (!success) {
            success = M5.Display.drawJpg(buffer, fileSize, x, y, maxW, maxH);
        }
    }
    
    free(buffer);
    return success;
}

// =============================================================================
// GALLERY DISPLAY
// =============================================================================

void drawGallery(const std::vector<ImageFile>& images) {
    M5.Display.fillScreen(TFT_WHITE);
    displayStatusBar();
    
    M5.Display.setTextSize(1.5);
    M5.Display.setTextColor(TFT_BLACK);
    M5.Display.drawString("Gallery", PADDING_MEDIUM, STATUS_BAR_HEIGHT + PADDING_MEDIUM);
    
    if (images.empty()) {
        M5.Display.setTextSize(1);
        M5.Display.drawCentreString("(No images found)", SCREEN_WIDTH / 2, SCREEN_HEIGHT / 2);
    } else {
        // Grid layout
        int gridStartY = STATUS_BAR_HEIGHT + 55;
        int cellWidth = (SCREEN_WIDTH - 4 * PADDING_SMALL) / 3;
        int cellHeight = 150;
        
        for (size_t i = 0; i < images.size() && i < 12; i++) {
            int col = i % 3;
            int row = i / 3;
            int cellX = PADDING_SMALL + col * (cellWidth + PADDING_SMALL);
            int cellY = gridStartY + row * cellHeight;
            
            // Draw cell placeholder
            M5.Display.drawRoundRect(cellX, cellY, cellWidth, cellHeight - 10, 8, TFT_BLACK);
            M5.Display.setTextSize(0.8);
            
            String label = images[i].filename;
            if (label.length() > 15) {
                label = label.substring(0, 12) + "...";
            }
            M5.Display.drawCentreString(label, cellX + cellWidth/2, cellY + cellHeight - 25);
        }
    }
    
    // Back button
    int backBtnY = SCREEN_HEIGHT - 70;
    M5.Display.drawRoundRect(PADDING_MEDIUM, backBtnY, NAV_BTN_WIDTH, NAV_BTN_HEIGHT, 8, TFT_BLACK);
    M5.Display.setTextSize(1);
    M5.Display.drawCentreString("BACK", PADDING_MEDIUM + NAV_BTN_WIDTH/2, backBtnY + 15);
    
    incrementPartialRefresh();
}

// =============================================================================
// SUMMARIES FOR INFOGRAPHIC
// =============================================================================

void drawSummariesForInfographic(const std::vector<SummaryFile>& summaries) {
    M5.Display.fillScreen(TFT_WHITE);
    displayStatusBar();
    
    M5.Display.setTextSize(1.25);
    M5.Display.setTextColor(TFT_BLACK);
    M5.Display.drawString("Select Summary for Infographic", PADDING_MEDIUM, STATUS_BAR_HEIGHT + PADDING_MEDIUM);
    
    // List items
    int startY = STATUS_BAR_HEIGHT + 70;
    M5.Display.setTextSize(1);
    
    for (size_t i = 0; i < summaries.size() && i < 8; i++) {
        int y = startY + i * (LIST_ITEM_HEIGHT + PADDING_SMALL);
        
        // Item background
        M5.Display.drawRoundRect(PADDING_MEDIUM, y, SCREEN_WIDTH - 2 * PADDING_MEDIUM, LIST_ITEM_HEIGHT, LIST_ITEM_RADIUS, TFT_BLACK);
        
        // Item text
        String label = summaries[i].filename;
        if (label.length() > 35) {
            label = label.substring(0, 32) + "...";
        }
        M5.Display.drawString(label, PADDING_MEDIUM + 15, y + LIST_ITEM_HEIGHT/2 - 10);
        
        // Status indicator if available
        if (summaries[i].status.length() > 0) {
            M5.Display.setTextColor(TFT_DARKGREY);
            M5.Display.drawString(summaries[i].status, PADDING_MEDIUM + 15, y + LIST_ITEM_HEIGHT/2 + 10);
            M5.Display.setTextColor(TFT_BLACK);
        }
    }
    
    // Back button
    int backBtnY = SCREEN_HEIGHT - 70;
    M5.Display.drawRoundRect(PADDING_MEDIUM, backBtnY, NAV_BTN_WIDTH, NAV_BTN_HEIGHT, 8, TFT_BLACK);
    M5.Display.drawCentreString("BACK", PADDING_MEDIUM + NAV_BTN_WIDTH/2, backBtnY + 15);
    
    incrementPartialRefresh();
}

// =============================================================================
// INFOGRAPHICS ON SD CARD (GRID VIEW)
// =============================================================================

void drawInfographicsOnSD(const std::vector<String>& infographics) {
    M5.Display.fillScreen(TFT_WHITE);
    displayStatusBar();
    
    M5.Display.setTextSize(1.25);
    M5.Display.setTextColor(TFT_BLACK);
    M5.Display.drawString("Saved Infographics", PADDING_MEDIUM, STATUS_BAR_HEIGHT + PADDING_MEDIUM);
    
    if (infographics.empty()) {
        M5.Display.setTextSize(1);
        M5.Display.drawCentreString("(No infographics saved)", SCREEN_WIDTH / 2, SCREEN_HEIGHT / 2);
    } else {
        // Grid layout - 3 columns
        int gridStartY = STATUS_BAR_HEIGHT + 55;
        int cellWidth = (SCREEN_WIDTH - 4 * PADDING_SMALL) / 3;
        int cellHeight = 180;
        int thumbHeight = 130;
        int maxRows = (SCREEN_HEIGHT - gridStartY - 80) / cellHeight;
        int maxItems = maxRows * 3;
        
        for (size_t i = 0; i < infographics.size() && (int)i < maxItems; i++) {
            int col = i % 3;
            int row = i / 3;
            int cellX = PADDING_SMALL + col * (cellWidth + PADDING_SMALL);
            int cellY = gridStartY + row * cellHeight;
            
            // Draw cell border
            M5.Display.drawRoundRect(cellX, cellY, cellWidth, cellHeight - 10, 8, TFT_BLACK);
            
            // Calculate thumbnail area within cell
            int thumbX = cellX + 5;
            int thumbY = cellY + 5;
            int thumbW = cellWidth - 10;
            
            // Try to draw thumbnail from SD card
            String filepath = "/infographics/" + infographics[i];
            bool drawn = drawImageFromSDPath(filepath, thumbX, thumbY, thumbW, thumbHeight);
            
            if (!drawn) {
                // Fallback: draw placeholder box
                M5.Display.drawRect(thumbX, thumbY, thumbW, thumbHeight, TFT_LIGHTGREY);
                M5.Display.drawCentreString("?", thumbX + thumbW/2, thumbY + thumbHeight/2 - 10);
            }
            
            // Filename label at bottom of cell
            M5.Display.setTextSize(0.75);
            String label = infographics[i];
            if (label.length() > 18) {
                label = label.substring(0, 15) + "...";
            }
            M5.Display.drawCentreString(label, cellX + cellWidth/2, cellY + cellHeight - 25);
            M5.Display.setTextSize(1);
        }
    }
    
    // Back button
    int backBtnY = SCREEN_HEIGHT - 70;
    M5.Display.drawRoundRect(PADDING_MEDIUM, backBtnY, NAV_BTN_WIDTH, NAV_BTN_HEIGHT, 8, TFT_BLACK);
    M5.Display.drawCentreString("BACK", PADDING_MEDIUM + NAV_BTN_WIDTH/2, backBtnY + 15);
    
    incrementPartialRefresh();
}

// =============================================================================
// IMAGE MESSAGE
// =============================================================================

void drawImageMessage(const String& title, const String& message) {
    M5.Display.fillScreen(TFT_WHITE);
    displayStatusBar();
    
    M5.Display.setTextSize(1.5);
    M5.Display.setTextColor(TFT_BLACK);
    M5.Display.drawCentreString(title, SCREEN_WIDTH / 2, SCREEN_HEIGHT / 2 - 40);
    
    M5.Display.setTextSize(1);
    M5.Display.drawCentreString(message, SCREEN_WIDTH / 2, SCREEN_HEIGHT / 2 + 20);
    
    incrementPartialRefresh();
}

// =============================================================================
// DRAW IMAGE FROM FILE
// =============================================================================

void drawImageFromFile(const String& label, const String& path) {
    M5.Display.fillScreen(TFT_WHITE);
    
    // Force full refresh for images
    forceFullRefresh();
    
    Serial.printf("[IMAGE] Loading from file: %s\n", path.c_str());
    
    // Check if file exists
    if (!SD.exists(path)) {
        Serial.printf("[IMAGE] File not found: %s\n", path.c_str());
        displayStatusBar();
        M5.Display.setTextSize(1.5);
        M5.Display.setTextColor(TFT_BLACK);
        M5.Display.drawCentreString("File not found", SCREEN_WIDTH / 2, SCREEN_HEIGHT / 2);
        
        // Back button
        int backBtnY = SCREEN_HEIGHT - 70;
        M5.Display.drawRoundRect(PADDING_MEDIUM, backBtnY, NAV_BTN_WIDTH, NAV_BTN_HEIGHT, 8, TFT_BLACK);
        M5.Display.drawCentreString("BACK", PADDING_MEDIUM + NAV_BTN_WIDTH/2, backBtnY + 15);
        return;
    }
    
    // Image display area
    int imgY = STATUS_BAR_HEIGHT + 40;
    int imgH = SCREEN_HEIGHT - STATUS_BAR_HEIGHT - 120;
    
    // Draw image using buffer-based helper
    bool success = drawImageFromSDPath(path, 0, imgY, SCREEN_WIDTH, imgH);
    
    if (!success) {
        Serial.printf("[IMAGE] Failed to decode: %s\n", path.c_str());
        M5.Display.fillScreen(TFT_WHITE);
        displayStatusBar();
        M5.Display.setTextSize(1.5);
        M5.Display.setTextColor(TFT_BLACK);
        M5.Display.drawCentreString("Failed to decode image", SCREEN_WIDTH / 2, SCREEN_HEIGHT / 2);
    } else {
        Serial.println("[IMAGE] Decode success");
        // Draw status bar and label on top
        displayStatusBar();
        
        // Label at top
        if (label.length() > 0) {
            M5.Display.fillRect(0, STATUS_BAR_HEIGHT, SCREEN_WIDTH, 30, TFT_WHITE);
            M5.Display.setTextSize(1);
            M5.Display.setTextColor(TFT_BLACK);
            M5.Display.drawCentreString(label, SCREEN_WIDTH / 2, STATUS_BAR_HEIGHT + 8);
        }
    }
    
    // Back button
    int backBtnY = SCREEN_HEIGHT - 70;
    M5.Display.fillRect(PADDING_MEDIUM - 5, backBtnY - 5, NAV_BTN_WIDTH + 10, NAV_BTN_HEIGHT + 10, TFT_WHITE);
    M5.Display.drawRoundRect(PADDING_MEDIUM, backBtnY, NAV_BTN_WIDTH, NAV_BTN_HEIGHT, 8, TFT_BLACK);
    M5.Display.setTextSize(1);
    M5.Display.setTextColor(TFT_BLACK);
    M5.Display.drawCentreString("BACK", PADDING_MEDIUM + NAV_BTN_WIDTH/2, backBtnY + 15);
}

// =============================================================================
// DRAW IMAGE FROM BUFFER
// =============================================================================

void drawImageFromBuffer(const String& label, const uint8_t* data, size_t len) {
    M5.Display.fillScreen(TFT_WHITE);
    forceFullRefresh();
    
    displayStatusBar();
    
    if (!data || len == 0) {
        M5.Display.setTextSize(1.5);
        M5.Display.setTextColor(TFT_BLACK);
        M5.Display.drawCentreString("No image data", SCREEN_WIDTH / 2, SCREEN_HEIGHT / 2);
        
        int backBtnY = SCREEN_HEIGHT - 70;
        M5.Display.drawRoundRect(PADDING_MEDIUM, backBtnY, NAV_BTN_WIDTH, NAV_BTN_HEIGHT, 8, TFT_BLACK);
        M5.Display.drawCentreString("BACK", PADDING_MEDIUM + NAV_BTN_WIDTH/2, backBtnY + 15);
        return;
    }
    
    int imgY = STATUS_BAR_HEIGHT + 40;
    int imgH = SCREEN_HEIGHT - STATUS_BAR_HEIGHT - 120;
    
    // Try PNG first, then JPEG using M5GFX built-in decoders
    bool success = M5.Display.drawPng(data, len, 0, imgY, SCREEN_WIDTH, imgH);
    if (!success) {
        success = M5.Display.drawJpg(data, len, 0, imgY, SCREEN_WIDTH, imgH);
    }
    
    if (!success) {
        M5.Display.setTextSize(1.5);
        M5.Display.setTextColor(TFT_BLACK);
        M5.Display.drawCentreString("Failed to decode image", SCREEN_WIDTH / 2, SCREEN_HEIGHT / 2);
    }
    
    // Label
    if (label.length() > 0) {
        M5.Display.setTextSize(1);
        M5.Display.setTextColor(TFT_BLACK);
        M5.Display.drawCentreString(label, SCREEN_WIDTH / 2, STATUS_BAR_HEIGHT + 15);
    }
    
    // Back button
    int backBtnY = SCREEN_HEIGHT - 70;
    M5.Display.drawRoundRect(PADDING_MEDIUM, backBtnY, NAV_BTN_WIDTH, NAV_BTN_HEIGHT, 8, TFT_BLACK);
    M5.Display.drawCentreString("BACK", PADDING_MEDIUM + NAV_BTN_WIDTH/2, backBtnY + 15);
}
