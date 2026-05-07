#include "tetrimone_qt5.h"
#include "audiomanager.h"
#include <iostream>
#include <string>
#include <algorithm>
#include "highscores.h"
#include "freedom_messages.h"

// Qt5 GUI updates are minimal - the actual rendering happens in Qt widget code
// This file contains only backend-agnostic game logic

// Core game functions are defined the same way for both backends
// They don't touch GUI-specific code

void updateLabels(TetrimoneApp* app) {
    if (!app || !app->board) return;
    // Qt5: Update is handled by Qt signals/slots in the main window
    // No direct label manipulation needed here
}

void startGame(TetrimoneApp* app) {
    if (!app || !app->board) return;
    
    app->board->restart();
    
    if (!app->backgroundMusicPlaying) {
        app->board->resumeBackgroundMusic();
    }
    
    if (app->board->getJunkLinesPercentage() > 0) {
        app->board->generateJunkLines(app->board->getJunkLinesPercentage());
    }
    
    if (app->board->getJunkLinesPerLevel() > 0) {
        app->board->addJunkLinesFromBottom(app->board->getJunkLinesPerLevel());
    }
}

void pauseGame(TetrimoneApp* app) {
    if (!app || !app->board) return;
    
    app->board->setPaused(!app->board->isPaused());
    
    if (app->backgroundMusicPlaying) {
        app->board->pauseBackgroundMusic();
    }
}

void resetUI(TetrimoneApp* app) {
    if (!app || !app->board) return;
    // Qt5: UI reset handled by Qt framework
}

void cleanupApp(TetrimoneApp* app) {
    if (!app) return;
    
    if (app->board) {
        app->board->pauseBackgroundMusic();
        app->board->cleanupAudio();
        delete app->board;
        app->board = nullptr;
    }
    
    if (app->joystick) {
        SDL_JoystickClose(app->joystick);
        app->joystick = nullptr;
    }
    
    shutdownSDL(app);
}

void initSDL(TetrimoneApp* app) {
    if (!app) return;
    
    if (SDL_Init(SDL_INIT_JOYSTICK) < 0) {
        std::cerr << "Failed to initialize SDL for joystick: " << SDL_GetError() << std::endl;
        return;
    }
    
    int numJoysticks = SDL_NumJoysticks();
    if (numJoysticks > 0) {
        app->joystick = SDL_JoystickOpen(0);
        if (app->joystick) {
            app->joystickEnabled = true;
        }
    }
}

void shutdownSDL(TetrimoneApp* app) {
    if (!app) return;
    
    if (app->joystick) {
        SDL_JoystickClose(app->joystick);
        app->joystick = nullptr;
    }
    
    SDL_Quit();
}

void updateDisplay(TetrimoneApp *app) {
    if (!app || !app->board) return;
    
    // Qt5: Drawing is handled by Qt's paint events
    drawBoard(app->board);
    drawNextPieceArea(app->board);
}

void drawBoard(TetrimoneBoard *board) {
    // Stub: Qt5 rendering is handled by custom QWidget
}

void drawNextPieceArea(TetrimoneBoard *board) {
    // Stub: Qt5 rendering is handled by custom QWidget
}

void saveJoystickMapping(TetrimoneApp* app) {
    if (!app) return;
    // Load/save mapping to config file
}

void loadJoystickMapping(TetrimoneApp* app) {
    if (!app) return;
    // Load from config file
}

bool saveGameSettings(TetrimoneApp* app) {
    if (!app || !app->board) return false;
    // Save to config file
    return true;
}

bool loadGameSettings(TetrimoneApp* app) {
    if (!app || !app->board) return false;
    // Load from config file
    return true;
}

void resetGameSettings(TetrimoneApp* app) {
    if (!app) return;
    // Reset to defaults
}

std::array<double, 3> getHeatModifiedColor(const std::array<double, 3>& baseColor, float heatLevel) {
    std::array<double, 3> result = baseColor;
    
    if (heatLevel > 0.5f) {
        float intensity = (heatLevel - 0.5f) * 2.0f;
        result[0] = std::min(1.0, result[0] + intensity * 0.5);
        result[1] = std::max(0.0, result[1] - intensity * 0.3);
        result[2] = std::max(0.0, result[2] - intensity * 0.2);
    }
    
    return result;
}

void showIdeologicalFailureDialog(TetrimoneApp* app) {
    if (!app) return;
    // Show dialog
}

void showPatrioticPerformanceDialog(TetrimoneApp* app) {
    if (!app || !app->board) return;
    // Show dialog
}

int ui_run_application(int argc, char *argv[], TetrimoneApp *app, const CommandLineArgs *args) {
    // Qt5: This is handled by QApplication::exec() in main()
    return 0;
}
