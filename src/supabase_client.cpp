#include "supabase_client.h"
#include "config.h"
#include "wifi_manager.h"
#include <HTTPClient.h>
#include <WiFiClientSecure.h>
#include <ArduinoJson.h>
#include <SD.h>
#include <vector>
#include <cstring>

static const char* REST_AUDIO_PATH = "/rest/v1/audio_records?select=id,file_name,storage_path,summary_id,status&order=recording_date.desc&limit=50";
static const char* REST_SUMMARY_PATH = "/rest/v1/summaries?select=id,audio_id,status,summary_storage_path,infographic_id&audio_id=eq.";
// Some databases may not include created_at; order by id to avoid 42703 errors.
static const char* REST_SUMMARY_LIST = "/rest/v1/summaries?select=id,audio_id,status,summary_storage_path&order=id.desc&limit=50";
static const char* REST_IMAGE_LIST = "/rest/v1/infographics?select=id,summary_id,storage_path,image_file_name,generation_date&order=generation_date.desc&limit=50";

static bool getJson(const String& url, const String& bearer, DynamicJsonDocument& doc) {
    if (!isWiFiConnected()) return false;
    HTTPClient http;
    WiFiClientSecure client;
    client.setInsecure();
    http.begin(client, url);
    http.addHeader("apikey", bearer);
    http.addHeader("Authorization", "Bearer " + bearer);
    http.setTimeout(15000);
    int code = http.GET();
    if (code != 200) {
        Serial.printf("[SUPABASE] GET failed %d\n", code);
        http.end();
        return false;
    }
    
    // Handle chunked responses by reading entire payload into buffer first
    String payload = http.getString();
    http.end();
    
    if (payload.isEmpty()) {
        Serial.println("[SUPABASE] Empty response body");
        return false;
    }
    
    DeserializationError err = deserializeJson(doc, payload);
    if (err) {
        Serial.printf("[SUPABASE] JSON parse error: %s\n", err.c_str());
        Serial.printf("[SUPABASE] Payload: %s\n", payload.c_str());
        return false;
    }
    return true;
}

bool fetchSupabaseAudio(std::vector<AudioFile>& out) {
    out.clear();
    if (!SUPABASE_URL || !SUPABASE_ANON_KEY) return false;
    String url = String(SUPABASE_URL) + REST_AUDIO_PATH;
    DynamicJsonDocument doc(8192);
    if (!getJson(url, SUPABASE_ANON_KEY, doc)) return false;
    if (!doc.is<JsonArray>()) return false;
    for (JsonObject obj : doc.as<JsonArray>()) {
        AudioFile a;
        a.id = obj["id"].as<String>();
        a.filename = obj["file_name"].as<String>();
        a.storagePath = obj["storage_path"].as<String>();
        a.summaryId = obj["summary_id"].as<String>();
        a.status = obj["status"].as<String>();
        out.push_back(a);
    }
    return true;
}

bool fetchSupabaseSummary(const String& audioId, SummaryFile& summary) {
    if (!SUPABASE_URL || !SUPABASE_ANON_KEY) return false;
    String url = String(SUPABASE_URL) + REST_SUMMARY_PATH + audioId + "&limit=1";
    DynamicJsonDocument doc(4096);
    if (!getJson(url, SUPABASE_ANON_KEY, doc)) return false;
    if (!doc.is<JsonArray>() || doc.as<JsonArray>().size() == 0) return false;
    JsonObject obj = doc.as<JsonArray>()[0];
    summary.filename = obj["id"].as<String>() + ".txt";
    summary.relatedAudioFilename = audioId;
    summary.id = obj["id"].as<String>();
    summary.storagePath = obj["summary_storage_path"].as<String>();
    summary.status = obj["status"].as<String>();
    return true;
}

bool fetchSupabaseSummaries(std::vector<SummaryFile>& out) {
    out.clear();
    if (!SUPABASE_URL || !SUPABASE_ANON_KEY) return false;
    String url = String(SUPABASE_URL) + REST_SUMMARY_LIST;
    DynamicJsonDocument doc(8192);
    if (!getJson(url, SUPABASE_ANON_KEY, doc)) return false;
    if (!doc.is<JsonArray>()) return false;
    for (JsonObject obj : doc.as<JsonArray>()) {
        SummaryFile s;
        s.id = obj["id"].as<String>();
        s.filename = s.id + ".txt";
        s.relatedAudioFilename = obj["audio_id"].as<String>();
        s.storagePath = obj["summary_storage_path"].as<String>();
        s.status = obj["status"].as<String>();
        out.push_back(s);
    }
    return true;
}

bool fetchSupabaseImages(std::vector<ImageFile>& out) {
    out.clear();
    if (!SUPABASE_URL || !SUPABASE_ANON_KEY) return false;
    String url = String(SUPABASE_URL) + REST_IMAGE_LIST;
    DynamicJsonDocument doc(8192);
    if (!getJson(url, SUPABASE_ANON_KEY, doc)) return false;
    if (!doc.is<JsonArray>()) return false;
    for (JsonObject obj : doc.as<JsonArray>()) {
        ImageFile img;
        img.id = obj["id"].as<String>();
        img.summaryId = obj["summary_id"].as<String>();
        img.storagePath = obj["storage_path"].as<String>();
        img.filename = obj["image_file_name"].as<String>();
        out.push_back(img);
    }
    return true;
}

static bool downloadToSD(const String& url, const String& destPath) {
    if (!isWiFiConnected()) return false;
    HTTPClient http;
    WiFiClientSecure client;
    client.setInsecure();
    http.begin(client, url);
    int code = http.GET();
    if (code != 200) {
        Serial.printf("[SUPABASE] download failed %d\n", code);
        http.end();
        return false;
    }
    File f = SD.open(destPath, FILE_WRITE);
    if (!f) { http.end(); return false; }
    uint8_t buf[1024];
    WiFiClient* stream = http.getStreamPtr();
    while (http.connected()) {
        size_t avail = stream->available();
        if (avail) {
            int r = stream->readBytes((char*)buf, avail > sizeof(buf) ? sizeof(buf) : avail);
            f.write(buf, r);
        } else {
            delay(1);
        }
    }
    f.close();
    http.end();
    return true;
}

static String callSignedUrl(const String& fn, const String& id) {
    if (!SUPABASE_URL || !SUPABASE_EDGE_TOKEN) return "";
    HTTPClient http;
    WiFiClientSecure client;
    client.setInsecure();
    String url = String(SUPABASE_URL) + "/functions/v1/" + fn + "?id=" + id;
    http.begin(client, url);
    http.addHeader("Authorization", "Bearer " + String(SUPABASE_EDGE_TOKEN));
    int code = http.GET();
    if (code != 200) { http.end(); return ""; }
    DynamicJsonDocument doc(1024);
    DeserializationError err = deserializeJson(doc, http.getString());
    http.end();
    if (err) return "";
    return doc["url"].as<String>();
}

bool downloadAudioFromSupabase(const AudioFile& remote, String& localPath) {
    String signedUrl = callSignedUrl("signed-audio", remote.id);
    if (signedUrl.isEmpty()) return false;
    localPath = "/audio/cloud_" + remote.id + ".wav";
    return downloadToSD(signedUrl, localPath);
}

bool downloadSummaryFromSupabase(const String& summaryId, String& localPath) {
    String signedUrl = callSignedUrl("signed-summary", summaryId);
    if (signedUrl.isEmpty()) return false;
    localPath = "/summaries/summary_" + summaryId + ".txt";
    return downloadToSD(signedUrl, localPath);
}

bool downloadImageFromSupabase(const String& imageId, String& localPath, const String& filename) {
    String signedUrl = callSignedUrl("signed-image", imageId);
    if (signedUrl.isEmpty()) return false;
    String resolvedName = filename.length() ? filename : ("gallery_" + imageId + ".png");
    localPath = "/infographics/" + resolvedName;
    return downloadToSD(signedUrl, localPath);
}

bool fetchSummaryTextFromSupabase(const String& summaryId, String& outText) {
    String signedUrl = callSignedUrl("signed-summary", summaryId);
    if (signedUrl.isEmpty()) return false;
    if (!isWiFiConnected()) return false;
    HTTPClient http;
    WiFiClientSecure client;
    client.setInsecure();
    http.begin(client, signedUrl);
    int code = http.GET();
    if (code != 200) { http.end(); return false; }
    outText = http.getString();
    http.end();
    return true;
}

bool fetchImageToBuffer(const String& imageId, std::vector<uint8_t>& outBuf) {
    String signedUrl = callSignedUrl("signed-image", imageId);
    if (signedUrl.isEmpty() || !isWiFiConnected()) return false;
    HTTPClient http;
    WiFiClientSecure client;
    client.setInsecure();
    http.begin(client, signedUrl);
    int code = http.GET();
    if (code != 200) {
        http.end();
        return false;
    }

    WiFiClient* stream = http.getStreamPtr();
    outBuf.clear();
    uint8_t temp[1024];
    while (http.connected()) {
        size_t avail = stream->available();
        if (avail) {
            size_t toRead = avail > sizeof(temp) ? sizeof(temp) : avail;
            int r = stream->readBytes((char*)temp, toRead);
            if (r > 0) {
                size_t offset = outBuf.size();
                outBuf.resize(offset + r);
                memcpy(outBuf.data() + offset, temp, r);
            }
        } else {
            delay(1);
        }
    }

    http.end();
    return !outBuf.empty();
}

bool triggerProcessAudio(const String& audioId) {
    if (!SUPABASE_URL || !SUPABASE_EDGE_TOKEN) return false;
    HTTPClient http;
    WiFiClientSecure client;
    client.setInsecure();
    String url = String(SUPABASE_URL) + "/functions/v1/process-audio";
    http.begin(client, url);
    http.addHeader("Content-Type", "application/json");
    http.addHeader("Authorization", "Bearer " + String(SUPABASE_EDGE_TOKEN));
    StaticJsonDocument<128> body;
    body["audio_id"] = audioId;
    String payload;
    serializeJson(body, payload);
    int code = http.POST(payload);
    http.end();
    return code == 200 || code == 202;
}
