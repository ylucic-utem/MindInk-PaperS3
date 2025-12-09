# Quick Start Guide - Local Processing

## What Changed? 🎉

Your MindInk PaperS3 device now processes audio **locally** using the SD card! No more Edge Function issues.

## How It Works

```
You → Select Audio → Device downloads to SD card
                  ↓
            Device sends to ElevenLabs API
                  ↓
            Saves transcript to SD card
                  ↓
            Device sends to Gemini API
                  ↓
            Saves summary to SD card
                  ↓
            Shows summary on screen ✓
```

## SD Card Setup

### 1. Format Your SD Card
- Use FAT32 format
- Format on your computer (not on device)
- Insert into PaperS3

### 2. First Boot
The device will automatically create these folders:
```
/audio/         - Downloaded audio files
/transcripts/   - Transcription results
/summaries/     - AI summaries
/infographics/  - Generated images
/recordings/    - Your voice recordings
```

## Usage Example (Serial Monitor Test)

### Test 1: Check SD Card
```cpp
// In Serial Monitor, you should see:
[STORAGE] SD Card initialized
[STORAGE] SD Card is properly formatted
[STORAGE] Folder exists: /audio
[STORAGE] Folder exists: /transcripts
[STORAGE] Folder exists: /summaries
[STORAGE] SD folder structure ready
```

### Test 2: Process an Audio File
```cpp
// Add this to your code or button handler:
String audioId = "62cd9c95-51d2-434e-8baf-1eeef165a6a0"; // From Supabase
bool success = processAudioLocally(audioId);

// Watch Serial Monitor:
[LOCAL PROCESSING] Starting workflow for audio: 62cd9c95...
[STEP 1/5] Downloading audio from Supabase...
[LOCAL PROCESSING] Audio downloaded to: /audio/62cd9c95...webm
[STEP 2/5] Transcribing audio with ElevenLabs...
[API] Transcription complete (125 chars)
[STEP 3/5] Saving transcript to SD card...
[LOCAL PROCESSING] Transcript saved successfully
[STEP 4/5] Generating summary with Gemini...
[API] Summary complete (68 chars)
[STEP 5/5] Saving summary to SD card...
[LOCAL PROCESSING] Summary saved successfully
[LOCAL PROCESSING] Workflow complete!
```

## Integration with Main Menu

To add a "Process Audio" button to your menu, add this to `main.cpp`:

```cpp
// Add after other menu buttons in setup()
else if (checkButtonPress(tx, ty, menuButtons[4])) { // New button
    // Show audio list from Supabase
    std::vector<AudioFile> audioList;
    if (fetchSupabaseAudio(audioList)) {
        // Let user select audio
        String selectedId = audioList[0].id; // First audio for testing
        
        // Show "Processing..." screen
        M5.Display.clear();
        M5.Display.setCursor(50, 100);
        M5.Display.setTextSize(2);
        M5.Display.println("Processing...");
        
        // Process locally
        if (processAudioLocally(selectedId)) {
            // Success! Read and display summary
            String summary;
            readSummaryLocally(selectedId + "_summary", summary);
            
            M5.Display.clear();
            M5.Display.setCursor(10, 10);
            M5.Display.setTextSize(1);
            M5.Display.println("Summary:");
            M5.Display.println(summary);
        } else {
            M5.Display.clear();
            M5.Display.println("Error processing audio");
        }
    }
}
```

## API Keys Required

Make sure you have these set in `src/config.cpp`:

```cpp
const char* ELEVEN_LABS_API_KEY = "sk_your_elevenlabs_key_here";
const char* GEMINI_API_KEY = "AIza_your_gemini_key_here";
```

Get them from:
- ElevenLabs: https://elevenlabs.io/app/settings/api-keys
- Gemini: https://aistudio.google.com/apikey

## Testing Checklist

- [ ] SD card formatted as FAT32
- [ ] SD card inserted in device
- [ ] Device boots and shows folder creation logs
- [ ] WiFi connected
- [ ] API keys configured in config.cpp
- [ ] Audio available in Supabase
- [ ] Call `processAudioLocally(audioId)` from button or test code

## Advantages Over Old System

✅ **No Edge Function needed** - Device talks directly to APIs  
✅ **Easier debugging** - See everything in Serial Monitor  
✅ **Offline mode** - Cached results work without WiFi  
✅ **Faster** - No Edge Function overhead  
✅ **Local storage** - All files on SD card  
✅ **Full control** - Device manages entire workflow  

## File Locations on SD Card

After processing audio ID `62cd9c95-51d2-434e-8baf-1eeef165a6a0`:

```
/audio/62cd9c95-51d2-434e-8baf-1eeef165a6a0.webm          (downloaded audio)
/transcripts/62cd9c95-51d2-434e-8baf-1eeef165a6a0.txt     (transcription)
/summaries/62cd9c95-51d2-434e-8baf-1eeef165a6a0_summary.txt (summary)
```

## Next Steps

1. **Upload firmware** to your PaperS3 device
2. **Insert formatted SD card**
3. **Watch Serial Monitor** during boot
4. **Test with existing audio** from Supabase
5. **Integrate into UI** with menu button

## Troubleshooting

### "SD Card not found"
- Check SD card is properly inserted
- Try reformatting as FAT32
- Check pin definitions in config.h match your hardware

### "Audio ID not found in Supabase"
- Verify audio exists: https://supabase.com/dashboard/project/fptyrdjmrzabvimbzupv/editor
- Check WiFi is connected
- Ensure Supabase anon key is correct

### "Transcription failed"
- Check ElevenLabs API key is valid
- Verify audio file downloaded successfully (check SD card)
- Check Serial Monitor for HTTP error codes

### "Summary generation failed"  
- Check Gemini API key is valid
- Ensure transcript was saved successfully
- Check WiFi connection

## Performance

**Typical processing time for 1 minute of audio:**
- Download: 5-10 seconds
- Transcription: 10-30 seconds
- Summary: 5-10 seconds
- **Total: ~30-60 seconds**

**SD Card space used:**
- Audio: 100-500 KB per file
- Transcript: 1-5 KB per file  
- Summary: 500 bytes per file

A 1GB SD card can store **hundreds** of processed audio files!

## Success! 🎉

You're now running the new local processing architecture. The device has full control and everything is cached locally on the SD card.

Enjoy your MindInk PaperS3! 📝✨
