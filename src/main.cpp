/**
 * @file main.cpp
 * @brief MindInk - M5 Paper S3 Cloud Remote & Viewer
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
 * 
 * Architecture:
 * - main.cpp: Setup, loop, power management
 * - state_handlers: Touch event handling per app state
 * - display: Core UI primitives (menu, lists, buttons)
 * - ebook_reader: Text viewing component
 * - image_viewer: Image/gallery display
 * - display_refresh: Anti-ghosting logic
 */

#include <M5Unified.h>
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <SD.h>

// Include modular headers
#include "config.h"
#include "storage.h"
#include "display.h"
#include "state_handlers.h"
#include "ebook_reader.h"
#include "display_refresh.h"
#include "wifi_manager.h"
#include "supabase_client.h"
#include "config_manager.h"
#include "memory_utils.h"

// =============================================================================
// POWER MANAGEMENT STATE
// =============================================================================
static unsigned long lastActivityTime = 0;
static unsigned long lastBatteryCheckTime = 0;
static bool idleWarningShown = false;
static int lastBatteryLevel = 100;

// =============================================================================
// POWER MANAGEMENT FUNCTIONS
// =============================================================================

/**
 * @brief Reset idle timer (called on user activity)
 */
void powerResetIdleTimer() {
    lastActivityTime = millis();
    idleWarningShown = false;
}

/**
 * @brief Check and handle power management (battery, auto-sleep)
 */
void powerUpdateLoop() {
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
}

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
// LOOP
// ============================================================================

void loop() {
    M5.update();
    
    // Handle touch events
    if (M5.Touch.getCount() > 0) {
        auto t = M5.Touch.getDetail(0);
        if (t.wasPressed()) {
            // Dispatch to state handlers
            handleTouchEvent(t.x, t.y);
            
            // Reset idle timer on any touch
            powerResetIdleTimer();
        }
    }
    
    // Anti-ghosting check
    checkAndRefresh();
    
    // Power management
    powerUpdateLoop();
    
    delay(50);
}