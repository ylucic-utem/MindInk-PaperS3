# MindInk - M5Stack PaperS3

MindInk is a powerful, portable note-taking and infographic companion for the M5Stack PaperS3 e-ink display. It is designed to be your intelligent productivity partner, capable of recording audio, transcribing speech, auto-generating summaries, and even creating visual infographics—all processed directly on the device with local caching.

## Key Features

- **Local Audio Recording**: Capture voice notes directly on your M5Stack PaperS3.
- **On-Device Orchestration**: Efficiently manages the flow from recording to processing without complex edge functions.
- **AI-Powered**:
    - **Transcription**: Uses Eleven Labs API to convert speech to accurate text.
    - **Summarization**: Leveraging Google's Gemini API for concise, actionable summaries.
    - **Infographics**: Visually represent your notes with generated infographics.
- **Offline Capable**: All audio, transcripts, and summaries are cached to the SD card, allowing you to access your content anywhere.
- **Supabase Integration**: Seamlessly syncs data with the cloud for backup and multi-device access.
- **E-Ink Optimized UI**: A custom-designed interface built for the unique properties of the PaperS3 display.

## Getting Started

This repository contains the firmware source code for the M5Stack PaperS3. 

### Prerequisites
- **Hardware**: M5Stack PaperS3, microSD card (FAT32 formatted).
- **Software**: PlatformIO (VS Code extension recommended).
- **API Keys**: You will need API keys for Eleven Labs and Google Gemini (configured in `src/config.cpp`).

### Build & Upload
1.  Clone the repository.
2.  Open in PlatformIO.
3.  Configure your credentials in `src/config.cpp` (see `src/config.h` for details).
4.  Run `pio run --target upload` to flash the firmware.

## Documentation

Detailed documentation, implementation plans, and troubleshooting guides are available locally in the `MD/` directory of this repo (excluded from git tracking).

## License

This project is licensed under the MIT License.
