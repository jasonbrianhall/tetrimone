#ifndef TETRIMONE_CORE_H
#define TETRIMONE_CORE_H

#include <vector>
#include <array>
#include <deque>
#include <random>
#include <chrono>
#include <memory>
#include <atomic>
#include <string>
#include "audiomanager.h"
#include <SDL2/SDL.h>
#include <cairo/cairo.h>

#ifdef GTK3
    #include <glib.h>
#else
    class QTimer;
#endif

#include "themes.h"
#include "tetrimoneblock.h"
#include "highscores.h"
#include "propaganda_messages.h"

struct FireworkParticle {
    double x, y, vx, vy, life, maxLife, size, gravity, fade;
    std::array<double, 3> color;
};

struct BlockTrail {
    double x, y, life, maxLife, alpha;
    int rotation, pieceType;
    std::array<double, 3> color;
    std::vector<std::vector<int>> shape;
};

struct LineClearAnimValues {
    double alpha, scale, offsetX, offsetY;
};

enum class GameSoundEvent {
    BackgroundMusic, BackgroundMusic2, BackgroundMusic3, BackgroundMusic4, BackgroundMusic5,
    Single, Double, Triple, Gameover, GameoverRetro, Clear, Drop, LateralMove,
    LevelUp, LevelUpRetro, Rotate, Select, Start, Tetrimone, Excellent,
    BackgroundMusicRetro, BackgroundMusic2Retro, BackgroundMusic3Retro, BackgroundMusic4Retro, BackgroundMusic5Retro,
    PatrioticMusicRetro, PatrioticMusic2Retro, PatrioticMusic3Retro, PatrioticMusic4Retro, PatrioticMusic5Retro
};

typedef struct {
    int rotate_cw_button, rotate_ccw_button, hard_drop_button, pause_button;
    int x_axis, y_axis;
    bool invert_x, invert_y;
} JoystickMapping;

extern int currentThemeIndex, GRID_WIDTH, GRID_HEIGHT, BLOCK_SIZE;
const int MIN_GRID_WIDTH = 8, MAX_GRID_WIDTH = 16, MIN_GRID_HEIGHT = 16, MAX_GRID_HEIGHT = 30;
const int MIN_BLOCK_SIZE = 20, MAX_BLOCK_SIZE = 80, INITIAL_SPEED = 500;

class TetrimoneBlock;
class TetrimoneBoard;
struct TetrimoneApp;

class TetrimoneBlock {
private:
    int type, rotation, x, y;
public:
    TetrimoneBlock(int type);
    void rotate(bool clockwise = true);
    int getRotation() const;
    void move(int dx, int dy);
    std::vector<std::vector<int>> getShape() const;
    std::array<double, 3> getColor() const;
    int getType() const { return type; }
    int getX() const { return x; }
    int getY() const { return y; }
    void setPosition(int newX, int newY);
    bool isValid() const { 
        return type >= 0 && type < 14 && rotation >= 0 && rotation < 4; 
    }
};

class TetrimoneBoard {
private:
    // Core game state
    float heatLevel;
    #ifdef QT5
        QTimer* heatDecayTimer = nullptr;
    #endif
    
    #ifdef GTK3
        unsigned int heatDecayTimer;
    #endif
    std::vector<std::vector<int>> grid;
    std::unique_ptr<TetrimoneBlock> currentPiece;
    std::deque<std::unique_ptr<TetrimoneBlock>> nextPieces;
    int score, level, linesCleared;
    bool gameOver, paused;
    bool gameOverSoundPlayed = false;  // Ensures game over sound plays only once
    std::mt19937 rng;
    bool splashScreenActive;
    std::atomic<bool> musicStopFlag{false};
    int minBlockSize = 4;
    int gridWidth = GRID_WIDTH, gridHeight = GRID_HEIGHT;
    bool ghostPieceEnabled;
    int consecutiveClears, maxConsecutiveClears, lastClearCount;
    bool sequenceActive;
    Highscores highScores;
    int currentAnimationType;

    // Theme transition
    bool isThemeTransitioning;
    double themeTransitionProgress;
    int oldThemeIndex, newThemeIndex;
    static const int THEME_TRANSITION_DURATION = 3000;

    // Line clear animation
    bool lineClearActive;
    std::vector<int> linesBeingCleared;
    double lineClearProgress;
    static const int LINE_CLEAR_ANIMATION_DURATION = 600;
    std::chrono::high_resolution_clock::time_point lineClearStartTime;

    // Smooth movement
    double currentPieceInterpolatedX, currentPieceInterpolatedY;
    int lastPieceX, lastPieceY;
    double movementProgress;
    static const int MOVEMENT_ANIMATION_DURATION = 100;
    std::chrono::high_resolution_clock::time_point movementStartTime, themeStartTime;

    // Fireworks
    bool fireworksActive;
    std::vector<FireworkParticle> fireworkParticles;
    std::chrono::high_resolution_clock::time_point fireworksStartTime;
    static const int FIREWORKS_DURATION = 2000;
    int fireworksType;

    // Block trails
    bool trailsEnabled;
    std::vector<BlockTrail> blockTrails;
    std::chrono::high_resolution_clock::time_point lastTrailTime;
    int maxTrailSegments;
    double trailOpacity, trailDuration;
    static const int TRAIL_UPDATE_INTERVAL = 16;
    static constexpr double TRAIL_SPAWN_DELAY = 120.0;

    // Background transition
    bool isTransitioning;
    double transitionOpacity;
    int transitionDirection;
    void* oldBackground;

    // Platform-specific timer members (declared in tetrimone_gtk.h or tetrimone_qt5.h)
    #ifdef GTK3
        unsigned int smoothMovementTimer = 0;
        unsigned int lineClearAnimationTimer = 0;
        unsigned int themeTransitionTimer = 0;
        unsigned int fireworksTimer = 0;
        unsigned int trailUpdateTimer = 0;
        unsigned int propagandaTimerId = 0;
        unsigned int propagandaScaleTimerId = 0;
        unsigned int backgroundImageTimer = 0;
        unsigned int transitionTimerId = 0;
    #endif
    
    #ifdef QT5
        QTimer* smoothMovementTimer = nullptr;
        QTimer* lineClearAnimationTimer = nullptr;
        QTimer* themeTransitionTimer = nullptr;
        QTimer* fireworksTimer = nullptr;
        QTimer* trailUpdateTimer = nullptr;
        QTimer* propagandaTimerId = nullptr;
        QTimer* propagandaScaleTimerId = nullptr;
        QTimer* backgroundImageTimer = nullptr;
        QTimer* transitionTimerId = nullptr;
    #endif

public:
    // Propaganda messages
    int propagandaMessageDuration;
    std::string currentPropagandaMessage;
    double propagandaMessageScale;
    bool propagandaScalingUp;
    bool showPropagandaMessage;
    int currentPatriotBackgroundIndex;

    // Background images and state
    void* backgroundImage;
    double backgroundOpacity;
    int currentBackgroundIndex, currentPatrioticBackgroundIndex;
    bool useBackgroundZip, useBackgroundImage;
    void setUseBackgroundImage(bool use) { useBackgroundImage = use; }
    void setUseBackgroundZip(bool use) { useBackgroundZip = use; }

    // Status flags (public for callback access)
    bool musicPaused = false;
    bool highScoreAlreadyProcessed = false;
    TetrimoneApp* app;

    // Public mode flags and data
    bool simpleBlocksActive = false;
    bool retroModeActive = false;
    bool patrioticModeActive = false;
    bool retroMusicActive = false;
    bool showGridLines = false;
    bool sound_enabled_ = true;
    std::string backgroundImagePath, sounds_zip_path_ = "sound.zip";
    bool enabledTracks[5];
    int junkLinesPercentage = 0, junkLinesPerLevel = 0, initialLevel = 1;
    
    // Background images (public for callback access)
    std::vector<cairo_surface_t*> patriotBackgroundImages;
    std::vector<cairo_surface_t*> backgroundImages;

    std::string backgroundZipPath;

    // Constructors
    TetrimoneBoard();
    TetrimoneBoard(int width, int height);
    ~TetrimoneBoard();

    // Core game methods
    void initialize();
    void spawnNewPiece();
    bool canPlace(const TetrimoneBlock& block, int x, int y) const;
    void placePiece();
    bool moveLeft(), moveRight(), moveDown();
    bool rotateCW(), rotateCCW();
    void hardDrop();
    void clearFullLines();
    void updateGameState();
    bool movePiece(int dx, int dy);
    bool rotatePiece(bool clockwise);
    bool checkCollision(const TetrimoneBlock &piece) const;
    void lockPiece();
    int clearLines();
    void generateNewPiece();
    void updateGame();
    void restart();

    // Junk lines
    void fillJunkRows(int startRow, int endRow);
    std::vector<int> generateRandomPositions(int count, int maxWidth);
    int selectNextBlockType(int currentType, int currentTypeCount);
    void shiftGridContentUp(int numRows);
    void shiftCurrentPieceUp(int numRows);
    void repositionPieceAboveJunk(TetrimoneBlock* piece, int junkStartRow);
    void ensureValidPiecePosition();

    // Grid/State
    int getGridValue(int x, int y) const;
    const std::vector<std::vector<int>>& getGrid() const { return grid; }
    void dismissSplashScreen();
    void togglePause() { paused = !paused; }
    void generateJunkLines(int percentage);
    void addJunkLinesFromBottom(int count);

    // Score/Status
    int getScore() const { return score; }
    int getLevel() const { return level; }
    int getLinesCleared() const { return linesCleared; }
    bool isGameOver() const;
    bool isPaused() const { return paused; }
    void setPaused(bool p) { paused = p; }
    void setGameOver(bool g) { gameOver = g; }

    // Pieces
    const TetrimoneBlock* getCurrentPiece() const { 
      if (!currentPiece) {
        fprintf(stderr, "ERROR: currentPiece is null!\n");
        return nullptr;
      }
      // Always return the piece, even if invalid - let drawing code handle it
      if (!currentPiece->isValid()) {
        fprintf(stderr, "WARNING: currentPiece is invalid! type=%d, rotation=%d\n", 
                currentPiece->getType(), currentPiece->getRotation());
      }
      return currentPiece.get(); 
    }
    const TetrimoneBlock& getCurrentPieceRef() const { 
      if (!currentPiece) {
        // This shouldn't happen, but safety fallback
        static TetrimoneBlock fallback(0);
        return fallback;
      }
      return *currentPiece; 
    }
    const TetrimoneBlock* getNextPiece(int index = 0) const { 
      if (index < 0 || index >= (int)nextPieces.size()) {
        return nullptr;
      }
      return nextPieces[index].get();
    }
    
    const std::deque<std::unique_ptr<TetrimoneBlock>>& getNextPieces() const { 
      return nextPieces; 
    }
    
    size_t getNextPiecesCount() const {
      return nextPieces.size();
    }

    // Heat
    float getHeatLevel();
    void setHeatLevel(float level);
    void decreaseHeat(float amount);
    void increaseHeat(float amount);
    void coolDown();
    void updateHeat();

    void setGridDimensions(int width, int height);

    // Ghost piece
    bool isGhostPieceEnabled() const { return ghostPieceEnabled; }
    void setGhostPieceEnabled(bool enabled) { ghostPieceEnabled = enabled; }
    int getGhostPieceY() const;

    // Block size
    int getMinBlockSize() const { return minBlockSize; }
    void setMinBlockSize(int size) { 
      // Clamp value between 1 and 4
      minBlockSize = std::max(1, std::min(size, 4)); 
      // Note: This may require regenerating the next pieces queue
      // if called during gameplay
    }
    
    // Safe difficulty setter for GUI - implemented in tetrimone.cpp
    void setDifficultyFromGUI(int newDifficulty);
    
    // Safe block size setter for GUI
    void setMinBlockSizeFromGUI(int size) {
      if (size < 1 || size > 4) {
        return; // Invalid block size
      }
      setMinBlockSize(size);
    }

    // Splash screen
    bool isSplashScreenActive() const { return splashScreenActive; }
    void setSplashScreenActive(bool active) { splashScreenActive = active; }

    // Theme transition
    void startThemeTransition(int newTheme);
    void updateThemeTransition();
    void cancelThemeTransition();
    bool isThemeTransitionActive() const { return isThemeTransitioning; }
    double getThemeTransitionProgress() const { return themeTransitionProgress; }
    int getOldThemeIndex() const { return oldThemeIndex; }
    int getNewThemeIndex() const { return newThemeIndex; }
    std::array<double, 3> getInterpolatedColor(int blockType, double progress) const;
    int getCurrentAnimationType() const { return currentAnimationType; }

    // Background management
    const std::string& getBackgroundImagePath() const { return backgroundImagePath; }
    const void* getBackgroundImage() const { return backgroundImage; }
    void setBackgroundImage(void* surface) { backgroundImage = surface; }
    double getBackgroundOpacity() const { return backgroundOpacity; }
    void setBackgroundOpacity(double opacity) { backgroundOpacity = std::max(0.0, std::min(1.0, opacity)); }
    bool isUsingBackgroundImage() const { return useBackgroundImage; }
    bool isUsingBackgroundZip() const { return useBackgroundZip; }
    bool loadBackgroundImage(const std::string& imagePath);
    bool loadBackgroundImagesFromZip(const std::string& zipPath);
    void selectRandomBackground();
    void cleanupBackgroundImages();

    // Grid lines and difficulty
    void setShowGridLines(bool show) { showGridLines = show; }
    bool isShowingGridLines() const { return showGridLines; }
    std::string getDifficultyText(int difficulty) const;
    void setLevel(int newLevel);
    void setMinBlock(int size);

    // Consecutive clears (combo)
    int getConsecutiveClears() const { return consecutiveClears; }
    int getMaxConsecutiveClears() const { return maxConsecutiveClears; }
    bool isSequenceActive() const { return sequenceActive; }

    // Propaganda
    bool isShowingPropagandaMessage() const { return showPropagandaMessage; }
    const std::string& getCurrentPropagandaMessage() const { return currentPropagandaMessage; }

    // Line clear animation
    bool isLineClearActive() const { return lineClearActive; }
    bool isLineBeingCleared(int y) const;
    double getLineClearProgress() const { return lineClearProgress; }
    void getCurrentPieceInterpolatedPosition(double &x, double &y) const;
    void startLineClearAnimation(const std::vector<int> &clearedLines);
    void updateLineClearAnimation();

    // Movement animation
    void startSmoothMovement(int newX, int newY);
    void updateSmoothMovement();

    // Fireworks
    void startFireworksAnimation(int linesCleared);
    void updateFireworksAnimation();
    void createFireworkBurst(double centerX, double centerY, const std::array<double, 3>& baseColor, int particleCount);
    bool isFireworksActive() const { return fireworksActive; }
    const std::vector<FireworkParticle>& getFireworkParticles() const { return fireworkParticles; }

    // Block trails
    void setTrailsEnabled(bool enabled) { trailsEnabled = enabled; }
    bool isTrailsEnabled() const { return trailsEnabled; }
    void setMaxTrailSegments(int segments) { maxTrailSegments = std::max(1, std::min(segments, 15)); }
    int getMaxTrailSegments() const { return maxTrailSegments; }
    void setTrailOpacity(double opacity) { trailOpacity = std::max(0.1, std::min(1.0, opacity)); }
    double getTrailOpacity() const { return trailOpacity; }
    void setTrailDuration(double duration) { trailDuration = std::max(0.05, std::min(2.0, duration)); }
    double getTrailDuration() const { return trailDuration; }
    void updateBlockTrails();
    void createBlockTrail();
    bool isBlockTrailsActive() const { return !blockTrails.empty(); }
    const std::vector<BlockTrail>& getBlockTrails() const { return blockTrails; }

    // Background transition
    void startBackgroundTransition();
    void updateBackgroundTransition();
    void cancelBackgroundTransition();
    bool isInBackgroundTransition() const { return isTransitioning; }
    double getTransitionOpacity() const { return transitionOpacity; }
    void* getOldBackground() const { return oldBackground; }
    int getTransitionDirection() const { return transitionDirection; }

    // Game setup
    int getJunkLinesPercentage() const { return junkLinesPercentage; }
    void setJunkLinesPercentage(int p) { junkLinesPercentage = p; }
    int getJunkLinesPerLevel() const { return junkLinesPerLevel; }
    void setJunkLinesPerLevel(int l) { junkLinesPerLevel = l; }
    bool isSoundEnabled() const { return sound_enabled_; }
    void setSoundEnabled(bool enabled) { sound_enabled_ = enabled; }

    // High scores
    bool checkAndRecordHighScore(TetrimoneApp* app);
    const Highscores& getHighScores() const { return highScores; }

    // Audio
    void playSound(GameSoundEvent event);
    void resumeBackgroundMusic();
    void pauseBackgroundMusic();
    bool initializeAudio();
    void cleanupAudio();
    void playBackgroundMusic();
    bool extractFileFromZip(const std::string& zipFilePath, const std::string& fileName, std::vector<unsigned char>& data);
    std::string getCacheFilePath(const std::string& soundFileName);
    bool loadCachedWav(const std::string& cacheFilePath, std::vector<unsigned char>& wavData);
    bool saveCachedWav(const std::string& cacheFilePath, const std::vector<unsigned char>& wavData);
    bool loadSoundFromZip(GameSoundEvent event, const std::string& soundFileName);
    bool setSoundsZipPath(const std::string& path);

    void setApp(TetrimoneApp* appPtr) { app = appPtr; }
    bool isInThemeTransition() const { return isThemeTransitioning; }
};

// Platform-agnostic utility and UI functions
std::array<double, 3> getHeatModifiedColor(const std::array<double, 3>& baseColor, float heatLevel);

void updateLabels(TetrimoneApp* app);
void startGame(TetrimoneApp* app);
void pauseGame(TetrimoneApp* app);
void resetUI(TetrimoneApp* app);
void cleanupApp(TetrimoneApp* app);
void initSDL(TetrimoneApp* app);
void shutdownSDL(TetrimoneApp* app);
void updateDisplay(TetrimoneApp *app);
void drawBoard(TetrimoneBoard *board);
void drawNextPieceArea(TetrimoneBoard *board);
void rebuildGameUI(TetrimoneApp* app);
void showIdeologicalFailureDialog(TetrimoneApp* app);
void showPatrioticPerformanceDialog(TetrimoneApp* app);
void saveJoystickMapping(TetrimoneApp* app);
void loadJoystickMapping(TetrimoneApp* app);
bool saveGameSettings(TetrimoneApp* app);
bool loadGameSettings(TetrimoneApp* app);
void resetGameSettings(TetrimoneApp* app);
void adjustDropSpeed(TetrimoneApp* app);
void calculateBlockSize(TetrimoneApp* app);


// UI state management
void ui_set_sound_enabled(TetrimoneApp *app, bool enabled);
void ui_set_active_theme(TetrimoneApp *app, int index);
void ui_set_background_enabled(TetrimoneApp *app, bool enabled);
void ui_set_window_title(TetrimoneApp *app, const char *title);
void ui_window_fullscreen(TetrimoneApp *app);
void ui_set_difficulty_label(TetrimoneApp *app, const char *markup);
void ui_set_pause_menu_label(TetrimoneApp *app, const char *text);
void ui_set_isusingbackgroundimage_enabled(TetrimoneApp *app);
void set_difficulty_menu(TetrimoneApp *app, int difficulty);
void ui_update_track_menu(TetrimoneApp *app);
void ui_set_mediumMenuItem_enabled(TetrimoneApp *app, bool enabled);
void app_set_track_items_active(TetrimoneApp* app, int count, bool active);

// Drawing functions - Cairo version (implementation of platform-agnostic interface)
void drawGameOver(cairo_t *cr, TetrimoneBoard *board);
void drawBlockTrails(cairo_t *cr, TetrimoneBoard *board);
void drawFireworks(cairo_t *cr, TetrimoneBoard *board, TetrimoneApp *app);
void drawPauseMenu(cairo_t *cr, TetrimoneBoard *board);
void drawGridLines(cairo_t *cr, TetrimoneBoard *board);
void drawFailureLine(cairo_t *cr);
void drawSplashScreen(cairo_t *cr, TetrimoneBoard *board, TetrimoneApp *app);
void drawGhostPiece(cairo_t *cr, TetrimoneBoard *board);
void drawCurrentPiece(cairo_t *cr, TetrimoneBoard *board);
void drawPropagandaMessage(cairo_t *cr, TetrimoneBoard *board);

// GTK JPEG image loading utilities
cairo_surface_t* cairo_image_surface_create_from_jpeg(const char* filename);
cairo_surface_t* cairo_image_surface_create_from_memory(const void* data, size_t length);

// Heat effect functions - Cairo versions (legacy)
void drawFreezyEffect(cairo_t* cr, double x, double y, double size, float heatLevel, double time);
void drawFireyGlow(cairo_t* cr, double x, double y, double size, float heatLevel, double time);


// Helper function
LineClearAnimValues getLineClearAnimationValues(int animationType, double progress, int x, int y);

#include "commandline.h"
int ui_run_application(int argc, char *argv[], TetrimoneApp *app, const CommandLineArgs *args);

#endif // TETRIMONE_CORE_H
