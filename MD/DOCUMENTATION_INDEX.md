# Documentation Index

## Quick Start (Start Here! 👈)

**[PYTHON_TEST_GUIDE.md](PYTHON_TEST_GUIDE.md)** - Complete setup guide
- 3-step quick start
- Success checklist
- Key files overview

**[QUICKSTART_REFERENCE.md](QUICKSTART_REFERENCE.md)** - Fast reference
- 5 minutes to success
- Copy/paste commands
- API key locations

## Python Test Script

**[test_local_processing.py](test_local_processing.py)** - Main test script
- Downloads audio from Supabase
- Calls ElevenLabs for transcription
- Calls Gemini for summarization
- Saves results locally

**[TEST_SCRIPT_README.md](TEST_SCRIPT_README.md)** - Detailed documentation
- Step-by-step walkthrough
- How it works
- Performance metrics
- Features

**[requirements.txt](requirements.txt)** - Python dependencies
- Supabase client
- Environment variables
- HTTP requests
- Gemini API

**[.env.example](.env.example)** - Configuration template
- Copy to `.env` and edit
- Add your API keys here

**[setup.sh](setup.sh)** - Linux/Mac setup
- One-click installation
- Creates directories
- Validates Python

**[setup.bat](setup.bat)** - Windows setup
- One-click installation
- Creates directories
- Validates Python

## Architecture & Design

**[LOCAL_PROCESSING_ARCHITECTURE.md](LOCAL_PROCESSING_ARCHITECTURE.md)** - Complete system design
- Architecture comparison (old vs new)
- SD card folder structure
- Processing workflow
- API requirements
- Cache management
- Error handling
- Integration guide

**[QUICKSTART_LOCAL_PROCESSING.md](QUICKSTART_LOCAL_PROCESSING.md)** - Device integration guide
- What changed
- How it works
- SD card setup
- Usage examples
- API keys required
- Testing checklist

## Implementation

**[src/local_processing.cpp](src/local_processing.cpp)** - C++ implementation
- Download audio from Supabase
- Call ElevenLabs API
- Call Gemini API
- Cache results on SD card
- Complete workflow function

**[include/storage.h](include/storage.h)** (updated)
- SD card folder management
- Cache path helpers
- Cache checking functions

**[src/storage.cpp](src/storage.cpp)** (updated)
- Folder initialization
- Format checking
- Path management

**[include/api.h](include/api.h)** (updated)
- Local processing functions
- File operations
- Workflow orchestration

## Testing & Troubleshooting

**[TROUBLESHOOTING.md](TROUBLESHOOTING.md)** - Problem solving guide
- Setup issues
- Configuration problems
- Network issues
- API errors
- Data issues
- Specific error messages
- Debugging steps

**[test/test_local_processing.cpp](test/test_local_processing.cpp)** - Device test program
- Full workflow test
- Hardware validation
- Serial Monitor output
- Debugging info

## Project Files

### Source Code
```
src/
├── main.cpp                 # Main application
├── api.cpp                  # API calls (ElevenLabs, Gemini)
├── audio.cpp               # Audio recording/playback
├── display.cpp             # E-ink display
├── storage.cpp             # SD card management
├── wifi_manager.cpp        # WiFi connectivity
├── supabase_client.cpp     # Supabase REST API
├── config.cpp              # Configuration
└── local_processing.cpp    # LOCAL PROCESSING (NEW!)

include/
├── api.h                   # API function declarations
├── storage.h               # Storage management (UPDATED!)
├── config.h                # Config variables
├── display.h               # Display functions
├── audio.h                 # Audio functions
├── wifi_manager.h          # WiFi functions
└── supabase_client.h       # Supabase functions
```

### Documentation
```
├── PYTHON_TEST_GUIDE.md              # THIS FILE - Start here!
├── QUICKSTART_REFERENCE.md           # Quick reference
├── QUICKSTART_LOCAL_PROCESSING.md    # Device guide
├── LOCAL_PROCESSING_ARCHITECTURE.md  # Complete design
├── TEST_SCRIPT_README.md             # Test script docs
├── TROUBLESHOOTING.md                # Problem solving
├── README.md                         # Project overview
├── SETUP.md                          # Original setup guide
└── PLAN.md                           # Project planning docs
```

### Configuration
```
├── .env.example                      # Configuration template
├── .env                              # Your API keys (CREATE THIS!)
├── requirements.txt                  # Python dependencies
├── setup.sh                          # Linux/Mac setup
├── setup.bat                         # Windows setup
├── platformio.ini                    # PlatformIO config
└── tsconfig.json                     # TypeScript config (webapp)
```

### Storage
```
audio_cache/                          # Python test results
├── uuid.webm                         # Downloaded audio
├── uuid_transcript.txt               # ElevenLabs result
└── uuid_summary.txt                  # Gemini result
```

## Reading Path

### New to MindInk? 👇

1. **[PYTHON_TEST_GUIDE.md](PYTHON_TEST_GUIDE.md)** - Understand the system
2. **[QUICKSTART_REFERENCE.md](QUICKSTART_REFERENCE.md)** - Get it running
3. Run `python test_local_processing.py` - Test it
4. **[QUICKSTART_LOCAL_PROCESSING.md](QUICKSTART_LOCAL_PROCESSING.md)** - Device integration
5. **[LOCAL_PROCESSING_ARCHITECTURE.md](LOCAL_PROCESSING_ARCHITECTURE.md)** - Deep dive

### Want to Debug? 👇

1. **[TROUBLESHOOTING.md](TROUBLESHOOTING.md)** - Find your error
2. **[TEST_SCRIPT_README.md](TEST_SCRIPT_README.md)** - Verify setup
3. Check Serial Monitor output on device
4. **[LOCAL_PROCESSING_ARCHITECTURE.md](LOCAL_PROCESSING_ARCHITECTURE.md)** - Understand flow

### Implementing on Device? 👇

1. **[QUICKSTART_LOCAL_PROCESSING.md](QUICKSTART_LOCAL_PROCESSING.md)** - Overview
2. Review **[src/local_processing.cpp](src/local_processing.cpp)** - Implementation
3. Check **[include/storage.h](include/storage.h)** - SD card management
4. Integrate into **[src/main.cpp](src/main.cpp)** - Add button/menu

### Want Full Understanding? 👇

Read in this order:
1. **[PYTHON_TEST_GUIDE.md](PYTHON_TEST_GUIDE.md)** - What we're doing
2. **[LOCAL_PROCESSING_ARCHITECTURE.md](LOCAL_PROCESSING_ARCHITECTURE.md)** - How it works
3. **[QUICKSTART_LOCAL_PROCESSING.md](QUICKSTART_LOCAL_PROCESSING.md)** - Integration
4. Review source code
5. **[TROUBLESHOOTING.md](TROUBLESHOOTING.md)** - Problem solving

## By Task

### "I want to test the workflow"
→ **[PYTHON_TEST_GUIDE.md](PYTHON_TEST_GUIDE.md)** + run `test_local_processing.py`

### "I need to set up on device"
→ **[QUICKSTART_LOCAL_PROCESSING.md](QUICKSTART_LOCAL_PROCESSING.md)**

### "Something's not working"
→ **[TROUBLESHOOTING.md](TROUBLESHOOTING.md)**

### "I want to understand the architecture"
→ **[LOCAL_PROCESSING_ARCHITECTURE.md](LOCAL_PROCESSING_ARCHITECTURE.md)**

### "I need quick API key info"
→ **[QUICKSTART_REFERENCE.md](QUICKSTART_REFERENCE.md)**

### "I want implementation details"
→ **[src/local_processing.cpp](src/local_processing.cpp)**

## Key Concepts

### The Workflow
```
Fetch Audio List (Supabase)
           ↓
        Download (HTTP)
           ↓
     Transcribe (ElevenLabs API)
           ↓
      Summarize (Gemini API)
           ↓
      Save Locally (SD/Cache)
```

### File Locations
- **Python cache:** `./audio_cache/`
- **Device cache:** `/audio/`, `/transcripts/`, `/summaries/`
- **Config:** `.env` file

### API Keys Needed
- **ElevenLabs:** `sk_...` from https://elevenlabs.io/app/settings/api-keys
- **Gemini:** `AIza...` from https://aistudio.google.com/apikey
- **Supabase:** Already configured

### Speed
- Download: 5-10s
- Transcribe: 10-30s
- Summarize: 5-10s
- **Total: 30-60s per audio**

## File Sizes

| Component | Size |
|-----------|------|
| Python script | ~8 KB |
| Documentation | ~100 KB |
| Audio cache | Variable (100-500 KB per file) |
| C++ firmware | ~500-700 KB |

## Version Info

- **Python:** 3.8+
- **C++:** C++17
- **Supabase:** Latest
- **ElevenLabs API:** v1
- **Google Gemini:** Latest

---

## TL;DR - Get Started Now

```bash
# 1. Setup (1 minute)
setup.bat              # Windows
# or
bash setup.sh          # Mac/Linux

# 2. Configure (2 minutes)
# Edit .env with your API keys from:
# - https://elevenlabs.io/app/settings/api-keys
# - https://aistudio.google.com/apikey

# 3. Test (5 minutes)
python test_local_processing.py

# 4. Enjoy! ✨
```

That's it! The workflow is:
1. Select audio
2. Download from Supabase
3. Transcribe with ElevenLabs
4. Summarize with Gemini
5. View results!

---

**Questions?** Check the relevant guide above.

**Something broken?** → [TROUBLESHOOTING.md](TROUBLESHOOTING.md)

**Want more details?** → [LOCAL_PROCESSING_ARCHITECTURE.md](LOCAL_PROCESSING_ARCHITECTURE.md)

Happy processing! 🚀
