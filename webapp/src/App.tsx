import React from 'react';
import { Recorder } from './components/Recorder';

function App() {
  return (
    <div className="app-shell">
      <Recorder />
      <div className="status" style={{ marginTop: 18 }}>
        <p>Tip: keep this repo in `webapp/` folder; deploy via Render static site.</p>
      </div>
    </div>
  );
}

export default App;
