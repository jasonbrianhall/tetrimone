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
class Tetromino;
class TetrimoneBoard;
struct TetrimoneApp;

// Define the callback data structure after TetrimoneApp is forward declared
struct BlockSizeCallbackData {
    struct TetrimoneApp* app;
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
const std::vector<std::vector<std::array<double, 3>>> TETROMINO_COLOR_THEMES = {
    // Theme 1: Classic Tetrimone (Levels 1-3)
    {
        {0.0, 0.7, 0.9},  // I - Cyan
        {0.9, 0.9, 0.0},  // O - Yellow
        {0.8, 0.0, 0.8},  // T - Purple
        {0.0, 0.8, 0.0},  // S - Green
        {0.9, 0.0, 0.0},  // Z - Red
        {0.0, 0.0, 0.8},  // J - Blue
        {0.9, 0.5, 0.0}   // L - Orange
    },
    
    // Theme 2: Neon (Levels 4-6)
    {
        {0.0, 1.0, 1.0},  // I - Neon Cyan
        {1.0, 1.0, 0.0},  // O - Neon Yellow
        {1.0, 0.0, 1.0},  // T - Neon Magenta
        {0.0, 1.0, 0.0},  // S - Neon Green
        {1.0, 0.0, 0.0},  // Z - Neon Red
        {0.3, 0.3, 1.0},  // J - Neon Blue
        {1.0, 0.5, 0.0}   // L - Neon Orange
    },
    
    // Theme 3: Pastel (Levels 7-9)
    {
        {0.5, 0.8, 1.0},  // I - Pastel Blue
        {1.0, 1.0, 0.6},  // O - Pastel Yellow
        {0.9, 0.6, 0.9},  // T - Pastel Purple
        {0.6, 0.9, 0.6},  // S - Pastel Green
        {1.0, 0.6, 0.6},  // Z - Pastel Red
        {0.6, 0.6, 1.0},  // J - Pastel Blue
        {1.0, 0.8, 0.6}   // L - Pastel Orange
    },
    
    // Theme 4: Earth Tones (Levels 10-12)
    {
        {0.5, 0.8, 0.8},  // I - Soft Teal
        {0.9, 0.8, 0.4},  // O - Sand
        {0.7, 0.5, 0.3},  // T - Brown
        {0.5, 0.7, 0.3},  // S - Olive Green
        {0.8, 0.5, 0.3},  // Z - Terra Cotta
        {0.3, 0.5, 0.7},  // J - Slate Blue
        {0.6, 0.4, 0.2}   // L - Sienna
    },
    
    // Theme 5: Monochrome Blue (Levels 13-15)
    {
        {0.0, 0.2, 0.4},  // I - Navy
        {0.3, 0.5, 0.7},  // O - Steel Blue
        {0.1, 0.3, 0.5},  // T - Denim
        {0.2, 0.4, 0.6},  // S - Blue Gray
        {0.0, 0.1, 0.3},  // Z - Dark Blue
        {0.4, 0.6, 0.8},  // J - Sky Blue
        {0.5, 0.7, 0.9}   // L - Light Blue
    },
    
    // Theme 6: Monochrome Green (Levels 16-18)
    {
        {0.0, 0.4, 0.2},  // I - Forest Green
        {0.3, 0.7, 0.5},  // O - Mint
        {0.1, 0.5, 0.3},  // T - Emerald
        {0.2, 0.6, 0.4},  // S - Jade
        {0.0, 0.3, 0.1},  // Z - Dark Green
        {0.4, 0.8, 0.6},  // J - Sea Green
        {0.5, 0.9, 0.7}   // L - Light Green
    },
    
    // Theme 7: Sunset (Levels 19-21)
    {
        {0.9, 0.6, 0.1},  // I - Golden
        {0.9, 0.3, 0.1},  // O - Sunset Orange
        {0.8, 0.1, 0.1},  // T - Crimson
        {0.7, 0.2, 0.3},  // S - Ruby
        {0.6, 0.0, 0.1},  // Z - Dark Red
        {0.9, 0.5, 0.3},  // J - Salmon
        {1.0, 0.7, 0.4}   // L - Peach
    },
    
    // Theme 8: Ocean (Levels 22-24)
    {
        {0.0, 0.2, 0.6},  // I - Deep Blue
        {0.1, 0.5, 0.8},  // O - Ocean Blue
        {0.0, 0.3, 0.5},  // T - Navy
        {0.2, 0.6, 0.7},  // S - Turquoise
        {0.1, 0.2, 0.3},  // Z - Dark Blue-Gray
        {0.4, 0.7, 0.8},  // J - Sky Blue
        {0.6, 0.8, 0.9}   // L - Light Blue
    },
    
    // Theme 9: Grayscale (Levels 25-27)
    {
        {0.1, 0.1, 0.1},  // I - Near Black
        {0.9, 0.9, 0.9},  // O - Near White
        {0.3, 0.3, 0.3},  // T - Dark Gray
        {0.7, 0.7, 0.7},  // S - Light Gray
        {0.2, 0.2, 0.2},  // Z - Darker Gray
        {0.5, 0.5, 0.5},  // J - Medium Gray
        {0.8, 0.8, 0.8}   // L - Lighter Gray
    },
    
    // Theme 10: Candy (Levels 28-30)
    {
        {0.9, 0.5, 0.8},  // I - Pink
        {0.9, 0.8, 0.4},  // O - Light Yellow
        {0.7, 0.4, 0.9},  // T - Lavender
        {0.5, 0.9, 0.6},  // S - Mint Green
        {1.0, 0.5, 0.5},  // Z - Salmon
        {0.4, 0.7, 1.0},  // J - Baby Blue
        {1.0, 0.7, 0.3}   // L - Peach
    },
    
    // Theme 11: Neon Dark (Levels 31-33)
    {
        {0.0, 0.9, 0.9},  // I - Neon Cyan
        {0.9, 0.9, 0.0},  // O - Neon Yellow
        {0.9, 0.0, 0.9},  // T - Neon Magenta
        {0.0, 0.9, 0.0},  // S - Neon Green
        {0.9, 0.0, 0.0},  // Z - Neon Red
        {0.0, 0.0, 0.9},  // J - Neon Blue
        {0.9, 0.5, 0.0}   // L - Neon Orange
    },
    
    // Theme 12: Jewel Tones (Levels 34-36)
    {
        {0.0, 0.6, 0.8},  // I - Sapphire
        {0.9, 0.8, 0.0},  // O - Amber
        {0.6, 0.0, 0.6},  // T - Amethyst
        {0.0, 0.6, 0.3},  // S - Emerald
        {0.8, 0.0, 0.2},  // Z - Ruby
        {0.2, 0.2, 0.7},  // J - Lapis
        {0.8, 0.4, 0.0}   // L - Topaz
    },
    
    // Theme 13: Retro Gaming (Levels 37-39)
    {
        {0.0, 0.8, 0.0},  // I - Green Phosphor
        {0.8, 0.8, 0.0},  // O - Amber Phosphor
        {0.0, 0.7, 0.0},  // T - Dark Green Phosphor
        {0.0, 0.9, 0.0},  // S - Light Green Phosphor
        {0.0, 0.6, 0.0},  // Z - Medium Green Phosphor
        {0.0, 0.7, 0.3},  // J - Teal Phosphor
        {0.3, 0.8, 0.0}   // L - Yellow-Green Phosphor
    },
    
    // Theme 14: Autumn (Levels 40-42)
    {
        {0.7, 0.3, 0.1},  // I - Rust
        {0.9, 0.7, 0.2},  // O - Gold
        {0.6, 0.2, 0.0},  // T - Mahogany
        {0.5, 0.6, 0.1},  // S - Olive
        {0.8, 0.3, 0.0},  // Z - Copper
        {0.4, 0.2, 0.1},  // J - Brown
        {0.9, 0.5, 0.0}   // L - Orange
    },
    
    // Theme 15: Winter (Levels 43-45)
    {
        {0.7, 0.9, 1.0},  // I - Ice Blue
        {1.0, 1.0, 1.0},  // O - Snow White
        {0.8, 0.9, 0.9},  // T - Frost
        {0.5, 0.7, 0.8},  // S - Winter Sky
        {0.7, 0.8, 1.0},  // Z - Pale Blue
        {0.3, 0.5, 0.7},  // J - Slate Blue
        {0.9, 0.9, 0.9}   // L - Silver
    },
    
    // Theme 16: Spring (Levels 46-48)
    {
        {0.7, 0.9, 0.7},  // I - Mint Green
        {1.0, 0.9, 0.7},  // O - Peach
        {0.9, 0.7, 0.9},  // T - Lilac
        {0.7, 0.9, 0.5},  // S - Lime Green
        {1.0, 0.7, 0.7},  // Z - Light Pink
        {0.5, 0.7, 0.9},  // J - Sky Blue
        {1.0, 1.0, 0.7}   // L - Cream
    },
    
    // Theme 17: Summer (Levels 49-51)
    {
        {0.0, 0.8, 0.8},  // I - Turquoise
        {1.0, 0.8, 0.0},  // O - Sunshine Yellow
        {0.9, 0.5, 0.7},  // T - Coral
        {0.3, 0.9, 0.3},  // S - Bright Green
        {1.0, 0.3, 0.3},  // Z - Watermelon
        {0.1, 0.6, 0.9},  // J - Azure
        {1.0, 0.6, 0.0}   // L - Orange
    },
    
    // Theme 18: Monochrome Purple (Levels 52-54)
    {
        {0.2, 0.0, 0.4},  // I - Deep Purple
        {0.8, 0.6, 1.0},  // O - Light Lavender
        {0.4, 0.0, 0.6},  // T - Royal Purple
        {0.6, 0.4, 0.8},  // S - Medium Lavender
        {0.3, 0.0, 0.5},  // Z - Dark Purple
        {0.5, 0.2, 0.7},  // J - Violet
        {0.7, 0.5, 0.9}   // L - Light Purple
    },
    
    // Theme 19: Desert (Levels 55-57)
    {
        {0.9, 0.8, 0.6},  // I - Sand
        {0.7, 0.6, 0.4},  // O - Khaki
        {0.8, 0.5, 0.3},  // T - Terra Cotta
        {0.6, 0.5, 0.3},  // S - Olive Brown
        {0.9, 0.6, 0.4},  // Z - Peach
        {0.5, 0.4, 0.3},  // J - Dark Taupe
        {0.8, 0.7, 0.5}   // L - Beige
    },
    
    // Theme 20: Rainbow (Levels 58+)
    {
        {1.0, 0.0, 0.0},  // I - Red
        {1.0, 0.5, 0.0},  // O - Orange
        {1.0, 1.0, 0.0},  // T - Yellow
        {0.0, 1.0, 0.0},  // S - Green
        {0.0, 0.0, 1.0},  // Z - Blue
        {0.3, 0.0, 0.5},  // J - Indigo
        {0.5, 0.0, 1.0}   // L - Violet
    }
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
    std::atomic<bool> musicStopFlag{false};

    
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
    bool checkCollision(const Tetromino& piece) const;
    void lockPiece();
    int clearLines();
    void generateNewPiece();
    void updateGame();
    void hardDrop();
    void togglePause() { paused = !paused; }
    void setPaused(bool pause) { paused = pause; }
    
    bool isGameOver() const;
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

#endif // TETRIS_H


