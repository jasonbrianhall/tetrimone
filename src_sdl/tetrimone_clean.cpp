#include "tetrimone.h"
#include "audiomanager.h"
#include <iostream>
#include <string>
#include <algorithm>
#include "highscores.h"
#include "propaganda_messages.h"
#include "freedom_messages.h"
#include "commandline.h"

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

int BLOCK_SIZE = 30;
int currentThemeIndex = 0;
int GRID_WIDTH = 10;
int GRID_HEIGHT = 22;

static bool keyDownPressed = false;
static bool keyLeftPressed = false;
static bool keyRightPressed = false;
static uint32_t keyDownTimer = 0;
static uint32_t keyLeftTimer = 0;
static uint32_t keyRightTimer = 0;
static int keyDownDelay = 150;
static int keyLeftDelay = 150;
static int keyRightDelay = 150;
static int keyDownCount = 0;
static int keyLeftCount = 0;
static int keyRightCount = 0;

// ============================================================================
// KEYBOARD HANDLERS - SIMPLIFIED (No GTK)
// ============================================================================

bool onKeyDownTick(void* userData) {
  TetrimoneApp *app = static_cast<TetrimoneApp *>(userData);
  
  if (!app->board->isPaused() && !app->board->isGameOver() && keyDownPressed) {
    app->board->movePiece(0, 1);
    keyDownCount++;
    
    if (keyDownCount > 6) {
      keyDownDelay = 20;
    } else if (keyDownCount > 4) {
      keyDownDelay = 30;
    } else if (keyDownCount > 2) {
      keyDownDelay = 60;
    }
    app->needsRedraw = true;
    return TRUE;
  }
  
  keyDownTimer = 0;
  return FALSE;
}

bool onKeyLeftTick(void* userData) {
  TetrimoneApp *app = static_cast<TetrimoneApp *>(userData);
  
  if (!app->board->isPaused() && !app->board->isGameOver() && keyLeftPressed) {
    app->board->movePiece(-1, 0);
    keyLeftCount++;
    
    if (keyLeftCount > 6) {
      keyLeftDelay = 30;
    } else if (keyLeftCount > 4) {
      keyLeftDelay = 50;
    } else if (keyLeftCount > 2) {
      keyLeftDelay = 100;
    }
    app->needsRedraw = true;
    return TRUE;
  }
  
  keyLeftTimer = 0;
  return FALSE;
}

bool onKeyRightTick(void* userData) {
  TetrimoneApp *app = static_cast<TetrimoneApp *>(userData);
  
  if (!app->board->isPaused() && !app->board->isGameOver() && keyRightPressed) {
    app->board->movePiece(1, 0);
    keyRightCount++;
    
    if (keyRightCount > 6) {
      keyRightDelay = 30;
    } else if (keyRightCount > 4) {
      keyRightDelay = 50;
    } else if (keyRightCount > 2) {
      keyRightDelay = 100;
    }
    app->needsRedraw = true;
    return TRUE;
  }
  
  keyRightTimer = 0;
  return FALSE;
}

// ============================================================================
// EVENT HANDLER
// ============================================================================

void handleSDLEvent(const SDL_Event& event, TetrimoneApp* app) {
  if (!app || !app->board) return;
  
  switch(event.type) {
    case SDL_KEYDOWN:
      switch(event.key.keysym.sym) {
        case SDLK_LEFT:
          app->board->movePiece(-1, 0);
          app->needsRedraw = true;
          break;
        case SDLK_RIGHT:
          app->board->movePiece(1, 0);
          app->needsRedraw = true;
          break;
        case SDLK_DOWN:
          app->board->movePiece(0, 1);
          app->needsRedraw = true;
          break;
        case SDLK_UP:
        case SDLK_z:
          app->board->rotatePiece();
          app->needsRedraw = true;
          break;
        case SDLK_x:
          app->board->rotatePiece(false);
          app->needsRedraw = true;
          break;
        case SDLK_SPACE:
          app->board->hardDrop();
          app->needsRedraw = true;
          break;
        case SDLK_p:
          app->board->togglePause();
          app->needsRedraw = true;
          break;
        default:
          break;
      }
      break;
  }
}

// ============================================================================
// GAME UPDATE LOOP
// ============================================================================

bool onTimerTick(void* data) {
  TetrimoneApp *app = static_cast<TetrimoneApp *>(data);
  
  if (!app || !app->board) return FALSE;
  
  if (!app->board->isPaused() && !app->board->isGameOver()) {
    app->board->movePiece(0, 1);
    app->needsRedraw = true;
  }
  
  return TRUE;
}

// ============================================================================
// GAME CONTROL
// ============================================================================

void updateLabels(TetrimoneApp* app) {
  if (!app) return;
  // Score/level display would go here
}

void resetUI(TetrimoneApp* app) {
  if (!app) return;
  app->needsRedraw = true;
}

void startGame(TetrimoneApp* app) {
  if (!app || !app->board) return;
  app->board->restart();
  app->needsRedraw = true;
}

void pauseGame(TetrimoneApp* app) {
  if (!app || !app->board) return;
  app->board->togglePause();
  app->needsRedraw = true;
}

void onStartGame(TetrimoneApp* app) {
  startGame(app);
}

void onPauseGame(TetrimoneApp* app) {
  pauseGame(app);
}

void onRestartGame(TetrimoneApp* app) {
  if (app && app->board) app->board->restart();
  if (app) app->needsRedraw = true;
}

void onQuitGame(TetrimoneApp* app) {
  // SDL quit handled by main loop
}

void adjustDropSpeed(TetrimoneApp* app) {
  if (!app) return;
  // Drop speed adjustment
}

// ============================================================================
// STUBS FOR LINKING
// ============================================================================

void calculateBlockSize(TetrimoneApp *app) {}
bool pollJoystick(TetrimoneApp* app) { return true; }
void initSDL(TetrimoneApp* app) {}
void shutdownSDL(TetrimoneApp* app) {}
void onBackgroundImageDialog(TetrimoneApp* app) {}
void onBackgroundToggled(bool useBackground, void* userData) {}
void onBackgroundOpacityDialog(TetrimoneApp* app) {}
void onBackgroundZipDialog(TetrimoneApp* app) {}
void onVolumeDialog(TetrimoneApp* app) {}
void onVolumeValueChanged(float volume, void* userData) {}
void onMusicVolumeValueChanged(float volume, void* userData) {}
void onTestSound(TetrimoneApp* app) {}
void onJoystickConfig(TetrimoneApp* app) {}
void saveJoystickMapping(TetrimoneApp* app) {}
void loadJoystickMapping(TetrimoneApp* app) {}
void onGameSetupDialog(TetrimoneApp* app) {}
bool saveGameSettings(TetrimoneApp* app) { return true; }
bool loadGameSettings(TetrimoneApp* app) { return true; }
void resetGameSettings(TetrimoneApp* app) {}
void onGhostPieceToggled(bool enabled, void* userData) {}
void setWindowIcon(SDL_Window* window) {}
void showIdeologicalFailureDialog(TetrimoneApp* app) {}
void showPatrioticPerformanceDialog(TetrimoneApp* app) {}
void onViewHighScores(TetrimoneApp* app) {}
void onThemeChanged(int themeIndex, void* userData) {}
void drawGameCairo(cairo_t* cr, TetrimoneBoard* board, int width, int height) {}
void drawNextPieceCairo(cairo_t* cr, TetrimoneBoard* board, int width, int height) {}
void drawFreezyEffect(cairo_t* cr, double x, double y, double size, float heatLevel, double time) {}
void drawFireyGlow(cairo_t* cr, double x, double y, double size, float heatLevel, double time) {}
void onBlockTrailsToggled(bool enabled, void* userData) {}
void onBlockTrailsConfig(TetrimoneApp* app) {}
void onSoundToggled(bool enabled, void* userData) {}
std::array<double, 3> getHeatModifiedColor(const std::array<double, 3>& baseColor, float heatLevel) {
  return baseColor;
}
cairo_surface_t* cairo_image_surface_create_from_jpeg(const char* filename) { return nullptr; }
cairo_surface_t* cairo_image_surface_create_from_memory(const void* data, size_t length) { return nullptr; }
void shutdownApp(TetrimoneApp* app) {}
