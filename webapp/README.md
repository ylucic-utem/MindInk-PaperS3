# MindInk Web App (Recorder)

Mobile-first Vite + React SPA to record audio on iPhone and upload to Supabase Storage (`audio-files`).

## Getting started

```bash
cd webapp
npm install
npm run dev # http://localhost:4173
```

Create `.env.local` based on `.env.example` with your Supabase URL and anon key.

## Deploy (Render)
- Target: static site
- Build command: `npm run build`
- Publish directory: `dist`
- Env vars: `VITE_SUPABASE_URL`, `VITE_SUPABASE_ANON_KEY`

## Flow
- Uses `MediaRecorder` (WebM/Opus) to capture audio.
- Uploads file to bucket `audio-files` with ISO timestamp filename.
- Device lists entries from `audio_records` table (populated by your backend/edge flow after upload).

## Notes
- For MP3 conversion, add an API route or Edge Function with ffmpeg server-side; client stays lightweight.
- Keep secrets (service keys) off the client; only anon key in the SPA.
