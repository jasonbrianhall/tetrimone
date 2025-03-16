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
        
        // Initialize SDL_mixer with higher frequency for better music quality
        if (Mix_OpenAudio(44100, MIX_DEFAULT_FORMAT, 2, 4096) < 0) {
            std::cerr << "SDL_mixer init failed: " << Mix_GetError() << std::endl;
            return false;
        }
        
        // Initialize music and sound formats
        int flags = MIX_INIT_MP3 | MIX_INIT_OGG;
        int initted = Mix_Init(flags);
        if ((initted & flags) != flags) {
            std::cerr << "Mix_Init: Failed to init required formats! " << Mix_GetError() << std::endl;
            // Continue anyway, as we might still be able to play some formats
        }
        
        // Allocate channels for mixing
        Mix_AllocateChannels(16);
        
        // Set up callback for when sound finishes
        Mix_ChannelFinished(channelFinishedCallback);
        
        // Set up music finished callback
        Mix_HookMusicFinished(musicFinishedCallback);
        
        // Store a reference to this instance for the callback
        instance_ = this;
        
        initialized_ = true;
        return true;
    }
    
void shutdown() override {
    if (!initialized_) {
        return;
    }
    
    // First, set initialized_ to false to prevent new audio from playing
    initialized_ = false;
    
    // Free all chunks and music
    {
        std::lock_guard<std::mutex> lock(mutex_);
        
        // Halt all playing channels first
        Mix_HaltChannel(-1);  // -1 means all channels
        
        // Then halt music (with a brief fade to avoid pops)
        if (current_music_) {
            Mix_FadeOutMusic(100);  // 100ms fade
            
            // Brief delay to allow the fadeout to take effect
            SDL_Delay(150);
            
            Mix_HaltMusic();
            Mix_FreeMusic(current_music_);
            current_music_ = nullptr;
        }
        
        // Free all sound chunks
        for (auto& chunk : sound_chunks_) {
            if (chunk.second) {
                Mix_FreeChunk(chunk.second);
            }
        }
        sound_chunks_.clear();
        
        // Clear all promises
        channel_promises_.clear();
        music_completion_promise_.reset();
    }
    
    // Reset the instance pointer before quitting SDL_mixer
    instance_ = nullptr;
    
    // Quit SDL_mixer and cleanup with proper sequence
    Mix_HookMusicFinished(nullptr);  // Remove callback
    Mix_ChannelFinished(nullptr);    // Remove callback
    
    Mix_CloseAudio();
    Mix_Quit();
    
    // If we initialized SDL Audio ourselves, quit that subsystem
    if (SDL_WasInit(SDL_INIT_AUDIO)) {
        SDL_QuitSubSystem(SDL_INIT_AUDIO);
    }
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
        
        // Determine if this is likely a music file based on format and naming convention
        bool isMusic = false;
        std::string formatLower = format;
        std::transform(formatLower.begin(), formatLower.end(), formatLower.begin(), ::tolower);
        
        // Check if the format or filename suggests it's music (more reliable than size)
        if (formatLower == "mp3" || formatLower == "ogg" || formatLower == "wav") {
            // Background music is typically a longer file
            // The BackgroundMusic enum in SoundEvent is 0, which we can check
            // based on data characteristics or naming pattern
            if (data.size() > 500000) {  // Larger than 500KB is likely music
                isMusic = true;
            }
        }
        
        // Create SDL_RWops from memory
        SDL_RWops* rw = SDL_RWFromConstMem(data.data(), data.size());
        if (!rw) {
            std::cerr << "Failed to create RWops: " << SDL_GetError() << std::endl;
            if (completionPromise) {
                completionPromise->set_value();
            }
            return;
        }
        
        // Store the completion promise for music if needed
        if (isMusic) {
            music_completion_promise_ = completionPromise;
        }
        
        if (isMusic) {
            // Stop any currently playing music
            if (current_music_) {
                Mix_HaltMusic();
                Mix_FreeMusic(current_music_);
                current_music_ = nullptr;
            }
            
            // Attempt to load as music
            Mix_Music* music = Mix_LoadMUS_RW(rw, 1);  // 1 means SDL_RWops will be auto-freed
            if (music) {
                current_music_ = music;
                Mix_VolumeMusic(static_cast<int>(MIX_MAX_VOLUME * volume_));
                
                // Play once (0) for sound effects, -1 for looping background music
                int loops = 0;  // Default to playing once
                
                // If we're playing background music, loop it indefinitely
                if (data.size() > 500000) {  // Larger than 500KB is likely background music
                    loops = -1;  // Loop indefinitely
                }
                
                if (Mix_PlayMusic(current_music_, loops) == -1) {
                    std::cerr << "Failed to play music: " << Mix_GetError() << std::endl;
                    Mix_FreeMusic(current_music_);
                    current_music_ = nullptr;
                    
                    if (completionPromise) {
                        completionPromise->set_value();
                    }
                }
                
                // If we're not looping and there's no music finished callback to handle it,
                // we need to complete the promise now
                if (loops == 0 && completionPromise) {
                    completionPromise->set_value();
                }
                
                return;
            } else {
                std::cerr << "Failed to load music: " << Mix_GetError() << std::endl;
                // Fall back to trying as a sound effect
                SDL_RWclose(rw);
                rw = SDL_RWFromConstMem(data.data(), data.size());
                if (!rw) {
                    if (completionPromise) {
                        completionPromise->set_value();
                    }
                    return;
                }
            }
        }
        
        // If we get here, load as sound effect
        Mix_Chunk* chunk = Mix_LoadWAV_RW(rw, 1);  // 1 means SDL_RWops will be auto-freed
        if (!chunk) {
            std::cerr << "Failed to load sound: " << Mix_GetError() << std::endl;
            if (completionPromise) {
                completionPromise->set_value();
            }
            return;
        }
        
        // Set volume
        Mix_VolumeChunk(chunk, static_cast<int>(MIX_MAX_VOLUME * volume_));
        
        // Find an available channel
        int channel = Mix_PlayChannel(-1, chunk, 0);  // 0 = no looping
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
        
        // Store the completion promise for this channel if needed
        if (completionPromise) {
            channel_promises_[channel] = completionPromise;
        }
    }
    
void stopAllSounds() override {
    if (!initialized_) {
        return;
    }
    
    try {
        std::lock_guard<std::mutex> lock(mutex_);
        
        // First, safely stop music playback
        if (current_music_) {
            // FadeOut is much safer than an immediate halt
            Mix_FadeOutMusic(100); // Fade out over 100ms
        }
        
        // Create a local copy of active channels
        std::vector<int> active_channels;
        for (const auto& pair : sound_chunks_) {
            active_channels.push_back(pair.first);
        }
        
        // Individually halt each channel to avoid a mass channel halt
        // which can cause race conditions and crashes
        for (int channel : active_channels) {
            if (Mix_Playing(channel)) {
                Mix_FadeOutChannel(channel, 50); // Fade out over 50ms
            }
        }
        
        // Give SDL a brief moment to process the stop commands
        // This prevents race conditions with callbacks
        SDL_Delay(10);
        
        // We don't immediately free the chunks here - they'll be cleaned up
        // by the channel finished callbacks
    }
    catch (const std::exception& e) {
        std::cerr << "Exception in stopAllSounds: " << e.what() << std::endl;
        // Continue execution, don't let exceptions escape
    }
    catch (...) {
        std::cerr << "Unknown exception in stopAllSounds" << std::endl;
        // Continue execution, don't let exceptions escape
    }
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
        
        // Complete the promise for this channel if it exists
        auto promiseIt = channel_promises_.find(channel);
        if (promiseIt != channel_promises_.end()) {
            if (promiseIt->second) {
                promiseIt->second->set_value();
            }
            channel_promises_.erase(promiseIt);
        }
        
        // Free the chunk
        auto it = sound_chunks_.find(channel);
        if (it != sound_chunks_.end()) {
            Mix_FreeChunk(it->second);
            sound_chunks_.erase(it);
        }
    }
    
    void onMusicFinished() {
        std::lock_guard<std::mutex> lock(mutex_);
        
        // Complete the music promise if it exists
        if (music_completion_promise_) {
            music_completion_promise_->set_value();
            music_completion_promise_.reset();
        }
        
        // Don't free the music here - it will be freed when new music is loaded or on shutdown
    }
    
private:
    static void channelFinishedCallback(int channel) {
        if (instance_) {
            instance_->onSoundFinished(channel);
        }
    }
    
    static void musicFinishedCallback() {
        if (instance_) {
            instance_->onMusicFinished();
        }
    }
    
    bool initialized_;
    float volume_;
    Mix_Music* current_music_;
    std::map<int, Mix_Chunk*> sound_chunks_;
    std::map<int, std::shared_ptr<std::promise<void>>> channel_promises_;
    std::shared_ptr<std::promise<void>> music_completion_promise_;
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
