#include <cstring>
#include <iostream>
#include "tetrimone.h"
#include "commandline.h"
#include "tetrimone.h"
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

void drawPlacedBlocks(cairo_t *cr, TetrimoneBoard *board, TetrimoneApp *app) {
  for (int y = 0; y < GRID_HEIGHT; ++y) {
    for (int x = 0; x < GRID_WIDTH; ++x) {
      int value = board->getGridValue(x, y);
      if (value > 0) {
        // Check if this line is being cleared
        double alpha = 1.0;
        double scale = 1.0;
        double offsetX = 0.0;
        double offsetY = 0.0;
        
        if (board->isLineClearActive() && board->isLineBeingCleared(y)) {
          // Get animation progress (0.0 to 1.0)
          double progress = board->getLineClearProgress();
          
          if (board->retroModeActive) {
            // Soviet-era computer animation: Simple scan line effect
            if (progress < 0.3) {
              // Horizontal scan line sweep from left to right
              double scanProgress = progress / 0.3;
              int scanX = (int)(scanProgress * GRID_WIDTH);
              
              // Only affect blocks that have been "scanned"
              if (x <= scanX) {
                alpha = 0.3 + 0.4 * sin(progress * 20.0); // Subtle flicker
              } else {
                alpha = 1.0; // Normal until scanned
              }
              scale = 1.0;
            } else if (progress < 0.7) {
              // All blocks flash in unison (like old CRT monitors)
              double flashProgress = (progress - 0.3) / 0.4;
              alpha = 1.0 - flashProgress * 0.7;
              
              // Simulate old monitor "collapse" effect - vertical compression
              scale = 1.0;
              offsetY = flashProgress * BLOCK_SIZE * 0.3; // Slight downward compression
            } else {
              // Final "wipe" effect - blocks disappear in chunks
              double wipeProgress = (progress - 0.7) / 0.3;
              
              // Divide line into segments that disappear sequentially
              int segment = x / 3; // 3-block segments
              double segmentDelay = segment * 0.2;
              
              if (wipeProgress > segmentDelay) {
                alpha = 0.0; // Instant disappear once segment is reached
                scale = 0.0;
              } else {
                alpha = 1.0 - wipeProgress * 0.5;
                scale = 1.0;
              }
            }
          } else {
            // Modern animations - 10 different types selected randomly
            int animationType = board->getCurrentAnimationType();
            LineClearAnimValues animValues = getLineClearAnimationValues(animationType, progress, x, y);
            alpha = animValues.alpha;
            scale = animValues.scale;
            offsetX = animValues.offsetX;
            offsetY = animValues.offsetY;
          }
        }

        // Get color from tetrimoneblock colors
        auto baseColor = board->isInThemeTransition() ? 
        board->getInterpolatedColor(value - 1, board->getThemeTransitionProgress()) :
        TETRIMONEBLOCK_COLOR_THEMES[currentThemeIndex][value - 1];
        auto color = getHeatModifiedColor(baseColor, board->getHeatLevel());

        cairo_set_source_rgba(cr, color[0], color[1], color[2], alpha);

        // Calculate position with animation offsets
        double drawX = x * BLOCK_SIZE + offsetX + (BLOCK_SIZE * (1.0 - scale)) / 2;
        double drawY = y * BLOCK_SIZE + offsetY + (BLOCK_SIZE * (1.0 - scale)) / 2;
        double drawSize = BLOCK_SIZE * scale;

        if (board->retroModeActive || board->simpleBlocksActive) {
          // Simple blocks
          cairo_rectangle(cr, drawX, drawY, drawSize, drawSize);
          cairo_fill(cr);
        } else {
          // 3D blocks with scaling
          cairo_rectangle(cr, drawX + 1, drawY + 1, drawSize - 2, drawSize - 2);
          cairo_fill(cr);

          // Draw highlight (3D effect)
          cairo_set_source_rgba(cr, 1, 1, 1, 0.3 * alpha);
          cairo_move_to(cr, drawX + 1, drawY + 1);
          cairo_line_to(cr, drawX + drawSize - 1, drawY + 1);
          cairo_line_to(cr, drawX + 1, drawY + drawSize - 1);
          cairo_close_path(cr);
          cairo_fill(cr);

          // Draw shadow (3D effect)
          cairo_set_source_rgba(cr, 0, 0, 0, 0.3 * alpha);
          cairo_move_to(cr, drawX + drawSize - 1, drawY + 1);
          cairo_line_to(cr, drawX + drawSize - 1, drawY + drawSize - 1);
          cairo_line_to(cr, drawX + 1, drawY + drawSize - 1);
          cairo_close_path(cr);
          cairo_fill(cr);
        }
        
        if (!board->retroModeActive) { // Only apply effects in modern mode
          float heatLevel = board->getHeatLevel();
          
          // Get current time for animation
          auto now = std::chrono::high_resolution_clock::now();
          auto timeMs = std::chrono::duration<double, std::milli>(
              now.time_since_epoch()).count();
          
          // Draw fiery glow effect when hot
          if (heatLevel > 0.7f) {
            drawFireyGlow(cr, drawX, drawY, drawSize, heatLevel, timeMs);
            updateDisplay(app);
          }
          
          // Draw freezy effect when cold
          if (heatLevel < 0.3f) {
            drawFreezyEffect(cr, drawX, drawY, drawSize, heatLevel, timeMs);
            updateDisplay(app);
          }
        }   
      }
    }
  }
}

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
            gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(app->backgroundToggleMenuItem), TRUE);
            
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

int ui_run_application(int argc, char *argv[], TetrimoneApp *app, const CommandLineArgs *args)
{
    GtkApplication *gtkApp =
        gtk_application_new("org.gtk.tetrimone", G_APPLICATION_DEFAULT_FLAGS);

    g_object_set_data(G_OBJECT(gtkApp), "tetrimone-app", app);
    g_object_set_data(G_OBJECT(gtkApp), "cmdline-args", (gpointer)args);

    g_signal_connect(gtkApp, "activate", G_CALLBACK(onAppActivate), NULL);

    int status = g_application_run(G_APPLICATION(gtkApp), argc, argv);
    g_object_unref(gtkApp);
    return status;
}



void drawBackground(cairo_t *cr, TetrimoneBoard *board, int width, int height) {
  // Draw solid background color
  cairo_set_source_rgb(cr, 0.1, 0.1, 0.1);
  cairo_rectangle(cr, 0, 0, width, height);
  cairo_fill(cr);

  // Draw background image if enabled
  if ((board->isUsingBackgroundImage() || board->isUsingBackgroundZip()) &&
      board->getBackgroundImage() != nullptr) {
    // Save the current state
    cairo_save(cr);

    // Check if we're in a transition
    if (board->isInBackgroundTransition()) {
      // If fading out, draw old background first
      if (board->getTransitionDirection() == -1 &&
          board->getOldBackground() != nullptr) {
        // Get the image dimensions
        int imgWidth = cairo_image_surface_get_width(board->getOldBackground());
        int imgHeight =
            cairo_image_surface_get_height(board->getOldBackground());

        // Calculate scaling to fill the game area while maintaining aspect
        // ratio
        double scaleX = static_cast<double>(width) / imgWidth;
        double scaleY = static_cast<double>(height) / imgHeight;
        double scale = std::max(scaleX, scaleY);

        // Calculate position to center the image
        double x = (width - imgWidth * scale) / 2;
        double y = (height - imgHeight * scale) / 2;

        // Apply the transformation
        cairo_translate(cr, x, y);
        cairo_scale(cr, scale, scale);

        // Draw the old image with current transition opacity
        cairo_set_source_surface(cr, board->getOldBackground(), 0, 0);
        cairo_paint_with_alpha(cr, board->getTransitionOpacity());

        // Reset transformation for next drawing
        cairo_restore(cr);
        cairo_save(cr);
      } else if (board->getTransitionDirection() == 1) {
        // Fading in - draw new background with transition opacity
        // Get the image dimensions
        int imgWidth =
            cairo_image_surface_get_width(board->getBackgroundImage());
        int imgHeight =
            cairo_image_surface_get_height(board->getBackgroundImage());

        // Calculate scaling to fill the game area while maintaining aspect
        // ratio
        double scaleX = static_cast<double>(width) / imgWidth;
        double scaleY = static_cast<double>(height) / imgHeight;
        double scale = std::max(scaleX, scaleY);

        // Calculate position to center the image
        double x = (width - imgWidth * scale) / 2;
        double y = (height - imgHeight * scale) / 2;

        // Apply the transformation
        cairo_translate(cr, x, y);
        cairo_scale(cr, scale, scale);

        // Draw the new image with transition opacity
        cairo_set_source_surface(cr, board->getBackgroundImage(), 0, 0);
        cairo_paint_with_alpha(cr, board->getTransitionOpacity());
      }
    } else {
      // Normal drawing (no transition)
      // Get the image dimensions
      int imgWidth = cairo_image_surface_get_width(board->getBackgroundImage());
      int imgHeight =
          cairo_image_surface_get_height(board->getBackgroundImage());

      // Calculate scaling to fill the game area while maintaining aspect ratio
      double scaleX = static_cast<double>(width) / imgWidth;
      double scaleY = static_cast<double>(height) / imgHeight;
      double scale = std::max(scaleX, scaleY);

      // Calculate position to center the image
      double x = (width - imgWidth * scale) / 2;
      double y = (height - imgHeight * scale) / 2;

      // Apply the transformation
      cairo_translate(cr, x, y);
      cairo_scale(cr, scale, scale);

      // Draw the image with normal opacity
      cairo_set_source_surface(cr, board->getBackgroundImage(), 0, 0);
      cairo_paint_with_alpha(cr, board->getBackgroundOpacity());
    }

    // Restore the original state
    cairo_restore(cr);
  }
}

gboolean onDrawGameArea(GtkWidget *widget, cairo_t *cr, gpointer data) {
  TetrimoneApp *app = static_cast<TetrimoneApp *>(data);
  TetrimoneBoard *board = app->board;

  // Get widget dimensions
  GtkAllocation allocation;
  gtk_widget_get_allocation(widget, &allocation);

  // Draw background
  drawBackground(cr, board, allocation.width, allocation.height);

  // Draw gridlines
  drawGridLines(cr, board);

  // Draw failure line
  drawFailureLine(cr);

  // Draw placed blocks with line clearing animation
  drawPlacedBlocks(cr, board, app);

  // Draw splash screen if active
  if (board->isSplashScreenActive()) {
    drawSplashScreen(cr, board, app);
    return FALSE;
  }

  // Draw propaganda messages
  drawPropagandaMessage(cr, board);

  // Draw current piece with smooth movement animation
  drawCurrentPiece(cr, board);

  // Draw ghost piece with interpolated position
  drawGhostPiece(cr, board);

  // Draw enhanced pause menu if paused
  drawPauseMenu(cr, board);

  // Draw game over text if needed
  drawGameOver(cr, board);


  if (board->isFireworksActive()) {
    drawFireworks(cr, board, app);
  }
 
  if (board->isTrailsEnabled() && board->isBlockTrailsActive()) {
    drawBlockTrails(cr, board);
  }
  
  return FALSE;
}

gboolean onDrawNextPiece(GtkWidget *widget, cairo_t *cr, gpointer data) {
  TetrimoneApp *app = static_cast<TetrimoneApp *>(data);
  TetrimoneBoard *board = app->board;

  // Get widget dimensions
  GtkAllocation allocation;
  gtk_widget_get_allocation(widget, &allocation);

  // Draw background
  cairo_set_source_rgb(cr, 0.1, 0.1, 0.1);
  cairo_rectangle(cr, 0, 0, allocation.width, allocation.height);
  cairo_fill(cr);

  if (!board->isGameOver()) {
    // Calculate section width - each piece gets exactly 1/3 of the total width
    int sectionWidth = allocation.width / 3;

    // Calculate preview block size (half of the normal block size)
    int previewBlockSize = BLOCK_SIZE / 2;

    // Draw dividers between sections
    cairo_set_source_rgb(cr, 0.3, 0.3, 0.3);
    cairo_set_line_width(cr, 2);
    cairo_move_to(cr, sectionWidth, 0);
    cairo_line_to(cr, sectionWidth, allocation.height);
    cairo_move_to(cr, sectionWidth * 2, 0);
    cairo_line_to(cr, sectionWidth * 2, allocation.height);
    cairo_stroke(cr);

    // Process each piece
    for (int pieceIndex = 0; pieceIndex < 3; pieceIndex++) {
      // Calculate the section's X position
      int sectionX = pieceIndex * sectionWidth;

      // Reserve space for the header
      int headerHeight = 25;

      // Get the piece information
      const TetrimoneBlock &piece = board->getNextPiece(pieceIndex);
      auto shape = piece.getShape();
      auto color = piece.getColor();

      // Calculate the shape dimensions in blocks
      int pieceWidth = 0;
      for (const auto &row : shape) {
        pieceWidth = std::max(pieceWidth, (int)row.size());
      }
      int pieceHeight = shape.size();

      // Center the piece in the available space (using the preview block size)
      int availableWidth = sectionWidth;
      int offsetX =
          sectionX + (availableWidth - pieceWidth * previewBlockSize) / 2;
      int offsetY = headerHeight + (allocation.height - headerHeight -
                                    pieceHeight * previewBlockSize) /
                                       2;

      // Draw piece number label at the top of each section
      cairo_set_source_rgb(cr, 0.8, 0.8, 0.8);
      cairo_select_font_face(cr, "Sans", CAIRO_FONT_SLANT_NORMAL,
                             CAIRO_FONT_WEIGHT_BOLD);

      // Scale font size with block size
      double fontSize = std::max(10.0, std::min(16.0, previewBlockSize * 0.5));
      cairo_set_font_size(cr, fontSize);

      // Use a shorter label to avoid width issues
      char pieceLabel[10];
      snprintf(pieceLabel, sizeof(pieceLabel), "%d", pieceIndex + 1);

      cairo_text_extents_t extents;
      cairo_text_extents(cr, pieceLabel, &extents);

      // Center the text in the section
      double textX = sectionX + (sectionWidth - extents.width) / 2;
      cairo_move_to(cr, textX, headerHeight - 5);
      cairo_show_text(cr, pieceLabel);

      // Set color for drawing
      cairo_set_source_rgb(cr, color[0], color[1], color[2]);

      // Draw the piece blocks with half size
      for (size_t y = 0; y < shape.size(); ++y) {
        for (size_t x = 0; x < shape[y].size(); ++x) {
          if (shape[y][x] == 1) {
            int drawX = offsetX + x * previewBlockSize;
            int drawY = offsetY + y * previewBlockSize;
     if (board->retroModeActive || board->simpleBlocksActive) {
        // In retro mode, draw simple blocks without 3D effects
        cairo_rectangle(cr, drawX, drawY, previewBlockSize, previewBlockSize);
        cairo_fill(cr);
      } else {
            // Draw block with a small margin
            cairo_rectangle(cr, drawX + 1, drawY + 1, previewBlockSize - 2,
                            previewBlockSize - 2);
            cairo_fill(cr);

            // Draw highlight (3D effect)
            cairo_set_source_rgba(cr, 1, 1, 1, 0.3);
            cairo_move_to(cr, drawX + 1, drawY + 1);
            cairo_line_to(cr, drawX + previewBlockSize - 1, drawY + 1);
            cairo_line_to(cr, drawX + 1, drawY + previewBlockSize - 1);
            cairo_close_path(cr);
            cairo_fill(cr);

            // Draw shadow (3D effect)
            cairo_set_source_rgba(cr, 0, 0, 0, 0.3);
            cairo_move_to(cr, drawX + previewBlockSize - 1, drawY + 1);
            cairo_line_to(cr, drawX + previewBlockSize - 1,
                          drawY + previewBlockSize - 1);
            cairo_line_to(cr, drawX + 1, drawY + previewBlockSize - 1);
            cairo_close_path(cr);
            cairo_fill(cr);

            // Reset color for next block
            cairo_set_source_rgb(cr, color[0], color[1], color[2]);
}
          }
        }
      }
    }
  }

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
    gtk_file_chooser_set_select_multiple(GTK_FILE_CHOOSER(dialog), TRUE);
    
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
            gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(app->backgroundToggleMenuItem), TRUE);
            
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
      gtk_widget_set_sensitive(app->startMenuItem, TRUE);
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

void drawBoard(TetrimoneBoard *board) {
     gtk_widget_queue_draw(board->app->gameArea);
}

void drawNextPieceArea(TetrimoneBoard *board) {
     gtk_widget_queue_draw(board->app->nextPieceArea); // For theme color changes
}

void set_difficulty_menu(TetrimoneApp *app, int difficulty)
{
    GtkWidget *items[] = {
        app->zenMenuItem,
        app->easyMenuItem,
        app->mediumMenuItem,
        app->hardMenuItem,
        app->extremeMenuItem,
        app->insaneMenuItem
    };

    const char *labels[] = {
        "Zen", "Easy", "Medium", "Hard", "Extreme", "Insane"
    };

    if (difficulty < 0 || difficulty >= G_N_ELEMENTS(items)) {
        printf("DEBUG: Invalid difficulty %d\n", difficulty);
        return;
    }

    printf("DEBUG: Setting %s difficulty\n", labels[difficulty]);
    gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(items[difficulty]), TRUE);
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
        TRUE
    );
}

void ui_set_sound_enabled(TetrimoneApp *app, bool enabled)
{
    gtk_check_menu_item_set_active(
        GTK_CHECK_MENU_ITEM(app->soundToggleMenuItem),
        enabled
    );
}

void ui_set_active_theme(TetrimoneApp *app, int index)
{
    gtk_check_menu_item_set_active(
        GTK_CHECK_MENU_ITEM(app->themeMenuItems[index]),
        TRUE
    );
}

void ui_set_background_enabled(TetrimoneApp *app, bool enabled)
{
    gtk_check_menu_item_set_active(
        GTK_CHECK_MENU_ITEM(app->backgroundToggleMenuItem),
        enabled
    );
}

void ui_set_window_title(TetrimoneApp *app, const char *title)
{
    gtk_window_set_title(GTK_WINDOW(app->window), title);
}

void ui_window_fullscreen(TetrimoneApp *app)
{
    gtk_window_fullscreen(GTK_WINDOW(app->window));
}

void ui_set_difficulty_label(TetrimoneApp *app, const char *markup)
{
    gtk_label_set_markup(GTK_LABEL(app->difficultyLabel), markup);
}

void ui_set_pause_menu_label(TetrimoneApp *app, const char *text)
{
    gtk_menu_item_set_label(GTK_MENU_ITEM(app->pauseMenuItem), text);
}

void ui_update_track_menu(TetrimoneApp *app)
{
    for (int i = 0; i < 5; i++) {
        gtk_check_menu_item_set_active(
            GTK_CHECK_MENU_ITEM(app->trackMenuItems[i]),
            app->board->enabledTracks[i]
        );
    }
}

