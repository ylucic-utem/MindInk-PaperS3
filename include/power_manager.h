/**
 * @file power_manager.h
 * @brief Centralized power management for MindInk PaperS3
 * 
 * Handles deep sleep, auto-sleep, battery monitoring, and wake resume.
 */

#ifndef POWER_MANAGER_H
#define POWER_MANAGER_H

#include <M5Unified.h>

// =============================================================================
// POWER CONFIGURATION CONSTANTS
// =============================================================================

// Auto-sleep timeout (3 minutes default)
constexpr unsigned long PM_AUTO_SLEEP_TIMEOUT = 180000;

// Idle warning before sleep (30 seconds)
constexpr unsigned long PM_IDLE_WARNING_TIME = 30000;

// Battery check interval (60 seconds)
constexpr unsigned long PM_BATTERY_CHECK_INTERVAL = 60000;

// Battery thresholds (percentage)
constexpr int PM_BATTERY_LOW = 15;
constexpr int PM_BATTERY_CRITICAL = 5;

// Anti-ghosting refresh interval (5 minutes)
constexpr unsigned long PM_GHOST_REFRESH_INTERVAL = 300000;

// =============================================================================
// POWER MANAGER STATE
// =============================================================================

struct PowerManagerState {
    unsigned long lastActivityTime;
    unsigned long lastBatteryCheckTime;
    unsigned long lastGhostRefreshTime;
    int lastBatteryLevel;
    bool idleWarningShown;
    bool isInitialized;
    
    PowerManagerState() 
        : lastActivityTime(0)
        , lastBatteryCheckTime(0)
        , lastGhostRefreshTime(0)
        , lastBatteryLevel(100)
        , idleWarningShown(false)
        , isInitialized(false) {}
};

// Global power manager state
extern PowerManagerState pmState;

// =============================================================================
// POWER MANAGER FUNCTIONS
// =============================================================================

/**
 * @brief Initialize power manager
 */
void pmInit();

/**
 * @brief Update activity timer (call on user interaction)
 */
void pmResetActivityTimer();

/**
 * @brief Check and update power state (call in main loop)
 * @return true if device should continue running, false if entering sleep
 */
bool pmUpdate();

/**
 * @brief Get current battery level (cached)
 * @return Battery percentage 0-100
 */
int pmGetBatteryLevel();

/**
 * @brief Check if battery is low
 * @return true if below low threshold
 */
bool pmIsBatteryLow();

/**
 * @brief Enter deep sleep mode with proper shutdown sequence
 */
void pmEnterDeepSleep();

/**
 * @brief Perform anti-ghosting refresh if needed
 * @return true if refresh was performed
 */
bool pmCheckGhosting();

#endif // POWER_MANAGER_H
