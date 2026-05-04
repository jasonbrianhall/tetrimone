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



void applyCommandLineArgs(TetrimoneApp* app, const CommandLineArgs& args) {
    printf("DEBUG: Applying command line args...\n");
    printf("DEBUG: retroMode=%d, initialLevel=%d, minBlockSize=%d\n", 
           args.retroMode, args.initialLevel, args.minBlockSize);
    printf("DEBUG: difficulty=%d, blockSize=%d, gridWidth=%d, gridHeight=%d\n",
           args.difficulty, args.blockSize, args.gridWidth, args.gridHeight);
    printf("DEBUG: themeIndex=%d, soundEnabled=%d, ghostPiece=%d\n",
           args.themeIndex, args.soundEnabled, args.ghostPiece);
    
    // Apply grid dimensions first (before calculating block size)
    if (args.gridWidth != -1) {
        printf("DEBUG: Setting grid width to %d\n", args.gridWidth);
        GRID_WIDTH = args.gridWidth;
    }
    if (args.gridHeight != -1) {
        printf("DEBUG: Setting grid height to %d\n", args.gridHeight);
        GRID_HEIGHT = args.gridHeight;
    }
    
    // Apply block size (after grid dimensions are set)
    if (args.blockSize != -1) {
        printf("DEBUG: Setting block size to %d\n", args.blockSize);
        BLOCK_SIZE = args.blockSize;
    }
    
    // Apply difficulty
    if (args.difficulty != -1) {
        printf("DEBUG: Setting difficulty to %d\n", args.difficulty);
        app->difficulty = args.difficulty;
        // Update menu selection
        switch (args.difficulty) {
            case 0: 
                printf("DEBUG: Setting Zen difficulty\n");
                gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(app->zenMenuItem), TRUE); 
                break;
            case 1: 
                printf("DEBUG: Setting Easy difficulty\n");
                gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(app->easyMenuItem), TRUE); 
                break;
            case 2: 
                printf("DEBUG: Setting Medium difficulty\n");
                gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(app->mediumMenuItem), TRUE); 
                break;
            case 3: 
                printf("DEBUG: Setting Hard difficulty\n");
                gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(app->hardMenuItem), TRUE); 
                break;
            case 4: 
                printf("DEBUG: Setting Extreme difficulty\n");
                gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(app->extremeMenuItem), TRUE); 
                break;
            case 5: 
                printf("DEBUG: Setting Insane difficulty\n");
                gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(app->insaneMenuItem), TRUE); 
                break;
        }
    }
    
    // Apply theme
    if (args.themeIndex != -1) {
        printf("DEBUG: Setting theme index to %d\n", args.themeIndex);
        currentThemeIndex = args.themeIndex;
        gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(app->themeMenuItems[args.themeIndex]), TRUE);
    }
    
    // Apply board settings
    if (args.minBlockSize != -1) {
        printf("DEBUG: Setting min block size to %d\n", args.minBlockSize);
        app->board->setMinBlockSize(args.minBlockSize);
    }
    
    if (args.initialLevel != -1) {
        printf("DEBUG: Setting initial level to %d\n", args.initialLevel);
        app->board->initialLevel = args.initialLevel;
        app->board->setLevel(args.initialLevel); 
    }

    
    if (args.junkLinesPercentage != -1) {
        printf("DEBUG: Setting junk lines percentage to %d\n", args.junkLinesPercentage);
        app->board->junkLinesPercentage = args.junkLinesPercentage;
    }
    
    if (args.junkLinesPerLevel != -1) {
        printf("DEBUG: Setting junk lines per level to %d\n", args.junkLinesPerLevel);
        app->board->junkLinesPerLevel = args.junkLinesPerLevel;
    }
    
    // Apply display settings
    printf("DEBUG: Setting ghost piece to %d\n", args.ghostPiece);
    app->board->setGhostPieceEnabled(args.ghostPiece);
    
    printf("DEBUG: Setting grid lines to %d\n", args.gridLines);
    app->board->setShowGridLines(args.gridLines);
    
    printf("DEBUG: Setting simple blocks to %d\n", args.simpleBlocks);
    app->board->simpleBlocksActive = args.simpleBlocks;
    
    // Apply audio settings
    printf("DEBUG: Setting sound enabled to %d\n", args.soundEnabled);
    app->board->sound_enabled_ = args.soundEnabled;
    
    printf("DEBUG: Setting retro music to %d\n", args.retroMusic);
    app->board->retroMusicActive = args.retroMusic;
    
    // Update menu items to reflect settings
    gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(app->soundToggleMenuItem), args.soundEnabled);
    
    // Apply retro mode
    if (args.retroMode) {
        printf("DEBUG: Applying retro mode\n");
        app->board->retroModeActive = true;
        gtk_window_set_title(GTK_WINDOW(app->window), "БЛОЧНАЯ РЕВОЛЮЦИЯ");
        currentThemeIndex = NUM_COLOR_THEMES - 1; // Soviet Retro theme
        gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(app->themeMenuItems[currentThemeIndex]), TRUE);
        
        // In retro mode, disable backgrounds
        printf("DEBUG: Disabling backgrounds for retro mode\n");
        app->board->setUseBackgroundImage(false);
        app->board->setUseBackgroundZip(false);
        gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(app->backgroundToggleMenuItem), FALSE);
    }
    
    // Apply background settings
    if (!args.backgroundImage.empty()) {
        printf("DEBUG: Loading background image: %s\n", args.backgroundImage.c_str());
        if (app->board->loadBackgroundImage(args.backgroundImage)) {
            printf("DEBUG: Successfully loaded background image\n");
            app->board->setUseBackgroundImage(true);
            gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(app->backgroundToggleMenuItem), TRUE);
        } else {
            printf("DEBUG: Failed to load background image\n");
        }
    }
    
    if (!args.backgroundZip.empty()) {
        printf("DEBUG: Loading background ZIP: %s\n", args.backgroundZip.c_str());
        if (app->board->loadBackgroundImagesFromZip(args.backgroundZip)) {
            printf("DEBUG: Successfully loaded background ZIP\n");
            app->board->setUseBackgroundZip(true);
            gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(app->backgroundToggleMenuItem), TRUE);
        } else {
            printf("DEBUG: Failed to load background ZIP\n");
        }
    }
    
    if (args.backgroundOpacity != -1.0) {
        printf("DEBUG: Setting background opacity to %f\n", args.backgroundOpacity);
        app->board->setBackgroundOpacity(args.backgroundOpacity);
    }
    
    if (!args.soundZip.empty()) {
        printf("DEBUG: Setting sound ZIP path: %s\n", args.soundZip.c_str());
        app->board->setSoundsZipPath(args.soundZip);
    }
    
    // Apply fullscreen
    if (args.fullscreen) {
        printf("DEBUG: Setting fullscreen mode\n");
        gtk_window_fullscreen(GTK_WINDOW(app->window));
    }
    
    // Update labels to reflect new settings
    printf("DEBUG: Updating difficulty label\n");
    gtk_label_set_markup(GTK_LABEL(app->difficultyLabel),
                         app->board->getDifficultyText(app->difficulty).c_str());
                         
    printf("DEBUG: Command line args application complete\n");
}

int main(int argc, char *argv[]) {
    // Parse our arguments first and create filtered argc/argv for GTK
    CommandLineArgs args = parseCommandLine(argc, argv);
    
    // Handle help and version before GTK initialization
    if (args.help) {
        printHelp(argv[0]);
        return 0;
    }
    
    if (args.version) {
        printVersion();
        return 0;
    }
    
    // Create filtered argv with only GTK-compatible arguments
    std::vector<char*> gtkArgv;
    gtkArgv.push_back(argv[0]); // Always keep program name
    
    // Add any GTK-specific arguments you want to preserve
    // For now, just keep the program name
    int gtkArgc = gtkArgv.size();
    
#ifdef DEBUG
    freopen("debug_output.log", "w", stdout);
    freopen("debug_output.log", "a", stderr);
#endif

    GtkApplication *app = gtk_application_new("org.gtk.tetrimone", G_APPLICATION_DEFAULT_FLAGS);
    
    // Store command line args in the application data
    g_object_set_data(G_OBJECT(app), "cmdline-args", &args);
    
    g_signal_connect(app, "activate", G_CALLBACK(onAppActivate), NULL);
    
    // Pass filtered arguments to GTK
    int status = g_application_run(G_APPLICATION(app), gtkArgc, gtkArgv.data());
    g_object_unref(app);
    return status;
}

void drawBackground(cairo_t *cr, TetrimoneBoard *board, const GtkAllocation &allocation) {
  // Draw solid background color
  cairo_set_source_rgb(cr, 0.1, 0.1, 0.1);
  cairo_rectangle(cr, 0, 0, allocation.width, allocation.height);
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
        double scaleX = static_cast<double>(allocation.width) / imgWidth;
        double scaleY = static_cast<double>(allocation.height) / imgHeight;
        double scale = std::max(scaleX, scaleY);

        // Calculate position to center the image
        double x = (allocation.width - imgWidth * scale) / 2;
        double y = (allocation.height - imgHeight * scale) / 2;

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
        double scaleX = static_cast<double>(allocation.width) / imgWidth;
        double scaleY = static_cast<double>(allocation.height) / imgHeight;
        double scale = std::max(scaleX, scaleY);

        // Calculate position to center the image
        double x = (allocation.width - imgWidth * scale) / 2;
        double y = (allocation.height - imgHeight * scale) / 2;

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
      double scaleX = static_cast<double>(allocation.width) / imgWidth;
      double scaleY = static_cast<double>(allocation.height) / imgHeight;
      double scale = std::max(scaleX, scaleY);

      // Calculate position to center the image
      double x = (allocation.width - imgWidth * scale) / 2;
      double y = (allocation.height - imgHeight * scale) / 2;

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
            gtk_widget_queue_draw(app->gameArea);
          }
          
          // Draw freezy effect when cold
          if (heatLevel < 0.3f) {
            drawFreezyEffect(cr, drawX, drawY, drawSize, heatLevel, timeMs);
            gtk_widget_queue_draw(app->gameArea);
          }
        }   
      }
    }
  }
}
