# Supabase Edge Function Specs (MindInk)

Use a service-role or dedicated edge token; keep secrets off the device. Base URL: `https://fptyrdjmrzabvimbzupv.supabase.co/functions/v1`.

## process-audio
- **Method**: POST `/process-audio`
- **Auth**: `Authorization: Bearer <edge_token>`
- **Body**:
```json
{ "audio_id": "<uuid>", "force": false }
```
- **Flow**: fetch audio from `audio-files` → ElevenLabs transcription → Gemini summary → optional Gemini image → write to `summaries-text` and `gallery-images` → update `audio_records.summary_id`, insert `summaries` row, insert `infographics` row, set `status` fields.
- **Response**: `{ "job_id": "<uuid>", "status": "accepted" }`

## signed-audio
- **Method**: GET `/signed-audio?id=<audio_id>`
- **Auth**: `Authorization: Bearer <edge_token>` or anon if you allow.
- **Behavior**: look up `audio_records.storage_path`, return signed URL for `audio-files`.
- **Response**: `{ "url": "https://..." , "expires_in": 300 }

## signed-summary
- **Method**: GET `/signed-summary?id=<summary_id>`
- **Auth**: `Authorization: Bearer <edge_token>` or anon if allowed.
- **Behavior**: look up `summaries.summary_storage_path`, return signed URL for `summaries-text`.

## signed-image
- **Method**: GET `/signed-image?id=<infographic_id>`
- **Auth**: `Authorization: Bearer <edge_token>` or anon if allowed.
- **Behavior**: look up `infographics.storage_path`, return signed URL for `gallery-images`.

## insert-audio-record (optional helper)
- **Method**: POST `/insert-audio-record`
- **Auth**: `Authorization: Bearer <edge_token>`
- **Body**:
```json
{ "file_name": "audio-2025-12-05.webm", "storage_path": "audio-2025-12-05.webm" }
```
- **Behavior**: inserts `audio_records` row; sets status `new`.

## Device call sequence
1) List audio: `GET /rest/v1/audio_records?select=id,file_name,storage_path,summary_id,status&order=recording_date.desc&limit=50` with anon key.
2) Download/play: call `signed-audio` → download via signed URL → play.
3) Make summary: POST `process-audio` with `audio_id` using edge token.
4) Poll status: `GET /rest/v1/summaries?audio_id=eq.<uuid>&select=id,status,summary_storage_path,infographic_id&limit=1`.
5) Download summary: `signed-summary` → store to SD → display.
6) Gallery: list `infographics`; fetch via `signed-image` for selected.

## Implementation notes
- Prefer `supabase-js` in Edge Function; use service role key there.
- Signed URL TTL: 300s recommended for device playback.
- Set CORS for device and Render domains.
- Log failures and write `status`/`error_message` fields for polling UX.
