#include "tetrimone.h"
#include <iostream>
#include <string>
#include <iomanip>

// Static variables to backup volume values in case AudioManager doesn't persist them
static float s_lastSfxVolume = 0.5f;    // Default to 50% if not set
static float s_lastMusicVolume = 0.5f;  // Default to 50% if not set
static bool s_volumesInitialized = false;

void onVolumeDialog(GtkMenuItem* menuItem, gpointer userData) {
    TetrisApp* app = static_cast<TetrisApp*>(userData);
    
    // Create dialog
    GtkWidget* dialog = gtk_dialog_new_with_buttons(
        "Volume Control",
        GTK_WINDOW(app->window),
        GTK_DIALOG_MODAL,
        "_OK", GTK_RESPONSE_OK,
        NULL
    );
    
    // Make it a reasonable size
    gtk_window_set_default_size(GTK_WINDOW(dialog), 300, 200);
    
    // Create content area
    GtkWidget* contentArea = gtk_dialog_get_content_area(GTK_DIALOG(dialog));
    gtk_container_set_border_width(GTK_CONTAINER(contentArea), 10);
    
    // Create a vertical box for content
    GtkWidget* vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 10);
    gtk_container_add(GTK_CONTAINER(contentArea), vbox);
    
    // === SOUND EFFECTS VOLUME CONTROLS ===
    // Add a label for sound effects
    GtkWidget* sfxLabel = gtk_label_new("Sound Effects Volume:");
    gtk_widget_set_halign(sfxLabel, GTK_ALIGN_START);
    gtk_box_pack_start(GTK_BOX(vbox), sfxLabel, FALSE, FALSE, 0);
    
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
    std::cout << "Setting SFX slider to: " << volumePercent << "%" << std::endl;
    
    // Create a horizontal scale (slider) for sound effects using percentage (0-100)
    GtkWidget* sfxScale = gtk_scale_new_with_range(GTK_ORIENTATION_HORIZONTAL, 
                                               0.0, 100.0, 10.0);
    gtk_range_set_value(GTK_RANGE(sfxScale), volumePercent);
    gtk_scale_set_digits(GTK_SCALE(sfxScale), 0); // No decimal places for percentage
    gtk_scale_set_value_pos(GTK_SCALE(sfxScale), GTK_POS_RIGHT);
    gtk_box_pack_start(GTK_BOX(vbox), sfxScale, FALSE, FALSE, 0);
    
    // Add min/max labels for sound effects
    GtkWidget* sfxRangeBox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
    gtk_box_pack_start(GTK_BOX(vbox), sfxRangeBox, FALSE, FALSE, 0);
    
    GtkWidget* sfxMinLabel = gtk_label_new("Mute");
    gtk_widget_set_halign(sfxMinLabel, GTK_ALIGN_START);
    gtk_box_pack_start(GTK_BOX(sfxRangeBox), sfxMinLabel, TRUE, TRUE, 0);
    
    GtkWidget* sfxMaxLabel = gtk_label_new("Max");
    gtk_widget_set_halign(sfxMaxLabel, GTK_ALIGN_END);
    gtk_box_pack_end(GTK_BOX(sfxRangeBox), sfxMaxLabel, TRUE, TRUE, 0);
    
    // Connect value-changed signal to update the volume in real-time
    g_signal_connect(G_OBJECT(sfxScale), "value-changed", 
                   G_CALLBACK(onVolumeValueChanged), app);
    
    // Add a separator
    GtkWidget* separator = gtk_separator_new(GTK_ORIENTATION_HORIZONTAL);
    gtk_box_pack_start(GTK_BOX(vbox), separator, FALSE, FALSE, 5);
    
    // === MUSIC VOLUME CONTROLS ===
    // Add a label for music
    GtkWidget* musicLabel = gtk_label_new("Music Volume:");
    gtk_widget_set_halign(musicLabel, GTK_ALIGN_START);
    gtk_box_pack_start(GTK_BOX(vbox), musicLabel, FALSE, FALSE, 0);
    
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
    
    // Create a horizontal scale (slider) for music using percentage (0-100)
    GtkWidget* musicScale = gtk_scale_new_with_range(GTK_ORIENTATION_HORIZONTAL, 
                                                 0.0, 100.0, 10.0);
    gtk_range_set_value(GTK_RANGE(musicScale), musicVolumePercent);
    gtk_scale_set_digits(GTK_SCALE(musicScale), 0); // No decimal places for percentage
    gtk_scale_set_value_pos(GTK_SCALE(musicScale), GTK_POS_RIGHT);
    gtk_box_pack_start(GTK_BOX(vbox), musicScale, FALSE, FALSE, 0);
    
    // Add min/max labels for music
    GtkWidget* musicRangeBox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
    gtk_box_pack_start(GTK_BOX(vbox), musicRangeBox, FALSE, FALSE, 0);
    
    GtkWidget* musicMinLabel = gtk_label_new("Mute");
    gtk_widget_set_halign(musicMinLabel, GTK_ALIGN_START);
    gtk_box_pack_start(GTK_BOX(musicRangeBox), musicMinLabel, TRUE, TRUE, 0);
    
    GtkWidget* musicMaxLabel = gtk_label_new("Max");
    gtk_widget_set_halign(musicMaxLabel, GTK_ALIGN_END);
    gtk_box_pack_end(GTK_BOX(musicRangeBox), musicMaxLabel, TRUE, TRUE, 0);
    
    // Connect value-changed signal to update the music volume in real-time
    g_signal_connect(G_OBJECT(musicScale), "value-changed", 
                   G_CALLBACK(onMusicVolumeValueChanged), app);
    
    // Show all dialog widgets
    gtk_widget_show_all(dialog);
    
    // Run the dialog
    gtk_dialog_run(GTK_DIALOG(dialog));
    
    // Destroy dialog
    gtk_widget_destroy(dialog);
}

void onVolumeValueChanged(GtkRange* range, gpointer userData) {
    TetrisApp* app = static_cast<TetrisApp*>(userData);
    
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
    TetrisApp* app = static_cast<TetrisApp*>(userData);
    
    // Get the percentage value from the slider
    double musicVolumePercent = gtk_range_get_value(range);
    
    // Convert from percentage (0-100) to decimal (0.0-1.0)
    double musicVolume = musicVolumePercent / 100.0;
    
    // Update our backup value
    s_lastMusicVolume = musicVolume;
    
    // Update the AudioManager music volume
    AudioManager::getInstance().setMusicVolume(musicVolume);
}
