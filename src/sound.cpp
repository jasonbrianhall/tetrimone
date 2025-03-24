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
    sounds_zip_path_ =
        "sound.zip"; // Default location, can be changed via settings
  }

  // Try to initialize the audio system
  if (AudioManager::getInstance().initialize()) {
    // Attempt to load the theme music
    if (loadSoundFromZip(GameSoundEvent::BackgroundMusic, "theme.wav") &&
        loadSoundFromZip(GameSoundEvent::Gameover, "gameover.wav") &&
        loadSoundFromZip(GameSoundEvent::Clear, "clear.wav") &&
        loadSoundFromZip(GameSoundEvent::Drop, "drop.wav") &&
        loadSoundFromZip(GameSoundEvent::LateralMove, "lateralmove.wav") &&
        loadSoundFromZip(GameSoundEvent::LevelUp, "levelup.wav") &&
        loadSoundFromZip(GameSoundEvent::Rotate, "rotate.wav") &&
        loadSoundFromZip(GameSoundEvent::Select, "select.wav") &&
        loadSoundFromZip(GameSoundEvent::Start, "start.wav") &&
        loadSoundFromZip(GameSoundEvent::Tetris, "tetris.wav") &&
        loadSoundFromZip(GameSoundEvent::Excellent, "excellent.wav")) {
      return true;
    } else {
      std::cerr << "Failed to load background music. Sound will be disabled."
                << std::endl;
      AudioManager::getInstance().shutdown();
      sound_enabled_ = false;
      return false;
    }
  } else {
    std::cerr << "Failed to initialize audio system. Sound will be disabled."
              << std::endl;
    sound_enabled_ = false;
    return false;
  }
}

void TetrisBoard::dismissSplashScreen() { 
    splashScreenActive = false; 
    playSound(GameSoundEvent::Start);
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
    audioEvent = SoundEvent::BackgroundMusic;
    break;
  case GameSoundEvent::Gameover:
    audioEvent = SoundEvent::Gameover;
    break;
  case GameSoundEvent::Clear:
    audioEvent = SoundEvent::Clear;
    break;
  case GameSoundEvent::Drop:
    audioEvent = SoundEvent::Drop;
    break;
  case GameSoundEvent::LateralMove:
    audioEvent = SoundEvent::LateralMove;
    break;
  case GameSoundEvent::LevelUp:
    audioEvent = SoundEvent::LevelUp;
    break;
  case GameSoundEvent::Rotate:
    audioEvent = SoundEvent::Rotate;
    break;
  case GameSoundEvent::Select:
    audioEvent = SoundEvent::Select;
    break;
  case GameSoundEvent::Start:
    audioEvent = SoundEvent::Start;
    break;
  case GameSoundEvent::Tetris:
    audioEvent = SoundEvent::Tetris;
    break;
  case GameSoundEvent::Excellent:
    audioEvent = SoundEvent::Excellent;
    break;
  default:
    std::cerr << "Unknown sound event" << std::endl;
    return false;
  }

  // Load the sound data into the audio manager
  return AudioManager::getInstance().loadSoundFromMemory(audioEvent, soundData,
                                                         format);
}

void TetrisBoard::playBackgroundMusic() {
  if (!sound_enabled_) {
    return;
  }

#ifdef _WIN32
  // For Windows, the AudioManager handles the threading internally
  // so we can just play the sound once and it will handle looping
  if (!musicPaused) {
    AudioManager::getInstance().setMuted(false);
    AudioManager::getInstance().playSound(SoundEvent::BackgroundMusic);
  }
#else
  // PulseAudio implementation - Create a background thread that loops the music
  static bool musicThreadRunning = false;
  static std::atomic<bool> stopFlag(false);

  // Kill existing thread if it's running but shouldn't be
  if (musicThreadRunning && (musicPaused || !sound_enabled_)) {
    stopFlag = true;
    // Give a moment for the thread to exit
    std::this_thread::sleep_for(std::chrono::milliseconds(200));
    musicThreadRunning = false;
  }

  // Only start the thread if music should be playing and thread isn't already running
  if (!musicThreadRunning && sound_enabled_ && !musicPaused) {
    musicThreadRunning = true;
    stopFlag = false;

    std::thread([this, &stopFlag]() {
      while (sound_enabled_ && !stopFlag && !musicPaused) {
        // Map GameSoundEvent to AudioManager's SoundEvent
        SoundEvent audioEvent = SoundEvent::BackgroundMusic;

        bool isMuted = AudioManager::getInstance().isMuted();

        if (!isMuted) {
          // Play the sound and wait for it to complete
          AudioManager::getInstance().playSoundAndWait(audioEvent);
        }

        // Short delay between loops
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
      }

      musicThreadRunning = false;
    }).detach();
  }
#endif
}

void TetrisBoard::pauseBackgroundMusic() {
  // If sound is being turned off (not just paused), we need to stop immediately
  if (!sound_enabled_) {
    // This will kill the thread at the next check, but we also need to
    // tell AudioManager to stop any currently playing sound
    AudioManager::getInstance().setMuted(true);
    return;
  }

  // For regular pause during gameplay, just pause
  musicPaused = true;
  AudioManager::getInstance().setMuted(true);
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
  case GameSoundEvent::Gameover:
    audioEvent = SoundEvent::Gameover;
    break;
  case GameSoundEvent::Clear:
    audioEvent = SoundEvent::Clear;
    break;
  case GameSoundEvent::Drop:
    audioEvent = SoundEvent::Drop;
    break;
  case GameSoundEvent::LateralMove:
    audioEvent = SoundEvent::LateralMove;
    break;
  case GameSoundEvent::LevelUp:
    audioEvent = SoundEvent::LevelUp;
    break;
  case GameSoundEvent::Rotate:
    audioEvent = SoundEvent::Rotate;
    break;
  case GameSoundEvent::Select:
    audioEvent = SoundEvent::Select;
    break;
  case GameSoundEvent::Start:
    audioEvent = SoundEvent::Start;
    break;
  case GameSoundEvent::Tetris:
    audioEvent = SoundEvent::Tetris;
    break;
  case GameSoundEvent::Excellent:
    audioEvent = SoundEvent::Excellent;
    break;
  default:
    std::cerr << "Unknown sound event" << std::endl;
    return;
  }
  // Play the sound asynchronously
  AudioManager::getInstance().playSound(audioEvent);
}

void TetrisBoard::cleanupAudio() {
  // First check if we need to do anything
  if (!sound_enabled_) {
    return;
  }
    
  // Set sound_enabled_ to false first to prevent any new sounds from playing
  sound_enabled_ = false;
    
#ifdef _WIN32
  // For Windows, just tell the AudioManager to stop immediately
  try {
    AudioManager::getInstance().setMuted(true);
    
    // Give time for sound effects to stop
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    
    // Shutdown the audio manager
    AudioManager::getInstance().shutdown();
  }
  catch (const std::exception& e) {
    std::cerr << "Exception during audio cleanup: " << e.what() << std::endl;
  }
  catch (...) {
    std::cerr << "Unknown exception during audio cleanup" << std::endl;
  }
#else
  // For PulseAudio, we need to handle the background music thread
  
  // Stop the background music thread by setting musicPaused
  musicPaused = true;
    
  // Give the background music thread time to exit
  std::this_thread::sleep_for(std::chrono::milliseconds(200));
    
  try {
    // Tell audio manager to stop all sounds
    AudioManager::getInstance().setMuted(true);
        
    // Give time for sound effects to stop
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
        
    // Shutdown the audio manager
    AudioManager::getInstance().shutdown();
  }
  catch (const std::exception& e) {
    std::cerr << "Exception during audio cleanup: " << e.what() << std::endl;
    // Continue with cleanup despite errors
  }
  catch (...) {
    std::cerr << "Unknown exception during audio cleanup" << std::endl;
    // Continue with cleanup despite errors
  }
#endif
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
  if (!sound_enabled_) {
    // Enable sound if it was disabled
    sound_enabled_ = true;

    // Make sure audio is initialized if we're re-enabling sound
    if (!AudioManager::getInstance().isAvailable()) {
      if (!initializeAudio()) {
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
}
