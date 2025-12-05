import React from 'react';

type Props = {
  state: 'idle' | 'recording' | 'processing' | 'uploading' | 'done' | 'error';
  message?: string;
  progress?: number; // 0-1
};

export function UploadStatus({ state, message, progress }: Props) {
  const text =
    message ||
    {
      idle: 'Ready to record',
      recording: 'Recording... tap to stop',
      processing: 'Optimizing audio...',
      uploading: 'Uploading to Supabase...',
      done: 'Uploaded',
      error: 'Something went wrong',
    }[state];

  return (
    <div className="status">
      <p>{text}</p>
      {typeof progress === 'number' && (
        <div className="progress-bar" aria-label="upload progress">
          <div className="progress-fill" style={{ ['--progress' as string]: `${Math.round(progress * 100)}%` }} />
        </div>
      )}
    </div>
  );
}
