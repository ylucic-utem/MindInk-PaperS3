import React, { useEffect, useRef, useState } from 'react';
import { supabase } from '../lib/supabaseClient';
import { UploadStatus } from './UploadStatus';

const MAX_DURATION_MS = 5 * 60 * 1000; // 5 minutes safety cap

function isoFileName(prefix: string, ext: string) {
  return `${prefix}-${new Date().toISOString().replace(/[:.]/g, '-')}.${ext}`;
}

// NOTE: WebM/Opus works but ElevenLabs prefers MP3/WAV/FLAC.
// Consider server-side conversion if transcription fails.

export const Recorder: React.FC = () => {
  const [state, setState] = useState<'idle' | 'recording' | 'processing' | 'uploading' | 'done' | 'error'>('idle');
  const [error, setError] = useState<string | undefined>(undefined);
  const mediaRecorderRef = useRef<MediaRecorder | null>(null);
  const chunksRef = useRef<BlobPart[]>([]);
  const stopTimerRef = useRef<number | undefined>(undefined);

  useEffect(() => {
    return () => {
      if (stopTimerRef.current) {
        clearTimeout(stopTimerRef.current);
      }
      mediaRecorderRef.current?.stream.getTracks().forEach((t: MediaStreamTrack) => t.stop());
    };
  }, []);

  const start = async () => {
    setError(undefined);

    if (!('MediaRecorder' in window)) {
      setError('MediaRecorder not supported on this browser. Use iOS Safari 14+ or Chrome.');
      return;
    }

    try {
      const stream = await navigator.mediaDevices.getUserMedia({ audio: true });
      const recorder = new MediaRecorder(stream, { mimeType: 'audio/webm;codecs=opus' });
      chunksRef.current = [];

      recorder.ondataavailable = (e) => {
        if (e.data.size > 0) chunksRef.current.push(e.data);
      };

      recorder.onstop = async () => {
        setState('processing');
        const blob = new Blob(chunksRef.current, { type: 'audio/webm' });
        await upload(blob);
      };

      recorder.start();
      mediaRecorderRef.current = recorder;
      setState('recording');

      // Safety stop
      stopTimerRef.current = window.setTimeout(() => stop(), MAX_DURATION_MS);
    } catch (err: any) {
      setError(err?.message || 'Could not start recording');
      setState('error');
    }
  };

  const stop = () => {
    if (mediaRecorderRef.current && mediaRecorderRef.current.state === 'recording') {
      mediaRecorderRef.current.stop();
      mediaRecorderRef.current.stream.getTracks().forEach((t: MediaStreamTrack) => t.stop());
    }
    setState('processing');
  };

  const upload = async (blob: Blob) => {
    setState('uploading');

    const fileName = isoFileName('audio', 'webm');
    const filePath = fileName; // flat path inside bucket

    try {
      // 1. Upload to storage bucket
      const { data: storageData, error: uploadError } = await supabase.storage
        .from('audio-files')
        .upload(filePath, blob, {
          contentType: 'audio/webm',
          upsert: false,
        });

      if (uploadError) {
        setError(uploadError.message);
        setState('error');
        return;
      }

      console.log('[Recorder] Storage upload successful:', storageData);

      // 2. Insert record into audio_records table
      const { data: recordData, error: recordError } = await supabase
        .from('audio_records')
        .insert({
          file_name: fileName,
          storage_path: filePath, // Use flat path, not duplicated
          status: 'new',
        })
        .select()
        .single();

      if (recordError) {
        console.error('[Recorder] Failed to insert audio_record:', recordError);
        setError(`Uploaded but DB insert failed: ${recordError.message}`);
        setState('error');
        return;
      }

      console.log('[Recorder] Audio record created:', recordData);
      setState('done');
    } catch (err: any) {
      setError(err?.message || 'Upload failed');
      setState('error');
    }
  };

  const buttonLabel =
    state === 'recording' ? '⏹ Stop' : state === 'uploading' ? 'Uploading…' : state === 'processing' ? 'Processing…' : '🎤 Record';

  const buttonDisabled = state === 'uploading' || state === 'processing';

  return (
    <div className="card" role="region" aria-label="recorder">
      <header>
        <h1>MindInk Recorder</h1>
        <span className="badge">Supabase</span>
      </header>
      <p style={{ color: 'var(--muted)' }}>Record on iPhone, upload to Supabase `audio-files` bucket.</p>

      <div style={{ margin: '16px 0' }}>
        <button className={`button ${state === 'recording' ? 'secondary' : ''}`} disabled={buttonDisabled} onClick={state === 'recording' ? stop : start}>
          {buttonLabel}
        </button>
      </div>

      <UploadStatus state={state} message={error} />

      <div className="list" aria-label="post-upload tips">
        <div className="list-item">
          <div>
            <strong>After upload</strong>
            <div><small>Device lists via Supabase REST `audio_records`.</small></div>
          </div>
        </div>
        <div className="list-item">
          <div>
            <strong>Format</strong>
            <div><small>WebM/Opus. ElevenLabs prefers MP3/WAV - may need conversion.</small></div>
          </div>
        </div>
        <div className="list-item">
          <div>
            <strong>Processing</strong>
            <div><small>Record appears in audio_records with status='new'.</small></div>
          </div>
        </div>
      </div>
    </div>
  );
};
