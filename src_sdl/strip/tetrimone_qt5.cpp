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
    
    if (app->board->junkLinesPercentage > 0) {
        app->board->generateJunkLines(app->board->junkLinesPercentage);
    }
    
    if (app->board->junkLinesPerLevel > 0) {
        app->board->addJunkLinesFromBottom(app->board->junkLinesPerLevel);
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

