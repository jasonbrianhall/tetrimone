#include "tetrimone_gtk.h"
#include "gtk3_dialog_helpers.h"
#include <iostream>
#include <string>
#include <iomanip>

using namespace GTK3Helpers;

// Static variables to backup volume values in case AudioManager doesn't persist them
static float s_lastSfxVolume = 0.50f;    // Default to 50% if not set
static float s_lastMusicVolume = 0.50f;  // Default to 50% if not set
static bool s_volumesInitialized = false;

void onVolumeDialog(GtkMenuItem* menuItem, gpointer userData) {
    TetrimoneApp* app = static_cast<TetrimoneApp*>(userData);
    
    // Check if retro mode is active
    bool isRetroMode = app->board->retroModeActive;
    
    // Current sound effects volume
    float currentVolume = AudioManager::getInstance().getVolume();
    
    // Initialize backup values on first run if they haven't been set yet
    if (!s_volumesInitialized) {
        // Only set default values on the first run
        if (currentVolume > 0.0f) {
            s_lastSfxVolume = currentVolume;
        }
        s_volumesInitialized = true;
    }
    
    // If AudioManager returns 0, use our backup value
    if (currentVolume <= 0.0f && s_lastSfxVolume > 0.0f) {
        std::cout << "AudioManager returned 0 for SFX volume, using backup value: " 
                  << s_lastSfxVolume << std::endl;
        currentVolume = s_lastSfxVolume;
        
        // Also update the AudioManager with our backup value
        AudioManager::getInstance().setVolume(currentVolume);
    }
    
    // Convert to percentage for UI
    int volumePercent = static_cast<int>(currentVolume * 100.0);
    
    // Current music volume
    float currentMusicVolume = AudioManager::getInstance().getMusicVolume();
    
    // Initialize music volume backup on first run
    if (!s_volumesInitialized && currentMusicVolume > 0.0f) {
        s_lastMusicVolume = currentMusicVolume;
    }
    
    // If AudioManager returns 0, use our backup value
    if (currentMusicVolume <= 0.0f && s_lastMusicVolume > 0.0f) {
        std::cout << "AudioManager returned 0 for music volume, using backup value: " 
                  << s_lastMusicVolume << std::endl;
        currentMusicVolume = s_lastMusicVolume;
        
        // Also update the AudioManager with our backup value
        AudioManager::getInstance().setMusicVolume(currentMusicVolume);
    }
    
    // Convert to percentage for UI
    int musicVolumePercent = static_cast<int>(currentMusicVolume * 100.0);
    
    // Create config
    VolumeControlConfig config{
        .title = isRetroMode ? "ЦЕНТРАЛЬНЫЙ КОНТРОЛЬ ЗВУКА" : "Volume Control",
        .okButtonLabel = isRetroMode ? "_ПРИНЯТО" : "_OK",
        .isRetroMode = isRetroMode,
        .sfxVolume = volumePercent,
        .musicVolume = musicVolumePercent,
        .width = 350,
        .height = 250
    };
    
    // Create and run dialog via helper
    createVolumeControlDialog(GTK_WINDOW(app->window), config, app);
}

void onTestSound(GtkButton* button, gpointer userData) {
    TetrimoneApp* app = static_cast<TetrimoneApp*>(userData);
    
    // Get current volume settings
    float currentSfxVolume = AudioManager::getInstance().getVolume();
    float currentMusicVolume = AudioManager::getInstance().getMusicVolume();
    
    // Determine which sound to play based on current mode
    GameSoundEvent testSound = app->board->retroModeActive ? 
                               GameSoundEvent::Select : 
                               GameSoundEvent::LevelUp;
    
    // If sound effects are muted, temporarily set to 50% for test
    bool wasMuted = (currentSfxVolume <= 0.0f);
    if (wasMuted) {
        AudioManager::getInstance().setVolume(0.5f); // Set to 50% temporarily
    }
    
    // Play the test sound
    app->board->playSound(testSound);
    
    // If sound was originally muted, restore after a short delay
    if (wasMuted) {
        // Schedule a function to restore volume after a short delay
        g_timeout_add(500, // 500ms delay to allow sound to play
            [](gpointer userData) -> gboolean {
                float* originalVolume = static_cast<float*>(userData);
                AudioManager::getInstance().setVolume(*originalVolume);
                delete originalVolume; // Clean up allocated memory
                return G_SOURCE_REMOVE; // Remove this timeout
            }, 
            new float(currentSfxVolume)); // Pass the original volume value
    }
}


void onVolumeValueChanged(GtkRange* range, gpointer userData) {
    TetrimoneApp* app = static_cast<TetrimoneApp*>(userData);
    
    // Get the percentage value from the slider
    double volumePercent = gtk_range_get_value(range);
    
    // Convert from percentage (0-100) to decimal (0.0-1.0)
    double volume = volumePercent / 100.0;
    
    
    // Update our backup value
    s_lastSfxVolume = volume;
    
    // Update the AudioManager volume
    AudioManager::getInstance().setVolume(volume);
}

void onMusicVolumeValueChanged(GtkRange* range, gpointer userData) {
    TetrimoneApp* app = static_cast<TetrimoneApp*>(userData);
    
    // Get the percentage value from the slider
    double musicVolumePercent = gtk_range_get_value(range);
    
    // Convert from percentage (0-100) to decimal (0.0-1.0)
    double musicVolume = musicVolumePercent / 100.0;
    
    // Update our backup value
    s_lastMusicVolume = musicVolume;
    
    // Update the AudioManager music volume
    AudioManager::getInstance().setMusicVolume(musicVolume);
}
