#include <SDL2/SDL.h>
#include <SDL2/SDL_mixer.h>
#include <vector>
#include <iostream>
#include <cstring>
#include "midiplayer.h"
#include "dbopl_wrapper.h"

#ifdef WIN32
#include <windows.h>
#endif

// External variables and functions needed from midiplayer.cpp
extern double playTime;
extern bool isPlaying;
extern double playwait;
extern int globalVolume;
extern void processEvents();

bool convertMidiToWav(const char* midi_filename, const char* wav_filename, int volume);

// In-memory MIDI to WAV conversion function
bool convertMidiToWavInMemory(const std::vector<uint8_t>& midiData, std::vector<uint8_t>& wavData) {
    // Create a temporary file to write the MIDI data
    char tempMidiPath[L_tmpnam] = {0};
    playTime = 0.0;
    isPlaying = false;
    playwait = 0.0;

    
#ifdef WIN32
    // Windows-specific temp file handling
    char tempPath[MAX_PATH];
    char tempFileName[MAX_PATH];
    
    // Get the temp path
    DWORD tempPathResult = GetTempPath(MAX_PATH, tempPath);
    if (tempPathResult > MAX_PATH || tempPathResult == 0) {
        std::cerr << "Failed to get temporary path" << std::endl;
        return false;
    }
    
    // Generate a unique filename
    UINT tempFileResult = GetTempFileName(tempPath, "mid", 0, tempFileName);
    if (tempFileResult == 0) {
        std::cerr << "Failed to create temporary MIDI file name" << std::endl;
        return false;
    }
    
    strcpy(tempMidiPath, tempFileName);
#else
    // Use tmpnam for non-Windows platforms
    if (std::tmpnam(tempMidiPath) == NULL) {
        std::cerr << "Failed to create temporary MIDI file path" << std::endl;
        return false;
    }
#endif
    
    // Write MIDI data to temp file
    FILE* tempMidi = fopen(tempMidiPath, "wb");
    if (!tempMidi) {
        std::cerr << "Failed to open temporary MIDI file for writing" << std::endl;
        return false;
    }
    
    fwrite(midiData.data(), 1, midiData.size(), tempMidi);
    fclose(tempMidi);
    
    // Create a temporary file for the WAV output
    char tempWavPath[L_tmpnam] = {0};
    
#ifdef WIN32
    // Windows-specific temp file handling for WAV
    char tempWavFileName[MAX_PATH];
    
    // Generate a unique filename
    tempFileResult = GetTempFileName(tempPath, "wav", 0, tempWavFileName);
    if (tempFileResult == 0) {
        std::cerr << "Failed to create temporary WAV file name" << std::endl;
        remove(tempMidiPath);
        return false;
    }
    
    strcpy(tempWavPath, tempWavFileName);
#else
    // Use tmpnam for non-Windows platforms
    if (std::tmpnam(tempWavPath) == NULL) {
        std::cerr << "Failed to create temporary WAV file path" << std::endl;
        remove(tempMidiPath);
        return false;
    }
#endif
    
    // Use the existing convertMidiToWav function
    bool conversionSuccess = convertMidiToWav(tempMidiPath, tempWavPath, 1000);
    
    if (!conversionSuccess) {
        std::cerr << "MIDI to WAV conversion failed" << std::endl;
        remove(tempMidiPath);
        remove(tempWavPath);
        return false;
    }
    
    // Read the resulting WAV file into memory
    FILE* wavFile = fopen(tempWavPath, "rb");
    if (!wavFile) {
        std::cerr << "Failed to open temporary WAV file for reading" << std::endl;
        remove(tempMidiPath);
        remove(tempWavPath);
        return false;
    }
    
    // Get the WAV file size
    fseek(wavFile, 0, SEEK_END);
    long wavSize = ftell(wavFile);
    fseek(wavFile, 0, SEEK_SET);
    
    // Check specifically for files that are 44 bytes or less (WAV header with no data)
    if (wavSize <= 44) {
        std::cerr << "WAV file contains only header with no audio data (" << wavSize << " bytes)" << std::endl;
        fclose(wavFile);
        remove(tempMidiPath);
        remove(tempWavPath);
        return false;
    }
    
    // Read the entire WAV file
    wavData.resize(wavSize);
    size_t bytesRead = fread(wavData.data(), 1, wavSize, wavFile);
    fclose(wavFile);
    
    if (bytesRead != wavSize) {
        std::cerr << "Failed to read WAV data" << std::endl;
        remove(tempMidiPath);
        remove(tempWavPath);
        return false;
    }
    
    // Clean up temporary files
    remove(tempMidiPath);
    remove(tempWavPath);
    
    std::cerr << "MIDI to WAV conversion successful, read " << wavSize << " bytes of WAV data" << std::endl;
    
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
