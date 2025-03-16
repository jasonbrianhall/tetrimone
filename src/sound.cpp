#include "audiomanager.h"
#include "tetris.h"
#include <algorithm>
#include <cctype> // Added for std::tolower
#include <chrono> // Add this for std::chrono and std::this_thread::sleep_for
#include <fstream>
#include <iostream>
#include <memory>
#include <string>
#include <sys/stat.h>
#include <thread>
#include <vector>
#include <zip.h>
#ifdef _WIN32
#include <direct.h>
#endif

// Function to extract a file from a ZIP archive into memory
bool TetrisBoard::extractFileFromZip(const std::string &zipFilePath,
                                       const std::string &fileName,
                                       std::vector<uint8_t> &fileData) {
  int errCode = 0;
  zip_t *archive = zip_open(zipFilePath.c_str(), 0, &errCode);

  if (!archive) {
    zip_error_t zipError;
    zip_error_init_with_code(&zipError, errCode);
    std::cerr << "Failed to open ZIP archive: " << zip_error_strerror(&zipError)
              << std::endl;
    zip_error_fini(&zipError);
    return false;
  }

  // Find the file in the archive
  zip_int64_t index = zip_name_locate(archive, fileName.c_str(), 0);
  if (index < 0) {
    std::cerr << "File not found in ZIP archive: " << fileName << std::endl;
    zip_close(archive);
    return false;
  }

  // Open the file in the archive
  zip_file_t *file = zip_fopen_index(archive, index, 0);
  if (!file) {
    std::cerr << "Failed to open file in ZIP archive: " << zip_strerror(archive)
              << std::endl;
    zip_close(archive);
    return false;
  }

  // Get file size
  zip_stat_t stat;
  if (zip_stat_index(archive, index, 0, &stat) < 0) {
    std::cerr << "Failed to get file stats: " << zip_strerror(archive)
              << std::endl;
    zip_fclose(file);
    zip_close(archive);
    return false;
  }

  // Resize the buffer to fit the file
  fileData.resize(stat.size);

  // Read the file content
  zip_int64_t bytesRead = zip_fread(file, fileData.data(), stat.size);
  if (bytesRead < 0 || static_cast<zip_uint64_t>(bytesRead) != stat.size) {
    std::cerr << "Failed to read file: " << zip_file_strerror(file)
              << std::endl;
    zip_fclose(file);
    zip_close(archive);
    return false;
  }

  // Close everything
  zip_fclose(file);
  zip_close(archive);
#ifdef DEBUG
  std::cout << "Successfully extracted file (" << fileData.size() << " bytes)"
            << std::endl;
#endif
  return true;
}

// Custom Audio Manager function to load sound from memory
bool loadSoundFromMemory(SoundEvent event, const std::vector<uint8_t> &data,
                         const std::string &format) {
  // Get file extension to determine format
  if (format != "wav" && format != "mp3") {
    std::cerr << "Unsupported audio format: " << format << std::endl;
    return false;
  }

  // Store the sound data
  return AudioManager::getInstance().loadSoundFromMemory(event, data, format);
}

bool TetrisBoard::initializeAudio() {
  // Don't do anything if sound is disabled
  if (!sound_enabled_) {
    return false;
  }

  if (sounds_zip_path_.empty()) {
    sounds_zip_path_ = "sound.zip"; // Default location, can be changed via settings
  }

  // Try to initialize the audio system
  if (AudioManager::getInstance().initialize()) {
    // Attempt to load the theme music
    if (loadSoundFromZip(GameSoundEvent::BackgroundMusic, "theme.wav")) {
      std::cout << "Sound system initialized successfully." << std::endl;
      return true;
    } else {
      std::cerr << "Failed to load background music. Sound will be disabled." << std::endl;
      AudioManager::getInstance().shutdown();
      sound_enabled_ = false;
      return false;
    }
  } else {
    std::cerr << "Failed to initialize audio system. Sound will be disabled." << std::endl;
    sound_enabled_ = false;
    return false;
  }
}

bool TetrisBoard::loadSoundFromZip(GameSoundEvent event,
                                     const std::string &soundFileName) {
  // Extract the sound file from the ZIP archive
  std::vector<uint8_t> soundData;
  if (!extractFileFromZip(sounds_zip_path_, soundFileName, soundData)) {
    std::cerr << "Failed to extract sound file from ZIP archive: "
              << soundFileName << std::endl;
    return false;
  }

  // Get file extension to determine format
  std::string format;
  size_t dotPos = soundFileName.find_last_of('.');
  if (dotPos != std::string::npos) {
    format = soundFileName.substr(dotPos + 1);
    // Convert to lowercase
    std::transform(format.begin(), format.end(), format.begin(),
                   [](unsigned char c) { return std::tolower(c); });
  } else {
    std::cerr << "Sound file has no extension: " << soundFileName << std::endl;
    return false;
  }

  // Map GameSoundEvent to AudioManager's SoundEvent
  SoundEvent audioEvent;
  switch (event) {
    case GameSoundEvent::BackgroundMusic:
        audioEvent = SoundEvent::BackgroundMusic;  // Map to the background music event
        break;
   default:
    std::cerr << "Unknown sound event" << std::endl;
    return false;
  }

  // Load the sound data into the audio manager
  return AudioManager::getInstance().loadSoundFromMemory(audioEvent, soundData,
                                                         format);
}

void TetrisBoard::pauseBackgroundMusic() {
    std::cout << "DEBUG: TetrisBoard::pauseBackgroundMusic() called" << std::endl;
    
    // If sound is being turned off (not just paused), we need to stop immediately
    if (!sound_enabled_) {
        std::cout << "DEBUG: Sound is being completely disabled" << std::endl;
        // This will kill the thread at the next check, but we also need to
        // tell AudioManager to stop any currently playing sound
        AudioManager::getInstance().setMuted(true);
        return;
    }
    
    // For regular pause during gameplay, just pause
    std::cout << "DEBUG: Setting musicPaused=true and calling setMuted(true)" << std::endl;
    musicPaused = true;
    AudioManager::getInstance().setMuted(true);
}

void TetrisBoard::playBackgroundMusic() {
    std::cout << "DEBUG: TetrisBoard::playBackgroundMusic() called" << std::endl;
    
    if (!sound_enabled_) {
        std::cout << "DEBUG: Sound not enabled, returning" << std::endl;
        return;
    }
    
    // Create a single background thread that loops the music
    static bool musicThreadRunning = false;
    static std::atomic<bool> stopFlag(false);
    
    std::cout << "DEBUG: musicThreadRunning=" << (musicThreadRunning ? "true" : "false")
              << ", musicPaused=" << (musicPaused ? "true" : "false") << std::endl;
    
    // Kill existing thread if it's running but shouldn't be
    if (musicThreadRunning && (musicPaused || !sound_enabled_)) {
        std::cout << "DEBUG: Stopping existing music thread" << std::endl;
        stopFlag = true;
        // Give a moment for the thread to exit
        std::this_thread::sleep_for(std::chrono::milliseconds(200));
        musicThreadRunning = false;
    }
    
    // Only start the thread if music should be playing and thread isn't already running
    if (!musicThreadRunning && sound_enabled_ && !musicPaused) {
        std::cout << "DEBUG: Starting music thread" << std::endl;
        musicThreadRunning = true;
        stopFlag = false;
        
        std::thread([this, &stopFlag]() {
            std::cout << "DEBUG: Music thread started" << std::endl;
            while (sound_enabled_ && !stopFlag && !musicPaused) {
                std::cout << "DEBUG: About to play background music" << std::endl;
                // Map GameSoundEvent to AudioManager's SoundEvent
                SoundEvent audioEvent = SoundEvent::BackgroundMusic;
                
                bool isMuted = AudioManager::getInstance().isMuted();
                std::cout << "DEBUG: AudioManager muted state: " << (isMuted ? "true" : "false") << std::endl;
                
                if (!isMuted) {
                    std::cout << "DEBUG: Calling playSoundAndWait()" << std::endl;
                    // Play the sound and wait for it to complete
                    AudioManager::getInstance().playSoundAndWait(audioEvent);
                    std::cout << "DEBUG: playSoundAndWait() returned" << std::endl;
                } else {
                    std::cout << "DEBUG: AudioManager is muted, not playing" << std::endl;
                }
                
                // Short delay between loops
                std::cout << "DEBUG: Waiting before next loop iteration" << std::endl;
                std::this_thread::sleep_for(std::chrono::milliseconds(100));
                
                std::cout << "DEBUG: End of loop iteration - sound_enabled_=" << (sound_enabled_ ? "true" : "false") 
                          << ", stopFlag=" << (stopFlag ? "true" : "false")
                          << ", musicPaused=" << (musicPaused ? "true" : "false") << std::endl;
            }
            
            std::cout << "DEBUG: Music thread exiting" << std::endl;
            musicThreadRunning = false;
        }).detach();
    } else {
        std::cout << "DEBUG: Music thread already running or shouldn't start" << std::endl;
    }
}

void TetrisBoard::playSound(GameSoundEvent event) {
  if (!sound_enabled_) {
    return;
  }

  // Map GameSoundEvent to AudioManager's SoundEvent
  SoundEvent audioEvent;
  switch (event) {
    case GameSoundEvent::BackgroundMusic:
      audioEvent = SoundEvent::BackgroundMusic;
      break;
    default:
      return;
  }

  // Play the sound asynchronously
  AudioManager::getInstance().playSound(audioEvent);
}

void TetrisBoard::cleanupAudio() {
    std::cout << "DEBUG: TetrisBoard::cleanupAudio() called" << std::endl;
    
    if (sound_enabled_) {
        std::cout << "DEBUG: Shutting down audio" << std::endl;
        AudioManager::getInstance().shutdown();
        sound_enabled_ = false;
    } else {
        std::cout << "DEBUG: Audio already disabled" << std::endl;
    }
}

bool TetrisBoard::setSoundsZipPath(const std::string &path) {
  // Save original path in case of failure
  std::string original_path = sounds_zip_path_;

  // Update path
  sounds_zip_path_ = path;

  // If sound is enabled, reload all sounds
  if (sound_enabled_) {
    // First clean up existing audio
    cleanupAudio();

    // Try to reinitialize with new path
    if (!initializeAudio()) {
      // If failed, restore original path and reinitialize with that
      sounds_zip_path_ = original_path;
      initializeAudio();
      return false;
    }
  }
  return true;
}


void TetrisBoard::resumeBackgroundMusic() {
    std::cout << "DEBUG: TetrisBoard::resumeBackgroundMusic() called" << std::endl;
    
    if (!sound_enabled_) {
        std::cout << "DEBUG: Sound not enabled, enabling it now" << std::endl;
        // Enable sound if it was disabled
        sound_enabled_ = true;
        
        // Make sure audio is initialized if we're re-enabling sound
        if (!AudioManager::getInstance().isAvailable()) {
            std::cout << "DEBUG: Audio system not initialized, initializing now" << std::endl;
            if (!initializeAudio()) {
                std::cout << "DEBUG: Failed to initialize audio system" << std::endl;
                return;
            }
        }
    }
    
    // Unmute the AudioManager
    AudioManager::getInstance().setMuted(false);
    // Unpause playback
    musicPaused = false;
    
    // Make sure music is actually playing
    playBackgroundMusic();
    
    std::cout << "DEBUG: Background music resumed" << std::endl;
}
