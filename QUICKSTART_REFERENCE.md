# Quick Reference - Testing the Workflow

## Python Test Script - 5 Minutes to Success

### Setup (First Time)
```bash
# 1. Install Python packages
pip install -r requirements.txt

# 2. Copy environment template
cp .env.example .env

# 3. Add your API keys to .env
# Edit .env and add:
#   ELEVEN_LABS_API_KEY=sk_...
#   GEMINI_API_KEY=AIza_...
```

### Run Test
```bash
python test_local_processing.py
```

### What You'll See
```
============================================================
  STEP 1: Fetching Audio List from Supabase
============================================================
✅ Found 5 audio files:

  [1] audio-2025-12-05T05-31-08-730Z.webm
      ID: 62cd9c95-51d2-434e-8baf-1eeef165a6a0
      Status: new
      Date: 2025-12-05T05:31:08+00:00

  [2] audio-2025-12-05T04-40-15-321Z.webm
      ID: 131cdc64-63b5-4cb2-a1b3-1c89dbdb16e4
      Status: new

Select audio to process (1-5): 1
✅ Selected: audio-2025-12-05T05-31-08-730Z.webm

============================================================
  STEP 2: Downloading Audio from Supabase
============================================================
Downloading: audio-2025-12-05T05-31-08-730Z.webm
✅ Downloaded: 250.5 KB
📁 Saved to: ./audio_cache/62cd9c95-51d2-434e-8baf-1eeef165a6a0.webm

============================================================
  STEP 3: Transcribing with ElevenLabs API
============================================================
File: 62cd9c95-51d2-434e-8baf-1eeef165a6a0.webm
Size: 250.5 KB
Sending to ElevenLabs...
✅ Transcription successful!
Length: 1,256 characters

--- TRANSCRIPT START ---
This is a test audio about machine learning and its
applications in modern software development...
--- TRANSCRIPT END ---

============================================================
  STEP 4: Summarizing with Google Gemini API
============================================================
Input length: 1,256 characters
Sending to Gemini...
✅ Summary generated!
Length: 185 characters

--- SUMMARY START ---
Machine learning is transforming software development
by automating complex tasks and improving application
intelligence through data-driven decision making.
--- SUMMARY END ---

============================================================
  STEP 5: Saving Results
============================================================
✅ Transcript saved: ./audio_cache/62cd9c95...transcript.txt
✅ Summary saved: ./audio_cache/62cd9c95...summary.txt

============================================================
  WORKFLOW COMPLETE ✅
============================================================
Audio: audio-2025-12-05T05-31-08-730Z.webm
Audio ID: 62cd9c95-51d2-434e-8baf-1eeef165a6a0

Transcript length: 1,256 characters
Summary length: 185 characters

Results cached in:
  📁 ./audio_cache/
```

## Getting API Keys

### ElevenLabs
1. Visit: https://elevenlabs.io/app/settings/api-keys
2. Copy API key (starts with `sk_`)
3. Add to `.env`:
   ```
   ELEVEN_LABS_API_KEY=sk_your_key_here
   ```

### Google Gemini
1. Visit: https://aistudio.google.com/apikey
2. Click "Create API Key"
3. Copy the key (starts with `AIza`)
4. Add to `.env`:
   ```
   GEMINI_API_KEY=AIza_your_key_here
   ```

## File Structure After Test

```
.
├── test_local_processing.py          (the test script)
├── .env                              (your API keys - DON'T COMMIT!)
├── .env.example                      (template - safe to commit)
├── requirements.txt                  (Python dependencies)
├── TEST_SCRIPT_README.md             (detailed guide)
├── QUICKSTART_REFERENCE.md           (this file)
└── audio_cache/                      (results folder)
    ├── uuid.webm                     (downloaded audio)
    ├── uuid_transcript.txt           (transcription)
    └── uuid_summary.txt              (summary)
```

## Comparison: Python vs C++ Device

The Python script tests the **exact same workflow** your device will run:

| Operation | Python | C++ Device |
|-----------|--------|-----------|
| Fetch audio list | Supabase REST | Supabase REST |
| Download audio | HTTP GET | HTTP GET |
| Transcribe | ElevenLabs API | ElevenLabs API |
| Summarize | Gemini API | Gemini API |
| Save locally | `audio_cache/` | `/summaries/` on SD |

Once the Python script works, the C++ device will work the same way!

## Common Issues & Solutions

### Issue: "ModuleNotFoundError: No module named 'supabase'"
```bash
Solution: pip install supabase-py
```

### Issue: "KeyError: 'ELEVEN_LABS_API_KEY'"
```
Solution: 
1. Check .env file exists in same directory as script
2. Verify ELEVEN_LABS_API_KEY line has no spaces: key=value
3. Don't use quotes around values
```

### Issue: "HTTP 401 Unauthorized"
```
Solution:
1. Verify API key is correct
2. Key should start with sk_ for ElevenLabs, AIza for Gemini
3. Check keys aren't expired
4. Try generating new keys
```

### Issue: "No audio files found"
```
Solution:
1. Ensure you've uploaded audio via the webapp
2. Check Supabase dashboard: https://supabase.com/dashboard
3. Verify SUPABASE_ANON_KEY is correct
```

## Next Steps After Successful Test

1. ✅ **Python test passed** → Workflow is correct!
2. ✅ **Check transcript** → Open `audio_cache/uuid_transcript.txt`
3. ✅ **Check summary** → Open `audio_cache/uuid_summary.txt`
4. ✅ **Upload to device** → Burn firmware to PaperS3
5. ✅ **Test on device** → Run `processAudioLocally()` from button

## Success Checklist

- [ ] Dependencies installed (`pip install -r requirements.txt`)
- [ ] `.env` file created with API keys
- [ ] ElevenLabs API key verified (not expired)
- [ ] Gemini API key verified (not expired)
- [ ] Audio exists in Supabase
- [ ] Script runs without errors
- [ ] Audio downloaded successfully
- [ ] Transcript generated
- [ ] Summary generated
- [ ] Files saved to `audio_cache/`

## Running the Device Code

Once Python test succeeds, upload to device:

```cpp
// In main.cpp button handler:
if (processAudioLocally("62cd9c95-51d2-434e-8baf-1eeef165a6a0")) {
    // Success! Same results as Python test
    String summary;
    readSummaryLocally("62cd9c95-51d2-434e-8baf-1eeef165a6a0_summary", summary);
    M5.Display.println(summary);
}
```

The workflow is **identical** between Python and C++!

## Questions?

Check these files for more details:
- `LOCAL_PROCESSING_ARCHITECTURE.md` - Full system design
- `QUICKSTART_LOCAL_PROCESSING.md` - Device integration guide
- `TEST_SCRIPT_README.md` - Detailed test script docs

Enjoy! 🚀
