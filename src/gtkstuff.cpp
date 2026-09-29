#include <cstring>
#include <iostream>
#include "commandline.h"
#include "tetrimone_gtk.h"
#include <iostream>
#include <string>
#include "zip.h"
#include <fstream>
#include <algorithm>
#include "gtk3_dialog_helpers.h"
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
    
    // Create dialog interface
    GTK3Helpers::GTK3FileDialog fileDialog(GTK_WINDOW(app->window));
    
    // Open file dialog
    std::string filePath = fileDialog.openFile(
        "Select Background Images ZIP File",
        "*.zip",
        "ZIP Files"
    );
    
    // Process the selected file path
    if (!filePath.empty()) {
        if (app->board->loadBackgroundImagesFromZip(filePath)) {
            gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(app->backgroundToggleMenuItem), true);
            
            // Process any pending events before showing opacity dialog
            while (gtk_events_pending())
                gtk_main_iteration();
                
            // Now show the opacity dialog
            onBackgroundOpacityDialog(NULL, app);
        } else {
            fileDialog.showError(
                "Error Loading Background",
                "Failed to load background images from ZIP: " + filePath
            );
        }
    }
    
    // Redraw the game area
    updateDisplay(app);
    
    // Resume the game if it wasn't paused before
    if (!wasPaused && !app->board->isGameOver() && !app->board->isSplashScreenActive()) {
        onPauseGame(GTK_MENU_ITEM(app->pauseMenuItem), app);
    }
}

void onBackgroundOpacityDialog(GtkMenuItem *menuItem, gpointer userData) {
  TetrimoneApp *app = static_cast<TetrimoneApp *>(userData);

  GTK3Helpers::OpacitySliderConfig config{
      .title = "Background Opacity",
      .minValue = 0.0,
      .maxValue = 1.0,
      .stepValue = 0.05,
      .currentValue = app->board->getBackgroundOpacity(),
      .width = 300,
      .height = 150
  };

  GTK3Helpers::createOpacitySliderDialog(
      GTK_WINDOW(app->window),
      config,
      G_CALLBACK(onOpacityValueChanged),
      app
  );
}

gboolean onDrawGameArea(GtkWidget *widget, cairo_t *cr, gpointer data) {
  TetrimoneApp *app = static_cast<TetrimoneApp *>(data);

  // Get widget dimensions
  GtkAllocation allocation;
  gtk_widget_get_allocation(widget, &allocation);

  OnDrawGameAreaCairo(cr, app, allocation.width, allocation.height);
  
  return FALSE;
}

gboolean onDrawNextPiece(GtkWidget *widget, cairo_t *cr, gpointer data) {
  TetrimoneApp *app = static_cast<TetrimoneApp *>(data);

  // Get widget dimensions
  GtkAllocation allocation;
  gtk_widget_get_allocation(widget, &allocation);
  onDrawNextPieceCairo(cr, app, allocation.width, allocation.height);

  return FALSE;
}

void onBackgroundImageDialog(GtkMenuItem* menuItem, gpointer userData) {
    TetrimoneApp* app = static_cast<TetrimoneApp*>(userData);
    
    // Pause the game if it's running
    bool wasPaused = app->board->isPaused();
    if (!wasPaused && !app->board->isGameOver() && !app->board->isSplashScreenActive()) {
        onPauseGame(GTK_MENU_ITEM(app->pauseMenuItem), app);
    }
    
    std::vector<std::string> filePaths;
    
#ifdef _WIN32
    // Use Windows native dialog with multi-select support
    OPENFILENAME ofn;
    char szFile[4096] = {0};  // Increased buffer size to support multiple files
    
    ZeroMemory(&ofn, sizeof(ofn));
    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner = NULL;  // Ideally get the HWND from GTK window
    ofn.lpstrFile = szFile;
    ofn.nMaxFile = sizeof(szFile);
    ofn.lpstrFilter = "Image Files\0*.png;*.jpg;*.jpeg\0PNG Images\0*.png\0JPEG Images\0*.jpg;*.jpeg\0All Files\0*.*\0";
    ofn.nFilterIndex = 1;
    ofn.lpstrFileTitle = NULL;
    ofn.nMaxFileTitle = 0;
    ofn.lpstrInitialDir = NULL;
    ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST | OFN_ALLOWMULTISELECT | OFN_EXPLORER;
    
    if (GetOpenFileName(&ofn)) {
        // Parse multiple file selection
        char* filePtr = ofn.lpstrFile;
        
        // First string is the directory
        std::string directory(filePtr);
        filePtr += directory.length() + 1;
        
        // If no files selected after directory, it means only one file was chosen
        if (*filePtr == '\0') {
            filePaths.push_back(directory);
        } else {
            // Multiple files selected
            while (*filePtr != '\0') {
                std::string filename(filePtr);
                std::string fullPath = directory + "\\" + filename;
                filePaths.push_back(fullPath);
                
                // Move to next filename
                filePtr += filename.length() + 1;
            }
        }
    }
#else
    // Use GTK dialog on other platforms
    GtkWidget* dialog = gtk_file_chooser_dialog_new(
        "Select Background Images",
        GTK_WINDOW(app->window),
        GTK_FILE_CHOOSER_ACTION_OPEN,
        "_Cancel", GTK_RESPONSE_CANCEL,
        "_Open", GTK_RESPONSE_ACCEPT,
        NULL
    );
    
    // Allow multiple file selection
    gtk_file_chooser_set_select_multiple(GTK_FILE_CHOOSER(dialog), true);
    
    // Add filter for image files
    GtkFileFilter* filterAll = gtk_file_filter_new();
    gtk_file_filter_set_name(filterAll, "All Image Files");
    gtk_file_filter_add_mime_type(filterAll, "image/png");
    gtk_file_filter_add_mime_type(filterAll, "image/jpeg");
    gtk_file_chooser_add_filter(GTK_FILE_CHOOSER(dialog), filterAll);
    
    // Add filter for PNG files
    GtkFileFilter* filterPng = gtk_file_filter_new();
    gtk_file_filter_set_name(filterPng, "PNG Images");
    gtk_file_filter_add_pattern(filterPng, "*.png");
    gtk_file_filter_add_mime_type(filterPng, "image/png");
    gtk_file_chooser_add_filter(GTK_FILE_CHOOSER(dialog), filterPng);
    
    // Add filter for JPEG files
    GtkFileFilter* filterJpeg = gtk_file_filter_new();
    gtk_file_filter_set_name(filterJpeg, "JPEG Images");
    gtk_file_filter_add_pattern(filterJpeg, "*.jpg");
    gtk_file_filter_add_pattern(filterJpeg, "*.jpeg");
    gtk_file_filter_add_mime_type(filterJpeg, "image/jpeg");
    gtk_file_chooser_add_filter(GTK_FILE_CHOOSER(dialog), filterJpeg);
    
    // Run the dialog
    if (gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_ACCEPT) {
        // Get the selected filenames
        GSList* filenames = gtk_file_chooser_get_filenames(GTK_FILE_CHOOSER(dialog));
        
        // Convert GSList to vector of strings
        for (GSList* list = filenames; list != NULL; list = list->next) {
            char* filename = static_cast<char*>(list->data);
            filePaths.push_back(filename);
            g_free(filename);
        }
        
        g_slist_free(filenames);
    }
    
    gtk_widget_destroy(dialog);
#endif
    
    // Process the selected file paths
    if (!filePaths.empty()) {
        // Clean up existing background images
        app->board->cleanupBackgroundImages();
        
        // Flag to track successful image loading
        bool imagesLoaded = false;
        
        // Process each selected file
        for (const auto& filepath : filePaths) {
            // Get file extension
            std::string extension = filepath.substr(filepath.find_last_of(".") + 1);
            std::transform(extension.begin(), extension.end(), extension.begin(), ::tolower);
            
            cairo_surface_t* surface = nullptr;
            
            if (extension == "jpg" || extension == "jpeg") {
                // Load JPEG
                surface = cairo_image_surface_create_from_jpeg(filepath.c_str());
            } else {
                // Default to PNG
                surface = cairo_image_surface_create_from_png(filepath.c_str());
            }
            
            // Check if the surface was created successfully
            if (surface && cairo_surface_status(surface) == CAIRO_STATUS_SUCCESS) {
                app->board->backgroundImages.push_back(surface);
                imagesLoaded = true;
            } else {
                std::cerr << "Failed to load image: " << filepath << std::endl;
                if (surface) cairo_surface_destroy(surface);
            }
        }
        
        if (imagesLoaded) {
            // Set background modes
            app->board->useBackgroundZip = true;
            app->board->useBackgroundImage = true;
            
            // Select initial random background
            app->board->selectRandomBackground();
            
            // Activate background toggle
            gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(app->backgroundToggleMenuItem), true);
            
            // Process any pending events before showing opacity dialog
            while (gtk_events_pending())
                gtk_main_iteration();
                
            // Show opacity dialog
            onBackgroundOpacityDialog(NULL, app);
        } else {
            // No images loaded
            GtkWidget* errorDialog = gtk_message_dialog_new(
                GTK_WINDOW(app->window),
                GTK_DIALOG_MODAL,
                GTK_MESSAGE_ERROR,
                GTK_BUTTONS_OK,
                "No valid image files could be loaded."
            );
            gtk_dialog_run(GTK_DIALOG(errorDialog));
            gtk_widget_destroy(errorDialog);
        }
    }
    
    // Redraw the game area
    updateDisplay(app);
    
    // Resume the game if it wasn't paused before
    if (!wasPaused && !app->board->isGameOver() && !app->board->isSplashScreenActive()) {
        onPauseGame(GTK_MENU_ITEM(app->pauseMenuItem), app);
    }
}

void onOpacityValueChanged(GtkRange *range, gpointer userData) {
  TetrimoneApp *app = static_cast<TetrimoneApp *>(userData);

  // Update the opacity in the board
  double opacity = gtk_range_get_value(range);

  // Early return if the background image isn't valid
  if (!app->board->isUsingBackgroundImage() ||
      app->board->getBackgroundImage() == nullptr) {
    return;
  }

  // Check surface status before attempting to draw
  cairo_status_t status =
      cairo_surface_status(app->board->getBackgroundImage());
  if (status != CAIRO_STATUS_SUCCESS) {
    std::cerr << "Invalid background image surface during opacity change: "
              << cairo_status_to_string(status) << std::endl;
    return;
  }

  app->board->setBackgroundOpacity(opacity);

  // Queue a redraw rather than forcing immediate redraw
  updateDisplay(app);
}

void onPauseGame(GtkMenuItem *menuItem, gpointer userData) {
  TetrimoneApp *app = static_cast<TetrimoneApp *>(userData);
  if (!app->board->isGameOver()) {
    bool isPaused = app->board->isPaused();
    app->board->togglePause();

    if (app->board->isPaused()) {
      pauseGame(app);
      ui_set_pause_menu_label(app, "Resume");
      gtk_widget_set_sensitive(app->startMenuItem, true);
    } else {
      startGame(app);
      ui_set_pause_menu_label(app, "Pause");

      gtk_widget_set_sensitive(app->startMenuItem, FALSE);
    }

    updateDisplay(app);
  }
}

void updateDisplay(TetrimoneApp *app) {
    gtk_widget_queue_draw(app->gameArea);
    gtk_widget_queue_draw(app->nextPieceArea);
}

void set_theme_menu(TetrimoneApp *app, int index)
{
    if (index < 0) {
        printf("DEBUG: Invalid theme index %d\n", index);
        return;
    }

    printf("DEBUG: Setting theme index to %d\n", index);
    gtk_check_menu_item_set_active(
        GTK_CHECK_MENU_ITEM(app->themeMenuItems[index]),
        true
    );
}


