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
extern const char* SUPABASE_URL;
extern const char* SUPABASE_ANON_KEY;
extern const char* SUPABASE_EDGE_TOKEN;

// ============================================================================
// API ENDPOINTS
// ============================================================================
extern const char* EL_API_URL;
extern const char* GEMINI_TEXT_URL;
extern const char* GEMINI_IMAGE_URL;

// ============================================================================
// HARDWARE CONFIG - PAPERS3 SPECIFIC (for SD caching)
// ============================================================================
#define SD_CS   47
#define SD_SCK  39
#define SD_MOSI 38
#define SD_MISO 40

#endif // CONFIG_H

