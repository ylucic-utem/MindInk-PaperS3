/**
 * @file state_handlers.cpp
 * @brief State machine handlers for MindInk PaperS3
 * 
 * Extracted from main.cpp to modularize state handling logic.
 */

#include "state_handlers.h"
#include "display.h"
#include "ebook_reader.h"
#include "image_viewer.h"
#include "storage.h"
#include "supabase_client.h"
#include "wifi_manager.h"
#include "config_manager.h"
#include <M5Unified.h>
#include <SD.h>

// =============================================================================
// GLOBAL STATE DEFINITIONS
// =============================================================================
std::vector<AudioFile> recentAudio;
std::vector<SummaryFile> recentSummaries;
std::vector<ImageFile> recentImages;

int selectedAudioIndex = -1;
int selectedSummaryIndex = -1;
int selectedImageIndex = -1;

String pendingSummaryIdForInfographic = "";
unsigned long infographicGenerationStartTime = 0;

// =============================================================================
// HELPER FUNCTIONS
// =============================================================================

bool checkBackButton(int tx, int ty) {
    int backBtnY = SCREEN_HEIGHT - 70;
    return (ty >= backBtnY && ty <= backBtnY + NAV_BTN_HEIGHT && 
            tx >= PADDING_MEDIUM && tx <= PADDING_MEDIUM + NAV_BTN_WIDTH);
}

bool checkListItemTouch(int tx, int ty, int itemIndex, int /*itemCount*/) {
    int startY = STATUS_BAR_HEIGHT + 70;
    int y = startY + itemIndex * (LIST_ITEM_HEIGHT + PADDING_SMALL);
    return (ty >= y && ty <= y + LIST_ITEM_HEIGHT && 
            tx >= PADDING_MEDIUM && tx <= SCREEN_WIDTH - PADDING_MEDIUM);
}

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

void drawGalleryList() {
    std::vector<String> list;
    for (const auto& img : recentImages) {
        String item = img.filename.length() > 0 ? img.filename : "(image)";
        list.push_back(item);
    }
    drawList("Gallery", list.empty() ? std::vector<String>{"(No images found)"} : list);
}

// =============================================================================
// MAIN TOUCH DISPATCHER
// =============================================================================

void handleTouchEvent(int tx, int ty) {
    switch (currentState) {
        case STATE_MENU:
            handleMenuState(tx, ty);
            break;
        case STATE_LIST_AUDIO:
            handleListAudioState(tx, ty);
            break;
        case STATE_LIST_SUMMARIES:
            handleListSummariesState(tx, ty);
            break;
        case STATE_PROCESSING:
            handleProcessingState(tx, ty);
            break;
        case STATE_GALLERY:
            handleGalleryState(tx, ty);
            break;
        case STATE_GALLERY_SUMMARIES:
            handleGallerySummariesState(tx, ty);
            break;
        case STATE_GALLERY_SAVED:
            handleGallerySavedState(tx, ty);
            break;
        case STATE_VIEW_SUMMARY:
        case STATE_VIEW_IMAGE:
        case STATE_ERROR:
            handleViewState(tx, ty);
            break;
        case STATE_DEEP_SLEEP_CONFIRM:
            handleDeepSleepConfirmState(tx, ty);
            break;
        default:
            break;
    }
}

// =============================================================================
// STATE: MENU
// =============================================================================

void handleMenuState(int tx, int ty) {
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

// =============================================================================
// STATE: LIST AUDIO
// =============================================================================

void handleListAudioState(int tx, int ty) {
    bool itemSelected = false;
    
    for (size_t i = 0; i < recentAudio.size(); i++) {
        if (checkListItemTouch(tx, ty, i, recentAudio.size())) {
            itemSelected = true;
            
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
    
    // Back button
    if (!itemSelected && checkBackButton(tx, ty)) {
        currentState = STATE_MENU;
        drawMenu();
    }
}

// =============================================================================
// STATE: PROCESSING
// =============================================================================

void handleProcessingState(int tx, int ty) {
    int w = M5.Display.width();
    int h = M5.Display.height();
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

// =============================================================================
// STATE: LIST SUMMARIES
// =============================================================================

void handleListSummariesState(int tx, int ty) {
    bool itemSelected = false;
    
    for (size_t i = 0; i < recentSummaries.size(); i++) {
        if (checkListItemTouch(tx, ty, i, recentSummaries.size())) {
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
    
    // Back button
    if (!itemSelected && checkBackButton(tx, ty)) {
        currentState = STATE_MENU;
        drawMenu();
    }
}

// =============================================================================
// STATE: GALLERY
// =============================================================================

void handleGalleryState(int tx, int ty) {
    bool itemSelected = false;
    
    // Gallery menu has 2 options: Create Infographic, View Saved
    for (size_t i = 0; i < 2; i++) {
        if (checkListItemTouch(tx, ty, i, 2)) {
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
    if (!itemSelected && checkBackButton(tx, ty)) {
        currentState = STATE_MENU;
        drawMenu();
    }
}

// =============================================================================
// STATE: GALLERY SUMMARIES
// =============================================================================

void handleGallerySummariesState(int tx, int ty) {
    bool itemSelected = false;
    
    for (size_t i = 0; i < recentSummaries.size(); i++) {
        if (checkListItemTouch(tx, ty, i, recentSummaries.size())) {
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
    if (!itemSelected && checkBackButton(tx, ty)) {
        currentState = STATE_GALLERY;
        std::vector<String> options;
        options.push_back("Create Infographic");
        options.push_back("View Saved");
        drawList("Gallery Menu", options);
    }
}

// =============================================================================
// STATE: GALLERY SAVED
// =============================================================================

void handleGallerySavedState(int tx, int ty) {
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
            
            Serial.printf("[STATE] Selected infographic %d: %s\n", i, filename.c_str());
            
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
    if (!itemSelected && checkBackButton(tx, ty)) {
        currentState = STATE_GALLERY;
        std::vector<String> options;
        options.push_back("Create Infographic");
        options.push_back("View Saved");
        drawList("Gallery Menu", options);
    }
}

// =============================================================================
// STATE: VIEW (Summary, Image, Error)
// =============================================================================

void handleViewState(int tx, int ty) {
    if (currentState == STATE_VIEW_SUMMARY) {
        // Use ebook reader touch handling
        if (!handleEbookTouch(tx, ty)) {
            // User pressed BACK
            currentState = STATE_LIST_SUMMARIES;
            drawSummariesList();
        }
    } else {
        // Back button for images and errors
        if (checkBackButton(tx, ty)) {
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

// =============================================================================
// STATE: DEEP SLEEP CONFIRMATION
// =============================================================================

void handleDeepSleepConfirmState(int tx, int ty) {
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
