#include "tetris.h"
#include <iostream>
#include <string>

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
    gtk_window_set_default_size(GTK_WINDOW(dialog), 300, 150);
    
    // Create content area
    GtkWidget* contentArea = gtk_dialog_get_content_area(GTK_DIALOG(dialog));
    gtk_container_set_border_width(GTK_CONTAINER(contentArea), 10);
    
    // Create a vertical box for content
    GtkWidget* vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 6);
    gtk_container_add(GTK_CONTAINER(contentArea), vbox);
    
    // Add a label
    GtkWidget* label = gtk_label_new("Adjust sound volume:");
    gtk_widget_set_halign(label, GTK_ALIGN_START);
    gtk_box_pack_start(GTK_BOX(vbox), label, FALSE, FALSE, 0);
    
    // Current volume (get from AudioManager)
    float currentVolume = AudioManager::getInstance().getVolume();
    
    // Create a horizontal scale (slider)
    GtkWidget* scale = gtk_scale_new_with_range(GTK_ORIENTATION_HORIZONTAL, 
                                             0.0, 1.0, 0.05);
    gtk_range_set_value(GTK_RANGE(scale), currentVolume);
    gtk_scale_set_digits(GTK_SCALE(scale), 2); // 2 decimal places
    gtk_scale_set_value_pos(GTK_SCALE(scale), GTK_POS_RIGHT);
    gtk_box_pack_start(GTK_BOX(vbox), scale, FALSE, FALSE, 0);
    
    // Add min/max labels
    GtkWidget* rangeBox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
    gtk_box_pack_start(GTK_BOX(vbox), rangeBox, FALSE, FALSE, 0);
    
    GtkWidget* minLabel = gtk_label_new("Mute");
    gtk_widget_set_halign(minLabel, GTK_ALIGN_START);
    gtk_box_pack_start(GTK_BOX(rangeBox), minLabel, TRUE, TRUE, 0);
    
    GtkWidget* maxLabel = gtk_label_new("Max");
    gtk_widget_set_halign(maxLabel, GTK_ALIGN_END);
    gtk_box_pack_end(GTK_BOX(rangeBox), maxLabel, TRUE, TRUE, 0);
    
    // Connect value-changed signal to update the volume in real-time
    g_signal_connect(G_OBJECT(scale), "value-changed", 
                   G_CALLBACK(onVolumeValueChanged), app);
    
    // Show all dialog widgets
    gtk_widget_show_all(dialog);
    
    // Run the dialog
    gtk_dialog_run(GTK_DIALOG(dialog));
    
    // Destroy dialog
    gtk_widget_destroy(dialog);
}

void onVolumeValueChanged(GtkRange* range, gpointer userData) {
    TetrisApp* app = static_cast<TetrisApp*>(userData);
    
    // Update the volume 
    double volume = gtk_range_get_value(range);
    
    // Update the AudioManager volume
    AudioManager::getInstance().setVolume(volume);
}
