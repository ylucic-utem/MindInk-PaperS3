/**
 * @file ui_common.h
 * @brief Shared UI primitives and constants for MindInk PaperS3
 * 
 * Contains common display functions, button handling, and status bar.
 * Extracted from display.h/cpp for modularity.
 */

#ifndef UI_COMMON_H
#define UI_COMMON_H

#include <M5Unified.h>
#include <vector>
#include <WString.h>

// =============================================================================
// SCREEN DIMENSIONS (PaperS3: 540x960 in portrait rotation 2)
// =============================================================================
constexpr int UI_SCREEN_WIDTH = 540;
constexpr int UI_SCREEN_HEIGHT = 960;

// =============================================================================
// SPACING CONSTANTS
// =============================================================================
constexpr int UI_PADDING_SMALL = 10;
constexpr int UI_PADDING_MEDIUM = 20;
constexpr int UI_PADDING_LARGE = 30;

// =============================================================================
// COMPONENT DIMENSIONS
// =============================================================================
constexpr int UI_STATUS_BAR_HEIGHT = 50;
constexpr int UI_MENU_BTN_WIDTH = 230;
constexpr int UI_MENU_BTN_HEIGHT = 120;
constexpr int UI_MENU_BTN_RADIUS = 10;
constexpr int UI_LIST_ITEM_HEIGHT = 80;
constexpr int UI_LIST_ITEM_RADIUS = 8;
constexpr int UI_NAV_BTN_WIDTH = 140;
constexpr int UI_NAV_BTN_HEIGHT = 50;

// =============================================================================
// TOUCH HANDLING
// =============================================================================
constexpr unsigned long UI_TOUCH_DEBOUNCE_MS = 100;

// =============================================================================
// BUTTON STRUCTURE
// =============================================================================
struct UIButton {
    int x, y, w, h;
    const char* label;
    
    UIButton() : x(0), y(0), w(0), h(0), label(nullptr) {}
    UIButton(int x_, int y_, int w_, int h_, const char* label_) 
        : x(x_), y(y_), w(w_), h(h_), label(label_) {}
    
    bool containsPoint(int px, int py) const {
        return (px >= x && px <= x + w && py >= y && py <= y + h);
    }
};

// =============================================================================
// COMMON UI FUNCTIONS
// =============================================================================

/**
 * @brief Draw status bar with WiFi, battery, and storage indicators
 */
void uiDrawStatusBar();

/**
 * @brief Draw a standard button
 * @param btn Button definition
 */
void uiDrawButton(const UIButton& btn);

/**
 * @brief Draw a button in pressed (inverted) state
 * @param btn Button definition
 */
void uiDrawButtonPressed(const UIButton& btn);

/**
 * @brief Show brief visual feedback on button press
 * @param btn Button definition
 */
void uiShowButtonFeedback(const UIButton& btn);

/**
 * @brief Draw a message dialog with title and body text
 * @param title Dialog title
 * @param msg Dialog message (word-wrapped)
 */
void uiDrawMessage(const String& title, const String& msg);

/**
 * @brief Draw a processing/loading screen
 * @param message Message to display
 */
void uiDrawProcessing(const String& message = "Processing...");

/**
 * @brief Draw an error screen
 * @param message Error message
 */
void uiDrawError(const String& message);

/**
 * @brief Force full display refresh to clear ghosting
 */
void uiForceFullRefresh();

#endif // UI_COMMON_H
