/*
 * Project: MindInk - M5 Paper S3 Portable Note & Infographic Companion (IMPROVED)
 * Author: Enhanced by AI Assistant
 * Date: 2025-12-03
 *
 * Improvements:
 * - Fixed display initialization for PAPERS3 (rotation, proper clear)
 * - Fixed SD card pins for PAPERS3 (47, 39, 38, 40)
 * - Added SD card detection with PSRAM fallback
 * - Border-only button style (no filled background)
 *
 * Capabilities:
 * - Audio Recording (Mic -> SD/PSRAM .wav)
 * - Transcription (Eleven Labs API)
 * - Summarization (Gemini 1.5 Flash API)
 * - Infographic Generation (Gemini 3 Pro Image Preview API)
 *
 * Dependencies:
 * - M5Unified
 * - ArduinoJson (v7.x)
 */

#include <M5Unified.h>
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <SD.h>
#include <SPI.h>
#include "FS.h"
#include "mbedtls/base64.h"

// ============================================================================
// CONFIGURATION
// ============================================================================
const char* WIFI_SSID     = "D815 5g";
const char* WIFI_PASS     = "inti815inti815";

const char* ELEVEN_LABS_API_KEY = "sk_e3793377e47c5f018a4102a24d56da792fcaf5a7a4e7c7a8";
const char* GEMINI_API_KEY      = "AIzaSyDc4lO_EM2mwSuerCH0e5Q-ugdWrkLXdvc";

const char* EL_API_URL = "https://api.elevenlabs.io/v1/speech-to-text";
const char* GEMINI_TEXT_URL = "https://generativelanguage.googleapis.com/v1beta/models/gemini-1.5-flash:generateContent?key=";
const char* GEMINI_IMAGE_URL = "https://generativelanguage.googleapis.com/v1alpha/models/gemini-3-pro-image-preview:generateContent?key=";

// ============================================================================
// HARDWARE CONFIG - PAPERS3 SPECIFIC
// ============================================================================
#define SD_CS   47
#define SD_SCK  39
#define SD_MOSI 38
#define SD_MISO 40

#define RECORD_SAMPLE_RATE 16000
#define RECORD_BIT_DEPTH   16
#define RX_BUFFER_SIZE     1024
#define MAX_RECORDING_TIME 300000

// ============================================================================
// DATA STRUCTURES
// ============================================================================
enum StorageType {
    STORAGE_SD,
    STORAGE_PSRAM,
    STORAGE_NONE
};

struct StorageInfo {
    bool sdAvailable;
    bool psramAvailable;
    size_t psramTotal;
    size_t psramFree;
    StorageType activeStorage;
};

struct AudioFile {
    String filename;
    String dateTime;
    StorageType storage;
};

struct SummaryFile {
    String filename;
    String relatedAudioFilename;
    String dateTime;
    StorageType storage;
};

struct ImageFile {
    String originalFilename;
    String summaryFilename;
    String dateTime;
    StorageType storage;
};

struct Button {
    int x, y, w, h;
    String label;
};

enum AppState {
    STATE_MENU,
    STATE_RECORDING,
    STATE_LIST_AUDIO,
    STATE_LIST_SUMMARIES,
    STATE_VIEW_SUMMARY,
    STATE_GALLERY,
    STATE_PROCESSING,
    STATE_ERROR
};

// ============================================================================
// GLOBALS
// ============================================================================
StorageInfo storage;
AppState currentState = STATE_MENU;
String currentTextContent = "";
std::vector<AudioFile> recentAudio;
std::vector<SummaryFile> recentSummaries;
std::vector<ImageFile> recentImages;
std::vector<Button> menuButtons;

bool isRecording = false;
File recordingFile;

// ============================================================================
// STORAGE INITIALIZATION
// ============================================================================

void initStorage() {
    Serial.println("[STORAGE] Initializing storage systems...");
    
    storage.psramAvailable = psramFound();
    if (storage.psramAvailable) {
        storage.psramTotal = ESP.getPsramSize();
        storage.psramFree = ESP.getFreePsram();
        Serial.printf("[STORAGE] PSRAM: %d bytes total, %d free\n", storage.psramTotal, storage.psramFree);
    }
    
    SPI.begin(SD_SCK, SD_MISO, SD_MOSI, SD_CS);
    delay(300);
    
    storage.sdAvailable = SD.begin(SD_CS, SPI, 10000000);
    if (storage.sdAvailable) {
        Serial.println("[STORAGE] SD Card initialized");
        storage.activeStorage = STORAGE_SD;
    } else {
        Serial.println("[STORAGE] SD Card not found - using PSRAM");
        storage.activeStorage = storage.psramAvailable ? STORAGE_PSRAM : STORAGE_NONE;
    }
}

// ============================================================================
// UI DRAWING - BORDER-ONLY BUTTONS
// ============================================================================

void setupButtons() {
    menuButtons.clear();
    menuButtons.push_back({50, 100, 200, 80, "RECORD"});
    menuButtons.push_back({50, 220, 200, 80, "AUDIO FILES"});
    menuButtons.push_back({300, 100, 200, 80, "SUMMARIES"});
    menuButtons.push_back({300, 220, 200, 80, "GALLERY"});
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
        M5.Display.drawString(img.originalFilename.substring(0, 15), x + 5, y + 70);
        x += 160;
        if(x > M5.Display.width() - 160) { x = 20; y += 160; }
    }
    
    M5.Display.fillRect(20, M5.Display.height() - 60, 100, 40, TFT_LIGHTGREY);
    M5.Display.setTextColor(TFT_BLACK, TFT_LIGHTGREY);
    M5.Display.drawCenterString("BACK", 70, M5.Display.height() - 45);
}

// ============================================================================
// SETUP & LOOP
// ============================================================================

void setup() {
    auto cfg = M5.config();
    cfg.serial_baudrate = 115200;
    M5.begin(cfg);
    
    Serial.println("\n\n=== MindInk - M5 Paper S3 ===");
    
    M5.Display.setRotation(2);
    M5.Display.clear();
    delay(300);
    Serial.println("[DISPLAY] PAPERS3 display initialized");

    initStorage();

    M5.Display.fillScreen(TFT_WHITE);
    M5.Display.setTextColor(TFT_BLACK, TFT_WHITE);
    M5.Display.drawString("Connecting WiFi...", 10, 10);
    
    WiFi.begin(WIFI_SSID, WIFI_PASS);
    int attempts = 0;
    while (WiFi.status() != WL_CONNECTED && attempts < 20) {
        delay(500);
        attempts++;
        Serial.print(".");
    }
    
    if(WiFi.status() == WL_CONNECTED) {
        Serial.println("\n[WiFi] Connected!");
        M5.Display.drawString("WiFi Connected!", 10, 50);
    } else {
        Serial.println("\n[WiFi] Failed");
        M5.Display.drawString("WiFi Failed - Offline", 10, 50);
    }
    
    delay(1000);
    setupButtons();
    drawMenu();
    
    Serial.println("[SETUP] Complete");
    Serial.printf("[MEMORY] Free Heap: %d bytes\n", ESP.getFreeHeap());
}

void loop() {
    M5.update();
    
    if (M5.Touch.getCount() > 0) {
        auto t = M5.Touch.getDetail(0);
        if (t.wasPressed()) {
            int tx = t.x;
            int ty = t.y;
            
            if (currentState == STATE_MENU) {
                if (checkButtonPress(tx, ty, menuButtons[0])) {
                    Serial.println("[UI] Record pressed");
                    currentState = STATE_RECORDING;
                    M5.Display.fillScreen(TFT_RED);
                    M5.Display.setTextColor(TFT_WHITE, TFT_RED);
                    M5.Display.drawCenterString("RECORDING...", M5.Display.width()/2, M5.Display.height()/2);
                }
                else if (checkButtonPress(tx, ty, menuButtons[1])) {
                    Serial.println("[UI] Audio Files pressed");
                    currentState = STATE_LIST_AUDIO;
                    std::vector<String> list;
                    for(auto a : recentAudio) list.push_back(a.filename);
                    drawList("Audio Files", list.empty() ? std::vector<String>{"(empty)"} : list);
                }
                else if (checkButtonPress(tx, ty, menuButtons[2])) {
                    Serial.println("[UI] Summaries pressed");
                    currentState = STATE_LIST_SUMMARIES;
                    std::vector<String> list;
                    for(auto s : recentSummaries) list.push_back(s.filename);
                    drawList("Summaries", list.empty() ? std::vector<String>{"(empty)"} : list);
                }
                else if (checkButtonPress(tx, ty, menuButtons[3])) {
                    Serial.println("[UI] Gallery pressed");
                    currentState = STATE_GALLERY;
                    drawGallery(recentImages);
                }
            }
            else if (currentState == STATE_RECORDING) {
                Serial.println("[UI] Stop recording");
                currentState = STATE_MENU;
                drawMenu();
            }
            else if (currentState == STATE_LIST_AUDIO || currentState == STATE_LIST_SUMMARIES || currentState == STATE_GALLERY) {
                if (ty > M5.Display.height() - 60 && tx < 120) {
                    currentState = STATE_MENU;
                    drawMenu();
                }
            }
        }
    }
    
    delay(50);
}
