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

// Auto-sleep timer
unsigned long lastActivityTime = 0;

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

    // Initialize storage systems (for caching)
    initStorage();
    
    // Initialize WiFi
    initWiFi();
    
    // Initialize UI
    setupButtons();
    drawMenu();
    
    Serial.println("[SETUP] Complete - Cloud Remote & Viewer Mode");
    Serial.printf("[MEMORY] Free Heap: %d bytes\n", ESP.getFreeHeap());
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
                    drawProcessing("Fetching images...");
                    
                    if (isWiFiConnected() && fetchSupabaseImages(recentImages)) {
                        drawGalleryList();
                    } else {
                        drawError("Failed to fetch images. Check WiFi connection.");
                        currentState = STATE_ERROR;
                    }
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
            // STATE: PROCESSING (Message display - just waiting for OK)
            // ================================================================
            else if (currentState == STATE_PROCESSING) {
                // Any tap returns to menu
                int btnW = 100;
                int btnH = 40;
                int btnX = (w - btnW) / 2;
                int btnY = h - 60;
                
                // Check if OK button area tapped
                if (tx >= btnX && tx <= btnX + btnW && ty >= btnY && ty <= btnY + btnH) {
                    currentState = STATE_MENU;
                    drawMenu();
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
                // Check list items (using new LIST_ITEM_HEIGHT)
                int startY = STATUS_BAR_HEIGHT + 70;
                bool itemSelected = false;
                
                for (size_t i = 0; i < recentImages.size(); i++) {
                    int y = startY + i * (LIST_ITEM_HEIGHT + PADDING_SMALL);
                    if (ty >= y && ty <= y + LIST_ITEM_HEIGHT && tx >= PADDING_MEDIUM && tx <= SCREEN_WIDTH - PADDING_MEDIUM) {
                        itemSelected = true;
                        
                        ImageFile& selected = recentImages[i];
                        bool shown = false;
                        String localPath = "/sd/" + (selected.filename.length() > 0 ? selected.filename : ("gallery_" + selected.id + ".png"));
                        
                        // Try SD cache with download if needed
                        if (storage.activeStorage == STORAGE_SD) {
                            if (!SD.exists(localPath) && isWiFiConnected() && !selected.id.isEmpty()) {
                                drawProcessing("Downloading image...");
                                downloadImageFromSupabase(selected.id, localPath, selected.filename);
                            }
                            if (SD.exists(localPath)) {
                                drawImageFromFile(selected.filename, localPath);
                                currentState = STATE_VIEW_IMAGE;
                                shown = true;
                            }
                        }
                        
                        // If not cached, fetch to buffer
                        if (!shown && isWiFiConnected() && !selected.id.isEmpty()) {
                            drawProcessing("Loading image...");
                            std::vector<uint8_t> buf;
                            if (fetchImageToBuffer(selected.id, buf) && !buf.empty()) {
                                drawImageFromBuffer(selected.filename, buf.data(), buf.size());
                                currentState = STATE_VIEW_IMAGE;
                                shown = true;
                            }
                        }
                        
                        if (!shown) {
                            drawImageMessage("Gallery", "Image not available offline");
                            currentState = STATE_VIEW_IMAGE;
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
                            currentState = STATE_GALLERY;
                            drawGalleryList();
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
        }
    }
    
    // Anti-ghosting check
    checkAndRefresh();
    
    // Auto-sleep after 10 minutes of inactivity
    if (lastActivityTime > 0 && (millis() - lastActivityTime > AUTO_SLEEP_TIMEOUT)) {
        drawDeepSleepScreen();
        delay(500);
        M5.Power.deepSleep();
    }
    
    delay(50);
}