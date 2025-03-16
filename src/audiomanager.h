#ifndef AUDIO_MANAGER_H
#define AUDIO_MANAGER_H

#include <condition_variable>
#include <future>
#include <memory>
#include <mutex>
#include <string>
#include <unordered_map>
#include <vector>

// Sound event types for the game
enum class SoundEvent {
  BackgroundMusic,
  Gameover,
  Clear,
  Drop,
  LateralMove,
  LevelUp,
  Rotate,
  Select,
  Start,
  Tetris,
  Excellent
};

// Platform-independent class to handle sound playback
class AudioPlayer {
public:
  AudioPlayer();
  virtual ~AudioPlayer();

  // Initialize the audio system
  virtual bool initialize() = 0;

  // Clean up resources
  virtual void shutdown() = 0;

  // Play a sound from memory with optional callback when complete
  virtual void playSound(
      const std::vector<uint8_t> &data, const std::string &format,
      std::shared_ptr<std::promise<void>> completionPromise = nullptr) = 0;

  // Set volume (0.0 - 1.0)
  virtual void setVolume(float volume) = 0;

  virtual void stopAllSounds() = 0;
};

// Factory function to create the appropriate platform-specific player
std::unique_ptr<AudioPlayer> createAudioPlayer();

// Thread-safe singleton audio manager
class AudioManager {
public:
  // Get the singleton instance
  static AudioManager &getInstance();

  // Delete copy and move constructors and operators
  AudioManager(const AudioManager &) = delete;
  AudioManager &operator=(const AudioManager &) = delete;
  AudioManager(AudioManager &&) = delete;
  AudioManager &operator=(AudioManager &&) = delete;

  ~AudioManager();

  // Initialize the audio system
  bool initialize();

  // Clean up resources
  void shutdown();

  // Load a sound file
  bool loadSound(SoundEvent event, const std::string &filePath);

  // Load a sound from memory
  bool loadSoundFromMemory(SoundEvent event, const std::vector<uint8_t> &data,
                           const std::string &format);

  // Play a sound asynchronously
  void playSound(SoundEvent event);

  // Play a sound and wait for it to complete
  void playSoundAndWait(SoundEvent event);

  // Set volume (0.0 - 1.0)
  void setVolume(float volume);

  // Mute/unmute all sounds
  void setMuted(bool muted);

  // Check if audio is muted
  bool isMuted() const;

  // Check if audio is initialized and available
  bool isAvailable() const;

private:
  // Private constructor for singleton
  AudioManager();

  // Helper method to get file extension
  std::string getFileExtension(const std::string &filePath);

  // Implementation
  struct SoundData {
    std::vector<uint8_t> data;
    std::string format;
  };

  std::unordered_map<SoundEvent, SoundData> sounds_;
  std::unique_ptr<AudioPlayer> player_;
  float volume_;
  bool muted_;
  bool initialized_;
  std::mutex mutex_;
};

#endif // AUDIO_MANAGER_H
