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
#include "themes.h"
#include "tetrimoneblock.h"

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
  Clear,
  Drop,
  LateralMove,
  LevelUp,
  Rotate,
  Select,
  Start,
  Tetrimone,
  Excellent
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
const int GRID_WIDTH = 10;
const int GRID_HEIGHT = 22;

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

    int difficulty; // 1 = Easy, 2 = Medium, 3 = Hard

    SDL_Joystick* joystick;
    bool joystickEnabled;
    guint joystickTimerId;
    JoystickMapping joystickMapping;
};

// Class for the game board
class TetrimoneBoard {
private:
    std::vector<std::vector<int>> grid;  // Grid of placed blocks (0 = empty, 1-7 = tetrimoneblock type + 1)
    std::unique_ptr<TetrimoneBlock> currentPiece;
    std::unique_ptr<TetrimoneBlock> nextPiece;
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
    
public:
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

    TetrimoneBoard();
    ~TetrimoneBoard();
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
    const TetrimoneBlock& getNextPiece() const { return *nextPiece; }
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

};

// Function declarations
gboolean onDrawGameArea(GtkWidget* widget, cairo_t* cr, gpointer data);
gboolean onDrawNextPiece(GtkWidget* widget, cairo_t* cr, gpointer data);
gboolean onKeyPress(GtkWidget* widget, GdkEventKey* event, gpointer data);
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
std::string getDifficultyText(int difficulty);
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
#endif // TETRIMONE_H


