// ============================================================================
// joystick_gtk.cpp - GTK3-specific UI and event handling for joystick
// Interfaces with joystick_core.cpp for input processing
// ============================================================================

#ifdef GTK3

#include "tetrimone_gtk.h"
#include "gtk3_dialog_helpers.h"
#include <gtk/gtk.h>
#include <algorithm>

using namespace GTK3Helpers;

// ============================================================================
// Forward Declarations - Core Functions (defined in joystick_core.cpp)
// ============================================================================

extern void initSDL(TetrimoneApp *app);
extern void shutdownSDL(TetrimoneApp *app);
extern bool processJoystickButtons(
    TetrimoneApp* app,
    void (*onPauseCallback)(TetrimoneApp*, bool shouldPause),
    void (*onRotateCallback)(TetrimoneApp*, bool clockwise),
    void (*onHardDropCallback)(TetrimoneApp*));
extern void getJoystickAnalogMovement(TetrimoneApp* app, int* outX, int* outY);
extern void saveJoystickMapping(TetrimoneApp* app);
extern void loadJoystickMapping(TetrimoneApp* app);

// ============================================================================
// Constants
// ============================================================================

const int AXIS_REPEAT_DELAY = 150;  // ms between repeated movement inputs

// ============================================================================
// GTK3-Specific Input Processing
// ============================================================================

typedef struct {
  bool active;
  int direction;
  Uint32 lastMoveTime;
  Uint32 repeatDelay;
  int moveCount;
} DirectionalControl;

typedef struct {
  TetrimoneApp* app;
  GtkTextBuffer* buffer;
} JoystickTestData;

// Callbacks for button inputs - GTK3 specific
static void onJoystickPause(TetrimoneApp* app, bool shouldPause) {
  if (app->board->isSplashScreenActive()) {
    app->board->dismissSplashScreen();
    startGame(app);
    gtk_widget_queue_draw(app->gameArea);
    gtk_widget_queue_draw(app->nextPieceArea);
    updateLabels(app);
  } else if (app->board->isGameOver()) {
    onRestartGame(GTK_MENU_ITEM(app->restartMenuItem), app);
  } else if (shouldPause) {
    onPauseGame(GTK_MENU_ITEM(app->pauseMenuItem), app);
  }
}

static void onJoystickRotate(TetrimoneApp* app, bool clockwise) {
  app->board->rotatePiece(clockwise);
  gtk_widget_queue_draw(app->gameArea);
}

static void onJoystickHardDrop(TetrimoneApp* app) {
  app->board->hardDrop();
  gtk_widget_queue_draw(app->gameArea);
  updateLabels(app);
}

// ============================================================================
// Joystick Polling Timer - GTK3 Implementation
// ============================================================================

gboolean pollJoystick(gpointer data) {
  TetrimoneApp *app = static_cast<TetrimoneApp *>(data);

  if (!app || !app->joystickEnabled || !app->joystick || !app->board) {
    return FALSE;
  }

  SDL_JoystickUpdate();

  // Process button inputs
  processJoystickButtons(app, onJoystickPause, onJoystickRotate, onJoystickHardDrop);

  // Skip movement if game is paused or over
  if (app->board->isGameOver() || app->board->isPaused()) {
    return TRUE;
  }

  // Process analog movement
  int moveX = 0, moveY = 0;
  getJoystickAnalogMovement(app, &moveX, &moveY);

  static DirectionalControl horizontalControl = {false, 0, 0, AXIS_REPEAT_DELAY, 0};
  static DirectionalControl verticalControl = {false, 0, 0, AXIS_REPEAT_DELAY, 0};

  Uint32 currentTime = SDL_GetTicks();

  // Process horizontal movement
  if (moveX != 0) {
    if (!horizontalControl.active || horizontalControl.direction != moveX) {
      horizontalControl.active = true;
      horizontalControl.direction = moveX;
      horizontalControl.lastMoveTime = currentTime;
      horizontalControl.moveCount = 0;
      
      app->board->movePiece(moveX, 0);
      gtk_widget_queue_draw(app->gameArea);
      updateLabels(app);
    } else if (currentTime - horizontalControl.lastMoveTime > horizontalControl.repeatDelay) {
      horizontalControl.moveCount++;
      int acceleration = std::min(5, horizontalControl.moveCount / 10);
      int moves = 1 + acceleration;
      
      for (int i = 0; i < moves; i++) {
        app->board->movePiece(moveX, 0);
      }
      horizontalControl.lastMoveTime = currentTime;
      gtk_widget_queue_draw(app->gameArea);
      updateLabels(app);
    }
  } else {
    horizontalControl.active = false;
    horizontalControl.direction = 0;
  }

  // Process vertical movement (drop)
  if (moveY != 0) {
    if (!verticalControl.active || verticalControl.direction != moveY) {
      verticalControl.active = true;
      verticalControl.direction = moveY;
      verticalControl.lastMoveTime = currentTime;
      verticalControl.moveCount = 0;
      
      if (moveY > 0) {  // Down/drop
        app->board->movePiece(0, 1);
      }
      gtk_widget_queue_draw(app->gameArea);
      updateLabels(app);
    } else if (currentTime - verticalControl.lastMoveTime > verticalControl.repeatDelay) {
      verticalControl.moveCount++;
      int acceleration = std::min(5, verticalControl.moveCount / 10);
      int drops = 1 + acceleration;
      
      if (moveY > 0) {
        for (int i = 0; i < drops; i++) {
          app->board->movePiece(0, 1);
        }
      }
      verticalControl.lastMoveTime = currentTime;
      gtk_widget_queue_draw(app->gameArea);
      updateLabels(app);
    }
  } else {
    verticalControl.active = false;
    verticalControl.direction = 0;
  }

  return TRUE;  // Keep timer running
}

// ============================================================================
// Joystick Test Display
// ============================================================================

gboolean updateJoystickTestDisplay(gpointer userData) {
  JoystickTestData* data = static_cast<JoystickTestData*>(userData);
  GtkTextBuffer* buffer = data->buffer;
  TetrimoneApp* app = data->app;
  
  if (!app->joystickEnabled || !app->joystick) {
    return TRUE;
  }
  
  SDL_JoystickUpdate();
  
  std::string info = "Joystick: " + std::string(SDL_JoystickName(app->joystick)) + "\n\n";
  
  // Display buttons in three columns
  info += "Buttons:\n";
  int numButtons = SDL_JoystickNumButtons(app->joystick);
  int buttonsPerColumn = (numButtons + 2) / 3;
  
  std::vector<std::string> buttonStrings;
  for (int i = 0; i < numButtons; i++) {
    bool pressed = SDL_JoystickGetButton(app->joystick, i);
    buttonStrings.push_back("Button " + std::to_string(i) + ": " + 
                          (pressed ? "PRESSED" : "released"));
  }
  
  for (int row = 0; row < buttonsPerColumn; row++) {
    std::string line = "";
    
    if (row < (int)buttonStrings.size()) {
      line += buttonStrings[row];
      line += std::string(25 - buttonStrings[row].length(), ' ');
    }
    
    int col2Index = row + buttonsPerColumn;
    if (col2Index < (int)buttonStrings.size()) {
      line += buttonStrings[col2Index];
      line += std::string(25 - buttonStrings[col2Index].length(), ' ');
    }
    
    int col3Index = row + (2 * buttonsPerColumn);
    if (col3Index < (int)buttonStrings.size()) {
      line += buttonStrings[col3Index];
    }
    
    info += line + "\n";
  }
  
  // Display axes
  info += "\nAxes:\n";
  int numAxes = SDL_JoystickNumAxes(app->joystick);
  for (int i = 0; i < numAxes; i++) {
    int value = SDL_JoystickGetAxis(app->joystick, i);
    float normalizedValue = value / 32768.0f;
    info += "Axis " + std::to_string(i) + ": " + std::to_string(value) + 
            " (" + std::to_string(normalizedValue) + ")\n";
  }
  
  // Display hats
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
  
  gtk_text_buffer_set_text(buffer, info.c_str(), -1);
  return TRUE;
}

// ============================================================================
// Joystick Configuration Dialogs
// ============================================================================

// Callback for joystick mapping dialog
void onJoystickMappingApply(int rotate_cw, int rotate_ccw, int hard_drop, int pause_btn,
                           int x_axis, int y_axis, bool invert_x, bool invert_y, gpointer userData) {
  TetrimoneApp* app = static_cast<TetrimoneApp*>(userData);
  
  app->joystickMapping.rotate_cw_button = rotate_cw;
  app->joystickMapping.rotate_ccw_button = rotate_ccw;
  app->joystickMapping.hard_drop_button = hard_drop;
  app->joystickMapping.pause_button = pause_btn;
  app->joystickMapping.x_axis = x_axis;
  app->joystickMapping.y_axis = y_axis;
  app->joystickMapping.invert_x = invert_x;
  app->joystickMapping.invert_y = invert_y;
  
  saveJoystickMapping(app);
}

void onJoystickConfig(GtkMenuItem* menuItem, gpointer userData) {
  TetrimoneApp* app = static_cast<TetrimoneApp*>(userData);
  (void)menuItem;
  
  if (!app->joystick) {
    DialogConfig errorConfig{
        .title = "Error",
        .acceptButtonLabel = "_OK",
        .width = 300,
        .height = 150
    };
    
    std::vector<TextConfig> errorText{
        {
            .content = "No joystick connected!",
            .markup = "",
            .isMarkup = false,
            .marginTop = 10,
            .marginBottom = 10
        }
    };
    
    createAndRunDialog(GTK_WINDOW(app->window), errorConfig, errorText);
    return;
  }
  
  // Pause game if running
  bool wasPaused = app->board->isPaused();
  if (!wasPaused && !app->board->isGameOver() && !app->board->isSplashScreenActive()) {
    onPauseGame(GTK_MENU_ITEM(app->pauseMenuItem), app);
  }
  
  // Create and show configuration dialog
  int numButtons = std::min(16, SDL_JoystickNumButtons(app->joystick));
  int numAxes = std::min(6, SDL_JoystickNumAxes(app->joystick));
  
  JoystickMappingConfig config{
      .title = "Joystick Configuration - Mapping",
      .numButtons = numButtons,
      .numAxes = numAxes,
      .rotate_cw = app->joystickMapping.rotate_cw_button,
      .rotate_ccw = app->joystickMapping.rotate_ccw_button,
      .hard_drop = app->joystickMapping.hard_drop_button,
      .pause_button = app->joystickMapping.pause_button,
      .x_axis = app->joystickMapping.x_axis,
      .y_axis = app->joystickMapping.y_axis,
      .invert_x = app->joystickMapping.invert_x,
      .invert_y = app->joystickMapping.invert_y,
      .width = 500,
      .height = 400
  };
  
  createJoystickMappingDialog(GTK_WINDOW(app->window), config, onJoystickMappingApply, app);
  
  // Resume game if needed
  if (!wasPaused && !app->board->isGameOver() && !app->board->isSplashScreenActive()) {
    onPauseGame(GTK_MENU_ITEM(app->pauseMenuItem), app);
  }
}

#endif  // GTK3
