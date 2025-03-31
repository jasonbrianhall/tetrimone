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
#include <random>
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
    if (
    
#ifdef _WIN32
    loadSoundFromZip(GameSoundEvent::BackgroundMusic, "themeall.mp3") &&
#else    
    loadSoundFromZip(GameSoundEvent::BackgroundMusic, "theme.mp3") &&
#endif
        loadSoundFromZip(GameSoundEvent::BackgroundMusic2, "TetrisA.mp3") &&
        loadSoundFromZip(GameSoundEvent::BackgroundMusic3, "TetrisB.mp3") &&
        loadSoundFromZip(GameSoundEvent::BackgroundMusic4, "TetrisC.mp3") &&
        loadSoundFromZip(GameSoundEvent::BackgroundMusic5, "futuristic.mp3") &&
        loadSoundFromZip(GameSoundEvent::Single, "single.mp3") &&
        loadSoundFromZip(GameSoundEvent::Double, "double.mp3") &&
        loadSoundFromZip(GameSoundEvent::Triple, "triple.mp3") &&
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

    // Track the length of the audio file
    size_t audioLength = soundData.size();

    // Check if it's an MP3 and convert to WAV
    if (format == "mp3") {
        std::vector<uint8_t> wavData;
        if (convertMp3ToWavInMemory(soundData, wavData)) {
            // Replace the original data with the converted WAV data
            soundData = std::move(wavData);
            format = "wav";
            
            // Update audio length after conversion
            audioLength = soundData.size();
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
    case GameSoundEvent::Single:
        audioEvent = SoundEvent::Single;
        break;
    case GameSoundEvent::Double:
        audioEvent = SoundEvent::Double;
        break;
    case GameSoundEvent::Triple:
        audioEvent = SoundEvent::Triple;
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

    // Load the sound data into the audio manager with its length
    AudioManager &audioManager = AudioManager::getInstance();
    if (audioManager.loadSoundFromMemory(audioEvent, soundData, format, audioLength)) {
        return true;
    }

    return false;
}

void TetrisBoard::playBackgroundMusic() {
  if (!sound_enabled_) {
    return;
  }

#ifdef _WIN32
  // Windows implementation - Play the combined themeall.mp3 file on a loop
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
      AudioManager& audioManager = AudioManager::getInstance();

      while (sound_enabled_ && !stopFlag && !musicPaused) {
        // Check if not muted
        bool isMuted = audioManager.isMuted();

        if (!isMuted) {
          // Get the sound data and format for the combined theme file
          std::vector<uint8_t> soundData;
          std::string format;
          if (audioManager.getSoundData(SoundEvent::BackgroundMusic, soundData, format)) {
            // Get the track length
            size_t trackLength = audioManager.getSoundLength(SoundEvent::BackgroundMusic);

            // Play the combined theme file in a loop
            audioManager.playBackgroundMusicLooped(soundData, format);

            // Wait for the track length (or a reasonable default if length is 0)
            if (trackLength > 0) {
              std::this_thread::sleep_for(std::chrono::milliseconds(trackLength));
            } else {
              // Default to 3 minutes if length is unknown
              std::this_thread::sleep_for(std::chrono::minutes(3));
            }
          } else {
            // If sound data retrieval fails, wait a bit before trying again
            std::this_thread::sleep_for(std::chrono::seconds(1));
          }
        } else {
          // If muted, just wait a bit
          std::this_thread::sleep_for(std::chrono::seconds(1));
        }
      }

      musicThreadRunning = false;
    }).detach();
  }
#else
  // Existing PulseAudio implementation remains the same
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
      // Background music tracks to cycle through
      const std::vector<SoundEvent> backgroundMusicTracks = {
        SoundEvent::BackgroundMusic,
        SoundEvent::BackgroundMusic2,
        SoundEvent::BackgroundMusic3,
        SoundEvent::BackgroundMusic4,
        SoundEvent::BackgroundMusic5
      };

      AudioManager& audioManager = AudioManager::getInstance();
      size_t currentTrackIndex = 0;

      while (sound_enabled_ && !stopFlag && !musicPaused) {
        // Get the current background music track
        SoundEvent audioEvent = backgroundMusicTracks[currentTrackIndex];

        bool isMuted = audioManager.isMuted();

        if (!isMuted) {
          // Play the sound and wait for it to complete
          audioManager.playSoundAndWait(audioEvent);

          // Move to next track, wrapping around if at the end
          currentTrackIndex = (currentTrackIndex + 1) % backgroundMusicTracks.size();
        }

        // Short delay between loops to prevent tight looping
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
  case GameSoundEvent::Single:
    audioEvent = SoundEvent::Single;
    break;
  case GameSoundEvent::Double:
    audioEvent = SoundEvent::Double;
    break;
  case GameSoundEvent::Triple:
    audioEvent = SoundEvent::Triple;
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
