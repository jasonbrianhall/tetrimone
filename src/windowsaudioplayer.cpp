#ifdef _WIN32

#include "audiomanager.h"
#include <iostream>
#include <fstream>
#include <chrono>
#include <iomanip>
#include <windows.h>
#include <mmsystem.h>
#include <mmreg.h>
#include <thread>
#include <mutex>
#include <vector>
#include <list>
#include <algorithm>
#include <condition_variable>
#include <atomic>
#include <cstring>
#include <optional>
#pragma comment(lib, "winmm.lib")

// Global logger class for audio debugging
class AudioLogger {
public:
    static AudioLogger& getInstance() {
        static AudioLogger instance;
        return instance;
    }
    
    void log(const std::string& message) {
        std::lock_guard<std::mutex> lock(logMutex_);
        if (!logFile_.is_open()) {
            openLogFile();
        }
        
        auto now = std::chrono::system_clock::now();
        auto now_time_t = std::chrono::system_clock::to_time_t(now);
        auto now_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
            now.time_since_epoch()) % 1000;
            
        std::tm now_tm;
        localtime_s(&now_tm, &now_time_t);
        
        logFile_ << "["
                 << std::setfill('0') << std::setw(2) << now_tm.tm_hour << ":"
                 << std::setfill('0') << std::setw(2) << now_tm.tm_min << ":"
                 << std::setfill('0') << std::setw(2) << now_tm.tm_sec << "."
                 << std::setfill('0') << std::setw(3) << now_ms.count() << "] "
                 << message << std::endl;
        
        // Also output to console in debug mode
#ifdef DEBUG
        std::cerr << message << std::endl;
#endif
    }
    
    ~AudioLogger() {
        if (logFile_.is_open()) {
            logFile_.close();
        }
    }
    
private:
    AudioLogger() {
        openLogFile();
    }
    
    void openLogFile() {
        logFile_.open("audio_debug.log", std::ios::out | std::ios::app);
        if (!logFile_.is_open()) {
            std::cerr << "ERROR: Failed to open audio debug log file" << std::endl;
        }
        
        logFile_ << "\n-------- Audio System Session Started at " 
                 << getCurrentTimeString() << " --------\n" << std::endl;
    }
    
    std::string getCurrentTimeString() {
        auto now = std::chrono::system_clock::now();
        auto now_time_t = std::chrono::system_clock::to_time_t(now);
        
        std::tm now_tm;
        localtime_s(&now_tm, &now_time_t);
        
        std::stringstream ss;
        ss << std::put_time(&now_tm, "%Y-%m-%d %H:%M:%S");
        return ss.str();
    }
    
    std::ofstream logFile_;
    std::mutex logMutex_;
};

// Macro for easier logging
#define AUDIO_LOG(msg) AudioLogger::getInstance().log(msg)

// Structure to hold WAV data information
struct WavSound {
    std::vector<uint8_t> data;     // Original WAV data
    int16_t* samples;              // Pointer to PCM sample data
    size_t sampleCount;            // Number of samples (considering all channels)
    size_t dataOffset;             // Offset to the data chunk
    uint16_t channels;             // Number of channels
    uint32_t sampleRate;           // Sample rate
    uint16_t bitsPerSample;        // Bits per sample
    bool isPlaying;                // Whether this sound is currently playing
    size_t position;               // Current playback position in frames (not samples)
    float volume;                  // Volume multiplier (0.0 to 1.0)
    std::shared_ptr<std::promise<void>> completionPromise;
    std::string soundName;         // Name/ID for debugging purposes
    
    WavSound() : samples(nullptr), sampleCount(0), dataOffset(0), 
                 channels(0), sampleRate(0), bitsPerSample(0),
                 isPlaying(false), position(0), volume(1.0f),
                 soundName("Unknown") {}
};

// Audio Mixer class that handles all sound playback through a single output stream
class AudioMixer {
public:
    static AudioMixer& getInstance() {
        static AudioMixer instance;
        return instance;
    }
    
    ~AudioMixer() {
        shutdown();
    }
    
    // Initialize the mixer
    bool initialize() {
        std::lock_guard<std::mutex> lock(mutex_);
        
        if (initialized_) {
            AUDIO_LOG("AudioMixer already initialized, returning");
            return true;
        }
        
        AUDIO_LOG("Initializing Windows Audio Mixer");
        
        // Check if audio devices are available
        if (waveOutGetNumDevs() == 0) {
            AUDIO_LOG("ERROR: No audio output devices available");
            return false;
        }
        
        AUDIO_LOG("Found " + std::to_string(waveOutGetNumDevs()) + " audio devices");
        
        // Set up the output wave format
        wfx_.wFormatTag = WAVE_FORMAT_PCM;
        wfx_.nChannels = OUTPUT_CHANNELS;
        wfx_.nSamplesPerSec = OUTPUT_SAMPLE_RATE;
        wfx_.wBitsPerSample = OUTPUT_BITS_PER_SAMPLE;
        wfx_.nBlockAlign = wfx_.nChannels * wfx_.wBitsPerSample / 8;
        wfx_.nAvgBytesPerSec = wfx_.nSamplesPerSec * wfx_.nBlockAlign;
        wfx_.cbSize = 0;
        
        std::stringstream ss;
        ss << "Audio format: " << wfx_.nSamplesPerSec << "Hz, " 
           << wfx_.nChannels << " channels, " 
           << wfx_.wBitsPerSample << " bits per sample";
        AUDIO_LOG(ss.str());
        
        // Open the wave device
        MMRESULT result = waveOutOpen(&hWaveOut_, WAVE_MAPPER, &wfx_, 
                                     (DWORD_PTR)waveOutProc, (DWORD_PTR)this, CALLBACK_FUNCTION);
        if (result != MMSYSERR_NOERROR) {
            AUDIO_LOG("ERROR: Failed to open wave device, error: " + std::to_string(result));
            return false;
        }
        
        // Create a buffer for mixed audio
        bufferSizeSamples_ = (OUTPUT_SAMPLE_RATE * OUTPUT_CHANNELS * BUFFER_DURATION_MS) / 1000;
        
        AUDIO_LOG("Creating " + std::to_string(NUM_BUFFERS) + " audio buffers, each with " + 
                 std::to_string(bufferSizeSamples_) + " samples (" + 
                 std::to_string(BUFFER_DURATION_MS) + "ms)");
                 
        for (int i = 0; i < NUM_BUFFERS; i++) {
            buffers_[i].resize(bufferSizeSamples_ * sizeof(int16_t));
            
            // Setup wave headers
            ZeroMemory(&waveHeaders_[i], sizeof(WAVEHDR));
            waveHeaders_[i].lpData = reinterpret_cast<LPSTR>(buffers_[i].data());
            waveHeaders_[i].dwBufferLength = static_cast<DWORD>(buffers_[i].size());
            waveHeaders_[i].dwUser = i;
            
            // Prepare the header
            result = waveOutPrepareHeader(hWaveOut_, &waveHeaders_[i], sizeof(WAVEHDR));
            if (result != MMSYSERR_NOERROR) {
                AUDIO_LOG("ERROR: Failed to prepare header " + std::to_string(i) + 
                         ", error: " + std::to_string(result));
                waveOutClose(hWaveOut_);
                return false;
            }
        }
        
        AUDIO_LOG("Starting mixer thread");
        
        // Start playback thread
        exitThread_ = false;
        mixerThread_ = std::thread(&AudioMixer::mixerThreadFunc, this);
        
        initialized_ = true;
        AUDIO_LOG("Windows Audio Mixer initialized successfully");
        return true;
    }
    
    // Shutdown the mixer
    void shutdown() {
        AUDIO_LOG("Shutting down Windows Audio Mixer");
        
        // Signal the thread to exit and wait for it
        if (mixerThread_.joinable()) {
            AUDIO_LOG("Signaling mixer thread to exit");
            exitThread_ = true;
            condVar_.notify_all();
            mixerThread_.join();
            AUDIO_LOG("Mixer thread joined successfully");
        }
        
        std::lock_guard<std::mutex> lock(mutex_);
        
        if (!initialized_) {
            AUDIO_LOG("Audio Mixer already shut down, returning");
            return;
        }
        
        // Reset wave device and unprepare headers
        AUDIO_LOG("Resetting waveOut device");
        waveOutReset(hWaveOut_);
        
        for (int i = 0; i < NUM_BUFFERS; i++) {
            if (waveHeaders_[i].dwFlags & WHDR_PREPARED) {
                MMRESULT result = waveOutUnprepareHeader(hWaveOut_, &waveHeaders_[i], sizeof(WAVEHDR));
                if (result != MMSYSERR_NOERROR) {
                    AUDIO_LOG("WARNING: Failed to unprepare header " + std::to_string(i) + 
                             ", error: " + std::to_string(result));
                }
            }
        }
        
        // Close the wave device
        AUDIO_LOG("Closing waveOut device");
        MMRESULT result = waveOutClose(hWaveOut_);
        if (result != MMSYSERR_NOERROR) {
            AUDIO_LOG("WARNING: Failed to close waveOut device, error: " + 
                     std::to_string(result));
        }
        
        hWaveOut_ = NULL;
        
        // Complete all pending sounds
        AUDIO_LOG("Completing promises for " + std::to_string(sounds_.size()) + " active sounds");
        for (auto& sound : sounds_) {
            AUDIO_LOG("Completing promise for sound: " + sound.soundName);
            if (sound.completionPromise) {
                try {
                    sound.completionPromise->set_value();
                } catch (const std::exception& e) {
                    AUDIO_LOG("Exception while completing promise: " + std::string(e.what()));
                }
            }
        }
        sounds_.clear();
        
        // Complete music promise if active
        if (backgroundMusic_) {
            AUDIO_LOG("Completing promise for background music: " + 
                     backgroundMusic_->sound.soundName);
            if (backgroundMusic_->sound.completionPromise) {
                try {
                    backgroundMusic_->sound.completionPromise->set_value();
                } catch (const std::exception& e) {
                    AUDIO_LOG("Exception while completing music promise: " + 
                             std::string(e.what()));
                }
            }
            backgroundMusic_.reset();
        }
        
        initialized_ = false;
        AUDIO_LOG("Windows Audio Mixer shutdown complete");
    }
    
    // Add a sound to the mixer
    bool addSound(const std::vector<uint8_t>& data, const std::string& format, 
                 std::shared_ptr<std::promise<void>> completionPromise,
                 const std::string& soundName = "SoundEffect") {
        // Only support WAV format
        if (format != "wav" && format != "WAV") {
            AUDIO_LOG("ERROR: Unsupported format for sound '" + soundName + 
                     "': " + format + " (only WAV supported)");
            if (completionPromise) {
                try {
                    completionPromise->set_value();
                } catch (const std::exception& e) {
                    AUDIO_LOG("Exception while completing promise: " + std::string(e.what()));
                }
            }
            return false;
        }
        
        std::stringstream ss;
        ss << "Playing sound '" << soundName << "', format: " << format 
           << ", size: " << data.size() << " bytes";
        AUDIO_LOG(ss.str());
        
        // Basic validation
        if (data.size() < 44 || 
            memcmp(data.data(), "RIFF", 4) != 0 || 
            memcmp(data.data() + 8, "WAVE", 4) != 0) {
            AUDIO_LOG("ERROR: Invalid WAV header for sound '" + soundName + "'");
            if (completionPromise) {
                try {
                    completionPromise->set_value();
                } catch (const std::exception& e) {
                    AUDIO_LOG("Exception while completing promise: " + std::string(e.what()));
                }
            }
            return false;
        }
        
        // Create a new sound entry
        WavSound sound;
        sound.data = data;
        sound.completionPromise = completionPromise;
        sound.isPlaying = true;
        sound.position = 0;
        sound.volume = masterVolume_;
        sound.soundName = soundName;
        
        // Parse the WAV header
        if (!parseWavHeader(sound)) {
            AUDIO_LOG("ERROR: Failed to parse WAV header for sound '" + soundName + "'");
            if (completionPromise) {
                try {
                    completionPromise->set_value();
                } catch (const std::exception& e) {
                    AUDIO_LOG("Exception while completing promise: " + std::string(e.what()));
                }
            }
            return false;
        }
        
        std::stringstream ss2;
        ss2 << "Sound '" << soundName << "' properties: sample rate=" << sound.sampleRate 
            << "Hz, channels=" << sound.channels 
            << ", bits=" << sound.bitsPerSample 
            << ", total samples=" << sound.sampleCount;
        AUDIO_LOG(ss2.str());
        
        // Add the sound to our list
        {
            std::lock_guard<std::mutex> lock(mutex_);
            sounds_.push_back(std::move(sound));
        }
        
        // Signal the mixer thread that we have a new sound
        condVar_.notify_one();
        
        return true;
    }
    
    // Special handling for background music with looping support
    bool playBackgroundMusic(const std::vector<uint8_t>& data, const std::string& format, 
                            bool loop, std::shared_ptr<std::promise<void>> completionPromise = nullptr) {
        // Only support WAV format for background music
        if (format != "wav" && format != "WAV") {
            AUDIO_LOG("ERROR: Unsupported format for background music: " + 
                     format + " (only WAV supported)");
            if (completionPromise) {
                try {
                    completionPromise->set_value();
                } catch (const std::exception& e) {
                    AUDIO_LOG("Exception while completing promise: " + std::string(e.what()));
                }
            }
            return false;
        }
        
        std::stringstream ss;
        ss << "Playing background music, format: " << format 
           << ", size: " << data.size() << " bytes"
           << ", looping: " << (loop ? "yes" : "no");
        AUDIO_LOG(ss.str());
        
        // Basic validation
        if (data.size() < 44 || 
            memcmp(data.data(), "RIFF", 4) != 0 || 
            memcmp(data.data() + 8, "WAVE", 4) != 0) {
            AUDIO_LOG("ERROR: Invalid WAV header for background music");
            if (completionPromise) {
                try {
                    completionPromise->set_value();
                } catch (const std::exception& e) {
                    AUDIO_LOG("Exception while completing promise: " + std::string(e.what()));
                }
            }
            return false;
        }
        
        // Stop any currently playing background music
        stopBackgroundMusic();
        
        // Create new background music structure
        BackgroundMusic music;
        music.sound.data = data;
        music.sound.completionPromise = completionPromise;
        music.sound.isPlaying = true;
        music.sound.position = 0;
        music.sound.volume = masterVolume_;
        music.sound.soundName = "BackgroundMusic";
        music.looping = loop;
        
        // Parse the WAV header
        if (!parseWavHeader(music.sound)) {
            AUDIO_LOG("ERROR: Failed to parse WAV header for background music");
            if (completionPromise) {
                try {
                    completionPromise->set_value();
                } catch (const std::exception& e) {
                    AUDIO_LOG("Exception while completing promise: " + std::string(e.what()));
                }
            }
            return false;
        }
        
        std::stringstream ss2;
        ss2 << "Background music properties: sample rate=" << music.sound.sampleRate 
            << "Hz, channels=" << music.sound.channels 
            << ", bits=" << music.sound.bitsPerSample 
            << ", total samples=" << music.sound.sampleCount;
        AUDIO_LOG(ss2.str());
        
        // Add the music to our background music slot
        {
            std::lock_guard<std::mutex> lock(mutex_);
            backgroundMusic_ = std::move(music);
        }
        
        // Signal the mixer thread that we have new music
        condVar_.notify_one();
        
        return true;
    }
    
    // Set master volume
    void setVolume(float volume) {
        std::lock_guard<std::mutex> lock(mutex_);
        
        float oldVolume = masterVolume_;
        masterVolume_ = std::max(0.0f, std::min(1.0f, volume));
        
        AUDIO_LOG("Setting master volume from " + std::to_string(oldVolume) + 
                 " to " + std::to_string(masterVolume_));
        
        // Update volume for all active sounds
        for (auto& sound : sounds_) {
            sound.volume = masterVolume_;
        }
        
        // Update background music volume
        if (backgroundMusic_) {
            backgroundMusic_->sound.volume = masterVolume_;
        }
    }
    
void stopAllSounds() {
    try {
        AUDIO_LOG("Muting all sounds in mixer");
        
        std::lock_guard<std::mutex> lock(mutex_);
        
        if (!initialized_) {
            AUDIO_LOG("Audio Mixer not initialized, ignoring stopAllSounds");
            return;
        }
        
        // Store the previous volume level before muting
        masterVolume_ = 0.0f;
                
        // Apply muting to all active sounds
        for (auto& sound : sounds_) {
            sound.volume = 0.0f;
        }
        
        // Mute background music as well, if it exists
        if (backgroundMusic_) {
            backgroundMusic_->sound.volume = 0.0f;
        }
        
        AUDIO_LOG("All sounds muted successfully");
    }
    catch (const std::exception& e) {
        AUDIO_LOG("Exception in stopAllSounds: " + std::string(e.what()));
    }
    catch (...) {
        AUDIO_LOG("Unknown exception in stopAllSounds");
    }
}

// Add a new method to restore volume
void restoreVolume() {
    try {
        if (!initialized_) {
            AUDIO_LOG("Audio Mixer not initialized, ignoring restoreVolume");
            return;
        }
        
        std::lock_guard<std::mutex> lock(mutex_);
        
            masterVolume_ = 1.0f;
            
            // Apply restored volume to all active sounds
            for (auto& sound : sounds_) {
                sound.volume = masterVolume_;
            }
            
            // Restore background music volume as well, if it exists
            if (backgroundMusic_) {
                backgroundMusic_->sound.volume = masterVolume_;
            }
            
            AUDIO_LOG("Volume restored successfully");
    }
    catch (const std::exception& e) {
        AUDIO_LOG("Exception in restoreVolume: " + std::string(e.what()));
    }
    catch (...) {
        AUDIO_LOG("Unknown exception in restoreVolume");
    }
}
    
    // Stop just the background music
    void stopBackgroundMusic() {
        try {
            std::lock_guard<std::mutex> lock(mutex_);
            
            if (!initialized_ || !backgroundMusic_) {
                AUDIO_LOG("No background music to stop");
                return;
            }
            
            AUDIO_LOG("Stopping background music");
            
            // Complete the promise if it exists
            if (backgroundMusic_->sound.completionPromise) {
                try {
                    backgroundMusic_->sound.completionPromise->set_value();
                } catch (const std::exception& e) {
                    AUDIO_LOG("Exception while completing music promise: " + 
                             std::string(e.what()));
                }
            }
            
            // Remove the background music
            backgroundMusic_.reset();
            
            // No need to reset the device - the mixer will just stop sending this audio
            
            // Notify mixer thread that the state has changed
            condVar_.notify_one();
        }
        catch (const std::exception& e) {
            AUDIO_LOG("Exception in stopBackgroundMusic: " + std::string(e.what()));
        }
        catch (...) {
            AUDIO_LOG("Unknown exception in stopBackgroundMusic");
        }
    }

private:
    // Special structure for background music with looping
    struct BackgroundMusic {
        WavSound sound;
        bool looping;
        
        BackgroundMusic() : looping(false) {}
    };

bool parseWavHeader(WavSound& sound) {
    try {
        const auto& data = sound.data;
        
        // Parse basic header info
        sound.channels = *reinterpret_cast<const uint16_t*>(&data[22]);
        sound.sampleRate = *reinterpret_cast<const uint32_t*>(&data[24]);
        sound.bitsPerSample = *reinterpret_cast<const uint16_t*>(&data[34]);
        
        // Support both 8-bit and 16-bit PCM
        if (sound.bitsPerSample != 8 && sound.bitsPerSample != 16) {
            AUDIO_LOG("Unsupported WAV format: " + std::to_string(sound.bitsPerSample) + 
                     " bits per sample (only 8-bit and 16-bit supported)");
            return false;
        }
        
        // Find the data chunk
        sound.dataOffset = 0;
        for (size_t i = 12; i < data.size() - 8; ) {
            if (memcmp(data.data() + i, "data", 4) == 0) {
                sound.dataOffset = i + 8;
                break;
            }
            
            uint32_t chunkSize = *reinterpret_cast<const uint32_t*>(&data[i + 4]);
            i += 8 + chunkSize + (chunkSize & 1);
            
            if (i >= data.size()) break;
        }
        
        if (sound.dataOffset == 0 || sound.dataOffset >= data.size()) {
            AUDIO_LOG("Data chunk not found in WAV file");
            return false;
        }
        
        // Calculate number of samples
        size_t dataSize = data.size() - sound.dataOffset;
        
        if (sound.bitsPerSample == 8) {
            // For 8-bit WAV, we need to convert to 16-bit for processing
            AUDIO_LOG("Converting 8-bit WAV to 16-bit for " + sound.soundName);
            
            // Store the original data
            std::vector<uint8_t> originalData = sound.data;
            
            // Create a new buffer for 16-bit samples (twice as large)
            size_t numSamples = dataSize;
            std::vector<uint8_t> newData(sound.dataOffset + (numSamples * 2));
            
            // Copy the header
            memcpy(newData.data(), originalData.data(), sound.dataOffset);
            
            // Convert 8-bit unsigned samples to 16-bit signed
            const uint8_t* src = originalData.data() + sound.dataOffset;
            int16_t* dst = reinterpret_cast<int16_t*>(newData.data() + sound.dataOffset);
            
            for (size_t i = 0; i < numSamples; i++) {
                // Convert 8-bit unsigned [0,255] to 16-bit signed [-32768,32767]
                dst[i] = static_cast<int16_t>((static_cast<int>(src[i]) - 128) * 256);
            }
            
            // Update sound data and values
            sound.data = std::move(newData);
            sound.bitsPerSample = 16;
            
            // Update the sample count
            sound.sampleCount = numSamples;
            sound.samples = reinterpret_cast<int16_t*>(const_cast<uint8_t*>(&sound.data[sound.dataOffset]));
            
            AUDIO_LOG("Successfully converted 8-bit WAV to 16-bit format");
            
            // Update WAV header fields in the new buffer
            // Update bits per sample
            *reinterpret_cast<uint16_t*>(&sound.data[34]) = 16;
            
            // Update block align (bytes per sample)
            uint16_t newBlockAlign = sound.channels * (16 / 8);
            *reinterpret_cast<uint16_t*>(&sound.data[32]) = newBlockAlign;
            
            // Update bytes per second
            uint32_t newBytesPerSec = sound.sampleRate * newBlockAlign;
            *reinterpret_cast<uint32_t*>(&sound.data[28]) = newBytesPerSec;
            
            // Update data chunk size
            uint32_t newDataSize = static_cast<uint32_t>(numSamples * 2);
            for (size_t i = 12; i < sound.dataOffset - 4; ) {
                if (memcmp(sound.data.data() + i, "data", 4) == 0) {
                    *reinterpret_cast<uint32_t*>(&sound.data[i + 4]) = newDataSize;
                    break;
                }
                i += 8 + *reinterpret_cast<const uint32_t*>(&sound.data[i + 4]);
            }
            
            // Update RIFF chunk size
            uint32_t newRiffSize = static_cast<uint32_t>(sound.data.size() - 8);
            *reinterpret_cast<uint32_t*>(&sound.data[4]) = newRiffSize;
        } else {
            // For 16-bit, just calculate sample count normally
            sound.sampleCount = dataSize / (sound.bitsPerSample / 8);
            sound.samples = reinterpret_cast<int16_t*>(const_cast<uint8_t*>(&sound.data[sound.dataOffset]));
        }
        
        return true;
    }
    catch (const std::exception& e) {
        AUDIO_LOG("Exception in parseWavHeader: " + std::string(e.what()));
        return false;
    }
    catch (...) {
        AUDIO_LOG("Unknown exception in parseWavHeader");
        return false;
    }
}    
    // Resample a sound to match the output sample rate
    void resampleSound(const WavSound& sound, float* resampledBuffer, 
                     size_t outputSamples, size_t startPos, size_t& endPos) {
        try {
            // Calculate ratio between source and target sample rates
            double ratio = static_cast<double>(sound.sampleRate) / OUTPUT_SAMPLE_RATE;
            
            // Simple linear interpolation resampling
            for (size_t i = 0; i < outputSamples; i++) {
                double srcPos = (startPos + i) * ratio;
                size_t srcPosInt = static_cast<size_t>(srcPos);
                float fraction = static_cast<float>(srcPos - srcPosInt);
                
                // If we've reached the end of the source data
                if (srcPosInt >= sound.sampleCount / sound.channels - 1) {
                    endPos = i;
                    return;
                }
                
                // For each channel
                for (uint16_t ch = 0; ch < sound.channels; ch++) {
                    // Get samples at position and next position
                    int16_t sample1 = sound.samples[(srcPosInt * sound.channels) + ch];
                    int16_t sample2 = sound.samples[((srcPosInt + 1) * sound.channels) + ch];
                    
                    // Linear interpolation
                    float sample = sample1 * (1.0f - fraction) + sample2 * fraction;
                    
                    // Store in output buffer, normalize to float [-1.0, 1.0]
                    resampledBuffer[(i * OUTPUT_CHANNELS) + (ch % OUTPUT_CHANNELS)] = 
                        sample / 32768.0f;
                }
            }
            
            endPos = outputSamples;
        }
        catch (const std::exception& e) {
            AUDIO_LOG("Exception in resampleSound: " + std::string(e.what()));
            endPos = 0;
        }
        catch (...) {
            AUDIO_LOG("Unknown exception in resampleSound");
            endPos = 0;
        }
    }
    
    // Mix all active sounds into a buffer with resampling
    void mixSounds(int16_t* outputBuffer, size_t sampleCount) {
        try {
            // First clear the output buffer
            std::memset(outputBuffer, 0, sampleCount * sizeof(int16_t));
            
            // Create a float buffer for mixing (to avoid clipping)
            std::vector<float> mixBuffer(sampleCount * OUTPUT_CHANNELS, 0.0f);
            
            std::lock_guard<std::mutex> lock(mutex_);
            
            // First mix the background music if it's playing
            if (backgroundMusic_ && backgroundMusic_->sound.isPlaying) {
                // Create temporary buffer for resampled data
                std::vector<float> resampledBuffer(sampleCount * OUTPUT_CHANNELS, 0.0f);
                
                // Calculate samples considering sample rate conversion
                size_t samplesResampled = 0;
                resampleSound(backgroundMusic_->sound, resampledBuffer.data(), sampleCount, 
                            backgroundMusic_->sound.position, samplesResampled);
                
                // Mix resampled sound to the float buffer
                for (size_t i = 0; i < samplesResampled * OUTPUT_CHANNELS; i++) {
                    // Apply volume - background music at half volume by default
                    float sample = resampledBuffer[i] * backgroundMusic_->sound.volume * 0.5f;
                    
                    // Add to mix buffer
                    mixBuffer[i] += sample;
                }
                
                // Update position 
                backgroundMusic_->sound.position += samplesResampled;
                
                // Check if we reached the end and need to loop
                if (samplesResampled < sampleCount || 
                    backgroundMusic_->sound.position >= backgroundMusic_->sound.sampleCount / backgroundMusic_->sound.channels) {
                    
                    if (backgroundMusic_->looping) {
                        // Loop back to beginning for looping music
                        AUDIO_LOG("Looping background music back to start position");
                        backgroundMusic_->sound.position = 0;
                    } else {
                        // Music is done playing and not looping
                        AUDIO_LOG("Background music finished playing");
                        backgroundMusic_->sound.isPlaying = false;
                        if (backgroundMusic_->sound.completionPromise) {
                            try {
                                backgroundMusic_->sound.completionPromise->set_value();
                            } catch (const std::exception& e) {
                                AUDIO_LOG("Exception while completing music promise: " + 
                                         std::string(e.what()));
                            }
backgroundMusic_->sound.completionPromise.reset();
                        }
                        backgroundMusic_.reset();
                    }
                }
            }
            
            // Now process each regular sound effect
            auto it = sounds_.begin();
            int activeCount = 0;
            while (it != sounds_.end()) {
                if (!it->isPlaying) {
                    // Clean up completed sounds
                    AUDIO_LOG("Sound completed: " + it->soundName);
                    if (it->completionPromise) {
                        try {
                            it->completionPromise->set_value();
                        } catch (const std::exception& e) {
                            AUDIO_LOG("Exception while completing promise: " + std::string(e.what()));
                        }
                    }
                    it = sounds_.erase(it);
                    continue;
                }
                
                // Create temporary buffer for resampled data
                std::vector<float> resampledBuffer(sampleCount * OUTPUT_CHANNELS, 0.0f);
                
                // Calculate samples considering sample rate conversion
                size_t samplesResampled = 0;
                resampleSound(*it, resampledBuffer.data(), sampleCount, 
                            it->position, samplesResampled);
                
                // Mix resampled sound to the float buffer
                for (size_t i = 0; i < samplesResampled * OUTPUT_CHANNELS; i++) {
                    // Apply volume
                    float sample = resampledBuffer[i] * it->volume;
                    
                    // Add to mix buffer
                    mixBuffer[i] += sample;
                }
                
                // Update position and check if finished
                it->position += samplesResampled;
                activeCount++;
                
                if (samplesResampled < sampleCount || it->position >= it->sampleCount / it->channels) {
                    AUDIO_LOG("Sound finished playing: " + it->soundName);
                    it->isPlaying = false;
                    if (it->completionPromise) {
                        try {
                            it->completionPromise->set_value();
                        } catch (const std::exception& e) {
                            AUDIO_LOG("Exception while completing promise: " + std::string(e.what()));
                        }
                    }
                    it = sounds_.erase(it);
                } else {
                    ++it;
                }
            }
            
            if (activeCount > 0) {
                AUDIO_LOG("Mixed " + std::to_string(activeCount) + " active sound effects");
            }
            
            // Convert float mix buffer back to int16_t with clipping protection
            for (size_t i = 0; i < sampleCount * OUTPUT_CHANNELS; i++) {
                float sample = mixBuffer[i];
                // Clip to [-1.0, 1.0]
                sample = std::max(-1.0f, std::min(1.0f, sample));
                // Convert to int16_t
                outputBuffer[i] = static_cast<int16_t>(sample * 32767.0f);
            }
        }
        catch (const std::exception& e) {
            AUDIO_LOG("Exception in mixSounds: " + std::string(e.what()));
            // Clear output buffer in case of exception
            std::memset(outputBuffer, 0, sampleCount * sizeof(int16_t));
        }
        catch (...) {
            AUDIO_LOG("Unknown exception in mixSounds");
            // Clear output buffer in case of exception
            std::memset(outputBuffer, 0, sampleCount * sizeof(int16_t));
        }
    }
    
    // Thread function for the mixer
    void mixerThreadFunc() {
        AUDIO_LOG("Mixer thread started");
        
        // Keep track of any errors
        int consecutiveErrors = 0;
        
        try {
            while (!exitThread_) {
                bool hasActiveSounds = false;
                bool hasBackgroundMusic = false;
                
                // Wait for buffer completion or new sounds
                {
                    std::unique_lock<std::mutex> lock(mutex_);
                    condVar_.wait(lock, [this] { 
                        return !sounds_.empty() || (backgroundMusic_ && backgroundMusic_->sound.isPlaying) || exitThread_; 
                    });
                    
                    if (exitThread_) {
                        AUDIO_LOG("Mixer thread received exit signal");
                        break;
                    }
                    
                    hasActiveSounds = !sounds_.empty();
                    hasBackgroundMusic = (backgroundMusic_ && backgroundMusic_->sound.isPlaying);
                }
                
                // Check which buffer is available
                int bufferIndex = -1;
                for (int i = 0; i < NUM_BUFFERS; i++) {
                    if (!(waveHeaders_[i].dwFlags & WHDR_INQUEUE)) {
                        bufferIndex = i;
                        break;
                    }
                }
                
                if (bufferIndex != -1) {
                    // Mix audio into this buffer
                    int16_t* outputBuffer = reinterpret_cast<int16_t*>(buffers_[bufferIndex].data());
                    
                    try {
                        mixSounds(outputBuffer, bufferSizeSamples_ / OUTPUT_CHANNELS);
                        
                        // Write the buffer to the output device
                        MMRESULT result = waveOutWrite(hWaveOut_, &waveHeaders_[bufferIndex], sizeof(WAVEHDR));
                        if (result != MMSYSERR_NOERROR) {
                            AUDIO_LOG("ERROR: Failed to write audio buffer, error: " + 
                                     std::to_string(result));
                            consecutiveErrors++;
                            
                            // If we've had too many consecutive errors, sleep for a bit
                            if (consecutiveErrors > 5) {
                                AUDIO_LOG("Too many consecutive errors, pausing mixer thread");
                                std::this_thread::sleep_for(std::chrono::milliseconds(100));
                                consecutiveErrors = 0;
                            }
                        } else {
                            // Reset error counter on success
                            consecutiveErrors = 0;
                        }
                    }
                    catch (const std::exception& e) {
                        AUDIO_LOG("Exception in mixer thread: " + std::string(e.what()));
                        std::this_thread::sleep_for(std::chrono::milliseconds(10));
                    }
                    catch (...) {
                        AUDIO_LOG("Unknown exception in mixer thread");
                        std::this_thread::sleep_for(std::chrono::milliseconds(10));
                    }
                } else {
                    // If all buffers are in use, wait a bit
                    std::this_thread::sleep_for(std::chrono::milliseconds(1));
                }
                
                // If there are no active sounds and no background music, sleep a bit longer
                // to reduce CPU usage when idle
                if (!hasActiveSounds && !hasBackgroundMusic) {
                    std::this_thread::sleep_for(std::chrono::milliseconds(10));
                }
            }
        }
        catch (const std::exception& e) {
            AUDIO_LOG("CRITICAL: Exception in mixer thread main loop: " + std::string(e.what()));
        }
        catch (...) {
            AUDIO_LOG("CRITICAL: Unknown exception in mixer thread main loop");
        }
        
        AUDIO_LOG("Mixer thread exiting");
    }
    
    // Static callback for waveOut
    static void CALLBACK waveOutProc(HWAVEOUT hwo, UINT uMsg, DWORD_PTR dwInstance, 
                                    DWORD_PTR dwParam1, DWORD_PTR dwParam2) {
        if (uMsg == WOM_DONE) {
            AudioMixer* mixer = reinterpret_cast<AudioMixer*>(dwInstance);
            if (mixer) {
                mixer->condVar_.notify_one();
            }
        }
    }
    
    // Private constructor for singleton
    AudioMixer() : initialized_(false), hWaveOut_(NULL), 
                  currentBuffer_(0), masterVolume_(1.0f), 
                  exitThread_(false), backgroundMusic_(std::nullopt) {
        AUDIO_LOG("AudioMixer instance created");
    }
    
    // Constants
    static constexpr int NUM_BUFFERS = 4;
    static constexpr int BUFFER_DURATION_MS = 50;
    static constexpr int OUTPUT_CHANNELS = 2;
    static constexpr int OUTPUT_SAMPLE_RATE = 44100;
    static constexpr int OUTPUT_BITS_PER_SAMPLE = 16;
    
    // Member variables
    bool initialized_;
    HWAVEOUT hWaveOut_;
    WAVEFORMATEX wfx_;
    std::vector<uint8_t> buffers_[NUM_BUFFERS];
    WAVEHDR waveHeaders_[NUM_BUFFERS];
    size_t bufferSizeSamples_;
    int currentBuffer_;
    float masterVolume_;
    
    std::list<WavSound> sounds_;
    std::optional<BackgroundMusic> backgroundMusic_;
    std::mutex mutex_;
    std::condition_variable condVar_;
    std::thread mixerThread_;
    std::atomic<bool> exitThread_;
};

// Windows implementation of AudioPlayer using the mixer
class WindowsAudioPlayer : public AudioPlayer {
public:
    WindowsAudioPlayer() : initialized_(false), volume_(1.0f) {
        AUDIO_LOG("Creating WindowsAudioPlayer instance");
    }
    
    ~WindowsAudioPlayer() override {
        AUDIO_LOG("Destroying WindowsAudioPlayer instance");
        shutdown();
    }

void restoreVolume() override {
    if (!initialized_) {
        AUDIO_LOG("WindowsAudioPlayer not initialized, ignoring restoreVolume");
        return;
    }
    
    AUDIO_LOG("WindowsAudioPlayer::restoreVolume");
    try {
        AudioMixer::getInstance().restoreVolume();
    }
    catch (const std::exception& e) {
        AUDIO_LOG("Exception in WindowsAudioPlayer::restoreVolume: " + std::string(e.what()));
    }
    catch (...) {
        AUDIO_LOG("Unknown exception in WindowsAudioPlayer::restoreVolume");
    }
}
    
    bool initialize() override {
        std::lock_guard<std::mutex> lock(mutex_);
        
        if (initialized_) {
            AUDIO_LOG("WindowsAudioPlayer already initialized");
            return true;
        }
        
        AUDIO_LOG("Initializing WindowsAudioPlayer");
        
        if (AudioMixer::getInstance().initialize()) {
            initialized_ = true;
            AUDIO_LOG("WindowsAudioPlayer initialized successfully");
            return true;
        }
        
        AUDIO_LOG("WindowsAudioPlayer initialization failed");
        return false;
    }
    
    void shutdown() override {
        std::lock_guard<std::mutex> lock(mutex_);
        
        if (!initialized_) {
            AUDIO_LOG("WindowsAudioPlayer not initialized, shutdown ignored");
            return;
        }
        
        AUDIO_LOG("Shutting down WindowsAudioPlayer");
        AudioMixer::getInstance().shutdown();
        initialized_ = false;
        AUDIO_LOG("WindowsAudioPlayer shutdown complete");
    }
    
    void playSound(const std::vector<uint8_t>& data, 
                  const std::string& format,
                  std::shared_ptr<std::promise<void>> completionPromise = nullptr) override {
        if (!initialized_) {
            AUDIO_LOG("WindowsAudioPlayer not initialized, ignoring playSound");
            if (completionPromise) {
                try {
                    completionPromise->set_value();
                } catch (const std::exception& e) {
                    AUDIO_LOG("Exception while completing promise: " + std::string(e.what()));
                }
            }
            return;
        }
        
        // Generate a unique name for debugging
        static int soundCounter = 0;
        std::string soundName = "Sound_" + std::to_string(++soundCounter);
        
        AUDIO_LOG("WindowsAudioPlayer::playSound '" + soundName + "', format: " + format);
        AudioMixer::getInstance().addSound(data, format, completionPromise, soundName);
    }
    
    bool playBackgroundMusic(const std::vector<uint8_t>& data, 
                            const std::string& format,
                            bool loop = true,
                            std::shared_ptr<std::promise<void>> completionPromise = nullptr) {
        if (!initialized_) {
            AUDIO_LOG("WindowsAudioPlayer not initialized, ignoring playBackgroundMusic");
            if (completionPromise) {
                try {
                    completionPromise->set_value();
                } catch (const std::exception& e) {
                    AUDIO_LOG("Exception while completing promise: " + std::string(e.what()));
                }
            }
            return false;
        }
        
        AUDIO_LOG("WindowsAudioPlayer::playBackgroundMusic, format: " + format + 
                 ", loop: " + (loop ? "yes" : "no"));
        return AudioMixer::getInstance().playBackgroundMusic(data, format, loop, completionPromise);
    }
    
    void stopBackgroundMusic() {
        if (!initialized_) {
            AUDIO_LOG("WindowsAudioPlayer not initialized, ignoring stopBackgroundMusic");
            return;
        }
        
        AUDIO_LOG("WindowsAudioPlayer::stopBackgroundMusic");
        AudioMixer::getInstance().stopBackgroundMusic();
    }
    
    void setVolume(float volume) override {
        std::lock_guard<std::mutex> lock(mutex_);
        
        float oldVolume = volume_;
        volume_ = std::max(0.0f, std::min(1.0f, volume));
        
        AUDIO_LOG("WindowsAudioPlayer::setVolume from " + std::to_string(oldVolume) + 
                 " to " + std::to_string(volume_));
        
        if (initialized_) {
            AudioMixer::getInstance().setVolume(volume_);
        }
    }
    
    void stopAllSounds() override {
        if (!initialized_) {
            AUDIO_LOG("WindowsAudioPlayer not initialized, ignoring stopAllSounds");
            return;
        }
        
        AUDIO_LOG("WindowsAudioPlayer::stopAllSounds");
        try {
            AudioMixer::getInstance().stopAllSounds();
        }
        catch (const std::exception& e) {
            AUDIO_LOG("Exception in WindowsAudioPlayer::stopAllSounds: " + std::string(e.what()));
        }
        catch (...) {
            AUDIO_LOG("Unknown exception in WindowsAudioPlayer::stopAllSounds");
        }
    }
    
private:
    bool initialized_;
    float volume_;
    std::mutex mutex_;
};

// Factory function implementation for Windows
std::unique_ptr<AudioPlayer> createAudioPlayer() {
    AUDIO_LOG("Creating WindowsAudioPlayer via factory function");
    return std::make_unique<WindowsAudioPlayer>();
}

#endif // _WIN32
