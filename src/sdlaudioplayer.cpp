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
        
        std::cerr << "DEBUG: Initializing SDL Audio Player" << std::endl;
        
        // Initialize SDL audio subsystem
        if (SDL_WasInit(SDL_INIT_AUDIO) == 0) {
            if (SDL_InitSubSystem(SDL_INIT_AUDIO) < 0) {
                std::cerr << "SDL audio init failed: " << SDL_GetError() << std::endl;
                return false;
            }
        }
        
        // Initialize SDL_mixer with higher frequency for better music quality
        // Use a smaller buffer size on Windows for better responsiveness
        #ifdef _WIN32
        int bufferSize = 16384;
        #else
        int bufferSize = 4096;
        #endif
        
        if (Mix_OpenAudio(44100, MIX_DEFAULT_FORMAT, 2, bufferSize) < 0) {
            std::cerr << "SDL_mixer init failed: " << Mix_GetError() << std::endl;
            return false;
        }
        
        // Print out the actual audio specs we got
        int frequency, channels;
        Uint16 format;
        if (Mix_QuerySpec(&frequency, &format, &channels) == 0) {
            std::cerr << "DEBUG: Failed to query audio specs" << std::endl;
        } else {
            std::cerr << "DEBUG: Audio specs - frequency: " << frequency 
                      << ", format: " << format << ", channels: " << channels << std::endl;
        }
        
        // Initialize music and sound formats - be more explicit about what we're initializing
        int flags = 0;
        // Add MP3 support if available
        #ifdef MIX_INIT_MP3
        flags |= MIX_INIT_MP3;
        #endif
        // Add OGG support if available
        #ifdef MIX_INIT_OGG
        flags |= MIX_INIT_OGG;
        #endif
        // Add MOD support if available
        #ifdef MIX_INIT_MOD
        flags |= MIX_INIT_MOD;
        #endif
        // Add FLAC support if available
        #ifdef MIX_INIT_FLAC
        flags |= MIX_INIT_FLAC;
        #endif
        
        if (flags == 0) {
            std::cerr << "DEBUG: No audio formats to initialize" << std::endl;
        } else {
            int initted = Mix_Init(flags);
            if (initted == 0) {
                std::cerr << "DEBUG: All audio format initializations failed" << std::endl;
            } else {
                std::cerr << "DEBUG: Initialized audio formats: " << initted << std::endl;
                if ((initted & flags) != flags) {
                    std::cerr << "DEBUG: Some audio formats failed to initialize" << std::endl;
                }
            }
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
        std::cerr << "DEBUG: SDL Audio Player initialized successfully" << std::endl;
        return true;
    }
    
    void shutdown() override {
        if (!initialized_) {
            return;
        }
        
        // First, set initialized_ to false to prevent new audio from playing
        initialized_ = false;
        
        // Remove callbacks BEFORE clearing instance to avoid race conditions
        Mix_HookMusicFinished(nullptr);
        Mix_ChannelFinished(nullptr);
        
        // Reset the instance pointer before doing any cleanup
        // This ensures callbacks won't try to access data being cleaned up
        SDLAudioPlayer* oldInstance = instance_;
        instance_ = nullptr;
        
        // Make sure our instance check is consistent
        if (oldInstance != this) {
            std::cerr << "Warning: SDL Audio instance mismatch in shutdown" << std::endl;
        }
        
        // Clear all channels first
        Mix_HaltChannel(-1);
        
        // Free all chunks and music without holding the mutex during SDL calls
        {
            std::lock_guard<std::mutex> lock(mutex_);
            
            // Store chunks in a local structure to free outside of the lock
            std::vector<Mix_Chunk*> chunksToFree;
            for (auto& chunk : sound_chunks_) {
                if (chunk.second) {
                    chunksToFree.push_back(chunk.second);
                }
            }
            sound_chunks_.clear();
            
            // Clear all promises while we have the lock
            channel_promises_.clear();
            music_completion_promise_.reset();
            
            // Store current music to free outside the lock
            Mix_Music* musicToFree = current_music_;
            current_music_ = nullptr;
            
            // Release lock before SDL operations
            lock.~lock_guard();
            
            // Now free resources outside the lock
            if (musicToFree) {
                Mix_FreeMusic(musicToFree);
            }
            
            for (Mix_Chunk* chunk : chunksToFree) {
                Mix_FreeChunk(chunk);
            }
        }
        
        // Close audio and quit SDL_mixer
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
        
        // Debug output to help with troubleshooting
        std::cerr << "DEBUG: Playing sound, format: " << format << ", size: " << data.size() << " bytes" << std::endl;
        
        // Special handling for WAV files on Windows to ensure proper format detection
        bool isWav = false;
        if (format == "wav" || format == "WAV") {
            isWav = true;
            if (data.size() >= 12) {
                // Check WAV header magic numbers
                if (memcmp(data.data(), "RIFF", 4) == 0 &&
                    memcmp(data.data() + 8, "WAVE", 4) == 0) {
                    std::cerr << "DEBUG: Valid WAV header detected" << std::endl;
                } else {
                    std::cerr << "DEBUG: Invalid WAV header" << std::endl;
                }
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
        
        // Different approach based on format to better handle Windows compatibility
        bool tryAsMusic = false;
        Mix_Chunk* chunk = nullptr;
        
        // Handle WAV files directly first
        if (isWav) {
            chunk = Mix_LoadWAV_RW(rw, 0);  // 0 means don't free RWops yet
            if (!chunk) {
                std::cerr << "DEBUG: Failed to load WAV: " << Mix_GetError() << std::endl;
            }
        } 
        // For MP3 and OGG, try as music first on Windows
        else if (format == "mp3" || format == "ogg" || format == "MP3" || format == "OGG") {
            tryAsMusic = true;
        } 
        // For unknown formats, try both approaches
        else {
            chunk = Mix_LoadWAV_RW(rw, 0);
            if (!chunk) {
                tryAsMusic = true;
            }
        }
        
        // If we successfully loaded a chunk, play it as a sound effect
        if (chunk) {
            // Set volume
            Mix_VolumeChunk(chunk, static_cast<int>(MIX_MAX_VOLUME * volume_));
            
            // Find an available channel
            int channel = Mix_PlayChannel(-1, chunk, 0);  // 0 = no looping
            if (channel == -1) {
                std::cerr << "Failed to play sound: " << Mix_GetError() << std::endl;
                Mix_FreeChunk(chunk);
                SDL_RWclose(rw);
                if (completionPromise) {
                    completionPromise->set_value();
                }
                return;
            }
            
            std::cerr << "DEBUG: Playing sound on channel " << channel << std::endl;
            
            // Store the chunk and channel for later cleanup
            sound_chunks_[channel] = chunk;
            
            // Store the completion promise for this channel if needed
            if (completionPromise) {
                channel_promises_[channel] = completionPromise;
            }
            
            // Free the RWops now that we've loaded the sound
            SDL_RWclose(rw);
            return;
        }
        
        // If we need to try as music or if the chunk loading failed
        if (tryAsMusic) {
            // Make sure we're at the beginning of the data
            SDL_RWseek(rw, 0, RW_SEEK_SET);
            
            Mix_Music* music = Mix_LoadMUS_RW(rw, 1);  // 1 means SDL_RWops will be auto-freed
            if (!music) {
                std::cerr << "Failed to load audio as either sound or music: " << Mix_GetError() << std::endl;
                SDL_RWclose(rw);  // Close if we couldn't load as music (and auto-free is 0)
                if (completionPromise) {
                    completionPromise->set_value();
                }
                return;
            }
            
            std::cerr << "DEBUG: Successfully loaded as music" << std::endl;
            
            // Stop any currently playing music
            if (current_music_) {
                Mix_HaltMusic();
                Mix_FreeMusic(current_music_);
                current_music_ = nullptr;
            }
            
            // Store the new music
            current_music_ = music;
            
            // Store the completion promise for music
            music_completion_promise_ = completionPromise;
            
            // Set volume
            Mix_VolumeMusic(static_cast<int>(MIX_MAX_VOLUME * volume_));
            
            // Play once (not looping)
            if (Mix_PlayMusic(current_music_, 0) == -1) {
                std::cerr << "Failed to play music: " << Mix_GetError() << std::endl;
                Mix_FreeMusic(current_music_);
                current_music_ = nullptr;
                
                if (completionPromise) {
                    completionPromise->set_value();
                }
            } else {
                std::cerr << "DEBUG: Music playback started" << std::endl;
            }
        } else {
            // If we got here, WAV loading failed
            SDL_RWclose(rw);
            if (completionPromise) {
                completionPromise->set_value();
            }
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
