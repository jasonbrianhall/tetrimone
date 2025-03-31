#include "audiomanager.h"
#include <SDL2/SDL.h>
#include <SDL2/SDL_mixer.h>
#include <iostream>
#include <vector>
#include <map>
#include <mutex>
#include <memory>
#include <cstring>
#include <algorithm>

class SDLAudioPlayer : public AudioPlayer {
public:
    SDLAudioPlayer() : initialized_(false), volume_(1.0f), musicVolume_(1.0f),
                      isMuted_(false) {}
    
    ~SDLAudioPlayer() override {
        shutdown();
    }
    
    bool initialize() override {
        if (initialized_) {
            return true;
        }
        
        std::cout << "Initializing SDL Audio Player" << std::endl;
        
        // Initialize SDL audio subsystem
        if (SDL_WasInit(SDL_INIT_AUDIO) == 0) {
            if (SDL_InitSubSystem(SDL_INIT_AUDIO) < 0) {
                std::cerr << "SDL audio init failed: " << SDL_GetError() << std::endl;
                return false;
            }
        }
        
        // Initialize SDL_mixer
        int bufferSize = 4096;
        
        if (Mix_OpenAudio(44100, MIX_DEFAULT_FORMAT, 2, bufferSize) < 0) {
            std::cerr << "SDL_mixer init failed: " << Mix_GetError() << std::endl;
            return false;
        }
        
        // Initialize codecs
        int flags = 0;
        #ifdef MIX_INIT_MP3
        flags |= MIX_INIT_MP3;
        #endif
        #ifdef MIX_INIT_OGG
        flags |= MIX_INIT_OGG;
        #endif
        
        if (flags != 0) {
            Mix_Init(flags);
        }
        
        // Allocate channels
        Mix_AllocateChannels(32);
        
        // Set up callbacks
        Mix_ChannelFinished(channelFinishedCallback);
        
        // Store instance for callbacks
        instance_ = this;
        
        initialized_ = true;
        std::cout << "SDL Audio Player initialized successfully" << std::endl;
        return true;
    }
    
    void shutdown() override {
        if (!initialized_) {
            return;
        }
        
        initialized_ = false;
        
        // Remove callbacks
        Mix_ChannelFinished(nullptr);
        
        // Reset instance
        SDLAudioPlayer* oldInstance = instance_;
        instance_ = nullptr;
        
        // Stop all audio
        Mix_HaltChannel(-1);
        
        // Free chunks
        for (auto& chunk : sound_chunks_) {
            if (chunk.second) {
                Mix_FreeChunk(chunk.second);
            }
        }
        sound_chunks_.clear();
        sound_is_music_.clear();
        
        // Clear promises
        channel_promises_.clear();
        
        // Close audio
        Mix_CloseAudio();
        Mix_Quit();
        
        if (SDL_WasInit(SDL_INIT_AUDIO)) {
            SDL_QuitSubSystem(SDL_INIT_AUDIO);
        }
        
        std::cout << "SDL Audio Player shut down" << std::endl;
    }
    
    void playSound(const std::vector<uint8_t>& data, 
                  const std::string& format,
                  std::shared_ptr<std::promise<void>> completionPromise = nullptr) override {
        if (!initialized_) {
            if (completionPromise) {
                completionPromise->set_value();
            }
            return;
        }
        
        // Determine if this is music based on format or duration
        bool isMusic = false;
        
        // Check WAV duration
        if (format == "wav" || format == "WAV") {
            if (data.size() >= 44 && 
                memcmp(data.data(), "RIFF", 4) == 0 && 
                memcmp(data.data() + 8, "WAVE", 4) == 0) {
                
                // Find 'fmt ' and 'data' chunks
                uint32_t sampleRate = 44100;
                uint16_t channels = 2;
                uint16_t bitsPerSample = 16;
                uint32_t dataSize = 0;
                
                for (size_t i = 12; i < data.size() - 8;) {
                    char chunkId[5] = {0};
                    memcpy(chunkId, &data[i], 4);
                    uint32_t chunkSize = *reinterpret_cast<const uint32_t*>(&data[i + 4]);
                    
                    if (strcmp(chunkId, "fmt ") == 0) {
                        channels = *reinterpret_cast<const uint16_t*>(&data[i + 10]);
                        sampleRate = *reinterpret_cast<const uint32_t*>(&data[i + 12]);
                        bitsPerSample = *reinterpret_cast<const uint16_t*>(&data[i + 22]);
                    }
                    
                    if (strcmp(chunkId, "data") == 0) {
                        dataSize = chunkSize;
                        
                        // Calculate duration
                        uint32_t bytesPerSample = bitsPerSample / 8;
                        if (bytesPerSample > 0 && channels > 0 && sampleRate > 0) {
                            uint32_t bytesPerSecond = sampleRate * channels * bytesPerSample;
                            if (bytesPerSecond > 0) {
                                float durationInSeconds = static_cast<float>(dataSize) / bytesPerSecond;
                                isMusic = (durationInSeconds > 3.0f);
                                std::cout << "Audio duration: " << durationInSeconds << "s, classified as " 
                                          << (isMusic ? "music" : "sound effect") << std::endl;
                            }
                        }
                        break;
                    }
                    
                    i += 8 + chunkSize + (chunkSize & 1);
                    if (i >= data.size()) break;
                }
            }
        } else {
            // MP3, OGG are considered music
            isMusic = (format == "mp3" || format == "ogg" || format == "MP3" || format == "OGG");
        }
        
        // Create RWops from memory
        SDL_RWops* rw = SDL_RWFromConstMem(data.data(), data.size());
        if (!rw) {
            std::cerr << "Failed to create RWops: " << SDL_GetError() << std::endl;
            if (completionPromise) {
                completionPromise->set_value();
            }
            return;
        }
        
        // Load sound
        Mix_Chunk* chunk = Mix_LoadWAV_RW(rw, 1); // 1 = auto-free
        if (!chunk) {
            std::cerr << "Failed to load sound: " << Mix_GetError() << std::endl;
            if (completionPromise) {
                completionPromise->set_value();
            }
            return;
        }
        
        // Play the sound
        int channel = Mix_PlayChannel(-1, chunk, 0);
        if (channel == -1) {
            std::cerr << "Failed to play sound: " << Mix_GetError() << std::endl;
            Mix_FreeChunk(chunk);
            if (completionPromise) {
                completionPromise->set_value();
            }
            return;
        }
        
        // Store info about this sound
        sound_chunks_[channel] = chunk;
        sound_is_music_[channel] = isMusic;
        
        // Store promise
        if (completionPromise) {
            channel_promises_[channel] = completionPromise;
        }
        
        // Apply volume (after playing)
        float effectiveVolume = isMusic ? (volume_ * musicVolume_) : volume_;
        if (isMuted_) {
            effectiveVolume = 0.0f;
        }
        
        Mix_Volume(channel, static_cast<int>(MIX_MAX_VOLUME * effectiveVolume));
        
        std::cout << "Playing " << (isMusic ? "music" : "sound effect") << " on channel " << channel 
                 << " with volume " << static_cast<int>(MIX_MAX_VOLUME * effectiveVolume) << std::endl;
    }
    
    void stopAllSounds() override {
        if (!initialized_) {
            return;
        }
        
        std::lock_guard<std::mutex> lock(mutex_);
        
        // Set muted flag
        isMuted_ = true;
        
        // Set all channel volumes to 0
        for (auto& pair : sound_chunks_) {
            Mix_Volume(pair.first, 0);
        }
        
        std::cout << "All sounds muted" << std::endl;
    }
    
    void restoreVolume() override {
        if (!initialized_) {
            return;
        }
        
        std::lock_guard<std::mutex> lock(mutex_);
        
        // Unmute
        isMuted_ = false;
        
        // Restore volumes
        for (auto& pair : sound_chunks_) {
            int channel = pair.first;
            bool isMusic = sound_is_music_[channel];
            float effectiveVolume = isMusic ? (volume_ * musicVolume_) : volume_;
            
            Mix_Volume(channel, static_cast<int>(MIX_MAX_VOLUME * effectiveVolume));
        }
        
        std::cout << "Volume restored" << std::endl;
    }
    
    void setVolume(float volume) override {
        std::lock_guard<std::mutex> lock(mutex_);
        
        volume_ = std::clamp(volume, 0.0f, 1.0f);
        std::cout << "Master volume set to " << volume_ << std::endl;
        
        if (!isMuted_) {
            // Update volumes for all channels
            for (auto& pair : sound_chunks_) {
                int channel = pair.first;
                bool isMusic = sound_is_music_[channel];
                float effectiveVolume = isMusic ? (volume_ * musicVolume_) : volume_;
                
                Mix_Volume(channel, static_cast<int>(MIX_MAX_VOLUME * effectiveVolume));
            }
        }
    }
    
    void setMusicVolume(float volume) override {
        std::lock_guard<std::mutex> lock(mutex_);
        
        musicVolume_ = std::clamp(volume, 0.0f, 1.0f);
        std::cout << "Music volume set to " << musicVolume_ << std::endl;
        
        if (!isMuted_) {
            // Update volumes for music channels only
            for (auto& pair : sound_chunks_) {
                int channel = pair.first;
                bool isMusic = sound_is_music_[channel];
                
                if (isMusic) {
                    float effectiveVolume = volume_ * musicVolume_;
                    Mix_Volume(channel, static_cast<int>(MIX_MAX_VOLUME * effectiveVolume));
                }
            }
        }
    }
    
    float getMusicVolume() const {
        return musicVolume_;
    }
    
private:
    static void channelFinishedCallback(int channel) {
        if (instance_) {
            instance_->onChannelFinished(channel);
        }
    }
    
    void onChannelFinished(int channel) {
        std::lock_guard<std::mutex> lock(mutex_);
        
        // Complete promise
        auto promiseIt = channel_promises_.find(channel);
        if (promiseIt != channel_promises_.end()) {
            if (promiseIt->second) {
                promiseIt->second->set_value();
            }
            channel_promises_.erase(promiseIt);
        }
        
        // Free chunk
        auto chunkIt = sound_chunks_.find(channel);
        if (chunkIt != sound_chunks_.end()) {
            Mix_FreeChunk(chunkIt->second);
            sound_chunks_.erase(chunkIt);
        }
        
        // Remove music flag
        sound_is_music_.erase(channel);
    }
    
    bool initialized_;
    float volume_;
    float musicVolume_;
    bool isMuted_;
    
    std::map<int, Mix_Chunk*> sound_chunks_;
    std::map<int, bool> sound_is_music_;
    std::map<int, std::shared_ptr<std::promise<void>>> channel_promises_;
    std::mutex mutex_;
    
    static SDLAudioPlayer* instance_;
};

// Initialize static member
SDLAudioPlayer* SDLAudioPlayer::instance_ = nullptr;

// Factory function implementation
std::unique_ptr<AudioPlayer> createAudioPlayer() {
    return std::make_unique<SDLAudioPlayer>();
}
