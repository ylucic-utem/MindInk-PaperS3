#include "supabase_client.h"
#include "config.h"
#include "wifi_manager.h"
#include <HTTPClient.h>
#include <WiFiClientSecure.h>
#include <ArduinoJson.h>
#include <SD.h>

static const char* REST_AUDIO_PATH = "/rest/v1/audio_records?select=id,file_name,storage_path,summary_id,status&order=recording_date.desc&limit=50";
static const char* REST_SUMMARY_PATH = "/rest/v1/summaries?select=id,audio_id,status,summary_storage_path,infographic_id&audio_id=eq.";

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
    DeserializationError err = deserializeJson(doc, http.getStream());
    http.end();
    if (err) {
        Serial.printf("[SUPABASE] JSON parse error: %s\n", err.c_str());
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
    localPath = "/sd/cloud_" + remote.id + ".wav";
    return downloadToSD(signedUrl, localPath);
}

bool downloadSummaryFromSupabase(const String& summaryId, String& localPath) {
    String signedUrl = callSignedUrl("signed-summary", summaryId);
    if (signedUrl.isEmpty()) return false;
    localPath = "/sd/summary_" + summaryId + ".txt";
    return downloadToSD(signedUrl, localPath);
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
