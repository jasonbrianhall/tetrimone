#ifndef TETRIS_H
#define TETRIS_H

#include <gtk/gtk.h>
#include <vector>
#include <array>
#include <random>
#include <chrono>
#include <memory>
#include "audiomanager.h"

enum class GameSoundEvent {
  BackgroundMusic
};

// Constants
const int GRID_WIDTH = 10;
const int GRID_HEIGHT = 20;
const int BLOCK_SIZE = 30;
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

// Forward declarations
class Tetromino;
class TetrisBoard;
struct TetrisApp;

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
    
public:
    TetrisBoard();
    ~TetrisBoard() = default;
    
    bool movePiece(int dx, int dy);
    bool rotatePiece();
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

// TetrisApp structure - holds application state
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

#endif // TETRIS_H
