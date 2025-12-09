#!/usr/bin/env python3
"""
MindInk Edge Function Test Script

Tests the complete workflow using Supabase Edge Functions:
1. Fetch audio files from Supabase
2. Select audio file
3. Trigger process-audio edge function
4. Wait for processing to complete
5. Retrieve summary via signed-summary edge function

Prerequisites:
- Python 3.8+
- pip install supabase-py python-dotenv requests
- .env file with:
  SUPABASE_URL=https://fptyrdjmrzabvimbzupv.supabase.co
  SUPABASE_ANON_KEY=eyJhbGciOiJIUzI1NiIsInR5cCI6IkpXVCJ9...
"""

import os
import sys
import json
import time
from typing import Optional, Dict, List
from dotenv import load_dotenv
import requests
from supabase import create_client, Client

# Load environment variables
load_dotenv()

SUPABASE_URL = os.getenv("SUPABASE_URL", "https://fptyrdjmrzabvimbzupv.supabase.co")
SUPABASE_ANON_KEY = os.getenv("SUPABASE_ANON_KEY")

# Validate environment variables
if not SUPABASE_ANON_KEY:
    print("❌ Missing SUPABASE_ANON_KEY!")
    print("Please create a .env file with:")
    print("  SUPABASE_ANON_KEY=...")
    sys.exit(1)

# Initialize clients
supabase: Client = create_client(SUPABASE_URL, SUPABASE_ANON_KEY)

# Edge function URLs
PROCESS_AUDIO_URL = f"{SUPABASE_URL}/functions/v1/process-audio"
SIGNED_SUMMARY_URL = f"{SUPABASE_URL}/functions/v1/signed-summary"

# Headers for edge function calls
HEADERS = {
    "Authorization": f"Bearer {SUPABASE_ANON_KEY}",
    "Content-Type": "application/json"
}


class EdgeFunctionTester:
    """Test the complete MindInk processing workflow using Edge Functions."""

    def __init__(self):
        self.supabase = supabase

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

    def trigger_processing(self, audio_id: str) -> bool:
        """Trigger the process-audio edge function."""
        self.print_section("STEP 2: Triggering Edge Function Processing")

        print(f"Audio ID: {audio_id}")
        print(f"Calling: {PROCESS_AUDIO_URL}")

        try:
            payload = {"audio_id": audio_id}
            response = requests.post(PROCESS_AUDIO_URL, json=payload, headers=HEADERS)

            if response.status_code == 202:
                data = response.json()
                print("✅ Processing triggered successfully!")
                print(f"Response: {data.get('message', 'Processing started')}")
                return True
            else:
                print(f"❌ Failed to trigger processing: HTTP {response.status_code}")
                print(f"Response: {response.text}")
                return False

        except Exception as e:
            print(f"❌ Error triggering processing: {e}")
            return False

    def wait_for_processing(self, audio_id: str, max_wait: int = 120) -> bool:
        """Wait for processing to complete by polling status."""
        self.print_section("STEP 3: Waiting for Processing to Complete")

        print(f"Polling status for audio ID: {audio_id}")
        print(f"Max wait time: {max_wait} seconds")

        start_time = time.time()

        while time.time() - start_time < max_wait:
            try:
                # Fetch current status
                response = self.supabase.table("audio_records").select(
                    "status,summary_id"
                ).eq("id", audio_id).execute()

                if response.data:
                    record = response.data[0]
                    status = record.get("status")
                    summary_id = record.get("summary_id")

                    print(f"Status: {status} (elapsed: {int(time.time() - start_time)}s)")

                    if status == "done":
                        print("✅ Processing completed!")
                        return True
                    elif status == "error":
                        print("❌ Processing failed with error status")
                        return False
                    elif status == "processing":
                        time.sleep(5)  # Wait 5 seconds before next check
                    else:
                        print(f"Unknown status: {status}")
                        time.sleep(5)
                else:
                    print("❌ Audio record not found")
                    return False

            except Exception as e:
                print(f"❌ Error checking status: {e}")
                time.sleep(5)

        print(f"❌ Timeout waiting for processing (waited {max_wait} seconds)")
        return False

    def get_summary(self, summary_id: str) -> Optional[str]:
        """Get summary text via signed-summary edge function."""
        self.print_section("STEP 4: Retrieving Summary")

        print(f"Summary ID: {summary_id}")
        print(f"Calling: {SIGNED_SUMMARY_URL}?id={summary_id}")

        try:
            # Get signed URL for summary
            response = requests.get(f"{SIGNED_SUMMARY_URL}?id={summary_id}", headers=HEADERS)

            if response.status_code != 200:
                print(f"❌ Failed to get signed URL: HTTP {response.status_code}")
                print(f"Response: {response.text}")
                return None

            url_data = response.json()
            signed_url = url_data.get("url")

            if not signed_url:
                print("❌ No signed URL in response")
                return None

            print("✅ Got signed URL, fetching summary...")

            # Fetch the summary text
            summary_response = requests.get(signed_url)

            if summary_response.status_code != 200:
                print(f"❌ Failed to fetch summary: HTTP {summary_response.status_code}")
                return None

            summary = summary_response.text
            print("✅ Summary retrieved!")
            print(f"Length: {len(summary)} characters")
            print()
            print("--- SUMMARY START ---")
            print(summary)
            print("--- SUMMARY END ---")

            return summary

        except Exception as e:
            print(f"❌ Error retrieving summary: {e}")
            return None

    def run(self):
        """Run the complete edge function test workflow."""
        print("\n🚀 MindInk Edge Function Test")
        print("=" * 60)

        # Step 1: Fetch audio list
        audio_list = self.fetch_audio_list()
        if not audio_list:
            return

        # Step 2: Select audio
        audio = self.select_audio(audio_list)
        if not audio:
            return

        audio_id = audio["id"]

        # Step 3: Trigger processing
        if not self.trigger_processing(audio_id):
            return

        # Step 4: Wait for completion
        if not self.wait_for_processing(audio_id):
            return

        # Step 5: Get summary_id from audio record
        try:
            response = self.supabase.table("audio_records").select(
                "summary_id"
            ).eq("id", audio_id).execute()

            if response.data and response.data[0].get("summary_id"):
                summary_id = response.data[0]["summary_id"]
            else:
                print("❌ No summary_id found in audio record")
                return
        except Exception as e:
            print(f"❌ Error fetching summary_id: {e}")
            return

        # Step 6: Get and display summary
        summary = self.get_summary(summary_id)
        if summary:
            self.print_section("WORKFLOW COMPLETE ✅")
            print(f"Audio: {audio['file_name']}")
            print(f"Audio ID: {audio_id}")
            print(f"Summary ID: {summary_id}")
            print(f"Summary length: {len(summary)} characters")
        else:
            print("❌ Failed to retrieve summary")


def main():
    """Main entry point."""
    try:
        tester = EdgeFunctionTester()
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