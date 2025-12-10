# Python Test Script - Complete Setup Guide

## What You Get

A complete Python test script that validates the entire audio processing workflow:
- ✅ Fetch audio from Supabase
- ✅ Download to local cache
- ✅ Transcribe with ElevenLabs
- ✅ Summarize with Google Gemini
- ✅ Save results locally

## Files Created

```
test_local_processing.py        # Main test script
.env.example                    # Template for API keys
.env                           # Your actual keys (create this)
requirements.txt               # Python dependencies
QUICKSTART_REFERENCE.md        # Quick start guide
TEST_SCRIPT_README.md          # Detailed documentation
TROUBLESHOOTING.md             # Problem solving
setup.sh                       # Linux/Mac setup script
setup.bat                      # Windows setup script
audio_cache/                   # Results folder (auto-created)
```

## 3-Step Quick Start

### Step 1: Install
**Windows:**
```bash
setup.bat
```

**Mac/Linux:**
```bash
bash setup.sh
```

**Manual:**
```bash
pip install -r requirements.txt
cp .env.example .env
```

### Step 2: Configure
Edit `.env` and add your API keys:
```
ELEVEN_LABS_API_KEY=sk_...
GEMINI_API_KEY=AIza_...
```

Get keys from:
- ElevenLabs: https://elevenlabs.io/app/settings/api-keys
- Gemini: https://aistudio.google.com/apikey

### Step 3: Run
```bash
python test_local_processing.py
```

## What Happens

The script will:

1. **Connect to Supabase** and list available audio files
2. **Let you select** which audio to process
3. **Download** the audio file to `./audio_cache/`
4. **Send to ElevenLabs** for transcription
5. **Send to Gemini** for summarization
6. **Save results** to local files
7. **Display** transcript and summary in terminal

## Output

```
============================================================
  STEP 1: Fetching Audio List from Supabase
============================================================
✅ Found 5 audio files:
  [1] audio-2025-12-05T05-31-08-730Z.webm
  [2] audio-2025-12-05T04-40-15-321Z.webm
  ...

Select audio to process (1-5): 1
✅ Selected: audio-2025-12-05T05-31-08-730Z.webm

[Continues through steps 2-5...]

============================================================
  WORKFLOW COMPLETE ✅
============================================================
Results cached in: ./audio_cache/
```

## Success Checklist

- [ ] Python 3.8+ installed
- [ ] Dependencies installed (`pip install -r requirements.txt`)
- [ ] `.env` file created
- [ ] ElevenLabs API key added to `.env`
- [ ] Gemini API key added to `.env`
- [ ] Audio exists in Supabase
- [ ] Script runs without errors
- [ ] Transcript generated
- [ ] Summary generated
- [ ] Files saved to `audio_cache/`

## Key Files

| File | Purpose |
|------|---------|
| `test_local_processing.py` | Main script - run this |
| `.env` | Your API keys (create from `.env.example`) |
| `requirements.txt` | Python packages to install |
| `QUICKSTART_REFERENCE.md` | Quick reference guide |
| `TEST_SCRIPT_README.md` | Detailed documentation |
| `TROUBLESHOOTING.md` | Problem solving guide |

## Important Notes

⚠️ **Security:**
- Never commit `.env` file to Git (it's in `.gitignore`)
- Don't share your API keys
- Keep keys private and secure

⚠️ **API Costs:**
- ElevenLabs: Free tier has limits
- Gemini: Free tier has limits (~60 requests/min)
- Check your account dashboard for usage

✅ **Caching:**
- Audio is cached locally in `audio_cache/`
- Won't re-download same file
- Saves bandwidth and time

## Next Steps After Success

1. **Review results** in `audio_cache/` folder
2. **Check transcript** accuracy
3. **Check summary** quality
4. **Test on device** with same workflow
5. **Integrate into your app** using `processAudioLocally()`

## Device Integration

Once Python test works, your C++ device code uses the same workflow:

```cpp
// Same steps as Python test
bool success = processAudioLocally(audioId);

if (success) {
    // Audio downloaded ✓
    // Transcript saved ✓
    // Summary saved ✓
    // All cached on SD card ✓
}
```

## Support

For issues, check:
1. `QUICKSTART_REFERENCE.md` - Quick answers
2. `TROUBLESHOOTING.md` - Problem solving
3. `TEST_SCRIPT_README.md` - Detailed docs

Common issues:
- Missing API keys → Check `.env`
- Python not found → Install Python 3.8+
- API error → Verify keys are correct
- No audio → Upload via webapp first

## Workflow Comparison

This Python script tests the **exact same workflow** your device will run:

**Python (this test):**
```
fetch → download → transcribe → summarize → save
```

**C++ Device (final implementation):**
```
fetch → download → transcribe → summarize → save
```

✨ **Same workflow, different implementation!**

## Quick Command Reference

```bash
# First time setup
setup.bat                          # Windows
bash setup.sh                      # Mac/Linux

# Run the test
python test_local_processing.py

# Install dependencies manually
pip install -r requirements.txt

# Check Python version
python --version

# Check dependencies installed
pip list

# View .env file
cat .env                          # Mac/Linux
type .env                         # Windows

# Delete cache and start fresh
rm -rf audio_cache                # Mac/Linux
rmdir /s audio_cache              # Windows
```

## Troubleshooting Quick Links

- **Python not found** → See TROUBLESHOOTING.md "Python Not Found"
- **Missing API keys** → See TROUBLESHOOTING.md "Missing .env File"
- **API errors** → See TROUBLESHOOTING.md "API Issues"
- **No audio files** → See TROUBLESHOOTING.md "No Audio Files Found"
- **Download fails** → See TROUBLESHOOTING.md "Audio Files Listed But Download Fails"

---

**You're all set!** 🎉 Run the script and watch the magic happen. ✨
