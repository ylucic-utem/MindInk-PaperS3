/**
 * @file display.cpp
 * @brief Core display functions for MindInk PaperS3
 * 
 * This is the refactored display module containing only:
 * - Button setup and handling
 * - Status bar drawing
 * - Menu drawing
 * - List drawing
 * - Message/Error/Processing screens
 * - Power screens
 * 
 * Image viewing, ebook reader, and refresh logic are in separate modules.
 */

#include "display.h"
#include "display_refresh.h"
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

// ============================================================================
// UI SETUP
// ============================================================================

void setupButtons() {
    menuButtons.clear();
    
    // Calculate button positions for 2x3 grid layout
    int startY = STATUS_BAR_HEIGHT + 50;
    int btnSpacing = 30;
    int col1X = (SCREEN_WIDTH / 2) - MENU_BTN_WIDTH - (btnSpacing / 2);
    int col2X = (SCREEN_WIDTH / 2) + (btnSpacing / 2);
    
    // Row 1: Audio Files, Summaries
    menuButtons.push_back(Button(col1X, startY, MENU_BTN_WIDTH, MENU_BTN_HEIGHT, "AUDIO"));
    menuButtons.push_back(Button(col2X, startY, MENU_BTN_WIDTH, MENU_BTN_HEIGHT, "SUMMARIES"));
    
    // Row 2: Gallery, Power Off
    int row2Y = startY + MENU_BTN_HEIGHT + btnSpacing;
    menuButtons.push_back(Button(col1X, row2Y, MENU_BTN_WIDTH, MENU_BTN_HEIGHT, "GALLERY"));
    menuButtons.push_back(Button(col2X, row2Y, MENU_BTN_WIDTH, MENU_BTN_HEIGHT, "POWER OFF"));
    
    // Row 3: Deep Sleep (centered)
    int row3Y = row2Y + MENU_BTN_HEIGHT + btnSpacing;
    int sleepBtnX = (SCREEN_WIDTH - MENU_BTN_WIDTH) / 2;
    menuButtons.push_back(Button(sleepBtnX, row3Y, MENU_BTN_WIDTH, MENU_BTN_HEIGHT, "SLEEP"));
}

bool checkButtonPress(int x, int y, const Button& btn) {
    return (x >= btn.x && x <= btn.x + btn.w && y >= btn.y && y <= btn.y + btn.h);
}

void drawButton(const Button& btn) {
    M5.Display.drawRoundRect(btn.x, btn.y, btn.w, btn.h, MENU_BTN_RADIUS, TFT_BLACK);
    M5.Display.setTextColor(TFT_BLACK);
    M5.Display.setTextSize(1.25);
    M5.Display.drawCentreString(btn.label, btn.x + btn.w / 2, btn.y + btn.h / 2 - 10, 1);
}

void drawButtonPressed(const Button& btn) {
    // Draw inverted button for press feedback
    M5.Display.fillRoundRect(btn.x, btn.y, btn.w, btn.h, MENU_BTN_RADIUS, TFT_BLACK);
    M5.Display.setTextColor(TFT_WHITE);
    M5.Display.setTextSize(1.25);
    M5.Display.drawCentreString(btn.label, btn.x + btn.w / 2, btn.y + btn.h / 2 - 10, 1);
}

void showButtonFeedback(const Button& btn) {
    drawButtonPressed(btn);
    delay(100);
    drawButton(btn);
}

// ============================================================================
// STATUS BAR
// ============================================================================

void displayStatusBar() {
    // Status bar background
    M5.Display.fillRect(0, 0, SCREEN_WIDTH, STATUS_BAR_HEIGHT, TFT_WHITE);
    M5.Display.drawLine(0, STATUS_BAR_HEIGHT - 1, SCREEN_WIDTH, STATUS_BAR_HEIGHT - 1, TFT_BLACK);
    
    M5.Display.setTextColor(TFT_BLACK);
    M5.Display.setTextSize(1);
    
    int iconY = 15;
    int iconSpacing = 10;
    int x = PADDING_SMALL;
    
    // Battery icon and percentage
    int batteryLevel = M5.Power.getBatteryLevel();
    bool isCharging = M5.Power.isCharging();
    
    // Draw battery outline
    M5.Display.drawRect(x, iconY, 30, 16, TFT_BLACK);
    M5.Display.fillRect(x + 30, iconY + 4, 3, 8, TFT_BLACK);  // Battery tip
    
    // Battery fill based on level
    int fillWidth = (batteryLevel * 26) / 100;
    if (fillWidth < 2) fillWidth = 2;
    M5.Display.fillRect(x + 2, iconY + 2, fillWidth, 12, TFT_BLACK);
    
    // Percentage text
    x += 40;
    String battStr = String(batteryLevel) + "%";
    if (isCharging) battStr += "+";
    M5.Display.drawString(battStr, x, iconY);
    
    // WiFi indicator
    x = SCREEN_WIDTH / 2 - 40;
    bool wifiConnected = WiFi.status() == WL_CONNECTED;
    
    if (wifiConnected) {
        int rssi = WiFi.RSSI();
        int bars = 0;
        if (rssi >= -50) bars = 4;
        else if (rssi >= -60) bars = 3;
        else if (rssi >= -70) bars = 2;
        else bars = 1;
        
        // Draw WiFi bars
        for (int i = 0; i < 4; i++) {
            int barH = 4 + i * 3;
            int barY = iconY + 16 - barH;
            if (i < bars) {
                M5.Display.fillRect(x + i * 7, barY, 5, barH, TFT_BLACK);
            } else {
                M5.Display.drawRect(x + i * 7, barY, 5, barH, TFT_DARKGREY);
            }
        }
    } else {
        M5.Display.drawString("No WiFi", x, iconY);
    }
    
    // SD card indicator
    x = SCREEN_WIDTH - PADDING_SMALL - 70;
    if (storage.activeStorage == STORAGE_SD) {
        M5.Display.drawString("SD OK", x, iconY);
    } else if (storage.activeStorage == STORAGE_PSRAM) {
        M5.Display.drawString("PSRAM", x, iconY);
    } else {
        M5.Display.drawString("No SD", x, iconY);
    }
}

// ============================================================================
// MENU DRAWING
// ============================================================================

void drawMenu() {
    M5.Display.fillScreen(TFT_WHITE);
    displayStatusBar();
    
    // Title
    M5.Display.setTextSize(2);
    M5.Display.setTextColor(TFT_BLACK);
    M5.Display.drawCentreString("MindInk", SCREEN_WIDTH / 2, STATUS_BAR_HEIGHT + PADDING_MEDIUM, 1);
    
    // Draw all menu buttons
    for (const auto& btn : menuButtons) {
        drawButton(btn);
    }
    
    // Footer with version
    M5.Display.setTextSize(0.75);
    M5.Display.setTextColor(TFT_DARKGREY);
    M5.Display.drawCentreString("Cloud Remote & Viewer v1.0", SCREEN_WIDTH / 2, SCREEN_HEIGHT - 30, 1);
    
    incrementPartialRefresh();
}

// ============================================================================
// LIST DRAWING
// ============================================================================

void drawList(String title, const std::vector<String>& items) {
    M5.Display.fillScreen(TFT_WHITE);
    displayStatusBar();
    
    // Title
    M5.Display.setTextSize(1.25);
    M5.Display.setTextColor(TFT_BLACK);
    M5.Display.drawString(title, PADDING_MEDIUM, STATUS_BAR_HEIGHT + PADDING_MEDIUM);
    
    // List items
    int startY = STATUS_BAR_HEIGHT + 70;
    M5.Display.setTextSize(1);
    
    int maxItems = (SCREEN_HEIGHT - startY - 80) / (LIST_ITEM_HEIGHT + PADDING_SMALL);
    
    for (size_t i = 0; i < items.size() && (int)i < maxItems; i++) {
        int y = startY + i * (LIST_ITEM_HEIGHT + PADDING_SMALL);
        
        // Item background
        M5.Display.drawRoundRect(PADDING_MEDIUM, y, SCREEN_WIDTH - 2 * PADDING_MEDIUM, 
                                 LIST_ITEM_HEIGHT, LIST_ITEM_RADIUS, TFT_BLACK);
        
        // Item text (truncate if needed)
        String label = items[i];
        if (label.length() > 40) {
            label = label.substring(0, 37) + "...";
        }
        M5.Display.drawString(label, PADDING_MEDIUM + 15, y + LIST_ITEM_HEIGHT / 2 - 8);
    }
    
    // Back button
    int backBtnY = SCREEN_HEIGHT - 70;
    M5.Display.drawRoundRect(PADDING_MEDIUM, backBtnY, NAV_BTN_WIDTH, NAV_BTN_HEIGHT, 8, TFT_BLACK);
    M5.Display.drawCentreString("BACK", PADDING_MEDIUM + NAV_BTN_WIDTH / 2, backBtnY + 15, 1);
    
    incrementPartialRefresh();
}

// ============================================================================
// TEXT VIEW (simple, for short content)
// ============================================================================

void drawTextView(String text) {
    M5.Display.fillScreen(TFT_WHITE);
    displayStatusBar();
    
    M5.Display.setTextSize(1);
    M5.Display.setTextColor(TFT_BLACK);
    
    // Simple word-wrapped text display
    int y = STATUS_BAR_HEIGHT + PADDING_MEDIUM;
    int lineHeight = 25;
    int maxWidth = SCREEN_WIDTH - 2 * PADDING_MEDIUM;
    
    int start = 0;
    while (start < (int)text.length() && y < SCREEN_HEIGHT - 80) {
        int end = start + 50;  // Approximate chars per line
        if (end > (int)text.length()) end = text.length();
        
        // Find last space if not at end
        if (end < (int)text.length()) {
            int lastSpace = text.lastIndexOf(' ', end);
            if (lastSpace > start + 10) {
                end = lastSpace;
            }
        }
        
        String line = text.substring(start, end);
        M5.Display.drawString(line, PADDING_MEDIUM, y);
        y += lineHeight;
        start = end + 1;
    }
    
    // Back button
    int backBtnY = SCREEN_HEIGHT - 70;
    M5.Display.drawRoundRect(PADDING_MEDIUM, backBtnY, NAV_BTN_WIDTH, NAV_BTN_HEIGHT, 8, TFT_BLACK);
    M5.Display.drawCentreString("BACK", PADDING_MEDIUM + NAV_BTN_WIDTH / 2, backBtnY + 15, 1);
    
    incrementPartialRefresh();
}

// ============================================================================
// PROCESSING SCREEN
// ============================================================================

void drawProcessing(const String& message) {
    M5.Display.fillScreen(TFT_WHITE);
    displayStatusBar();
    
    // Centered processing message
    M5.Display.setTextSize(1.5);
    M5.Display.setTextColor(TFT_BLACK);
    M5.Display.drawCentreString(message, SCREEN_WIDTH / 2, SCREEN_HEIGHT / 2 - 20, 1);
    
    // Simple loading indicator
    M5.Display.setTextSize(2);
    M5.Display.drawCentreString("...", SCREEN_WIDTH / 2, SCREEN_HEIGHT / 2 + 30, 1);
    
    incrementPartialRefresh();
}

// ============================================================================
// ERROR SCREEN
// ============================================================================

void drawError(const String& message) {
    M5.Display.fillScreen(TFT_WHITE);
    displayStatusBar();
    
    // Error icon (simple X)
    int iconX = SCREEN_WIDTH / 2;
    int iconY = SCREEN_HEIGHT / 2 - 80;
    int iconSize = 40;
    M5.Display.drawLine(iconX - iconSize, iconY - iconSize, iconX + iconSize, iconY + iconSize, TFT_BLACK);
    M5.Display.drawLine(iconX + iconSize, iconY - iconSize, iconX - iconSize, iconY + iconSize, TFT_BLACK);
    
    // Error title
    M5.Display.setTextSize(1.5);
    M5.Display.setTextColor(TFT_BLACK);
    M5.Display.drawCentreString("Error", SCREEN_WIDTH / 2, SCREEN_HEIGHT / 2 - 20, 1);
    
    // Error message (word wrap if long)
    M5.Display.setTextSize(1);
    
    if (message.length() <= 45) {
        M5.Display.drawCentreString(message, SCREEN_WIDTH / 2, SCREEN_HEIGHT / 2 + 20, 1);
    } else {
        // Split into lines
        int midPoint = message.lastIndexOf(' ', 45);
        if (midPoint < 20) midPoint = 45;
        
        String line1 = message.substring(0, midPoint);
        String line2 = message.substring(midPoint + 1);
        
        M5.Display.drawCentreString(line1, SCREEN_WIDTH / 2, SCREEN_HEIGHT / 2 + 15, 1);
        M5.Display.drawCentreString(line2, SCREEN_WIDTH / 2, SCREEN_HEIGHT / 2 + 40, 1);
    }
    
    // Back button
    int backBtnY = SCREEN_HEIGHT - 70;
    M5.Display.drawRoundRect(PADDING_MEDIUM, backBtnY, NAV_BTN_WIDTH, NAV_BTN_HEIGHT, 8, TFT_BLACK);
    M5.Display.drawCentreString("BACK", PADDING_MEDIUM + NAV_BTN_WIDTH / 2, backBtnY + 15, 1);
    
    incrementPartialRefresh();
}

// ============================================================================
// MESSAGE DISPLAY (for confirmation dialogs)
// ============================================================================

void drawMessage(const String& title, const String& msg) {
    M5.Display.fillScreen(TFT_WHITE);
    displayStatusBar();
    
    int boxY = SCREEN_HEIGHT / 2 - 100;
    int boxH = 200;
    
    // Message box border
    M5.Display.drawRoundRect(PADDING_LARGE, boxY, SCREEN_WIDTH - 2 * PADDING_LARGE, boxH, 8, TFT_BLACK);
    
    // Title
    M5.Display.setTextSize(1.5);
    M5.Display.setTextColor(TFT_BLACK);
    M5.Display.drawCentreString(title, SCREEN_WIDTH / 2, boxY + 25, 1);
    
    // Divider
    M5.Display.drawLine(PADDING_LARGE + 20, boxY + 55, SCREEN_WIDTH - PADDING_LARGE - 20, boxY + 55, TFT_BLACK);
    
    // Message body (word wrap)
    M5.Display.setTextSize(1);
    
    int textY = boxY + 70;
    int maxWidth = SCREEN_WIDTH - 2 * PADDING_LARGE - 40;
    int charsPerLine = maxWidth / 10;  // Approximate
    
    String remaining = msg;
    int lineCount = 0;
    
    while (remaining.length() > 0 && lineCount < 4) {
        String line;
        if ((int)remaining.length() <= charsPerLine) {
            line = remaining;
            remaining = "";
        } else {
            int breakPoint = remaining.lastIndexOf(' ', charsPerLine);
            if (breakPoint < charsPerLine / 2) breakPoint = charsPerLine;
            line = remaining.substring(0, breakPoint);
            remaining = remaining.substring(breakPoint + 1);
        }
        
        M5.Display.drawCentreString(line, SCREEN_WIDTH / 2, textY, 1);
        textY += 25;
        lineCount++;
    }
    
    // OK button
    int btnW = 100;
    int btnH = 40;
    int btnX = (SCREEN_WIDTH - btnW) / 2;
    int btnY = SCREEN_HEIGHT - 60;
    
    M5.Display.fillRoundRect(btnX, btnY, btnW, btnH, 8, TFT_BLACK);
    M5.Display.setTextColor(TFT_WHITE);
    M5.Display.drawCentreString("OK", btnX + btnW / 2, btnY + 12, 1);
    
    incrementPartialRefresh();
}

// ============================================================================
// POWER OFF SCREEN
// ============================================================================

void drawPowerOffScreen() {
    forceFullRefresh();
    
    M5.Display.fillScreen(TFT_WHITE);
    
    // Large centered "MindInk" text
    M5.Display.setTextSize(3);
    M5.Display.setTextColor(TFT_BLACK);
    M5.Display.drawCentreString("MindInk", SCREEN_WIDTH / 2, SCREEN_HEIGHT / 2 - 50, 1);
    
    M5.Display.setTextSize(1.5);
    M5.Display.drawCentreString("Powering Off...", SCREEN_WIDTH / 2, SCREEN_HEIGHT / 2 + 30, 1);
}

// ============================================================================
// DEEP SLEEP CONFIRMATION
// ============================================================================

void drawDeepSleepConfirm() {
    M5.Display.fillScreen(TFT_WHITE);
    displayStatusBar();
    
    // Confirmation message
    M5.Display.setTextSize(1.5);
    M5.Display.setTextColor(TFT_BLACK);
    M5.Display.drawCentreString("Enter Deep Sleep?", SCREEN_WIDTH / 2, SCREEN_HEIGHT / 2 - 60, 1);
    
    M5.Display.setTextSize(1);
    M5.Display.drawCentreString("Touch screen or button to wake", SCREEN_WIDTH / 2, SCREEN_HEIGHT / 2 - 20, 1);
    
    // YES and CANCEL buttons
    int btnY = SCREEN_HEIGHT / 2 + 30;
    int btnSpacing = 30;
    
    // YES button
    int yesBtnX = SCREEN_WIDTH / 2 - MENU_BTN_WIDTH - btnSpacing / 2;
    M5.Display.fillRoundRect(yesBtnX, btnY, MENU_BTN_WIDTH, 80, MENU_BTN_RADIUS, TFT_BLACK);
    M5.Display.setTextColor(TFT_WHITE);
    M5.Display.setTextSize(1.25);
    M5.Display.drawCentreString("YES", yesBtnX + MENU_BTN_WIDTH / 2, btnY + 30, 1);
    
    // CANCEL button
    int cancelBtnX = SCREEN_WIDTH / 2 + btnSpacing / 2;
    M5.Display.drawRoundRect(cancelBtnX, btnY, MENU_BTN_WIDTH, 80, MENU_BTN_RADIUS, TFT_BLACK);
    M5.Display.setTextColor(TFT_BLACK);
    M5.Display.drawCentreString("CANCEL", cancelBtnX + MENU_BTN_WIDTH / 2, btnY + 30, 1);
    
    incrementPartialRefresh();
}

// ============================================================================
// DEEP SLEEP SCREEN
// ============================================================================

void drawDeepSleepScreen() {
    forceFullRefresh();
    
    M5.Display.fillScreen(TFT_WHITE);
    
    // Large centered "MindInk" text
    M5.Display.setTextSize(3);
    M5.Display.setTextColor(TFT_BLACK);
    M5.Display.drawCentreString("MindInk", SCREEN_WIDTH / 2, SCREEN_HEIGHT / 2 - 50, 1);
    
    M5.Display.setTextSize(1);
    M5.Display.drawCentreString("Touch to wake", SCREEN_WIDTH / 2, SCREEN_HEIGHT / 2 + 30, 1);
}

// ============================================================================
// CONTROL BUTTON HELPER
// ============================================================================

void drawControlBtn(int x, int y, int w, int h, const char* label, bool active) {
    if (active) {
        M5.Display.fillRoundRect(x, y, w, h, 8, TFT_BLACK);
        M5.Display.setTextColor(TFT_WHITE);
    } else {
        M5.Display.drawRoundRect(x, y, w, h, 8, TFT_BLACK);
        M5.Display.setTextColor(TFT_BLACK);
    }
    M5.Display.drawCentreString(label, x + w / 2, y + h / 2 - 8, 1);
}
