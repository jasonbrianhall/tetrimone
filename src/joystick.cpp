#include "tetris.h"

void initSDL(TetrisApp* app) {
    // Make sure we initialize app values first
    app->joystick = NULL;
    app->joystickEnabled = false;
    app->joystickTimerId = 0;
    
    // Initialize SDL joystick subsystem
    if (SDL_Init(SDL_INIT_JOYSTICK) < 0) {
        printf("SDL could not initialize! SDL Error: %s\n", SDL_GetError());
        return;
    }
    
    // Check for joysticks
    if (SDL_NumJoysticks() < 1) {
        printf("No joysticks connected!\n");
        SDL_QuitSubSystem(SDL_INIT_JOYSTICK);
        return;
    }
    
    // Open the first joystick
    app->joystick = SDL_JoystickOpen(0);
    if (app->joystick == NULL) {
        printf("Unable to open joystick! SDL Error: %s\n", SDL_GetError());
        SDL_QuitSubSystem(SDL_INIT_JOYSTICK);
        return;
    }
    
    printf("Joystick connected: %s\n", SDL_JoystickName(app->joystick));
    printf("Number of buttons: %d\n", SDL_JoystickNumButtons(app->joystick));
    printf("Number of axes: %d\n", SDL_JoystickNumAxes(app->joystick));
    
    app->joystickEnabled = true;
    
    // Start the joystick polling timer
    app->joystickTimerId = g_timeout_add(16, pollJoystick, app);
}

// Shutdown SDL
void shutdownSDL(TetrisApp* app) {
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
    
    // Quit SDL subsystem
    SDL_QuitSubSystem(SDL_INIT_JOYSTICK);
    app->joystickEnabled = false;
}

// Joystick polling function (called by timer)
gboolean pollJoystick(gpointer data) {
    TetrisApp* app = static_cast<TetrisApp*>(data);
    
    if (!app->joystickEnabled || app->joystick == NULL || app->board == NULL) {
        return FALSE;
    }
    
    TetrisBoard* board = app->board;
    
    // Update joystick state
    SDL_JoystickUpdate();
    
    // Static variables to track previous state
    static int lastXDir = 0;
    static int lastYDir = 0;
    static Uint32 lastButtonPressTime = 0;
    static const Uint32 BUTTON_DEBOUNCE_TIME = 200; // ms between button actions
    static Uint32 lastDpadActionTime = 0;
    static const Uint32 DPAD_DEBOUNCE_TIME = 100; // ms between D-pad actions
    
    // Get current time
    Uint32 currentTime = SDL_GetTicks();
    
    // Process buttons with debounce
    int numButtons = SDL_JoystickNumButtons(app->joystick);
    for (int i = 0; i < numButtons; i++) {
        if (SDL_JoystickGetButton(app->joystick, i)) {
            // Only process button if enough time has passed since last press
            if (currentTime - lastButtonPressTime > BUTTON_DEBOUNCE_TIME) {
                bool buttonProcessed = true;
                
                switch (i) {
                    case 0:  // A button - rotate clockwise
                        board->rotatePiece(true);  // true for clockwise
                        break;
                        
                    case 1:  // B button - counter-rotate
                        board->rotatePiece(false);  // false for counter-clockwise
                        break;
                        
                    case 3:  // Y button - hard drop
                        board->hardDrop();
                        break;
                        
                    case 7:  // Start button - pause/resume
                        if (!board->isGameOver()) {
                            onPauseGame(GTK_MENU_ITEM(app->pauseMenuItem), app);
                        }
                        break;
                        
                    case 6:  // Select/Back button - restart if game over
                        if (board->isGameOver()) {
                            onRestartGame(GTK_MENU_ITEM(app->restartMenuItem), app);
                        }
                        break;
                        
                    default:
                        buttonProcessed = false;
                        break;
                }
                
                // Update the last button press time if we processed a button
                if (buttonProcessed) {
                    lastButtonPressTime = currentTime;
                }
            }
        }
    }
    
    // Process axes
    const int DEADZONE = 8000;
    
    if (SDL_JoystickNumAxes(app->joystick) >= 2) {
        // X axis
        int xValue = SDL_JoystickGetAxis(app->joystick, 0);
        int xDir = 0;
        
        if (xValue < -DEADZONE) {
            xDir = -1;  // Left
        } else if (xValue > DEADZONE) {
            xDir = 1;   // Right
        }
        
        // Only process when direction changes
        if (xDir != lastXDir) {
            lastXDir = xDir;
            
            if (xDir == -1) {
                board->movePiece(-1, 0);
            } else if (xDir == 1) {
                board->movePiece(1, 0);
            }
        }
        
        // Y axis
        int yValue = SDL_JoystickGetAxis(app->joystick, 1);
        int yDir = 0;
        
        if (yValue < -DEADZONE) {
            yDir = -1;  // Up
        } else if (yValue > DEADZONE) {
            yDir = 1;   // Down
        }
        
        // Only process when direction changes
        if (yDir != lastYDir) {
            lastYDir = yDir;
            
            if (yDir == -1) {
                board->rotatePiece(true);  // true for clockwise
            } else if (yDir == 1) {
                board->movePiece(0, 1);
            }
        }
    }
    
    // Process D-pad hat if present with debounce
    if (currentTime - lastDpadActionTime > DPAD_DEBOUNCE_TIME) {
        bool actionTaken = false;
        
        for (int i = 0; i < SDL_JoystickNumHats(app->joystick); i++) {
            Uint8 hatState = SDL_JoystickGetHat(app->joystick, i);
            
            if (hatState & SDL_HAT_LEFT) {
                board->movePiece(-1, 0);
                actionTaken = true;
            }
            else if (hatState & SDL_HAT_RIGHT) {
                board->movePiece(1, 0);
                actionTaken = true;
            }
            
            if (hatState & SDL_HAT_UP) {
                board->rotatePiece(true);  // true for clockwise
                actionTaken = true;
            }
            else if (hatState & SDL_HAT_DOWN) {
                board->movePiece(0, 1);
                actionTaken = true;
            }
        }
        
        if (actionTaken) {
            lastDpadActionTime = currentTime;
        }
    }
    
    // Update display
    gtk_widget_queue_draw(app->gameArea);
    gtk_widget_queue_draw(app->nextPieceArea);
    updateLabels(app);
    
    return TRUE;  // Keep the timer going
}
