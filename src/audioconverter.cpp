#include <SDL2/SDL.h>
#include <SDL2/SDL_mixer.h>
#include <vector>
#include <iostream>
#include <cstring>
#include "midiplayer.h"
#include "dbopl_wrapper.h"

// External variables and functions needed from midiplayer.cpp
extern double playTime;
extern bool isPlaying;
extern double playwait;
extern int globalVolume;
extern void processEvents();

// In-memory MIDI to WAV conversion function
bool convertMidiToWavInMemory(const std::vector<uint8_t>& midiData, std::vector<uint8_t>& wavData) {
    // Create a temporary file to write the MIDI data
    char tempMidiPath[L_tmpnam];
    char tempWavPath[L_tmpnam];
    std::tmpnam(tempMidiPath);
    std::tmpnam(tempWavPath);
    
    // Write MIDI data to temp file
    FILE* tempMidi = fopen(tempMidiPath, "wb");
    if (!tempMidi) {
        return false;
    }
    fwrite(midiData.data(), 1, midiData.size(), tempMidi);
    fclose(tempMidi);
    
    // Reset global state variables
    playTime = 0;
    isPlaying = true;
    
    // Set global volume to 100% (default)
    globalVolume = 100;
    
    // Initialize SDL and audio systems if not already initialized
    static bool sdl_initialized = false;
    if (!sdl_initialized) {
        if (!initSDL()) {
            remove(tempMidiPath);
            return false;
        }
        sdl_initialized = true;
    }
    
    // Load MIDI file
    if (!loadMidiFile(tempMidiPath)) {
        remove(tempMidiPath);
        return false;
    }
    
    // Create WAV header
    struct WavHeader {
        // RIFF header
        char riff_header[4] = {'R', 'I', 'F', 'F'};
        uint32_t wav_size;        // File size - 8
        char wave_header[4] = {'W', 'A', 'V', 'E'};
        
        // Format chunk
        char fmt_header[4] = {'f', 'm', 't', ' '};
        uint32_t fmt_chunk_size = 16;
        uint16_t audio_format = 1; // PCM
        uint16_t num_channels = 2; // Stereo
        uint32_t sample_rate = SAMPLE_RATE;
        uint32_t byte_rate = sample_rate * num_channels * 2; // bytes per second
        uint16_t block_align = num_channels * 2; // bytes per sample
        uint16_t bits_per_sample = 16;
        
        // Data chunk
        char data_header[4] = {'d', 'a', 't', 'a'};
        uint32_t data_bytes;      // Size of actual audio data
    };
    
    // Temporary buffer for audio generation
    const int AUDIO_BUFFER_SIZE = 4096;
    std::vector<int16_t> audioBuffer(AUDIO_BUFFER_SIZE * 2); // Stereo
    std::vector<uint8_t> tempWavData;
    
    // Duration of an audio buffer in seconds
    double buffer_duration = (double)AUDIO_BUFFER_SIZE / SAMPLE_RATE;
    
    // Begin conversion
    // Initialize playwait for the first events
    processEvents();
    
    // Continue processing as long as the MIDI is still playing
    while (isPlaying) {
        // Generate audio block
        std::memset(audioBuffer.data(), 0, audioBuffer.size() * sizeof(int16_t));
        OPL_Generate(audioBuffer.data(), AUDIO_BUFFER_SIZE);
        
        // Append to WAV data
        size_t currentSize = tempWavData.size();
        tempWavData.resize(currentSize + audioBuffer.size() * sizeof(int16_t));
        std::memcpy(tempWavData.data() + currentSize, audioBuffer.data(), 
                   audioBuffer.size() * sizeof(int16_t));
        
        // Update playTime
        playTime += buffer_duration;
        
        // Process events if needed
        playwait -= buffer_duration;
        
        // Process events when timer reaches zero or below
        while (playwait <= 0 && isPlaying) {
            processEvents();
        }
    }
    
    // Create and fill the WAV header
    WavHeader header;
    header.data_bytes = tempWavData.size();
    header.wav_size = 36 + header.data_bytes; // 36 is the size of the header excluding RIFF header
    
    // Create the final WAV data with header + audio data
    wavData.resize(sizeof(header) + tempWavData.size());
    
    // Copy the header to the beginning of wavData
    std::memcpy(wavData.data(), &header, sizeof(header));
    
    // Copy the PCM data after the header
    std::memcpy(wavData.data() + sizeof(header), tempWavData.data(), tempWavData.size());
    
    // Clean up temp files
    remove(tempMidiPath);
    
    // Reset OPL state
    OPL_Reset();
    
    return true;
}


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
