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

// ============================================================================
// Qt5-specific TetrimoneApp structure
// ============================================================================

struct TetrimoneApp {
    QApplication* app;
    QWidget*      window;
    QWidget*      mainBox;
    QWidget*      gameArea;
    QWidget*      nextPieceArea;

    QLabel*       scoreLabel;
    QLabel*       levelLabel;
    QLabel*       linesLabel;
    QLabel*       difficultyLabel;

    bool          backgroundMusicPlaying = false;
    TetrimoneBoard* board;

    int           timerId;        // replaces guint
    int           dropSpeed;

    QAction*      backgroundToggleMenuItem;

    // Menu related widgets
    QMenuBar*     menuBar;
    QAction*      startMenuItem;
    QAction*      pauseMenuItem;
    QAction*      restartMenuItem;
    QAction*      soundToggleMenuItem;
    QAction*      zenMenuItem;
    QAction*      easyMenuItem;
    QAction*      mediumMenuItem;
    QAction*      hardMenuItem;
    QAction*      extremeMenuItem;
    QAction*      insaneMenuItem;

    QAction*      trackMenuItems[5];
    QAction*      themeMenuItems[31];

    QLabel*       sequenceLabel;
    QLabel*       controlsLabel;

    int           difficulty; // 1 = Easy, 2 = Medium, 3 = Hard, 0 = Zen, 4 = Extreme

    QLabel*       controlsHeaderLabel;

    SDL_Joystick* joystick;
    bool          joystickEnabled;
    int           joystickTimerId;   // replaces guint

    JoystickMapping joystickMapping;
    bool            pausedByFocusLoss = false;

    // Rendering mode selection
    enum RenderingMode {
        RENDER_CAIRO  = 0,
        RENDER_OPENGL = 1
    };
    RenderingMode renderingMode;

    QAction*      renderModeMenuItems[2];  // Radio menu items for Cairo and OpenGL
};

// ============================================================================
// Qt5-specific UI functions
// ============================================================================

// These are Qt5 implementations of platform-agnostic functions
// Declarations are in tetrimone_core.h, implementations are Qt5-specific

#endif // TETRIMONE_QT5_H
