#ifndef TETRIMONE_SDL2_H
#define TETRIMONE_SDL2_H

#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
#include <vector>
#include <array>
#include <random>
#include <chrono>
#include <memory>
#include <string>
#include "audiomanager.h"
#include "themes.h"
#include "tetrimoneblock.h"
#include "highscores.h"
#include "propaganda_messages.h"
#include "freedom_messages.h"

// Constants
extern int GRID_WIDTH;
extern int GRID_HEIGHT;
extern int BLOCK_SIZE;

const int MIN_GRID_WIDTH = 8;
const int MAX_GRID_WIDTH = 16;
const int MIN_GRID_HEIGHT = 16;
const int MAX_GRID_HEIGHT = 30;
const int MIN_BLOCK_SIZE = 20;
const int MAX_BLOCK_SIZE = 80;
const int INITIAL_SPEED = 500;

// Rendering constants
const int WINDOW_PADDING = 20;
const int SIDEBAR_WIDTH = 200;
const int MIN_WINDOW_WIDTH = 800;
const int MIN_WINDOW_HEIGHT = 600;

// Animation timing
const double MOVEMENT_ANIMATION_DURATION = 50.0;  // ms
const double LINE_CLEAR_ANIMATION_DURATION = 300.0;
const double THEME_TRANSITION_DURATION = 500.0;

// Sound event enum
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

// Joystick mapping
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

// Forward declarations
class TetrimoneBlock;
class TetrimoneBoard;
struct TetrimoneApp;

// Firewerk particle structure
struct FireworkParticle {
    double x, y;
    double vx, vy;
    double lifetime;
    std::array<double, 3> color;
};

// Block trail structure
struct BlockTrail {
    double x, y;
    double age;
    std::array<double, 3> color;
};

// TetrimoneBlock class
class TetrimoneBlock {
private:
    int type;       // 0-6 for I, O, T, S, Z, J, L
    int rotation;   // 0-3 for rotations
    int x, y;       // Position on grid

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

// Main app structure
struct TetrimoneApp {
    SDL_Window* window;
    SDL_Renderer* renderer;
    TTF_Font* mainFont;
    TTF_Font* smallFont;
    TTF_Font* largeFont;
    
    TetrimoneBoard* board;
    
    // UI state
    bool running;
    bool showMenu;
    uint32_t lastFrameTime;
    uint32_t frameCount;
    float fps;
    
    // Audio
    bool soundEnabled;
    bool musicEnabled;
    
    // Joystick
    SDL_Joystick* joystick;
    bool joystickEnabled;
    JoystickMapping joystickMapping;
    
    // Settings
    int difficulty;  // 0=Zen, 1=Easy, 2=Medium, 3=Hard, 4=Extreme, 5=Insane
    
    // Dialog state
    enum class DialogState {
        NONE,
        MAIN_MENU,
        PAUSE_MENU,
        GAME_OVER,
        SETTINGS,
        HIGH_SCORES,
        GAME_SETUP,
        JOYSTICK_CONFIG,
        ABOUT,
        CONFIRMATION,
    };
    DialogState activeDialog;
};

// TetrimoneBoard class
class TetrimoneBoard {
private:
    TetrimoneApp* app;
    float heatLevel;
    std::vector<std::vector<int>> grid;
    std::unique_ptr<TetrimoneBlock> currentPiece;
    std::vector<std::unique_ptr<TetrimoneBlock>> nextPieces;
    
    int score;
    int level;
    int linesCleared;
    bool gameOver;
    bool paused;
    bool splashScreenActive;
    
    std::mt19937 rng;
    
    // Grid sizing
    int gridWidth;
    int gridHeight;
    int minBlockSize;
    
    // Game modes
    bool ghostPieceEnabled;
    bool retroModeActive;
    bool patrioticModeActive;
    bool simpleBlocksMode;
    
    // Animation state
    bool isThemeTransitioning;
    int oldThemeIndex;
    int newThemeIndex;
    double themeTransitionProgress;
    std::chrono::high_resolution_clock::time_point themeStartTime;
    
    bool lineClearActive;
    std::vector<int> linesBeingCleared;
    double lineClearProgress;
    std::chrono::high_resolution_clock::time_point lineClearStartTime;
    int currentAnimationType;
    
    double movementProgress;
    int lastPieceX, lastPieceY;
    std::chrono::high_resolution_clock::time_point movementStartTime;
    
    // Fireworks
    bool fireworksActive;
    std::vector<FireworkParticle> fireworkParticles;
    std::chrono::high_resolution_clock::time_point fireworksStartTime;
    
    // Block trails
    bool trailsEnabled;
    std::vector<BlockTrail> blockTrails;
    int maxTrailSegments;
    double trailOpacity;
    double trailDuration;
    
    // Background
    SDL_Texture* backgroundTexture;
    double backgroundOpacity;
    std::vector<SDL_Texture*> backgroundImages;
    int currentBackgroundIndex;
    bool useBackgroundImage;
    bool useBackgroundZip;
    bool isTransitioning;
    double transitionOpacity;
    int transitionDirection;
    SDL_Texture* oldBackgroundTexture;
    
    // Propaganda
    bool showPropagandaMessage;
    std::string currentPropagandaMessage;
    double propagandaMessageScale;
    bool propagandaScalingUp;
    int propagandaMessageDuration;
    std::chrono::high_resolution_clock::time_point propagandaStartTime;
    
    // Settings
    int junkLinesPercentage;
    int junkLinesPerLevel;
    int initialLevel;
    
    // Timing
    uint32_t lastDropTime;
    int dropSpeed;
    
    // Sequences
    int consecutiveClears;
    int maxConsecutiveClears;
    int lastClearCount;
    bool sequenceActive;
    
    // High scores
    Highscores highScores;
    
public:
    TetrimoneBoard(TetrimoneApp* appPtr);
    ~TetrimoneBoard();
    
    bool gridLinesEnabled;

    // Game loop
    void update(uint32_t deltaTime);
    void render(SDL_Renderer* renderer, int screenWidth, int screenHeight);
    
    // Piece management
    void movePiece(int dx, int dy);
    void rotatePiece(bool clockwise = true);
    void hardDropPiece();
    void dropPiece();
    
    // Grid management
    bool checkCollision(const TetrimoneBlock& piece) const;
    std::vector<int> checkForLines();
    void clearLines(const std::vector<int>& lines);
    void generateJunkLines(int percentage);
    void addJunkLinesFromBottom(int numLines);
    
    // Game state
    void restart();
    void togglePause();
    bool isPaused() const { return paused; }
    bool isGameOver() const { return gameOver; }
    bool isSplashScreenActive() const { return splashScreenActive; }
    
    // Getters
    int getScore() const { return score; }
    int getLevel() const { return level; }
    int getLinesCleared() const { return linesCleared; }
    const std::vector<std::vector<int>>& getGrid() const { return grid; }
    const TetrimoneBlock* getCurrentPiece() const { return currentPiece.get(); }
    const std::vector<std::unique_ptr<TetrimoneBlock>>& getNextPieces() const { return nextPieces; }
    int getGridWidth() const { return gridWidth; }
    int getGridHeight() const { return gridHeight; }
    int getGhostPieceY() const;
    
    // Setters
    void setApp(TetrimoneApp* appPtr) { app = appPtr; }
    void setGhostPieceEnabled(bool enabled) { ghostPieceEnabled = enabled; }
    void setRetroMode(bool enabled) { retroModeActive = enabled; }
    void setPatrioticMode(bool enabled) { patrioticModeActive = enabled; }
    void setSimpleBlocksMode(bool enabled) { simpleBlocksMode = enabled; }
    
    // Audio
    void playSound(GameSoundEvent event);
    
    // Background
    bool loadBackgroundImage(const std::string& filepath);
    bool loadBackgroundImagesFromZip(const std::string& filepath);
    void setUseBackgroundImage(bool use) { useBackgroundImage = use; }
    void setBackgroundOpacity(double opacity) { backgroundOpacity = opacity; }
    double getBackgroundOpacity() const { return backgroundOpacity; }
    void setUseBackgroundZip(bool use) { useBackgroundZip = use; }
    bool isUsingBackgroundZip() const { return useBackgroundZip; }
    
    // Theme
    void setTheme(int themeIndex);
    int getCurrentTheme() const;
    
    // Animation
    void startSmoothMovement(int newX, int newY);
    void updateSmoothMovement();
    void startLineClearAnimation(const std::vector<int>& clearedLines);
    void updateLineClearAnimation();
    void startThemeTransition(int targetTheme);
    void updateThemeTransition();
    void startFireworksAnimation(int linesCleared);
    void updateFireworksAnimation();
    
    // Propaganda
    void showPropagandaMsg(const std::string& message);
    void updatePropagandaMessage();
    
    // Utilities
    std::array<double, 3> getInterpolatedColor(int blockType, double progress) const;
    void getCurrentPieceInterpolatedPosition(double& x, double& y) const;
    bool isLineBeingCleared(int y) const;
};

// ============================================================================
// Function declarations
// ============================================================================

// Main app lifecycle
int tetrimone_main(int argc, char* argv[]);
void app_init(TetrimoneApp& app);
void app_cleanup(TetrimoneApp& app);
void app_handle_events(TetrimoneApp& app);
void app_update(TetrimoneApp& app);
void app_render(TetrimoneApp& app);

// Rendering
void render_game_area(SDL_Renderer* renderer, TetrimoneApp& app, int x, int y, int w, int h);
void render_sidebar(SDL_Renderer* renderer, TetrimoneApp& app, int x, int y, int w, int h);
void render_menu(SDL_Renderer* renderer, TetrimoneApp& app);
void render_pause_menu(SDL_Renderer* renderer, TetrimoneApp& app);
void render_game_over_screen(SDL_Renderer* renderer, TetrimoneApp& app);
void render_grid_lines(SDL_Renderer* renderer, int x, int y, int gridW, int gridH, int blockSize);
void render_blocks(SDL_Renderer* renderer, TetrimoneApp& app, int x, int y);
void render_current_piece(SDL_Renderer* renderer, TetrimoneApp& app, int x, int y);
void render_ghost_piece(SDL_Renderer* renderer, TetrimoneApp& app, int x, int y);
void render_next_pieces(SDL_Renderer* renderer, TetrimoneApp& app, int x, int y, int w, int h);
void render_text(SDL_Renderer* renderer, TTF_Font* font, const std::string& text, int x, int y, SDL_Color color);
void render_text_centered(SDL_Renderer* renderer, TTF_Font* font, const std::string& text, int x, int y, int w, SDL_Color color);

// UI dialogs (SDL modal-like)
void show_main_menu(TetrimoneApp& app);
void show_pause_menu(TetrimoneApp& app);
void show_game_over_dialog(TetrimoneApp& app);
void show_settings_dialog(TetrimoneApp& app);
void show_high_scores_dialog(TetrimoneApp& app);
void show_game_setup_dialog(TetrimoneApp& app);
void show_volume_dialog(TetrimoneApp& app);
void show_about_dialog(TetrimoneApp& app);

// Input handling
void handle_keyboard_input(TetrimoneApp& app, SDL_KeyboardEvent& event);
void handle_mouse_input(TetrimoneApp& app, SDL_MouseButtonEvent& event);
void handle_joystick_input(TetrimoneApp& app);

// File operations
bool file_open_dialog(std::string& result, const char* filter);
bool file_save_dialog(std::string& result, const char* filter);

// Utilities
SDL_Color rgb_to_sdl(double r, double g, double b);
void draw_rect_filled(SDL_Renderer* renderer, int x, int y, int w, int h, SDL_Color color);
void draw_rect_outline(SDL_Renderer* renderer, int x, int y, int w, int h, SDL_Color color, int thickness);
void draw_filled_rect_with_border(SDL_Renderer* renderer, int x, int y, int w, int h, SDL_Color fill, SDL_Color border);
void draw_circle(SDL_Renderer* renderer, int centerX, int centerY, int radius, SDL_Color color, bool filled);
void draw_block(SDL_Renderer* renderer, int x, int y, int size, SDL_Color color, bool is3d = true);

#endif // TETRIMONE_SDL2_H
