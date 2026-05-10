#ifndef TETRIMONE_QT5_H
#define TETRIMONE_QT5_H

#include <QApplication>
#include <QWidget>
#include <QLabel>
#include <QMenuBar>
#include <QMenu>
#include <QAction>
#include <QActionGroup>
#include <QTimer>
#include <SDL2/SDL.h>
#include "audiomanager.h"
#include "tetrimone_core.h"

// Forward declarations
struct TetrimoneApp;
class GameAreaWidget;
class NextPieceWidget;

// ============================================================================
// Qt5-specific TetrimoneApp structure
// ============================================================================


struct TetrimoneApp {
    QApplication* app = nullptr;
    QWidget*      window = nullptr;
    QWidget*      mainBox = nullptr;
    QWidget*      gameArea = nullptr;
    QWidget*      nextPieceArea = nullptr;

    QLabel*       scoreLabel = nullptr;
    QLabel*       levelLabel = nullptr;
    QLabel*       linesLabel = nullptr;
    QLabel*       difficultyLabel = nullptr;

    bool          backgroundMusicPlaying = false;
    TetrimoneBoard* board = nullptr;

    int           timerId = 0;
    int           dropSpeed = 500;

    QAction*      backgroundToggleMenuItem = nullptr;

    // Menu related widgets
    QMenuBar*     menuBar = nullptr;
    QAction*      startMenuItem = nullptr;
    QAction*      pauseMenuItem = nullptr;
    QAction*      restartMenuItem = nullptr;
    QAction*      soundToggleMenuItem = nullptr;
    QAction*      zenMenuItem = nullptr;
    QAction*      easyMenuItem = nullptr;
    QAction*      mediumMenuItem = nullptr;
    QAction*      hardMenuItem = nullptr;
    QAction*      extremeMenuItem = nullptr;
    QAction*      insaneMenuItem = nullptr;

    QAction*      trackMenuItems[5] = {nullptr};
    QAction*      themeMenuItems[31] = {nullptr};

    QLabel*       sequenceLabel = nullptr;
    QLabel*       controlsLabel = nullptr;

    int           difficulty = 1; // 0=Zen, 1=Easy, 2=Medium, 3=Hard, 4=Extreme

    QLabel*       controlsHeaderLabel = nullptr;

    SDL_Joystick* joystick = nullptr;
    bool          joystickEnabled = false;
    int           joystickTimerId = 0;

    JoystickMapping joystickMapping;
    bool            pausedByFocusLoss = false;

    // Rendering mode selection
    enum RenderingMode {
        RENDER_CAIRO  = 0,
        RENDER_OPENGL = 1
    };
    RenderingMode renderingMode = RENDER_CAIRO;

    QAction*      renderModeMenuItems[2] = {nullptr};

    CommandLineArgs* cmdlineArgs = nullptr;
};

// ============================================================================
// Qt5-specific UI functions
// ============================================================================

// Core game functions
void updateLabels(TetrimoneApp* app);
void startGame(TetrimoneApp* app);
void pauseGame(TetrimoneApp* app);
void resetUI(TetrimoneApp* app);
void cleanupApp(TetrimoneApp* app);
void updateDisplay(TetrimoneApp* app);

// SDL/Joystick initialization
void initSDL(TetrimoneApp* app);
void shutdownSDL(TetrimoneApp* app);

// Event handlers
void onKeyDownTick(TetrimoneApp* app);
void onKeyLeftTick(TetrimoneApp* app);
void onKeyRightTick(TetrimoneApp* app);
void onGameTick(TetrimoneApp* app);

// Help dialogs
void onAboutDialog(void* menuItem, void* userData);
void onInstructionsDialog(void* menuItem, void* userData);

#ifdef QT5
void onAppActivate(TetrimoneApp* app);
#endif

// Menu actions
void onStartGameAction(TetrimoneApp* app);
void onPauseGameAction(TetrimoneApp* app);
void onRestartGameAction(TetrimoneApp* app);
void onQuitGameAction(TetrimoneApp* app);
void onSoundToggleAction(TetrimoneApp* app, bool enabled);
void onDifficultyChanged(TetrimoneApp* app, int difficulty);

// UI setup
void setupMenuBar(TetrimoneApp* app);
void setupGameUI(TetrimoneApp* app, int width, int height);

// Application entry point
int main_qt5(int argc, char* argv[], TetrimoneApp* app);

// Global variables
extern int BLOCK_SIZE;
extern int currentThemeIndex;
extern int GRID_WIDTH;
extern int GRID_HEIGHT;

// Key state variables
extern bool keyDownPressed;
extern bool keyLeftPressed;
extern bool keyRightPressed;

#endif // TETRIMONE_QT5_H
