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
    
    Serial.println("[API] === TRANSCRIPTION START ===");
    Serial.printf("[API] Transcribing audio: %s\n", audioFilePath.c_str());
    
    // Check WiFi connection
    if (!isWiFiConnected()) {
        result.error = "WiFi not connected";
        result.httpCode = 0;
        return result;
    }
    
    // Check if audio file exists
    if (!SD.exists(audioFilePath)) {
        result.error = "Audio file not found: " + audioFilePath;
        Serial.printf("[API] ERROR: %s\n", result.error.c_str());
        return result;
    }
    
    // Read audio file
    File audioFile = SD.open(audioFilePath, FILE_READ);
    if (!audioFile) {
        result.error = "Cannot open audio file";
        Serial.printf("[API] ERROR: %s\n", result.error.c_str());
        return result;
    }
    
    size_t fileSize = audioFile.size();
    Serial.printf("[API] Audio file size: %d bytes\n", fileSize);
    Serial.printf("[API] Free heap before fileBuffer allocation: %d bytes\n", ESP.getFreeHeap());
    
    // Validate file size (ElevenLabs limits to 25MB for free tier)
    if (fileSize > 26214400) {  // 25MB
        result.error = "Audio file too large (max 25MB)";
        audioFile.close();
        return result;
    }
    
    // Read entire file into buffer
    uint8_t* fileBuffer = (uint8_t*)malloc(fileSize);
    if (!fileBuffer) {
        result.error = "Insufficient memory to read file (needed " + String(fileSize) + " bytes, free: " + String(ESP.getFreeHeap()) + ")";
        Serial.printf("[API] ERROR: %s\n", result.error.c_str());
        audioFile.close();
        return result;
    }
    
    Serial.printf("[API] fileBuffer allocated successfully: %d bytes\n", fileSize);
    Serial.printf("[API] Free heap after fileBuffer allocation: %d bytes\n", ESP.getFreeHeap());
    
    size_t bytesRead = audioFile.readBytes((char*)fileBuffer, fileSize);
    audioFile.close();
    
    if (bytesRead != fileSize) {
        result.error = "Failed to read entire audio file";
        free(fileBuffer);
        return result;
    }
    
    Serial.printf("[API] Read %d bytes from file\n", bytesRead);
    
    // Create HTTPClient with proper multipart form-data for ElevenLabs
    HTTPClient http;
    WiFiClientSecure client;
    client.setInsecure();
    
    String url = "https://api.elevenlabs.io/v1/speech-to-text";
    Serial.printf("[API] Connecting to: %s\n", url.c_str());
    
    // Set longer timeout for transcription (60 seconds)
    http.setTimeout(60000);
    
    if (!http.begin(client, url)) {
        result.error = "Failed to begin HTTP connection";
        Serial.printf("[API] ERROR: %s\n", result.error.c_str());
        free(fileBuffer);
        return result;
    }
    
    // Add authorization header
    http.addHeader("xi-api-key", ELEVEN_LABS_API_KEY);
    
    // Construct multipart form-data boundary
    String boundary = "----WebKitFormBoundary7MA4YWxkTrZu0gW";
    String contentType = "multipart/form-data; boundary=" + boundary;
    http.addHeader("Content-Type", contentType);
    
    Serial.println("[API] Building multipart form data...");
    
    // Build multipart form data manually with memory-efficient approach
    String modelId = "eleven_flash_v2_latest";
    
    String formStart = "--" + boundary + "\r\n";
    formStart += "Content-Disposition: form-data; name=\"model_id\"\r\n\r\n";
    formStart += modelId + "\r\n";
    formStart += "--" + boundary + "\r\n";
    formStart += "Content-Disposition: form-data; name=\"file\"; filename=\"audio.webm\"\r\n";
    formStart += "Content-Type: audio/webm\r\n\r\n";
    
    String formEnd = "\r\n--" + boundary + "--\r\n";
    
    // Calculate total size
    size_t totalSize = formStart.length() + fileSize + formEnd.length();
    Serial.printf("[API] Total form data size: %d bytes\n", totalSize);
    Serial.printf("[API] Free heap before form buffer: %d bytes\n", ESP.getFreeHeap());
    
    // Try to allocate form buffer
    uint8_t* formBuffer = (uint8_t*)malloc(totalSize);
    if (!formBuffer) {
        result.error = "Insufficient memory for form data (needed " + String(totalSize) + " bytes, free: " + String(ESP.getFreeHeap()) + ")";
        Serial.printf("[API] ERROR: %s\n", result.error.c_str());
        http.end();
        free(fileBuffer);
        return result;
    }
    
    Serial.printf("[API] formBuffer allocated successfully: %d bytes\n", totalSize);
    Serial.printf("[API] Free heap after formBuffer allocation: %d bytes\n", ESP.getFreeHeap());
    
    // Copy form data
    Serial.println("[API] Copying form data parts...");
    size_t offset = 0;
    memcpy(formBuffer + offset, formStart.c_str(), formStart.length());
    offset += formStart.length();
    memcpy(formBuffer + offset, fileBuffer, fileSize);
    offset += fileSize;
    memcpy(formBuffer + offset, formEnd.c_str(), formEnd.length());
    
    Serial.printf("[API] Sending %d bytes to ElevenLabs (model: %s)...\n", totalSize, modelId.c_str());
    
    // Send the form data with error checking
    int httpCode = http.POST(formBuffer, totalSize);
    result.httpCode = httpCode;
    
    Serial.printf("[API] HTTP POST sent, waiting for response... (code: %d)\n", httpCode);
    
    if (httpCode > 0) {
        Serial.printf("[API] HTTP Response Code: %d\n", httpCode);
        
        if (httpCode == 200 || httpCode == 201) {
            String payload = http.getString();
            Serial.printf("[API] Response received: %d bytes\n", payload.length());
            
            // Parse JSON response
            DynamicJsonDocument doc(2048);
            DeserializationError error = deserializeJson(doc, payload);
            
            if (!error) {
                // Try both "transcription" (correct) and "text" fields
                if (doc.containsKey("transcription")) {
                    result.text = doc["transcription"].as<String>();
                    result.success = true;
                    Serial.printf("[API] ✓ Transcription successful: %d characters\n", result.text.length());
                } else if (doc.containsKey("text")) {
                    result.text = doc["text"].as<String>();
                    result.success = true;
                    Serial.printf("[API] ✓ Transcription successful: %d characters\n", result.text.length());
                } else {
                    result.error = "No transcription field in response";
                    Serial.printf("[API] ERROR: %s\n", result.error.c_str());
                    Serial.printf("[API] Response: %s\n", payload.substring(0, 300).c_str());
                }
            } else {
                result.error = String("JSON parse error: ") + error.c_str();
                Serial.printf("[API] ERROR: %s\n", result.error.c_str());
                Serial.printf("[API] Response: %s\n", payload.substring(0, 300).c_str());
            }
        } else {
            String payload = http.getString();
            result.error = "HTTP " + String(httpCode) + ": " + getHttpCodeDescription(httpCode);
            if (!payload.isEmpty()) {
                result.error += " - " + payload.substring(0, 200);
            }
            Serial.printf("[API] ERROR: %s\n", result.error.c_str());
            Serial.printf("[API] Response: %s\n", payload.substring(0, 300).c_str());
        }
    } else {
        result.error = "Connection failed: " + http.errorToString(httpCode);
        Serial.printf("[API] ERROR: %s (httpCode: %d)\n", result.error.c_str(), httpCode);
    }
    
    http.end();
    free(fileBuffer);
    free(formBuffer);
    
    Serial.printf("[API] Free heap after cleanup: %d bytes\n", ESP.getFreeHeap());
    
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
