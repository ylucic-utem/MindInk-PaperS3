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

// ============================================================================
// UI SETUP
// ============================================================================

void setupButtons() {
    menuButtons.clear();
    Button btn1 = {50, 100, 200, 80, "RECORD"};
    menuButtons.push_back(btn1);
    Button btn2 = {50, 220, 200, 80, "AUDIO FILES"};
    menuButtons.push_back(btn2);
    Button btn3 = {300, 100, 200, 80, "SUMMARIES"};
    menuButtons.push_back(btn3);
    Button btn4 = {300, 220, 200, 80, "GALLERY"};
    menuButtons.push_back(btn4);
    Button btn5 = {50, 340, 200, 80, "DEEP SLEEP"};
    menuButtons.push_back(btn5);
    Button btn6 = {300, 340, 200, 80, "POWER OFF"};
    menuButtons.push_back(btn6);
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
    M5.Display.drawCenterString(message, M5.Display.width()/2, M5.Display.height()/2, M5.Display.width() - 40);
    
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

void drawRecordingPage(bool isRecording, bool isPaused, bool hasRecordedData) {
    M5.Display.fillScreen(TFT_WHITE);
    displayStatusBar();
    
    // Title
    M5.Display.setTextColor(TFT_BLACK, TFT_WHITE);
    M5.Display.setTextSize(2);
    M5.Display.drawCenterString("RECORDING STUDIO", M5.Display.width()/2, 50);
    
    int btnW = 120;
    int btnH = 60;
    int centerX = M5.Display.width()/2;
    int centerY = M5.Display.height()/2;
    
    if (isRecording) {
        // Recording in progress
        M5.Display.setTextSize(1);
        M5.Display.drawCenterString("Recording...", centerX, 90);
        M5.Display.fillCircle(centerX - 60, 90, 6, TFT_RED);
        
        // PAUSE (Left)
        drawControlBtn(centerX - btnW - 10, centerY, btnW, btnH, "PAUSE");
        
        // STOP (Right)
        drawControlBtn(centerX + 10, centerY, btnW, btnH, "STOP");
        
    } else if (hasRecordedData) {
        // Recording finished, waiting for action
        M5.Display.setTextSize(1);
        M5.Display.drawCenterString("Recording Complete", centerX, 90);
        
        // TRANSCRIBE (Left)
        drawControlBtn(centerX - btnW - 10, centerY, btnW, btnH, "TRANSCRIBE");
        
        // DISCARD (Right)
        drawControlBtn(centerX + 10, centerY, btnW, btnH, "DISCARD");
        
    } else {
        // Ready to record
        M5.Display.setTextSize(1);
        M5.Display.drawCenterString("Ready", centerX, 90);
        
        // START RECORDING (Center)
        drawControlBtn(centerX - btnW/2, centerY, btnW, btnH, "START REC");
        
        // BACK (Bottom)
        drawControlBtn(20, M5.Display.height() - 50, 80, 40, "BACK");
    }
}

void drawAudioPlayer(String filename, bool isPlaying, bool isPaused) {
    M5.Display.fillScreen(TFT_WHITE);
    displayStatusBar();
    
    // Title
    M5.Display.setTextColor(TFT_BLACK, TFT_WHITE);
    M5.Display.setTextSize(2);
    M5.Display.drawCenterString("AUDIO PLAYER", M5.Display.width()/2, 40);
    
    // Filename
    M5.Display.setTextSize(1);
    M5.Display.drawCenterString(filename, M5.Display.width()/2, 80);
    
    // Status
    String status = isPlaying ? (isPaused ? "Paused" : "Playing...") : "Stopped";
    M5.Display.setTextColor(TFT_DARKGREY, TFT_WHITE);
    M5.Display.drawCenterString(status, M5.Display.width()/2, 110);
    
    int btnW = 100;
    int btnH = 50;
    int startX = (M5.Display.width() - (3 * btnW + 40)) / 2;
    int y = 160;
    
    // PLAY
    drawControlBtn(startX, y, btnW, btnH, "PLAY", isPlaying && !isPaused);
    
    // PAUSE
    drawControlBtn(startX + btnW + 20, y, btnW, btnH, "PAUSE", isPaused);
    
    // STOP
    drawControlBtn(startX + 2 * (btnW + 20), y, btnW, btnH, "STOP", !isPlaying);
    
    // BACK
    drawControlBtn(20, M5.Display.height() - 50, 80, 40, "BACK");
}

void drawPowerOffScreen() {
    M5.Display.fillScreen(TFT_WHITE);
    M5.Display.setTextColor(TFT_BLACK, TFT_WHITE);
    M5.Display.setTextSize(4);
    M5.Display.drawCenterString("MindInk", M5.Display.width()/2, M5.Display.height()/2 - 20);
}
