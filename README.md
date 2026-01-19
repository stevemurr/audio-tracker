# Audio Tracker

Real-time audio analysis system for monitoring metrics during DAW recording sessions.

## Components

1. **CLAP Audio Plugin** (`AudioTrackerCLAP/`) - C++ plugin using macOS Accelerate framework for FFT analysis
2. **Go Server** (`main.go`) - HTTP server accepting metrics from the plugin
3. **React Frontend** (`app/`) - Chart.js visualization polling the server

## Installation

```bash
# Install dependencies
brew install golang yarn

# Build and install the CLAP plugin
cd AudioTrackerCLAP
make
sudo make install  # Installs to /Library/Audio/Plug-Ins/CLAP/

# Run the Go server
go run main.go

# Run the frontend
cd app && yarn install && yarn start
```

## Usage

1. Start the Go server first: `go run main.go`
2. Open your DAW (Bitwig, etc.) and add "AudioTracker" as an effect on the track you want to analyze
3. Play audio - metrics are streamed every 100ms to the server
4. Open `http://localhost:3000` to view the live chart

## Audio Metrics

The plugin computes:
- **F0**: Fundamental frequency (pitch) via FFT peak detection, 60-600 Hz range
- **RMS**: Root mean square energy in dB
- **Spectral Centroid**: Brightness measure from FFT magnitudes

## API Endpoints

- `GET /api/audio` - Returns all stored audio data as JSON array
- `POST /api/audio` - Accepts audio metrics JSON from plugin
- `GET /api/audio/chart` - Returns Chart.js-formatted data

## Legacy

- JUCE AU plugin available in `plugin/` folder
- Swift AU v3 implementation on `legacy/swift` branch

## Credits

- [CLAP](https://github.com/free-audio/clap) - Audio plugin standard
- [JUCE](https://www.juce.com/) - Original plugin framework
- [Gist](https://github.com/adamstark/Gist) - Audio analysis library (legacy)
