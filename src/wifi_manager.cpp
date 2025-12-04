#include "wifi_manager.h"
#include "config.h"

void initWiFi() {
    Serial.println("[WiFi] Initializing WiFi module...");
    M5.Display.fillScreen(TFT_WHITE);
    M5.Display.setTextColor(TFT_BLACK, TFT_WHITE);
    M5.Display.drawString("WiFi: Initializing...", 10, 10);
    
    // Simplified connection logic based on M5Unified example
    WiFi.disconnect();
    WiFi.softAPdisconnect(true);
    WiFi.mode(WIFI_STA);
    
    // Set hostname
    WiFi.setHostname("MindInk-PaperS3");
    
    Serial.printf("[WiFi] Connecting to: %s\n", WIFI_SSID);
    M5.Display.drawString("Connecting...", 10, 30);
    
    // Begin connection
    if (WIFI_SSID && WIFI_PASS) {
         WiFi.begin(WIFI_SSID, WIFI_PASS);
    } else {
         WiFi.begin();
    }
    
    int attempts = 0;
    const int MAX_ATTEMPTS = 40;  // 20 seconds
    
    while (WiFi.status() != WL_CONNECTED && attempts < MAX_ATTEMPTS) {
        delay(500);
        attempts++;
        Serial.printf("[WiFi] Status: %d, Attempt: %d\n", WiFi.status(), attempts);
        M5.Display.drawString(".", 10 + (attempts % 40) * 5, 30);
    }
    
    Serial.println();
    if(WiFi.status() == WL_CONNECTED) {
        Serial.println("[WiFi] Connected successfully!");
        M5.Display.fillScreen(TFT_WHITE);
        M5.Display.drawString("WiFi Connected!", 10, 10);
        M5.Display.drawString("IP: " + WiFi.localIP().toString(), 10, 30);
        Serial.printf("[WiFi] IP Address: %s\n", WiFi.localIP().toString().c_str());
    } else {
        Serial.printf("[WiFi] Failed to connect. Status: %d\n", WiFi.status());
        M5.Display.fillScreen(TFT_WHITE);
        M5.Display.drawString("WiFi Failed", 10, 10);
        M5.Display.drawString("Operating Offline", 10, 30);
        M5.Display.drawString("Status Code: " + String(WiFi.status()), 10, 50);
    }
    
    delay(2000);
}

bool isWiFiConnected() {
    return WiFi.status() == WL_CONNECTED;
}
