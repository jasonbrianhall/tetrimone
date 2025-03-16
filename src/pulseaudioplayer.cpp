#ifndef _WIN32

#include "audiomanager.h"
#include <cstring>
#include <iostream>
#include <pulse/pulseaudio.h>
#include <pulse/simple.h>
#include <thread>
#include <atomic>

// Global atomic flag to signal stopping playback
namespace {
  std::atomic<bool> g_shouldStopPlayback(false);
}

class PulseAudioPlayer : public AudioPlayer {
public:
  PulseAudioPlayer() : volume_(1.0f) {
    // Set default sample specification
    sampleSpec_.format = PA_SAMPLE_S16LE;
    sampleSpec_.rate = 44100;
    sampleSpec_.channels = 2;
  }

  ~PulseAudioPlayer() override { 
    shutdown(); 
  }

  bool initialize() override {
    // Test if we can create a connection to PulseAudio
    int error = 0;
    pa_simple *s = pa_simple_new(NULL,               // Use default server
                                 "TetrisAudio",      // Application name
                                 PA_STREAM_PLAYBACK, // Stream direction
                                 NULL,               // Default device
                                 "Test",             // Stream description
                                 &sampleSpec_,       // Sample format
                                 NULL,               // Default channel map
                                 NULL,               // Default buffering attributes
                                 &error              // Error code
    );

    if (s) {
      pa_simple_free(s);
      return true;
    } else {
      std::cerr << "DEBUG: Failed to initialize PulseAudio: " << pa_strerror(error) << std::endl;
      return false;
    }
  }

  void shutdown() override {
    // Reset the stop flag before shutting down
    g_shouldStopPlayback = true;
    
    // Give active threads time to notice and exit
    std::this_thread::sleep_for(std::chrono::milliseconds(200));
    
    g_shouldStopPlayback = false;
  }

  void stopAllSounds() override {
    // Set the global flag to signal stopping
    g_shouldStopPlayback = true;
    
    // Give threads a moment to exit
    std::this_thread::sleep_for(std::chrono::milliseconds(200));
    
    // Reset the flag for future playback
    g_shouldStopPlayback = false;
  }

  void playSound(const std::vector<uint8_t> &data, const std::string &format,
                 std::shared_ptr<std::promise<void>> completionPromise =
                     nullptr) override {
                     
    // If the stop flag is set, don't start playback
    if (g_shouldStopPlayback) {
      if (completionPromise) {
        completionPromise->set_value();
      }
      return;
    }
                     
    // Spawn a thread to handle the audio playback
    std::thread playbackThread([this, data, format, completionPromise]() {
      
      // For WAV files, we need to find the actual data chunk instead of just
      // skipping a fixed header
      size_t dataOffset = 0;
      pa_sample_spec ss = sampleSpec_; // Use default values initially

      if (format == "wav" && data.size() >= 44) {
        
        // First verify this is actually a WAV file
        if (memcmp(data.data(), "RIFF", 4) != 0 ||
            memcmp(data.data() + 8, "WAVE", 4) != 0) {
          std::cerr << "DEBUG: Not a valid WAV file" << std::endl;
          if (completionPromise) {
            completionPromise->set_value();
          }
          return;
        }

        // Find the 'data' chunk
        for (size_t i = 12; i < data.size() - 8;) {
          // Read chunk ID and length
          char chunkId[5] = {0};
          memcpy(chunkId, &data[i], 4);
          uint32_t chunkSize =
              *reinterpret_cast<const uint32_t *>(&data[i + 4]);

          
          // If this is the 'fmt ' chunk, parse audio format
          if (strcmp(chunkId, "fmt ") == 0) {
            ss.channels = *reinterpret_cast<const uint16_t *>(&data[i + 10]);
            ss.rate = *reinterpret_cast<const uint32_t *>(&data[i + 12]);
            uint16_t bitsPerSample =
                *reinterpret_cast<const uint16_t *>(&data[i + 22]);

            // Set format based on bits per sample
            if (bitsPerSample == 8) {
              ss.format = PA_SAMPLE_U8;
            } else if (bitsPerSample == 16) {
              ss.format = PA_SAMPLE_S16LE;
            } else if (bitsPerSample == 24) {
              ss.format = PA_SAMPLE_S24LE;
            } else if (bitsPerSample == 32) {
              ss.format = PA_SAMPLE_S32LE;
            }

          }
          // If this is the 'data' chunk, we've found our audio data
          if (strcmp(chunkId, "data") == 0) {
            dataOffset = i + 8; // Skip chunk ID and size
            break;
          }

          // Move to next chunk (add 8 for header size plus chunk size)
          // Ensure chunk size is even by adding 1 if needed
          i += 8 + chunkSize + (chunkSize & 1);
          if (i >= data.size())
            break;
        }

        if (dataOffset == 0) {
          std::cerr << "DEBUG: No 'data' chunk found in WAV file" << std::endl;
          if (completionPromise) {
            completionPromise->set_value();
          }
          return;
        }
      }

      // Check if we should stop before opening PulseAudio
      if (g_shouldStopPlayback) {
        if (completionPromise) {
          completionPromise->set_value();
        }
        return;
      }

      // Create the PulseAudio stream
      int error = 0;
      pa_simple *s =
          pa_simple_new(NULL, "TetrisAudio", PA_STREAM_PLAYBACK, NULL,
                        format.c_str(), &ss, NULL, NULL, &error);

      if (!s) {
        std::cerr << "DEBUG: Failed to open PulseAudio stream: " << pa_strerror(error) << std::endl;
        if (completionPromise) {
          completionPromise->set_value();
        }
        return;
      }

      // Add a sanity check for data length
      size_t dataLength = data.size() - dataOffset;
      if (dataLength > 100 * 1024 * 1024) { // Limit to 100MB as a safety check
        std::cerr << "DEBUG: Data size too large: " << dataLength << std::endl;
        pa_simple_free(s);
        if (completionPromise) {
          completionPromise->set_value();
        }
        return;
      }
      
      // Write data in chunks to allow stopping mid-playback
      const size_t CHUNK_SIZE = 32 * 1024; // 32KB chunks
      const uint8_t* audioData = data.data() + dataOffset;
      size_t remaining = dataLength;
      size_t offset = 0;
      
      
      while (remaining > 0 && !g_shouldStopPlayback) {
        size_t chunkSize = std::min(remaining, CHUNK_SIZE);
        
        if (pa_simple_write(s, audioData + offset, chunkSize, &error) < 0) {
          std::cerr << "DEBUG: Failed to write audio data: " << pa_strerror(error) << std::endl;
          break;
        }
        
        offset += chunkSize;
        remaining -= chunkSize;
        
        // Check if we should stop after each chunk
        if (g_shouldStopPlayback) {
          break;
        }
      }

      // Check if we were interrupted or finished normally
      if (g_shouldStopPlayback) {
        pa_simple_flush(s, &error);
      } else {
        if (pa_simple_drain(s, &error) < 0) {
          std::cerr << "DEBUG: Failed to drain audio: " << pa_strerror(error) << std::endl;
        }
      }

      // Clean up
      pa_simple_free(s);

      // Signal completion
      if (completionPromise) {
        completionPromise->set_value();
      }
      
    });

    playbackThread.detach();
  }

  void setVolume(float volume) override {
    volume_ = volume;

    // Note: PulseAudio volume control is more complex and would require
    // using the PulseAudio context API instead of the simple API
    // For a basic implementation, we don't modify system volume here
  }

private:
  float volume_;
  pa_sample_spec sampleSpec_;
};

// Factory function implementation for PulseAudio
std::unique_ptr<AudioPlayer> createAudioPlayer() {
  return std::make_unique<PulseAudioPlayer>();
}

#endif // !_WIN32
