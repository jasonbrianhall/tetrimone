#include "tetrimone.h"
#include <iostream>
#include <string>
#include "zip.h"
#include <fstream>
#include <algorithm>
#ifdef _WIN32
#include <windows.h>
#include <commdlg.h>
#include <direct.h>
#endif

void onBackgroundZipDialog(GtkMenuItem* menuItem, gpointer userData) {
    TetrimoneApp* app = static_cast<TetrimoneApp*>(userData);
    
    // Pause the game if it's running
    bool wasPaused = app->board->isPaused();
    if (!wasPaused && !app->board->isGameOver() && !app->board->isSplashScreenActive()) {
        onPauseGame(GTK_MENU_ITEM(app->pauseMenuItem), app);
    }
    
    std::string filePath;
    
#ifdef _WIN32
    // Use Windows native dialog
    OPENFILENAME ofn;
    char szFile[260] = {0};
    
    ZeroMemory(&ofn, sizeof(ofn));
    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner = NULL;  // Ideally get the HWND from GTK window
    ofn.lpstrFile = szFile;
    ofn.nMaxFile = sizeof(szFile);
    ofn.lpstrFilter = "ZIP Files\0*.zip\0All Files\0*.*\0";
    ofn.nFilterIndex = 1;
    ofn.lpstrFileTitle = NULL;
    ofn.nMaxFileTitle = 0;
    ofn.lpstrInitialDir = NULL;
    ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST;
    
    if (GetOpenFileName(&ofn)) {
        filePath = ofn.lpstrFile;
    }
#else
    // Use GTK dialog on other platforms
    GtkWidget* dialog = gtk_file_chooser_dialog_new(
        "Select Background Images ZIP File",
        GTK_WINDOW(app->window),
        GTK_FILE_CHOOSER_ACTION_OPEN,
        "_Cancel", GTK_RESPONSE_CANCEL,
        "_Open", GTK_RESPONSE_ACCEPT,
        NULL
    );
    
    // Add filter for ZIP files only
    GtkFileFilter* filter = gtk_file_filter_new();
    gtk_file_filter_set_name(filter, "ZIP Files");
    gtk_file_filter_add_pattern(filter, "*.zip");
    gtk_file_filter_add_mime_type(filter, "application/zip");
    gtk_file_chooser_add_filter(GTK_FILE_CHOOSER(dialog), filter);
    
    // Run the dialog
    if (gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_ACCEPT) {
        char* filename = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(dialog));
        filePath = filename;
        g_free(filename);
    }
    
    gtk_widget_destroy(dialog);
#endif
    
    // Process the selected file path
    if (!filePath.empty()) {
        if (app->board->loadBackgroundImagesFromZip(filePath)) {
            gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(app->backgroundToggleMenuItem), TRUE);
            
            // Process any pending events before showing opacity dialog
            while (gtk_events_pending())
                gtk_main_iteration();
                
            // Now show the opacity dialog
            onBackgroundOpacityDialog(NULL, app);
        } else {
            GtkWidget* errorDialog = gtk_message_dialog_new(
                GTK_WINDOW(app->window),
                GTK_DIALOG_MODAL,
                GTK_MESSAGE_ERROR,
                GTK_BUTTONS_OK,
                "Failed to load background images from ZIP: %s", filePath.c_str()
            );
            gtk_dialog_run(GTK_DIALOG(errorDialog));
            gtk_widget_destroy(errorDialog);
        }
    }
    
    // Redraw the game area
    gtk_widget_queue_draw(app->gameArea);
    
    // Resume the game if it wasn't paused before
    if (!wasPaused && !app->board->isGameOver() && !app->board->isSplashScreenActive()) {
        onPauseGame(GTK_MENU_ITEM(app->pauseMenuItem), app);
    }
}

// Update the background toggle handler to handle ZIP mode
void onBackgroundToggled(GtkCheckMenuItem* menuItem, gpointer userData) {
    TetrimoneApp* app = static_cast<TetrimoneApp*>(userData);
    bool useBackground = gtk_check_menu_item_get_active(menuItem);
    
    // The toggle should control visibility, regardless of background mode
    app->board->setUseBackgroundImage(useBackground);
    
    // Also update ZIP mode flag to match if using backgrounds from ZIP
    if (app->board->isUsingBackgroundZip()) {
        app->board->setUseBackgroundZip(useBackground);
    }
    
    // Redraw the game area
    gtk_widget_queue_draw(app->gameArea);
}

void onBackgroundOpacityDialog(GtkMenuItem *menuItem, gpointer userData) {
  TetrimoneApp *app = static_cast<TetrimoneApp *>(userData);

  // Create dialog
  GtkWidget *dialog = gtk_dialog_new_with_buttons(
      "Background Opacity", GTK_WINDOW(app->window), GTK_DIALOG_MODAL, "_OK",
      GTK_RESPONSE_OK, NULL);

  // Make it a reasonable size
  gtk_window_set_default_size(GTK_WINDOW(dialog), 300, 150);

  // Create content area
  GtkWidget *contentArea = gtk_dialog_get_content_area(GTK_DIALOG(dialog));
  gtk_container_set_border_width(GTK_CONTAINER(contentArea), 10);

  // Create a vertical box for content
  GtkWidget *vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 6);
  gtk_container_add(GTK_CONTAINER(contentArea), vbox);

  // Add a label
  GtkWidget *label = gtk_label_new("Adjust background opacity:");
  gtk_widget_set_halign(label, GTK_ALIGN_START);
  gtk_box_pack_start(GTK_BOX(vbox), label, FALSE, FALSE, 0);

  // Create a horizontal scale (slider)
  GtkWidget *scale =
      gtk_scale_new_with_range(GTK_ORIENTATION_HORIZONTAL, 0.0, 1.0, 0.05);
  gtk_range_set_value(GTK_RANGE(scale), app->board->getBackgroundOpacity());
  gtk_scale_set_digits(GTK_SCALE(scale), 2); // 2 decimal places
  gtk_scale_set_value_pos(GTK_SCALE(scale), GTK_POS_RIGHT);
  gtk_box_pack_start(GTK_BOX(vbox), scale, FALSE, FALSE, 0);

  // Add min/max labels
  GtkWidget *rangeBox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
  gtk_box_pack_start(GTK_BOX(vbox), rangeBox, FALSE, FALSE, 0);

  GtkWidget *minLabel = gtk_label_new("Transparent");
  gtk_widget_set_halign(minLabel, GTK_ALIGN_START);
  gtk_box_pack_start(GTK_BOX(rangeBox), minLabel, TRUE, TRUE, 0);

  GtkWidget *maxLabel = gtk_label_new("Opaque");
  gtk_widget_set_halign(maxLabel, GTK_ALIGN_END);
  gtk_box_pack_end(GTK_BOX(rangeBox), maxLabel, TRUE, TRUE, 0);

  // Connect value-changed signal to update the opacity in real-time
  g_signal_connect(G_OBJECT(scale), "value-changed",
                   G_CALLBACK(onOpacityValueChanged), app);

  // Show all dialog widgets
  gtk_widget_show_all(dialog);

  // Run the dialog
  gtk_dialog_run(GTK_DIALOG(dialog));

  // Destroy dialog
  gtk_widget_destroy(dialog);
}
