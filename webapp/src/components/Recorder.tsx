import React, { useEffect, useRef, useState } from 'react';
import { supabase } from '../lib/supabaseClient';
import { UploadStatus } from './UploadStatus';

const MAX_DURATION_MS = 5 * 60 * 1000; // 5 minutes safety cap

function isoFileName(prefix: string, ext: string) {
  return `${prefix}-${new Date().toISOString().replace(/[:.]/g, '-')}.${ext}`;
}

function formatTime(seconds: number): string {
  const mins = Math.floor(seconds / 60);
  const secs = seconds % 60;
  return `${mins.toString().padStart(2, '0')}:${secs.toString().padStart(2, '0')}`;
}

export const Recorder: React.FC = () => {
  const [state, setState] = useState<'idle' | 'recording' | 'processing' | 'uploading' | 'done' | 'error'>('idle');
  const [error, setError] = useState<string | undefined>(undefined);
  const [recordingTime, setRecordingTime] = useState(0);
  const mediaRecorderRef = useRef<MediaRecorder | null>(null);
  const chunksRef = useRef<BlobPart[]>([]);
  const stopTimerRef = useRef<number | undefined>(undefined);
  const recordingTimerRef = useRef<number | undefined>(undefined);

  useEffect(() => {
    return () => {
      if (stopTimerRef.current) {
        clearTimeout(stopTimerRef.current);
      }
      if (recordingTimerRef.current) {
        clearInterval(recordingTimerRef.current);
      }
      mediaRecorderRef.current?.stream.getTracks().forEach((t: MediaStreamTrack) => t.stop());
    };
  }, []);

  const start = async () => {
    setError(undefined);
    setRecordingTime(0);

    if (!('MediaRecorder' in window)) {
      setError('MediaRecorder not supported. Use Safari 14+ or Chrome.');
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
        if (recordingTimerRef.current) {
          clearInterval(recordingTimerRef.current);
        }
        setState('processing');
        const blob = new Blob(chunksRef.current, { type: 'audio/webm' });
        await upload(blob);
      };

      recorder.start();
      mediaRecorderRef.current = recorder;
      setState('recording');

      // Recording time counter
      recordingTimerRef.current = window.setInterval(() => {
        setRecordingTime(prev => prev + 1);
      }, 1000);

      // Safety stop
      stopTimerRef.current = window.setTimeout(() => stop(), MAX_DURATION_MS);
    } catch (err: any) {
      setError(err?.message || 'Could not access microphone');
      setState('error');
    }
  };

  const stop = () => {
    if (mediaRecorderRef.current && mediaRecorderRef.current.state === 'recording') {
      mediaRecorderRef.current.stop();
      mediaRecorderRef.current.stream.getTracks().forEach((t: MediaStreamTrack) => t.stop());
    }
    if (stopTimerRef.current) {
      clearTimeout(stopTimerRef.current);
    }
    setState('processing');
  };

  const upload = async (blob: Blob) => {
    setState('uploading');

    const fileName = isoFileName('audio', 'webm');
    const filePath = fileName;

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
          storage_path: filePath,
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

      // Reset after success
      setTimeout(() => {
        setState('idle');
        setRecordingTime(0);
      }, 3000);
    } catch (err: any) {
      setError(err?.message || 'Upload failed');
      setState('error');
    }
  };

  const reset = () => {
    setState('idle');
    setError(undefined);
    setRecordingTime(0);
  };

  return (
    <div className="recorder-section">
      {state === 'recording' ? (
        <>
          <div className="recording-indicator">
            <div className="recording-pulse">🎙️</div>
            <div className="recording-info">
              <div className="recording-label">Recording...</div>
              <div className="recording-time">{formatTime(recordingTime)}</div>
            </div>
          </div>
          <button className="button danger" onClick={stop}>
            <span className="button-icon">⏹</span>
            Stop Recording
          </button>
        </>
      ) : (
        <button
          className="button"
          disabled={state === 'uploading' || state === 'processing'}
          onClick={start}
        >
          {state === 'uploading' ? (
            <>
              <span className="spinner"></span>
              Uploading...
            </>
          ) : state === 'processing' ? (
            <>
              <span className="spinner"></span>
              Processing...
            </>
          ) : (
            <>
              <span className="button-icon">🎤</span>
              Start Recording
            </>
          )}
        </button>
      )}

      <UploadStatus
        state={state}
        message={error}
        customMessages={{
          done: 'Recording uploaded successfully! 🎉',
        }}
      />

      {state === 'error' && (
        <button className="button secondary" onClick={reset} style={{ marginTop: 12 }}>
          Try Again
        </button>
      )}
    </div>
  );
};
