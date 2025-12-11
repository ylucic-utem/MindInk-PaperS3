/**
 * @file display_refresh.h
 * @brief E-paper display refresh and anti-ghosting for MindInk PaperS3
 * 
 * Extracted from display.cpp for modularity.
 * Handles partial/full refresh logic and diagnostics.
 */

#ifndef DISPLAY_REFRESH_H
#define DISPLAY_REFRESH_H

#include <WString.h>

// =============================================================================
// ANTI-GHOSTING CONSTANTS
// =============================================================================

// Force full refresh every N partial updates (prevents ghost buildup)
constexpr int PARTIAL_REFRESH_THRESHOLD = 5;

// =============================================================================
// REFRESH FUNCTIONS
// =============================================================================

/**
 * @brief Force a full display refresh to clear ghosting
 */
void forceFullRefresh();

/**
 * @brief Increment partial refresh counter
 */
void incrementPartialRefresh();

/**
 * @brief Check if refresh is needed and perform if necessary
 */
void checkAndRefresh();

/**
 * @brief Request full refresh before next draw operation
 */
void requestFullRefresh();

// =============================================================================
// DIAGNOSTICS
// =============================================================================

/**
 * @brief Draw diagnostics screen with system info
 */
void drawDiagnosticsScreen();

#endif // DISPLAY_REFRESH_H
