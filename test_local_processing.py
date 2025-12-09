#!/usr/bin/env python3
"""
MindInk Local Processing Test Script

Tests the complete workflow:
1. Fetch audio files from Supabase
2. Download selected audio file
3. Transcribe with ElevenLabs API
4. Summarize with Google Gemini API
5. Display results

Prerequisites:
- Python 3.8+
- pip install supabase-py python-dotenv requests google-genai elevenlabs
- .env file with:
  SUPABASE_URL=https://fptyrdjmrzabvimbzupv.supabase.co
  SUPABASE_ANON_KEY=eyJhbGciOiJIUzI1NiIsInR5cCI6IkpXVCJ9...
  ELEVEN_LABS_API_KEY=sk_...
  GEMINI_API_KEY=AIza...
"""

import os
import sys
import json
import time
from pathlib import Path
from typing import Optional, Dict, List
from dotenv import load_dotenv
import requests
from supabase import create_client, Client
from google import genai
from google.genai import types
from elevenlabs import ElevenLabs

# Load environment variables
load_dotenv()

SUPABASE_URL = os.getenv("SUPABASE_URL", "https://fptyrdjmrzabvimbzupv.supabase.co")
SUPABASE_ANON_KEY = os.getenv("SUPABASE_ANON_KEY")
ELEVEN_LABS_API_KEY = os.getenv("ELEVEN_LABS_API_KEY")
GEMINI_API_KEY = os.getenv("GEMINI_API_KEY")

# Validate environment variables
if not all([SUPABASE_ANON_KEY, ELEVEN_LABS_API_KEY, GEMINI_API_KEY]):
    print("❌ Missing environment variables!")
    print("Please create a .env file with:")
    print("  SUPABASE_ANON_KEY=...")
    print("  ELEVEN_LABS_API_KEY=...")
    print("  GEMINI_API_KEY=...")
    sys.exit(1)

# Initialize clients
supabase: Client = create_client(SUPABASE_URL, SUPABASE_ANON_KEY)
gemini_client = genai.Client(api_key=GEMINI_API_KEY)
elevenlabs_client = ElevenLabs(api_key=ELEVEN_LABS_API_KEY)

# Cache directory
CACHE_DIR = Path("./audio_cache")
CACHE_DIR.mkdir(exist_ok=True)


class MindInkTester:
    """Test the complete MindInk processing workflow."""
    
    def __init__(self):
        self.supabase = supabase
        self.cache_dir = CACHE_DIR
        self.gemini_client = gemini_client
        self.elevenlabs_client = elevenlabs_client
        
    def print_section(self, title: str):
        """Print a formatted section header."""
        print("\n" + "=" * 60)
        print(f"  {title}")
        print("=" * 60)
    
    def fetch_audio_list(self) -> List[Dict]:
        """Fetch audio files from Supabase."""
        self.print_section("STEP 1: Fetching Audio List from Supabase")
        
        try:
            response = self.supabase.table("audio_records").select(
                "id,file_name,storage_path,status,recording_date"
            ).order("recording_date", desc=True).limit(20).execute()
            
            audio_list = response.data
            
            if not audio_list:
                print("❌ No audio files found in Supabase")
                return []
            
            print(f"✅ Found {len(audio_list)} audio files:")
            print()
            
            for i, audio in enumerate(audio_list, 1):
                status = audio.get("status", "unknown")
                filename = audio.get("file_name", "unknown")
                print(f"  [{i}] {filename}")
                print(f"      ID: {audio['id']}")
                print(f"      Status: {status}")
                if audio.get("recording_date"):
                    print(f"      Date: {audio['recording_date']}")
                print()
            
            return audio_list
        
        except Exception as e:
            print(f"❌ Error fetching audio: {e}")
            return []
    
    def select_audio(self, audio_list: List[Dict]) -> Optional[Dict]:
        """Let user select an audio file."""
        if not audio_list:
            return None
        
        while True:
            try:
                choice = input(f"Select audio to process (1-{len(audio_list)}): ").strip()
                idx = int(choice) - 1
                
                if 0 <= idx < len(audio_list):
                    selected = audio_list[idx]
                    print(f"\n✅ Selected: {selected['file_name']}")
                    return selected
                else:
                    print(f"❌ Please enter a number between 1 and {len(audio_list)}")
            except ValueError:
                print("❌ Please enter a valid number")
    
    def get_signed_url(self, storage_path: str) -> Optional[str]:
        """Get signed URL for audio download using Supabase SDK."""
        print("\n[INFO] Getting signed URL...")
        
        try:
            # Use Supabase storage API to get a signed URL
            # The URL will be valid for 1 hour
            response = self.supabase.storage.from_("audio-files").create_signed_url(
                path=storage_path,
                expires_in=3600
            )
            
            if response and "signedURL" in response:
                return response["signedURL"]
            elif isinstance(response, str):
                return response
            else:
                print(f"[INFO] Unexpected response format: {response}")
                return None
        
        except Exception as e:
            print(f"⚠️  Could not get signed URL: {e}")
            return None
    
    def download_audio(self, audio: Dict) -> Optional[Path]:
        """Download audio file from Supabase."""
        self.print_section("STEP 2: Downloading Audio from Supabase")
        
        audio_id = audio["id"]
        filename = audio["file_name"]
        storage_path = audio.get("storage_path", filename)
        
        cache_file = self.cache_dir / f"{audio_id}.webm"
        
        # Check if already cached
        if cache_file.exists():
            print(f"✅ Audio already cached: {cache_file}")
            print(f"📁 Path: {cache_file}")
            return cache_file
        
        print(f"Downloading: {filename}")
        print(f"Storage path: {storage_path}")
        
        try:
            # Method 1: Try to get signed URL from Supabase Storage
            signed_url = self.get_signed_url(storage_path)
            
            if signed_url:
                print(f"[INFO] Using signed URL")
                response = requests.get(signed_url)
                
                if response.status_code == 200:
                    with open(cache_file, "wb") as f:
                        f.write(response.content)
                    
                    size_kb = cache_file.stat().st_size / 1024
                    print(f"✅ Downloaded: {size_kb:.1f} KB")
                    print(f"📁 Saved to: {cache_file}")
                    return cache_file
                else:
                    print(f"⚠️  Signed URL download failed: HTTP {response.status_code}")
            
            # Method 2: Try public storage URL (if bucket is public)
            print("[INFO] Attempting public storage URL...")
            public_url = self.supabase.storage.from_("audio-files").get_public_url(storage_path)
            
            if public_url:
                print(f"[INFO] Using public URL")
                response = requests.get(public_url)
                
                if response.status_code == 200:
                    with open(cache_file, "wb") as f:
                        f.write(response.content)
                    
                    size_kb = cache_file.stat().st_size / 1024
                    print(f"✅ Downloaded: {size_kb:.1f} KB")
                    print(f"📁 Saved to: {cache_file}")
                    return cache_file
                else:
                    print(f"⚠️  Public URL download failed: HTTP {response.status_code}")
            
            # If both methods fail
            print(f"❌ Could not download audio from either signed or public URL")
            print(f"   Please verify:")
            print(f"   - Bucket name is 'audio-files'")
            print(f"   - Storage path is correct: {storage_path}")
            print(f"   - User has read permissions")
            return None
        
        except Exception as e:
            print(f"❌ Error downloading audio: {e}")
            import traceback
            traceback.print_exc()
            return None
    
    def transcribe_audio(self, audio_file: Path) -> Optional[str]:
        """Transcribe audio using ElevenLabs API."""
        self.print_section("STEP 3: Transcribing with ElevenLabs API")
        
        print(f"File: {audio_file.name}")
        print(f"Size: {audio_file.stat().st_size / 1024:.1f} KB")
        print("Sending to ElevenLabs...")
        
        try:
            with open(audio_file, "rb") as f:
                response = self.elevenlabs_client.speech_to_text.convert(
                    file=f,
                    model_id="scribe_v2"
                )
            
            # Extract transcript from response
            transcript = response.text if hasattr(response, 'text') else (response.get("text", "") if isinstance(response, dict) else "")
            
            print(f"✅ Transcription successful!")
            if hasattr(response, 'language'):
                print(f"Language: {response.language}")
            if hasattr(response, 'language_probability'):
                print(f"Confidence: {response.language_probability}")
            print(f"Length: {len(transcript)} characters")
            print()
            print("--- TRANSCRIPT START ---")
            print(transcript)
            print("--- TRANSCRIPT END ---")
            
            return transcript
        
        except Exception as e:
            print(f"❌ Transcription error: {e}")
            import traceback
            traceback.print_exc()
            return None
    
    def summarize_text(self, text: str) -> Optional[str]:
        """Summarize text using Google Gemini API."""
        self.print_section("STEP 4: Summarizing with Google Gemini API")
        
        print(f"Input length: {len(text)} characters")
        print("Sending to Gemini 2.5 Flash Lite...")
        
        try:
            prompt = f"""Please provide a concise summary of the following text. 
Keep it brief and highlight the key points.

TEXT:
{text}

SUMMARY:"""
            
            contents = [
                types.Content(
                    role="user",
                    parts=[
                        types.Part.from_text(text=prompt),
                    ],
                ),
            ]
            
            generate_content_config = types.GenerateContentConfig(
                temperature=1,
            )
            
            # Stream the response
            summary = ""
            for chunk in self.gemini_client.models.generate_content_stream(
                model="gemini-2.5-flash-lite",
                contents=contents,
                config=generate_content_config,
            ):
                summary += chunk.text
            
            print(f"✅ Summary generated!")
            print(f"Length: {len(summary)} characters")
            print()
            print("--- SUMMARY START ---")
            print(summary)
            print("--- SUMMARY END ---")
            
            return summary
        
        except Exception as e:
            print(f"❌ Summarization error: {e}")
            import traceback
            traceback.print_exc()
            return None
    
    def save_results(self, audio_id: str, transcript: str, summary: str):
        """Save results to local files."""
        self.print_section("STEP 5: Saving Results")
        
        try:
            transcript_file = self.cache_dir / f"{audio_id}_transcript.txt"
            summary_file = self.cache_dir / f"{audio_id}_summary.txt"
            
            with open(transcript_file, "w") as f:
                f.write(transcript)
            print(f"✅ Transcript saved: {transcript_file}")
            
            with open(summary_file, "w") as f:
                f.write(summary)
            print(f"✅ Summary saved: {summary_file}")
            
        except Exception as e:
            print(f"❌ Error saving results: {e}")
    
    def print_summary(self, audio: Dict, transcript: str, summary: str):
        """Print final summary."""
        self.print_section("WORKFLOW COMPLETE ✅")
        
        print(f"Audio: {audio['file_name']}")
        print(f"Audio ID: {audio['id']}")
        print()
        print(f"Transcript length: {len(transcript)} characters")
        print(f"Summary length: {len(summary)} characters")
        print()
        print("Results cached in:")
        print(f"  📁 {self.cache_dir}/")
        print()
        print("You can now:")
        print("  • Upload transcript to Supabase storage")
        print("  • Save summary to database")
        print("  • Process next audio file")
    
    def run(self):
        """Run the complete test workflow."""
        print("\n🎉 MindInk Local Processing Test")
        print("=" * 60)
        
        # Step 1: Fetch audio list
        audio_list = self.fetch_audio_list()
        if not audio_list:
            return
        
        # Step 2: Select audio
        audio = self.select_audio(audio_list)
        if not audio:
            return
        
        # Step 3: Download audio
        audio_file = self.download_audio(audio)
        if not audio_file:
            return
        
        # Step 4: Transcribe
        transcript = self.transcribe_audio(audio_file)
        if not transcript:
            return
        
        time.sleep(1)  # Brief pause between API calls
        
        # Step 5: Summarize
        summary = self.summarize_text(transcript)
        if not summary:
            return
        
        # Step 6: Save results
        self.save_results(audio["id"], transcript, summary)
        
        # Step 7: Print summary
        self.print_summary(audio, transcript, summary)


def main():
    """Main entry point."""
    try:
        tester = MindInkTester()
        tester.run()
    except KeyboardInterrupt:
        print("\n\n❌ Interrupted by user")
        sys.exit(1)
    except Exception as e:
        print(f"\n❌ Unexpected error: {e}")
        import traceback
        traceback.print_exc()
        sys.exit(1)


if __name__ == "__main__":
    main()
