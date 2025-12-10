# Edge Function Diagnostics

## Current Status (December 5, 2025 05:45 GMT)

### ✅ Working Components
1. **Device to Edge Function Communication**: Device successfully POSTs to `/functions/v1/process-audio` and receives HTTP 202 Accepted
2. **Edge Function Deployment**: Version 8 deployed successfully
3. **Database Record Creation**: Record `ac5385f0-44f0-43dd-8a16-5aece28c4e27` status changed from 'new' to 'processing'
4. **Authentication**: Device has valid anon key and Edge Function accepts requests

### ❌ Failing Component
**Background Processing Not Completing**: Record stays in 'processing' status indefinitely, no transcription or summary created

## Root Cause: Missing Environment Variables

The Edge Function `process-audio` requires these environment variables to call external APIs:

```
ELEVEN_LABS_API_KEY=<your-elevenlabs-api-key>
GEMINI_API_KEY=<your-google-gemini-api-key>
```

**Without these keys**, the Edge Function:
- Accepts the request (returns 202)
- Updates database status to 'processing'
- Attempts to call ElevenLabs API
- **FAILS** because API key is undefined
- Does NOT update status to 'error' (silent failure)
- Does NOT create transcription or summary records

## Evidence

### Database Query Results
```sql
-- Record stuck in 'processing' status
SELECT id, file_name, status, summary_id
FROM audio_records
WHERE id = 'ac5385f0-44f0-43dd-8a16-5aece28c4e27';

-- Result:
-- status = 'processing'
-- summary_id = NULL
-- No transcription record
-- No summary record
```

### Edge Function Logs
```
POST | 202 | /functions/v1/process-audio
execution_time_ms: 476
version: 8
timestamp: 1764913494370000
```

**Missing from logs**: The detailed `console.log()` output from `processAudioAsync()`:
- "Downloaded X bytes from storage"
- "Sending to ElevenLabs..."
- "Got transcription: ..."
- "Calling Gemini..."
- "Saved summary to storage"

These logs don't appear in the Edge Function service logs, suggesting either:
1. Processing failed before reaching those log statements
2. Console logs from background async functions aren't captured
3. API keys are undefined causing immediate failure

## Solution: Set Environment Variables in Supabase Dashboard

### Step 1: Get API Keys

**ElevenLabs API Key**:
1. Go to https://elevenlabs.io/app/settings/api-keys
2. Create new API key or copy existing one
3. Format: `sk_...` (starts with `sk_`)

**Google Gemini API Key**:
1. Go to https://aistudio.google.com/apikey
2. Create API key
3. Format: `AIza...` (starts with `AIza`)

### Step 2: Set in Supabase

1. Open Supabase Dashboard: https://supabase.com/dashboard/project/fptyrdjmrzabvimbzupv
2. Navigate to: **Project Settings** → **Edge Functions** → **process-audio**
3. Click **Environment Variables** section
4. Add these two variables:
   ```
   ELEVEN_LABS_API_KEY = sk_your_actual_key_here
   GEMINI_API_KEY = AIza_your_actual_key_here
   ```
5. Click **Save**

### Step 3: Redeploy Edge Function (Optional)

Environment variable changes take effect immediately on next function invocation. No redeploy needed.

### Step 4: Test Again

Trigger processing from device:
```cpp
// ESP32 code already working:
POST https://fptyrdjmrzabvimbzupv.supabase.co/functions/v1/process-audio
Authorization: Bearer eyJhbGciOiJIUzI1NiIsInR5cCI6IkpXVCJ9...
Content-Type: application/json

{"audio_id":"62cd9c95-51d2-434e-8baf-1eeef165a6a0"}
```

Expected results within 30 seconds:
- Database status changes from 'processing' to 'done'
- `transcriptions` table gets new record
- `summaries` table gets new record
- Device can fetch summary via `/rest/v1/summaries` endpoint

## Verification Query

Run this after setting environment variables and triggering processing:

```sql
SELECT 
  ar.id,
  ar.file_name,
  ar.status,
  t.transcription_text IS NOT NULL as has_transcription,
  s.summary_storage_path IS NOT NULL as has_summary,
  s.status as summary_status
FROM audio_records ar
LEFT JOIN transcriptions t ON t.audio_id = ar.id
LEFT JOIN summaries s ON s.audio_id = ar.id
WHERE ar.id = '62cd9c95-51d2-434e-8baf-1eeef165a6a0';
```

Expected output after successful processing:
```
status: "done"
has_transcription: true
has_summary: true
summary_status: "done"
```

## Alternative: Check Environment Variables are Set

If you have Supabase CLI installed:

```bash
supabase functions env list --project-ref fptyrdjmrzabvimbzupv
```

Should show:
```
ELEVEN_LABS_API_KEY (set)
GEMINI_API_KEY (set)
```

## Next Steps

1. **URGENT**: Set the two environment variables in Supabase Dashboard
2. Reset test record to 'new' status:
   ```sql
   UPDATE audio_records 
   SET status = 'new' 
   WHERE id = 'ac5385f0-44f0-43dd-8a16-5aece28c4e27';
   ```
3. Trigger processing from device again
4. Wait 30 seconds
5. Query database to verify transcription and summary were created
6. If still failing, check Edge Function real-time logs in Supabase Dashboard during processing

## Edge Function Code Reference

Current version 8 code expects environment variables:

```typescript
// In supabase/functions/process-audio/index.ts
const elevenLabsApiKey = Deno.env.get('ELEVEN_LABS_API_KEY');
const geminiApiKey = Deno.env.get('GEMINI_API_KEY');

// If these are undefined, API calls will fail with 401 Unauthorized
const response = await fetch('https://api.elevenlabs.io/v1/audio-to-text', {
  headers: {
    'xi-api-key': elevenLabsApiKey, // undefined if env var not set!
  },
  // ...
});
```

## Status Records

Current database state:

| ID | File Name | Status | Has Transcription | Has Summary |
|---|---|---|---|---|
| ac5385f0-44f0-43dd-8a16-5aece28c4e27 | audio-2025-12-05T05-44-21-234Z.webm | processing | false | false |
| 892f49f7-552e-47df-b796-b93fab25f637 | audio-2025-12-05T05-44-21-234Z.webm | new | false | false |
| 62cd9c95-51d2-434e-8baf-1eeef165a6a0 | audio-2025-12-05T05-31-08-730Z.webm | new | false | false |
| 131cdc64-63b5-4cb2-a1b3-1c89dbdb16e4 | audio-2025-12-05T04-40-15-321Z.webm | new | false | false |

Ready for testing once environment variables are set.
