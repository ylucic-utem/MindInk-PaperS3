/*
 * Project: MindInk - M5 Paper S3 Portable Note & Infographic Companion
 * Author: Enhanced by AI Assistant
 * Date: 2025-12-04
 *
 * Refactored version with modular architecture
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

// Include all modular headers
#include "config.h"
#include "storage.h"
#include "display.h"
#include "audio.h"
#include "api.h"

// ============================================================================
// CONFIGURATION VALUES
// ============================================================================
// IMPORTANT: These credentials are defined in src/config.cpp
// Create your own config.cpp from config.cpp.example
// DO NOT commit credentials to version control!

// API URLs (public - safe to keep here)
const char* EL_API_URL = "https://api.elevenlabs.io/v1/speech-to-text";
const char* GEMINI_TEXT_URL = "https://generativelanguage.googleapis.com/v1beta/models/gemini-1.5-flash:generateContent?key=";
const char* GEMINI_IMAGE_URL = "https://generativelanguage.googleapis.com/v1alpha/models/gemini-3-pro-image-preview:generateContent?key=";

// ============================================================================
// GLOBAL STATE
// ============================================================================
std::vector<AudioFile> recentAudio;
std::vector<SummaryFile> recentSummaries;
std::vector<ImageFile> recentImages;

// ============================================================================
// SETUP
// ============================================================================

void initWiFi() {
    Serial.println("[WiFi] Connecting...");
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
}

void setup() {
    auto cfg = M5.config();
    cfg.serial_baudrate = 115200;
    M5.begin(cfg);
    
    Serial.println("\n\n=== MindInk - M5 Paper S3 (Refactored) ===");
    
    M5.Display.setRotation(2);
    M5.Display.clear();
    delay(300);
    Serial.println("[DISPLAY] PAPERS3 display initialized");

    // Initialize storage systems
    initStorage();
    
    // Initialize WiFi
    initWiFi();
    
    // Initialize UI
    setupButtons();
    drawMenu();
    
    Serial.println("[SETUP] Complete");
    Serial.printf("[MEMORY] Free Heap: %d bytes\n", ESP.getFreeHeap());
}

// ============================================================================
// LOOP
// ============================================================================

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
                    drawProcessing("RECORDING...");
                    
                    // TODO: Implement actual recording
                    delay(2000);
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
            else if (currentState == STATE_LIST_AUDIO || currentState == STATE_LIST_SUMMARIES || 
                     currentState == STATE_VIEW_SUMMARY || currentState == STATE_GALLERY) {
                if (ty > M5.Display.height() - 60 && tx < 120) {
                    currentState = STATE_MENU;
                    drawMenu();
                }
            }
        }
    }
    
    delay(50);
}
