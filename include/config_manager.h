/**
 * @file config_manager.h
 * @brief Configuration management for MindInk PaperS3
 * 
 * Persistent user preferences using ESP32 NVS (Non-Volatile Storage).
 */

#ifndef CONFIG_MANAGER_H
#define CONFIG_MANAGER_H

#include <Arduino.h>
#include <Preferences.h>

// =============================================================================
// DEFAULT CONFIGURATION VALUES
// =============================================================================

// Display settings
constexpr int CFG_DEFAULT_FONT_SIZE = 1;        // 0=16pt, 1=20pt, 2=24pt, 3=28pt
constexpr int CFG_DEFAULT_BRIGHTNESS = 100;     // 0-100%

// Power settings
constexpr unsigned long CFG_DEFAULT_SLEEP_TIMEOUT = 180000;  // 3 minutes
constexpr int CFG_DEFAULT_LOW_BATTERY_THRESHOLD = 15;        // 15%

// Behavior settings
constexpr bool CFG_DEFAULT_AUTO_REFRESH = true;
constexpr bool CFG_DEFAULT_WIFI_AUTO_CONNECT = true;

// =============================================================================
// CONFIGURATION STRUCTURE
// =============================================================================

struct AppConfig {
    // Display
    int fontSize;
    int brightness;
    
    // Power
    unsigned long sleepTimeout;
    int lowBatteryThreshold;
    
    // Behavior
    bool autoRefresh;
    bool wifiAutoConnect;
    
    // Last state
    int lastViewedSummaryIndex;
    int lastEbookPage;
    
    AppConfig() 
        : fontSize(CFG_DEFAULT_FONT_SIZE)
        , brightness(CFG_DEFAULT_BRIGHTNESS)
        , sleepTimeout(CFG_DEFAULT_SLEEP_TIMEOUT)
        , lowBatteryThreshold(CFG_DEFAULT_LOW_BATTERY_THRESHOLD)
        , autoRefresh(CFG_DEFAULT_AUTO_REFRESH)
        , wifiAutoConnect(CFG_DEFAULT_WIFI_AUTO_CONNECT)
        , lastViewedSummaryIndex(-1)
        , lastEbookPage(0) {}
};

// Global config instance
extern AppConfig appConfig;

// =============================================================================
// CONFIGURATION FUNCTIONS
// =============================================================================

/**
 * @brief Initialize configuration system, load from NVS
 */
void configInit();

/**
 * @brief Save current configuration to NVS
 */
void configSave();

/**
 * @brief Reset configuration to defaults
 */
void configReset();

/**
 * @brief Get current configuration (read-only)
 * @return Reference to AppConfig
 */
const AppConfig& configGet();

/**
 * @brief Set font size preference
 * @param size Font size level (0-3)
 */
void configSetFontSize(int size);

/**
 * @brief Set sleep timeout
 * @param timeout Timeout in milliseconds
 */
void configSetSleepTimeout(unsigned long timeout);

/**
 * @brief Save last viewed summary index for resume
 * @param index Summary index
 */
void configSetLastSummary(int index);

/**
 * @brief Save last ebook page for resume
 * @param page Page number
 */
void configSetLastPage(int page);

#endif // CONFIG_MANAGER_H
