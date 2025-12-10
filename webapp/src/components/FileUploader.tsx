import React, { useState, useRef, useCallback } from 'react';
import { supabase } from '../lib/supabaseClient';
import { UploadStatus } from './UploadStatus';

function isoFileName(prefix: string, ext: string) {
    return `${prefix}-${new Date().toISOString().replace(/[:.]/g, '-')}.${ext}`;
}

function formatFileSize(bytes: number): string {
    if (bytes === 0) return '0 Bytes';
    const k = 1024;
    const sizes = ['Bytes', 'KB', 'MB', 'GB'];
    const i = Math.floor(Math.log(bytes) / Math.log(k));
    return parseFloat((bytes / Math.pow(k, i)).toFixed(2)) + ' ' + sizes[i];
}

function getFileExtension(filename: string): string {
    return filename.split('.').pop()?.toLowerCase() || '';
}

// Supported audio file types
const SUPPORTED_FORMATS = ['mp3', 'wav', 'webm', 'm4a', 'aac', 'ogg', 'flac', 'mp4'];
const SUPPORTED_MIME_TYPES = [
    'audio/mpeg',
    'audio/wav',
    'audio/x-wav',
    'audio/webm',
    'audio/mp4',
    'audio/m4a',
    'audio/x-m4a',
    'audio/aac',
    'audio/ogg',
    'audio/flac',
    'video/mp4', // Sometimes audio files are classified as video/mp4
];

export const FileUploader: React.FC = () => {
    const [state, setState] = useState<'idle' | 'uploading' | 'done' | 'error'>('idle');
    const [error, setError] = useState<string | undefined>(undefined);
    const [selectedFile, setSelectedFile] = useState<File | null>(null);
    const [dragOver, setDragOver] = useState(false);
    const [uploadProgress, setUploadProgress] = useState(0);
    const fileInputRef = useRef<HTMLInputElement>(null);

    const validateFile = (file: File): boolean => {
        const ext = getFileExtension(file.name);
        const isValidType = SUPPORTED_FORMATS.includes(ext) || SUPPORTED_MIME_TYPES.includes(file.type);

        if (!isValidType) {
            setError(`Unsupported file format: ${ext || file.type}. Supported: ${SUPPORTED_FORMATS.join(', ')}`);
            return false;
        }

        // Max file size: 100MB
        const maxSize = 100 * 1024 * 1024;
        if (file.size > maxSize) {
            setError('File too large. Maximum size is 100MB.');
            return false;
        }

        return true;
    };

    const handleFileSelect = useCallback((file: File) => {
        setError(undefined);
        setState('idle');

        if (validateFile(file)) {
            setSelectedFile(file);
        }
    }, []);

    const handleInputChange = (e: React.ChangeEvent<HTMLInputElement>) => {
        const file = e.target.files?.[0];
        if (file) {
            handleFileSelect(file);
        }
    };

    const handleDragOver = (e: React.DragEvent) => {
        e.preventDefault();
        setDragOver(true);
    };

    const handleDragLeave = (e: React.DragEvent) => {
        e.preventDefault();
        setDragOver(false);
    };

    const handleDrop = (e: React.DragEvent) => {
        e.preventDefault();
        setDragOver(false);

        const file = e.dataTransfer.files?.[0];
        if (file) {
            handleFileSelect(file);
        }
    };

    const removeFile = () => {
        setSelectedFile(null);
        setError(undefined);
        setState('idle');
        if (fileInputRef.current) {
            fileInputRef.current.value = '';
        }
    };

    const upload = async () => {
        if (!selectedFile) return;

        setState('uploading');
        setUploadProgress(0);
        setError(undefined);

        const ext = getFileExtension(selectedFile.name);
        const fileName = isoFileName('audio', ext || 'audio');
        const filePath = fileName;

        try {
            // Simulate progress (Supabase JS client doesn't support upload progress natively)
            const progressInterval = setInterval(() => {
                setUploadProgress(prev => {
                    if (prev >= 90) {
                        clearInterval(progressInterval);
                        return prev;
                    }
                    return prev + Math.random() * 15;
                });
            }, 200);

            // 1. Upload to storage bucket
            const { data: storageData, error: uploadError } = await supabase.storage
                .from('audio-files')
                .upload(filePath, selectedFile, {
                    contentType: selectedFile.type || 'audio/mpeg',
                    upsert: false,
                });

            clearInterval(progressInterval);

            if (uploadError) {
                setError(uploadError.message);
                setState('error');
                return;
            }

            setUploadProgress(95);
            console.log('[FileUploader] Storage upload successful:', storageData);

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
                console.error('[FileUploader] Failed to insert audio_record:', recordError);
                setError(`Uploaded but DB insert failed: ${recordError.message}`);
                setState('error');
                return;
            }

            setUploadProgress(100);
            console.log('[FileUploader] Audio record created:', recordData);
            setState('done');

            // Reset after success
            setTimeout(() => {
                setSelectedFile(null);
                setState('idle');
                setUploadProgress(0);
                if (fileInputRef.current) {
                    fileInputRef.current.value = '';
                }
            }, 3000);
        } catch (err: any) {
            setError(err?.message || 'Upload failed');
            setState('error');
        }
    };

    return (
        <div className="upload-section">
            {!selectedFile ? (
                <div
                    className={`upload-zone ${dragOver ? 'drag-over' : ''}`}
                    onDragOver={handleDragOver}
                    onDragLeave={handleDragLeave}
                    onDrop={handleDrop}
                >
                    <span className="upload-zone-icon">📁</span>
                    <div className="upload-zone-title">Drop audio file here</div>
                    <div className="upload-zone-subtitle">
                        or tap to browse from your device
                    </div>
                    <div className="upload-zone-formats">
                        {['MP3', 'WAV', 'M4A', 'WEBM', 'AAC'].map(format => (
                            <span key={format} className="format-tag">{format}</span>
                        ))}
                    </div>
                    <input
                        ref={fileInputRef}
                        type="file"
                        accept="audio/*,.mp3,.wav,.m4a,.webm,.aac,.ogg,.flac,.mp4"
                        onChange={handleInputChange}
                    />
                </div>
            ) : (
                <>
                    <div className="file-preview">
                        <div className="file-icon">🎵</div>
                        <div className="file-info">
                            <div className="file-name">{selectedFile.name}</div>
                            <div className="file-size">{formatFileSize(selectedFile.size)}</div>
                        </div>
                        <button
                            className="file-remove"
                            onClick={removeFile}
                            disabled={state === 'uploading'}
                            aria-label="Remove file"
                        >
                            ✕
                        </button>
                    </div>

                    <button
                        className="button"
                        disabled={state === 'uploading'}
                        onClick={upload}
                    >
                        {state === 'uploading' ? (
                            <>
                                <span className="spinner"></span>
                                Uploading...
                            </>
                        ) : (
                            <>
                                <span className="button-icon">☁️</span>
                                Upload to Cloud
                            </>
                        )}
                    </button>
                </>
            )}

            <UploadStatus
                state={state === 'idle' && selectedFile ? 'idle' : state}
                message={error}
                progress={state === 'uploading' ? uploadProgress / 100 : undefined}
                customMessages={{
                    idle: selectedFile ? 'Ready to upload' : 'Select an audio file',
                    done: 'File uploaded successfully! 🎉',
                }}
            />
        </div>
    );
};
