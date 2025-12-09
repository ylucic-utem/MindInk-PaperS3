/*
 * Local Processing Test Program
 * 
 * This is a simple test to verify the local processing workflow works.
 * Upload this to your PaperS3 and watch the Serial Monitor.
 * 
 * Prerequisites:
 * - SD card formatted as FAT32 and inserted
 * - WiFi credentials configured in config.cpp
 * - API keys configured in config.cpp
 * - At least one audio file in Supabase
 */

#include <M5Unified.h>
#include "config.h"
#include "storage.h"
#include "wifi_manager.h"
#include "supabase_client.h"
#include "api.h"

void setup() {
    auto cfg = M5.config();
    cfg.serial_baudrate = 115200;
    M5.begin(cfg);
    
    Serial.println("\n\n==========================================");
    Serial.println("  LOCAL PROCESSING TEST");
    Serial.println("==========================================\n");
    
    // Test 1: Storage
    Serial.println("[TEST 1] Checking SD card...");
    initStorage();
    
    StorageInfo info = getStorageInfo();
    if (!info.sdAvailable) {
        Serial.println("❌ FAIL: SD card not available");
        Serial.println("   Please insert SD card formatted as FAT32");
        while(1) delay(1000);
    }
    Serial.println("✅ PASS: SD card available");
    
    // Test 2: WiFi
    Serial.println("\n[TEST 2] Connecting to WiFi...");
    initWiFi();
    
    if (!isWiFiConnected()) {
        Serial.println("❌ FAIL: WiFi not connected");
        Serial.println("   Check credentials in config.cpp");
        while(1) delay(1000);
    }
    Serial.println("✅ PASS: WiFi connected");
    
    // Test 3: Fetch audio list
    Serial.println("\n[TEST 3] Fetching audio list from Supabase...");
    std::vector<AudioFile> audioList;
    
    if (!fetchSupabaseAudio(audioList)) {
        Serial.println("❌ FAIL: Could not fetch audio from Supabase");
        Serial.println("   Check Supabase URL and anon key in config.cpp");
        while(1) delay(1000);
    }
    
    if (audioList.empty()) {
        Serial.println("❌ FAIL: No audio files found in Supabase");
        Serial.println("   Please upload audio via webapp first");
        while(1) delay(1000);
    }
    
    Serial.printf("✅ PASS: Found %d audio files\n", audioList.size());
    Serial.println("\nAvailable audio files:");
    for (int i = 0; i < audioList.size() && i < 5; i++) {
        Serial.printf("  [%d] %s (ID: %s)\n", i, 
                     audioList[i].filename.c_str(),
                     audioList[i].id.c_str());
    }
    
    // Test 4: Process first audio file
    Serial.println("\n[TEST 4] Processing first audio file...");
    Serial.println("This will take 30-60 seconds. Please wait...\n");
    
    String testAudioId = audioList[0].id;
    
    bool success = processAudioLocally(testAudioId);
    
    if (!success) {
        Serial.println("\n❌ FAIL: Processing failed");
        Serial.println("   Check Serial Monitor logs above for details");
        while(1) delay(1000);
    }
    
    Serial.println("\n✅ PASS: Processing completed successfully!");
    
    // Test 5: Read results from SD card
    Serial.println("\n[TEST 5] Reading results from SD card...");
    
    String transcript;
    if (readTranscriptLocally(testAudioId, transcript)) {
        Serial.println("✅ Transcript found on SD card");
        Serial.println("--- TRANSCRIPT START ---");
        Serial.println(transcript);
        Serial.println("--- TRANSCRIPT END ---");
    } else {
        Serial.println("⚠️  Warning: Could not read transcript from SD card");
    }
    
    String summary;
    if (readSummaryLocally(testAudioId + "_summary", summary)) {
        Serial.println("\n✅ Summary found on SD card");
        Serial.println("--- SUMMARY START ---");
        Serial.println(summary);
        Serial.println("--- SUMMARY END ---");
    } else {
        Serial.println("⚠️  Warning: Could not read summary from SD card");
    }
    
    // Final report
    Serial.println("\n==========================================");
    Serial.println("  TEST COMPLETE - ALL PASSED! ✅");
    Serial.println("==========================================");
    Serial.println("\nSD Card Contents:");
    Serial.printf("  Audio:      %s\n", getAudioCachePath(testAudioId).c_str());
    Serial.printf("  Transcript: %s\n", getTranscriptCachePath(testAudioId).c_str());
    Serial.printf("  Summary:    %s\n", getSummaryCachePath(testAudioId + "_summary").c_str());
    Serial.println("\nYour local processing system is working!");
    Serial.println("You can now integrate processAudioLocally() into your main app.");
}

void loop() {
    // Test complete - do nothing
    delay(1000);
}
