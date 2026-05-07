// ============================================================================
// joystick_qt5.cpp - Qt5-specific UI and event handling for joystick
// Interfaces with joystick_core.cpp for input processing
// ============================================================================

#ifdef QT5

#include "tetrimone_qt5.h"
#include <QTimer>
#include <QTextEdit>
#include <QVBoxLayout>
#include <QDateTime>
#include <QMessageBox>
#include <algorithm>

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
const int POLL_INTERVAL = 16;       // ms between joystick polls (~60 Hz)

// ============================================================================
// Joystick Polling - Qt5 Implementation
// ============================================================================

typedef struct {
  bool active;
  int direction;
  qint64 lastMoveTime;
  int repeatDelay;
  int moveCount;
} DirectionalControl;

class JoystickPoller : public QObject {
  Q_OBJECT

private:
  TetrimoneApp* app;
  QTimer* pollTimer;
  DirectionalControl horizontalControl;
  DirectionalControl verticalControl;

public:
  JoystickPoller(TetrimoneApp* appInstance) : app(appInstance) {
    pollTimer = new QTimer(this);
    connect(pollTimer, &QTimer::timeout, this, &JoystickPoller::onPollTimeout);
    
    horizontalControl = {false, 0, 0, AXIS_REPEAT_DELAY, 0};
    verticalControl = {false, 0, 0, AXIS_REPEAT_DELAY, 0};
  }

  void start() {
    if (app->joystick) {
      pollTimer->start(POLL_INTERVAL);
    }
  }

  void stop() {
    pollTimer->stop();
  }

  ~JoystickPoller() {
    pollTimer->stop();
  }

private slots:
  void onPollTimeout() {
    if (!app || !app->joystickEnabled || !app->joystick || !app->board) {
      return;
    }

    SDL_JoystickUpdate();

    // Process button inputs
    processJoystickButtons(app, 
        &JoystickPoller::onPauseCallback,
        &JoystickPoller::onRotateCallback,
        &JoystickPoller::onHardDropCallback);

    // Skip movement if game is paused or over
    if (app->board->isGameOver() || app->board->isPaused()) {
      return;
    }

    // Process analog movement
    int moveX = 0, moveY = 0;
    getJoystickAnalogMovement(app, &moveX, &moveY);

    qint64 currentTime = QDateTime::currentMSecsSinceEpoch();

    // Process horizontal movement
    if (moveX != 0) {
      if (!horizontalControl.active || horizontalControl.direction != moveX) {
        horizontalControl.active = true;
        horizontalControl.direction = moveX;
        horizontalControl.lastMoveTime = currentTime;
        horizontalControl.moveCount = 0;
        
        app->board->movePiece(moveX, 0);
        if (app->gameWidget) {
          app->gameWidget->update();
        }
        updateLabels(app);
      } else if (currentTime - horizontalControl.lastMoveTime > horizontalControl.repeatDelay) {
        horizontalControl.moveCount++;
        int acceleration = std::min(5, horizontalControl.moveCount / 10);
        int moves = 1 + acceleration;
        
        for (int i = 0; i < moves; i++) {
          app->board->movePiece(moveX, 0);
        }
        horizontalControl.lastMoveTime = currentTime;
        if (app->gameWidget) {
          app->gameWidget->update();
        }
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
        if (app->gameWidget) {
          app->gameWidget->update();
        }
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
        if (app->gameWidget) {
          app->gameWidget->update();
        }
        updateLabels(app);
      }
    } else {
      verticalControl.active = false;
      verticalControl.direction = 0;
    }
  }

private:
  static void onPauseCallback(TetrimoneApp* app, bool shouldPause) {
    if (app->board->isSplashScreenActive()) {
      app->board->dismissSplashScreen();
      startGame(app);
      if (app->gameWidget) app->gameWidget->update();
      if (app->nextPieceWidget) app->nextPieceWidget->update();
      updateLabels(app);
    } else if (app->board->isGameOver()) {
      onRestartGame(app);
    } else if (shouldPause) {
      onPauseGame(app);
    }
  }

  static void onRotateCallback(TetrimoneApp* app, bool clockwise) {
    app->board->rotatePiece(clockwise);
    if (app->gameWidget) app->gameWidget->update();
  }

  static void onHardDropCallback(TetrimoneApp* app) {
    app->board->hardDrop();
    if (app->gameWidget) app->gameWidget->update();
    updateLabels(app);
  }
};

// Global joystick poller instance per application
static JoystickPoller* g_joystickPoller = nullptr;

void initJoystickPoller(TetrimoneApp* app) {
  if (g_joystickPoller) {
    delete g_joystickPoller;
  }
  g_joystickPoller = new JoystickPoller(app);
  g_joystickPoller->start();
}

void shutdownJoystickPoller() {
  if (g_joystickPoller) {
    g_joystickPoller->stop();
    delete g_joystickPoller;
    g_joystickPoller = nullptr;
  }
}

// ============================================================================
// Joystick Configuration
// ============================================================================

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

void onJoystickConfig(TetrimoneApp* app) {
  if (!app->joystick) {
    // Show error dialog in Qt5 style
    QMessageBox::critical(app->mainWindow, "Error", "No joystick connected!");
    return;
  }
  
  // Pause game if running
  bool wasPaused = app->board->isPaused();
  if (!wasPaused && !app->board->isGameOver() && !app->board->isSplashScreenActive()) {
    onPauseGame(app);
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
  
  // TODO: Implement Qt5 joystick mapping dialog
  // createJoystickMappingDialog(app->mainWindow, config, onJoystickMappingApply, app);
  
  // Resume game if needed
  if (!wasPaused && !app->board->isGameOver() && !app->board->isSplashScreenActive()) {
    onPauseGame(app);
  }
}

#endif  // QT5
