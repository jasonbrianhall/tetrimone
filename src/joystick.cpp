#include "tetris.h"

// Memory corruption is likely happening because of a problem with SDL cleanup
// Let's adjust our initialization/shutdown process

void initSDL(TetrisApp *app) {
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

  // Check for joysticks
  if (SDL_NumJoysticks() < 1) {
    printf("No joysticks connected!\n");
    SDL_Quit();
    app->joystick = NULL;
    app->joystickEnabled = false;
    app->joystickTimerId = 0;
    return;
  }

  // Open the first joystick
  app->joystick = SDL_JoystickOpen(0);
  if (app->joystick == NULL) {
    printf("Unable to open joystick! SDL Error: %s\n", SDL_GetError());
    SDL_Quit();
    app->joystickEnabled = false;
    app->joystickTimerId = 0;
    return;
  }

  printf("Joystick connected: %s\n", SDL_JoystickName(app->joystick));
  printf("Number of buttons: %d\n", SDL_JoystickNumButtons(app->joystick));
  printf("Number of axes: %d\n", SDL_JoystickNumAxes(app->joystick));

  app->joystickEnabled = true;

  // Start the joystick polling timer only if it's not already running
  if (app->joystickTimerId == 0) {
    app->joystickTimerId = g_timeout_add(16, pollJoystick, app);
  }
}

// Shutdown SDL
void shutdownSDL(TetrisApp *app) {
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

  // Quit SDL completely - this may fix memory issues
  SDL_Quit();
  app->joystickEnabled = false;
}

// Add these acceleration-related data structures
typedef struct {
  bool active;
  int direction;
  Uint32 lastMoveTime;
  Uint32 repeatDelay;
  int moveCount;
} DirectionalControl;

// Joystick polling function (called by timer)
gboolean pollJoystick(gpointer data) {
  TetrisApp *app = static_cast<TetrisApp *>(data);

  if (!app || !app->joystickEnabled || !app->joystick || !app->board) {
    return FALSE;
  }

  // Get current joystick state
  SDL_JoystickUpdate();

  // Get current time
  Uint32 currentTime = SDL_GetTicks();

  // Static variables for tracking state
  static Uint32 lastButtonPressTime = 0;
  static const Uint32 BUTTON_DEBOUNCE_TIME = 200; // ms between button actions

  // Directional controls with acceleration
  static DirectionalControl horizontalControl = {false, 0, 0, 150, 0};
  static DirectionalControl verticalControl = {false, 0, 0, 150, 0};
  static DirectionalControl dpadHorizontal = {false, 0, 0, 150, 0};
  static DirectionalControl dpadVertical = {false, 0, 0, 150, 0};

  // Handle button presses even when game is paused or over
  // Process buttons with debounce
  int numButtons = SDL_JoystickNumButtons(app->joystick);
  // Clamp number of buttons to avoid out-of-bounds access
  if (numButtons > 16)
    numButtons = 16;

  for (int i = 0; i < numButtons; i++) {
    if (SDL_JoystickGetButton(app->joystick, i)) {
      // Print which button was pressed for debugging

      // Only process button if enough time has passed since last press
      if (currentTime - lastButtonPressTime > BUTTON_DEBOUNCE_TIME) {
        bool buttonProcessed = true;

        if (i == 9) { // Start button - pause/unpause/restart
          if (app->board->isGameOver()) {
            // If game is over, restart
            onRestartGame(GTK_MENU_ITEM(app->restartMenuItem), app);
          } else {
            // If game is running or paused, toggle pause state
            onPauseGame(GTK_MENU_ITEM(app->pauseMenuItem), app);
          }
          buttonProcessed = true;
        } // Game action buttons only work when game is running
        else if (!app->board->isGameOver() && !app->board->isPaused()) {
          switch (i) {
          case 0: // A button - rotate clockwise
            app->board->rotatePiece(true);
            break;

          case 1: // B button - counter-rotate
            app->board->rotatePiece(false);
            break;

          case 3: // Y button - hard drop
            app->board->hardDrop();
            break;

          default:
            buttonProcessed = false;
            break;
          }
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

  // Process axes with acceleration
  const int DEADZONE = 8000;

  if (SDL_JoystickNumAxes(app->joystick) >= 2) {
    // X axis (horizontal movement)
    int xValue = SDL_JoystickGetAxis(app->joystick, 0);
    int newDir = 0;

    if (xValue < -DEADZONE) {
      newDir = -1; // Left
    } else if (xValue > DEADZONE) {
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
        if (horizontalControl.moveCount > 4) {
          horizontalControl.repeatDelay = 50; // Fast
        } else if (horizontalControl.moveCount > 2) {
          horizontalControl.repeatDelay = 100; // Medium
        }
      }
    } else {
      // No direction - reset horizontal control
      horizontalControl.active = false;
    }

    // Y axis (vertical movement - only downward)
    int yValue = SDL_JoystickGetAxis(app->joystick, 1);
    newDir = 0;

    if (yValue > DEADZONE) {
      newDir = 1; // Down
    } else if (yValue < -DEADZONE) {
      // Up is for rotation, handle separately
      if (!verticalControl.active || verticalControl.direction != -1) {
        // Only rotate once when first pushing up
        app->board->rotatePiece(true);
        verticalControl.active = true;
        verticalControl.direction = -1;
      }
    }

    // Handle vertical movement (down) with acceleration
    if (newDir == 1) {
      // If downward movement just started
      if (!verticalControl.active || verticalControl.direction != newDir) {
        verticalControl.active = true;
        verticalControl.direction = newDir;
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
        if (verticalControl.moveCount > 4) {
          verticalControl.repeatDelay = 30; // Very fast
        } else if (verticalControl.moveCount > 2) {
          verticalControl.repeatDelay = 60; // Fast
        }
      }
    } else if (newDir == 0 && yValue > -DEADZONE) {
      // No downward direction - reset vertical control
      verticalControl.active = false;
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
        if (dpadHorizontal.moveCount > 4) {
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
        if (dpadVertical.moveCount > 4) {
          dpadVertical.repeatDelay = 30; // Very fast
        } else if (dpadVertical.moveCount > 2) {
          dpadVertical.repeatDelay = 60; // Fast
        }
      }
    } else if (dpadY == 0 && !(hatState & SDL_HAT_UP)) {
      // No downward direction - reset vertical control
      dpadVertical.active = false;
    }
  }

  // Update display
  gtk_widget_queue_draw(app->gameArea);
  gtk_widget_queue_draw(app->nextPieceArea);
  updateLabels(app);

  return TRUE; // Keep the timer going
}
