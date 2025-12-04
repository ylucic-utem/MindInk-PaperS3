# M5Unified Audio Implementation Reference

## Overview

This document details the M5Unified microphone integration for the MindInk-PaperS3 project. The implementation leverages the official M5Stack `Mic_Class` from M5Unified to provide professional-grade I2S audio recording on the ESP32-S3.

**Sources:**
- https://github.com/m5stack/M5Unified/blob/master/src/utility/Mic_Class.cpp (1500+ lines)
- https://github.com/m5stack/M5Unified/blob/master/src/utility/Mic_Class.hpp

## Architecture

### M5Unified Microphone Task Model

M5Unified implements audio recording using a **background FreeRTOS task** that continuously collects I2S samples via DMA:

```
┌─────────────────────────────────────────┐
│     Application Main Loop               │
│  (can continue to handle UI, etc.)      │
└────────────────┬────────────────────────┘
                 │
                 │ Calls M5.Mic.begin()
                 ▼
┌─────────────────────────────────────────┐
│   FreeRTOS "mic_task" (Background)     │
│                                         │
│  ┌────────────────────────────────────┐ │
│  │  I2S DMA FIFO                      │ │
│  │  (Automatically fills from mic)    │ │
│  └────────┬───────────────────────────┘ │
│           │                              │
│           ▼                              │
│  ┌────────────────────────────────────┐ │
│  │  Signal Processing:                │ │
│  │  - PDM to PCM conversion          │ │
│  │  - Gain adjustment                │ │
│  │  - Noise filtering (optional)     │ │
│  │  - Oversampling averaging        │ │
│  └────────┬───────────────────────────┘ │
│           │                              │
│           ▼                              │
│  ┌────────────────────────────────────┐ │
│  │  Double-Buffer Management:        │ │
│  │  - rec_info[0]: Current writing   │ │
│  │  - rec_info[1]: Ready to read     │ │
│  └────────────────────────────────────┘ │
└─────────────────────────────────────────┘
```

### Key Components

#### 1. **mic_config_t Structure**

```cpp
struct mic_config_t {
    int pin_data_in = -1;           // I2S DIN pin (required)
    int pin_bck = I2S_PIN_NO_CHANGE;    // Bit clock (optional for PDM)
    int pin_mck = I2S_PIN_NO_CHANGE;    // Master clock (optional)
    int pin_ws = I2S_PIN_NO_CHANGE;     // Word select/LRCK (optional for PDM)
    
    uint32_t sample_rate = 16000;   // 16kHz default
    input_channel_t input_channel = input_only_right;  // Mono/Stereo
    uint8_t over_sampling = 2;      // Oversample for quality
    uint8_t magnification = 16;     // Gain amplification
    uint8_t noise_filter_level = 0; // 0-255 noise threshold
    bool use_adc = false;           // ADC mode (ESP32 only)
    
    size_t dma_buf_len = 128;       // Samples per DMA buffer
    size_t dma_buf_count = 8;       // Number of DMA buffers
    uint8_t task_priority = 2;      // FreeRTOS task priority
    uint8_t task_pinned_core = -1;  // CPU core (-1 = any)
    i2s_port_t i2s_port = I2S_NUM_0;
};
```

#### 2. **M5Stack PaperS3 Specific Configuration**

For PaperS3, the microphone is connected via **PDM mode** (Pulse Density Modulation):

```cpp
// M5Stack PaperS3 uses PDM microphone (1-bit, high frequency)
// Automatic conversion to PCM by I2S peripheral

auto mic_cfg = M5.Mic.config();
mic_cfg.sample_rate = 16000;        // 16kHz target output
mic_cfg.over_sampling = 2;          // 2x oversampling for accuracy
mic_cfg.magnification = 16;         // 16x gain
mic_cfg.pin_data_in = 2;            // GPIO2 for PDM data (PaperS3 default)
// BCK and WS pins not used in PDM mode (set to I2S_PIN_NO_CHANGE)

M5.Mic.config(mic_cfg);
M5.Mic.begin();
```

#### 3. **Microphone Task Flow (Simplified)**

The FreeRTOS `mic_task` runs continuously:

```cpp
void Mic_Class::mic_task(void* args) {
    // 1. Calculate clock dividers for desired sample rate
    uint32_t div_a, div_b, div_n;
    calcClockDiv(&div_a, &div_b, &div_n, PLL_D2_CLK, sample_rate * oversampling);
    
    // 2. Configure I2S registers for PDM mode (PaperS3)
    i2s_config.clk_cfg.clk_src = I2S_CLK_SRC_PLL_160M;
    i2s_config.slot_cfg.data_bit_width = I2S_DATA_BIT_WIDTH_16BIT;
    i2s_config.slot_cfg.slot_mode = I2S_SLOT_MODE_MONO;  // Mono input
    err = i2s_channel_init_pdm_rx_mode(...);  // PDM receive mode
    
    // 3. Start I2S and wait for data
    i2s_channel_enable(_i2s_handle[port]);
    
    // 4. Main recording loop
    while (_task_running) {
        // Read raw samples from I2S DMA FIFO
        i2s_channel_read(handle, src_buf, dma_buf_len, &src_len, timeout);
        
        // Process samples: oversampling, gain, noise filtering
        for each sample in src_buf:
            apply_gain(sample, magnification);
            apply_noise_filter(sample);
            accumulate_oversample(sample);
        
        // Write processed samples to double buffer
        write_to_buffer(processed_samples, &rec_info[_rec_flip]);
        
        // Swap buffer when full
        if (rec_info[_rec_flip].length == 0) {
            _rec_flip = !_rec_flip;
            signal_buffer_ready();
        }
    }
}
```

## Implementation in MindInk

### In `include/audio.h`

```cpp
// WAV format output
struct WAVHeader {
    uint32_t riff = 0x46464952;  // "RIFF"
    uint32_t fileSize;            // Total file size - 8
    uint32_t wave = 0x45564157;  // "WAVE"
    uint32_t fmt = 0x20746d66;   // "fmt "
    // ... PCM format parameters
};

// Configuration constants
#define AUDIO_SAMPLE_RATE 16000         // 16kHz
#define AUDIO_CHANNELS 1                // Mono
#define AUDIO_BITS_PER_SAMPLE 16        // 16-bit PCM
#define AUDIO_MAX_RECORDING_SECONDS 60  // 1 minute max
```

### In `src/audio.cpp`

#### Initialization with M5Unified

```cpp
void initAudio() {
    // Configure microphone
    auto mic_cfg = M5.Mic.config();
    mic_cfg.sample_rate = AUDIO_SAMPLE_RATE;    // 16kHz
    mic_cfg.over_sampling = 2;                  // 2x oversampling
    mic_cfg.magnification = 16;                 // 16x gain
    M5.Mic.config(mic_cfg);
    
    // Allocate recording buffer
    allocateAudioBuffer();  // 1.92MB in PSRAM (or smaller in SRAM)
}
```

#### Start Recording

```cpp
bool startAudioRecording() {
    audioBufferIndex = 0;
    recordingByteCount = 0;
    
    // Start M5Unified microphone task
    if (!M5.Mic.begin()) {
        Serial.println("Microphone start failed!");
        return false;
    }
    
    isRecording = true;
    // Microphone task now runs in background, collecting samples via I2S DMA
    return true;
}
```

#### Stop Recording & Save

```cpp
bool stopAudioRecording(AudioFile& audioFile) {
    M5.Mic.end();  // Stop background microphone task
    isRecording = false;
    
    // Write WAV file with collected audio
    File file = SD.open(filename, FILE_WRITE);
    writeWAVHeader(file, recordingByteCount);  // PCM header
    file.write(audioBuffer, recordingByteCount);  // Audio data
    file.close();
    
    return true;
}
```

## I2S Configuration Details

### Clock Generation

For 16kHz sampling on PaperS3:

```
PLL_160M = 160 MHz base clock
Target output: 16 kHz * 16-bit * 1-channel = 256 kHz I2S clock

Divider calculation:
div_m = 8 (divider for BCK)
div_n, div_a, div_b calculated to produce exact 16 kHz

Formula: f_out = (PLL_160M * div_a) / ((div_n * div_a + div_b) * div_m)
```

### PDM Mode Conversion

PaperS3 microphone outputs PDM (1-bit, ~2.4 MHz):

```
I2S Peripheral automatically converts PDM → PCM:
- Decimation: 64 or 128 (configurable)
- Low-pass filter removes high-frequency PDM carrier
- Output: 16-bit PCM at target sample rate
```

### DMA Configuration

```cpp
// Double buffering ensures continuous recording
i2s_chan_config_t chan_cfg = I2S_CHANNEL_DEFAULT_CONFIG(I2S_NUM_0, I2S_ROLE_MASTER);
chan_cfg.dma_desc_num = 8;      // 8 DMA descriptors
chan_cfg.dma_frame_num = 128;   // 128 frames per buffer (2KB per buffer)

// Total DMA memory: 8 * 128 * 2 bytes = 2KB (very small!)
// Microphone task moves data to larger recording buffer
```

## Buffer Management

### Ring Buffer Strategy

```
┌──────────────────────────────┐
│   Audio Recording Buffer     │
│   (PSRAM: 1.92MB for 60sec)  │
│                              │
│  ┌────────────────────────┐  │
│  │ DMA writes here        │  │
│  │ (audioBufferIndex)     │  │
│  └────────────────────────┘  │
│           │                   │
│           │ On stopRecord()    │
│           ▼                   │
│  ┌────────────────────────┐  │
│  │ Write to WAV file      │  │
│  │ (with header)          │  │
│  └────────────────────────┘  │
└──────────────────────────────┘
```

### Memory Layout

```
16kHz * 1 channel * 2 bytes/sample * 60 seconds = 1,920,000 bytes (1.92 MB)

PSRAM allocation:
- Available on ESP32-S3 with PSRAM (typical 8MB)
- Allocate on startup via ps_malloc()
- Fallback to smaller SRAM buffer if PSRAM unavailable

WAV file format:
[44 bytes] WAV header (fixed size)
[variable] PCM audio data (16-bit little-endian)
```

## Gain and Signal Processing

### Automatic Gain Control (AGC)

```cpp
// M5Unified applies gain at audio collection time
int32_t gain = _cfg.magnification;  // 1-255
const float f_gain = (float)gain / (oversampling << 1);

// Applied to each sample:
for (int i = 0; i < 2; ++i) {
    sum_value[i] *= f_gain;  // Amplify weak signals
}
```

### Noise Filtering

```cpp
// Optional noise gate reduces low-level noise
uint8_t noise_filter_level = 0;  // 0 = disabled, 255 = strong

if (noise_filter) {
    for (int i = 0; i < 2; ++i) {
        int32_t v = (sum_value[i] * (256 - noise_filter) + 
                     prev_value[i] * noise_filter + 128) >> 8;
        prev_value[i] = v;
        sum_value[i] = v * f_gain;
    }
}
```

### Automatic Zero-Level Offset

M5Unified compensates for DC offset in microphone signal:

```cpp
int32_t offset = self->_offset;

// Automatic zero level adjustment
offset -= (value_tmp + offset + 16) >> 5;  // Slow IIR filter
self->_offset = offset;

// Apply offset to samples
sum_value[0] = sv0 + offset;
sum_value[1] = sv1 + offset;
```

## Troubleshooting

### No Audio Recorded

1. Check microphone is enabled: `M5.Mic.isRecording()` returns > 0
2. Verify pin configuration: PaperS3 default is GPIO2
3. Check PSRAM: `psramFound()` returns true
4. Monitor buffer fill: Print `audioBufferIndex` periodically

### Noise in Recording

1. Increase `magnification` gradually (current: 16)
2. Enable noise filter: `mic_cfg.noise_filter_level = 32`
3. Verify external power supply (microphone is sensitive to noise)
4. Check cable connections for physical microphonics

### Recording Duration Limits

- Current: 60 seconds max (1.92MB PSRAM)
- To extend: Increase `AUDIO_MAX_RECORDING_SECONDS` and ensure PSRAM available
- Alternative: Stream to SD card instead of buffering (requires `readRawData()` implementation)

## Advanced Topics

### Stereo Recording

To record stereo, modify configuration:

```cpp
mic_cfg.input_channel = input_stereo;  // or input_only_left
mic_cfg.stereo = true;
// Update WAV header: numChannels = 2, byteRate *= 2, etc.
```

### Sample Rate Conversion

To record at 48kHz instead:

```cpp
mic_cfg.sample_rate = 48000;  // Will adjust I2S clock dividers
// Update AUDIO_SAMPLE_RATE constant
// Update WAV header accordingly
```

### Direct-to-File Streaming

Instead of buffering, stream to file:

```cpp
// Requires M5.Mic.readRawData() to be exposed (currently internal)
while (recording) {
    size_t bytesRead = M5.Mic.readRawData(tempBuffer, BUFFER_SIZE);
    file.write(tempBuffer, bytesRead);
}
```

## References

- **M5Unified GitHub**: https://github.com/m5stack/M5Unified
- **Mic_Class Source**: https://github.com/m5stack/M5Unified/blob/master/src/utility/Mic_Class.cpp
- **ESP32-S3 I2S Manual**: https://docs.espressif.com/projects/esp-idf/en/latest/esp32s3/api-reference/peripherals/i2s.html
- **PaperS3 Schematic**: M5Stack documentation (microphone connected to GPIO2 via PDM)
