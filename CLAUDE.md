# CLAUDE.md

This file provides context for AI assistants working with this codebase.

## Project Overview

Audio Tracker is a three-part system for real-time audio analysis during DAW recording sessions:

1. **CLAP Audio Plugin** (`AudioTrackerCLAP/`) - C++ plugin using Accelerate framework for FFT analysis
2. **Go Backend** (`main.go`) - Echo-based HTTP server storing metrics in memory
3. **React Frontend** (`app/`) - Chart.js visualization polling the server

## Quick Commands

```bash
# Run the Go server
go run main.go

# Build and install the CLAP plugin
cd AudioTrackerCLAP && make install

# Run React frontend
cd app && yarn start
```

## Architecture

```
Plugin (C++) --HTTP POST--> Server (Go:9091) <--HTTP GET-- Frontend (React:3000)
```

The plugin sends audio metrics when silence is detected (audio drops below -50dB threshold for 0.5s). The frontend polls every 1.5 seconds for updates.

## Key Files

| File | Purpose |
|------|---------|
| `main.go` | Go server with `/api/audio` endpoints |
| `AudioTrackerCLAP/src/plugin.cpp` | CLAP plugin with audio analysis and HTTP posting |
| `AudioTrackerCLAP/Makefile` | Build and install commands |
| `app/src/App.js` | React chart component |

## API Endpoints

- `GET /api/audio` - Returns all stored audio data as JSON array
- `POST /api/audio` - Accepts audio metrics JSON from plugin
- `GET /api/audio/chart` - Returns Chart.js-formatted data structure

## Audio Metrics

The plugin computes these metrics using Apple's Accelerate framework (vDSP):
- **F0**: Fundamental frequency (pitch) via autocorrelation, filtered 60-600 Hz
- **RMS**: Root mean square energy in dB
- **Spectral Centroid**: Brightness measure from FFT magnitudes

## CLAP Plugin

The plugin is built as a shared library (`.clap`) that gets installed to:
```
~/Library/Audio/Plug-Ins/CLAP/AudioTracker.clap
```

Compatible with CLAP-supporting DAWs like Bitwig Studio.

### Building

```bash
cd AudioTrackerCLAP
make          # Build
make install  # Install to CLAP folder
make clean    # Remove build artifacts
```

### Dependencies

- macOS Accelerate framework (for FFT)
- libcurl (for HTTP requests)
- CLAP SDK (included as git submodule)

## JSON Payload Format

```json
{
  "f0": 220.5,
  "centroid": 2500.0,
  "rms": -18.3,
  "startedAt": "00:01:30.000",
  "endedAt": "00:02:15.500",
  "localTime": 1642345678000
}
```

## Development Notes

- Server stores data in-memory only (no persistence)
- Legacy JUCE plugin in `plugin/` folder (AU format)
- Swift AU v3 version available on `legacy/swift` branch
