#include "audiomanager.h"
#include <SDL2/SDL.h>
#include <SDL2/SDL_mixer.h>
#include <iostream>
#include <vector>
#include <map>
#include <mutex>
#include <memory>
#include <cstring>

class SDLAudioPlayer : public AudioPlayer {
public:
    SDLAudioPlayer() : initialized_(false), volume_(1.0f), current_music_(nullptr) {}
    
    ~SDLAudioPlayer() override {
        shutdown();
    }
    
    bool initialize() override {
        if (initialized_) {
            return true;
        }
        
        // Initialize SDL audio subsystem
        if (SDL_WasInit(SDL_INIT_AUDIO) == 0) {
            if (SDL_InitSubSystem(SDL_INIT_AUDIO) < 0) {
                std::cerr << "SDL audio init failed: " << SDL_GetError() << std::endl;
                return false;
            }
        }
        
        // Initialize SDL_mixer
        if (Mix_OpenAudio(44100, MIX_DEFAULT_FORMAT, 2, 2048) < 0) {
            std::cerr << "SDL_mixer init failed: " << Mix_GetError() << std::endl;
            return false;
        }
        
        // Allocate channels for mixing
        Mix_AllocateChannels(16);
        
        // Set up callback for when sound finishes
        Mix_ChannelFinished(channelFinishedCallback);
        
        // Store a reference to this instance for the callback
        instance_ = this;
        
        initialized_ = true;
        return true;
    }
    
    void shutdown() override {
        if (!initialized_) {
            return;
        }
        
        // Free all chunks and music
        {
            std::lock_guard<std::mutex> lock(mutex_);
            for (auto& chunk : sound_chunks_) {
                if (chunk.second) {
                    Mix_FreeChunk(chunk.second);
                }
            }
            sound_chunks_.clear();
            
            if (current_music_) {
                Mix_FreeMusic(current_music_);
                current_music_ = nullptr;
            }
        }
        
        // Reset the instance pointer
        instance_ = nullptr;
        
        // Close SDL_mixer
        Mix_CloseAudio();
        
        initialized_ = false;
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
        
        std::lock_guard<std::mutex> lock(mutex_);
        
        // Create SDL_RWops from memory
        SDL_RWops* rw = SDL_RWFromConstMem(data.data(), data.size());
        if (!rw) {
            std::cerr << "Failed to create RWops: " << SDL_GetError() << std::endl;
            if (completionPromise) {
                completionPromise->set_value();
            }
            return;
        }
        
        // Try to load as music (for background)
        if (data.size() > 100000) {  // If larger than ~100KB, assume it's music
            Mix_Music* music = Mix_LoadMUS_RW(rw, 0);
            if (music) {
                // Free previous music if any
                if (current_music_) {
                    Mix_FreeMusic(current_music_);
                }
                
                current_music_ = music;
                Mix_PlayMusic(current_music_, -1);  // Loop indefinitely
                
                if (completionPromise) {
                    completionPromise->set_value();
                }
                return;
            }
        }
        
        // Load as sound effect
        Mix_Chunk* chunk = Mix_LoadWAV_RW(rw, 0);
        if (!chunk) {
            std::cerr << "Failed to load sound: " << Mix_GetError() << std::endl;
            SDL_RWclose(rw);
            if (completionPromise) {
                completionPromise->set_value();
            }
            return;
        }
        
        // Set volume
        Mix_VolumeChunk(chunk, static_cast<int>(MIX_MAX_VOLUME * volume_));
        
        // Find an available channel
        int channel = Mix_PlayChannel(-1, chunk, 0);
        if (channel == -1) {
            std::cerr << "Failed to play sound: " << Mix_GetError() << std::endl;
            Mix_FreeChunk(chunk);
            if (completionPromise) {
                completionPromise->set_value();
            }
            return;
        }
        
        // Store the chunk and channel for later cleanup
        sound_chunks_[channel] = chunk;
        
        // Complete the promise immediately, as SDL_mixer handles playback asynchronously
        if (completionPromise) {
            completionPromise->set_value();
        }
    }
    
    void stopAllSounds() override {
        if (!initialized_) {
            return;
        }
        
        std::lock_guard<std::mutex> lock(mutex_);
        
        // Stop all channels
        Mix_HaltChannel(-1);
        
        // Stop music
        Mix_HaltMusic();
    }
    
    void setVolume(float volume) override {
        volume_ = std::max(0.0f, std::min(1.0f, volume));
        
        if (initialized_) {
            std::lock_guard<std::mutex> lock(mutex_);
            
            // Set master volume for channels
            Mix_Volume(-1, static_cast<int>(MIX_MAX_VOLUME * volume_));
            
            // Set music volume
            Mix_VolumeMusic(static_cast<int>(MIX_MAX_VOLUME * volume_));
            
            // Also update individual chunk volumes
            for (auto& chunk : sound_chunks_) {
                Mix_VolumeChunk(chunk.second, static_cast<int>(MIX_MAX_VOLUME * volume_));
            }
        }
    }
    
    void onSoundFinished(int channel) {
        std::lock_guard<std::mutex> lock(mutex_);
        
        auto it = sound_chunks_.find(channel);
        if (it != sound_chunks_.end()) {
            Mix_FreeChunk(it->second);
            sound_chunks_.erase(it);
        }
    }
    
private:
    static void channelFinishedCallback(int channel) {
        if (instance_) {
            instance_->onSoundFinished(channel);
        }
    }
    
    bool initialized_;
    float volume_;
    Mix_Music* current_music_;
    std::map<int, Mix_Chunk*> sound_chunks_;
    std::mutex mutex_;
    
    // Static instance pointer for callbacks
    static SDLAudioPlayer* instance_;
};

// Initialize static member
SDLAudioPlayer* SDLAudioPlayer::instance_ = nullptr;

// Factory function implementation
std::unique_ptr<AudioPlayer> createAudioPlayer() {
    return std::make_unique<SDLAudioPlayer>();
}
