#ifndef CONFIG_H
#define CONFIG_H

// ============================================================================
// WIFI CONFIGURATION
// ============================================================================
extern const char* WIFI_SSID;
extern const char* WIFI_PASS;

// ============================================================================
// API KEYS
// ============================================================================
extern const char* ELEVEN_LABS_API_KEY;
extern const char* GEMINI_API_KEY;

// ============================================================================
// API ENDPOINTS
// ============================================================================
extern const char* EL_API_URL;
extern const char* GEMINI_TEXT_URL;
extern const char* GEMINI_IMAGE_URL;

// ============================================================================
// AUDIO RECORDING CONFIG
// ============================================================================
#define RECORD_SAMPLE_RATE 16000
#define RECORD_BIT_DEPTH   16
#define RX_BUFFER_SIZE     1024
#define MAX_RECORDING_TIME 300000

// ============================================================================
// HARDWARE CONFIG - PAPERS3 SPECIFIC
// ============================================================================
#define SD_CS   47
#define SD_SCK  39
#define SD_MOSI 38
#define SD_MISO 40

#endif // CONFIG_H
