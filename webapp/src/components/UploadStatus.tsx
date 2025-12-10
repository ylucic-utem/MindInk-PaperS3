import React from 'react';

type Props = {
  state: 'idle' | 'recording' | 'processing' | 'uploading' | 'done' | 'error';
  message?: string;
  progress?: number; // 0-1
  customMessages?: Partial<Record<'idle' | 'recording' | 'processing' | 'uploading' | 'done' | 'error', string>>;
};

const defaultMessages = {
  idle: 'Ready to record',
  recording: 'Recording in progress...',
  processing: 'Processing audio...',
  uploading: 'Uploading to Supabase...',
  done: 'Upload complete!',
  error: 'Something went wrong',
};

const statusIcons = {
  idle: '⚪',
  recording: '🔴',
  processing: '⚙️',
  uploading: '☁️',
  done: '✅',
  error: '❌',
};

export function UploadStatus({ state, message, progress, customMessages }: Props) {
  const messages = { ...defaultMessages, ...customMessages };
  const text = message || messages[state];
  const icon = statusIcons[state];

  const statusClass =
    state === 'done' ? 'success' :
      state === 'error' ? 'error' :
        state === 'recording' ? 'recording' :
          state === 'uploading' ? 'uploading' : '';

  return (
    <div className="status">
      <div className={`status-text ${statusClass}`}>
        <span className="status-icon">{icon}</span>
        <span>{text}</span>
      </div>
      {typeof progress === 'number' && state === 'uploading' && (
        <div className="progress-bar" aria-label="upload progress">
          <div
            className="progress-fill"
            style={{ ['--progress' as string]: `${Math.round(progress * 100)}%` }}
          />
        </div>
      )}
    </div>
  );
}
