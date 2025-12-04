# MindInk - M5Stack PaperS3

A powerful portable note-taking and infographic companion for the M5Stack PaperS3 e-ink display. Record audio, transcribe it with Eleven Labs, summarize with Google Gemini, and generate infographics—all on your wrist!

## Features

✨ **Audio Recording** - Record notes and reminders directly on the device
🎤 **Transcription** - Convert speech to text using Eleven Labs API
📝 **Summarization** - Auto-summarize transcriptions with Gemini 1.5 Flash
🖼️ **Infographic Generation** - Create visual summaries with Gemini 3 Pro
💾 **Dual Storage** - Save to SD card or PSRAM depending on availability
🔋 **Efficient Display** - E-ink display consumes minimal power
📲 **Touch Interface** - Intuitive menu navigation with border-only button design

## Project Structure

This project has been refactored into a modular architecture for better maintainability and development:

```
src/
├── main.cpp          # Application entry point and main loop
├── config.cpp        # Configuration management
├── storage.cpp       # Storage initialization and management
├── display.cpp       # UI drawing and display functions
├── audio.cpp         # Audio recording functionality
└── api.cpp           # API communication (Eleven Labs, Gemini)

include/
├── config.h          # Configuration declarations
├── storage.h         # Storage structures and declarations
├── display.h         # Display and UI declarations
├── audio.h           # Audio function declarations
└── api.h             # API function declarations
```

## Module Breakdown

### 📦 `config.h / config.cpp`
Centralized configuration including:
- WiFi credentials
- API keys (Eleven Labs, Gemini)
- Hardware pins (SD card, audio)
- Recording parameters

### 💾 `storage.h / storage.cpp`
Manages persistent storage:
- `StorageInfo` - Storage system status
- `AudioFile` - Audio recording metadata
- `SummaryFile` - Summary metadata
- `ImageFile` - Generated infographic metadata
- SD card and PSRAM initialization

### 🎨 `display.h / display.cpp`
UI and rendering system:
- `AppState` - Navigation states (menu, recording, gallery, etc.)
- `Button` - Touchable UI elements
- Display functions for menu, lists, text, gallery
- Status bar with WiFi and storage info

### 🎙️ `audio.h / audio.cpp`
Audio recording management:
- `startAudioRecording()` - Begin recording
- `stopAudioRecording()` - End recording and save
- `getRecentAudioFiles()` - List recorded audio

### 🌐 `api.h / api.cpp`
API communication layer:
- `transcribeAudio()` - Eleven Labs speech-to-text
- `summarizeText()` - Gemini text summarization
- `generateInfographic()` - Gemini image generation
- Structured result types with error handling

### 🚀 `main.cpp`
Application orchestration:
- Setup WiFi and hardware
- Main event loop and touch handling
- State machine for app navigation
- Integration of all modules

## Hardware Requirements

- **M5Stack PaperS3** with ESP32-S3
- Microphone (built-in on PaperS3)
- Optional: microSD card for extended storage
- WiFi connection for API calls

## Dependencies

- **M5Unified** - M5Stack hardware abstraction
- **ArduinoJson** (v7.x) - JSON parsing
- **WiFi** - Network connectivity (built-in)
- **HTTPClient** - HTTP requests (built-in)

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

### 2. Configure Credentials
Edit `src/config.cpp` and update:
```cpp
const char* WIFI_SSID = "Your WiFi SSID";
const char* WIFI_PASS = "Your WiFi Password";
const char* ELEVEN_LABS_API_KEY = "your-api-key";
const char* GEMINI_API_KEY = "your-api-key";
```

### 3. Build and Upload
```bash
# Build for PaperS3
pio run --environment PaperS3 --target build

# Upload to device
pio run --environment PaperS3 --target upload

# View serial output
pio device monitor
```

## API Integration

### Eleven Labs (Transcription)
- Endpoint: `https://api.elevenlabs.io/v1/speech-to-text`
- Get API key: https://elevenlabs.io/

### Google Gemini (Summarization & Infographics)
- Endpoints:
  - Text: `https://generativelanguage.googleapis.com/v1beta/models/gemini-1.5-flash:generateContent`
  - Images: `https://generativelanguage.googleapis.com/v1alpha/models/gemini-3-pro-image-preview:generateContent`
- Get API key: https://aistudio.google.com

## Usage

1. **Main Menu** - Select from Record, Audio Files, Summaries, or Gallery
2. **Record** - Press Record button, speak, tap to stop
3. **Process** - Transcribe, summarize, and generate infographics
4. **View** - Browse summaries and generated infographics in the gallery

## Development Tips

### Adding New Modules
1. Create `include/mymodule.h` with declarations
2. Create `src/mymodule.cpp` with implementation
3. Include in `main.cpp`
4. PlatformIO automatically compiles and links

### Debugging
Enable serial output at 115200 baud. Monitor messages prefixed with `[MODULE]` (e.g., `[STORAGE]`, `[WiFi]`).

### Performance Considerations
- E-ink display refreshes are slow—batch updates when possible
- PSRAM is limited—stream large audio files when transcribing
- WiFi connection required for API calls

## Troubleshooting

**WiFi Won't Connect**
- Check SSID and password in `config.cpp`
- Verify PaperS3 is within range
- Check router supports 2.4 GHz (PaperS3 doesn't support 5 GHz)

**API Calls Fail**
- Verify API keys are correct
- Check WiFi connection
- Review HTTP response codes in serial output

**Storage Issues**
- Device defaults to PSRAM if SD card not detected
- Format SD card as FAT32
- Check pin definitions in `config.h`

## Future Enhancements

- [ ] Local ML model for transcription
- [ ] Voice commands and hotword detection
- [ ] Offline mode with local summarization
- [ ] Cloud sync for backups
- [ ] Battery optimization improvements
- [ ] Multi-language support

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
