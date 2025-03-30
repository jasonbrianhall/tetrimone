#include <SDL2/SDL.h>
#include <SDL2/SDL_mixer.h>
#include <vector>
#include <iostream>
#include <cstring>

// Convert MP3 to WAV using SDL2_mixer
bool convertMp3ToWavInMemory(const std::vector<uint8_t>& mp3Data, std::vector<uint8_t>& wavData) {
    // Initialize SDL and SDL_mixer if not already initialized
    static bool sdl_initialized = false;
    if (!sdl_initialized) {
        if (SDL_Init(SDL_INIT_AUDIO) < 0) {
            std::cerr << "SDL could not initialize! SDL Error: " << SDL_GetError() << std::endl;
            return false;
        }
        
        // Initialize SDL_mixer with default frequency, format, channels, and chunksize
        if (Mix_OpenAudio(44100, MIX_DEFAULT_FORMAT, 2, 2048) < 0) {
            std::cerr << "SDL_mixer could not initialize! SDL_mixer Error: " << Mix_GetError() << std::endl;
            return false;
        }
        
        sdl_initialized = true;
    }
    
    // Create an SDL_RWops from the MP3 data
    SDL_RWops* rw = SDL_RWFromMem((void*)mp3Data.data(), mp3Data.size());
    if (!rw) {
        std::cerr << "Failed to create RWops from memory! SDL Error: " << SDL_GetError() << std::endl;
        return false;
    }
    
    // Load the MP3 as a chunk (using SDL_mixer)
    Mix_Chunk* chunk = Mix_LoadWAV_RW(rw, 0); // 0 means don't free the RWops when done
    
    // Make sure to close the RWops
    SDL_RWclose(rw);
    
    if (!chunk) {
        std::cerr << "Failed to load MP3! SDL_mixer Error: " << Mix_GetError() << std::endl;
        return false;
    }
    
    // At this point, SDL_mixer has decoded the MP3 to raw PCM data
    // chunk->abuf contains the audio data and chunk->alen is the length
    
    // Create a minimal WAV header
    struct WavHeader {
        // RIFF header
        char riff_header[4] = {'R', 'I', 'F', 'F'};
        uint32_t wav_size;        // File size - 8
        char wave_header[4] = {'W', 'A', 'V', 'E'};
        
        // Format chunk
        char fmt_header[4] = {'f', 'm', 't', ' '};
        uint32_t fmt_chunk_size = 16;
        uint16_t audio_format = 1; // PCM
        uint16_t num_channels = 2; // Stereo (SDL_mixer default)
        uint32_t sample_rate = 44100; // SDL_mixer default
        uint32_t byte_rate = sample_rate * num_channels * 2; // bytes per second
        uint16_t block_align = num_channels * 2; // bytes per sample
        uint16_t bits_per_sample = 16; // SDL_mixer default
        
        // Data chunk
        char data_header[4] = {'d', 'a', 't', 'a'};
        uint32_t data_bytes;      // chunk->alen
    };
    
    // Fill in the header with the correct values
    WavHeader header;
    header.data_bytes = chunk->alen;
    header.wav_size = 36 + header.data_bytes; // 36 is the size of the header excluding RIFF header
    
    // Create the WAV data with header + audio data
    wavData.resize(sizeof(header) + chunk->alen);
    
    // Copy the header to the beginning of wavData
    std::memcpy(wavData.data(), &header, sizeof(header));
    
    // Copy the PCM data after the header
    std::memcpy(wavData.data() + sizeof(header), chunk->abuf, chunk->alen);
    
    // Free the chunk
    Mix_FreeChunk(chunk);
    
    return true;
}
