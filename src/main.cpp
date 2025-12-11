/*
 * Project: MindInk - M5 Paper S3 Cloud Remote & Viewer
 * Author: Enhanced by AI Assistant
 * Date: 2025-12-09
 *
 * Cloud-focused architecture: No local audio processing
 * Device acts as a remote trigger for Supabase Edge Functions
 * and a viewer for summaries and infographics.
 *
 * Capabilities:
 * - List audio files from Supabase
 * - Trigger cloud processing via Edge Function
 * - View summaries from Supabase
 * - View infographics from Supabase
 *
 * Dependencies:
 * - M5Unified
 * - ArduinoJson (v7.x)
 */

#include <M5Unified.h>
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <SD.h>

// Include modular headers (no audio or local_processing)
#include "config.h"
#include "storage.h"
#include "display.h"
#include "wifi_manager.h"
#include "supabase_client.h"
#include "config_manager.h"
#include "memory_utils.h"

// ============================================================================
// GLOBAL STATE
// ============================================================================
std::vector<AudioFile> recentAudio;
std::vector<SummaryFile> recentSummaries;
std::vector<ImageFile> recentImages;

// Selection indices for list navigation
int selectedAudioIndex = -1;
int selectedSummaryIndex = -1;
int selectedImageIndex = -1;

// Processing state variables
String pendingSummaryIdForInfographic = "";
unsigned long infographicGenerationStartTime = 0;

// =============================================================================
// POWER MANAGEMENT STATE
// =============================================================================
unsigned long lastActivityTime = 0;
unsigned long lastBatteryCheckTime = 0;
bool idleWarningShown = false;
int lastBatteryLevel = 100;

// ============================================================================
// SETUP
// ============================================================================

void setup() {
    auto cfg = M5.config();
    cfg.serial_baudrate = 115200;
    M5.begin(cfg);
    
    Serial.println("\n\n=== MindInk - M5 Paper S3 (Cloud Remote & Viewer) ===");
    
    M5.Display.setRotation(2);
    M5.Display.clear();
    delay(300);
    Serial.println("[DISPLAY] PAPERS3 display initialized");

    // Initialize configuration (load saved preferences from NVS)
    configInit();
    
    // Initialize memory monitoring
    memInit();

    // Initialize storage systems (for caching)
    initStorage();
    
    // Initialize WiFi
    initWiFi();
    
    // Initialize UI
    setupButtons();
    
    // Apply saved font size preference to ebook reader
    ebookReader.fontSize = appConfig.fontSize;
    
    drawMenu();
    
    // Initialize activity timer
    lastActivityTime = millis();
    
    Serial.println("[SETUP] Complete - Cloud Remote & Viewer Mode");
    memPrintStatus();
}

// ============================================================================
// HELPER: Draw Audio List with Status Icons
// ============================================================================
void drawAudioListWithStatus() {
    std::vector<String> list;
    for (const auto& a : recentAudio) {
        String item = a.filename;
        if (a.status.length() > 0) {
            item += " [" + a.status + "]";
        }
        list.push_back(item);
    }
    drawList("Audio Files (Tap to Process)", list.empty() ? std::vector<String>{"(No audio files found)"} : list);
}

// ============================================================================
// HELPER: Draw Summaries List
// ============================================================================
void drawSummariesList() {
    std::vector<String> list;
    for (const auto& s : recentSummaries) {
        String item = s.filename;
        if (s.status.length() > 0) {
            item += " (" + s.status + ")";
        }
        list.push_back(item);
    }
    drawList("Summaries", list.empty() ? std::vector<String>{"(No summaries found)"} : list);
}

// ============================================================================
// HELPER: Draw Gallery List
// ============================================================================
void drawGalleryList() {
    std::vector<String> list;
    for (const auto& img : recentImages) {
        String item = img.filename.length() > 0 ? img.filename : "(image)";
        list.push_back(item);
    }
    drawList("Gallery", list.empty() ? std::vector<String>{"(No images found)"} : list);
}

// ============================================================================
// LOOP
// ============================================================================

void loop() {
    M5.update();
    
    int w = M5.Display.width();
    int h = M5.Display.height();
    
    if (M5.Touch.getCount() > 0) {
        auto t = M5.Touch.getDetail(0);
        if (t.wasPressed()) {
            int tx = t.x;
            int ty = t.y;
            
            // ================================================================
            // STATE: MENU
            // ================================================================
            if (currentState == STATE_MENU) {
                // Button 0: AUDIO FILES (Remote Trigger)
                if (checkButtonPress(tx, ty, menuButtons[0])) {
                    currentState = STATE_LIST_AUDIO;
                    drawProcessing("Fetching audio files...");
                    
                    if (isWiFiConnected() && fetchSupabaseAudio(recentAudio)) {
                        drawAudioListWithStatus();
                    } else {
                        drawError("Failed to fetch audio files. Check WiFi connection.");
                        currentState = STATE_ERROR;
                    }
                }
                // Button 1: SUMMARIES (View Text)
                else if (checkButtonPress(tx, ty, menuButtons[1])) {
                    currentState = STATE_LIST_SUMMARIES;
                    drawProcessing("Fetching summaries...");
                    
                    if (isWiFiConnected() && fetchSupabaseSummaries(recentSummaries)) {
                        drawSummariesList();
                    } else {
                        drawError("Failed to fetch summaries. Check WiFi connection.");
                        currentState = STATE_ERROR;
                    }
                }
                // Button 2: GALLERY (View Images)
                else if (checkButtonPress(tx, ty, menuButtons[2])) {
                    currentState = STATE_GALLERY;
                    drawProcessing("Loading gallery...");
                    
                    std::vector<String> options;
                    options.push_back("Create Infographic");
                    options.push_back("View Saved");
                    drawList("Gallery Menu", options);
                }
                // Button 3: POWER OFF
                else if (checkButtonPress(tx, ty, menuButtons[3])) {
                    drawPowerOffScreen();
                    delay(1000);
                    M5.Power.powerOff();
                }
                // Button 4: DEEP SLEEP
                else if (menuButtons.size() > 4 && checkButtonPress(tx, ty, menuButtons[4])) {
                    currentState = STATE_DEEP_SLEEP_CONFIRM;
                    drawDeepSleepConfirm();
                }
            }
            
            // ================================================================
            // STATE: LIST AUDIO (Remote Trigger)
            // ================================================================
            else if (currentState == STATE_LIST_AUDIO) {
                // Check list items (using new LIST_ITEM_HEIGHT)
                int startY = STATUS_BAR_HEIGHT + 70;
                bool itemSelected = false;
                
                for (size_t i = 0; i < recentAudio.size(); i++) {
                    int y = startY + i * (LIST_ITEM_HEIGHT + PADDING_SMALL);
                    if (ty >= y && ty <= y + LIST_ITEM_HEIGHT && tx >= PADDING_MEDIUM && tx <= SCREEN_WIDTH - PADDING_MEDIUM) {
                        itemSelected = true;
                        
                        // Trigger cloud processing for this audio file
                        AudioFile& selected = recentAudio[i];
                        
                        if (selected.id.isEmpty()) {
                            drawError("Invalid audio file ID");
                            currentState = STATE_ERROR;
                        } else {
                            drawProcessing("Triggering cloud processing...");
                            
                            if (triggerProcessAudio(selected.id)) {
                                drawMessage("Processing Triggered!", 
                                    "Cloud processing has started for: " + selected.filename + ". Check back later for results.");
                                currentState = STATE_PROCESSING;
                            } else {
                                drawError("Failed to trigger processing. Please try again.");
                                currentState = STATE_ERROR;
                            }
                        }
                        break;
                    }
                }
                
                // Back button (new position)
                int backBtnY = SCREEN_HEIGHT - 70;
                if (!itemSelected && ty >= backBtnY && ty <= backBtnY + NAV_BTN_HEIGHT && tx >= PADDING_MEDIUM && tx <= PADDING_MEDIUM + NAV_BTN_WIDTH) {
                    currentState = STATE_MENU;
                    drawMenu();
                }
            }
            
            // ================================================================
            // STATE: PROCESSING (Message display - waiting for user action)
            // ================================================================
            else if (currentState == STATE_PROCESSING) {
                int btnW = 100;
                int btnH = 40;
                int btnX = (w - btnW) / 2;
                int btnY = h - 60;
                
                // Check if OK button area tapped
                if (tx >= btnX && tx <= btnX + btnW && ty >= btnY && ty <= btnY + btnH) {
                    // If waiting for infographic, try to download it
                    if (!pendingSummaryIdForInfographic.isEmpty()) {
                        drawProcessing("Downloading infographic to SD card...");
                        String localPath;
                        
                        if (isWiFiConnected() && fetchAndSaveInfographicToSD(pendingSummaryIdForInfographic, localPath)) {
                            drawImageFromFile("Infographic", localPath);
                            currentState = STATE_VIEW_IMAGE;
                            selectedImageIndex = 1; // Mark as coming from processing
                            pendingSummaryIdForInfographic = "";
                        } else {
                            drawError("Failed to download infographic. Check WiFi and try again.");
                            currentState = STATE_ERROR;
                            pendingSummaryIdForInfographic = "";
                        }
                    } else {
                        currentState = STATE_MENU;
                        drawMenu();
                    }
                }
            }
            
            // ================================================================
            // STATE: LIST SUMMARIES
            // ================================================================
            else if (currentState == STATE_LIST_SUMMARIES) {
                // Check list items (using new LIST_ITEM_HEIGHT)
                int startY = STATUS_BAR_HEIGHT + 70;
                bool itemSelected = false;
                
                for (size_t i = 0; i < recentSummaries.size(); i++) {
                    int y = startY + i * (LIST_ITEM_HEIGHT + PADDING_SMALL);
                    if (ty >= y && ty <= y + LIST_ITEM_HEIGHT && tx >= PADDING_MEDIUM && tx <= SCREEN_WIDTH - PADDING_MEDIUM) {
                        itemSelected = true;
                        
                        SummaryFile& selected = recentSummaries[i];
                        String localPath = "/sd/" + selected.filename;
                        bool loaded = false;
                        String textContent = "";
                        
                        // Try local SD cache first
                        if (storage.activeStorage == STORAGE_SD && SD.exists(localPath)) {
                            File f = SD.open(localPath, FILE_READ);
                            if (f) {
                                textContent = f.readString();
                                f.close();
                                loaded = true;
                            }
                        }
                        
                        // If not cached, download from Supabase
                        if (!loaded && isWiFiConnected() && !selected.id.isEmpty()) {
                            drawProcessing("Downloading summary...");
                            
                            if (storage.activeStorage == STORAGE_SD) {
                                if (downloadSummaryFromSupabase(selected.id, localPath)) {
                                    File f = SD.open(localPath, FILE_READ);
                                    if (f) {
                                        textContent = f.readString();
                                        f.close();
                                        loaded = true;
                                    }
                                }
                            } else {
                                loaded = fetchSummaryTextFromSupabase(selected.id, textContent);
                            }
                        }
                        
                        if (!loaded) {
                            textContent = "(Summary not available offline)";
                        }
                        
                        currentState = STATE_VIEW_SUMMARY;
                        ebookReader.setText(textContent);
                        drawEbookPage();
                        break;
                    }
                }
                
                // Back button (new position)
                int backBtnY = SCREEN_HEIGHT - 70;
                if (!itemSelected && ty >= backBtnY && ty <= backBtnY + NAV_BTN_HEIGHT && tx >= PADDING_MEDIUM && tx <= PADDING_MEDIUM + NAV_BTN_WIDTH) {
                    currentState = STATE_MENU;
                    drawMenu();
                }
            }
            
            // ================================================================
            // STATE: GALLERY
            // ================================================================
            else if (currentState == STATE_GALLERY) {
                int startY = STATUS_BAR_HEIGHT + 70;
                bool itemSelected = false;
                
                // Gallery menu has 2 options: Create Infographic, View Saved
                for (size_t i = 0; i < 2; i++) {
                    int y = startY + i * (LIST_ITEM_HEIGHT + PADDING_SMALL);
                    if (ty >= y && ty <= y + LIST_ITEM_HEIGHT && tx >= PADDING_MEDIUM && tx <= SCREEN_WIDTH - PADDING_MEDIUM) {
                        itemSelected = true;
                        
                        if (i == 0) {
                            // Create Infographic: fetch summaries
                            currentState = STATE_GALLERY_SUMMARIES;
                            drawProcessing("Fetching summaries...");
                            
                            if (isWiFiConnected() && fetchSupabaseSummaries(recentSummaries)) {
                                drawSummariesForInfographic(recentSummaries);
                            } else {
                                drawError("Failed to fetch summaries. Check WiFi connection.");
                                currentState = STATE_ERROR;
                            }
                        } else {
                            // View Saved: list SD card infographics
                            currentState = STATE_GALLERY_SAVED;
                            std::vector<String> savedInfographics;
                            listInfographicsOnSD(savedInfographics);
                            drawInfographicsOnSD(savedInfographics);
                        }
                        break;
                    }
                }
                
                // Back button
                int backBtnY = SCREEN_HEIGHT - 70;
                if (!itemSelected && ty >= backBtnY && ty <= backBtnY + NAV_BTN_HEIGHT && tx >= PADDING_MEDIUM && tx <= PADDING_MEDIUM + NAV_BTN_WIDTH) {
                    currentState = STATE_MENU;
                    drawMenu();
                }
            }
            
            // ================================================================
            // STATE: GALLERY_SUMMARIES (Select summary to generate infographic)
            // ================================================================
            else if (currentState == STATE_GALLERY_SUMMARIES) {
                int startY = STATUS_BAR_HEIGHT + 70;
                bool itemSelected = false;
                
                for (size_t i = 0; i < recentSummaries.size(); i++) {
                    int y = startY + i * (LIST_ITEM_HEIGHT + PADDING_SMALL);
                    if (ty >= y && ty <= y + LIST_ITEM_HEIGHT && tx >= PADDING_MEDIUM && tx <= SCREEN_WIDTH - PADDING_MEDIUM) {
                        itemSelected = true;
                        
                        SummaryFile& selected = recentSummaries[i];
                        if (selected.id.isEmpty()) {
                            drawError("Invalid summary ID");
                            currentState = STATE_ERROR;
                        } else {
                            drawProcessing("Triggering infographic generation...");
                            
                            if (triggerGenerateInfographic(selected.id)) {
                                pendingSummaryIdForInfographic = selected.id;
                                infographicGenerationStartTime = millis();
                                drawMessage("Infographic Generating", 
                                    "Cloud processing started. Check back in 30-40 seconds to view.");
                                currentState = STATE_PROCESSING;
                            } else {
                                drawError("Failed to trigger infographic generation.");
                                currentState = STATE_ERROR;
                            }
                        }
                        break;
                    }
                }
                
                // Back button
                int backBtnY = SCREEN_HEIGHT - 70;
                if (!itemSelected && ty >= backBtnY && ty <= backBtnY + NAV_BTN_HEIGHT && tx >= PADDING_MEDIUM && tx <= PADDING_MEDIUM + NAV_BTN_WIDTH) {
                    currentState = STATE_GALLERY;
                    std::vector<String> options;
                    options.push_back("Create Infographic");
                    options.push_back("View Saved");
                    drawList("Gallery Menu", options);
                }
            }
            
            // ================================================================
            // STATE: GALLERY_SAVED (View saved infographics on SD - Grid Layout)
            // ================================================================
            else if (currentState == STATE_GALLERY_SAVED) {
                // Grid layout matching drawInfographicsOnSD
                int gridStartY = STATUS_BAR_HEIGHT + 55;
                int cellWidth = (SCREEN_WIDTH - 4 * PADDING_SMALL) / 3;
                int cellHeight = 180;
                int maxRows = (SCREEN_HEIGHT - gridStartY - 80) / cellHeight;
                int maxItems = maxRows * 3;
                bool itemSelected = false;
                
                std::vector<String> savedInfographics;
                listInfographicsOnSD(savedInfographics);
                
                // Grid-based touch detection
                for (size_t i = 0; i < savedInfographics.size() && (int)i < maxItems; i++) {
                    int col = i % 3;
                    int row = i / 3;
                    int cellX = PADDING_SMALL + col * (cellWidth + PADDING_SMALL);
                    int cellY = gridStartY + row * cellHeight;
                    
                    // Check if touch is within this grid cell
                    if (tx >= cellX && tx <= cellX + cellWidth && ty >= cellY && ty <= cellY + cellHeight) {
                        itemSelected = true;
                        selectedImageIndex = i;  // Track index for back navigation
                        
                        String filename = savedInfographics[i];
                        String localPath = "/infographics/" + filename;
                        
                        Serial.printf("[MAIN] Selected infographic %d: %s\n", i, filename.c_str());
                        
                        if (SD.exists(localPath)) {
                            drawImageFromFile(filename, localPath);
                            currentState = STATE_VIEW_IMAGE;
                        } else {
                            drawError("File not found on SD card: " + localPath);
                            currentState = STATE_ERROR;
                        }
                        break;
                    }
                }
                
                // Back button
                int backBtnY = SCREEN_HEIGHT - 70;
                if (!itemSelected && ty >= backBtnY && ty <= backBtnY + NAV_BTN_HEIGHT && tx >= PADDING_MEDIUM && tx <= PADDING_MEDIUM + NAV_BTN_WIDTH) {
                    currentState = STATE_GALLERY;
                    std::vector<String> options;
                    options.push_back("Create Infographic");
                    options.push_back("View Saved");
                    drawList("Gallery Menu", options);
                }
            }
            
            // ================================================================
            // STATE: VIEW SUMMARY / VIEW IMAGE / ERROR
            // ================================================================
            else if (currentState == STATE_VIEW_SUMMARY || currentState == STATE_VIEW_IMAGE || currentState == STATE_ERROR) {
                if (currentState == STATE_VIEW_SUMMARY) {
                    // Use ebook reader touch handling
                    if (!handleEbookTouch(tx, ty)) {
                        // User pressed BACK
                        currentState = STATE_LIST_SUMMARIES;
                        drawSummariesList();
                    }
                } else {
                    // Back button for images and errors (updated position)
                    int backBtnY = SCREEN_HEIGHT - 70;
                    if (ty >= backBtnY && ty <= backBtnY + NAV_BTN_HEIGHT && tx >= PADDING_MEDIUM && tx <= PADDING_MEDIUM + NAV_BTN_WIDTH) {
                        if (currentState == STATE_VIEW_IMAGE) {
                            // Determine where to go back based on previous state
                            if (selectedImageIndex >= 0) {
                                currentState = STATE_GALLERY_SAVED;
                                std::vector<String> savedInfographics;
                                listInfographicsOnSD(savedInfographics);
                                drawInfographicsOnSD(savedInfographics);
                            } else {
                                currentState = STATE_GALLERY;
                                std::vector<String> options;
                                options.push_back("Create Infographic");
                                options.push_back("View Saved");
                                drawList("Gallery Menu", options);
                            }
                        } else {
                            currentState = STATE_MENU;
                            drawMenu();
                        }
                    }
                }
            }
            
            // ================================================================
            // STATE: DEEP SLEEP CONFIRMATION
            // ================================================================
            else if (currentState == STATE_DEEP_SLEEP_CONFIRM) {
                int btnY = SCREEN_HEIGHT / 2 + 30;
                int btnSpacing = 30;
                
                // YES button
                int yesBtnX = SCREEN_WIDTH / 2 - MENU_BTN_WIDTH - btnSpacing / 2;
                if (tx >= yesBtnX && tx <= yesBtnX + MENU_BTN_WIDTH && ty >= btnY && ty <= btnY + 80) {
                    drawDeepSleepScreen();
                    delay(500);
                    M5.Power.deepSleep();
                }
                
                // CANCEL button
                int cancelBtnX = SCREEN_WIDTH / 2 + btnSpacing / 2;
                if (tx >= cancelBtnX && tx <= cancelBtnX + MENU_BTN_WIDTH && ty >= btnY && ty <= btnY + 80) {
                    currentState = STATE_MENU;
                    drawMenu();
                }
            }
            
            // Update activity time on any touch
            lastActivityTime = millis();
            idleWarningShown = false; // Reset warning on activity
        }
    }
    
    // Anti-ghosting check
    checkAndRefresh();
    
    // ==========================================================================
    // POWER MANAGEMENT
    // ==========================================================================
    unsigned long now = millis();
    unsigned long idleTime = (lastActivityTime > 0) ? (now - lastActivityTime) : 0;
    
    // Battery monitoring (every 60 seconds)
    if (now - lastBatteryCheckTime > BATTERY_CHECK_INTERVAL) {
        lastBatteryCheckTime = now;
        lastBatteryLevel = M5.Power.getBatteryLevel();
        Serial.printf("[POWER] Battery: %d%%\n", lastBatteryLevel);
        
        // Critical battery - force sleep immediately
        if (lastBatteryLevel <= BATTERY_CRITICAL_THRESHOLD && lastBatteryLevel > 0) {
            Serial.println("[POWER] CRITICAL battery! Entering deep sleep...");
            drawMessage("Low Battery", "Battery critical. Entering sleep mode.");
            delay(2000);
            drawDeepSleepScreen();
            delay(500);
            M5.Power.deepSleep();
        }
        
        // Low battery warning (show on menu only)
        if (lastBatteryLevel <= BATTERY_LOW_THRESHOLD && currentState == STATE_MENU) {
            Serial.printf("[POWER] Low battery warning: %d%%\n", lastBatteryLevel);
        }
    }
    
    // Idle warning (30 seconds before sleep)
    if (lastActivityTime > 0 && idleTime > (AUTO_SLEEP_TIMEOUT - IDLE_WARNING_TIME) && !idleWarningShown) {
        idleWarningShown = true;
        Serial.println("[POWER] Idle warning - sleep in 30 seconds");
        // Show brief message on status bar (don't interrupt current view)
        // Just log for now - visual indicator can be added later
    }
    
    // Auto-sleep after timeout
    if (lastActivityTime > 0 && idleTime > AUTO_SLEEP_TIMEOUT) {
        Serial.println("[POWER] Auto-sleep due to inactivity");
        
        // Proper shutdown sequence
        WiFi.disconnect(true);
        WiFi.mode(WIFI_OFF);
        
        drawDeepSleepScreen();
        delay(500);
        M5.Power.deepSleep();
    }
    
    delay(50);
}