/**
 * @file power_manager.cpp
 * @brief Implementation of centralized power management for MindInk PaperS3
 */

#include "power_manager.h"
#include "display.h"
#include <WiFi.h>

// Global state instance
PowerManagerState pmState;

// =============================================================================
// INITIALIZATION
// =============================================================================

void pmInit() {
    pmState.lastActivityTime = millis();
    pmState.lastBatteryCheckTime = 0;
    pmState.lastGhostRefreshTime = millis();
    pmState.lastBatteryLevel = M5.Power.getBatteryLevel();
    pmState.idleWarningShown = false;
    pmState.isInitialized = true;
    
    Serial.println("[POWER] Power manager initialized");
    Serial.printf("[POWER] Battery: %d%%\n", pmState.lastBatteryLevel);
}

// =============================================================================
// ACTIVITY TRACKING
// =============================================================================

void pmResetActivityTimer() {
    pmState.lastActivityTime = millis();
    pmState.idleWarningShown = false;
}

// =============================================================================
// BATTERY MONITORING
// =============================================================================

int pmGetBatteryLevel() {
    return pmState.lastBatteryLevel;
}

bool pmIsBatteryLow() {
    return pmState.lastBatteryLevel <= PM_BATTERY_LOW && pmState.lastBatteryLevel > 0;
}

// =============================================================================
// MAIN UPDATE LOOP
// =============================================================================

bool pmUpdate() {
    if (!pmState.isInitialized) {
        pmInit();
    }
    
    unsigned long now = millis();
    unsigned long idleTime = (pmState.lastActivityTime > 0) 
        ? (now - pmState.lastActivityTime) 
        : 0;
    
    // Battery monitoring
    if (now - pmState.lastBatteryCheckTime > PM_BATTERY_CHECK_INTERVAL) {
        pmState.lastBatteryCheckTime = now;
        pmState.lastBatteryLevel = M5.Power.getBatteryLevel();
        Serial.printf("[POWER] Battery: %d%%\n", pmState.lastBatteryLevel);
        
        // Critical battery - force sleep immediately
        if (pmState.lastBatteryLevel <= PM_BATTERY_CRITICAL && pmState.lastBatteryLevel > 0) {
            Serial.println("[POWER] CRITICAL battery! Entering deep sleep...");
            drawMessage("Low Battery", "Battery critical. Entering sleep mode.");
            delay(2000);
            pmEnterDeepSleep();
            return false;
        }
    }
    
    // Idle warning (30 seconds before sleep)
    if (pmState.lastActivityTime > 0 
        && idleTime > (PM_AUTO_SLEEP_TIMEOUT - PM_IDLE_WARNING_TIME) 
        && !pmState.idleWarningShown) {
        pmState.idleWarningShown = true;
        Serial.println("[POWER] Idle warning - sleep in 30 seconds");
    }
    
    // Auto-sleep after timeout
    if (pmState.lastActivityTime > 0 && idleTime > PM_AUTO_SLEEP_TIMEOUT) {
        Serial.println("[POWER] Auto-sleep due to inactivity");
        pmEnterDeepSleep();
        return false;
    }
    
    return true;
}

// =============================================================================
// DEEP SLEEP
// =============================================================================

void pmEnterDeepSleep() {
    Serial.println("[POWER] Entering deep sleep...");
    
    // Proper shutdown sequence
    WiFi.disconnect(true);
    WiFi.mode(WIFI_OFF);
    delay(100);
    
    // Show sleep screen
    drawDeepSleepScreen();
    delay(500);
    
    // Enter deep sleep
    M5.Power.deepSleep();
}

// =============================================================================
// ANTI-GHOSTING
// =============================================================================

bool pmCheckGhosting() {
    unsigned long now = millis();
    if (now - pmState.lastGhostRefreshTime > PM_GHOST_REFRESH_INTERVAL) {
        pmState.lastGhostRefreshTime = now;
        forceFullRefresh();
        return true;
    }
    return false;
}
