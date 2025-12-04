# Setup Guide for MindInk

## ⚠️ Security First

Your API keys are **sensitive credentials**. Never commit them to version control.

## Step 1: Clone the Repository

```bash
git clone https://github.com/ylucic-utem/MindInk-PaperS3.git
cd MindInk-PaperS3
```

## Step 2: Create Your Config File

**IMPORTANT**: Configuration with credentials is handled separately for security.

```bash
# Copy the example config
cp src/config.cpp.example src/config.cpp
```

The file `src/config.cpp` is in `.gitignore` and will never be committed to git.

## Step 3: Add Your Credentials

Edit `src/config.cpp` and replace the placeholders:

```cpp
// WiFi Configuration
const char* WIFI_SSID = "Your_WiFi_Network";
const char* WIFI_PASS = "Your_WiFi_Password";

// API Keys - Get these from:
// https://elevenlabs.io/ (Speech-to-text)
const char* ELEVEN_LABS_API_KEY = "sk_xxxxxxxxxxxxxxxxxx";

// https://aistudio.google.com/ (Summarization & Image generation)
const char* GEMINI_API_KEY = "AIzaxxxxxxxxxxxxxxxx";
```

⚠️ **Never commit `src/config.cpp` to git!**

## Step 4: Install Dependencies

```bash
# Using PlatformIO CLI
pio lib install "M5Unified" "ArduinoJson"

# Or let PlatformIO auto-install from platformio.ini
```

## Step 5: Configure Hardware

Update hardware pin definitions if needed in `include/config.h`:

```cpp
#define SD_CS   47    // SD card chip select
#define SD_SCK  39    // SD card clock
#define SD_MOSI 38    // SD card MOSI
#define SD_MISO 40    // SD card MISO
```

These are correct for M5Stack PaperS3 by default.

## Step 6: Build and Upload

### Using PlatformIO CLI:

```bash
# Build
pio run --environment PaperS3

# Upload to device
pio run --environment PaperS3 --target upload

# Monitor serial output
pio device monitor
```

### Using VSCode PlatformIO Extension:

1. Click the PlatformIO icon in the left sidebar
2. Select "esp32-s3-devkitm-1" environment
3. Click "Upload" to build and upload
4. Click "Monitor" to see serial output

## Step 7: Verify Installation

Connect to the device's serial port at 115200 baud and you should see:

```
=== MindInk - M5 Paper S3 (Refactored) ===
[DISPLAY] PAPERS3 display initialized
[STORAGE] Initializing storage systems...
[WiFi] Connecting...
[WiFi] Connected!
[SETUP] Complete
```

## Troubleshooting

### Build Fails: "config.h: No such file or directory"

- Ensure you have the latest PlatformIO IDE/CLI
- Clean build: `pio run --environment PaperS3 --target clean`
- Rebuild: `pio run --environment PaperS3`

### Upload Fails: Port Not Found

- Check USB connection to M5Stack PaperS3
- Install USB drivers: https://docs.m5stack.com/en/download
- Set correct port in `platformio.ini`: `upload_port = COM3` (or your port)

### WiFi Won't Connect

- Verify SSID and password in `src/config.cpp`
- Check if router supports 2.4 GHz (PaperS3 doesn't support 5 GHz)
- Check router for MAC filtering
- Monitor serial output for connection details

### API Calls Fail

- Verify API keys are correct in `src/config.cpp`
- Check WiFi connection with `pio device monitor`
- Confirm API key permissions at:
  - https://elevenlabs.io/app/settings
  - https://aistudio.google.com/app/apikey

### Low Memory

- Monitor free heap: Check serial output for `[MEMORY]` messages
- Reduce buffer sizes in `include/config.h` if needed
- Check for memory leaks: `delete` large allocations after use

## Next Steps

- Read [ARCHITECTURE.md](./ARCHITECTURE.md) to understand the modular structure
- Implement remaining API calls in `src/api.cpp`
- Implement audio recording in `src/audio.cpp`
- Customize UI styling in `src/display.cpp`

## Getting Help

- Check serial console output for error messages (prefixed with `[MODULE_NAME]`)
- Review [ARCHITECTURE.md](./ARCHITECTURE.md) for module documentation
- Open an issue on GitHub with error logs
- Check [PlatformIO documentation](https://docs.platformio.org/)

## Security Checklist

- [ ] Created `src/config.cpp` from `src/config.cpp.example`
- [ ] Added your WiFi credentials to `src/config.cpp`
- [ ] Added your API keys to `src/config.cpp`
- [ ] Verified `src/config.cpp` is in `.gitignore`
- [ ] Never pushed `src/config.cpp` to GitHub
- [ ] Double-checked credentials are not visible in git history

⚠️ If you accidentally commit credentials:
1. **Immediately regenerate all API keys** at their respective services
2. Force push a clean history (⚠️ affects all collaborators):
   ```bash
   git reset --hard <commit-before-leak>
   git push --force-with-lease origin main
   ```
3. Never use the old keys again

