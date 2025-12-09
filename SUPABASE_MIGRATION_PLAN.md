# Supabase Migration Plan (M5PaperS3 + Companion Web App)

## Objectives
- Replace Google Drive with Supabase (DB + Storage) without rewriting core M5PaperS3 navigation/action logic.
- Provide a mobile-first audio capture web app (SPA) that uploads optimized audio to Supabase.
- Use Supabase Edge Functions/Backend to run transcription + summarization + infographic generation and persist results.

## Repository & Deployment Layout
- Web App code: keep separate repo or a `web-app/` folder + branch in this repo; prefer separate repo for independent deploys and secrets.
- Backend/Edge Functions: colocate with web app repo; use Supabase CLI for local dev and migrations.
- M5PaperS3 firmware: keep existing repo; add minimal config to point to Supabase REST/Edge URLs.
- Deployment: Web App hosted on Render.com (static or minimal Node for SSR/upload proxy). Supabase handles API, auth, and storage.

## Supabase Resources
- Storage buckets: `audio-files` (optimized audio), `summaries-text` (summary text files), `gallery-images` (infographic images).
- Tables (see SQL DDL section in response): `audio_records`, `summaries`, `infographics`.
- RLS policies: enable row-level security; allow service key/Edge functions full access; allow authenticated users to insert/select own records (scoped by `auth.uid()`).
- Signed URLs: use short-lived signed URLs for M5PaperS3 playback/download and image fetches.

## Web App (SPA) Design
- Platform: lightweight Vite/React or Svelte SPA, mobile-first (iPhone viewport, touch targets ≥44px).
- Recording: use MediaRecorder to capture audio (48kHz PCM), show elapsed timer + level meter.
- Client-side optimize: convert to MP3/Opus via `ffmpeg.wasm` or send WAV to backend endpoint for server-side compression (simpler and faster on mobile: prefer server-side via Render).
- Upload flow: after record -> optional trim -> compress -> upload to Supabase Storage `audio-files` via signed upload URL or Supabase JS client with service role proxy endpoint.
- Metadata write: create `audio_records` row with `file_name`, `storage_path`, `recording_date`, status fields (if added) after successful upload.
- Auth: simple passwordless email magic link or one shared service token protected behind Render Basic Auth if single-user. Keep tokens server-side; issue signed URLs per request.
- UI states: recording, optimizing, uploading, done; show latest recordings list pulled from `audio_records` (ordered desc by `recording_date`).

## Backend / Edge Functions
- `process-audio` Edge Function (or Render backend) steps:
  1) Input: `audio_id` (UUID) from M5 device or web app.
  2) Fetch audio from `audio-files` via service key.
  3) Send to ElevenLabs (transcription); store transcription text.
  4) Send transcription to Gemini for summary + infographic prompt/image.
  5) Save summary text file to `summaries-text` bucket; insert/update `summaries` row linking to `audio_id`.
  6) Save infographic image to `gallery-images`; insert `infographics` row linked to summary.
  7) Update linking FKs and any status columns (e.g., `status`, `updated_at`).
- Expose lightweight REST endpoints for M5PaperS3:
  - `GET /audio`: list `audio_records` (id, file_name, storage_path, recording_date, summary_id).
  - `POST /audio/:id/process`: trigger pipeline above; return job id/status.
  - `GET /summaries`: list summaries (id, audio_id, summary_storage_path, optional signed URL).
  - `GET /infographics`: list images (id, summary_id, storage_path, signed URL).
  - `GET /audio/:id/signed-url`: short-lived signed URL for playback.

## M5PaperS3 Integration Changes (minimal)
- Replace Google Drive listing calls with Supabase REST calls to `audio_records` and `summaries` views/endpoints; preserve existing menu rendering.
- Config: add Supabase base URL, anon key (for reads) and optional device-scoped service key if allowed; otherwise hit proxy that returns signed URLs.
- Listing: fetch `audio_records` ordered by `recording_date DESC`; map to existing list UI (show `file_name`).
- "Make Summary" action: call `POST /audio/:id/process`; poll `summaries` table/endpoint for matching `audio_id` until `summary_storage_path` present (or `status=done`).
- Fetch summary text: request signed URL for `summary_storage_path`, download to SD card, display via existing reader.
- Playback: request signed URL for `audio-files` object, stream via M5Unified audio.
- Gallery: fetch `infographics` rows; for selected entry, fetch signed image URL and display.
- Error handling: reuse existing retry/backoff; surface short error messages; fallback when offline by reading SD cache.

## Deployment & Ops
- Use Supabase migrations for schema; keep DDL in repo.
- Render deploy: one service for SPA (static) and one for optional upload/compress proxy; environment vars for Supabase keys and API keys.
- Logging/observability: enable Supabase log drains; keep Edge Function logs for pipeline failures; add simple status field to tables for visibility.
- Security: keep service keys server-side only; device uses anon key plus signed URLs. Use CORS allowlist for Render domain.

## Optional Enhancements
- Add `status` and `error_message` columns to `audio_records` and `summaries` for better polling UX.
- Add `duration_seconds` and `transcript_language` to `audio_records` for richer filtering.
- Add `prompt_version` to `summaries` to track Gemini prompt tweaks.

## Supabase SQL DDL (tables)
```sql
create extension if not exists "uuid-ossp";

create table public.audio_records (
  id uuid primary key default uuid_generate_v4(),
  file_name text not null,
  storage_path text not null,
  recording_date timestamptz not null default now(),
  summary_id uuid,
  constraint audio_records_summary_fk foreign key (summary_id) references public.summaries(id)
);

create table public.summaries (
  id uuid primary key default uuid_generate_v4(),
  audio_id uuid not null,
  transcription_text text,
  summary_storage_path text,
  infographic_id uuid,
  constraint summaries_audio_fk foreign key (audio_id) references public.audio_records(id) on delete cascade,
  constraint summaries_infographic_fk foreign key (infographic_id) references public.infographics(id)
);

create table public.infographics (
  id uuid primary key default uuid_generate_v4(),
  summary_id uuid not null,
  image_file_name text not null,
  storage_path text not null,
  generation_date timestamptz not null default now(),
  constraint infographics_summary_fk foreign key (summary_id) references public.summaries(id) on delete cascade
);

-- Recommended indexes for device queries
create index if not exists idx_audio_records_recording_date on public.audio_records(recording_date desc);
create index if not exists idx_summaries_audio_id on public.summaries(audio_id);
create index if not exists idx_infographics_summary_id on public.infographics(summary_id);

-- Optional: status tracking columns (uncomment if used)
-- alter table public.audio_records add column status text check (status in ('new','processing','done','error')) default 'new';
-- alter table public.audio_records add column error_message text;
-- alter table public.summaries add column status text check (status in ('processing','done','error')) default 'processing';
-- alter table public.summaries add column error_message text;
```
