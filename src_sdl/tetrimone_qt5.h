#ifndef TETRIMONE_QT5_H
#define TETRIMONE_QT5_H

#include <vector>
#include <array>
#include <random>
#include <chrono>
#include <memory>
#include <atomic>
#include <string>
#include "audiomanager.h"
#include <SDL2/SDL.h>
#include "themes.h"
#include "tetrimoneblock.h"
#include "highscores.h"
#include "propaganda_messages.h"

#include <QApplication>
#include <QWidget>
#include <QLabel>
#include <QMenuBar>
#include <QMenu>
#include <QAction>
#include <QActionGroup>
#include <QTimer>


// Ensure FireworkParticle and BlockTrail are defined
// These should come from a particles header or be defined here
struct FireworkParticle {
    double x, y;
    double vx, vy;
    double life;
    double maxLife;
    std::array<double, 3> color;
    double size;
    double gravity;
    double fade;
};

struct BlockTrail {
    double x, y;
    int rotation;
    int pieceType;
    double life;
    double maxLife;
    double alpha;
    std::array<double, 3> color;
    std::vector<std::vector<int>> shape;
};

// Animation value struct
struct LineClearAnimValues {
  double alpha;
  double scale;
  double offsetX;
  double offsetY;
};

enum class GameSoundEvent {
  BackgroundMusic,
  BackgroundMusic2,
  BackgroundMusic3,
  BackgroundMusic4,
  BackgroundMusic5,
  Single,
  Double,
  Triple,
  Gameover,
  GameoverRetro,
  Clear,
  Drop,
  LateralMove,
  LevelUp,
  LevelUpRetro,
  Rotate,
  Select,
  Start,
  Tetrimone,
  Excellent,
  BackgroundMusicRetro,
  BackgroundMusic2Retro,
  BackgroundMusic3Retro,
  BackgroundMusic4Retro,
  BackgroundMusic5Retro,
  PatrioticMusicRetro,
  PatrioticMusic2Retro,
  PatrioticMusic3Retro,
  PatrioticMusic4Retro,
  PatrioticMusic5Retro,
};

extern int currentThemeIndex;

// Forward declarations
class TetrimoneBlock;
class TetrimoneBoard;
struct TetrimoneApp;

// Constants
extern int GRID_WIDTH;
extern int GRID_HEIGHT;

const int MIN_GRID_WIDTH = 8;
const int MAX_GRID_WIDTH = 16;
const int MIN_GRID_HEIGHT = 16;
const int MAX_GRID_HEIGHT = 30;

extern int BLOCK_SIZE;
const int MIN_BLOCK_SIZE = 20;
const int MAX_BLOCK_SIZE = 80;

const int INITIAL_SPEED = 500;

// TetrimoneBlock class (same as GTK3)
class TetrimoneBlock {
private:
    int type;
    int rotation;
    int x, y;
    
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
};

typedef struct {
    int rotate_cw_button;
    int rotate_ccw_button;
    int hard_drop_button;
    int pause_button;
    int x_axis;
    int y_axis;
    bool invert_x;
    bool invert_y;
} JoystickMapping;

// Qt5-specific TetrimoneApp structure
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



// TetrimoneBoard - backend-agnostic core
class TetrimoneBoard {
private:
    float heatLevel;
    unsigned int heatDecayTimer;
    std::vector<std::vector<int>> grid;
    std::unique_ptr<TetrimoneBlock> currentPiece;
    std::vector<std::unique_ptr<TetrimoneBlock>> nextPieces;
    int score;
    int level;
    int linesCleared;
    bool gameOver;
    bool paused;
    std::mt19937 rng;
    bool splashScreenActive;
    std::atomic<bool> musicStopFlag{false};
    int minBlockSize = 1;
    int gridWidth = GRID_WIDTH;
    int gridHeight = GRID_HEIGHT;
    bool ghostPieceEnabled;
    int consecutiveClears;
    int maxConsecutiveClears;
    int lastClearCount;
    bool sequenceActive;
    Highscores highScores;
    int currentAnimationType;

    bool isThemeTransitioning;
    double themeTransitionOpacity;
    unsigned int themeTransitionTimerId;
    int oldThemeIndex;
    int newThemeIndex;

    TetrimoneApp* app;

    bool useBackgroundZip;
    void* backgroundImageSurface;
    double backgroundOpacity;
    std::vector<FireworkParticle> fireworkParticles;
    bool fireworksActive;
    unsigned int fireworksTimer;
    std::chrono::high_resolution_clock::time_point fireworksStartTime;
    int fireworksType;

    bool lineClearActive;
    std::vector<int> linesToClear;
    double lineClearProgress;
    unsigned int lineClearAnimationTimer;
    std::chrono::high_resolution_clock::time_point lineClearStartTime;

    int smoothMoveStartX;
    int smoothMoveStartY;
    int smoothMoveTargetX;
    int smoothMoveTargetY;
    unsigned int smoothMovementTimer;
    std::chrono::high_resolution_clock::time_point movementStartTime;

    bool trailsEnabled;
    int maxTrailSegments;
    double trailOpacity;
    double trailDuration;
    std::vector<BlockTrail> blockTrails;
    unsigned int trailUpdateTimer;
    std::chrono::high_resolution_clock::time_point lastTrailTime;

    std::string sounds_zip_path_ = "sound.zip";  // Path to sounds ZIP file

    bool showPropagandaMessage;
    unsigned int propagandaTimerId;
    int propagandaMessageDuration;
    std::string currentPropagandaMessage;
    double propagandaMessageScale;
    bool propagandaScalingUp;
    unsigned int propagandaScaleTimerId;

    bool isTransitioning;
    double transitionOpacity;
    int transitionDirection;
    void* oldBackground;
    unsigned int transitionTimerId;

    bool musicPaused;
    bool highScoreAlreadyProcessed;

public:
    TetrimoneBoard(int width = GRID_WIDTH, int height = GRID_HEIGHT);
    ~TetrimoneBoard();

    bool simpleBlocksActive = false;
    bool retroModeActive = false;  // Flag for retro mode
    bool patrioticModeActive = false;
    bool retroMusicActive = false;
    bool showGridLines = false; // Grid lines off by default
    bool sound_enabled_ = true;
    std::string backgroundImagePath;

    const std::string& getBackgroundImagePath() const { return backgroundImagePath; }
    void initialize();
    void spawnNewPiece();
    bool canPlace(const TetrimoneBlock& block, int x, int y) const;
    void placePiece();
    bool moveLeft();
    bool moveRight();
    bool moveDown();
    bool rotateCW();
    bool rotateCCW();
    bool hardDrop();
    void clearFullLines();
    void updateGameState();
    bool movePiece(int dx, int dy);
    bool rotatePiece(bool clockwise);
    bool checkCollision(const TetrimoneBlock &piece) const;
    void lockPiece();
    int clearLines();
    void generateNewPiece();
    void updateGame();

    void fillJunkRows(int startRow, int endRow);
    std::vector<int> generateRandomPositions(int count, int maxWidth);
    int selectNextBlockType(int currentType, int currentTypeCount);
    void shiftGridContentUp(int numRows);
    void shiftCurrentPieceUp(int numRows);
    void repositionPieceAboveJunk(TetrimoneBlock* piece, int junkStartRow);
    void ensureValidPiecePosition();



    int getGridValue(int x, int y) const;
    void restart();
    void dismissSplashScreen();
    void togglePause();
    void generateJunkLines(int percentage);
    void addJunkLinesFromBottom(int count);
    bool checkAndRecordHighScore(TetrimoneApp* app);
    void coolDown();
    void playSound(GameSoundEvent event);
    void resumeBackgroundMusic();
    void pauseBackgroundMusic();
    bool initializeAudio();
    void cleanupAudio();
    void playBackgroundMusic();
    void setShowGridLines(bool show);
    bool isShowingGridLines() const;
    void setUseBackgroundImage(bool use);
    bool isUsingBackgroundImage() const;
    std::string getDifficultyText(int difficulty) const;
    void cleanupBackgroundImages();
    void setLevel(int newLevel);
    void setMinBlock(int size);
    
    int getScore() const { return score; }
    int getLevel() const { return level; }
    int getLinesCleared() const { return linesCleared; }
    bool isGameOver() const { return gameOver; }
    bool isPaused() const { return paused; }
    void setPaused(bool p) { paused = p; }
    void setGameOver(bool g) { gameOver = g; }
    
    const std::vector<std::vector<int>>& getGrid() const { return grid; }
    const TetrimoneBlock* getCurrentPiece() const { return currentPiece.get(); }
    const std::vector<std::unique_ptr<TetrimoneBlock>>& getNextPieces() const { return nextPieces; }
    
    float getHeatLevel() const { return heatLevel; }
    void setHeatLevel(float level) { heatLevel = std::max(0.0f, std::min(1.0f, level)); }
    void decreaseHeat(float amount) { heatLevel = std::max(0.0f, heatLevel - amount); }
    void increaseHeat(float amount) { heatLevel = std::min(1.0f, heatLevel + amount); }
    
    int getGridWidth() const { return gridWidth; }
    int getGridHeight() const { return gridHeight; }
    void setGridDimensions(int width, int height);
    
    bool isGhostPieceEnabled() const { return ghostPieceEnabled; }
    void setGhostPieceEnabled(bool enabled) { ghostPieceEnabled = enabled; }
    
    int getMinBlockSize() const { return minBlockSize; }
    void setMinBlockSize(int size) { minBlockSize = size; }
    
    bool isSplashScreenActive() const { return splashScreenActive; }
    void setSplashScreenActive(bool active) { splashScreenActive = active; }
    
    void startThemeTransition(int newTheme);
    void updateThemeTransition();
    void cancelThemeTransition();
    bool isThemeTransitionActive() const { return isThemeTransitioning; }
    double getThemeTransitionOpacity() const { return themeTransitionOpacity; }
    int getOldThemeIndex() const { return oldThemeIndex; }
    int getNewThemeIndex() const { return newThemeIndex; }

    const void* getBackgroundImage() const { return backgroundImageSurface; }
    void setBackgroundImage(void* surface) { backgroundImageSurface = surface; }
    double getBackgroundOpacity() const { return backgroundOpacity; }
    void setBackgroundOpacity(double opacity) { backgroundOpacity = opacity; }
    
    bool isUsingBackgroundZip() const { return useBackgroundZip; }
    void setUseBackgroundZip(bool use) { useBackgroundZip = use; }

    void startBackgroundTransition();
    void updateBackgroundTransition();
    void cancelBackgroundTransition();
    bool isInBackgroundTransition() const { return isTransitioning; }
    double getTransitionOpacity() const { return transitionOpacity; }
    void* getOldBackground() const { return oldBackground; }
    int getTransitionDirection() const { return transitionDirection; }
    int getGhostPieceY() const;

    int getConsecutiveClears() const { return consecutiveClears; }
    int getMaxConsecutiveClears() const { return maxConsecutiveClears; }
    bool isSequenceActive() const { return sequenceActive; }
    
    bool isShowingPropagandaMessage() const { return showPropagandaMessage; }
    const std::string& getCurrentPropagandaMessage() const { return currentPropagandaMessage; }

    bool isLineClearActive() const { return lineClearActive; }
    bool isLineBeingCleared(int y) const;
    double getLineClearProgress() const { return lineClearProgress; }
    void getCurrentPieceInterpolatedPosition(double &x, double &y) const;
    void startLineClearAnimation(const std::vector<int> &clearedLines);
    void updateLineClearAnimation();
    void startSmoothMovement(int newX, int newY);
    void updateSmoothMovement();
    void setApp(TetrimoneApp* appPtr) { app = appPtr; }
    
    void startFireworksAnimation(int linesCleared);
    void updateFireworksAnimation();
    void createFireworkBurst(double centerX, double centerY, const std::array<double, 3>& baseColor, int particleCount);
    bool isFireworksActive() const { return fireworksActive; }
    const std::vector<FireworkParticle>& getFireworkParticles() const { return fireworkParticles; }

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
    
    // Game setup accessors
    int getJunkLinesPercentage() const { return junkLinesPercentage; }
    void setJunkLinesPercentage(int p) { junkLinesPercentage = p; }
    int getJunkLinesPerLevel() const { return junkLinesPerLevel; }
    void setJunkLinesPerLevel(int l) { junkLinesPerLevel = l; }
    bool isSoundEnabled() const { return sound_enabled_; }
    void setSoundEnabled(bool enabled) { sound_enabled_ = enabled; }
    
    // Audio file handling
    bool extractFileFromZip(const std::string& zipFilePath, const std::string& fileName, std::vector<unsigned char>& data);
    std::string getCacheFilePath(const std::string& soundFileName);
    bool loadCachedWav(const std::string& cacheFilePath, std::vector<unsigned char>& wavData);
    bool saveCachedWav(const std::string& cacheFilePath, const std::vector<unsigned char>& wavData);
    bool loadSoundFromZip(GameSoundEvent event, const std::string& soundFileName);
    bool setSoundsZipPath(const std::string& path);

    int junkLinesPercentage;
    int junkLinesPerLevel;

    bool isUsingBackgroundImage_;
    std::string backgroundZipPath;
    std::vector<std::string> backgroundImages;
    bool enabledTracks[5];
    int initialLevel = 1;        // Default starting level
    
    bool loadBackgroundImage(const std::string& imagePath);
    bool loadBackgroundImagesFromZip(const std::string& zipPath);

};

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
void saveJoystickMapping(TetrimoneApp* app);
void loadJoystickMapping(TetrimoneApp* app);
bool saveGameSettings(TetrimoneApp* app);
bool loadGameSettings(TetrimoneApp* app);
void resetGameSettings(TetrimoneApp* app);
std::array<double, 3> getHeatModifiedColor(const std::array<double, 3>& baseColor, float heatLevel);
void showIdeologicalFailureDialog(TetrimoneApp* app);
void showPatrioticPerformanceDialog(TetrimoneApp* app);
void rebuildGameUI(TetrimoneApp* app);

void ui_set_sound_enabled(TetrimoneApp *app, bool enabled);
void ui_set_active_theme(TetrimoneApp *app, int index);
void ui_set_background_enabled(TetrimoneApp *app, bool enabled);
void ui_set_window_title(TetrimoneApp *app, const char *title);
void ui_window_fullscreen(TetrimoneApp *app);
void ui_set_difficulty_label(TetrimoneApp *app, const char *markup);
void ui_set_pause_menu_label(TetrimoneApp *app, const char *text);
void ui_set_isusingbackgroundimage_enabled(TetrimoneApp *app);
void ui_set_sound_enabled(TetrimoneApp *app);
void set_difficulty_menu(TetrimoneApp *app, int difficulty);
void ui_update_track_menu(TetrimoneApp *app);
void ui_set_mediumMenuItem_enabled(TetrimoneApp *app, bool enabled);
void app_set_track_items_active(TetrimoneApp* app, int count, bool active);

#include "commandline.h"
int ui_run_application(int argc, char *argv[], TetrimoneApp *app, const CommandLineArgs *args);

#endif // TETRIMONE_QT5_H
