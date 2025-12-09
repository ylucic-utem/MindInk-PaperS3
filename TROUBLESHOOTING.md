# Troubleshooting Guide - Local Processing Test

## Setup Issues

### Python Not Found
**Error:** `'python' is not recognized as an internal or external command`

**Solution:**
1. Install Python 3.8+ from https://python.org
2. **IMPORTANT:** Check "Add Python to PATH" during installation
3. Restart your terminal/command prompt
4. Verify: `python --version`

---

### pip Not Found
**Error:** `'pip' is not recognized`

**Solution:**
1. Upgrade pip: `python -m pip install --upgrade pip`
2. On Windows: `python -m pip install --upgrade pip`
3. On Mac/Linux: `pip3 install --upgrade pip`

---

### Module Import Errors
**Error:** `ModuleNotFoundError: No module named 'supabase'`

**Solution:**
```bash
pip install -r requirements.txt
```

If specific package fails:
```bash
pip install supabase-py
pip install python-dotenv
pip install requests
pip install google-generativeai
```

---

## Configuration Issues

### Missing .env File
**Error:** `KeyError: 'SUPABASE_ANON_KEY'` or similar

**Solution:**
```bash
# Create .env from template
cp .env.example .env

# Edit .env and add your keys
# On Windows: notepad .env
# On Mac/Linux: nano .env
```

**Verify .env format:**
```
SUPABASE_ANON_KEY=eyJhbGciOiJI...  # No quotes!
ELEVEN_LABS_API_KEY=sk_...
GEMINI_API_KEY=AIza_...
```

---

### Invalid API Keys
**Error:** `HTTP 401 Unauthorized`

**Solution:**

For **ElevenLabs:**
1. Go to https://elevenlabs.io/app/settings/api-keys
2. Copy your key (starts with `sk_`)
3. Check key hasn't expired
4. Try creating a new key if current one doesn't work
5. Update `.env`

For **Gemini:**
1. Go to https://aistudio.google.com/apikey
2. Click "Create API Key"
3. Verify project has Generative AI API enabled
4. Copy full key (starts with `AIza`)
5. Update `.env`

---

## Network Issues

### Cannot Connect to Supabase
**Error:** `Connection timeout` or `Cannot reach supabase`

**Solution:**
1. Check internet connection: `ping google.com`
2. Verify Supabase is online: https://status.supabase.io
3. Check URL in `.env`: `https://fptyrdjmrzabvimbzupv.supabase.co`
4. Verify anon key is from correct project

---

### Cannot Download Audio
**Error:** `HTTP 403 Forbidden` or `Download failed`

**Solution:**
1. Verify file exists in Supabase Storage
2. Check storage path is correct in database
3. Ensure `audio-files` bucket exists and is public
4. Try accessing signed URL manually
5. Check ANON_KEY permissions

---

## API Issues

### ElevenLabs Rate Limit
**Error:** `HTTP 429: Too Many Requests`

**Solution:**
- Wait 60 seconds before trying again
- Free tier has rate limits (~10 requests/minute)
- Pro tier has higher limits
- Use same audio multiple times (it's cached!)

---

### ElevenLabs Invalid Audio Format
**Error:** `HTTP 400: Bad Request` with "invalid audio format"

**Solution:**
- Supported formats: WebM, MP3, WAV, AAC, Ogg, FLAC
- Device records as WebM/Opus - should work
- Try transcoding audio to MP3:
  ```bash
  ffmpeg -i input.webm -codec:a libmp3lame -q:a 4 output.mp3
  ```

---

### Gemini API Not Enabled
**Error:** `400 Resource exhausted` or `API_KEY_SERVICE_BLOCKED`

**Solution:**
1. Go to https://console.cloud.google.com
2. Enable "Generative Language API"
3. Create new API key at https://aistudio.google.com/apikey
4. Update `.env` with new key
5. Wait 30 seconds for changes to propagate

---

### Gemini Quota Exceeded
**Error:** `429 Too Many Requests` from Gemini

**Solution:**
- Free tier: 60 requests per minute
- Paid tier: Higher limits
- Wait and retry
- Batch your requests

---

## Data Issues

### No Audio Files Found
**Error:** `No audio files found in Supabase`

**Solution:**
1. Check Supabase project: https://supabase.com/dashboard
2. Verify `audio_records` table exists
3. Upload audio via webapp first
4. Check table permissions (should be public for SELECT)
5. Verify ANON_KEY has correct role

---

### Audio Files Listed But Download Fails
**Error:** Audio shows in list but can't download

**Solution:**
1. Check file exists in storage bucket
2. Verify `storage_path` column in database matches actual location
3. Try manually downloading via Supabase dashboard
4. Check bucket is public (not private)
5. Verify CORS settings allow downloads

---

## Output Issues

### Empty Transcript
**Error:** Transcription returns empty text

**Solution:**
1. Check audio file is not silent/corrupted
2. Verify audio length (ElevenLabs works up to several hours)
3. Try with different audio file
4. Check ElevenLabs accepted the audio
5. Ensure audio format is WebM/Opus or MP3

---

### Empty Summary
**Error:** Gemini returns empty summary

**Solution:**
1. Verify transcript is not empty
2. Check transcript length (Gemini works with up to 30,000 characters)
3. Try simpler/shorter transcript
4. Verify Gemini API key works
5. Check internet connection to Gemini

---

## File Issues

### Permission Denied on Cache Directory
**Error:** `PermissionError: [Errno 13] Permission denied: './audio_cache/...'`

**Solution:**
1. Check folder permissions: `ls -la audio_cache/`
2. Make writable: `chmod 755 audio_cache/`
3. Delete and recreate: `rm -rf audio_cache && mkdir audio_cache`
4. Run with admin/sudo (not recommended)

---

### Disk Space Full
**Error:** `No space left on device`

**Solution:**
1. Check available space: `df -h` (Mac/Linux) or `dir` (Windows)
2. Delete old audio files: `rm -rf audio_cache/*`
3. Move cache to different drive
4. Use smaller audio files

---

## Specific Error Messages

### "Column 'created_at' does not exist"
**Cause:** Database schema mismatch

**Solution:**
- This is from Supabase logs, not your script
- Check table schema in Supabase dashboard
- Verify column names in query match your schema

---

### "CORS error" or "No 'Access-Control-Allow-Origin'"
**Cause:** Browser/CORS issue (shouldn't happen with Python)

**Solution:**
- This is typically a frontend issue
- Python script doesn't use CORS
- If you see this in Python, check internet proxy settings

---

### SSL Certificate Error
**Error:** `SSL: CERTIFICATE_VERIFY_FAILED`

**Solution:**
```bash
# For testing only (not recommended for production):
pip install certifi
# Then restart Python

# Or check your firewall/antivirus isn't blocking SSL
```

---

## Debugging Steps

### Enable Verbose Output
Add debugging to script:
```python
# At top of test_local_processing.py
import logging
logging.basicConfig(level=logging.DEBUG)
```

### Check Network
```bash
# Test Supabase connectivity
curl -H "apikey: YOUR_ANON_KEY" \
  https://fptyrdjmrzabvimbzupv.supabase.co/rest/v1/audio_records?limit=1

# Test ElevenLabs
curl -X POST \
  -H "xi-api-key: YOUR_KEY" \
  https://api.elevenlabs.io/v1/voices
```

### Check File System
```bash
# List cache directory
ls -lh audio_cache/

# Check file size
du -sh audio_cache/

# List all Python packages
pip list
```

### Test Individual Components
```python
# Test Supabase only
from supabase import create_client
client = create_client(url, key)
data = client.table("audio_records").select("*").limit(1).execute()
print(data)

# Test ElevenLabs only
import requests
r = requests.post(
    "https://api.elevenlabs.io/v1/voices",
    headers={"xi-api-key": key}
)
print(r.status_code, r.json())

# Test Gemini only
import google.generativeai as genai
genai.configure(api_key=key)
model = genai.GenerativeModel("gemini-1.5-flash")
response = model.generate_content("Hello")
print(response.text)
```

---

## Still Stuck?

1. **Check all `.md` files** in the project:
   - `QUICKSTART_REFERENCE.md`
   - `TEST_SCRIPT_README.md`
   - `LOCAL_PROCESSING_ARCHITECTURE.md`

2. **Review your API keys:**
   - Are they from the right services?
   - Do they have correct permissions?
   - Are they expired?

3. **Test one component at a time:**
   - Start with Supabase (audio fetch)
   - Then ElevenLabs (transcription)
   - Finally Gemini (summarization)

4. **Check the logs:**
   - Read the full error message (not just first line)
   - Search error in your browser
   - Check service status pages

5. **Verify prerequisites:**
   - Python 3.8+ installed
   - All dependencies installed
   - `.env` file with all keys
   - Internet connection working

Good luck! 🚀
