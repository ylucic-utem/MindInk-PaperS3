/**
 * @file display_refresh.cpp
 * @brief E-paper display refresh and anti-ghosting for MindInk PaperS3
 * 
 * Extracted from display.cpp for modularity.
 */

#include "display_refresh.h"
#include "display.h"
#include "memory_utils.h"
#include <M5Unified.h>
#include <WiFi.h>
#include <SD.h>

// =============================================================================
// ANTI-GHOSTING STATE
// =============================================================================
static unsigned long lastFullRefresh = 0;
static int partialRefreshCount = 0;
static bool fullRefreshRequested = false;

// =============================================================================
// REFRESH FUNCTIONS
// =============================================================================

void forceFullRefresh() {
    Serial.println("[REFRESH] Forcing full display refresh");
    
    // Multiple full refresh cycles to clear ghosting
    for (int i = 0; i < 2; i++) {
        M5.Display.fillScreen(TFT_WHITE);
        M5.Display.display();
        delay(100);
        M5.Display.fillScreen(TFT_BLACK);
        M5.Display.display();
        delay(100);
    }
    M5.Display.fillScreen(TFT_WHITE);
    M5.Display.display();
    
    lastFullRefresh = millis();
    partialRefreshCount = 0;
    fullRefreshRequested = false;
}

void incrementPartialRefresh() {
    partialRefreshCount++;
}

void checkAndRefresh() {
    unsigned long now = millis();
    
    // Check if full refresh was requested
    if (fullRefreshRequested) {
        forceFullRefresh();
        return;
    }
    
    // Check time-based refresh (every GHOST_REFRESH_INTERVAL ms)
    if (now - lastFullRefresh > GHOST_REFRESH_INTERVAL) {
        Serial.println("[REFRESH] Time-based ghosting prevention");
        forceFullRefresh();
        return;
    }
    
    // Check partial refresh count threshold
    if (partialRefreshCount >= PARTIAL_REFRESH_THRESHOLD) {
        Serial.printf("[REFRESH] Partial count threshold reached (%d)\n", partialRefreshCount);
        forceFullRefresh();
        return;
    }
}

void requestFullRefresh() {
    fullRefreshRequested = true;
}

// =============================================================================
// DIAGNOSTICS SCREEN
// =============================================================================

void drawDiagnosticsScreen() {
    M5.Display.fillScreen(TFT_WHITE);
    displayStatusBar();
    
    M5.Display.setTextSize(1.5);
    M5.Display.setTextColor(TFT_BLACK);
    M5.Display.drawString("System Diagnostics", PADDING_MEDIUM, STATUS_BAR_HEIGHT + PADDING_MEDIUM);
    
    int y = STATUS_BAR_HEIGHT + 60;
    int lineHeight = 35;
    
    M5.Display.setTextSize(1);
    
    // Battery
    int batteryLevel = M5.Power.getBatteryLevel();
    bool isCharging = M5.Power.isCharging();
    String batteryStr = "Battery: " + String(batteryLevel) + "%" + (isCharging ? " (charging)" : "");
    M5.Display.drawString(batteryStr, PADDING_MEDIUM, y);
    y += lineHeight;
    
    // WiFi
    bool wifiConnected = WiFi.status() == WL_CONNECTED;
    String wifiStr = "WiFi: " + String(wifiConnected ? "Connected" : "Disconnected");
    if (wifiConnected) {
        wifiStr += " (" + String(WiFi.RSSI()) + " dBm)";
    }
    M5.Display.drawString(wifiStr, PADDING_MEDIUM, y);
    y += lineHeight;
    
    if (wifiConnected) {
        String ipStr = "IP: " + WiFi.localIP().toString();
        M5.Display.drawString(ipStr, PADDING_MEDIUM, y);
        y += lineHeight;
    }
    
    // SD Card
    uint64_t sdTotal = SD.totalBytes();
    uint64_t sdUsed = SD.usedBytes();
    String sdStr;
    if (sdTotal > 0) {
        sdStr = "SD Card: " + String(sdUsed / 1048576) + " / " + String(sdTotal / 1048576) + " MB";
    } else {
        sdStr = "SD Card: Not available";
    }
    M5.Display.drawString(sdStr, PADDING_MEDIUM, y);
    y += lineHeight;
    
    // Memory
    uint32_t freeHeap = ESP.getFreeHeap();
    uint32_t totalHeap = ESP.getHeapSize();
    String memStr = "Free Heap: " + String(freeHeap / 1024) + " / " + String(totalHeap / 1024) + " KB";
    M5.Display.drawString(memStr, PADDING_MEDIUM, y);
    y += lineHeight;
    
    uint32_t freePsram = ESP.getFreePsram();
    uint32_t totalPsram = ESP.getPsramSize();
    if (totalPsram > 0) {
        String psramStr = "Free PSRAM: " + String(freePsram / 1024) + " / " + String(totalPsram / 1024) + " KB";
        M5.Display.drawString(psramStr, PADDING_MEDIUM, y);
        y += lineHeight;
    }
    
    // Largest free block
    uint32_t largestBlock = ESP.getMaxAllocHeap();
    String blockStr = "Largest Block: " + String(largestBlock / 1024) + " KB";
    M5.Display.drawString(blockStr, PADDING_MEDIUM, y);
    y += lineHeight;
    
    // Display refresh stats
    String refreshStr = "Partial Refreshes: " + String(partialRefreshCount);
    M5.Display.drawString(refreshStr, PADDING_MEDIUM, y);
    y += lineHeight;
    
    unsigned long sinceRefresh = (millis() - lastFullRefresh) / 1000;
    String timeStr = "Since Full Refresh: " + String(sinceRefresh) + "s";
    M5.Display.drawString(timeStr, PADDING_MEDIUM, y);
    y += lineHeight;
    
    // Chip info
    y += 10;
    M5.Display.drawString("ESP32 Model: " + String(ESP.getChipModel()), PADDING_MEDIUM, y);
    y += lineHeight;
    
    String coresStr = "Cores: " + String(ESP.getChipCores()) + " @ " + String(ESP.getCpuFreqMHz()) + " MHz";
    M5.Display.drawString(coresStr, PADDING_MEDIUM, y);
    y += lineHeight;
    
    // Back button
    int backBtnY = SCREEN_HEIGHT - 70;
    M5.Display.drawRoundRect(PADDING_MEDIUM, backBtnY, NAV_BTN_WIDTH, NAV_BTN_HEIGHT, 8, TFT_BLACK);
    M5.Display.drawCentreString("BACK", PADDING_MEDIUM + NAV_BTN_WIDTH/2, backBtnY + 15, 1);
    
    // Refresh button
    int refreshBtnX = SCREEN_WIDTH - PADDING_MEDIUM - NAV_BTN_WIDTH;
    M5.Display.drawRoundRect(refreshBtnX, backBtnY, NAV_BTN_WIDTH, NAV_BTN_HEIGHT, 8, TFT_BLACK);
    M5.Display.drawCentreString("REFRESH", refreshBtnX + NAV_BTN_WIDTH/2, backBtnY + 15, 1);
    
    incrementPartialRefresh();
}
