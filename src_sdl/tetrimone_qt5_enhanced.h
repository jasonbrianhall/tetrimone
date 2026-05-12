#ifndef TETRIMONE_QT5_H
#define TETRIMONE_QT5_H

#include <QApplication>
#include <QWidget>
#include <QLabel>
#include <QMenuBar>
#include <QMenu>
#include <QAction>
#include <QActionGroup>
#include <QTimer>
#include <QDialog>
#include <SDL2/SDL.h>
#include <cairo/cairo.h>
#include "audiomanager.h"
#include "tetrimone_core.h"

// Forward declarations
struct TetrimoneApp;
class GameAreaWidget;
class NextPieceWidget;
class TetrimoneWindow;

// ============================================================================
// SDL/Cairo GPU-Accelerated Renderer
// ============================================================================

class SDLCairoRenderer {
private:
    SDL_Window* window;
    SDL_Renderer* sdlRenderer;
    SDL_Texture* texture;
    cairo_surface_t* cairoSurface;
    cairo_t* cairoContext;
    int width, height;
    Uint32 pixelFormat;
    int pitch;
    void* pixelBuffer;

public:
    SDLCairoRenderer(int w, int h);
    ~SDLCairoRenderer();
    
    bool init(const char* title, Uint32 flags = SDL_WINDOW_OPENGL | SDL_WINDOW_SHOWN);
    cairo_t* getCairoContext() { return cairoContext; }
    cairo_surface_t* getCairoSurface() { return cairoSurface; }
    void clearCairoSurface(double r, double g, double b, double a = 1.0);
    void syncSurfaceToTexture();
    void present();
    bool resize(int newWidth, int newHeight);
    int getWidth() const { return width; }
    int getHeight() const { return height; }
    SDL_Window* getSDLWindow() { return window; }
    SDL_Renderer* getSDLRenderer() { return sdlRenderer; }
    bool saveFrameToPNG(const char* filename);
    void* getPixelBuffer() { return pixelBuffer; }
    void cleanup();
};

// ============================================================================
// Enhanced Game Window with Focus Tracking
// ============================================================================

class TetrimoneWindow : public QWidget {
    Q_OBJECT
public:
    explicit TetrimoneWindow(TetrimoneApp* app);
    
protected:
    void focusInEvent(QFocusEvent *event) override;
    void focusOutEvent(QFocusEvent *event) override;
    void closeEvent(QCloseEvent *event) override;
    
private:
    TetrimoneApp* app;
};

// ============================================================================
// Modal Dialog Base Class - Auto-pauses game
// ============================================================================

class GamePausingDialog : public QDialog {
    Q_OBJECT
public:
    explicit GamePausingDialog(TetrimoneApp* app, QWidget* parent = nullptr);
    ~GamePausingDialog();
    
protected:
    TetrimoneApp* app;
    bool wasGameRunning;
};

// ============================================================================
// Qt5-specific TetrimoneApp structure (Enhanced)
// ============================================================================

struct TetrimoneApp {
    QApplication* app = nullptr;
    TetrimoneWindow* window = nullptr;
    QWidget*      mainBox = nullptr;
    QWidget*      gameArea = nullptr;
    QWidget*      nextPieceArea = nullptr;

    QLabel*       scoreLabel = nullptr;
    QLabel*       levelLabel = nullptr;
    QLabel*       linesLabel = nullptr;
    QLabel*       difficultyLabel = nullptr;

    bool          backgroundMusicPlaying = false;
    TetrimoneBoard* board = nullptr;

    int           timerId = 0;
    int           dropSpeed = 500;

    QAction*      backgroundToggleMenuItem = nullptr;

    // ========== Game Control Menu ==========
    QMenuBar*     menuBar = nullptr;
    QAction*      startMenuItem = nullptr;
    QAction*      pauseMenuItem = nullptr;
    QAction*      restartMenuItem = nullptr;

    // ========== Sound Menu ==========
    QAction*      soundToggleMenuItem = nullptr;
    QAction*      trackMenuItems[5] = {nullptr};

    // ========== Difficulty Menu ==========
    QAction*      zenMenuItem = nullptr;
    QAction*      easyMenuItem = nullptr;
    QAction*      mediumMenuItem = nullptr;
    QAction*      hardMenuItem = nullptr;
    QAction*      extremeMenuItem = nullptr;
    QAction*      insaneMenuItem = nullptr;

    // ========== Theme Menu ==========
    QAction*      themeMenuItems[31] = {nullptr};

    // ========== Status Labels ==========
    QLabel*       sequenceLabel = nullptr;
    QLabel*       controlsLabel = nullptr;
    QLabel*       controlsHeaderLabel = nullptr;

    // ========== Game State ==========
    int           difficulty = 1; // 0=Zen, 1=Easy, 2=Medium, 3=Hard, 4=Extreme

    // ========== Input Handling ==========
    SDL_Joystick* joystick = nullptr;
    bool          joystickEnabled = false;
    int           joystickTimerId = 0;
    JoystickMapping joystickMapping;

    // ========== Focus and Pause State ==========
    bool          pausedByFocusLoss = false;
    bool          pausedByDialog = false;

    // ========== Rendering Mode Selection ==========
    enum RenderingMode {
        RENDER_CAIRO  = 0,
        RENDER_OPENGL = 1
    };
    RenderingMode renderingMode = RENDER_CAIRO;
    QAction*      renderModeMenuItems[2] = {nullptr};

    // ========== Command Line Arguments ==========
    CommandLineArgs* cmdlineArgs = nullptr;
    
    // ========== GPU-accelerated SDL/Cairo Renderer ==========
    SDLCairoRenderer* sdlCairoRenderer = nullptr;
    bool useGPUAcceleration = true;
};

// ============================================================================
// Qt5-specific UI functions
// ============================================================================

// Core game functions
void updateLabels(TetrimoneApp* app);
void startGame(TetrimoneApp* app);
void pauseGame(TetrimoneApp* app);
void resetUI(TetrimoneApp* app);
void cleanupApp(TetrimoneApp* app);
void updateDisplay(TetrimoneApp* app);

// SDL/Joystick initialization
void initSDL(TetrimoneApp* app);
void shutdownSDL(TetrimoneApp* app);

// Event handlers
void onKeyDownTick(TetrimoneApp* app);
void onKeyLeftTick(TetrimoneApp* app);
void onKeyRightTick(TetrimoneApp* app);
void onGameTick(TetrimoneApp* app);

// Game action handlers
void onStartGameAction(TetrimoneApp* app);
void onPauseGameAction(TetrimoneApp* app);
void onRestartGameAction(TetrimoneApp* app);
void onQuitGameAction(TetrimoneApp* app);
void onSoundToggleAction(TetrimoneApp* app, bool enabled);
void onDifficultyChanged(TetrimoneApp* app, int difficulty);

// Help dialogs
void onAboutDialog(void* menuItem, void* userData);
void onInstructionsDialog(void* menuItem, void* userData);
void showIdeologicalFailureDialog(TetrimoneApp* app);
void showPatrioticPerformanceDialog(TetrimoneApp* app);

// Dialog functions
void showGameSetupDialog(TetrimoneApp* app);
void showBlockSizeDialog(TetrimoneApp* app);
void showGameSizeDialog(TetrimoneApp* app);
void showBackgroundImageDialog(TetrimoneApp* app);
void showJoystickConfigDialog(TetrimoneApp* app);
void showVolumeDialog(TetrimoneApp* app);
void showBlockTrailsDialog(TetrimoneApp* app);
void showHighScoresDialog(TetrimoneApp* app);

#ifdef QT5
void onAppActivate(TetrimoneApp* app);
#endif

// UI setup
void setupMenuBar(TetrimoneApp* app);
void setupGameUI(TetrimoneApp* app, int width, int height);

// GPU rendering initialization and functions
void initGPURenderer(TetrimoneApp* app, int width, int height);
void shutdownGPURenderer(TetrimoneApp* app);
void renderFrameGPU(TetrimoneApp* app);

// Application entry point
int main_qt5(int argc, char* argv[], TetrimoneApp* app);

// Global variables
extern int BLOCK_SIZE;
extern int currentThemeIndex;
extern int GRID_WIDTH;
extern int GRID_HEIGHT;

// Key state variables
extern bool keyDownPressed;
extern bool keyLeftPressed;
extern bool keyRightPressed;

#endif // TETRIMONE_QT5_H
