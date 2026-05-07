// ============================================================================
// joystick_core.cpp - Framework-agnostic joystick logic
// This file contains all SDL joystick polling, mapping, and data management
// Framework-specific UI and event handling belongs in joystick_gtk.cpp, etc.
// ============================================================================

#include <SDL2/SDL.h>
#include <cstdio>
#include <string>
#include <cstring>
#include <cstdlib>
#include <algorithm>

#ifdef GTK3
#include "tetrimone_gtk.h"
#include "gtk3_dialog_helpers.h"
#endif

#ifdef QT5
#include "tetrimone_qt5.h"
#endif

// ============================================================================
// Types and Constants
// ============================================================================

typedef struct {
  bool active;
  int direction;
  Uint32 lastMoveTime;
  Uint32 repeatDelay;
  int moveCount;
} DirectionalControl;

const int DEFAULT_DEADZONE = 8000;        // ~25% of max axis value
const int MAX_JOYSTICK_BUTTONS = 16;
const Uint32 BUTTON_DEBOUNCE_TIME = 200;  // ms between button actions
const int AXIS_ACCELERATION_THRESHOLD = 25000;  // Threshold for acceleration
const int AXIS_REPEAT_DELAY = 150;        // ms between axis repeats

// ============================================================================
// Core SDL Initialization and Shutdown
// ============================================================================

void initSDL(TetrimoneApp *app) {
  if (app->joystickEnabled) {
    shutdownSDL(app);
  }

  if (SDL_Init(SDL_INIT_JOYSTICK) < 0) {
    printf("SDL could not initialize! SDL Error: %s\n", SDL_GetError());
    app->joystick = NULL;
    app->joystickEnabled = false;
    app->joystickTimerId = 0;
    return;
  }

  app->joystickEnabled = true;

  int numJoysticks = SDL_NumJoysticks();
  if (numJoysticks < 1) {
    printf("No joysticks connected!\n");
    app->joystick = NULL;
    app->joystickTimerId = 0;
    return;
  }

  printf("Found %d joystick(s)\n", numJoysticks);
  
  app->joystick = SDL_JoystickOpen(0);
  if (app->joystick == NULL) {
    printf("Unable to open joystick! SDL Error: %s\n", SDL_GetError());
    app->joystickTimerId = 0;
    return;
  }

  printf("Joystick connected: %s\n", SDL_JoystickName(app->joystick));
  printf("Number of buttons: %d\n", SDL_JoystickNumButtons(app->joystick));
  printf("Number of axes: %d\n", SDL_JoystickNumAxes(app->joystick));
  printf("Number of hats: %d\n", SDL_JoystickNumHats(app->joystick));

  // Set default mapping
  app->joystickMapping = {
      0,      // rotate_cw_button: A button
      1,      // rotate_ccw_button: B button
      3,      // hard_drop_button: Y button
      9,      // pause_button: Start button
      0,      // x_axis
      1,      // y_axis
      false,  // invert_x
      false   // invert_y
  };

  // Framework-specific timer setup is delegated to framework layer
  // (e.g., g_timeout_add for GTK3, QTimer for Qt5)
}

void shutdownSDL(TetrimoneApp *app) {
  // Framework-specific timer cleanup should be done before this
  // (e.g., g_source_remove for GTK3)
  if (app->joystickTimerId > 0) {
    // Note: This assumes the framework layer has a compatible timer ID
    // Otherwise, framework-specific shutdown should call this before removing timer
  }

  if (app->joystick != NULL) {
    SDL_JoystickClose(app->joystick);
    app->joystick = NULL;
  }

  SDL_Quit();
  app->joystickEnabled = false;
}

// ============================================================================
// Joystick Poll Logic - Core Input Processing
// ============================================================================

/**
 * Process button inputs and call callbacks for framework-specific UI updates
 * @param app Application instance
 * @param onPauseCallback Callback when pause button is pressed
 * @param onRotateCallback Callback when rotation buttons are pressed
 * @param onHardDropCallback Callback when hard drop is pressed
 * @return true if a button action was processed
 */
bool processJoystickButtons(
    TetrimoneApp* app,
    void (*onPauseCallback)(TetrimoneApp*, bool shouldPause),
    void (*onRotateCallback)(TetrimoneApp*, bool clockwise),
    void (*onHardDropCallback)(TetrimoneApp*)) {
  
  // Safety checks
  if (!app || !app->joystickEnabled || !app->joystick || !app->board) {
    return false;
  }

  static Uint32 lastButtonPressTime = 0;
  Uint32 currentTime = SDL_GetTicks();
  
  int numButtons = SDL_JoystickNumButtons(app->joystick);
  if (numButtons > MAX_JOYSTICK_BUTTONS) {
    numButtons = MAX_JOYSTICK_BUTTONS;
  }

  bool anyButtonProcessed = false;

  for (int i = 0; i < numButtons; i++) {
    if (SDL_JoystickGetButton(app->joystick, i)) {
      if (currentTime - lastButtonPressTime > BUTTON_DEBOUNCE_TIME) {
        bool buttonProcessed = true;

        // Pause button is always responsive
        if (i == app->joystickMapping.pause_button) {
          if (onPauseCallback) {
            bool shouldPause = !(app->board->isSplashScreenActive() || app->board->isGameOver());
            onPauseCallback(app, shouldPause);
          }
          buttonProcessed = true;
        } else if (!app->board->isGameOver() && !app->board->isPaused()) {
          // Game control buttons only active during gameplay
          if (i == app->joystickMapping.rotate_cw_button) {
            if (onRotateCallback) onRotateCallback(app, true);
            buttonProcessed = true;
          } else if (i == app->joystickMapping.rotate_ccw_button) {
            if (onRotateCallback) onRotateCallback(app, false);
            buttonProcessed = true;
          } else if (i == app->joystickMapping.hard_drop_button) {
            if (onHardDropCallback) onHardDropCallback(app);
            buttonProcessed = true;
          } else {
            // Legacy button mappings for compatibility
            switch (i) {
              case 2: // X button
                if (onRotateCallback) onRotateCallback(app, true);
                buttonProcessed = true;
                break;
              case 6:  // LB/L1
              case 10: // RB/R1
                if (onHardDropCallback) onHardDropCallback(app);
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

        if (buttonProcessed) {
          lastButtonPressTime = currentTime;
          anyButtonProcessed = true;
        }
      }
    }
  }

  return anyButtonProcessed;
}

/**
 * Process analog stick input and return movement direction
 * @param app Application instance
 * @param outX Output: -1, 0, or 1 for left, center, right
 * @param outY Output: -1, 0, or 1 for up, center, down
 */
void getJoystickAnalogMovement(TetrimoneApp* app, int* outX, int* outY) {
  *outX = 0;
  *outY = 0;

  if (!app || !app->joystickEnabled || !app->joystick) {
    return;
  }

  static DirectionalControl horizontalControl = {false, 0, 0, AXIS_REPEAT_DELAY, 0};
  static DirectionalControl verticalControl = {false, 0, 0, AXIS_REPEAT_DELAY, 0};
  
  Uint32 currentTime = SDL_GetTicks();

  // Process X-axis (horizontal movement)
  if (SDL_JoystickNumAxes(app->joystick) > app->joystickMapping.x_axis) {
    int xValue = SDL_JoystickGetAxis(app->joystick, app->joystickMapping.x_axis);
    
    if (app->joystickMapping.invert_x) {
      xValue = -xValue;
    }

    if (xValue < -AXIS_ACCELERATION_THRESHOLD) {
      *outX = -1;
    } else if (xValue > AXIS_ACCELERATION_THRESHOLD) {
      *outX = 1;
    } else {
      *outX = 0;
    }
  }

  // Process Y-axis (vertical movement)
  if (SDL_JoystickNumAxes(app->joystick) > app->joystickMapping.y_axis) {
    int yValue = SDL_JoystickGetAxis(app->joystick, app->joystickMapping.y_axis);
    
    if (app->joystickMapping.invert_y) {
      yValue = -yValue;
    }

    if (yValue < -AXIS_ACCELERATION_THRESHOLD) {
      *outY = -1;
    } else if (yValue > AXIS_ACCELERATION_THRESHOLD) {
      *outY = 1;
    } else {
      *outY = 0;
    }
  }
}

// ============================================================================
// Configuration Persistence
// ============================================================================

void saveJoystickMapping(TetrimoneApp* app) {
  std::string configDir = g_get_user_config_dir();
  std::string configFile = configDir + "/tetrimone/joystick.conf";
  
  g_mkdir_with_parents((configDir + "/tetrimone").c_str(), 0755);
  
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
  std::string configFile = configDir + "/tetrimone/joystick.conf";
  
  FILE* file = fopen(configFile.c_str(), "r");
  if (file) {
    char line[100];
    while (fgets(line, sizeof(line), file)) {
      char key[50];
      int value;
      if (sscanf(line, "%[^=]=%d", key, &value) == 2) {
        if (strcmp(key, "rotate_cw_button") == 0) 
          app->joystickMapping.rotate_cw_button = value;
        else if (strcmp(key, "rotate_ccw_button") == 0) 
          app->joystickMapping.rotate_ccw_button = value;
        else if (strcmp(key, "hard_drop_button") == 0) 
          app->joystickMapping.hard_drop_button = value;
        else if (strcmp(key, "pause_button") == 0) 
          app->joystickMapping.pause_button = value;
        else if (strcmp(key, "x_axis") == 0) 
          app->joystickMapping.x_axis = value;
        else if (strcmp(key, "y_axis") == 0) 
          app->joystickMapping.y_axis = value;
        else if (strcmp(key, "invert_x") == 0) 
          app->joystickMapping.invert_x = (value != 0);
        else if (strcmp(key, "invert_y") == 0) 
          app->joystickMapping.invert_y = (value != 0);
      }
    }
    fclose(file);
  }
}
