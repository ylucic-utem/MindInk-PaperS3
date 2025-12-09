# Audio Processing Issues & Solutions

## Problem Analysis

Based on your PlatformIO monitor logs and database inspection, I identified **three critical issues** preventing transcriptions and summaries from being generated:

### Issue 1: WebM Format Not Supported by ElevenLabs
**Problem:** The webapp records audio in WebM/Opus format, but ElevenLabs Speech-to-Text API has limited/unreliable support for WebM. It works best with:
- MP3 (recommended)
- WAV
- M4A
- FLAC
- OGG

**Evidence:** The Edge Function returns 202 (accepted) but never completes the transcription step.

### Issue 2: Missing Database Records
**Problem:** The webapp uploads files to the `audio-files` storage bucket but **does not** insert corresponding records into the `audio_records` table. Without database records:
- The device can't list uploaded files
- The Edge Function has no metadata to process
- Status tracking is impossible

**Evidence:** 
```sql
-- Storage has files:
audio-2025-12-05T05-31-08-730Z.webm ✓ (73KB)
audio-2025-12-05T04-40-15-321Z.webm ✓ (22KB)

-- But audio_records shows:
- Records stuck in "processing" status
- No summaries table entries
- No transcriptions table entries
```

### Issue 3: Storage Path Duplication Bug
**Problem:** Existing `audio_records` entries have duplicated paths:
```
storage_path: "audio-2025-12-05T05-31-08-730Z.webm/audio-2025-12-05T05-31-08-730Z.webm"
```
But the actual file in storage is at:
```
audio-2025-12-05T05-31-08-730Z.webm (flat path)
```
This causes the Edge Function to fail when trying to download the file.

---

## Solutions Implemented

### ✅ Solution 1: Fixed Webapp Database Insertion

**File:** `webapp/src/components/Recorder.tsx`

**Changes:**
1. Added database insertion after successful storage upload
2. Fixed storage path to use flat path (no duplication)
3. Added proper error handling and logging
4. Sets initial status to 'new' for processing

```typescript
// After storage upload:
const { data: recordData, error: recordError } = await supabase
  .from('audio_records')
  .insert({
    file_name: fileName,
    storage_path: filePath, // Flat path: "audio-2025-12-05.webm"
    status: 'new',
  })
  .select()
  .single();
```

### ✅ Solution 2: Improved Edge Function Error Handling

**File:** `supabase/functions/process-audio/index.ts`

**Changes:**
1. Added detailed logging at every step
2. Improved error messages from ElevenLabs and Gemini
3. Added audio file size logging
4. Better handling of WebM content type
5. Added validation of downloaded audio data

**Key improvements:**
```typescript
// Now logs:
console.log(`[process-audio] Downloaded ${audioBuffer.byteLength} bytes`);
console.log(`[process-audio] Sending ${contentType} to ElevenLabs...`);
console.log("[process-audio] Got transcription (length:", transcription.length, "chars)");

// Better error logging:
if (!elResponse.ok) {
  const errorText = await elResponse.text();
  console.error(`[process-audio] ElevenLabs error response:`, errorText);
  throw new Error(`ElevenLabs failed: ${elResponse.status} - ${errorText}`);
}
```

---

## Remaining Issues & Recommendations

### 🔶 Issue: WebM Format Compatibility

**Current State:** WebM/Opus is what browsers natively support for `MediaRecorder` API.

**Options:**

#### Option A: Server-Side Conversion (Recommended)
Convert WebM to MP3 on the server before sending to ElevenLabs.

**Implementation:**
1. Add FFmpeg to Edge Function environment
2. Convert audio before ElevenLabs call:
```typescript
// In process-audio Edge Function:
import { FFmpeg } from 'https://esm.sh/@ffmpeg/ffmpeg';

async function convertWebmToMp3(webmBuffer: ArrayBuffer): Promise<ArrayBuffer> {
  const ffmpeg = createFFmpeg({ log: true });
  await ffmpeg.load();
  
  ffmpeg.FS('writeFile', 'input.webm', new Uint8Array(webmBuffer));
  await ffmpeg.run('-i', 'input.webm', '-codec:a', 'libmp3lame', '-b:a', '64k', 'output.mp3');
  
  const mp3Data = ffmpeg.FS('readFile', 'output.mp3');
  return mp3Data.buffer;
}
```

**Pros:**
- No client-side changes needed
- Works with all browsers
- Optimizes file size for API calls

**Cons:**
- Requires FFmpeg in Edge Function
- Increases processing time (~2-5 seconds)
- May exceed Edge Function timeout for long audio

#### Option B: Client-Side Recording in MP3
Use a library like `lamejs` to encode MP3 directly in the browser.

**Implementation:**
```typescript
// In Recorder.tsx:
import lamejs from 'lamejs';

async function recordAsMP3(): Promise<Blob> {
  // Get PCM data from AudioContext
  const audioContext = new AudioContext({ sampleRate: 16000 });
  // ... process audio
  
  // Encode to MP3
  const mp3Encoder = new lamejs.Mp3Encoder(1, 16000, 64);
  const mp3Data = [];
  
  // ... encode chunks
  
  return new Blob(mp3Data, { type: 'audio/mp3' });
}
```

**Pros:**
- Native MP3 support in ElevenLabs
- Smaller file sizes
- No server-side conversion needed

**Cons:**
- More complex client code
- Encoding overhead on device
- May drain battery on mobile

#### Option C: Try Without Conversion First
Test if current WebM actually works. ElevenLabs documentation says "audio/webm" is supported for some endpoints.

**Testing Steps:**
1. Deploy the updated Edge Function
2. Upload a new audio file from webapp
3. Manually trigger processing via API or Supabase dashboard
4. Check Edge Function logs for detailed error messages

**Command to trigger manually:**
```bash
curl -X POST https://fptyrdjmrzabvimbzupv.supabase.co/functions/v1/process-audio \
  -H "Authorization: Bearer YOUR_ANON_KEY" \
  -H "Content-Type: application/json" \
  -d '{"audio_id":"62cd9c95-51d2-434e-8baf-1eeef165a6a0"}'
```

---

## Webhooks: Do You Need Them?

**Short Answer:** No, webhooks are NOT required for your current architecture.

**Current Flow (Polling):**
1. Device triggers processing via `POST /functions/v1/process-audio`
2. Edge Function returns 202 Accepted immediately
3. Processing happens in background (5-30 seconds)
4. Device polls `audio_records` table to check status field
5. When status='done', device fetches summary

**With Webhooks (Alternative):**
1. Device triggers processing
2. Edge Function completes
3. Edge Function calls device webhook URL with results
4. Device receives push notification

**Why Polling is Better for Your Use Case:**
- ✅ Device may not have static IP/accessible endpoint
- ✅ Simpler to implement
- ✅ No need to expose device to internet
- ✅ Works behind NAT/firewall
- ✅ Device controls when to check (battery-friendly)

**When to Use Webhooks:**
- If you had a webapp that needed real-time updates
- If processing took >5 minutes
- If you wanted Supabase to notify an external service

---

## Deployment Steps

### 1. Update Webapp
```bash
cd webapp
npm install
npm run build
```

Then deploy to your hosting (Render/Vercel/etc).

### 2. Deploy Updated Edge Function
```bash
# Install Supabase CLI if not already installed
npm install -g supabase

# Login to Supabase
supabase login

# Link to your project
supabase link --project-ref fptyrdjmrzabvimbzupv

# Deploy the function
supabase functions deploy process-audio
```

### 3. Clean Up Existing Bad Records
Run this SQL in Supabase SQL Editor to fix existing records:

```sql
-- Reset stuck processing records
UPDATE audio_records 
SET status = 'new' 
WHERE status = 'processing';

-- Fix duplicated storage paths
UPDATE audio_records 
SET storage_path = REGEXP_REPLACE(storage_path, '^(.+?)/\\1$', '\\1')
WHERE storage_path LIKE '%/%';

-- Verify paths are now correct
SELECT id, file_name, storage_path, status 
FROM audio_records 
ORDER BY recording_date DESC;
```

### 4. Test the Full Pipeline
1. Record new audio on iPhone webapp
2. Check that `audio_records` entry was created with status='new'
3. Trigger processing from device (or manually via curl)
4. Monitor Edge Function logs:
```bash
supabase functions logs process-audio --project-ref fptyrdjmrzabvimbzupv
```
5. Check for transcription and summary in database:
```sql
SELECT 
  ar.file_name,
  ar.status,
  t.transcription_text,
  s.summary_storage_path
FROM audio_records ar
LEFT JOIN transcriptions t ON t.audio_id = ar.id
LEFT JOIN summaries s ON s.audio_id = ar.id
ORDER BY ar.recording_date DESC
LIMIT 5;
```

---

## Monitoring & Debugging

### Check Edge Function Logs
```bash
# Real-time logs
supabase functions logs process-audio --follow

# Or via Supabase Dashboard:
# https://app.supabase.com/project/fptyrdjmrzabvimbzupv/functions/process-audio/logs
```

### Check for Transcription Errors
```sql
SELECT id, audio_id, status, created_at 
FROM audio_records 
WHERE status = 'error' 
ORDER BY created_at DESC;
```

### Manually Trigger Processing
```bash
# Get an audio_id from database
curl -X POST \
  https://fptyrdjmrzabvimbzupv.supabase.co/functions/v1/process-audio \
  -H "Authorization: Bearer YOUR_ANON_KEY" \
  -H "Content-Type: application/json" \
  -d '{"audio_id":"YOUR_AUDIO_ID_HERE"}'
```

---

## Next Steps

1. ✅ **Deploy the webapp changes** - Fixes database insertion
2. ✅ **Deploy the Edge Function** - Improved logging and error handling
3. ✅ **Clean up existing records** - Run SQL to fix bad data
4. 🔶 **Test with new upload** - See if WebM works now
5. 🔶 **If still failing** - Implement Option A (server-side conversion) or Option B (client MP3 encoding)

---

## Summary

**Root Causes:**
1. ❌ Webapp not inserting database records
2. ❌ WebM format may not be supported by ElevenLabs
3. ❌ Duplicated storage paths in existing records

**Fixes Applied:**
1. ✅ Added database insertion after upload
2. ✅ Improved Edge Function error logging
3. ✅ Fixed storage path format

**Still To Do:**
1. Deploy changes
2. Clean up bad data
3. Test if WebM now works
4. If not, add audio conversion

**Webhooks:** Not needed - polling works better for your architecture.
