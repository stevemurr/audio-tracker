// AudioTracker CLAP Plugin
// Real-time audio analysis with FFT, pitch detection, and metrics posting

#include <clap/clap.h>
#include <Accelerate/Accelerate.h>
#include <curl/curl.h>

#include <cmath>
#include <cstring>
#include <vector>
#include <string>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <atomic>
#include <queue>
#include <chrono>
#include <sstream>
#include <iomanip>

// Plugin constants
static constexpr uint32_t FFT_SIZE = 4096;
static constexpr uint32_t FFT_SIZE_HALF = FFT_SIZE / 2;
static constexpr float SILENCE_THRESHOLD_DB = -50.0f;
static constexpr float MIN_F0_HZ = 60.0f;
static constexpr float MAX_F0_HZ = 600.0f;
static constexpr const char* API_URL = "http://localhost:9091/api/audio";

// ============================================================================
// Audio Analyzer - all buffers pre-allocated
// ============================================================================

class AudioAnalyzer {
public:
    AudioAnalyzer() {
        fftLog2n_ = static_cast<vDSP_Length>(log2(FFT_SIZE));
        fftSetup_ = vDSP_create_fftsetup(fftLog2n_, FFT_RADIX2);

        window_.resize(FFT_SIZE);
        inputBuffer_.resize(FFT_SIZE);
        windowed_.resize(FFT_SIZE);
        fftReal_.resize(FFT_SIZE_HALF);
        fftImag_.resize(FFT_SIZE_HALF);
        magnitudes_.resize(FFT_SIZE_HALF);

        vDSP_hann_window(window_.data(), FFT_SIZE, vDSP_HANN_NORM);
    }

    ~AudioAnalyzer() {
        if (fftSetup_) {
            vDSP_destroy_fftsetup(fftSetup_);
        }
    }

    void setSampleRate(float sr) { sampleRate_ = sr; }
    float getSampleRate() const { return sampleRate_; }

    bool addSamples(const float* samples, uint32_t count) {
        uint32_t toCopy = std::min(count, FFT_SIZE - bufferPos_);
        memcpy(inputBuffer_.data() + bufferPos_, samples, toCopy * sizeof(float));
        bufferPos_ += toCopy;
        return bufferPos_ >= FFT_SIZE;
    }

    uint32_t getSamplesNeeded() const {
        return FFT_SIZE - bufferPos_;
    }

    void resetBuffer() { bufferPos_ = 0; }
    uint32_t getBufferPos() const { return bufferPos_; }

    float computeRMS() const {
        float sumSquares = 0.0f;
        vDSP_svesq(inputBuffer_.data(), 1, &sumSquares, FFT_SIZE);
        float rms = sqrtf(sumSquares / FFT_SIZE);
        return 20.0f * log10f(fmaxf(rms, 1e-10f));
    }

    void computeFFT() {
        vDSP_vmul(inputBuffer_.data(), 1, window_.data(), 1, windowed_.data(), 1, FFT_SIZE);

        DSPSplitComplex split = { fftReal_.data(), fftImag_.data() };
        vDSP_ctoz(reinterpret_cast<const DSPComplex*>(windowed_.data()), 2, &split, 1, FFT_SIZE_HALF);

        vDSP_fft_zrip(fftSetup_, &split, 1, fftLog2n_, FFT_FORWARD);
        vDSP_zvmags(&split, 1, magnitudes_.data(), 1, FFT_SIZE_HALF);

        float scale = 1.0f / (FFT_SIZE * 2);
        for (uint32_t i = 0; i < FFT_SIZE_HALF; ++i) {
            magnitudes_[i] = sqrtf(magnitudes_[i] * scale);
        }
    }

    float computeSpectralCentroid() const {
        float freqBinWidth = sampleRate_ / FFT_SIZE;
        float weightedSum = 0.0f;
        float totalMag = 0.0f;

        for (uint32_t i = 1; i < FFT_SIZE_HALF; ++i) {
            float freq = i * freqBinWidth;
            weightedSum += freq * magnitudes_[i];
            totalMag += magnitudes_[i];
        }

        return totalMag > 0.0f ? weightedSum / totalMag : 0.0f;
    }

    float detectF0() const {
        float freqBinWidth = sampleRate_ / FFT_SIZE;
        uint32_t minBin = static_cast<uint32_t>(MIN_F0_HZ / freqBinWidth);
        uint32_t maxBin = static_cast<uint32_t>(MAX_F0_HZ / freqBinWidth);
        maxBin = std::min(maxBin, FFT_SIZE_HALF - 1);

        float maxMag = 0.0f;
        uint32_t maxIdx = minBin;

        for (uint32_t i = minBin; i <= maxBin; ++i) {
            if (magnitudes_[i] > maxMag) {
                maxMag = magnitudes_[i];
                maxIdx = i;
            }
        }

        if (maxMag < 0.001f) return 0.0f;
        return maxIdx * freqBinWidth;
    }

private:
    float sampleRate_ = 44100.0f;
    FFTSetup fftSetup_ = nullptr;
    vDSP_Length fftLog2n_ = 0;

    std::vector<float> window_;
    std::vector<float> inputBuffer_;
    std::vector<float> windowed_;
    std::vector<float> fftReal_;
    std::vector<float> fftImag_;
    std::vector<float> magnitudes_;

    uint32_t bufferPos_ = 0;
};

// ============================================================================
// Streamer - independent timer thread that streams metrics regardless of process()
// ============================================================================

class MetricsStreamer {
public:
    MetricsStreamer() : running_(true) {
        streamerThread_ = std::thread(&MetricsStreamer::streamerLoop, this);
    }

    ~MetricsStreamer() {
        running_ = false;
        if (streamerThread_.joinable()) {
            streamerThread_.join();
        }
    }

    // Called from audio thread to update current metrics
    void updateMetrics(float f0, float centroid, float rms, double playhead) {
        std::lock_guard<std::mutex> lock(mutex_);
        currentF0_ = f0;
        currentCentroid_ = centroid;
        currentRms_ = rms;
        currentPlayhead_ = playhead;
        hasData_ = true;
    }

    static std::string formatTimestamp(double seconds) {
        int hours = static_cast<int>(seconds) / 3600;
        int mins = (static_cast<int>(seconds) % 3600) / 60;
        int secs = static_cast<int>(seconds) % 60;
        int millis = static_cast<int>((seconds - floor(seconds)) * 1000);

        std::ostringstream ss;
        ss << std::setfill('0');
        ss << std::setw(2) << hours << ":";
        ss << std::setw(2) << mins << ":";
        ss << std::setw(2) << secs << ".";
        ss << std::setw(3) << millis;
        return ss.str();
    }

private:
    void streamerLoop() {
        // Initialize CURL for this thread
        CURL* curl = curl_easy_init();
        struct curl_slist* headers = nullptr;
        if (curl) {
            headers = curl_slist_append(headers, "Content-Type: application/json");
        }

        while (running_) {
            // Sleep for streaming interval
            std::this_thread::sleep_for(std::chrono::milliseconds(100));

            if (!running_) break;

            // Get current metrics
            float f0, centroid, rms;
            double playhead;
            bool hasData;
            {
                std::lock_guard<std::mutex> lock(mutex_);
                f0 = currentF0_;
                centroid = currentCentroid_;
                rms = currentRms_;
                playhead = currentPlayhead_;
                hasData = hasData_;
            }

            if (!hasData || !curl) continue;

            // Build JSON payload
            std::ostringstream json;
            json << std::fixed << std::setprecision(2);
            json << "{";
            json << "\"f0\":" << f0 << ",";
            json << "\"centroid\":" << centroid << ",";
            json << "\"rms\":" << rms << ",";
            json << "\"startedAt\":\"" << formatTimestamp(playhead) << "\",";
            json << "\"endedAt\":\"" << formatTimestamp(playhead) << "\",";
            json << "\"localTime\":" << std::chrono::duration_cast<std::chrono::milliseconds>(
                std::chrono::system_clock::now().time_since_epoch()).count();
            json << "}";

            std::string payload = json.str();

            // Send request
            curl_easy_setopt(curl, CURLOPT_URL, API_URL);
            curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
            curl_easy_setopt(curl, CURLOPT_POSTFIELDS, payload.c_str());
            curl_easy_setopt(curl, CURLOPT_TIMEOUT_MS, 100L);
            curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT_MS, 50L);
            curl_easy_perform(curl);
        }

        if (headers) curl_slist_free_all(headers);
        if (curl) curl_easy_cleanup(curl);
    }

    std::thread streamerThread_;
    std::mutex mutex_;
    std::atomic<bool> running_;

    float currentF0_ = 0.0f;
    float currentCentroid_ = 0.0f;
    float currentRms_ = -100.0f;
    double currentPlayhead_ = 0.0;
    bool hasData_ = false;
};

// ============================================================================
// Plugin State
// ============================================================================

struct PluginState {
    AudioAnalyzer analyzer;
    MetricsStreamer streamer;  // Independent timer-based streamer

    float sampleRate = 44100.0f;
    double playheadPosition = 0.0;

    // Current frame metrics
    float currentF0 = 0.0f;
    float currentCentroid = 0.0f;
    float currentRms = -100.0f;

    // Pre-allocated mono buffer
    std::vector<float> monoBuffer;

    void reset() {
        currentF0 = 0.0f;
        currentCentroid = 0.0f;
        currentRms = -100.0f;
        analyzer.resetBuffer();
    }

    void ensureMonoBuffer(uint32_t size) {
        if (monoBuffer.size() < size) {
            monoBuffer.resize(size);
        }
    }
};

// ============================================================================
// CLAP Plugin Implementation
// ============================================================================

static const clap_plugin_descriptor_t pluginDescriptor = {
    .clap_version = CLAP_VERSION,
    .id = "com.audiotracker.clap",
    .name = "AudioTracker",
    .vendor = "AudioTracker",
    .url = "https://github.com/murr/audio-tracker",
    .manual_url = nullptr,
    .support_url = nullptr,
    .version = "1.0.0",
    .description = "Real-time audio analysis and metrics tracking",
    .features = (const char*[]){
        CLAP_PLUGIN_FEATURE_AUDIO_EFFECT,
        CLAP_PLUGIN_FEATURE_ANALYZER,
        CLAP_PLUGIN_FEATURE_UTILITY,
        nullptr
    }
};

static bool plugin_init(const clap_plugin_t* plugin);
static void plugin_destroy(const clap_plugin_t* plugin);
static bool plugin_activate(const clap_plugin_t* plugin, double sampleRate, uint32_t minFrames, uint32_t maxFrames);
static void plugin_deactivate(const clap_plugin_t* plugin);
static bool plugin_start_processing(const clap_plugin_t* plugin);
static void plugin_stop_processing(const clap_plugin_t* plugin);
static void plugin_reset(const clap_plugin_t* plugin);
static clap_process_status plugin_process(const clap_plugin_t* plugin, const clap_process_t* process);
static const void* plugin_get_extension(const clap_plugin_t* plugin, const char* id);
static void plugin_on_main_thread(const clap_plugin_t* plugin);

// Audio ports extension
static uint32_t audio_ports_count(const clap_plugin_t* /*plugin*/, bool /*isInput*/) {
    return 1;
}

static bool audio_ports_get(const clap_plugin_t* /*plugin*/, uint32_t index, bool isInput, clap_audio_port_info_t* info) {
    if (index != 0) return false;

    info->id = isInput ? 0 : 1;
    snprintf(info->name, sizeof(info->name), "%s", isInput ? "Input" : "Output");
    info->flags = CLAP_AUDIO_PORT_IS_MAIN;
    info->channel_count = 2;
    info->port_type = CLAP_PORT_STEREO;
    info->in_place_pair = isInput ? 1 : 0;

    return true;
}

static const clap_plugin_audio_ports_t audioPortsExtension = {
    .count = audio_ports_count,
    .get = audio_ports_get
};

// Tail extension - report infinite tail so we're always processed
static uint32_t tail_get(const clap_plugin_t* /*plugin*/) {
    return UINT32_MAX;  // Infinite tail - never stop processing us
}

static const clap_plugin_tail_t tailExtension = {
    .get = tail_get
};

static bool plugin_init(const clap_plugin_t* plugin) {
    curl_global_init(CURL_GLOBAL_DEFAULT);
    auto* state = new PluginState();
    const_cast<clap_plugin_t*>(plugin)->plugin_data = state;
    return true;
}

static void plugin_destroy(const clap_plugin_t* plugin) {
    auto* state = static_cast<PluginState*>(plugin->plugin_data);
    delete state;
    curl_global_cleanup();
}

static bool plugin_activate(const clap_plugin_t* plugin, double sampleRate, uint32_t /*minFrames*/, uint32_t maxFrames) {
    auto* state = static_cast<PluginState*>(plugin->plugin_data);
    state->sampleRate = static_cast<float>(sampleRate);
    state->analyzer.setSampleRate(state->sampleRate);
    state->monoBuffer.resize(maxFrames);
    return true;
}

static void plugin_deactivate(const clap_plugin_t* /*plugin*/) {
}

static bool plugin_start_processing(const clap_plugin_t* plugin) {
    auto* state = static_cast<PluginState*>(plugin->plugin_data);
    state->reset();
    return true;
}

static void plugin_stop_processing(const clap_plugin_t* /*plugin*/) {
}

static void plugin_reset(const clap_plugin_t* plugin) {
    auto* state = static_cast<PluginState*>(plugin->plugin_data);
    state->reset();
}

static clap_process_status plugin_process(const clap_plugin_t* plugin, const clap_process_t* process) {
    auto* state = static_cast<PluginState*>(plugin->plugin_data);

    const uint32_t frameCount = process->frames_count;
    if (frameCount == 0) {
        return CLAP_PROCESS_CONTINUE;
    }

    // Get transport info
    if (process->transport) {
        if (process->transport->flags & CLAP_TRANSPORT_HAS_SECONDS_TIMELINE) {
            state->playheadPosition = static_cast<double>(process->transport->song_pos_seconds) / CLAP_SECTIME_FACTOR;
        }
    }

    // Check for valid audio buffers
    if (!process->audio_inputs || !process->audio_outputs ||
        process->audio_inputs_count == 0 || process->audio_outputs_count == 0) {
        return CLAP_PROCESS_CONTINUE;
    }

    const float* inL = process->audio_inputs[0].data32 ? process->audio_inputs[0].data32[0] : nullptr;
    const float* inR = process->audio_inputs[0].data32 ? process->audio_inputs[0].data32[1] : nullptr;
    float* outL = process->audio_outputs[0].data32 ? process->audio_outputs[0].data32[0] : nullptr;
    float* outR = process->audio_outputs[0].data32 ? process->audio_outputs[0].data32[1] : nullptr;

    if (!inL || !outL) {
        return CLAP_PROCESS_CONTINUE;
    }

    // Pass through audio
    if (inL != outL) memcpy(outL, inL, frameCount * sizeof(float));
    if (inR && outR && inR != outR) memcpy(outR, inR, frameCount * sizeof(float));

    // Ensure mono buffer is large enough
    state->ensureMonoBuffer(frameCount);

    // Mix to mono
    if (inR) {
        for (uint32_t i = 0; i < frameCount; ++i) {
            state->monoBuffer[i] = (inL[i] + inR[i]) * 0.5f;
        }
    } else {
        memcpy(state->monoBuffer.data(), inL, frameCount * sizeof(float));
    }

    // Feed samples to analyzer
    uint32_t offset = 0;
    while (offset < frameCount) {
        uint32_t samplesNeeded = state->analyzer.getSamplesNeeded();
        uint32_t remaining = frameCount - offset;
        uint32_t toAdd = std::min(remaining, samplesNeeded);

        bool bufferFull = state->analyzer.addSamples(state->monoBuffer.data() + offset, toAdd);
        offset += toAdd;

        if (bufferFull) {
            float rms = state->analyzer.computeRMS();

            if (rms >= SILENCE_THRESHOLD_DB) {
                state->analyzer.computeFFT();
                state->currentF0 = state->analyzer.detectF0();
                state->currentCentroid = state->analyzer.computeSpectralCentroid();
                state->currentRms = rms;
            } else {
                state->currentF0 = 0.0f;
                state->currentCentroid = 0.0f;
                state->currentRms = rms;
            }

            state->analyzer.resetBuffer();

            // Update the streamer with current metrics (streamer handles timing independently)
            state->streamer.updateMetrics(
                state->currentF0,
                state->currentCentroid,
                state->currentRms,
                state->playheadPosition
            );
        }
    }

    return CLAP_PROCESS_CONTINUE;
}

static const void* plugin_get_extension(const clap_plugin_t* /*plugin*/, const char* id) {
    if (strcmp(id, CLAP_EXT_AUDIO_PORTS) == 0) {
        return &audioPortsExtension;
    }
    if (strcmp(id, CLAP_EXT_TAIL) == 0) {
        return &tailExtension;
    }
    return nullptr;
}

static void plugin_on_main_thread(const clap_plugin_t* /*plugin*/) {
}

static const clap_plugin_t* create_plugin(const clap_plugin_factory_t* /*factory*/,
                                          const clap_host_t* /*host*/,
                                          const char* pluginId) {
    if (strcmp(pluginId, pluginDescriptor.id) != 0) {
        return nullptr;
    }

    auto* plugin = new clap_plugin_t{
        .desc = &pluginDescriptor,
        .plugin_data = nullptr,
        .init = plugin_init,
        .destroy = plugin_destroy,
        .activate = plugin_activate,
        .deactivate = plugin_deactivate,
        .start_processing = plugin_start_processing,
        .stop_processing = plugin_stop_processing,
        .reset = plugin_reset,
        .process = plugin_process,
        .get_extension = plugin_get_extension,
        .on_main_thread = plugin_on_main_thread
    };

    return plugin;
}

static uint32_t factory_get_plugin_count(const clap_plugin_factory_t* /*factory*/) {
    return 1;
}

static const clap_plugin_descriptor_t* factory_get_plugin_descriptor(const clap_plugin_factory_t* /*factory*/, uint32_t index) {
    return index == 0 ? &pluginDescriptor : nullptr;
}

static const clap_plugin_t* factory_create_plugin(const clap_plugin_factory_t* factory,
                                                   const clap_host_t* host,
                                                   const char* pluginId) {
    return create_plugin(factory, host, pluginId);
}

static const clap_plugin_factory_t pluginFactory = {
    .get_plugin_count = factory_get_plugin_count,
    .get_plugin_descriptor = factory_get_plugin_descriptor,
    .create_plugin = factory_create_plugin
};

static bool entry_init(const char* /*pluginPath*/) {
    return true;
}

static void entry_deinit(void) {
}

static const void* entry_get_factory(const char* factoryId) {
    if (strcmp(factoryId, CLAP_PLUGIN_FACTORY_ID) == 0) {
        return &pluginFactory;
    }
    return nullptr;
}

extern "C" CLAP_EXPORT const clap_plugin_entry_t clap_entry = {
    .clap_version = CLAP_VERSION,
    .init = entry_init,
    .deinit = entry_deinit,
    .get_factory = entry_get_factory
};
