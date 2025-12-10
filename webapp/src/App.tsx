import React, { useState } from 'react';
import { Recorder } from './components/Recorder';
import { FileUploader } from './components/FileUploader';

type Tab = 'record' | 'upload';

function App() {
  const [activeTab, setActiveTab] = useState<Tab>('record');

  return (
    <div className="app-shell">
      {/* Header */}
      <header className="app-header">
        <div className="logo">
          <div className="logo-icon">🧠</div>
          <h1>MindInk</h1>
        </div>
        <p className="tagline">Capture your thoughts, anywhere</p>
      </header>

      {/* Main Card */}
      <div className="card">
        <div className="card-header">
          <h2>
            <span>🎵</span>
            Audio Capture
          </h2>
          <span className="badge connected">Connected</span>
        </div>

        <p className="card-description">
          Record live audio or upload existing files. Your audio is securely stored in the cloud and available on your MindInk device.
        </p>

        {/* Tabs */}
        <div className="tabs">
          <button
            className={`tab ${activeTab === 'record' ? 'active' : ''}`}
            onClick={() => setActiveTab('record')}
          >
            <span className="tab-icon">🎤</span>
            Record
          </button>
          <button
            className={`tab ${activeTab === 'upload' ? 'active' : ''}`}
            onClick={() => setActiveTab('upload')}
          >
            <span className="tab-icon">📁</span>
            Upload File
          </button>
        </div>

        {/* Content based on active tab */}
        {activeTab === 'record' ? <Recorder /> : <FileUploader />}
      </div>

      {/* Info Card */}
      <div className="card">
        <div className="card-header">
          <h2>
            <span>💡</span>
            How It Works
          </h2>
        </div>

        <div className="info-list">
          <div className="info-item">
            <div className="info-icon">☁️</div>
            <div className="info-content">
              <div className="info-title">Cloud Sync</div>
              <div className="info-description">
                Audio files are automatically synced to your MindInk device via Supabase.
              </div>
            </div>
          </div>

          <div className="info-item">
            <div className="info-icon">🔄</div>
            <div className="info-content">
              <div className="info-title">Processing</div>
              <div className="info-description">
                Your device can transcribe and summarize recordings using cloud AI.
              </div>
            </div>
          </div>

          <div className="info-item">
            <div className="info-icon">📱</div>
            <div className="info-content">
              <div className="info-title">Cross-Platform</div>
              <div className="info-description">
                Works on iPhone, Android, and desktop browsers. Access anywhere.
              </div>
            </div>
          </div>
        </div>
      </div>

      {/* Footer */}
      <footer className="app-footer">
        <p className="footer-text">
          Made with ❤️ for MindInk PaperS3
        </p>
      </footer>
    </div>
  );
}

export default App;
