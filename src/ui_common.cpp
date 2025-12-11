/**
 * @file ui_common.cpp
 * @brief Implementation of shared UI primitives for MindInk PaperS3
 */

#include "ui_common.h"
#include "storage.h"
#include <WiFi.h>

// =============================================================================
// STATUS BAR
// =============================================================================

void uiDrawStatusBar() {
    // Draw status bar background
    M5.Display.fillRect(0, 0, UI_SCREEN_WIDTH, UI_STATUS_BAR_HEIGHT, TFT_BLACK);
    M5.Display.setTextColor(TFT_WHITE, TFT_BLACK);
    M5.Display.setFont(&fonts::lgfxJapanGothic_16);
    
    // WiFi status indicator (left side)
    bool wifiConnected = (WiFi.status() == WL_CONNECTED);
    String wifiStr = wifiConnected ? "WiFi" : "No WiFi";
    M5.Display.drawString(wifiStr, UI_PADDING_MEDIUM, 15);
    
    // WiFi signal bars
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
    M5.Display.drawString(batteryStr, UI_SCREEN_WIDTH / 2 + 60, 15);
    
    // Battery icon
    int batX = UI_SCREEN_WIDTH / 2 + 110;
    int batY = 15;
    M5.Display.drawRect(batX, batY, 30, 16, TFT_WHITE);
    M5.Display.fillRect(batX + 30, batY + 4, 4, 8, TFT_WHITE);
    int fillWidth = (batteryLevel * 26) / 100;
    M5.Display.fillRect(batX + 2, batY + 2, fillWidth, 12, TFT_WHITE);
    
    // Storage indicator (right side)
    String storageStr = storage.activeStorage == STORAGE_SD ? "SD" : 
                        (storage.activeStorage == STORAGE_PSRAM ? "RAM" : "--");
    M5.Display.drawRightString(storageStr, UI_SCREEN_WIDTH - UI_PADDING_MEDIUM, 15);
    
    // Bottom separator line
    M5.Display.drawLine(0, UI_STATUS_BAR_HEIGHT - 1, UI_SCREEN_WIDTH, UI_STATUS_BAR_HEIGHT - 1, TFT_DARKGREY);
    
    M5.Display.setFont(nullptr);
}

// =============================================================================
// BUTTON DRAWING
// =============================================================================

void uiDrawButton(const UIButton& btn) {
    // Draw rounded rectangle border (no background fill, e-ink optimized)
    M5.Display.drawRoundRect(btn.x, btn.y, btn.w, btn.h, UI_MENU_BTN_RADIUS, TFT_BLACK);
    M5.Display.drawRoundRect(btn.x + 2, btn.y + 2, btn.w - 4, btn.h - 4, UI_MENU_BTN_RADIUS - 2, TFT_BLACK);
    
    // Draw text centered with larger font
    M5.Display.setTextColor(TFT_BLACK, TFT_WHITE);
    M5.Display.setFont(&fonts::lgfxJapanGothic_16);
    M5.Display.drawCenterString(btn.label, btn.x + btn.w/2, btn.y + btn.h/2 - 8);
    M5.Display.setFont(nullptr);
}

void uiDrawButtonPressed(const UIButton& btn) {
    // Fill with black background for pressed state
    M5.Display.fillRoundRect(btn.x, btn.y, btn.w, btn.h, UI_MENU_BTN_RADIUS, TFT_BLACK);
    
    // Draw text in white (inverted)
    M5.Display.setTextColor(TFT_WHITE, TFT_BLACK);
    M5.Display.setFont(&fonts::lgfxJapanGothic_16);
    M5.Display.drawCenterString(btn.label, btn.x + btn.w/2, btn.y + btn.h/2 - 8);
    M5.Display.setFont(nullptr);
    
    // Partial update for quick feedback
    M5.Display.display();
}

void uiShowButtonFeedback(const UIButton& btn) {
    uiDrawButtonPressed(btn);
    delay(100); // Brief flash
    uiDrawButton(btn);
    M5.Display.display();
}

// =============================================================================
// MESSAGE DIALOGS
// =============================================================================

void uiDrawMessage(const String& title, const String& msg) {
    M5.Display.fillScreen(TFT_WHITE);
    uiDrawStatusBar();
    
    // Title
    M5.Display.setTextColor(TFT_BLACK, TFT_WHITE);
    M5.Display.setTextSize(2);
    M5.Display.drawCenterString(title, M5.Display.width()/2, 80);
    
    // Message body with word wrap
    M5.Display.setTextSize(1);
    M5.Display.setTextColor(TFT_DARKGREY, TFT_WHITE);
    
    int lineY = 150;
    int maxWidth = M5.Display.width() - 40;
    String remaining = msg;
    
    while (remaining.length() > 0 && lineY < M5.Display.height() - 80) {
        String line = "";
        int charIdx = 0;
        
        while (charIdx < (int)remaining.length()) {
            String testLine = line + remaining.charAt(charIdx);
            if (M5.Display.textWidth(testLine) > maxWidth) {
                break;
            }
            line = testLine;
            charIdx++;
        }
        
        if (line.length() == 0 && remaining.length() > 0) {
            line = remaining.substring(0, 1);
            charIdx = 1;
        }
        
        M5.Display.drawCenterString(line, M5.Display.width()/2, lineY);
        remaining = remaining.substring(charIdx);
        remaining.trim();
        lineY += 25;
    }
    
    // OK button
    int btnW = 100;
    int btnH = 40;
    int btnX = (M5.Display.width() - btnW) / 2;
    int btnY = M5.Display.height() - 60;
    M5.Display.fillRect(btnX, btnY, btnW, btnH, TFT_LIGHTGREY);
    M5.Display.drawRect(btnX, btnY, btnW, btnH, TFT_BLACK);
    M5.Display.setTextColor(TFT_BLACK, TFT_LIGHTGREY);
    M5.Display.drawCenterString("OK", btnX + btnW/2, btnY + btnH/2 - 5);
}

void uiDrawProcessing(const String& message) {
    M5.Display.fillScreen(TFT_WHITE);
    uiDrawStatusBar();
    
    M5.Display.setTextColor(TFT_BLACK, TFT_WHITE);
    M5.Display.setTextSize(2);
    M5.Display.drawCenterString(message, M5.Display.width()/2, M5.Display.height()/2 - 20);
    
    M5.Display.setTextSize(1);
    M5.Display.drawCenterString("Please wait...", M5.Display.width()/2, M5.Display.height()/2 + 40);
}

void uiDrawError(const String& message) {
    M5.Display.fillScreen(TFT_WHITE);
    uiDrawStatusBar();
    
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

// =============================================================================
// DISPLAY UTILITIES
// =============================================================================

void uiForceFullRefresh() {
    // Flash screen black then white to clear ghosting
    M5.Display.fillScreen(TFT_BLACK);
    delay(50);
    M5.Display.fillScreen(TFT_WHITE);
    delay(50);
}
