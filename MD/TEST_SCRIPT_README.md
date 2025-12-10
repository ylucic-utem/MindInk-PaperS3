# MindInk Local Processing Test Script

A Python test script that validates the complete audio processing workflow:
1. Fetches audio files from Supabase
2. Downloads selected audio
3. Transcribes with ElevenLabs API
4. Summarizes with Google Gemini API
5. Saves results locally

## Quick Start

### 1. Install Dependencies

```bash
pip install supabase-py python-dotenv requests google-generativeai
```

Or using requirements file:

```bash
pip install -r requirements.txt
```

### 2. Create `.env` File

Copy `.env.example` to `.env` and add your API keys:

```bash
cp .env.example .env
```

Edit `.env` and add:
```
SUPABASE_ANON_KEY=your_key_here
ELEVEN_LABS_API_KEY=sk_your_key_here
GEMINI_API_KEY=AIza_your_key_here
```

### 3. Get Your API Keys

**ElevenLabs:**
1. Go to https://elevenlabs.io/app/settings/api-keys
2. Copy your API key (starts with `sk_`)
3. Paste into `.env` file

**Google Gemini:**
1. Go to https://aistudio.google.com/apikey
2. Create new API key
3. Copy and paste into `.env` file

**Supabase:**
- Already configured (keys are in the script)

### 4. Run the Test

```bash
python test_local_processing.py
```

## What It Does

### Step 1: Fetch Audio List
```
[Connects to Supabase]
✅ Found 5 audio files:
  [1] audio-2025-12-05T05-31-08-730Z.webm
  [2] audio-2025-12-05T04-40-15-321Z.webm
  [3] audio-2025-12-04T21-15-45-123Z.webm
  ...
```

### Step 2: Download Audio
```
[Downloads from Supabase Storage]
✅ Downloaded: 250.5 KB
📁 Saved to: ./audio_cache/uuid.webm
```

### Step 3: Transcribe
```
[Sends to ElevenLabs API]
✅ Transcription successful!
Length: 1,250 characters

--- TRANSCRIPT START ---
This is the transcribed text from the audio file...
--- TRANSCRIPT END ---
```

### Step 4: Summarize
```
[Sends to Google Gemini API]
✅ Summary generated!
Length: 185 characters

--- SUMMARY START ---
A concise summary of the key points...
--- SUMMARY END ---
```

### Step 5: Save Results
```
✅ Transcript saved: ./audio_cache/uuid_transcript.txt
✅ Summary saved: ./audio_cache/uuid_summary.txt
```

## Output Files

Results are saved in `./audio_cache/` directory:

```
audio_cache/
├── uuid.webm                    # Downloaded audio
├── uuid_transcript.txt          # ElevenLabs transcription
└── uuid_summary.txt             # Gemini summary
```

## Troubleshooting

### "Missing environment variables"
- Check that `.env` file exists in the same directory
- Verify all three keys are set: `SUPABASE_ANON_KEY`, `ELEVEN_LABS_API_KEY`, `GEMINI_API_KEY`
- Don't include quotes around values in `.env`

### "No audio files found"
- Check that your Supabase project has audio uploaded via the webapp
- Verify `SUPABASE_ANON_KEY` is correct
- Check internet connection to Supabase

### "API Error: HTTP 401"
- ElevenLabs key might be invalid
- Go to https://elevenlabs.io/app/settings/api-keys and verify
- Make sure key starts with `sk_`

### "Transcription error: Rate limit"
- Wait a few minutes and try again
- ElevenLabs has rate limits on free accounts

### "Summarization error"
- Verify Gemini API key is valid
- Check that your Google Cloud project has Gemini API enabled
- Go to https://aistudio.google.com/apikey to verify

## Performance

Typical processing time for 1 minute of audio:

| Step | Time |
|------|------|
| Download | 5-10s |
| Transcription | 10-30s |
| Summarization | 5-10s |
| **Total** | **~30-60s** |

## Features

✅ **Interactive selection** - Choose which audio to process  
✅ **Caching** - Downloaded audio is cached locally  
✅ **Error handling** - Graceful failures with helpful messages  
✅ **Detailed output** - See all transcripts and summaries  
✅ **File persistence** - Results saved for later review  

## Next Steps

After successful test:

1. **Verify results** - Check transcript and summary in `audio_cache/`
2. **Upload to Supabase** - Save results back to cloud storage
3. **Test on device** - Use same workflow on PaperS3
4. **Integrate into app** - Use `processAudioLocally()` in C++ code

## Example Integration

This Python script validates that the workflow works. Your C++ device code should:

1. Download audio (same as Python step 2)
2. Call ElevenLabs (same as Python step 3)
3. Call Gemini (same as Python step 4)
4. Save to SD card (same as Python step 5)

The workflow is identical - just implemented in different languages!

## Support

For issues:
1. Check `.env` file configuration
2. Verify API keys are correct and active
3. Check internet connection
4. Review error messages in console output
5. Check Supabase project status at https://supabase.com/dashboard

Enjoy testing! 🚀
