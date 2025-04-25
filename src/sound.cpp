#include "audiomanager.h"
#include "tetrimone.h"
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
#include <atomic>
#ifdef _WIN32
#include <direct.h>
#endif
#include "audioconverter.h"

// Add at the top of sound.cpp after the includes
#ifdef _WIN32
void log_to_file(const std::string& message) {
#ifdef DEBUG
    static std::mutex log_mutex;
    std::lock_guard<std::mutex> lock(log_mutex);
    
    std::ofstream log_file("tetrimone_audio_debug.log", std::ios_base::app);
    if (log_file.is_open()) {
        auto now = std::chrono::system_clock::now();
        auto now_time_t = std::chrono::system_clock::to_time_t(now);
        
        struct tm timeinfo;
        localtime_s(&timeinfo, &now_time_t);
        
        char timestamp[26];
        strftime(timestamp, sizeof(timestamp), "%Y-%m-%d %H:%M:%S", &timeinfo);
        
        log_file << "[" << timestamp << "] " << message << std::endl;
        log_file.close();
    }
    #endif
}
#else
void log_to_file(const std::string& message) {
    // No-op for non-Windows platforms
}
#endif

// Function to extract a file from a ZIP archive into memory
bool TetrimoneBoard::extractFileFromZip(const std::string &zipFilePath,
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

bool TetrimoneBoard::initializeAudio() {
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
    
        loadSoundFromZip(GameSoundEvent::BackgroundMusic, "theme.mp3") &&
        loadSoundFromZip(GameSoundEvent::BackgroundMusic2, "TetrimoneA.mp3") &&
        loadSoundFromZip(GameSoundEvent::BackgroundMusic3, "TetrimoneB.mp3") &&
        loadSoundFromZip(GameSoundEvent::BackgroundMusic4, "TetrimoneC.mp3") &&
        loadSoundFromZip(GameSoundEvent::BackgroundMusic5, "futuristic.mp3") &&
        loadSoundFromZip(GameSoundEvent::BackgroundMusicRetro, "theme.mid") &&
        loadSoundFromZip(GameSoundEvent::BackgroundMusic2Retro, "TetrimoneA.mid") &&
        loadSoundFromZip(GameSoundEvent::BackgroundMusic3Retro, "TetrimoneB.mid") &&
        loadSoundFromZip(GameSoundEvent::BackgroundMusic4Retro, "TetrimoneC.mid") &&
        loadSoundFromZip(GameSoundEvent::BackgroundMusic5Retro, "futuristic.mid") &&
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
        loadSoundFromZip(GameSoundEvent::Tetrimone, "tetrimone.mp3") &&
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

void TetrimoneBoard::dismissSplashScreen() { 
    splashScreenActive = false; 
    playSound(GameSoundEvent::Start);
}

bool TetrimoneBoard::loadSoundFromZip(GameSoundEvent event,
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

    // Check if it's an MIDI and convert to WAV
if (format == "mid" || format == "midi") {  // Added support for both .mid and .midi extensions
    std::vector<uint8_t> wavData;
    std::cerr << "Converting MIDI to WAV..." << std::endl;
    if (convertMidiToWavInMemory(soundData, wavData)) {
        // Replace the original data with the converted WAV data
        soundData = std::move(wavData);
        format = "wav";
        
        // Update audio length after conversion
        audioLength = soundData.size();
        std::cerr << "MIDI conversion successful, WAV size: " << audioLength << " bytes" << std::endl;
    } else {
        std::cerr << "Failed to convert MIDI to WAV: " << soundFileName << std::endl;
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
    case GameSoundEvent::Tetrimone:
        audioEvent = SoundEvent::Tetrimone;
        break;
    case GameSoundEvent::Excellent:
        audioEvent = SoundEvent::Excellent;
        break;
    case GameSoundEvent::BackgroundMusicRetro:
        audioEvent = SoundEvent::BackgroundMusicRetro;
        break;
    case GameSoundEvent::BackgroundMusic2Retro:
        audioEvent = SoundEvent::BackgroundMusic2Retro;
        break;
    case GameSoundEvent::BackgroundMusic3Retro:
        audioEvent = SoundEvent::BackgroundMusic3Retro;
        break;
    case GameSoundEvent::BackgroundMusic4Retro:
        audioEvent = SoundEvent::BackgroundMusic4Retro;
        break;
    case GameSoundEvent::BackgroundMusic5Retro:
        audioEvent = SoundEvent::BackgroundMusic5Retro;
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

void TetrimoneBoard::playBackgroundMusic() {
  log_to_file("playBackgroundMusic called");
  
  if (!sound_enabled_) {
    return;
  }

  static bool musicThreadRunning = false;
  
  if (musicThreadRunning && (musicPaused || !sound_enabled_)) {
    musicStopFlag = true;
    std::this_thread::sleep_for(std::chrono::milliseconds(200));
    musicThreadRunning = false;
  }

  if (!musicThreadRunning && sound_enabled_ && !musicPaused) {
    musicThreadRunning = true;
    musicStopFlag = false;

    std::thread([this]() {
      const std::vector<SoundEvent> backgroundMusicTracks = {
        SoundEvent::BackgroundMusic,
        SoundEvent::BackgroundMusic2,
        SoundEvent::BackgroundMusic3,
        SoundEvent::BackgroundMusic4,
        SoundEvent::BackgroundMusic5
      };

      const std::vector<SoundEvent> backgroundMusicTracksRetro = {
        SoundEvent::BackgroundMusicRetro,
        SoundEvent::BackgroundMusic2Retro,
        SoundEvent::BackgroundMusic3Retro,
        SoundEvent::BackgroundMusic4Retro,
        SoundEvent::BackgroundMusic5Retro
      };


      AudioManager& audioManager = AudioManager::getInstance();
      size_t currentTrackIndex = 0;

      // Check if we need to skip the current track because it's disabled
      while (!enabledTracks[currentTrackIndex]) {
        // If all tracks are disabled, just use the first one
        bool allDisabled = true;
        for (int i = 0; i < 5; i++) {
          if (enabledTracks[i]) {
            allDisabled = false;
            break;
          }
        }
        
        if (allDisabled) {
          // All tracks disabled, just use track 0
          log_to_file("All tracks disabled, defaulting to track 0");
          currentTrackIndex = 0;
          break;
        } else {
          // Move to next track and check again
          currentTrackIndex = (currentTrackIndex + 1) % backgroundMusicTracks.size();
        }
      }

      #ifdef _WIN32
      log_to_file("Using Windows-specific music playback");
      
      auto calculateWavDuration = [&audioManager](SoundEvent event) -> int {
        std::vector<uint8_t> data;
        std::string format;
        
        // Get the sound data
        if (!audioManager.getSoundData(event, data, format)) {
          log_to_file("Failed to get sound data for track");
          return 120; // Default fallback
        }
        
        // Calculate duration for WAV files
        if (format == "wav" && data.size() >= 44) {
          // Get header data
          uint32_t sampleRate = 0;
          uint16_t numChannels = 0;
          uint16_t bitsPerSample = 0;
          uint32_t dataSize = 0;
          
          // Look for fmt chunk
          for (size_t i = 12; i < data.size() - 8; ) {
            char chunkId[5] = {0};
            memcpy(chunkId, &data[i], 4);
            uint32_t chunkSize = *reinterpret_cast<const uint32_t*>(&data[i + 4]);
            
            if (strcmp(chunkId, "fmt ") == 0 && i + 16 < data.size()) {
              numChannels = *reinterpret_cast<const uint16_t*>(&data[i + 10]);
              sampleRate = *reinterpret_cast<const uint32_t*>(&data[i + 12]);
              bitsPerSample = *reinterpret_cast<const uint16_t*>(&data[i + 22]);
            }
            
            if (strcmp(chunkId, "data") == 0) {
              dataSize = chunkSize;
              break;
            }
            
            i += 8 + chunkSize + (chunkSize & 1);
            if (i >= data.size()) break;
          }
          
          // Calculate duration if we have valid data
          if (sampleRate > 0 && numChannels > 0 && bitsPerSample > 0 && dataSize > 0) {
            uint32_t bytesPerSample = bitsPerSample / 8;
            uint32_t bytesPerSecond = sampleRate * numChannels * bytesPerSample;
            
            if (bytesPerSecond > 0) {
              float durationInSeconds = static_cast<float>(dataSize) / bytesPerSecond;
              log_to_file("Calculated track duration: " + std::to_string(durationInSeconds) + "s");
              return static_cast<int>(durationInSeconds) + 1; // Add 1 sec buffer
            }
          }
        }
        
        log_to_file("Could not calculate duration, using default");
        return 120; // Default fallback duration
      };
      
      while (sound_enabled_ && !musicStopFlag.load()) {
        SoundEvent audioEvent;

        if (this->retroModeActive) {
            audioEvent = backgroundMusicTracksRetro[currentTrackIndex];
        } else {
            audioEvent = backgroundMusicTracks[currentTrackIndex];
        }    
        if (!audioManager.isMuted() && !musicPaused) {
          try {
            // Calculate the duration of this track
            int trackDuration = calculateWavDuration(audioEvent);
            log_to_file("Track duration: " + std::to_string(trackDuration) + " seconds");
            
            // Play the track
            audioManager.playSound(audioEvent);
            
            // Wait for the track to finish or pause to occur
            int elapsedSeconds = 0;
            while (elapsedSeconds < trackDuration && sound_enabled_ && !musicStopFlag.load()) {
              // Only increment timer if not paused
              if (!musicPaused) {
                std::this_thread::sleep_for(std::chrono::seconds(1));
                elapsedSeconds++;
              } else {
                // If paused, just do short sleep to avoid CPU spinning
                std::this_thread::sleep_for(std::chrono::milliseconds(100));
              }
            }
            
            // Only move to next track if we completed normally
            if (elapsedSeconds >= trackDuration && sound_enabled_ && !musicStopFlag.load()) {
              // Find the next enabled track
              size_t nextTrackIndex = (currentTrackIndex + 1) % backgroundMusicTracks.size();
              
              // Check if all tracks are disabled
              bool allDisabled = true;
              for (int i = 0; i < 5; i++) {
                if (enabledTracks[i]) {
                  allDisabled = false;
                  break;
                }
              }
              
              if (!allDisabled) {
                // Skip disabled tracks
                while (!enabledTracks[nextTrackIndex]) {
                  nextTrackIndex = (nextTrackIndex + 1) % backgroundMusicTracks.size();
                  // If we looped back to the current track, break to avoid infinite loop
                  if (nextTrackIndex == currentTrackIndex) {
                    break;
                  }
                }
              }
              
              currentTrackIndex = nextTrackIndex;
              log_to_file("Moving to track index: " + std::to_string(currentTrackIndex));
            }
          } catch (const std::exception& e) {
            log_to_file("Exception during music playback: " + std::string(e.what()));
            std::this_thread::sleep_for(std::chrono::seconds(2));
          } catch (...) {
            log_to_file("Unknown exception during music playback");
            std::this_thread::sleep_for(std::chrono::seconds(2));
          }
        } else {
          // If muted or paused, just wait a bit before checking again
          std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }
      }
      #else
      // Original implementation for non-Windows platforms with track skipping
      while (sound_enabled_ && !musicStopFlag.load()) {
        SoundEvent audioEvent;
        if (this->retroModeActive) {
            audioEvent = backgroundMusicTracksRetro[currentTrackIndex];
        
        } else {
            audioEvent = backgroundMusicTracks[currentTrackIndex];
        }
        if (!audioManager.isMuted() && !musicPaused) {
          try {
            audioManager.playSoundAndWait(audioEvent);
            std::this_thread::sleep_for(std::chrono::milliseconds(50));
            
            // Find the next enabled track
            size_t nextTrackIndex = (currentTrackIndex + 1) % backgroundMusicTracks.size();
            
            // Check if all tracks are disabled
            bool allDisabled = true;
            for (int i = 0; i < 5; i++) {
              if (enabledTracks[i]) {
                allDisabled = false;
                break;
              }
            }
            
            if (!allDisabled) {
              // Skip disabled tracks
              while (!enabledTracks[nextTrackIndex]) {
                nextTrackIndex = (nextTrackIndex + 1) % backgroundMusicTracks.size();
                // If we looped back to the current track, break to avoid infinite loop
                if (nextTrackIndex == currentTrackIndex) {
                  break;
                }
              }
            }
            
            currentTrackIndex = nextTrackIndex;
          } catch (const std::exception& e) {
            log_to_file("Exception during music playback: " + std::string(e.what()));
            break;
          } catch (...) {
            log_to_file("Unknown exception during music playback");
            break;
          }
        } else {
          // If muted or paused, just wait a bit
          std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }
      }
      #endif

      musicThreadRunning = false;
    }).detach();
  }
}

void TetrimoneBoard::pauseBackgroundMusic() {
  log_to_file("pauseBackgroundMusic called");
  
  if (!sound_enabled_) {
    log_to_file("sound not enabled, stopping music thread");
    musicStopFlag = true;
    AudioManager::getInstance().setMuted(true);
    return;
  }

  log_to_file("Setting musicPaused to true and muting audio");
  musicPaused = true;
  AudioManager::getInstance().setMuted(true);
}

void TetrimoneBoard::playSound(GameSoundEvent event) {
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
  case GameSoundEvent::Tetrimone:
    audioEvent = SoundEvent::Tetrimone;
    break;
  case GameSoundEvent::Excellent:
    audioEvent = SoundEvent::Excellent;
    break;
  case GameSoundEvent::BackgroundMusicRetro:
    audioEvent = SoundEvent::BackgroundMusicRetro;
    break;
  case GameSoundEvent::BackgroundMusic2Retro:
    audioEvent = SoundEvent::BackgroundMusic2Retro;
    break;
  case GameSoundEvent::BackgroundMusic3Retro:
    audioEvent = SoundEvent::BackgroundMusic3Retro;
    break;
  case GameSoundEvent::BackgroundMusic4Retro:
    audioEvent = SoundEvent::BackgroundMusic4Retro;
    break;
  case GameSoundEvent::BackgroundMusic5Retro:
    audioEvent = SoundEvent::BackgroundMusic5Retro;
    break;

  default:
    std::cerr << "Unknown sound event" << std::endl;
    return;
  }
  // Play the sound asynchronously
  AudioManager::getInstance().playSound(audioEvent);
}

void TetrimoneBoard::cleanupAudio() {
  log_to_file("cleanupAudio called");
  
  if (!sound_enabled_) {
    log_to_file("sound not enabled, nothing to clean up");
    return;
  }
    
  log_to_file("Setting sound_enabled_ to false");
  sound_enabled_ = false;
    
  log_to_file("Setting musicStopFlag and musicPaused");
  musicStopFlag = true;
  musicPaused = true;
    
  log_to_file("Waiting for background music thread to exit");
  std::this_thread::sleep_for(std::chrono::milliseconds(200));
    
  try {
    log_to_file("Telling audio manager to stop all sounds");
    AudioManager::getInstance().setMuted(true);
        
    log_to_file("Waiting for sound effects to stop");
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
        
    log_to_file("Shutting down audio manager");
    AudioManager::getInstance().shutdown();
    log_to_file("Audio manager shutdown complete");
  }
  catch (const std::exception& e) {
    log_to_file("Exception during audio cleanup: " + std::string(e.what()));
  }
  catch (...) {
    log_to_file("Unknown exception during audio cleanup");
  }
}

bool TetrimoneBoard::setSoundsZipPath(const std::string &path) {
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

void TetrimoneBoard::resumeBackgroundMusic() {
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
