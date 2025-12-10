# Quick Start Guide

## First Time Setup (5 minutes)

### 1. Prerequisites
- PlatformIO installed (VSCode extension or CLI)
- USB cable for M5Stack PaperS3
- WiFi network details

### 2. Clone and Configure
```bash
# Clone the repository
git clone https://github.com/ylucic-utem/MindInk-PaperS3.git
cd MindInk-PaperS3

# Open in VSCode
code .
```

### 3. Get API Keys
1. **Eleven Labs** (Transcription)
   - Sign up: https://elevenlabs.io/
   - Copy API key from settings

2. **Google Gemini** (Summarization & Infographics)
   - Sign up: https://aistudio.google.com
   - Create API key

### 4. Update Configuration
Edit `src/main.cpp` lines 39-44:

```cpp
const char* WIFI_SSID     = "Your WiFi Network";
const char* WIFI_PASS     = "Your WiFi Password";

const char* ELEVEN_LABS_API_KEY = "your-eleven-labs-key-here";
const char* GEMINI_API_KEY      = "your-gemini-key-here";
```

⚠️ **Important**: These are credentials. Consider using environment variables or secure storage for production.

### 5. Connect PaperS3
- Plug M5Stack PaperS3 into your computer via USB

### 6. Build and Upload
In VSCode terminal or PlatformIO CLI:

```bash
# Build
pio run --environment PaperS3

# Upload to device
pio run --environment PaperS3 --target upload

# Monitor serial output
pio device monitor --environment PaperS3
```

Or use VSCode PlatformIO sidebar:
- Click "Upload" in the PlatformIO Home
- Click "Serial Monitor" to watch compilation and runtime output

### 7. Test
After upload completes (1-2 minutes):
- Device display should show "MindInk" splash screen
- Should connect to WiFi automatically
- Main menu will appear

## Main Menu Navigation

Once running, you'll see the main menu with 4 buttons:

| Button | Function |
|--------|----------|
| **RECORD** | Start audio recording (tap to stop) |
| **AUDIO FILES** | Browse recorded audio |
| **SUMMARIES** | View transcribed and summarized text |
| **GALLERY** | View generated infographics |

## Common Tasks

### Recording Your First Note
1. Tap **RECORD** button
2. Speak clearly (audio is being recorded)
3. Tap screen again to stop
4. Device processes (shows "Processing..." screen)

### Processing Flow
1. Audio → Transcribed with Eleven Labs
2. Transcription → Summarized with Gemini
3. Summary → Infographic generated
4. Results saved and displayed

### Viewing Your Work
- **AUDIO FILES** - See your recordings
- **SUMMARIES** - Read transcriptions and summaries
- **GALLERY** - View generated infographics

## Troubleshooting

### Upload Fails
```bash
# Check USB connection
pio device list

# Try with different baud rate
pio run --environment PaperS3 --target upload --upload-speed 115200
```

### WiFi Won't Connect
- Check SSID and password in `main.cpp`
- PaperS3 only supports 2.4 GHz WiFi (not 5 GHz)
- Verify network is accessible

### API Calls Fail
- Check API keys are correct
- Verify WiFi is connected (see status bar)
- Check serial console for error details

### Serial Monitor Not Working
```bash
# List available ports
pio device list

# Monitor specific port
pio device monitor --port COM4 --baud 115200
```

## Development Tips

### Enabling Debug Output
The code includes debug logging. Check serial output:
```
[STORAGE] Initializing storage systems...
[DISPLAY] PAPERS3 display initialized
[WiFi] Connected!
[SETUP] Complete
```

### Modifying UI
Edit display functions in `src/display.cpp`:
- `drawMenu()` - Main menu layout
- `drawButton()` - Button styling
- `displayStatusBar()` - Top status bar

### Adding Features
See `ARCHITECTURE.md` for module structure and integration examples.

### Performance Monitoring
Add to `main.cpp` loop:
```cpp
Serial.printf("[MEMORY] Free Heap: %d bytes\n", ESP.getFreeHeap());
```

## Next Steps

1. **Read ARCHITECTURE.md** - Understand the modular structure
2. **Explore the code** - Each module is well-commented
3. **Customize** - Modify styling, add new features
4. **Extend** - Add new modules for new capabilities

## Resources

- **M5Stack PaperS3 Docs**: https://docs.m5stack.com/en/core/papers3
- **ESP32-S3 Datasheet**: https://www.espressif.com/sites/default/files/documentation/esp32-s3_datasheet_en.pdf
- **Eleven Labs API**: https://api.elevenlabs.io/docs
- **Gemini API**: https://ai.google.dev/docs
- **Arduino Reference**: https://www.arduino.cc/reference/en/

## Getting Help

1. Check serial console output for errors (starts with `[MODULE_NAME]`)
2. Review module documentation in `include/` headers
3. Check GitHub issues: https://github.com/ylucic-utem/MindInk-PaperS3/issues
4. Open a new issue with error details and serial output

Good luck! 🚀
