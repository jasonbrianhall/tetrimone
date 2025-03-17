#include <SDL2/SDL.h>
#include <SDL2/SDL_mixer.h>
#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <cstdio>
#include <thread>
#include <chrono>

// Redirect stdout/stderr to a file for logging
void setupLogging() {
    freopen("sdl_audio_test_log.txt", "w", stdout);
    freopen("sdl_audio_test_log.txt", "a", stderr);
    std::cout << "=== SDL Audio Device Test Started ===" << std::endl;
}

// Load a WAV file into memory
bool loadWavFile(const std::string& filename, std::vector<Uint8>& data) {
    std::ifstream file(filename, std::ios::binary);
    if (!file.is_open()) {
        std::cerr << "Error: Could not open file: " << filename << std::endl;
        return false;
    }

    // Get file size
    file.seekg(0, std::ios::end);
    size_t fileSize = file.tellg();
    file.seekg(0, std::ios::beg);
    
    std::cout << "File size: " << fileSize << " bytes" << std::endl;
    
    // Read entire file into vector
    data.resize(fileSize);
    file.read(reinterpret_cast<char*>(data.data()), fileSize);
    
    // Validate WAV header
    if (fileSize < 12 || 
        data[0] != 'R' || data[1] != 'I' || data[2] != 'F' || data[3] != 'F' ||
        data[8] != 'W' || data[9] != 'A' || data[10] != 'V' || data[11] != 'E') {
        std::cerr << "Error: Not a valid WAV file" << std::endl;
        return false;
    }
    
    std::cout << "Valid WAV header detected" << std::endl;
    return true;
}

// A function to try playing a WAV file with a specific audio driver and device
void testDeviceWithDriver(const char* driverName, int deviceIndex, const char* deviceName, const std::vector<Uint8>& wavData) {
    std::cout << "\n================================================" << std::endl;
    std::cout << "TESTING: Driver [" << driverName << "] with Device [" << deviceName << "]" << std::endl;
    std::cout << "================================================" << std::endl;
    
    // Initialize SDL Audio with the specific driver
    if (SDL_AudioInit(driverName) < 0) {
        std::cerr << "Failed to initialize audio driver " << driverName << ": " << SDL_GetError() << std::endl;
        return;
    }
    
    const char* currentDriver = SDL_GetCurrentAudioDriver();
    std::cout << "Current audio driver: " << (currentDriver ? currentDriver : "none") << std::endl;
    
    // Check if the requested device is still available after driver init
    int numDevices = SDL_GetNumAudioDevices(0);
    if (deviceIndex >= numDevices) {
        std::cerr << "Device index out of range after driver initialization" << std::endl;
        SDL_AudioQuit();
        return;
    }
    
    // Verify device name
    const char* actualDeviceName = SDL_GetAudioDeviceName(deviceIndex, 0);
    if (strcmp(deviceName, actualDeviceName) != 0) {
        std::cerr << "Device name mismatch. Expected: " << deviceName 
                  << ", Actual: " << actualDeviceName << std::endl;
    }
    
    // Try different buffer sizes
    for (int bufferSize : {8192, 4096, 2048, 1024, 512}) {
        std::cout << "\nTrying buffer size: " << bufferSize << std::endl;
        
        // Close audio if it was previously opened
        Mix_CloseAudio();
        
        // Initialize SDL_mixer with specific device
        if (Mix_OpenAudioDevice(44100, MIX_DEFAULT_FORMAT, 2, bufferSize, actualDeviceName, 0) < 0) {
            std::cerr << "Failed to open audio device with buffer size " << bufferSize 
                      << ": " << Mix_GetError() << std::endl;
            continue;
        }
        
        std::cout << "SDL_mixer initialized successfully with buffer size " << bufferSize << std::endl;
        
        // Print obtained audio specs
        int freq, channels;
        Uint16 format;
        if (Mix_QuerySpec(&freq, &format, &channels) == 0) {
            std::cerr << "Failed to query SDL_mixer specs: " << Mix_GetError() << std::endl;
        } else {
            std::cout << "SDL_mixer specs - frequency: " << freq 
                      << ", format: " << format << ", channels: " << channels << std::endl;
        }
        
        // Allocate channels for mixing
        Mix_AllocateChannels(16);
        
        // Set volume to maximum
        Mix_Volume(-1, MIX_MAX_VOLUME);
        
        // Create an SDL_RWops from memory
        SDL_RWops* rw = SDL_RWFromConstMem(wavData.data(), wavData.size());
        if (!rw) {
            std::cerr << "Failed to create RWops: " << SDL_GetError() << std::endl;
            Mix_CloseAudio();
            continue;
        }
        
        // Load WAV using SDL_mixer
        Mix_Chunk* chunk = Mix_LoadWAV_RW(rw, 1);  // 1 means SDL_RWops will be auto-freed
        if (!chunk) {
            std::cerr << "Failed to load WAV: " << Mix_GetError() << std::endl;
            Mix_CloseAudio();
            continue;
        }
        
        std::cout << "WAV loaded successfully" << std::endl;
        
        // Play the sound effect
        int channel = Mix_PlayChannel(-1, chunk, 0);  // Play once
        if (channel == -1) {
            std::cerr << "Failed to play WAV: " << Mix_GetError() << std::endl;
            Mix_FreeChunk(chunk);
            Mix_CloseAudio();
            continue;
        }
        
        std::cout << "Playing WAV on channel " << channel << std::endl;
        
        // Wait for a while to let the sound play
        std::cout << "Waiting for playback (3 seconds)..." << std::endl;
        std::this_thread::sleep_for(std::chrono::seconds(3));
        
        // Check if playback is still ongoing
        if (Mix_Playing(channel)) {
            std::cout << "Sound is still playing after 3 seconds" << std::endl;
        } else {
            std::cout << "Sound playback completed or failed" << std::endl;
        }
        
        // Clean up
        Mix_HaltChannel(channel);
        Mix_FreeChunk(chunk);
        Mix_CloseAudio();
    }
    
    SDL_AudioQuit();
}

int main(int argc, char* argv[]) {
    // Set up logging
    setupLogging();
    
    // Default WAV file path
    std::string wavFile = "test.wav";
    
    // If command line argument is provided, use that as the WAV file path
    if (argc > 1) {
        wavFile = argv[1];
    }
    
    std::cout << "Testing WAV file: " << wavFile << std::endl;
    
    // Initialize SDL
    if (SDL_Init(SDL_INIT_AUDIO) < 0) {
        std::cerr << "SDL initialization failed: " << SDL_GetError() << std::endl;
        return 1;
    }
    
    // Load WAV file
    std::vector<Uint8> wavData;
    if (!loadWavFile(wavFile, wavData)) {
        std::cerr << "Failed to load WAV file" << std::endl;
        SDL_Quit();
        return 1;
    }
    
    // Print all available audio drivers
    std::cout << "Available audio drivers:" << std::endl;
    int numDrivers = SDL_GetNumAudioDrivers();
    for (int i = 0; i < numDrivers; i++) {
        std::cout << " - " << SDL_GetAudioDriver(i) << std::endl;
    }
    
    // Drivers to test
    const char* drivers[] = {"wasapi", "directsound", "winmm"};
    
    // Test each driver with each device
    for (const char* driver : drivers) {
        // Quit any previous audio subsystem
        SDL_AudioQuit();
        
        // Initialize with current driver
        if (SDL_AudioInit(driver) < 0) {
            std::cerr << "Failed to initialize driver " << driver << ": " << SDL_GetError() << std::endl;
            continue;
        }
        
        std::cout << "\nDriver: " << driver << std::endl;
        
        // Get all audio devices for this driver
        int numDevices = SDL_GetNumAudioDevices(0);  // 0 for output devices
        std::cout << "Available audio devices for " << driver << ":" << std::endl;
        
        // Store device names
        std::vector<std::string> deviceNames;
        for (int i = 0; i < numDevices; i++) {
            const char* deviceName = SDL_GetAudioDeviceName(i, 0);
            std::cout << " - [" << i << "] " << deviceName << std::endl;
            deviceNames.push_back(deviceName);
        }
        
        // Test each device with current driver
        for (int i = 0; i < numDevices; i++) {
            testDeviceWithDriver(driver, i, deviceNames[i].c_str(), wavData);
        }
    }
    
    SDL_Quit();
    std::cout << "=== SDL Audio Device Test Ended ===" << std::endl;
    
    return 0;
}
