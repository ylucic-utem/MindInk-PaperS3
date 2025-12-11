/**
 * @file config_manager.cpp
 * @brief Configuration management implementation using ESP32 NVS
 */

#include "config_manager.h"

// Global config instance
AppConfig appConfig;

// NVS namespace for preferences
static Preferences prefs;
static const char* NVS_NAMESPACE = "mindink";

// NVS keys
static const char* KEY_FONT_SIZE = "fontSize";
static const char* KEY_BRIGHTNESS = "brightness";
static const char* KEY_SLEEP_TIMEOUT = "sleepTimeout";
static const char* KEY_LOW_BATTERY = "lowBattery";
static const char* KEY_AUTO_REFRESH = "autoRefresh";
static const char* KEY_WIFI_AUTO = "wifiAuto";
static const char* KEY_LAST_SUMMARY = "lastSummary";
static const char* KEY_LAST_PAGE = "lastPage";

// =============================================================================
// INITIALIZATION
// =============================================================================

void configInit() {
    Serial.println("[CONFIG] Initializing configuration manager...");
    
    // Open NVS namespace (read-write mode)
    prefs.begin(NVS_NAMESPACE, false);
    
    // Load saved values or use defaults
    appConfig.fontSize = prefs.getInt(KEY_FONT_SIZE, CFG_DEFAULT_FONT_SIZE);
    appConfig.brightness = prefs.getInt(KEY_BRIGHTNESS, CFG_DEFAULT_BRIGHTNESS);
    appConfig.sleepTimeout = prefs.getULong(KEY_SLEEP_TIMEOUT, CFG_DEFAULT_SLEEP_TIMEOUT);
    appConfig.lowBatteryThreshold = prefs.getInt(KEY_LOW_BATTERY, CFG_DEFAULT_LOW_BATTERY_THRESHOLD);
    appConfig.autoRefresh = prefs.getBool(KEY_AUTO_REFRESH, CFG_DEFAULT_AUTO_REFRESH);
    appConfig.wifiAutoConnect = prefs.getBool(KEY_WIFI_AUTO, CFG_DEFAULT_WIFI_AUTO_CONNECT);
    appConfig.lastViewedSummaryIndex = prefs.getInt(KEY_LAST_SUMMARY, -1);
    appConfig.lastEbookPage = prefs.getInt(KEY_LAST_PAGE, 0);
    
    Serial.println("[CONFIG] Configuration loaded:");
    Serial.printf("[CONFIG]   Font size: %d\n", appConfig.fontSize);
    Serial.printf("[CONFIG]   Sleep timeout: %lu ms\n", appConfig.sleepTimeout);
    Serial.printf("[CONFIG]   Auto refresh: %s\n", appConfig.autoRefresh ? "yes" : "no");
    
    prefs.end();
}

// =============================================================================
// SAVE / RESET
// =============================================================================

void configSave() {
    Serial.println("[CONFIG] Saving configuration...");
    
    prefs.begin(NVS_NAMESPACE, false);
    
    prefs.putInt(KEY_FONT_SIZE, appConfig.fontSize);
    prefs.putInt(KEY_BRIGHTNESS, appConfig.brightness);
    prefs.putULong(KEY_SLEEP_TIMEOUT, appConfig.sleepTimeout);
    prefs.putInt(KEY_LOW_BATTERY, appConfig.lowBatteryThreshold);
    prefs.putBool(KEY_AUTO_REFRESH, appConfig.autoRefresh);
    prefs.putBool(KEY_WIFI_AUTO, appConfig.wifiAutoConnect);
    prefs.putInt(KEY_LAST_SUMMARY, appConfig.lastViewedSummaryIndex);
    prefs.putInt(KEY_LAST_PAGE, appConfig.lastEbookPage);
    
    prefs.end();
    Serial.println("[CONFIG] Configuration saved.");
}

void configReset() {
    Serial.println("[CONFIG] Resetting configuration to defaults...");
    
    prefs.begin(NVS_NAMESPACE, false);
    prefs.clear();
    prefs.end();
    
    // Reset to defaults
    appConfig = AppConfig();
    
    Serial.println("[CONFIG] Configuration reset complete.");
}

// =============================================================================
// GETTERS / SETTERS
// =============================================================================

const AppConfig& configGet() {
    return appConfig;
}

void configSetFontSize(int size) {
    if (size >= 0 && size <= 3) {
        appConfig.fontSize = size;
        
        // Save immediately
        prefs.begin(NVS_NAMESPACE, false);
        prefs.putInt(KEY_FONT_SIZE, size);
        prefs.end();
        
        Serial.printf("[CONFIG] Font size set to %d\n", size);
    }
}

void configSetSleepTimeout(unsigned long timeout) {
    appConfig.sleepTimeout = timeout;
    
    prefs.begin(NVS_NAMESPACE, false);
    prefs.putULong(KEY_SLEEP_TIMEOUT, timeout);
    prefs.end();
    
    Serial.printf("[CONFIG] Sleep timeout set to %lu ms\n", timeout);
}

void configSetLastSummary(int index) {
    appConfig.lastViewedSummaryIndex = index;
    
    prefs.begin(NVS_NAMESPACE, false);
    prefs.putInt(KEY_LAST_SUMMARY, index);
    prefs.end();
}

void configSetLastPage(int page) {
    appConfig.lastEbookPage = page;
    
    prefs.begin(NVS_NAMESPACE, false);
    prefs.putInt(KEY_LAST_PAGE, page);
    prefs.end();
}
