#include "tetrimone.h"

typedef struct {
    TetrimoneApp* app;
    GtkTextBuffer* buffer;
} JoystickTestData;

gboolean updateJoystickTestDisplay(gpointer userData);

void initSDL(TetrimoneApp *app) {
  // Make sure SDL is not already initialized
  if (app->joystickEnabled) {
    shutdownSDL(app);
  }

  // Initialize SDL joystick subsystem
  if (SDL_Init(SDL_INIT_JOYSTICK) < 0) {
    printf("SDL could not initialize! SDL Error: %s\n", SDL_GetError());
    app->joystick = NULL;
    app->joystickEnabled = false;
    app->joystickTimerId = 0;
    return;
  }

  // SDL initialized successfully
  app->joystickEnabled = true;

  // Check for joysticks
  int numJoysticks = SDL_NumJoysticks();
  if (numJoysticks < 1) {
    printf("No joysticks connected!\n");
    // We keep SDL initialized but don't open any joystick
    app->joystick = NULL;
    app->joystickTimerId = 0;
    return;
  }

  printf("Found %d joystick(s)\n", numJoysticks);
  
  // Open the first joystick by default
  app->joystick = SDL_JoystickOpen(0);
  if (app->joystick == NULL) {
    printf("Unable to open joystick! SDL Error: %s\n", SDL_GetError());
    // We keep SDL initialized but don't have an active joystick
    app->joystickTimerId = 0;
    return;
  }

  printf("Joystick connected: %s\n", SDL_JoystickName(app->joystick));
  printf("Number of buttons: %d\n", SDL_JoystickNumButtons(app->joystick));
  printf("Number of axes: %d\n", SDL_JoystickNumAxes(app->joystick));
  printf("Number of hats: %d\n", SDL_JoystickNumHats(app->joystick));

app->joystickMapping = {
    0,      // rotate_cw_button: A button (0)
    1,      // rotate_ccw_button: B button (1)
    3,      // hard_drop_button: Y button (3)
    9,      // pause_button: Start button (9)
    0,      // x_axis: First axis (0)
    1,      // y_axis: Second axis (1)
    false,  // invert_x: Don't invert X
    false   // invert_y: Don't invert Y
};

  // Start the joystick polling timer only if it's not already running
  if (app->joystickTimerId == 0) {
    app->joystickTimerId = g_timeout_add(16, pollJoystick, app);
  }
}

// Shutdown SDL
void shutdownSDL(TetrimoneApp *app) {
  // Stop the joystick polling timer
  if (app->joystickTimerId > 0) {
    g_source_remove(app->joystickTimerId);
    app->joystickTimerId = 0;
  }

  // Close the joystick
  if (app->joystick != NULL) {
    SDL_JoystickClose(app->joystick);
    app->joystick = NULL;
  }

  // Quit SDL completely
  SDL_Quit();
  app->joystickEnabled = false;
}

typedef struct {
  bool active;
  int direction;
  Uint32 lastMoveTime;
  Uint32 repeatDelay;
  int moveCount;
} DirectionalControl;

// Add these constants to the top of joystick.cpp

// Configuration for joystick sensitivity
const int DEFAULT_DEADZONE = 8000;  // Default deadzone value (about 25% of max)
const int MAX_JOYSTICK_BUTTONS = 16; // Maximum number of buttons to check
const Uint32 BUTTON_DEBOUNCE_TIME = 200; // ms between button actions

gboolean pollJoystick(gpointer data) {
  TetrimoneApp *app = static_cast<TetrimoneApp *>(data);

  // Safety checks
  if (!app || !app->joystickEnabled || !app->joystick || !app->board) {
    return FALSE;
  }

  // Get current joystick state
  SDL_JoystickUpdate();

  // Get current time
  Uint32 currentTime = SDL_GetTicks();

  // Static variables for tracking state
  static Uint32 lastButtonPressTime = 0;

  // Directional controls with acceleration
  static DirectionalControl horizontalControl = {false, 0, 0, 150, 0};
  static DirectionalControl verticalControl = {false, 0, 0, 150, 0};
  static DirectionalControl dpadHorizontal = {false, 0, 0, 150, 0};
  static DirectionalControl dpadVertical = {false, 0, 0, 150, 0};

  // Handle button presses even when game is paused or over
  // Process buttons with debounce
  int numButtons = SDL_JoystickNumButtons(app->joystick);
  
  // Clamp number of buttons to avoid out-of-bounds access
  if (numButtons > MAX_JOYSTICK_BUTTONS)
    numButtons = MAX_JOYSTICK_BUTTONS;

  for (int i = 0; i < numButtons; i++) {
    if (SDL_JoystickGetButton(app->joystick, i)) {
      // Only process button if enough time has passed since last press
      if (currentTime - lastButtonPressTime > BUTTON_DEBOUNCE_TIME) {
        bool buttonProcessed = true;

        // Use the custom mapping for pause/start
        if (i == app->joystickMapping.pause_button) {
          if (app->board->isSplashScreenActive()) {
            // If splash screen is active, dismiss it and start game
            app->board->dismissSplashScreen();
            startGame(app);
            gtk_widget_queue_draw(app->gameArea);
            gtk_widget_queue_draw(app->nextPieceArea);
            updateLabels(app);
          } else if (app->board->isGameOver()) {
            // If game is over, restart
            onRestartGame(GTK_MENU_ITEM(app->restartMenuItem), app);
          } else {
            // If game is running or paused, toggle pause state
            onPauseGame(GTK_MENU_ITEM(app->pauseMenuItem), app);
          }
          buttonProcessed = true;
        } else if (!app->board->isGameOver() && !app->board->isPaused()) {
          // Use custom mapping for game controls
          if (i == app->joystickMapping.rotate_cw_button) {
            // Rotate clockwise
            app->board->rotatePiece(true);
            buttonProcessed = true;
          } else if (i == app->joystickMapping.rotate_ccw_button) {
            // Counter-rotate
            app->board->rotatePiece(false);
            buttonProcessed = true;
          } else if (i == app->joystickMapping.hard_drop_button) {
            // Hard drop
            app->board->hardDrop();
            buttonProcessed = true;
          } else {
            // Legacy mappings for backward compatibility
            switch (i) {
            case 2: // X button - alternate rotate button on some controllers
              app->board->rotatePiece(true);
              buttonProcessed = true;
              break;

            case 6: // LB/L1 button - hard drop (additional option)
            case 10: // RB/R1 button - hard drop (additional option)
              app->board->hardDrop();
              buttonProcessed = true;
              break;

            default:
              buttonProcessed = false;
              break;
            }
          }
        } else {
          buttonProcessed = false;
        }

        // Update the last button press time if we processed a button
        if (buttonProcessed) {
          lastButtonPressTime = currentTime;
        }
      }
    }
  }

  // Skip further input processing if game is over or paused
  if (app->board->isGameOver() || app->board->isPaused()) {
    return TRUE; // Keep the timer going but don't process movement inputs
  }

  // Process axes with acceleration using custom mapping
  if (SDL_JoystickNumAxes(app->joystick) >= 2) {
    // Read X axis (horizontal movement) using the mapped axis
    int xValue = 0;
    if (SDL_JoystickNumAxes(app->joystick) > app->joystickMapping.x_axis) {
      xValue = SDL_JoystickGetAxis(app->joystick, app->joystickMapping.x_axis);
      
      // Apply inversion if configured
      if (app->joystickMapping.invert_x) {
        xValue = -xValue;
      }
    }
    
    int newDir = 0;
    if (xValue < -DEFAULT_DEADZONE) {
      newDir = -1; // Left
    } else if (xValue > DEFAULT_DEADZONE) {
      newDir = 1; // Right
    }

    // Handle horizontal movement with acceleration
    if (newDir != 0) {
      // If direction changed or movement just started
      if (!horizontalControl.active || horizontalControl.direction != newDir) {
        horizontalControl.active = true;
        horizontalControl.direction = newDir;
        horizontalControl.lastMoveTime = currentTime;
        horizontalControl.repeatDelay = 150; // Initial delay
        horizontalControl.moveCount = 0;

        // Immediate move
        app->board->movePiece(newDir, 0);
      }
      // Accelerating repeat moves
      else if (currentTime - horizontalControl.lastMoveTime >
               horizontalControl.repeatDelay) {
        app->board->movePiece(newDir, 0);
        horizontalControl.lastMoveTime = currentTime;
        horizontalControl.moveCount++;

        // Gradually decrease delay for acceleration effect
        if (horizontalControl.moveCount > 6) {
          horizontalControl.repeatDelay = 30; // Very fast
        } else if (horizontalControl.moveCount > 4) {
          horizontalControl.repeatDelay = 50; // Fast
        } else if (horizontalControl.moveCount > 2) {
          horizontalControl.repeatDelay = 100; // Medium
        }
      }
    } else {
      // No direction - reset horizontal control
      horizontalControl.active = false;
    }

    // Read Y axis (vertical movement) using the mapped axis
    int yValue = 0;
    if (SDL_JoystickNumAxes(app->joystick) > app->joystickMapping.y_axis) {
      yValue = SDL_JoystickGetAxis(app->joystick, app->joystickMapping.y_axis);
      
      // Apply inversion if configured
      if (app->joystickMapping.invert_y) {
        yValue = -yValue;
      }
    }
    
    int newDirY = 0;
    if (yValue > DEFAULT_DEADZONE) {
      newDirY = 1; // Down
    } else if (yValue < -DEFAULT_DEADZONE) {
      // Up is for rotation, handle separately
      if (!verticalControl.active || verticalControl.direction != -1) {
        // Only rotate once when first pushing up
        app->board->rotatePiece(true);
        verticalControl.active = true;
        verticalControl.direction = -1;
      }
    }

    // Handle vertical movement (down) with acceleration
    if (newDirY == 1) {
      // If downward movement just started
      if (!verticalControl.active || verticalControl.direction != newDirY) {
        verticalControl.active = true;
        verticalControl.direction = newDirY;
        verticalControl.lastMoveTime = currentTime;
        verticalControl.repeatDelay = 150; // Initial delay
        verticalControl.moveCount = 0;

        // Immediate move
        app->board->movePiece(0, 1);
      }
      // Accelerating repeat moves
      else if (currentTime - verticalControl.lastMoveTime >
               verticalControl.repeatDelay) {
        app->board->movePiece(0, 1);
        verticalControl.lastMoveTime = currentTime;
        verticalControl.moveCount++;

        // Gradually decrease delay for acceleration effect
        if (verticalControl.moveCount > 6) {
          verticalControl.repeatDelay = 20; // Very fast
        } else if (verticalControl.moveCount > 4) {
          verticalControl.repeatDelay = 30; // Fast
        } else if (verticalControl.moveCount > 2) {
          verticalControl.repeatDelay = 60; // Medium
        }
      }
    } else if (newDirY == 0 && yValue > -DEFAULT_DEADZONE) {
      // No downward direction - reset vertical control
      verticalControl.active = false;
    }
    
    // Handle right analog stick if available (usually axes 2 and 3)
    if (SDL_JoystickNumAxes(app->joystick) >= 4) {
      int rxValue = SDL_JoystickGetAxis(app->joystick, 2);
      int ryValue = SDL_JoystickGetAxis(app->joystick, 3);
      
      // Right stick rotation control (more intuitive for some players)
      if (abs(rxValue) > DEFAULT_DEADZONE || abs(ryValue) > DEFAULT_DEADZONE) {
        // Calculate the angle of the stick
        float angle = atan2f(ryValue, rxValue) * 180.0f / G_PI;
        
        // Convert to a 0-360 range
        if (angle < 0) angle += 360.0f;
        
        // Determine rotation direction based on stick movement
        // Right = clockwise, Left = counter-clockwise
        static Uint32 lastRotateTime = 0;
        if (currentTime - lastRotateTime > 250) { // Prevent too rapid rotation
          if (rxValue > DEFAULT_DEADZONE && abs(ryValue) < DEFAULT_DEADZONE) {
            // Right
            app->board->rotatePiece(true);
            lastRotateTime = currentTime;
          } else if (rxValue < -DEFAULT_DEADZONE && abs(ryValue) < DEFAULT_DEADZONE) {
            // Left
            app->board->rotatePiece(false);
            lastRotateTime = currentTime;
          }
        }
      }
    }
  }

  // Process D-pad hat if present with acceleration
  int numHats = SDL_JoystickNumHats(app->joystick);
  for (int i = 0; i < numHats; i++) {
    Uint8 hatState = SDL_JoystickGetHat(app->joystick, i);

    // Horizontal D-pad
    int dpadX = 0;
    if (hatState & SDL_HAT_LEFT)
      dpadX = -1;
    else if (hatState & SDL_HAT_RIGHT)
      dpadX = 1;

    if (dpadX != 0) {
      // If direction changed or movement just started
      if (!dpadHorizontal.active || dpadHorizontal.direction != dpadX) {
        dpadHorizontal.active = true;
        dpadHorizontal.direction = dpadX;
        dpadHorizontal.lastMoveTime = currentTime;
        dpadHorizontal.repeatDelay = 150; // Initial delay
        dpadHorizontal.moveCount = 0;

        // Immediate move
        app->board->movePiece(dpadX, 0);
      }
      // Accelerating repeat moves
      else if (currentTime - dpadHorizontal.lastMoveTime >
               dpadHorizontal.repeatDelay) {
        app->board->movePiece(dpadX, 0);
        dpadHorizontal.lastMoveTime = currentTime;
        dpadHorizontal.moveCount++;

        // Gradually decrease delay for acceleration effect
        if (dpadHorizontal.moveCount > 6) {
          dpadHorizontal.repeatDelay = 30; // Very fast
        } else if (dpadHorizontal.moveCount > 4) {
          dpadHorizontal.repeatDelay = 50; // Fast
        } else if (dpadHorizontal.moveCount > 2) {
          dpadHorizontal.repeatDelay = 100; // Medium
        }
      }
    } else {
      // No direction - reset horizontal control
      dpadHorizontal.active = false;
    }

    // Vertical D-pad
    int dpadY = 0;
    if (hatState & SDL_HAT_UP) {
      // Up is for rotation, handle separately
      if (!dpadVertical.active || dpadVertical.direction != -1) {
        // Only rotate once when first pushing up
        app->board->rotatePiece(true);
        dpadVertical.active = true;
        dpadVertical.direction = -1;
        dpadVertical.lastMoveTime = currentTime;
      }
    } else if (hatState & SDL_HAT_DOWN)
      dpadY = 1;

    if (dpadY == 1) {
      // If downward movement just started
      if (!dpadVertical.active || dpadVertical.direction != dpadY) {
        dpadVertical.active = true;
        dpadVertical.direction = dpadY;
        dpadVertical.lastMoveTime = currentTime;
        dpadVertical.repeatDelay = 150; // Initial delay
        dpadVertical.moveCount = 0;

        // Immediate move
        app->board->movePiece(0, 1);
      }
      // Accelerating repeat moves
      else if (currentTime - dpadVertical.lastMoveTime >
               dpadVertical.repeatDelay) {
        app->board->movePiece(0, 1);
        dpadVertical.lastMoveTime = currentTime;
        dpadVertical.moveCount++;

        // Gradually decrease delay for acceleration effect
        if (dpadVertical.moveCount > 6) {
          dpadVertical.repeatDelay = 20; // Very fast
        } else if (dpadVertical.moveCount > 4) {
          dpadVertical.repeatDelay = 30; // Fast
        } else if (dpadVertical.moveCount > 2) {
          dpadVertical.repeatDelay = 60; // Medium
        }
      }
    } else if (dpadY == 0 && !(hatState & SDL_HAT_UP)) {
      // No downward direction - reset vertical control
      dpadVertical.active = false;
    }
  }

  // Handle trigger buttons (usually axes 4 and 5 on many controllers)
  if (SDL_JoystickNumAxes(app->joystick) >= 6) {
    // Left trigger (axis 4) - soft drop
    int leftTrigger = SDL_JoystickGetAxis(app->joystick, 4);
    if (leftTrigger > DEFAULT_DEADZONE) {
      // Map trigger value to drop speed
      float triggerValue = (float)leftTrigger / 32767.0f;
      
      // Use trigger acceleration based on how far it's pressed
      static Uint32 lastTriggerMoveTime = 0;
      int triggerDelay = 150 - (int)(120 * triggerValue); // From 150ms to 30ms
      
      if (currentTime - lastTriggerMoveTime > triggerDelay) {
        app->board->movePiece(0, 1);
        lastTriggerMoveTime = currentTime;
      }
    }
    
    // Right trigger (axis 5) - hard drop
    int rightTrigger = SDL_JoystickGetAxis(app->joystick, 5);
    if (rightTrigger > DEFAULT_DEADZONE) {
      static Uint32 lastHardDropTime = 0;
      if (currentTime - lastHardDropTime > 500) { // Prevent accidental double drops
        app->board->hardDrop();
        lastHardDropTime = currentTime;
      }
    }
  }

  // Update display
  gtk_widget_queue_draw(app->gameArea);
  gtk_widget_queue_draw(app->nextPieceArea);
  updateLabels(app);

  return TRUE; // Keep the timer going
}


// Function to update joystick info in the config dialog
void updateJoystickInfo(GtkLabel* infoLabel, TetrimoneApp* app) {
    std::string info;
    
    if (!app->joystickEnabled) {
        info = "Joystick support is not enabled.";
    } else {
        int numJoysticks = SDL_NumJoysticks();
        
        if (numJoysticks <= 0) {
            info = "No joysticks detected.";
        } else {
            info = "Detected " + std::to_string(numJoysticks) + " joystick(s):\n";
            
            for (int i = 0; i < numJoysticks; i++) {
                info += "  " + std::to_string(i) + ": " + SDL_JoystickName(SDL_JoystickOpen(i)) + "\n";
                SDL_JoystickClose(SDL_JoystickOpen(i)); // Close temp instance
            }
            
            info += "\nCurrent joystick: ";
            if (app->joystick != NULL) {
                info += SDL_JoystickName(app->joystick);
                info += "\nButtons: " + std::to_string(SDL_JoystickNumButtons(app->joystick));
                info += "\nAxes: " + std::to_string(SDL_JoystickNumAxes(app->joystick));
                info += "\nHats: " + std::to_string(SDL_JoystickNumHats(app->joystick));
            } else {
                info += "None (not opened)";
            }
        }
    }
    
    gtk_label_set_text(infoLabel, info.c_str());
}

void onJoystickTestButton(GtkButton* button, gpointer userData) {
    TetrimoneApp* app = static_cast<TetrimoneApp*>(userData);
    
    // Create a new dialog window
    GtkWidget* testDialog = gtk_dialog_new_with_buttons(
        "Joystick Test",
        GTK_WINDOW(gtk_widget_get_toplevel(GTK_WIDGET(button))),
        GTK_DIALOG_MODAL,
        "_Close", GTK_RESPONSE_CLOSE,
        NULL
    );
    
    // Set size
    gtk_window_set_default_size(GTK_WINDOW(testDialog), 600, 400);
    
    // Get content area
    GtkWidget* contentArea = gtk_dialog_get_content_area(GTK_DIALOG(testDialog));
    gtk_container_set_border_width(GTK_CONTAINER(contentArea), 10);
    
    // Create a text view for displaying joystick input
    GtkWidget* textScroll = gtk_scrolled_window_new(NULL, NULL);
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(textScroll),
                                 GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
    gtk_box_pack_start(GTK_BOX(contentArea), textScroll, TRUE, TRUE, 0);
    
    GtkWidget* textView = gtk_text_view_new();
    gtk_text_view_set_editable(GTK_TEXT_VIEW(textView), FALSE);
    gtk_container_add(GTK_CONTAINER(textScroll), textView);
    
    // Set monospace font
    gtk_text_view_set_monospace(GTK_TEXT_VIEW(textView), TRUE);
    
    // Get the buffer
    GtkTextBuffer* buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(textView));
    
    // Add instructions
    GtkWidget* instructionLabel = gtk_label_new("Move the joystick and press buttons to see the input values.");
    gtk_box_pack_start(GTK_BOX(contentArea), instructionLabel, FALSE, FALSE, 10);
    
    // Set up data for the joystick test timer callback
    JoystickTestData* testData = new JoystickTestData;
    testData->app = app;
    testData->buffer = buffer;
    
    // Set up a timer to update the text view with joystick info
    guint testTimerId = g_timeout_add_full(G_PRIORITY_DEFAULT, 100, 
                                          updateJoystickTestDisplay, 
                                          testData, 
                                          [](gpointer data) { delete static_cast<JoystickTestData*>(data); });
    
    // Show dialog
    gtk_widget_show_all(testDialog);
    
    // Run dialog
    gtk_dialog_run(GTK_DIALOG(testDialog));
    
    // Clean up timer when dialog is closed
    g_source_remove(testTimerId);
    
    // Destroy dialog
    gtk_widget_destroy(testDialog);
}

// Function to handle joystick rescan button
void onJoystickRescan(GtkButton* button, gpointer userData) {
    TetrimoneApp* app = static_cast<TetrimoneApp*>(userData);
    GtkLabel* infoLabel = GTK_LABEL(g_object_get_data(G_OBJECT(button), "info-label"));
    
    // Shutdown SDL to reset joystick subsystem
    shutdownSDL(app);
    
    // Re-initialize SDL
    initSDL(app);
    
    // Update info
    updateJoystickInfo(infoLabel, app);
    
    // Show a message with results
    GtkWidget* dialog = gtk_message_dialog_new(
        GTK_WINDOW(gtk_widget_get_toplevel(GTK_WIDGET(button))),
        GTK_DIALOG_MODAL,
        GTK_MESSAGE_INFO,
        GTK_BUTTONS_OK,
        "Joystick rescan complete.\nFound %d joystick(s).",
        SDL_NumJoysticks()
    );
    
    gtk_dialog_run(GTK_DIALOG(dialog));
    gtk_widget_destroy(dialog);
}

// Structure to store data needed for the joystick selection callback
typedef struct {
    TetrimoneApp* app;
    GtkWidget* idScale;
    GtkWidget* infoLabel;
    GtkWidget* dialog;
} JoystickSelectionData;

// Callback for Apply Joystick Selection button
void onJoystickSelectionApply(GtkButton* button, gpointer userData) {
    JoystickSelectionData* data = static_cast<JoystickSelectionData*>(userData);
    TetrimoneApp* app = data->app;
    GtkWidget* idScale = data->idScale;
    GtkWidget* infoLabel = data->infoLabel;
    GtkWidget* dialog = data->dialog;
    
    // Get the selected joystick ID
    int joystickId = (int)gtk_range_get_value(GTK_RANGE(idScale));
    
    // Close current joystick if open
    if (app->joystick != NULL) {
        SDL_JoystickClose(app->joystick);
        app->joystick = NULL;
    }
    
    // Try to open the selected joystick
    if (joystickId >= 0 && joystickId < SDL_NumJoysticks()) {
        app->joystick = SDL_JoystickOpen(joystickId);
        if (app->joystick != NULL) {
            app->joystickEnabled = true;
            
            // Start the joystick polling timer if it's not already running
            if (app->joystickTimerId == 0) {
                app->joystickTimerId = g_timeout_add(16, pollJoystick, app);
            }
            
            // Show success message
            GtkWidget* successDialog = gtk_message_dialog_new(
                GTK_WINDOW(dialog),
                GTK_DIALOG_MODAL,
                GTK_MESSAGE_INFO,
                GTK_BUTTONS_OK,
                "Successfully opened joystick: %s",
                SDL_JoystickName(app->joystick)
            );
            gtk_dialog_run(GTK_DIALOG(successDialog));
            gtk_widget_destroy(successDialog);
        } else {
            // Show error message
            GtkWidget* errorDialog = gtk_message_dialog_new(
                GTK_WINDOW(dialog),
                GTK_DIALOG_MODAL,
                GTK_MESSAGE_ERROR,
                GTK_BUTTONS_OK,
                "Failed to open joystick: %s",
                SDL_GetError()
            );
            gtk_dialog_run(GTK_DIALOG(errorDialog));
            gtk_widget_destroy(errorDialog);
        }
    }
    
    // Update joystick info
    updateJoystickInfo(GTK_LABEL(infoLabel), app);
}

gboolean updateJoystickTestDisplay(gpointer userData) {
    JoystickTestData* data = static_cast<JoystickTestData*>(userData);
    GtkTextBuffer* buffer = data->buffer;
    TetrimoneApp* app = data->app;
    
    if (!app->joystickEnabled || !app->joystick) {
        return TRUE; // Keep the timer going
    }
    
    // Update joystick state
    SDL_JoystickUpdate();
    
    // Build a string with joystick information
    std::string info = "Joystick: " + std::string(SDL_JoystickName(app->joystick)) + "\n\n";
    
    // Buttons - display in three columns
    info += "Buttons:\n";
    int numButtons = SDL_JoystickNumButtons(app->joystick);
    
    // Calculate how many buttons per column (rounded up)
    int buttonsPerColumn = (numButtons + 2) / 3;
    
    // Create a temporary vector to hold button state strings
    std::vector<std::string> buttonStrings;
    for (int i = 0; i < numButtons; i++) {
        bool pressed = SDL_JoystickGetButton(app->joystick, i);
        buttonStrings.push_back("Button " + std::to_string(i) + ": " + 
                              (pressed ? "PRESSED" : "released"));
    }
    
    // Output buttons in columns
    for (int row = 0; row < buttonsPerColumn; row++) {
        std::string line = "";
        
        // Column 1
        if (row < buttonStrings.size()) {
            line += buttonStrings[row];
            // Pad to fixed width
            line += std::string(25 - buttonStrings[row].length(), ' ');
        }
        
        // Column 2
        int col2Index = row + buttonsPerColumn;
        if (col2Index < buttonStrings.size()) {
            line += buttonStrings[col2Index];
            // Pad to fixed width
            line += std::string(25 - buttonStrings[col2Index].length(), ' ');
        }
        
        // Column 3
        int col3Index = row + (2 * buttonsPerColumn);
        if (col3Index < buttonStrings.size()) {
            line += buttonStrings[col3Index];
        }
        
        info += line + "\n";
    }
    
    // Axes
    info += "\nAxes:\n";
    int numAxes = SDL_JoystickNumAxes(app->joystick);
    for (int i = 0; i < numAxes; i++) {
        int value = SDL_JoystickGetAxis(app->joystick, i);
        float normalizedValue = value / 32768.0f;
        info += "Axis " + std::to_string(i) + ": " + std::to_string(value) + 
                " (" + std::to_string(normalizedValue) + ")\n";
    }
    
    // Hats
    info += "\nHats:\n";
    int numHats = SDL_JoystickNumHats(app->joystick);
    for (int i = 0; i < numHats; i++) {
        Uint8 value = SDL_JoystickGetHat(app->joystick, i);
        std::string position = "centered";
        if (value & SDL_HAT_UP) position = "up";
        if (value & SDL_HAT_RIGHT) position += value & SDL_HAT_UP ? "-right" : "right";
        if (value & SDL_HAT_DOWN) position = "down";
        if (value & SDL_HAT_LEFT) position += value & SDL_HAT_DOWN ? "-left" : "left";
        
        info += "Hat " + std::to_string(i) + ": " + position + "\n";
    }
    
    // Update the text buffer
    gtk_text_buffer_set_text(buffer, info.c_str(), -1);
    
    return TRUE; // Keep the timer going
}


void onJoystickConfig(GtkMenuItem* menuItem, gpointer userData) {
    TetrimoneApp* app = static_cast<TetrimoneApp*>(userData);
    
    (void)menuItem; // Avoid unused parameter warning
    
    // Pause the game if it's running
    bool wasPaused = app->board->isPaused();
    if (!wasPaused && !app->board->isGameOver() && !app->board->isSplashScreenActive()) {
        onPauseGame(GTK_MENU_ITEM(app->pauseMenuItem), app);
    }
    
    // Create dialog
    GtkWidget* dialog = gtk_dialog_new_with_buttons(
        "Joystick Configuration",
        GTK_WINDOW(app->window),
        GTK_DIALOG_MODAL,
        "_Close", GTK_RESPONSE_CLOSE,
        NULL
    );
    
    // Use a more reasonable size
    gtk_window_set_default_size(GTK_WINDOW(dialog), 500, 400);
    
    // Create content area
    GtkWidget* contentArea = gtk_dialog_get_content_area(GTK_DIALOG(dialog));
    gtk_container_set_border_width(GTK_CONTAINER(contentArea), 8);
    
    // Create a notebook (tabbed interface)
    GtkWidget* notebook = gtk_notebook_new();
    gtk_container_add(GTK_CONTAINER(contentArea), notebook);
    
    // ---- TAB 1: Status ----
    GtkWidget* statusBox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 5);
    gtk_container_set_border_width(GTK_CONTAINER(statusBox), 8);
    GtkWidget* statusLabel = gtk_label_new("Status");
    gtk_notebook_append_page(GTK_NOTEBOOK(notebook), statusBox, statusLabel);
    
    // Joystick info label
    GtkWidget* joystickInfoLabel = gtk_label_new("");
    gtk_widget_set_halign(joystickInfoLabel, GTK_ALIGN_START);
    gtk_box_pack_start(GTK_BOX(statusBox), joystickInfoLabel, FALSE, FALSE, 0);
    
    // Update the joystick info
    updateJoystickInfo(GTK_LABEL(joystickInfoLabel), app);
    
    // Rescan button
    GtkWidget* rescanButton = gtk_button_new_with_label("Rescan for Joysticks");
    gtk_box_pack_start(GTK_BOX(statusBox), rescanButton, FALSE, FALSE, 5);
    
    // Connect signal to rescan button
    g_object_set_data(G_OBJECT(rescanButton), "info-label", joystickInfoLabel);
    g_signal_connect(G_OBJECT(rescanButton), "clicked", 
                   G_CALLBACK(onJoystickRescan), app);
    
    // ---- TAB 2: Selection ----
    GtkWidget* selectionBox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 5);
    gtk_container_set_border_width(GTK_CONTAINER(selectionBox), 8);
    GtkWidget* selectionLabel = gtk_label_new("Selection");
    gtk_notebook_append_page(GTK_NOTEBOOK(notebook), selectionBox, selectionLabel);
    
    // Add label
    GtkWidget* idLabel = gtk_label_new("Select joystick device ID:");
    gtk_widget_set_halign(idLabel, GTK_ALIGN_START);
    gtk_box_pack_start(GTK_BOX(selectionBox), idLabel, FALSE, FALSE, 0);
    
    // Create a horizontal scale (slider) for joystick ID
    GtkWidget* idScale = gtk_scale_new_with_range(GTK_ORIENTATION_HORIZONTAL, 
                                               0, SDL_NumJoysticks() > 0 ? SDL_NumJoysticks() - 1 : 0, 1);
    gtk_range_set_value(GTK_RANGE(idScale), 0); // Default to first joystick
    gtk_scale_set_digits(GTK_SCALE(idScale), 0); // No decimal places
    gtk_scale_set_value_pos(GTK_SCALE(idScale), GTK_POS_RIGHT);
    gtk_box_pack_start(GTK_BOX(selectionBox), idScale, FALSE, FALSE, 0);
    
    // Add min/max labels
    GtkWidget* rangeBox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
    gtk_box_pack_start(GTK_BOX(selectionBox), rangeBox, FALSE, FALSE, 0);
    
    GtkWidget* minLabel = gtk_label_new("0");
    gtk_widget_set_halign(minLabel, GTK_ALIGN_START);
    gtk_box_pack_start(GTK_BOX(rangeBox), minLabel, TRUE, TRUE, 0);
    
    char maxJoystick[8];
    snprintf(maxJoystick, sizeof(maxJoystick), "%d", SDL_NumJoysticks() > 0 ? SDL_NumJoysticks() - 1 : 0);
    GtkWidget* maxLabel = gtk_label_new(maxJoystick);
    gtk_widget_set_halign(maxLabel, GTK_ALIGN_END);
    gtk_box_pack_end(GTK_BOX(rangeBox), maxLabel, TRUE, TRUE, 0);
    
    // Apply button for selecting joystick
    GtkWidget* applyButton = gtk_button_new_with_label("Apply Joystick Selection");
    gtk_box_pack_start(GTK_BOX(selectionBox), applyButton, FALSE, FALSE, 5);
    
    // Set up data for the joystick selection callback
    JoystickSelectionData* selectionData = new JoystickSelectionData;
    selectionData->app = app;
    selectionData->idScale = idScale;
    selectionData->infoLabel = joystickInfoLabel;
    selectionData->dialog = dialog;
    
    // Connect signal to apply button using regular callback
    g_signal_connect_data(G_OBJECT(applyButton), "clicked", 
                        G_CALLBACK(onJoystickSelectionApply), selectionData, 
                        [](gpointer data, GClosure*) { delete static_cast<JoystickSelectionData*>(data); }, 
                        (GConnectFlags)0);
    
    // ---- TAB 3: Testing ----
    GtkWidget* testBox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 5);
    gtk_container_set_border_width(GTK_CONTAINER(testBox), 8);
    GtkWidget* testingLabel = gtk_label_new("Testing");
    gtk_notebook_append_page(GTK_NOTEBOOK(notebook), testBox, testingLabel);
    
    // Add instruction
    GtkWidget* testLabel = gtk_label_new("Move joystick and press buttons to test. Input will be displayed here:");
    gtk_widget_set_halign(testLabel, GTK_ALIGN_START);
    gtk_box_pack_start(GTK_BOX(testBox), testLabel, FALSE, FALSE, 0);
    
    // Create a text view for displaying joystick input
    GtkWidget* textScroll = gtk_scrolled_window_new(NULL, NULL);
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(textScroll),
                                 GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
    gtk_box_pack_start(GTK_BOX(testBox), textScroll, TRUE, TRUE, 0);
    
    GtkWidget* textView = gtk_text_view_new();
    gtk_text_view_set_editable(GTK_TEXT_VIEW(textView), FALSE);
    gtk_container_add(GTK_CONTAINER(textScroll), textView);
    
    // Set monospace font
    gtk_text_view_set_monospace(GTK_TEXT_VIEW(textView), TRUE);
    
    // Get the buffer
    GtkTextBuffer* buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(textView));
    
    // Set up data for the joystick test timer callback
    JoystickTestData* testData = new JoystickTestData;
    testData->app = app;
    testData->buffer = buffer;
    
    // Set up a timer to update the text view with joystick info
    guint testTimerId = g_timeout_add_full(G_PRIORITY_DEFAULT, 100, 
                                          updateJoystickTestDisplay, 
                                          testData, 
                                          [](gpointer data) { delete static_cast<JoystickTestData*>(data); });
    
    // ---- TAB 4: Mapping ----
    GtkWidget* mappingBox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 5);
    gtk_container_set_border_width(GTK_CONTAINER(mappingBox), 8);
    GtkWidget* mappingLabel = gtk_label_new("Mapping");
    gtk_notebook_append_page(GTK_NOTEBOOK(notebook), mappingBox, mappingLabel);
    
    // Create a grid to lay out the mapping controls
    GtkWidget* grid = gtk_grid_new();
    gtk_grid_set_row_spacing(GTK_GRID(grid), 6);
    gtk_grid_set_column_spacing(GTK_GRID(grid), 10);
    gtk_box_pack_start(GTK_BOX(mappingBox), grid, FALSE, FALSE, 0);
    
    // Labels for the grid headers
    GtkWidget* actionsLabel = gtk_label_new("<b>Action</b>");
    gtk_label_set_use_markup(GTK_LABEL(actionsLabel), TRUE);
    gtk_grid_attach(GTK_GRID(grid), actionsLabel, 0, 0, 1, 1);
    
    GtkWidget* currentLabel = gtk_label_new("<b>Current</b>");
    gtk_label_set_use_markup(GTK_LABEL(currentLabel), TRUE);
    gtk_grid_attach(GTK_GRID(grid), currentLabel, 1, 0, 1, 1);
    
    GtkWidget* newLabel = gtk_label_new("<b>New</b>");
    gtk_label_set_use_markup(GTK_LABEL(newLabel), TRUE);
    gtk_grid_attach(GTK_GRID(grid), newLabel, 2, 0, 1, 1);
    
    char currentMapping[50];
    
    // Row 1: Rotate Clockwise
    GtkWidget* rotateLabel = gtk_label_new("Rotate Clockwise:");
    gtk_widget_set_halign(rotateLabel, GTK_ALIGN_START);
    gtk_grid_attach(GTK_GRID(grid), rotateLabel, 0, 1, 1, 1);
    
    snprintf(currentMapping, sizeof(currentMapping), "Button %d", app->joystickMapping.rotate_cw_button);
    GtkWidget* rotateCurrent = gtk_label_new(currentMapping);
    gtk_grid_attach(GTK_GRID(grid), rotateCurrent, 1, 1, 1, 1);
    
    GtkWidget* rotateCombo = gtk_combo_box_text_new();
    for (int i = 0; i < std::min(16, SDL_JoystickNumButtons(app->joystick)); i++) {
        char buttonText[20];
        snprintf(buttonText, sizeof(buttonText), "Button %d", i);
        gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(rotateCombo), buttonText);
    }
    gtk_combo_box_set_active(GTK_COMBO_BOX(rotateCombo), app->joystickMapping.rotate_cw_button);
    gtk_grid_attach(GTK_GRID(grid), rotateCombo, 2, 1, 1, 1);
    
    // Row 2: Rotate Counter-Clockwise
    GtkWidget* rotateCCWLabel = gtk_label_new("Rotate Counter-CW:");
    gtk_widget_set_halign(rotateCCWLabel, GTK_ALIGN_START);
    gtk_grid_attach(GTK_GRID(grid), rotateCCWLabel, 0, 2, 1, 1);
    
    snprintf(currentMapping, sizeof(currentMapping), "Button %d", app->joystickMapping.rotate_ccw_button);
    GtkWidget* rotateCCWCurrent = gtk_label_new(currentMapping);
    gtk_grid_attach(GTK_GRID(grid), rotateCCWCurrent, 1, 2, 1, 1);
    
    GtkWidget* rotateCCWCombo = gtk_combo_box_text_new();
    for (int i = 0; i < std::min(16, SDL_JoystickNumButtons(app->joystick)); i++) {
        char buttonText[20];
        snprintf(buttonText, sizeof(buttonText), "Button %d", i);
        gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(rotateCCWCombo), buttonText);
    }
    gtk_combo_box_set_active(GTK_COMBO_BOX(rotateCCWCombo), app->joystickMapping.rotate_ccw_button);
    gtk_grid_attach(GTK_GRID(grid), rotateCCWCombo, 2, 2, 1, 1);
    
    // Row 3: Hard Drop
    GtkWidget* hardDropLabel = gtk_label_new("Hard Drop:");
    gtk_widget_set_halign(hardDropLabel, GTK_ALIGN_START);
    gtk_grid_attach(GTK_GRID(grid), hardDropLabel, 0, 3, 1, 1);
    
    snprintf(currentMapping, sizeof(currentMapping), "Button %d", app->joystickMapping.hard_drop_button);
    GtkWidget* hardDropCurrent = gtk_label_new(currentMapping);
    gtk_grid_attach(GTK_GRID(grid), hardDropCurrent, 1, 3, 1, 1);
    
    GtkWidget* hardDropCombo = gtk_combo_box_text_new();
    for (int i = 0; i < std::min(16, SDL_JoystickNumButtons(app->joystick)); i++) {
        char buttonText[20];
        snprintf(buttonText, sizeof(buttonText), "Button %d", i);
        gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(hardDropCombo), buttonText);
    }
    gtk_combo_box_set_active(GTK_COMBO_BOX(hardDropCombo), app->joystickMapping.hard_drop_button);
    gtk_grid_attach(GTK_GRID(grid), hardDropCombo, 2, 3, 1, 1);
    
    // Row 4: Pause/Start
    GtkWidget* pauseLabel = gtk_label_new("Pause/Start:");
    gtk_widget_set_halign(pauseLabel, GTK_ALIGN_START);
    gtk_grid_attach(GTK_GRID(grid), pauseLabel, 0, 4, 1, 1);
    
    snprintf(currentMapping, sizeof(currentMapping), "Button %d", app->joystickMapping.pause_button);
    GtkWidget* pauseCurrent = gtk_label_new(currentMapping);
    gtk_grid_attach(GTK_GRID(grid), pauseCurrent, 1, 4, 1, 1);
    
    GtkWidget* pauseCombo = gtk_combo_box_text_new();
    for (int i = 0; i < std::min(16, SDL_JoystickNumButtons(app->joystick)); i++) {
        char buttonText[20];
        snprintf(buttonText, sizeof(buttonText), "Button %d", i);
        gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(pauseCombo), buttonText);
    }
    gtk_combo_box_set_active(GTK_COMBO_BOX(pauseCombo), app->joystickMapping.pause_button);
    gtk_grid_attach(GTK_GRID(grid), pauseCombo, 2, 4, 1, 1);
    
    // Row 5: X-Axis
    GtkWidget* xAxisLabel = gtk_label_new("Horizontal Movement:");
    gtk_widget_set_halign(xAxisLabel, GTK_ALIGN_START);
    gtk_grid_attach(GTK_GRID(grid), xAxisLabel, 0, 5, 1, 1);
    
    snprintf(currentMapping, sizeof(currentMapping), "Axis %d%s", 
          app->joystickMapping.x_axis, 
          app->joystickMapping.invert_x ? " (Inv)" : "");
    GtkWidget* xAxisCurrent = gtk_label_new(currentMapping);
    gtk_grid_attach(GTK_GRID(grid), xAxisCurrent, 1, 5, 1, 1);
    
    GtkWidget* xAxisBox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 5);
    gtk_grid_attach(GTK_GRID(grid), xAxisBox, 2, 5, 1, 1);
    
    GtkWidget* xAxisCombo = gtk_combo_box_text_new();
    for (int i = 0; i < std::min(6, SDL_JoystickNumAxes(app->joystick)); i++) {
        char axisText[20];
        snprintf(axisText, sizeof(axisText), "Axis %d", i);
        gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(xAxisCombo), axisText);
    }
    gtk_combo_box_set_active(GTK_COMBO_BOX(xAxisCombo), app->joystickMapping.x_axis);
    gtk_box_pack_start(GTK_BOX(xAxisBox), xAxisCombo, TRUE, TRUE, 0);
    
    GtkWidget* invertXCheck = gtk_check_button_new_with_label("Invert");
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(invertXCheck), 
                              app->joystickMapping.invert_x);
    gtk_box_pack_start(GTK_BOX(xAxisBox), invertXCheck, FALSE, FALSE, 0);
    
    // Row 6: Y-Axis
    GtkWidget* yAxisLabel = gtk_label_new("Vertical Movement:");
    gtk_widget_set_halign(yAxisLabel, GTK_ALIGN_START);
    gtk_grid_attach(GTK_GRID(grid), yAxisLabel, 0, 6, 1, 1);
    
    snprintf(currentMapping, sizeof(currentMapping), "Axis %d%s", 
          app->joystickMapping.y_axis, 
          app->joystickMapping.invert_y ? " (Inv)" : "");
    GtkWidget* yAxisCurrent = gtk_label_new(currentMapping);
    gtk_grid_attach(GTK_GRID(grid), yAxisCurrent, 1, 6, 1, 1);
    
    GtkWidget* yAxisBox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 5);
    gtk_grid_attach(GTK_GRID(grid), yAxisBox, 2, 6, 1, 1);
    
    GtkWidget* yAxisCombo = gtk_combo_box_text_new();
    for (int i = 0; i < std::min(6, SDL_JoystickNumAxes(app->joystick)); i++) {
        char axisText[20];
        snprintf(axisText, sizeof(axisText), "Axis %d", i);
        gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(yAxisCombo), axisText);
    }
    gtk_combo_box_set_active(GTK_COMBO_BOX(yAxisCombo), app->joystickMapping.y_axis);
    gtk_box_pack_start(GTK_BOX(yAxisBox), yAxisCombo, TRUE, TRUE, 0);
    
    GtkWidget* invertYCheck = gtk_check_button_new_with_label("Invert");
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(invertYCheck), 
                              app->joystickMapping.invert_y);
    gtk_box_pack_start(GTK_BOX(yAxisBox), invertYCheck, FALSE, FALSE, 0);
    
    // Add apply and reset buttons
    GtkWidget* buttonBox = gtk_button_box_new(GTK_ORIENTATION_HORIZONTAL);
    gtk_button_box_set_layout(GTK_BUTTON_BOX(buttonBox), GTK_BUTTONBOX_END);
    gtk_box_set_spacing(GTK_BOX(buttonBox), 10);
    gtk_box_pack_start(GTK_BOX(mappingBox), buttonBox, FALSE, FALSE, 10);
    
    GtkWidget* resetButton = gtk_button_new_with_label("Reset to Defaults");
    gtk_box_pack_start(GTK_BOX(buttonBox), resetButton, FALSE, FALSE, 0);
    
    GtkWidget* applyMappingButton = gtk_button_new_with_label("Apply Mapping");
    gtk_box_pack_start(GTK_BOX(buttonBox), applyMappingButton, FALSE, FALSE, 0);
    
    // Store all data needed for callbacks
    g_object_set_data(G_OBJECT(applyMappingButton), "app", app);
    g_object_set_data(G_OBJECT(applyMappingButton), "rotate_combo", rotateCombo);
    g_object_set_data(G_OBJECT(applyMappingButton), "rotate_ccw_combo", rotateCCWCombo);
    g_object_set_data(G_OBJECT(applyMappingButton), "hard_drop_combo", hardDropCombo);
    g_object_set_data(G_OBJECT(applyMappingButton), "pause_combo", pauseCombo);
    g_object_set_data(G_OBJECT(applyMappingButton), "x_axis_combo", xAxisCombo);
    g_object_set_data(G_OBJECT(applyMappingButton), "y_axis_combo", yAxisCombo);
    g_object_set_data(G_OBJECT(applyMappingButton), "invert_x_check", invertXCheck);
    g_object_set_data(G_OBJECT(applyMappingButton), "invert_y_check", invertYCheck);
    
    g_object_set_data(G_OBJECT(resetButton), "app", app);
    g_object_set_data(G_OBJECT(resetButton), "rotate_combo", rotateCombo);
    g_object_set_data(G_OBJECT(resetButton), "rotate_ccw_combo", rotateCCWCombo);
    g_object_set_data(G_OBJECT(resetButton), "hard_drop_combo", hardDropCombo);
    g_object_set_data(G_OBJECT(resetButton), "pause_combo", pauseCombo);
    g_object_set_data(G_OBJECT(resetButton), "x_axis_combo", xAxisCombo);
    g_object_set_data(G_OBJECT(resetButton), "y_axis_combo", yAxisCombo);
    g_object_set_data(G_OBJECT(resetButton), "invert_x_check", invertXCheck);
    g_object_set_data(G_OBJECT(resetButton), "invert_y_check", invertYCheck);
    
    // Connect signals with C-style callbacks
    g_signal_connect(G_OBJECT(applyMappingButton), "clicked", G_CALLBACK(onJoystickMapApply), NULL);
    g_signal_connect(G_OBJECT(resetButton), "clicked", G_CALLBACK(onJoystickMapReset), NULL);
    
    // Show all dialog widgets
    gtk_widget_show_all(dialog);
    
    // Run the dialog
    gtk_dialog_run(GTK_DIALOG(dialog));
    
    // Clean up the test timer
    g_source_remove(testTimerId);
    
    // Destroy dialog
    gtk_widget_destroy(dialog);
    
    // Resume the game if it wasn't paused before
    if (!wasPaused && !app->board->isGameOver() && !app->board->isSplashScreenActive()) {
        onPauseGame(GTK_MENU_ITEM(app->pauseMenuItem), app);
    }
}

void saveJoystickMapping(TetrimoneApp* app) {
    std::string configDir = g_get_user_config_dir();
    std::string configFile = configDir + "/tetris/joystick.conf";
    
    // Ensure directory exists
    g_mkdir_with_parents((configDir + "/tetris").c_str(), 0755);
    
    FILE* file = fopen(configFile.c_str(), "w");
    if (file) {
        fprintf(file, "rotate_cw_button=%d\n", app->joystickMapping.rotate_cw_button);
        fprintf(file, "rotate_ccw_button=%d\n", app->joystickMapping.rotate_ccw_button);
        fprintf(file, "hard_drop_button=%d\n", app->joystickMapping.hard_drop_button);
        fprintf(file, "pause_button=%d\n", app->joystickMapping.pause_button);
        fprintf(file, "x_axis=%d\n", app->joystickMapping.x_axis);
        fprintf(file, "y_axis=%d\n", app->joystickMapping.y_axis);
        fprintf(file, "invert_x=%d\n", app->joystickMapping.invert_x ? 1 : 0);
        fprintf(file, "invert_y=%d\n", app->joystickMapping.invert_y ? 1 : 0);
        fclose(file);
    }
}

void loadJoystickMapping(TetrimoneApp* app) {
    std::string configDir = g_get_user_config_dir();
    std::string configFile = configDir + "/tetris/joystick.conf";
    
    FILE* file = fopen(configFile.c_str(), "r");
    if (file) {
        char line[100];
        while (fgets(line, sizeof(line), file)) {
            char key[50];
            int value;
            if (sscanf(line, "%[^=]=%d", key, &value) == 2) {
                if (strcmp(key, "rotate_cw_button") == 0) app->joystickMapping.rotate_cw_button = value;
                else if (strcmp(key, "rotate_ccw_button") == 0) app->joystickMapping.rotate_ccw_button = value;
                else if (strcmp(key, "hard_drop_button") == 0) app->joystickMapping.hard_drop_button = value;
                else if (strcmp(key, "pause_button") == 0) app->joystickMapping.pause_button = value;
                else if (strcmp(key, "x_axis") == 0) app->joystickMapping.x_axis = value;
                else if (strcmp(key, "y_axis") == 0) app->joystickMapping.y_axis = value;
                else if (strcmp(key, "invert_x") == 0) app->joystickMapping.invert_x = (value != 0);
                else if (strcmp(key, "invert_y") == 0) app->joystickMapping.invert_y = (value != 0);
            }
        }
        fclose(file);
    }
}

void onJoystickMapApply(GtkButton* button, gpointer userData) {
    TetrimoneApp* app = static_cast<TetrimoneApp*>(g_object_get_data(G_OBJECT(button), "app"));
    GtkWidget* rotateCombo = static_cast<GtkWidget*>(g_object_get_data(G_OBJECT(button), "rotate_combo"));
    GtkWidget* rotateCCWCombo = static_cast<GtkWidget*>(g_object_get_data(G_OBJECT(button), "rotate_ccw_combo"));
    GtkWidget* hardDropCombo = static_cast<GtkWidget*>(g_object_get_data(G_OBJECT(button), "hard_drop_combo"));
    GtkWidget* pauseCombo = static_cast<GtkWidget*>(g_object_get_data(G_OBJECT(button), "pause_combo"));
    GtkWidget* xAxisCombo = static_cast<GtkWidget*>(g_object_get_data(G_OBJECT(button), "x_axis_combo"));
    GtkWidget* yAxisCombo = static_cast<GtkWidget*>(g_object_get_data(G_OBJECT(button), "y_axis_combo"));
    GtkWidget* invertXCheck = static_cast<GtkWidget*>(g_object_get_data(G_OBJECT(button), "invert_x_check"));
    GtkWidget* invertYCheck = static_cast<GtkWidget*>(g_object_get_data(G_OBJECT(button), "invert_y_check"));
    
    // Update the joystick mapping
    app->joystickMapping.rotate_cw_button = gtk_combo_box_get_active(GTK_COMBO_BOX(rotateCombo));
    app->joystickMapping.rotate_ccw_button = gtk_combo_box_get_active(GTK_COMBO_BOX(rotateCCWCombo));
    app->joystickMapping.hard_drop_button = gtk_combo_box_get_active(GTK_COMBO_BOX(hardDropCombo));
    app->joystickMapping.pause_button = gtk_combo_box_get_active(GTK_COMBO_BOX(pauseCombo));
    app->joystickMapping.x_axis = gtk_combo_box_get_active(GTK_COMBO_BOX(xAxisCombo));
    app->joystickMapping.y_axis = gtk_combo_box_get_active(GTK_COMBO_BOX(yAxisCombo));
    app->joystickMapping.invert_x = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(invertXCheck));
    app->joystickMapping.invert_y = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(invertYCheck));
    
    // Save the mapping
    saveJoystickMapping(app);
    
    // Show confirmation dialog
    GtkWidget* dialog = gtk_message_dialog_new(
        GTK_WINDOW(gtk_widget_get_toplevel(GTK_WIDGET(button))),
        GTK_DIALOG_MODAL,
        GTK_MESSAGE_INFO,
        GTK_BUTTONS_OK,
        "Joystick mapping has been updated and saved."
    );
    gtk_dialog_run(GTK_DIALOG(dialog));
    gtk_widget_destroy(dialog);
}

void onJoystickMapReset(GtkButton* button, gpointer userData) {
    TetrimoneApp* app = static_cast<TetrimoneApp*>(g_object_get_data(G_OBJECT(button), "app"));
    GtkWidget* rotateCombo = static_cast<GtkWidget*>(g_object_get_data(G_OBJECT(button), "rotate_combo"));
    GtkWidget* rotateCCWCombo = static_cast<GtkWidget*>(g_object_get_data(G_OBJECT(button), "rotate_ccw_combo"));
    GtkWidget* hardDropCombo = static_cast<GtkWidget*>(g_object_get_data(G_OBJECT(button), "hard_drop_combo"));
    GtkWidget* pauseCombo = static_cast<GtkWidget*>(g_object_get_data(G_OBJECT(button), "pause_combo"));
    GtkWidget* xAxisCombo = static_cast<GtkWidget*>(g_object_get_data(G_OBJECT(button), "x_axis_combo"));
    GtkWidget* yAxisCombo = static_cast<GtkWidget*>(g_object_get_data(G_OBJECT(button), "y_axis_combo"));
    GtkWidget* invertXCheck = static_cast<GtkWidget*>(g_object_get_data(G_OBJECT(button), "invert_x_check"));
    GtkWidget* invertYCheck = static_cast<GtkWidget*>(g_object_get_data(G_OBJECT(button), "invert_y_check"));
    
    // Reset to defaults
    gtk_combo_box_set_active(GTK_COMBO_BOX(rotateCombo), 0);      // A button
    gtk_combo_box_set_active(GTK_COMBO_BOX(rotateCCWCombo), 1);   // B button
    gtk_combo_box_set_active(GTK_COMBO_BOX(hardDropCombo), 3);    // Y button
    gtk_combo_box_set_active(GTK_COMBO_BOX(pauseCombo), 9);       // Start button
    gtk_combo_box_set_active(GTK_COMBO_BOX(xAxisCombo), 0);       // First axis
    gtk_combo_box_set_active(GTK_COMBO_BOX(yAxisCombo), 1);       // Second axis
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(invertXCheck), FALSE);
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(invertYCheck), FALSE);
    
    // Show confirmation dialog
    GtkWidget* dialog = gtk_message_dialog_new(
        GTK_WINDOW(gtk_widget_get_toplevel(GTK_WIDGET(button))),
        GTK_DIALOG_MODAL,
        GTK_MESSAGE_INFO,
        GTK_BUTTONS_OK,
        "Joystick mapping has been reset to defaults.\nClick Apply to save these changes."
    );
    gtk_dialog_run(GTK_DIALOG(dialog));
    gtk_widget_destroy(dialog);
}
