#include "tetrimone.h"
#include <iostream>
#include <cstring>
#include <algorithm>

// ============================================================================
// MINIMAL TETRIMONE - GAME BOARD ONLY, NO UI
// ============================================================================

// TetrimoneBlock implementation
TetrimoneBlock::TetrimoneBlock(int type) : type(type), rotation(0), x(0), y(0) {}

void TetrimoneBlock::rotate(bool clockwise) {
    rotation = (rotation + (clockwise ? 1 : 3)) % 4;
}

int TetrimoneBlock::getRotation() const {
    return rotation;
}

void TetrimoneBlock::move(int dx, int dy) {
    x += dx;
    y += dy;
}

std::vector<std::vector<int>> TetrimoneBlock::getShape() const {
    if (type >= 0 && type < (int)TETRIMONEBLOCK_SHAPES.size()) {
        return TETRIMONEBLOCK_SHAPES[type][rotation];
    }
    return std::vector<std::vector<int>>();
}

std::array<double, 3> TetrimoneBlock::getColor() const {
    int themeIndex = 0;  // Default theme
    if (themeIndex >= (int)TETRIMONEBLOCK_COLOR_THEMES.size()) {
        themeIndex = 0;
    }
    if (type >= 0 && type < (int)TETRIMONEBLOCK_COLOR_THEMES[themeIndex].size()) {
        return TETRIMONEBLOCK_COLOR_THEMES[themeIndex][type];
    }
    return {1.0, 1.0, 1.0};  // White fallback
}

void TetrimoneBlock::setPosition(int newX, int newY) {
    x = newX;
    y = newY;
}

// ============================================================================
// TetrimoneBoard - Core Game Logic Only
// ============================================================================

TetrimoneBoard::TetrimoneBoard() 
    : score(0), level(1), linesCleared(0), gameOver(false), paused(false),
      ghostPieceEnabled(true), splashScreenActive(true), heatLevel(0.0f),
      currentThemeIndex(0), isThemeTransitioning(false), 
      isLineClearActive(false), lineClearProgress(0.0f),
      smoothMovementTimer(0), lineClearAnimationTimer(0),
      themeTransitionTimer(0), heatDecayTimer(0),
      movementProgress(0.0f), fireworksActive(false),
      trailUpdateTimer(0), isTransitioning(false), transitionTimerId(0),
      propagandaTimerId(0), propagandaScaleTimerId(0),
      showPropagandaMessage(false), propagandaScalingUp(true),
      retroModeActive(false), patrioticModeActive(false),
      useBackgroundImage(false), useBackgroundZip(false),
      backgroundOpacity(0.5), simpleBlocksMode(false),
      junkLinesPercentage(0), junkLinesPerLevel(0), initialLevel(1),
      gridLinesEnabled(false), consecutiveClears(0), maxConsecutiveClears(0),
      lastClearCount(0), sequenceActive(false), currentAnimationType(0),
      trailsEnabled(false), maxTrailSegments(5), trailOpacity(0.6),
      trailDuration(0.5), backgroundMusic(NULL), isInBackgroundTransition(false),
      app(nullptr) {
    
    grid.resize(GRID_HEIGHT, std::vector<int>(GRID_WIDTH, 0));
    rng.seed(std::time(nullptr));
    
    // Create initial pieces
    nextPieces.push_back(std::make_unique<TetrimoneBlock>(rng() % 7));
    nextPieces.push_back(std::make_unique<TetrimoneBlock>(rng() % 7));
    nextPieces.push_back(std::make_unique<TetrimoneBlock>(rng() % 7));
    
    spawnNewPiece();
}

TetrimoneBoard::~TetrimoneBoard() {
    // Cleanup timers if they exist
}

void TetrimoneBoard::spawnNewPiece() {
    if (!nextPieces.empty()) {
        currentPiece = std::move(nextPieces[0]);
        nextPieces.erase(nextPieces.begin());
        nextPieces.push_back(std::make_unique<TetrimoneBlock>(rng() % 7));
    }
    
    if (currentPiece) {
        currentPiece->setPosition(GRID_WIDTH / 2 - 2, 0);
        if (checkCollision(*currentPiece)) {
            gameOver = true;
        }
    }
}

bool TetrimoneBoard::checkCollision(const TetrimoneBlock& piece) const {
    if (!piece) return false;
    
    auto shape = piece.getShape();
    for (int y = 0; y < (int)shape.size(); y++) {
        for (int x = 0; x < (int)shape[y].size(); x++) {
            if (shape[y][x] == 0) continue;
            
            int gridX = piece.getX() + x;
            int gridY = piece.getY() + y;
            
            if (gridX < 0 || gridX >= GRID_WIDTH || gridY >= GRID_HEIGHT) {
                return true;
            }
            if (gridY >= 0 && grid[gridY][gridX] != 0) {
                return true;
            }
        }
    }
    return false;
}

void TetrimoneBoard::moveLeft() {
    if (!currentPiece || paused || gameOver) return;
    currentPiece->move(-1, 0);
    if (checkCollision(*currentPiece)) {
        currentPiece->move(1, 0);
    }
}

void TetrimoneBoard::moveRight() {
    if (!currentPiece || paused || gameOver) return;
    currentPiece->move(1, 0);
    if (checkCollision(*currentPiece)) {
        currentPiece->move(-1, 0);
    }
}

void TetrimoneBoard::moveDown() {
    if (!currentPiece || paused || gameOver) return;
    currentPiece->move(0, 1);
    if (checkCollision(*currentPiece)) {
        currentPiece->move(0, -1);
        placePiece();
        int clearedLines = clearLines();
        score += clearedLines * 100;
        linesCleared += clearedLines;
        spawnNewPiece();
    }
}

void TetrimoneBoard::rotatePiece() {
    if (!currentPiece || paused || gameOver) return;
    currentPiece->rotate();
    if (checkCollision(*currentPiece)) {
        currentPiece->rotate(false);
    }
}

void TetrimoneBoard::hardDrop() {
    if (!currentPiece || paused || gameOver) return;
    while (!checkCollision(*currentPiece)) {
        currentPiece->move(0, 1);
    }
    currentPiece->move(0, -1);
    placePiece();
    int clearedLines = clearLines();
    score += clearedLines * 100;
    linesCleared += clearedLines;
    spawnNewPiece();
}

void TetrimoneBoard::placePiece() {
    if (!currentPiece) return;
    
    auto shape = currentPiece->getShape();
    for (int y = 0; y < (int)shape.size(); y++) {
        for (int x = 0; x < (int)shape[y].size(); x++) {
            if (shape[y][x] == 0) continue;
            
            int gridX = currentPiece->getX() + x;
            int gridY = currentPiece->getY() + y;
            
            if (gridY >= 0 && gridY < GRID_HEIGHT && gridX >= 0 && gridX < GRID_WIDTH) {
                grid[gridY][gridX] = currentPiece->getType() + 1;
            }
        }
    }
}

int TetrimoneBoard::clearLines() {
    std::vector<int> linesToClear;
    
    for (int y = 0; y < GRID_HEIGHT; y++) {
        bool full = true;
        for (int x = 0; x < GRID_WIDTH; x++) {
            if (grid[y][x] == 0) {
                full = false;
                break;
            }
        }
        if (full) {
            linesToClear.push_back(y);
        }
    }
    
    // Remove cleared lines (bottom to top)
    std::sort(linesToClear.begin(), linesToClear.end(), std::greater<int>());
    for (int y : linesToClear) {
        for (int moveY = y; moveY > 0; moveY--) {
            for (int x = 0; x < GRID_WIDTH; x++) {
                grid[moveY][x] = grid[moveY - 1][x];
            }
        }
        for (int x = 0; x < GRID_WIDTH; x++) {
            grid[0][x] = 0;
        }
    }
    
    return linesToClear.size();
}

void TetrimoneBoard::togglePause() {
    paused = !paused;
}

void TetrimoneBoard::restart() {
    grid.assign(GRID_HEIGHT, std::vector<int>(GRID_WIDTH, 0));
    score = 0;
    level = 1;
    linesCleared = 0;
    gameOver = false;
    paused = false;
    nextPieces.clear();
    nextPieces.push_back(std::make_unique<TetrimoneBlock>(rng() % 7));
    nextPieces.push_back(std::make_unique<TetrimoneBlock>(rng() % 7));
    nextPieces.push_back(std::make_unique<TetrimoneBlock>(rng() % 7));
    spawnNewPiece();
}

// ============================================================================
// SDL EVENT HANDLER
// ============================================================================

void handleSDLEvent(const SDL_Event& event, TetrimoneApp* app) {
    if (!app || !app->board) return;
    
    switch(event.type) {
        case SDL_KEYDOWN:
            switch(event.key.keysym.sym) {
                case SDLK_LEFT:
                    app->board->moveLeft();
                    break;
                case SDLK_RIGHT:
                    app->board->moveRight();
                    break;
                case SDLK_DOWN:
                    app->board->moveDown();
                    break;
                case SDLK_UP:
                case SDLK_z:
                    app->board->rotatePiece();
                    break;
                case SDLK_x:
                    app->board->rotatePiece(false);  // Counter-clockwise
                    break;
                case SDLK_SPACE:
                    app->board->hardDrop();
                    break;
                case SDLK_p:
                    app->board->togglePause();
                    break;
                default:
                    break;
            }
            break;
    }
}

// ============================================================================
// GAME UPDATE
// ============================================================================

bool onTimerTick(TetrimoneApp* app) {
    if (!app || !app->board) return true;
    
    if (!app->board->isPaused() && !app->board->isGameOver()) {
        app->board->moveDown();
        app->needsRedraw = true;
    }
    
    return true;
}

// ============================================================================
// STUBS FOR LINKING
// ============================================================================

void updateLabels(TetrimoneApp* app) {}
void startGame(TetrimoneApp* app) {}
void pauseGame(TetrimoneApp* app) {}
void resetUI(TetrimoneApp* app) {}
void shutdownApp(TetrimoneApp* app) {}
void onStartGame(TetrimoneApp* app) { startGame(app); }
void onPauseGame(TetrimoneApp* app) { pauseGame(app); }
void onRestartGame(TetrimoneApp* app) { if (app && app->board) app->board->restart(); }
void onQuitGame(TetrimoneApp* app) {}
void onSoundToggled(bool enabled, void* userData) {}
void adjustDropSpeed(TetrimoneApp* app) {}
void calculateBlockSize(TetrimoneApp* app) {}
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
std::array<double, 3> getHeatModifiedColor(const std::array<double, 3>& baseColor, float heatLevel) {
    return baseColor;
}
cairo_surface_t* cairo_image_surface_create_from_jpeg(const char* filename) { return nullptr; }
cairo_surface_t* cairo_image_surface_create_from_memory(const void* data, size_t length) { return nullptr; }

