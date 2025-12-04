# Module Documentation

## Overview

MindInk has been refactored into a modular architecture inspired by [PaperS3Weather](https://github.com/squirmen/PaperS3Weather). Each module handles a specific responsibility:

| Module | Responsibility |
|--------|-----------------|
| `config` | Configuration and constants |
| `storage` | Data structures and storage management |
| `display` | UI rendering and navigation |
| `audio` | Audio recording functionality |
| `api` | External API communication |
| `main` | Application orchestration |

## Detailed Module Descriptions

### Config Module (`config.h`)

**Purpose**: Centralized configuration and hardware definitions

**Key Constants**:
- WiFi credentials
- API endpoints and keys
- Hardware pin definitions (SD card SPI pins)
- Audio recording parameters

**Usage**:
```cpp
#include "config.h"
// Access credentials
Serial.println(WIFI_SSID);
Serial.println(ELEVEN_LABS_API_KEY);
```

**When to Modify**:
- Changing WiFi network
- Updating API keys
- Changing audio sample rate
- Modifying SD card pins for different hardware

---

### Storage Module (`storage.h` / `storage.cpp`)

**Purpose**: Manage persistent storage and data structures

**Data Structures**:

```cpp
// Storage system info
struct StorageInfo {
    bool sdAvailable;           // SD card detected
    bool psramAvailable;        // PSRAM available
    size_t psramTotal;          // Total PSRAM bytes
    size_t psramFree;           // Available PSRAM bytes
    StorageType activeStorage;  // Which storage is active
};

// Audio file metadata
struct AudioFile {
    String filename;     // Filename of audio recording
    String dateTime;     // Recording timestamp
    StorageType storage; // Where it's stored (SD or PSRAM)
};

// Summary file metadata
struct SummaryFile {
    String filename;
    String relatedAudioFilename; // Which audio was summarized
    String dateTime;
    StorageType storage;
};

// Generated image metadata
struct ImageFile {
    String originalFilename;  // Infographic filename
    String summaryFilename;   // Related summary
    String dateTime;
    StorageType storage;
};
```

**Public Functions**:

```cpp
void initStorage();              // Initialize SD card and PSRAM
StorageInfo getStorageInfo();    // Get current storage status
```

**Global Variables**:
- `StorageInfo storage` - Current storage state (accessible from other modules)

**When to Modify**:
- Changing SD card pin configuration
- Adding new storage types
- Modifying storage priority (e.g., prefer PSRAM over SD)

---

### Display Module (`display.h` / `display.cpp`)

**Purpose**: UI rendering and user interaction

**App States**:
```cpp
enum AppState {
    STATE_MENU,              // Main menu
    STATE_RECORDING,         // Recording in progress
    STATE_LIST_AUDIO,        // List of audio files
    STATE_LIST_SUMMARIES,    // List of summaries
    STATE_VIEW_SUMMARY,      // View single summary
    STATE_GALLERY,           // Image gallery
    STATE_PROCESSING,        // Processing (waiting)
    STATE_ERROR              // Error display
};
```

**UI Components**:
```cpp
struct Button {
    int x, y, w, h;    // Position and size
    String label;      // Button text
};
```

**Public Functions**:

```cpp
// Setup
void setupButtons();                    // Initialize UI buttons

// Interaction
bool checkButtonPress(int x, int y, const Button& btn);

// Drawing
void drawButton(const Button& btn);     // Draw single button
void displayStatusBar();                // WiFi and storage status
void drawMenu();                        // Main menu
void drawList(String title, const std::vector<String>& items);
void drawTextView(String text);         // Display summary text
void drawGallery(const std::vector<ImageFile>& images);
void drawProcessing(const String& message = "Processing...");
void drawError(const String& message);
```

**Global Variables**:
- `std::vector<Button> menuButtons` - Menu buttons
- `AppState currentState` - Current app state
- `String currentTextContent` - Currently viewed text

**Design Notes**:
- Border-only button style (no filled background)
- White background with black text for e-ink display
- Centered text positioning for balance

**When to Modify**:
- Adding new UI screens
- Changing button layout or styling
- Modifying color scheme
- Adding animations or transitions

---

### Audio Module (`audio.h` / `audio.cpp`)

**Purpose**: Audio recording functionality

**Global Variables**:
```cpp
bool isRecording;      // Recording status
File recordingFile;    // Current recording file
```

**Public Functions**:

```cpp
void startAudioRecording(AudioFile& audioFile);
void stopAudioRecording();
void getRecentAudioFiles(std::vector<AudioFile>& files);
```

**Implementation Notes**:
- Currently stubbed—awaiting I2S audio driver implementation
- Will use M5Stack PaperS3's built-in microphone
- Records to either SD card or PSRAM based on availability
- Output format: 16-bit PCM WAV at 16 kHz

**When to Modify**:
- Adding actual audio recording
- Changing audio format or bitrate
- Adding audio playback
- Implementing audio preprocessing (filtering, normalization)

---

### API Module (`api.h` / `api.cpp`)

**Purpose**: Handle external API communication

**Result Structures**:

```cpp
struct TranscriptionResult {
    bool success;      // Operation successful
    String text;       // Transcribed text
    String error;      // Error message if failed
};

struct SummaryResult {
    bool success;
    String text;       // Summarized text
    String error;
};

struct InfographicResult {
    bool success;
    String imageData;  // Base64 encoded image
    String error;
};
```

**Public Functions**:

```cpp
// Eleven Labs transcription
TranscriptionResult transcribeAudio(const String& audioFilePath);

// Gemini summarization
SummaryResult summarizeText(const String& text);

// Gemini infographic generation
InfographicResult generateInfographic(const String& summaryText);
```

**Implementation Notes**:
- Currently stubbed—awaiting API implementation
- Requires WiFi connection
- Uses HTTP/HTTPS for communication
- Includes error handling and retry logic
- Parses JSON responses with ArduinoJson

**API Details**:

| API | Endpoint | Auth | Model |
|-----|----------|------|-------|
| Eleven Labs | `/v1/speech-to-text` | API Key | Automatic |
| Gemini Text | `/v1beta/models/gemini-1.5-flash` | API Key | 1.5 Flash |
| Gemini Image | `/v1alpha/models/gemini-3-pro-image-preview` | API Key | 3 Pro |

**When to Modify**:
- Implementing actual API calls
- Adding new APIs or models
- Changing request/response formats
- Adding caching or offline support

---

### Main Module (`main.cpp`)

**Purpose**: Application orchestration and main loop

**Key Functions**:

```cpp
void initWiFi();   // Connect to WiFi network
void setup();      // Initialize all systems
void loop();       // Main event loop
```

**Setup Sequence**:
1. Initialize M5Unified
2. Set up display
3. Initialize storage
4. Connect WiFi
5. Initialize UI buttons
6. Draw main menu

**Main Loop**:
1. Update M5 (sensors, touch)
2. Check for touch input
3. Process button presses based on current state
4. Update display if needed
5. Small delay to reduce CPU usage

**Global Variables**:
```cpp
std::vector<AudioFile> recentAudio;        // Recent recordings
std::vector<SummaryFile> recentSummaries;  // Recent summaries
std::vector<ImageFile> recentImages;       // Generated images
```

**State Transitions**:
```
MENU → RECORDING → MENU
MENU → LIST_AUDIO → MENU
MENU → LIST_SUMMARIES → VIEW_SUMMARY → MENU
MENU → GALLERY → MENU
any → PROCESSING → previous state
any → ERROR → MENU
```

**When to Modify**:
- Changing startup sequence
- Adding new states or transitions
- Modifying touch handling
- Adjusting refresh rates or delays

---

## Integration Example

To add a new feature, modify these files in order:

### 1. Define Data Structure (storage.h)
```cpp
struct MyData {
    String id;
    String value;
};
```

### 2. Add Module Header (mymodule.h)
```cpp
#ifndef MYMODULE_H
#define MYMODULE_H
#include "storage.h"

void myFunction(const MyData& data);

#endif
```

### 3. Implement Module (mymodule.cpp)
```cpp
#include "mymodule.h"
#include <M5Unified.h>

void myFunction(const MyData& data) {
    Serial.printf("[MYMODULE] Processing %s\n", data.id.c_str());
    // Implementation
}
```

### 4. Integrate into Main (main.cpp)
```cpp
#include "mymodule.h"

void setup() {
    // existing setup code
    myFunction(myData);
}
```

---

## Best Practices

### Code Organization
- Keep each module focused on one responsibility
- Use descriptive names for functions and variables
- Include debug logging with module prefix `[MODULE_NAME]`

### Memory Management
- Use `String` class instead of raw char arrays
- Clear vectors when done to free memory
- Check PSRAM availability before allocating large buffers

### Error Handling
- Return structured result types (TranscriptionResult, etc.)
- Include error messages in results
- Log errors to serial for debugging

### Performance
- Avoid blocking operations in main loop
- Use `delay()` sparingly
- Batch display updates when possible
- Monitor free heap with `Serial.printf("[MEMORY] %d\n", ESP.getFreeHeap());`

---

## Compilation and Linking

PlatformIO automatically:
- Includes all `.h` files in `include/`
- Compiles all `.cpp` files in `src/`
- Links everything into final binary
- Manages library dependencies in `platformio.ini`

No explicit includes needed—just include headers as needed.

