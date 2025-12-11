/**
 * @file state_handlers.h
 * @brief State machine handlers for MindInk PaperS3
 * 
 * Extracted from main.cpp to modularize state handling logic.
 * Each state has its own handler function for touch events.
 */

#ifndef STATE_HANDLERS_H
#define STATE_HANDLERS_H

#include <vector>
#include <WString.h>
#include "storage.h"

// =============================================================================
// GLOBAL STATE (shared with main.cpp)
// =============================================================================
extern std::vector<AudioFile> recentAudio;
extern std::vector<SummaryFile> recentSummaries;
extern std::vector<ImageFile> recentImages;

extern int selectedAudioIndex;
extern int selectedSummaryIndex;
extern int selectedImageIndex;

extern String pendingSummaryIdForInfographic;
extern unsigned long infographicGenerationStartTime;

// =============================================================================
// MAIN TOUCH DISPATCHER
// =============================================================================

/**
 * @brief Main touch event dispatcher - routes to appropriate state handler
 * @param tx Touch X coordinate
 * @param ty Touch Y coordinate
 */
void handleTouchEvent(int tx, int ty);

// =============================================================================
// INDIVIDUAL STATE HANDLERS
// =============================================================================

/**
 * @brief Handle touch events in main menu state
 */
void handleMenuState(int tx, int ty);

/**
 * @brief Handle touch events in audio file list state
 */
void handleListAudioState(int tx, int ty);

/**
 * @brief Handle touch events in summaries list state
 */
void handleListSummariesState(int tx, int ty);

/**
 * @brief Handle touch events in processing/waiting state
 */
void handleProcessingState(int tx, int ty);

/**
 * @brief Handle touch events in gallery menu state
 */
void handleGalleryState(int tx, int ty);

/**
 * @brief Handle touch events in gallery summaries selection state
 */
void handleGallerySummariesState(int tx, int ty);

/**
 * @brief Handle touch events in saved infographics gallery state
 */
void handleGallerySavedState(int tx, int ty);

/**
 * @brief Handle touch events in view states (summary, image, error)
 */
void handleViewState(int tx, int ty);

/**
 * @brief Handle touch events in deep sleep confirmation state
 */
void handleDeepSleepConfirmState(int tx, int ty);

// =============================================================================
// HELPER FUNCTIONS
// =============================================================================

/**
 * @brief Draw audio list with status icons
 */
void drawAudioListWithStatus();

/**
 * @brief Draw summaries list
 */
void drawSummariesList();

/**
 * @brief Draw gallery list
 */
void drawGalleryList();

/**
 * @brief Check if touch is within back button area
 * @return true if back button was pressed
 */
bool checkBackButton(int tx, int ty);

/**
 * @brief Check if touch is within a list item
 * @param tx Touch X
 * @param ty Touch Y
 * @param itemIndex Index of the item to check
 * @param itemCount Total number of items
 * @return true if the item was touched
 */
bool checkListItemTouch(int tx, int ty, int itemIndex, int itemCount);

#endif // STATE_HANDLERS_H
