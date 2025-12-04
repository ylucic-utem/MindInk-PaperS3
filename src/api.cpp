#include "api.h"
#include "config.h"
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <SD.h>

// ============================================================================
// HELPER FUNCTIONS
// ============================================================================

#include "wifi_manager.h"

String getHttpCodeDescription(int code) {
    switch (code) {
        case 200: return "OK";
        case 201: return "Created";
        case 400: return "Bad Request";
        case 401: return "Unauthorized";
        case 403: return "Forbidden";
        case 404: return "Not Found";
        case 429: return "Too Many Requests";
        case 500: return "Internal Server Error";
        case 503: return "Service Unavailable";
        default: return "Unknown Error";
    }
}

// ============================================================================
// ELEVEN LABS TRANSCRIPTION
// ============================================================================

TranscriptionResult transcribeAudio(const String& audioFilePath) {
    TranscriptionResult result;
    
    Serial.printf("[API] Transcribing audio: %s\n", audioFilePath.c_str());
    
    // Check WiFi connection
    if (!isWiFiConnected()) {
        result.error = "WiFi not connected";
        result.httpCode = 0;
        return result;
    }
    
    // Check if audio file exists
    File audioFile;
    if (!SD.exists(audioFilePath)) {
        result.error = "Audio file not found: " + audioFilePath;
        return result;
    }
    
    // Read audio file
    audioFile = SD.open(audioFilePath);
    if (!audioFile) {
        result.error = "Cannot open audio file";
        return result;
    }
    
    size_t fileSize = audioFile.size();
    Serial.printf("[API] Audio file size: %d bytes\n", fileSize);
    
    // Create HTTPClient with HTTPS
    HTTPClient http;
    WiFiClientSecure client;
    client.setInsecure();  // For testing - use proper certificates in production
    
    // Construct request
    String url = String(EL_API_URL);
    
    http.begin(client, url);
    http.addHeader("xi-api-key", ELEVEN_LABS_API_KEY);
    http.addHeader("Content-Type", "audio/wav");
    
    // Send audio data
    Serial.println("[API] Sending audio to Eleven Labs...");
    
    // Read file in chunks to avoid memory issues
    uint8_t buffer[1024];
    int bytesRead = 0;
    
    for (int attempt = 0; attempt < API_MAX_RETRIES; attempt++) {
        if (attempt > 0) {
            Serial.printf("[API] Retry attempt %d/%d...\n", attempt + 1, API_MAX_RETRIES);
            delay(API_RETRY_DELAY_MS);
            audioFile.seek(0);  // Reset file pointer
        }
        
        // Re-establish HTTP connection for each retry
        http.begin(client, url);
        http.addHeader("xi-api-key", ELEVEN_LABS_API_KEY);
        http.addHeader("Content-Type", "audio/wav");
        
        // TODO: Implement proper streaming upload with chunked transfer
        // For now, we'll read entire file into memory (not ideal for large files)
        
        uint8_t* fileBuffer = (uint8_t*)malloc(fileSize);
        if (!fileBuffer) {
            result.error = "Insufficient memory to read file";
            audioFile.close();
            return result;
        }
        
        size_t bytesRead = audioFile.readBytes((char*)fileBuffer, fileSize);
        
        int httpCode = http.POST(fileBuffer, fileSize);
        result.httpCode = httpCode;
        
        free(fileBuffer);
        
        if (httpCode > 0) {
            Serial.printf("[API] HTTP Response Code: %d (%s)\n", httpCode, getHttpCodeDescription(httpCode).c_str());
            
            if (httpCode == 200 || httpCode == 201) {
                // Parse JSON response
                String payload = http.getString();
                Serial.printf("[API] Response: %s\n", payload.c_str());
                
                DynamicJsonDocument doc(1024);
                DeserializationError error = deserializeJson(doc, payload);
                
                if (!error) {
                    if (doc.containsKey("text")) {
                        result.text = doc["text"].as<String>();
                        result.success = true;
                        Serial.println("[API] Transcription successful");
                        break;  // Success - exit retry loop
                    } else {
                        result.error = "No text in response";
                    }
                } else {
                    result.error = "JSON parse error: " + String(error.c_str());
                }
            } else if (httpCode == 429) {
                result.error = "Rate limit exceeded - try again later";
                // Continue retry
            } else if (httpCode == 401 || httpCode == 403) {
                result.error = "Authentication failed - check API key";
                break;  // Don't retry authentication errors
            } else {
                result.error = "HTTP " + String(httpCode) + ": " + getHttpCodeDescription(httpCode);
            }
        } else {
            result.error = "Connection failed: " + http.errorToString(httpCode);
        }
        
        http.end();
    }
    
    audioFile.close();
    http.end();
    
    return result;
}

// ============================================================================
// GEMINI TEXT SUMMARIZATION
// ============================================================================

SummaryResult summarizeText(const String& text, int maxLength) {
    SummaryResult result;
    
    Serial.printf("[API] Summarizing text (%d chars)...\n", text.length());
    
    // Check WiFi connection
    if (!isWiFiConnected()) {
        result.error = "WiFi not connected";
        result.httpCode = 0;
        return result;
    }
    
    // Check text length
    if (text.length() == 0) {
        result.error = "Empty text provided";
        return result;
    }
    
    if (text.length() > 30000) {
        result.error = "Text too long (max 30000 characters)";
        return result;
    }
    
    HTTPClient http;
    WiFiClientSecure client;
    client.setInsecure();
    
    // Construct URL with API key
    String url = String(GEMINI_TEXT_URL) + String(GEMINI_API_KEY);
    
    // Create JSON request
    DynamicJsonDocument doc(2048);
    
    // Build the prompt
    String prompt = "Summarize the following text concisely";
    if (maxLength > 0) {
        prompt += " in " + String(maxLength) + " characters or less";
    }
    prompt += ":\n\n" + text;
    
    // Gemini API format
    doc["contents"][0]["parts"][0]["text"] = prompt;
    
    String jsonString;
    serializeJson(doc, jsonString);
    
    Serial.println("[API] Sending request to Gemini...");
    
    for (int attempt = 0; attempt < API_MAX_RETRIES; attempt++) {
        if (attempt > 0) {
            Serial.printf("[API] Retry attempt %d/%d...\n", attempt + 1, API_MAX_RETRIES);
            delay(API_RETRY_DELAY_MS);
        }
        
        http.begin(client, url);
        http.addHeader("Content-Type", "application/json");
        http.setTimeout(API_TIMEOUT_MS);
        
        int httpCode = http.POST(jsonString);
        result.httpCode = httpCode;
        
        if (httpCode > 0) {
            Serial.printf("[API] HTTP Response Code: %d (%s)\n", httpCode, getHttpCodeDescription(httpCode).c_str());
            
            if (httpCode == 200) {
                String payload = http.getString();
                
                DynamicJsonDocument responseDoc(4096);
                DeserializationError error = deserializeJson(responseDoc, payload);
                
                if (!error) {
                    // Extract text from Gemini response
                    if (responseDoc.containsKey("candidates") && 
                        responseDoc["candidates"].size() > 0 &&
                        responseDoc["candidates"][0].containsKey("content") &&
                        responseDoc["candidates"][0]["content"].containsKey("parts") &&
                        responseDoc["candidates"][0]["content"]["parts"].size() > 0) {
                        
                        result.text = responseDoc["candidates"][0]["content"]["parts"][0]["text"].as<String>();
                        result.success = true;
                        Serial.println("[API] Summarization successful");
                        break;
                    } else {
                        result.error = "Unexpected response format";
                    }
                } else {
                    result.error = "JSON parse error: " + String(error.c_str());
                }
            } else if (httpCode == 429) {
                result.error = "Rate limit exceeded";
            } else if (httpCode == 401 || httpCode == 403) {
                result.error = "Authentication failed - check API key";
                break;
            } else {
                result.error = "HTTP " + String(httpCode) + ": " + getHttpCodeDescription(httpCode);
            }
        } else {
            result.error = "Connection failed: " + http.errorToString(httpCode);
        }
        
        http.end();
    }
    
    http.end();
    
    return result;
}

// ============================================================================
// GEMINI INFOGRAPHIC GENERATION
// ============================================================================

InfographicResult generateInfographic(const String& summaryText, const String& style) {
    InfographicResult result;
    
    Serial.printf("[API] Generating infographic for summary (%d chars)...\n", summaryText.length());
    
    // Check WiFi connection
    if (!isWiFiConnected()) {
        result.error = "WiFi not connected";
        result.httpCode = 0;
        return result;
    }
    
    if (summaryText.length() == 0) {
        result.error = "Empty summary provided";
        return result;
    }
    
    HTTPClient http;
    WiFiClientSecure client;
    client.setInsecure();
    
    // Construct URL with API key
    String url = String(GEMINI_IMAGE_URL) + String(GEMINI_API_KEY);
    
    // Create JSON request for image generation
    DynamicJsonDocument doc(2048);
    
    String prompt = "Create a clean, simple infographic visual representation of the following summary:\n\n";
    prompt += summaryText;
    
    if (style.length() > 0) {
        prompt += "\n\nStyle: " + style;
    }
    
    prompt += "\n\nMake it suitable for an e-ink display. Use simple shapes, text, and icons.";
    
    doc["contents"][0]["parts"][0]["text"] = prompt;
    
    String jsonString;
    serializeJson(doc, jsonString);
    
    Serial.println("[API] Sending image generation request to Gemini...");
    
    for (int attempt = 0; attempt < API_MAX_RETRIES; attempt++) {
        if (attempt > 0) {
            Serial.printf("[API] Retry attempt %d/%d...\n", attempt + 1, API_MAX_RETRIES);
            delay(API_RETRY_DELAY_MS);
        }
        
        http.begin(client, url);
        http.addHeader("Content-Type", "application/json");
        http.setTimeout(API_TIMEOUT_MS);
        
        int httpCode = http.POST(jsonString);
        result.httpCode = httpCode;
        
        if (httpCode > 0) {
            Serial.printf("[API] HTTP Response Code: %d (%s)\n", httpCode, getHttpCodeDescription(httpCode).c_str());
            
            if (httpCode == 200) {
                String payload = http.getString();
                
                DynamicJsonDocument responseDoc(8192);
                DeserializationError error = deserializeJson(responseDoc, payload);
                
                if (!error) {
                    // Extract image data from Gemini response
                    if (responseDoc.containsKey("candidates") && 
                        responseDoc["candidates"].size() > 0 &&
                        responseDoc["candidates"][0].containsKey("content")) {
                        
                        // TODO: Extract image from response
                        // Gemini returns image data as base64
                        
                        result.success = true;
                        Serial.println("[API] Infographic generation successful");
                        break;
                    } else {
                        result.error = "Unexpected response format";
                    }
                } else {
                    result.error = "JSON parse error: " + String(error.c_str());
                }
            } else if (httpCode == 429) {
                result.error = "Rate limit exceeded";
            } else if (httpCode == 401 || httpCode == 403) {
                result.error = "Authentication failed - check API key";
                break;
            } else {
                result.error = "HTTP " + String(httpCode) + ": " + getHttpCodeDescription(httpCode);
            }
        } else {
            result.error = "Connection failed: " + http.errorToString(httpCode);
        }
        
        http.end();
    }
    
    http.end();
    
    return result;
}
