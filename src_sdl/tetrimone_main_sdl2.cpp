#include "tetrimone_sdl2.h"
#include <iostream>
#include <sstream>
#include <cmath>

// Global variables
int GRID_WIDTH = 10;
int GRID_HEIGHT = 22;
int BLOCK_SIZE = 30;
int currentThemeIndex = 0;

// Static state for keyboard repeat
static bool keyDownPressed = false;
static bool keyLeftPressed = false;
static bool keyRightPressed = false;
static uint32_t keyDownRepeatTime = 0;
static uint32_t keyLeftRepeatTime = 0;
static uint32_t keyRightRepeatTime = 0;

const uint32_t KEY_REPEAT_INITIAL_DELAY = 150;
const uint32_t KEY_REPEAT_INTERVAL = 50;

void app_init(TetrimoneApp& app) {
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_JOYSTICK | SDL_INIT_GAMECONTROLLER) < 0) {
        std::cerr << "SDL initialization failed: " << SDL_GetError() << std::endl;
        app.running = false;
        return;
    }

    if (TTF_Init() < 0) {
        std::cerr << "SDL_ttf initialization failed: " << TTF_GetError() << std::endl;
        app.running = false;
        return;
    }

    // Create window
    int windowWidth = std::max(MIN_WINDOW_WIDTH, GRID_WIDTH * BLOCK_SIZE + SIDEBAR_WIDTH + WINDOW_PADDING * 3);
    int windowHeight = std::max(MIN_WINDOW_HEIGHT, GRID_HEIGHT * BLOCK_SIZE + WINDOW_PADDING * 2);

    app.window = SDL_CreateWindow(
        "Tetrimone",
        SDL_WINDOWPOS_CENTERED,
        SDL_WINDOWPOS_CENTERED,
        windowWidth,
        windowHeight,
        SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE
    );

    if (!app.window) {
        std::cerr << "Window creation failed: " << SDL_GetError() << std::endl;
        app.running = false;
        return;
    }

    // Create renderer
    app.renderer = SDL_CreateRenderer(
        app.window,
        -1,
        SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC
    );

    if (!app.renderer) {
        std::cerr << "Renderer creation failed: " << SDL_GetError() << std::endl;
        app.running = false;
        return;
    }

    SDL_SetRenderDrawBlendMode(app.renderer, SDL_BLENDMODE_BLEND);

    // Load fonts
    // Note: Adjust paths to where your fonts are located
    app.mainFont = TTF_OpenFont("assets/fonts/DejaVuSans.ttf", 16);
    app.smallFont = TTF_OpenFont("assets/fonts/DejaVuSans.ttf", 12);
    app.largeFont = TTF_OpenFont("assets/fonts/DejaVuSans.ttf", 24);

    if (!app.mainFont) {
        std::cout << "Warning: Could not load main font, using default\n";
        // Create a default font fallback (this would require SDL_gfx or similar)
    }

    // Initialize joystick
    int joystickCount = SDL_NumJoysticks();
    if (joystickCount > 0) {
        app.joystick = SDL_JoystickOpen(0);
        app.joystickEnabled = app.joystick != nullptr;
        if (app.joystickEnabled) {
            std::cout << "Joystick detected: " << SDL_JoystickName(app.joystick) << std::endl;
        }
    }

    // Initialize board
    app.board = new TetrimoneBoard(&app);

    // Initialize other state
    app.running = true;
    app.showMenu = true;
    app.soundEnabled = true;
    app.musicEnabled = true;
    app.difficulty = 2;  // Medium
    app.activeDialog = TetrimoneApp::DialogState::MAIN_MENU;
    app.lastFrameTime = SDL_GetTicks();
    app.frameCount = 0;
    app.fps = 0.0f;
}

void app_cleanup(TetrimoneApp& app) {
    if (app.board) {
        delete app.board;
        app.board = nullptr;
    }

    if (app.joystick) {
        SDL_JoystickClose(app.joystick);
        app.joystick = nullptr;
    }

    if (app.mainFont) TTF_CloseFont(app.mainFont);
    if (app.smallFont) TTF_CloseFont(app.smallFont);
    if (app.largeFont) TTF_CloseFont(app.largeFont);

    if (app.renderer) SDL_DestroyRenderer(app.renderer);
    if (app.window) SDL_DestroyWindow(app.window);

    TTF_Quit();
    SDL_Quit();
}

void app_handle_events(TetrimoneApp& app) {
    SDL_Event event;

    while (SDL_PollEvent(&event)) {
        switch (event.type) {
            case SDL_QUIT:
                app.running = false;
                break;

            case SDL_KEYDOWN:
                handle_keyboard_input(app, event.key);
                break;

            case SDL_KEYUP: {
                SDL_Keycode key = event.key.keysym.sym;
                if (key == SDLK_DOWN) {
                    keyDownPressed = false;
                    keyDownRepeatTime = 0;
                } else if (key == SDLK_LEFT) {
                    keyLeftPressed = false;
                    keyLeftRepeatTime = 0;
                } else if (key == SDLK_RIGHT) {
                    keyRightPressed = false;
                    keyRightRepeatTime = 0;
                }
                break;
            }

            case SDL_MOUSEBUTTONDOWN:
                handle_mouse_input(app, event.button);
                break;

            case SDL_WINDOWEVENT:
                if (event.window.event == SDL_WINDOWEVENT_FOCUS_LOST) {
                    if (!app.board->isPaused() && !app.board->isGameOver()) {
                        app.board->togglePause();
                    }
                } else if (event.window.event == SDL_WINDOWEVENT_FOCUS_GAINED) {
                    // Auto-resume could go here
                }
                break;
        }
    }

    // Handle joystick input
    if (app.joystickEnabled) {
        handle_joystick_input(app);
    }
}

void handle_keyboard_input(TetrimoneApp& app, SDL_KeyboardEvent& event) {
    SDL_Keycode key = event.keysym.sym;
    bool isGameActive = !app.board->isPaused() && !app.board->isGameOver() && !app.board->isSplashScreenActive();

    switch (key) {
        case SDLK_DOWN:
            if (isGameActive && !keyDownPressed) {
                keyDownPressed = true;
                keyDownRepeatTime = SDL_GetTicks() + KEY_REPEAT_INITIAL_DELAY;
                app.board->movePiece(0, 1);
            }
            break;

        case SDLK_LEFT:
            if (isGameActive && !keyLeftPressed) {
                keyLeftPressed = true;
                keyLeftRepeatTime = SDL_GetTicks() + KEY_REPEAT_INITIAL_DELAY;
                app.board->movePiece(-1, 0);
            }
            break;

        case SDLK_RIGHT:
            if (isGameActive && !keyRightPressed) {
                keyRightPressed = true;
                keyRightRepeatTime = SDL_GetTicks() + KEY_REPEAT_INITIAL_DELAY;
                app.board->movePiece(1, 0);
            }
            break;

        case SDLK_z:
        case SDLK_a:
            if (isGameActive) app.board->rotatePiece(false);
            break;

        case SDLK_x:
        case SDLK_UP:
            if (isGameActive) app.board->rotatePiece(true);
            break;

        case SDLK_SPACE:
            if (isGameActive) app.board->hardDropPiece();
            break;

        case SDLK_p:
            if (!app.board->isGameOver() && !app.board->isSplashScreenActive()) {
                app.board->togglePause();
            }
            break;

        case SDLK_r:
            app.board->restart();
            app.activeDialog = TetrimoneApp::DialogState::NONE;
            break;

        case SDLK_ESCAPE:
            if (app.activeDialog != TetrimoneApp::DialogState::NONE) {
                app.activeDialog = TetrimoneApp::DialogState::NONE;
            } else if (!app.board->isGameOver()) {
                app.activeDialog = TetrimoneApp::DialogState::PAUSE_MENU;
                app.board->togglePause();
            }
            break;

        case SDLK_m:
            app.soundEnabled = !app.soundEnabled;
            break;

        case SDLK_h:
            show_high_scores_dialog(app);
            break;
    }
}

void handle_mouse_input(TetrimoneApp& app, SDL_MouseButtonEvent& event) {
    // Handle menu clicks, button clicks, etc.
    // This will be expanded when implementing UI dialogs
}

void handle_joystick_input(TetrimoneApp& app) {
    if (!app.joystick) return;

    SDL_JoystickUpdate();

    // Get axis values
    int16_t xAxis = SDL_JoystickGetAxis(app.joystick, 0);  // Left stick X
    int16_t yAxis = SDL_JoystickGetAxis(app.joystick, 1);  // Left stick Y

    const int16_t JOYSTICK_THRESHOLD = 16000;

    bool isGameActive = !app.board->isPaused() && !app.board->isGameOver() && !app.board->isSplashScreenActive();

    if (isGameActive) {
        // Horizontal movement
        if (xAxis < -JOYSTICK_THRESHOLD) {
            app.board->movePiece(-1, 0);
        } else if (xAxis > JOYSTICK_THRESHOLD) {
            app.board->movePiece(1, 0);
        }

        // Vertical movement
        if (yAxis > JOYSTICK_THRESHOLD) {
            app.board->movePiece(0, 1);
        }
    }

    // Button handling
    for (int i = 0; i < SDL_JoystickNumButtons(app.joystick); i++) {
        if (SDL_JoystickGetButton(app.joystick, i)) {
            // Button i is pressed
            if (i == app.joystickMapping.rotate_cw_button && isGameActive) {
                app.board->rotatePiece(true);
            } else if (i == app.joystickMapping.rotate_ccw_button && isGameActive) {
                app.board->rotatePiece(false);
            } else if (i == app.joystickMapping.hard_drop_button && isGameActive) {
                app.board->hardDropPiece();
            } else if (i == app.joystickMapping.pause_button) {
                app.board->togglePause();
            }
        }
    }
}

void app_update(TetrimoneApp& app) {
    uint32_t currentTime = SDL_GetTicks();
    uint32_t deltaTime = currentTime - app.lastFrameTime;
    app.lastFrameTime = currentTime;

    // Cap delta time to prevent huge jumps
    if (deltaTime > 50) deltaTime = 50;

    // Update FPS
    app.frameCount++;
    static uint32_t fpsUpdateTime = 0;
    if (currentTime - fpsUpdateTime >= 1000) {
        app.fps = app.frameCount * 1000.0f / (currentTime - fpsUpdateTime);
        app.frameCount = 0;
        fpsUpdateTime = currentTime;
    }

    // Handle keyboard repeat
    if (keyDownPressed && keyDownRepeatTime > 0 && currentTime >= keyDownRepeatTime) {
        if (!app.board->isPaused() && !app.board->isGameOver() && !app.board->isSplashScreenActive()) {
            app.board->movePiece(0, 1);
            keyDownRepeatTime = currentTime + KEY_REPEAT_INTERVAL;
        }
    }

    if (keyLeftPressed && keyLeftRepeatTime > 0 && currentTime >= keyLeftRepeatTime) {
        if (!app.board->isPaused() && !app.board->isGameOver() && !app.board->isSplashScreenActive()) {
            app.board->movePiece(-1, 0);
            keyLeftRepeatTime = currentTime + KEY_REPEAT_INTERVAL;
        }
    }

    if (keyRightPressed && keyRightRepeatTime > 0 && currentTime >= keyRightRepeatTime) {
        if (!app.board->isPaused() && !app.board->isGameOver() && !app.board->isSplashScreenActive()) {
            app.board->movePiece(1, 0);
            keyRightRepeatTime = currentTime + KEY_REPEAT_INTERVAL;
        }
    }

    // Update game state
    app.board->update(deltaTime);
}

void app_render(TetrimoneApp& app) {
    int windowWidth, windowHeight;
    SDL_GetWindowSize(app.window, &windowWidth, &windowHeight);

    // Clear screen
    SDL_SetRenderDrawColor(app.renderer, 30, 30, 35, 255);
    SDL_RenderClear(app.renderer);

    // Calculate layout
    int gameAreaWidth = windowWidth - SIDEBAR_WIDTH - WINDOW_PADDING * 3;
    int gameAreaHeight = windowHeight - WINDOW_PADDING * 2;
    int gameAreaX = WINDOW_PADDING;
    int gameAreaY = WINDOW_PADDING;
    int sidebarX = gameAreaX + gameAreaWidth + WINDOW_PADDING;
    int sidebarY = gameAreaY;
    int sidebarWidth = SIDEBAR_WIDTH;
    int sidebarHeight = gameAreaHeight;

    // Draw game area
    render_game_area(app.renderer, app, gameAreaX, gameAreaY, gameAreaWidth, gameAreaHeight);

    // Draw sidebar (info, next pieces, etc.)
    render_sidebar(app.renderer, app, sidebarX, sidebarY, sidebarWidth, sidebarHeight);

    // Draw dialogs on top
    switch (app.activeDialog) {
        case TetrimoneApp::DialogState::MAIN_MENU:
            render_menu(app.renderer, app);
            break;
        case TetrimoneApp::DialogState::PAUSE_MENU:
            render_pause_menu(app.renderer, app);
            break;
        case TetrimoneApp::DialogState::GAME_OVER:
            render_game_over_screen(app.renderer, app);
            break;
        default:
            break;
    }

    // Present
    SDL_RenderPresent(app.renderer);
}

int tetrimone_main(int argc, char* argv[]) {
    TetrimoneApp app = {};
    
    // Parse command line arguments (would be implemented based on your existing code)
    // For now, just initialize with defaults

    app_init(app);

    if (!app.running) {
        app_cleanup(app);
        return 1;
    }

    // Main game loop
    while (app.running) {
        app_handle_events(app);
        app_update(app);
        app_render(app);
    }

    app_cleanup(app);
    return 0;
}

int main(int argc, char* argv[]) {
    return tetrimone_main(argc, argv);
}
