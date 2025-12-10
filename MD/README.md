# MindInk - M5Stack PaperS3

A powerful portable note-taking and infographic companion for the M5Stack PaperS3 e-ink display. Record audio locally, transcribe it with Eleven Labs, summarize with Google Gemini, and generate infographics—all processed directly on the device with SD card caching!

## Features

✨ **Local Audio Recording** - Record notes and reminders directly on the device
🎤 **On-Device Transcription** - Convert speech to text using Eleven Labs API (direct from device)
📝 **AI Summarization** - Auto-summarize transcriptions with Google Gemini API
🖼️ **Infographic Generation** - Create visual summaries with Gemini
💾 **SD Card Caching** - All audio, transcripts, and summaries cached locally
🔋 **Offline Capable** - Works with cached content when offline
📲 **Touch Interface** - Intuitive menu navigation with border-only button design
🌐 **Supabase Integration** - Sync audio files and results with cloud storage

## Architecture Overview

The MindInk PaperS3 now uses **local processing architecture**:

```
Device → Record Audio → SD Card (/audio/)
                  ↓
            Transcribe → Eleven Labs API → SD Card (/transcripts/)
                  ↓
            Summarize → Gemini API → SD Card (/summaries/)
                  ↓
            Display → E-Ink Screen ✓
```

**Benefits:**
- ✅ No Edge Function complexity
- ✅ Full device control over processing
- ✅ Offline mode with cached results
- ✅ Faster response times
- ✅ Easier debugging via Serial Monitor

## Project Structure

This project uses a modular architecture with local processing capabilities:

```
src/
├── main.cpp              # Application entry point and main loop
├── config.cpp            # Configuration management (API keys, WiFi)
├── storage.cpp           # SD card and PSRAM storage management
├── display.cpp           # UI drawing and display functions
├── wifi_manager.cpp      # WiFi connectivity and management
├── supabase_client.cpp   # Supabase REST API client
└── [legacy files removed]

include/
├── config.h              # Configuration declarations
├── storage.h             # Storage structures and declarations
├── display.h             # Display and UI declarations
├── wifi_manager.h        # WiFi management declarations
└── supabase_client.h     # Supabase client declarations

webapp/                   # Companion web app for audio upload
├── index.html
├── package.json
└── src/

test/                     # Python test scripts
└── test_edge_processing.py
```

## SD Card Folder Structure

The device automatically creates this folder structure on first boot:

```
/ (SD card root)
├── audio/              # Raw audio recordings (.wav/.webm)
├── transcripts/        # ElevenLabs transcription results (.txt)
├── summaries/          # Gemini summarization results (.txt)
└── recordings/         # Device-recorded audio files
```

## Module Breakdown

### 📦 `config.h / config.cpp`
Centralized configuration including:
- WiFi credentials
- API keys (Eleven Labs, Gemini)
- Supabase credentials
- Hardware pins (SD card)
- Recording parameters

### 💾 `storage.h / storage.cpp`
Manages SD card and PSRAM storage:
- `StorageInfo` - Storage system status
- `AudioFile` - Audio recording metadata
- SD card folder initialization
- File caching and management

### 🌐 `wifi_manager.h / wifi_manager.cpp`
WiFi connectivity management:
- `initWiFi()` - Connect to configured network
- `isWiFiConnected()` - Check connection status
- Automatic reconnection handling

### ☁️ `supabase_client.h / supabase_client.cpp`
Supabase REST API integration:
- `fetchSupabaseAudio()` - List audio files from cloud
- `downloadAudioFromSupabase()` - Download audio to SD card
- Signed URL handling for secure downloads

### 🎨 `display.h / display.cpp`
UI and rendering system:
- `AppState` - Navigation states (menu, processing, gallery, etc.)
- `Button` - Touchable UI elements
- Display functions for menu, lists, text, gallery
- Status bar with WiFi and storage info

### 🚀 `main.cpp`
Application orchestration:
- Setup WiFi, SD card, and hardware
- Main event loop and touch handling
- State machine for app navigation
- Integration of all modules

## Hardware Requirements

- **M5Stack PaperS3** with ESP32-S3
- **microSD card** (FAT32 formatted, 8GB+ recommended)
- WiFi connection for API calls and cloud sync
- Optional: Microphone (for device recording)

## Dependencies

- **M5Unified** - M5Stack hardware abstraction
- **ArduinoJson** (v7.x) - JSON parsing for API responses
- **WiFi** - Network connectivity (built-in)
- **HTTPClient** - HTTP requests (built-in)
- **SD_MMC** - SD card support (built-in)

Install via PlatformIO:
```bash
pio lib install "M5Unified" "ArduinoJson"
```

## Setup Instructions

### 1. Clone the Repository
```bash
git clone https://github.com/ylucic-utem/MindInk-PaperS3.git
cd MindInk-PaperS3
```

### 2. SD Card Setup
1. Format microSD card as **FAT32**
2. Insert into PaperS3 device
3. Device will auto-create folders on first boot

### 3. Configure Credentials
Edit `src/config.cpp` and update:
```cpp
const char* WIFI_SSID = "Your WiFi SSID";
const char* WIFI_PASS = "Your WiFi Password";
const char* ELEVEN_LABS_API_KEY = "sk_your_elevenlabs_key";
const char* GEMINI_API_KEY = "AIza_your_gemini_key";
```

**Get API Keys:**
- ElevenLabs: https://elevenlabs.io/app/settings/api-keys
- Gemini: https://aistudio.google.com/apikey

### 4. Build and Upload
```bash
# Build for PaperS3
pio run --environment PaperS3 --target build

# Upload to device
pio run --environment PaperS3 --target upload

# View serial output
pio device monitor
```

### 5. First Boot
Watch Serial Monitor for:
```
[STORAGE] SD Card initialized
[STORAGE] SD Card is properly formatted
[STORAGE] Folder exists: /audio
[STORAGE] Folder exists: /transcripts
[STORAGE] Folder exists: /summaries
[STORAGE] SD folder structure ready
```

## API Integration

### Eleven Labs (Transcription)
- **Model:** `scribe_v2` (latest speech-to-text)
- **Endpoint:** `https://api.elevenlabs.io/v1/speech-to-text`
- **Supported formats:** WAV, MP3, WebM, M4A, OGG, FLAC
- **Get API key:** https://elevenlabs.io/app/settings/api-keys

### Google Gemini (Summarization)
- **Model:** `gemini-2.5-flash-lite` (fast, cost-effective)
- **Endpoint:** `https://generativelanguage.googleapis.com/v1beta/models/gemini-2.5-flash-lite:generateContent`
- **Get API key:** https://aistudio.google.com/apikey

### Supabase (Cloud Storage & Sync)
- **Purpose:** Audio file storage and metadata
- **Features:** REST API, signed URLs, storage buckets
- **Setup:** Free tier available at https://supabase.com

## Processing Workflow

1. **Fetch Audio List** - Get available audio from Supabase
2. **Download Audio** - Cache audio file to SD card (`/audio/`)
3. **Transcribe** - Send to ElevenLabs API, save transcript (`/transcripts/`)
4. **Summarize** - Send transcript to Gemini API, save summary (`/summaries/`)
5. **Display** - Show summary on E-Ink screen

**Complete workflow takes ~30-60 seconds per audio file.**

## Usage

### Main Menu Options
1. **Process Audio** - Download and process audio from Supabase
2. **View Summaries** - Read cached summaries from SD card
3. **Settings** - WiFi status, storage info, cache management

### Processing Audio
```cpp
// Example: Process specific audio file
String audioId = "62cd9c95-51d2-434e-8baf-1eeef165a6a0";
bool success = processAudioLocally(audioId);

if (success) {
    // Audio downloaded, transcribed, and summarized
    // Results cached on SD card
}
```

### Testing with Python Script
Test the complete workflow before deploying to device:
```bash
# Install dependencies
pip install -r requirements.txt

# Configure API keys in .env
cp .env.example .env
# Edit .env with your keys

# Run test
python test_local_processing.py
```

## Development Tips

## Development Tips

### Local Processing Architecture
- All processing happens on-device with SD card caching
- No Edge Functions or complex cloud infrastructure needed
- Easier debugging with Serial Monitor output
- Works offline with cached content

### Adding New Features
1. Create new functions in appropriate modules
2. Update `main.cpp` to integrate new functionality
3. Test with Serial Monitor output
4. Add UI elements in `display.cpp` if needed

### Performance Considerations
- SD card access is slower than PSRAM - cache frequently used data
- API calls require WiFi - handle offline scenarios gracefully
- E-ink refreshes are slow - batch UI updates when possible
- Memory is limited - stream large files when processing

## Troubleshooting

**SD Card Not Detected**
- Format SD card as FAT32 (not exFAT or NTFS)
- Ensure SD card is properly inserted
- Check Serial Monitor for `[STORAGE]` messages
- Try a different SD card if issues persist

**WiFi Won't Connect**
- Check SSID and password in `src/config.cpp`
- Verify PaperS3 is within range
- Check router supports 2.4 GHz (PaperS3 doesn't support 5 GHz)
- Look for `[WiFi]` messages in Serial Monitor

**API Calls Fail**
- Verify API keys are correct and not expired
- Check WiFi connection is stable
- Review HTTP response codes in Serial Monitor
- ElevenLabs: Check account has credits
- Gemini: Check API is enabled in Google Cloud

**Processing Takes Too Long**
- Normal processing time: 30-60 seconds
- Check WiFi speed and stability
- Large audio files take longer to process
- Monitor Serial Monitor for progress updates

**Storage Issues**
- Device requires SD card for local processing
- Format SD card as FAT32 on computer first
- Check available space (recommend 8GB+)
- Files are cached locally for offline access

## Future Enhancements

- [x] Local processing architecture (completed!)
- [x] SD card caching system (completed!)
- [ ] Device audio recording capability
- [ ] Voice commands and hotword detection
- [ ] Cloud sync for backups
- [ ] Battery optimization improvements
- [ ] Multi-language support
- [ ] Batch processing multiple audio files
- [ ] Progress indicators during processing

## License

MIT License - Feel free to fork and modify!

## References

- [PaperS3Weather](https://github.com/squirmen/PaperS3Weather) - Inspiration for modular architecture
- [M5Unified](https://github.com/m5stack/M5Unified)
- [EPDiy](https://github.com/vroland/epdiy)

## Support

For issues and questions:
- Open an issue on GitHub
- Check serial console output for debug messages
- Review the module documentation in `include/` headers
