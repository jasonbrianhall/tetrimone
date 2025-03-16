#ifndef TETRIS_H
#define TETRIS_H

#include <gtk/gtk.h>
#include <vector>
#include <array>
#include <random>
#include <chrono>
#include <memory>
#include "audiomanager.h"
#include <SDL2/SDL.h>

enum class GameSoundEvent {
  BackgroundMusic
};

// Forward declarations (need to be at the top)
class Tetromino;
class TetrisBoard;
struct TetrisApp;

// Define the callback data structure after TetrisApp is forward declared
struct BlockSizeCallbackData {
    struct TetrisApp* app;
    GtkWidget* label;
};

// Constants
const int GRID_WIDTH = 10;
const int GRID_HEIGHT = 20;

extern int BLOCK_SIZE;  // This will be calculated at runtime
const int MIN_BLOCK_SIZE = 20;  // Minimum block size to ensure visibility
const int MAX_BLOCK_SIZE = 80;  // Maximum block size to prevent extreme scaling

const int INITIAL_SPEED = 500; // milliseconds

// Colors for each tetromino type (RGB)
const std::vector<std::array<double, 3>> TETROMINO_COLORS = {
    {0.0, 0.7, 0.9},  // I - Cyan
    {0.9, 0.9, 0.0},  // O - Yellow
    {0.8, 0.0, 0.8},  // T - Purple
    {0.0, 0.8, 0.0},  // S - Green
    {0.9, 0.0, 0.0},  // Z - Red
    {0.0, 0.0, 0.8},  // J - Blue
    {0.9, 0.5, 0.0}   // L - Orange
};

const std::vector<std::vector<std::vector<std::vector<int>>>> TETROMINO_SHAPES = {
    // I-piece
    {
        {
            {0, 0, 0, 0},
            {1, 1, 1, 1},
            {0, 0, 0, 0},
            {0, 0, 0, 0}
        },
        {
            {0, 0, 1, 0},
            {0, 0, 1, 0},
            {0, 0, 1, 0},
            {0, 0, 1, 0}
        },
        {
            {0, 0, 0, 0},
            {0, 0, 0, 0},
            {1, 1, 1, 1},
            {0, 0, 0, 0}
        },
        {
            {0, 1, 0, 0},
            {0, 1, 0, 0},
            {0, 1, 0, 0},
            {0, 1, 0, 0}
        }
    },
    // O-piece
    {
        {
            {0, 0, 0, 0},
            {0, 1, 1, 0},
            {0, 1, 1, 0},
            {0, 0, 0, 0}
        },
        {
            {0, 0, 0, 0},
            {0, 1, 1, 0},
            {0, 1, 1, 0},
            {0, 0, 0, 0}
        },
        {
            {0, 0, 0, 0},
            {0, 1, 1, 0},
            {0, 1, 1, 0},
            {0, 0, 0, 0}
        },
        {
            {0, 0, 0, 0},
            {0, 1, 1, 0},
            {0, 1, 1, 0},
            {0, 0, 0, 0}
        }
    },
    // T-piece
    {
        {
            {0, 0, 0, 0},
            {0, 1, 0, 0},
            {1, 1, 1, 0},
            {0, 0, 0, 0}
        },
        {
            {0, 0, 0, 0},
            {0, 1, 0, 0},
            {0, 1, 1, 0},
            {0, 1, 0, 0}
        },
        {
            {0, 0, 0, 0},
            {0, 0, 0, 0},
            {1, 1, 1, 0},
            {0, 1, 0, 0}
        },
        {
            {0, 0, 0, 0},
            {0, 1, 0, 0},
            {1, 1, 0, 0},
            {0, 1, 0, 0}
        }
    },
    // S-piece
    {
        {
            {0, 0, 0, 0},
            {0, 1, 1, 0},
            {1, 1, 0, 0},
            {0, 0, 0, 0}
        },
        {
            {0, 0, 0, 0},
            {0, 1, 0, 0},
            {0, 1, 1, 0},
            {0, 0, 1, 0}
        },
        {
            {0, 0, 0, 0},
            {0, 0, 0, 0},
            {0, 1, 1, 0},
            {1, 1, 0, 0}
        },
        {
            {0, 0, 0, 0},
            {1, 0, 0, 0},
            {1, 1, 0, 0},
            {0, 1, 0, 0}
        }
    },
    // Z-piece
    {
        {
            {0, 0, 0, 0},
            {1, 1, 0, 0},
            {0, 1, 1, 0},
            {0, 0, 0, 0}
        },
        {
            {0, 0, 0, 0},
            {0, 0, 1, 0},
            {0, 1, 1, 0},
            {0, 1, 0, 0}
        },
        {
            {0, 0, 0, 0},
            {0, 0, 0, 0},
            {1, 1, 0, 0},
            {0, 1, 1, 0}
        },
        {
            {0, 0, 0, 0},
            {0, 1, 0, 0},
            {1, 1, 0, 0},
            {1, 0, 0, 0}
        }
    },
    // J-piece
    {
        {
            {0, 0, 0, 0},
            {1, 0, 0, 0},
            {1, 1, 1, 0},
            {0, 0, 0, 0}
        },
        {
            {0, 0, 0, 0},
            {0, 1, 1, 0},
            {0, 1, 0, 0},
            {0, 1, 0, 0}
        },
        {
            {0, 0, 0, 0},
            {0, 0, 0, 0},
            {1, 1, 1, 0},
            {0, 0, 1, 0}
        },
        {
            {0, 0, 0, 0},
            {0, 1, 0, 0},
            {0, 1, 0, 0},
            {1, 1, 0, 0}
        }
    },
    // L-piece
    {
        {
            {0, 0, 0, 0},
            {0, 0, 1, 0},
            {1, 1, 1, 0},
            {0, 0, 0, 0}
        },
        {
            {0, 0, 0, 0},
            {0, 1, 0, 0},
            {0, 1, 0, 0},
            {0, 1, 1, 0}
        },
        {
            {0, 0, 0, 0},
            {0, 0, 0, 0},
            {1, 1, 1, 0},
            {1, 0, 0, 0}
        },
        {
            {0, 0, 0, 0},
            {1, 1, 0, 0},
            {0, 1, 0, 0},
            {0, 1, 0, 0}
        }
    }
};

// Class for a single tetromino piece
class Tetromino {
private:
    int type;                   // 0-6 for I, O, T, S, Z, J, L
    int rotation;               // 0-3 for four possible rotations
    int x, y;                   // Position on the grid
    
public:
    Tetromino(int type);
    void rotate(bool clockwise = true);
    void move(int dx, int dy);
    std::vector<std::vector<int>> getShape() const;
    std::array<double, 3> getColor() const;
    int getType() const { return type; }
    int getX() const { return x; }
    int getY() const { return y; }
    void setPosition(int newX, int newY);
};

// TetrisApp structure needs to be defined before it's used in TetrisBoard
struct TetrisApp {
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
    TetrisBoard* board;
    guint timerId;
    int dropSpeed;
    
    // Menu related widgets
    GtkWidget* menuBar;
    GtkWidget* startMenuItem;
    GtkWidget* pauseMenuItem;
    GtkWidget* restartMenuItem;
    GtkWidget* soundToggleMenuItem;
    GtkWidget* easyMenuItem;
    GtkWidget* mediumMenuItem;
    GtkWidget* hardMenuItem;
    int difficulty; // 1 = Easy, 2 = Medium, 3 = Hard

    SDL_Joystick* joystick;
    bool joystickEnabled;
    guint joystickTimerId;
};

// Class for the game board
class TetrisBoard {
private:
    std::vector<std::vector<int>> grid;  // Grid of placed blocks (0 = empty, 1-7 = tetromino type + 1)
    std::unique_ptr<Tetromino> currentPiece;
    std::unique_ptr<Tetromino> nextPiece;
    int score;
    int level;
    int linesCleared;
    bool gameOver;
    bool paused;
    std::mt19937 rng;           // Random number generator
    bool splashScreenActive;

    
public:
    TetrisBoard();
    ~TetrisBoard() = default;
    bool isSplashScreenActive() const { return splashScreenActive; }
    void dismissSplashScreen() { splashScreenActive = false; }
    bool movePiece(int dx, int dy);
    bool rotatePiece(bool direction);
    bool checkCollision(const Tetromino& piece) const;
    void lockPiece();
    int clearLines();
    void generateNewPiece();
    void updateGame();
    void hardDrop();
    void togglePause() { paused = !paused; }
    void setPaused(bool pause) { paused = pause; }
    
    bool isGameOver() const { return gameOver; }
    bool isPaused() const { return paused; }
    int getScore() const { return score; }
    int getLevel() const { return level; }
    int getLinesCleared() const { return linesCleared; }
    const Tetromino& getCurrentPiece() const { return *currentPiece; }
    const Tetromino& getNextPiece() const { return *nextPiece; }
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
};

// Function declarations
gboolean onDrawGameArea(GtkWidget* widget, cairo_t* cr, gpointer data);
gboolean onDrawNextPiece(GtkWidget* widget, cairo_t* cr, gpointer data);
gboolean onKeyPress(GtkWidget* widget, GdkEventKey* event, gpointer data);
gboolean onTimerTick(gpointer data);
void updateLabels(TetrisApp* app);
void startGame(TetrisApp* app);
void pauseGame(TetrisApp* app);
void resetUI(TetrisApp* app);
void cleanupApp(gpointer data);
void onAppActivate(GtkApplication* app, gpointer userData);
void onMenuActivated(GtkWidget* widget, gpointer userData);
void onMenuDeactivated(GtkWidget* widget, gpointer userData);

// Menu related functions
void createMenu(TetrisApp* app);
void onStartGame(GtkMenuItem* menuItem, gpointer userData);
void onPauseGame(GtkMenuItem* menuItem, gpointer userData);
void onRestartGame(GtkMenuItem* menuItem, gpointer userData);
void onQuitGame(GtkMenuItem* menuItem, gpointer userData);
void onSoundToggled(GtkCheckMenuItem* menuItem, gpointer userData);
void onDifficultyChanged(GtkRadioMenuItem* menuItem, gpointer userData);
void onAboutDialog(GtkMenuItem* menuItem, gpointer userData);
void onInstructionsDialog(GtkMenuItem* menuItem, gpointer userData);
std::string getDifficultyText(int difficulty);
void adjustDropSpeed(TetrisApp* app);
void calculateBlockSize(TetrisApp* app);
gboolean pollJoystick(gpointer data);
void initSDL(TetrisApp* app);
void shutdownSDL(TetrisApp* app);
void onBlockSizeDialog(GtkMenuItem* menuItem, gpointer userData);
void onBlockSizeValueChanged(GtkRange* range, gpointer data);
void onResizeWindowButtonClicked(GtkWidget* button, gpointer data);


#endif // TETRIS_H
