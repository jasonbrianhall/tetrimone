#ifndef TETRIMONE_H
#define TETRIMONE_H

#include <gtk/gtk.h>
#include <vector>
#include <array>
#include <random>
#include <chrono>
#include <memory>
#include "audiomanager.h"
#include <SDL2/SDL.h>
#include <gdk-pixbuf/gdk-pixbuf.h>
#include <vector>
#include "themes.h"
#include "tetrimoneblock.h"
#include "highscores.h"
#include "propaganda_messages.h"

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

// Forward declarations (need to be at the top)
class TetrimoneBlock;
class TetrimoneBoard;
struct TetrimoneApp;

// Define the callback data structure after TetrimoneApp is forward declared
struct BlockSizeCallbackData {
    struct TetrimoneApp* app;
    GtkWidget* label;
};

// Constants
extern int GRID_WIDTH;
extern int GRID_HEIGHT;

const int MIN_GRID_WIDTH = 8;   // Minimum width to ensure the game is playable
const int MAX_GRID_WIDTH = 16;  // Maximum width to prevent extreme scaling
const int MIN_GRID_HEIGHT = 16; // Minimum height to ensure the game is playable
const int MAX_GRID_HEIGHT = 30; // Maximum height to prevent extreme scaling

extern int BLOCK_SIZE;  // This will be calculated at runtime
const int MIN_BLOCK_SIZE = 20;  // Minimum block size to ensure visibility
const int MAX_BLOCK_SIZE = 80;  // Maximum block size to prevent extreme scaling

const int INITIAL_SPEED = 500; // milliseconds

// Class for a single tetrimoneblock piece
class TetrimoneBlock {
private:
    int type;                   // 0-6 for I, O, T, S, Z, J, L
    int rotation;               // 0-3 for four possible rotations
    int x, y;                   // Position on the grid
    
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
    int rotate_cw_button;     // Button for rotate clockwise
    int rotate_ccw_button;    // Button for rotate counter-clockwise
    int hard_drop_button;     // Button for hard drop
    int pause_button;         // Button for pause/unpause
    int x_axis;               // Axis for left/right movement
    int y_axis;               // Axis for up/down movement
    bool invert_x;            // Whether to invert X axis
    bool invert_y;            // Whether to invert Y axis
} JoystickMapping;

// TetrimoneApp structure needs to be defined before it's used in TetrimoneBoard
struct TetrimoneApp {
    GtkApplication* app;
    GtkWidget* window;
    GtkWidget* mainBox;
    GtkWidget* gameArea;
    GtkWidget* nextPieceArea;
    GtkWidget* scoreLabel;
    GtkWidget* levelLabel;
    GtkWidget* linesLabel;
    GtkWidget* difficultyLabel;
    bool backgroundMusicPlaying = false;
    TetrimoneBoard* board;
    guint timerId;
    int dropSpeed;
    GtkWidget* backgroundToggleMenuItem;
    // Menu related widgets
    GtkWidget* menuBar;
    GtkWidget* startMenuItem;
    GtkWidget* pauseMenuItem;
    GtkWidget* restartMenuItem;
    GtkWidget* soundToggleMenuItem;
    GtkWidget* zenMenuItem;
    GtkWidget* easyMenuItem;
    GtkWidget* mediumMenuItem;
    GtkWidget* hardMenuItem;
    GtkWidget* extremeMenuItem;
    GtkWidget* insaneMenuItem;
    GtkWidget* trackMenuItems[5];
    GtkWidget* themeMenuItems[31];
    GtkWidget* sequenceLabel;
    GtkWidget* controlsLabel;
    int difficulty; // 1 = Easy, 2 = Medium, 3 = Hard, 0 = Zen, 4 = Extreme
    GtkWidget* controlsHeaderLabel;
    SDL_Joystick* joystick;
    bool joystickEnabled;
    guint joystickTimerId;
    JoystickMapping joystickMapping;
    bool pausedByFocusLoss = false;
};

// Class for the game board
class TetrimoneBoard {
private:
    TetrimoneApp* app;
    float heatLevel;        // 0.0 = frozen, 1.0 = max fire
    guint heatDecayTimer;   // Timer for cooling down
    std::vector<std::vector<int>> grid;
    std::unique_ptr<TetrimoneBlock> currentPiece;
    std::vector<std::unique_ptr<TetrimoneBlock>> nextPieces; // Vector of next pieces (3)
    int score;
    int level;
    int linesCleared;
    bool gameOver;
    bool paused;
    std::mt19937 rng;           // Random number generator
    bool splashScreenActive;
    std::atomic<bool> musicStopFlag{false};
    int minBlockSize = 1;
    int gridWidth = GRID_WIDTH;   // Default width
    int gridHeight = GRID_HEIGHT; // Default height
    bool ghostPieceEnabled;
    int consecutiveClears;
    int maxConsecutiveClears;
    int lastClearCount;
    bool sequenceActive;
    Highscores highScores;
    int currentAnimationType;

    // Theme transition variables
    bool isThemeTransitioning;
    int oldThemeIndex;
    int newThemeIndex;
    double themeTransitionProgress;
    guint themeTransitionTimer;
    static const int THEME_TRANSITION_DURATION = 3000; // milliseconds

    bool lineClearActive;
    std::vector<int> linesBeingCleared;
    double lineClearProgress;
    guint lineClearAnimationTimer;
    static const int LINE_CLEAR_ANIMATION_DURATION = 600; // milliseconds

    // Smooth movement variables
    double currentPieceInterpolatedX;
    double currentPieceInterpolatedY;
    int lastPieceX;
    int lastPieceY;
    guint smoothMovementTimer;
    double movementProgress;
    static const int MOVEMENT_ANIMATION_DURATION = 100; // milliseconds
    std::chrono::high_resolution_clock::time_point lineClearStartTime;
    std::chrono::high_resolution_clock::time_point movementStartTime;
    std::chrono::high_resolution_clock::time_point themeStartTime;

    std::string getCacheFilePath(const std::string& soundFileName);
    bool loadCachedWav(const std::string& cacheFilePath, std::vector<uint8_t>& wavData);
    bool saveCachedWav(const std::string& cacheFilePath, const std::vector<uint8_t>& wavData);
    
    
    struct FireworkParticle {
    double x, y;           // Current position
    double vx, vy;         // Velocity
    double life;           // Life remaining (1.0 to 0.0)
    double maxLife;        // Maximum life span
    std::array<double, 3> color; // RGB color
    double size;           // Particle size
    double gravity;        // Gravity effect
    double fade;           // Fade rate
};

bool fireworksActive;
std::vector<FireworkParticle> fireworkParticles;
guint fireworksTimer;
std::chrono::high_resolution_clock::time_point fireworksStartTime;
static const int FIREWORKS_DURATION = 2000; // 2 seconds
int fireworksType; // 0 = normal, 1 = tetrimone (4 lines)

struct BlockTrail {
    double x, y;                    // Position of the piece when this trail was created
    int rotation;                   // Rotation of the piece
    int pieceType;                  // Type of piece (I, O, T, etc.)
    double life;                    // Life remaining (1.0 to 0.0)
    double maxLife;                 // Maximum life span
    double alpha;                   // Transparency (starts high, fades to 0)
    std::array<double, 3> color;    // Color of the piece
    std::vector<std::vector<int>> shape; // The actual shape of the piece
};

bool trailsEnabled;
std::vector<BlockTrail> blockTrails;
guint trailUpdateTimer;
std::chrono::high_resolution_clock::time_point lastTrailTime;
int maxTrailSegments;               // How many trail segments to keep
double trailOpacity;                // Configurable opacity (0.1 to 1.0)
double trailDuration;               // Configurable duration (0.05 to 2.0 seconds)
static const int TRAIL_UPDATE_INTERVAL = 16; // ~60 FPS
static const double TRAIL_SPAWN_DELAY = 120.0; // milliseconds between trail segments (slower)
    
public:
    // Theme transition methods
    void setLevel(int levelnumber);
    void setMinBlock(int size);
    void coolDown();
    float getHeatLevel();
    void setHeatLevel(float heatLevel);
    void updateHeat();
    void startThemeTransition(int targetTheme);
    void updateThemeTransition();
    void cancelThemeTransition();
    bool isInThemeTransition() const { return isThemeTransitioning; }
    double getThemeTransitionProgress() const { return themeTransitionProgress; }
    int getOldThemeIndex() const { return oldThemeIndex; }
    std::array<double, 3> getInterpolatedColor(int blockType, double progress) const;

    int getCurrentAnimationType() const { return currentAnimationType; }

    int junkLinesPercentage = 0; // Default 0% (no junk lines)
    int initialLevel = 1;        // Default starting level
    void generateJunkLines(int percentage);
    int junkLinesPerLevel = 0;
    void addJunkLinesFromBottom(int numLines);

    
    bool retroModeActive = false;  // Flag for retro mode
    bool patrioticModeActive = false;
    bool simpleBlocksActive = false;
    bool retroMusicActive = false;
    bool showGridLines = false; // Grid lines off by default
    bool isShowingGridLines() const { return showGridLines; }
    void setShowGridLines(bool show) { showGridLines = show; }
    std::string getDifficultyText(int difficulty) const;
    TetrimoneBoard();
    ~TetrimoneBoard();
    bool checkAndRecordHighScore(TetrimoneApp* app);
    const Highscores& getHighScores() const { return highScores; }

    bool highScoreAlreadyProcessed = false;
 
    bool enabledTracks[5];
    // Background image handling
    cairo_surface_t* backgroundImage;
    bool useBackgroundImage;
    std::string backgroundImagePath;
    double backgroundOpacity; // 0.0 to 1.0
    std::vector<cairo_surface_t*> backgroundImages;
    std::string backgroundZipPath;
    bool useBackgroundZip;
    int currentBackgroundIndex;
    bool isGhostPieceEnabled() const { return ghostPieceEnabled; }
    void setGhostPieceEnabled(bool enabled) { ghostPieceEnabled = enabled; }


    bool isSplashScreenActive() const { return splashScreenActive; }
    void dismissSplashScreen();
    bool movePiece(int dx, int dy);
    bool rotatePiece(bool direction);
    bool checkCollision(const TetrimoneBlock& piece) const;
    void lockPiece();
    int clearLines();
    void generateNewPiece();
    void updateGame();
    void hardDrop();
    void togglePause() { paused = !paused; }
    void setPaused(bool pause) { paused = pause; }
    void setMinBlockSize(int size) { 
        minBlockSize = std::max(1, std::min(size, 4)); 
    }
    int getMinBlockSize() const { return minBlockSize; }
    bool isGameOver() const;
    bool isPaused() const { return paused; }
    int getScore() const { return score; }
    int getLevel() const { return level; }
    int getLinesCleared() const { return linesCleared; }
    const TetrimoneBlock& getCurrentPiece() const { return *currentPiece; }
    const std::vector<std::unique_ptr<TetrimoneBlock>>& getNextPieces() const { return nextPieces; }
    const TetrimoneBlock& getNextPiece(int index = 0) const { return *nextPieces[index]; }
    int getGridValue(int x, int y) const;
    bool sound_enabled_ = true;
    bool musicPaused = false;
    std::string sounds_zip_path_ = "sound.zip";
    void restart();

    bool initializeAudio();
    void playSound(GameSoundEvent event);
    void playBackgroundMusic();
    void pauseBackgroundMusic();
    void resumeBackgroundMusic();
    void cleanupAudio();
    bool loadSoundFromZip(GameSoundEvent event, const std::string& soundFileName);
    bool setSoundsZipPath(const std::string&);
    bool extractFileFromZip(const std::string &zipFilePath,
                          const std::string &fileName,
                          std::vector<uint8_t> &fileData);

    bool loadBackgroundImage(const std::string& imagePath);
    void setUseBackgroundImage(bool use) { useBackgroundImage = use; }
    bool isUsingBackgroundImage() const { return useBackgroundImage; }
    void setBackgroundOpacity(double opacity) { 
        backgroundOpacity = std::max(0.0, std::min(1.0, opacity)); 
    }
    double getBackgroundOpacity() const { return backgroundOpacity; }
    const std::string& getBackgroundImagePath() const { return backgroundImagePath; }
    cairo_surface_t* getBackgroundImage() const { return backgroundImage; }
    bool loadBackgroundImagesFromZip(const std::string& zipPath);
    void selectRandomBackground();
    void cleanupBackgroundImages();
    bool isUsingBackgroundZip() const { return useBackgroundZip; }
    void setUseBackgroundZip(bool use) { useBackgroundZip = use; }

    bool isTransitioning;           // Whether a background transition is in progress
    double transitionOpacity;       // Current opacity during transition
    int transitionDirection;        // 1 for fade in, -1 for fade out
    cairo_surface_t* oldBackground; // Store the previous background during transition
    guint transitionTimerId;        // Timer ID for the transition effect

    void startBackgroundTransition();
    void updateBackgroundTransition();
    void cancelBackgroundTransition();
    bool isInBackgroundTransition() const { return isTransitioning; }
    double getTransitionOpacity() const { return transitionOpacity; }
    cairo_surface_t* getOldBackground() const { return oldBackground; }
    int getTransitionDirection() const { return transitionDirection; }
    int getGhostPieceY() const;

    int getConsecutiveClears() const { return consecutiveClears; }
    int getMaxConsecutiveClears() const { return maxConsecutiveClears; }
    bool isSequenceActive() const { return sequenceActive; }
    
    bool isShowingPropagandaMessage() const { return showPropagandaMessage; }
    const std::string& getCurrentPropagandaMessage() const { return currentPropagandaMessage; }
    bool showPropagandaMessage;
    guint propagandaTimerId;
    int propagandaMessageDuration; // in milliseconds
    std::string currentPropagandaMessage;
    double propagandaMessageScale;
    bool propagandaScalingUp;
    guint propagandaScaleTimerId;

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


};

// Function declarations
gboolean onDrawGameArea(GtkWidget* widget, cairo_t* cr, gpointer data);
gboolean onDrawNextPiece(GtkWidget* widget, cairo_t* cr, gpointer data);
gboolean onKeyPress(GtkWidget* widget, GdkEventKey* event, gpointer data);
gboolean onKeyDownTick(gpointer userData);
gboolean onKeyLeftTick(gpointer userData);
gboolean onKeyRightTick(gpointer userData);
gboolean onTimerTick(gpointer data);
void updateLabels(TetrimoneApp* app);
void startGame(TetrimoneApp* app);
void pauseGame(TetrimoneApp* app);
void resetUI(TetrimoneApp* app);
void cleanupApp(gpointer data);
void onAppActivate(GtkApplication* app, gpointer userData);
void onMenuActivated(GtkWidget* widget, gpointer userData);
void onMenuDeactivated(GtkWidget* widget, gpointer userData);

// Menu related functions
void createMenu(TetrimoneApp* app);
void onStartGame(GtkMenuItem* menuItem, gpointer userData);
void onPauseGame(GtkMenuItem* menuItem, gpointer userData);
void onRestartGame(GtkMenuItem* menuItem, gpointer userData);
void onQuitGame(GtkMenuItem* menuItem, gpointer userData);
void onSoundToggled(GtkCheckMenuItem* menuItem, gpointer userData);
void onDifficultyChanged(GtkRadioMenuItem* menuItem, gpointer userData);
void onAboutDialog(GtkMenuItem* menuItem, gpointer userData);
void onInstructionsDialog(GtkMenuItem* menuItem, gpointer userData);
void adjustDropSpeed(TetrimoneApp* app);
void calculateBlockSize(TetrimoneApp* app);
gboolean pollJoystick(gpointer data);
void initSDL(TetrimoneApp* app);
void shutdownSDL(TetrimoneApp* app);
void onBlockSizeDialog(GtkMenuItem* menuItem, gpointer userData);
void onBlockSizeValueChanged(GtkRange* range, gpointer data);
void onResizeWindowButtonClicked(GtkWidget* button, gpointer data);
gboolean onDeleteEvent(GtkWidget* widget, GdkEvent* event, gpointer userData);

void onJoystickConfig(GtkMenuItem* menuItem, gpointer userData);
void onJoystickRescan(GtkButton* button, gpointer userData);
void updateJoystickInfo(GtkLabel* infoLabel, TetrimoneApp* app);
void onJoystickMapApply(GtkButton* button, gpointer userData);
void onJoystickMapReset(GtkButton* button, gpointer userData);
void saveJoystickMapping(TetrimoneApp* app);
void loadJoystickMapping(TetrimoneApp* app);
void onBackgroundImageDialog(GtkMenuItem* menuItem, gpointer userData);
void onBackgroundToggled(GtkCheckMenuItem* menuItem, gpointer userData);
void onBackgroundOpacityDialog(GtkMenuItem* menuItem, gpointer userData);
void onOpacityValueChanged(GtkRange* range, gpointer userData);
void rebuildGameUI(TetrimoneApp* app);
void updateSizeValueLabel(GtkRange* range, gpointer data);
void onBackgroundZipDialog(GtkMenuItem* menuItem, gpointer userData);
void onVolumeDialog(GtkMenuItem* menuItem, gpointer userData);
void onVolumeValueChanged(GtkRange* range, gpointer userData);
void onMusicVolumeValueChanged(GtkRange* range, gpointer userData);
void onTrackToggled(GtkCheckMenuItem* menuItem, gpointer userData);
void onBlockSizeRulesChanged(GtkRadioMenuItem* menuItem, gpointer userData);
void onGameSizeDialog(GtkMenuItem* menuItem, gpointer userData);
void onGridLinesToggled(GtkCheckMenuItem* menuItem, gpointer userData);
void updateWidthValueLabel(GtkAdjustment* adj, gpointer data);
void updateHeightValueLabel(GtkAdjustment* adj, gpointer data);
cairo_surface_t* cairo_image_surface_create_from_jpeg(const char* filename);
cairo_surface_t* cairo_image_surface_create_from_memory(const void* data, size_t length);

void onGhostPieceToggled(GtkCheckMenuItem* menuItem, gpointer userData);
void onViewHighScores(GtkMenuItem* menuItem, gpointer userData);
void setWindowIcon(GtkWindow* window);

void onSimpleBlocksToggled(GtkCheckMenuItem* menuItem, gpointer userData);
void onRetroMusicToggled(GtkCheckMenuItem* menuItem, gpointer userData);
void onTestSound(GtkButton* button, gpointer userData);
gboolean onWindowFocusChanged(GtkWidget *widget, GdkEventFocus *event, gpointer userData);
void showIdeologicalFailureDialog(TetrimoneApp* app);
void showPatrioticPerformanceDialog(TetrimoneApp* app);
void onGameSetupDialog(GtkMenuItem* menuItem, gpointer userData);
bool saveGameSettings(TetrimoneApp* app);
bool loadGameSettings(TetrimoneApp* app);
void resetGameSettings(TetrimoneApp* app);
void onResetSettings(GtkMenuItem* menuItem, gpointer userData);
void onThemeChanged(GtkRadioMenuItem *menuItem, gpointer userData);
std::array<double, 3> getHeatModifiedColor(const std::array<double, 3>& baseColor, float heatLevel);
void drawFreezyEffect(cairo_t* cr, double x, double y, double size, float heatLevel, double time);
void drawFireyGlow(cairo_t* cr, double x, double y, double size, float heatLevel, double time);
void onBlockTrailsToggled(GtkCheckMenuItem* menuItem, gpointer userData);
void onBlockTrailsConfig(GtkMenuItem* menuItem, gpointer userData);
void onTrailOpacityChanged(GtkAdjustment* adj, gpointer data);
void onTrailDurationChanged(GtkAdjustment* adj, gpointer data);
#endif // TETRIMONE_H
