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
#include "audioconverter.h"

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
    if (loadSoundFromZip(GameSoundEvent::BackgroundMusic, "theme.mp3") &&
        loadSoundFromZip(GameSoundEvent::BackgroundMusic2, "TetrisA.mp3") &&
        loadSoundFromZip(GameSoundEvent::BackgroundMusic3, "TetrisB.mp3") &&
        loadSoundFromZip(GameSoundEvent::BackgroundMusic4, "TetrisC.mp3") &&
        loadSoundFromZip(GameSoundEvent::BackgroundMusic5, "futuristic.mp3") &&
        loadSoundFromZip(GameSoundEvent::Gameover, "gameover.mp3") &&
        loadSoundFromZip(GameSoundEvent::Clear, "clear.mp3") &&
        loadSoundFromZip(GameSoundEvent::Drop, "drop.mp3") &&
        loadSoundFromZip(GameSoundEvent::LateralMove, "lateralmove.mp3") &&
        loadSoundFromZip(GameSoundEvent::LevelUp, "levelup.mp3") &&
        loadSoundFromZip(GameSoundEvent::Rotate, "rotate.mp3") &&
        loadSoundFromZip(GameSoundEvent::Select, "select.mp3") &&
        loadSoundFromZip(GameSoundEvent::Start, "start.mp3") &&
        loadSoundFromZip(GameSoundEvent::Tetris, "tetris.mp3") &&
        loadSoundFromZip(GameSoundEvent::Excellent, "excellent.mp3")) {
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
                                   const std::string& soundFileName) {
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

    // Check if it's an MP3 and convert to WAV
    if (format == "mp3") {
        std::vector<uint8_t> wavData;
        if (convertMp3ToWavInMemory(soundData, wavData)) {
            // Replace the original data with the converted WAV data
            soundData = std::move(wavData);
            format = "wav";
        } else {
            std::cerr << "Failed to convert MP3 to WAV: " << soundFileName << std::endl;
            return false;
        }
    }

    // Map GameSoundEvent to AudioManager's SoundEvent
    SoundEvent audioEvent;
    switch (event) {
    case GameSoundEvent::BackgroundMusic:
        audioEvent = SoundEvent::BackgroundMusic;
        break;
    case GameSoundEvent::BackgroundMusic2:
        audioEvent = SoundEvent::BackgroundMusic2;
        break;
    case GameSoundEvent::BackgroundMusic3:
        audioEvent = SoundEvent::BackgroundMusic3;
        break;
    case GameSoundEvent::BackgroundMusic4:
        audioEvent = SoundEvent::BackgroundMusic4;
        break;
    case GameSoundEvent::BackgroundMusic5:
        audioEvent = SoundEvent::BackgroundMusic5;
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
    return AudioManager::getInstance().loadSoundFromMemory(audioEvent, soundData, format);
}

void TetrisBoard::playBackgroundMusic() {
  std::cout << "playBackgroundMusic called - sound_enabled_: " << (sound_enabled_ ? "true" : "false") 
            << ", musicPaused: " << (musicPaused ? "true" : "false") << std::endl;
  
  if (!sound_enabled_) {
    std::cout << "Sound not enabled, returning early" << std::endl;
    return;
  }

#ifdef _WIN32
  // Windows implementation stays the same
#else
  // PulseAudio implementation with better error handling
  static bool musicThreadRunning = false;
  static std::atomic<bool> stopFlag(false);

  std::cout << "PulseAudio: Current thread state - Running: " << (musicThreadRunning ? "yes" : "no") 
            << ", StopFlag: " << (stopFlag ? "true" : "false") << std::endl;

  // Kill existing thread if it's running but shouldn't be
  if (musicThreadRunning && (musicPaused || !sound_enabled_)) {
    std::cout << "PulseAudio: Setting stop flag to kill music thread" << std::endl;
    stopFlag = true;
    // Give a moment for the thread to exit
    std::this_thread::sleep_for(std::chrono::milliseconds(200));
    musicThreadRunning = false;
    std::cout << "PulseAudio: Marked thread as not running" << std::endl;
  }

  // Only start the thread if music should be playing and thread isn't already running
  if (!musicThreadRunning && sound_enabled_ && !musicPaused) {
    std::cout << "PulseAudio: Starting music thread" << std::endl;
    musicThreadRunning = true;
    stopFlag = false;

    std::thread([this, &stopFlag]() {
      std::cout << "PulseAudio: Music thread started" << std::endl;
      
      // Track index to cycle through all 5 tracks
      int trackIndex = 0;
      
      try {
        while (sound_enabled_ && !stopFlag && !musicPaused) {
          // Map track index to AudioManager's SoundEvent
          SoundEvent audioEvent;
          switch (trackIndex) {
            case 0: 
              audioEvent = SoundEvent::BackgroundMusic;
              std::cout << "PulseAudio: Playing track 1 (BackgroundMusic)" << std::endl;
              break;
            case 1: 
              audioEvent = SoundEvent::BackgroundMusic2;
              std::cout << "PulseAudio: Playing track 2 (BackgroundMusic2)" << std::endl;
              break;
            case 2: 
              audioEvent = SoundEvent::BackgroundMusic3;
              std::cout << "PulseAudio: Playing track 3 (BackgroundMusic3)" << std::endl;
              break;
            case 3: 
              audioEvent = SoundEvent::BackgroundMusic4;
              std::cout << "PulseAudio: Playing track 4 (BackgroundMusic4)" << std::endl;
              break;
            case 4: 
              audioEvent = SoundEvent::BackgroundMusic5;
              std::cout << "PulseAudio: Playing track 5 (BackgroundMusic5)" << std::endl;
              break;
            default: 
              audioEvent = SoundEvent::BackgroundMusic;
              trackIndex = 0;
              std::cout << "PulseAudio: Resetting to track 1 (BackgroundMusic)" << std::endl;
              break;
          }

          bool isMuted = AudioManager::getInstance().isMuted();
          std::cout << "PulseAudio: In music thread loop - Muted: " << (isMuted ? "yes" : "no") << std::endl;

          if (!isMuted) {
            // Print log before playing
            std::cout << "PulseAudio: About to call playSoundAndWait for track index: " << trackIndex << std::endl;
            
            // Create a timeout mechanism to detect if playSoundAndWait is hanging
            bool soundCompleted = false;
            std::thread timeoutThread([&]() {
              // Wait for a reasonable amount of time (adjust as needed)
              std::this_thread::sleep_for(std::chrono::minutes(5));
              if (!soundCompleted && !stopFlag) {
                std::cout << "PulseAudio: TIMEOUT - playSoundAndWait taking too long, may be stuck" << std::endl;
              }
            });
            timeoutThread.detach();
            
            // Play the sound and wait for completion
            AudioManager::getInstance().playSoundAndWait(audioEvent);
            
            // Mark that the sound completed
            soundCompleted = true;
            
            std::cout << "PulseAudio: playSoundAndWait completed for track index: " << trackIndex << std::endl;
            
            // Advance to next track
            trackIndex = (trackIndex + 1) % 5;
            std::cout << "PulseAudio: Advanced to next track index: " << trackIndex << std::endl;
          }

          // Check if music thread should continue
          if (!sound_enabled_ || stopFlag || musicPaused) {
            std::cout << "PulseAudio: Breaking out of music thread loop due to state change" << std::endl;
            break;
          }

          // Short delay between tracks
          std::cout << "PulseAudio: Sleeping 100ms before next track" << std::endl;
          std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }
      } catch (const std::exception& e) {
        std::cout << "PulseAudio: EXCEPTION in music thread: " << e.what() << std::endl;
      } catch (...) {
        std::cout << "PulseAudio: UNKNOWN EXCEPTION in music thread" << std::endl;
      }

      std::cout << "PulseAudio: Music thread exiting - sound_enabled_: " << (sound_enabled_ ? "true" : "false") 
                << ", stopFlag: " << (stopFlag ? "true" : "false")
                << ", musicPaused: " << (musicPaused ? "true" : "false") << std::endl;
      musicThreadRunning = false;
    }).detach();
  } else {
    std::cout << "PulseAudio: Not starting music thread - Already running: " << (musicThreadRunning ? "yes" : "no")
              << ", Sound enabled: " << (sound_enabled_ ? "yes" : "no")
              << ", Music paused: " << (musicPaused ? "yes" : "no") << std::endl;
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
  case GameSoundEvent::BackgroundMusic2:
    audioEvent = SoundEvent::BackgroundMusic2;
    break;
  case GameSoundEvent::BackgroundMusic3:
    audioEvent = SoundEvent::BackgroundMusic3;
    break;
  case GameSoundEvent::BackgroundMusic4:
    audioEvent = SoundEvent::BackgroundMusic4;
    break;
  case GameSoundEvent::BackgroundMusic5:
    audioEvent = SoundEvent::BackgroundMusic5;
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
    // Restore volume to full
    AudioManager::getInstance().restoreVolume();
    
    // Unpause playback
    musicPaused = false;

    // Make sure music is actually playing
    playBackgroundMusic();
}
