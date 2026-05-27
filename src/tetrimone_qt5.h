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
#include <SDL2/SDL.h>
#include <cairo/cairo.h>
#include "audiomanager.h"
#include "tetrimone_core.h"

// Forward declarations
struct TetrimoneApp;
class GameAreaWidget;
class NextPieceWidget;

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
// Qt5-specific TetrimoneApp structure
// ============================================================================

struct TetrimoneApp {
    QApplication* app = nullptr;
    QWidget*      window = nullptr;
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
    QTimer*       gameTimer = nullptr;  // Pointer to the game timer for updating speed

    QAction*      backgroundToggleMenuItem = nullptr;

    // Menu related widgets
    QMenuBar*     menuBar = nullptr;
    QAction*      startMenuItem = nullptr;
    QAction*      pauseMenuItem = nullptr;
    QAction*      restartMenuItem = nullptr;
    QAction*      soundToggleMenuItem = nullptr;
    QAction*      zenMenuItem = nullptr;
    QAction*      easyMenuItem = nullptr;
    QAction*      mediumMenuItem = nullptr;
    QAction*      hardMenuItem = nullptr;
    QAction*      extremeMenuItem = nullptr;
    QAction*      insaneMenuItem = nullptr;

    QAction*      trackMenuItems[5] = {nullptr};
    QAction*      themeMenuItems[31] = {nullptr};

    // Additional menu items from GTK3 version
    QAction*      blockSizeMenuItem = nullptr;
    QAction*      joystickConfigMenuItem = nullptr;
    QAction*      backgroundImageMenuItem = nullptr;
    QAction*      backgroundToggleMenuItem_Display = nullptr;
    QAction*      backgroundOpacityMenuItem = nullptr;
    QAction*      backgroundZipMenuItem = nullptr;
    QAction*      volumeMenuItem = nullptr;
    QAction*      blockSizeRulesMenuItem = nullptr;
    QAction*      gameSizeMenuItem = nullptr;
    QAction*      gridLinesMenuItem = nullptr;
    QAction*      ghostPieceMenuItem = nullptr;
    QAction*      highScoresMenuItem = nullptr;
    QAction*      backgroundImagesMenuItem = nullptr;
    QAction*      simpleBlocksMenuItem = nullptr;
    QAction*      retroMusicMenuItem = nullptr;
    QAction*      blockTrailsMenuItem = nullptr;
    QAction*      blockTrailsConfigMenuItem = nullptr;
    QAction*      gameSetupMenuItem = nullptr;
    QAction*      resetSettingsMenuItem = nullptr;

    QLabel*       sequenceLabel = nullptr;
    QLabel*       controlsLabel = nullptr;

    int           difficulty = 1; // 0=Zen, 1=Easy, 2=Medium, 3=Hard, 4=Extreme

    QLabel*       controlsHeaderLabel = nullptr;

    SDL_Joystick* joystick = nullptr;
    bool          joystickEnabled = false;
    int           joystickTimerId = 0;

    JoystickMapping joystickMapping;
    bool            pausedByFocusLoss = false;

    // Rendering mode selection
    enum RenderingMode {
        RENDER_CAIRO  = 0,
        RENDER_OPENGL = 1
    };
    RenderingMode renderingMode = RENDER_CAIRO;

    QAction*      renderModeMenuItems[2] = {nullptr};

    CommandLineArgs* cmdlineArgs = nullptr;
    
    // GPU-accelerated SDL/Cairo renderer
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

// Help dialogs
void onAboutDialog(void* menuItem, void* userData);
void onInstructionsDialog(void* menuItem, void* userData);
void showIdeologicalFailureDialog(TetrimoneApp* app);
void showPatrioticPerformanceDialog(TetrimoneApp* app);

#ifdef QT5
void onAppActivate(TetrimoneApp* app);
#endif

// Menu actions
void onStartGameAction(TetrimoneApp* app);
void onPauseGameAction(TetrimoneApp* app);
void onRestartGameAction(TetrimoneApp* app);
void onQuitGameAction(TetrimoneApp* app);
void onSoundToggleAction(TetrimoneApp* app, bool enabled);
void onDifficultyChanged(TetrimoneApp* app, int difficulty);

// TODO: Additional menu callbacks from GTK3 version
void onBlockSizeDialog(TetrimoneApp* app);
void onBlockSizeValueChanged(int value, TetrimoneApp* app);
void onResizeWindowButtonClicked(TetrimoneApp* app);
void onJoystickConfig(TetrimoneApp* app);
void onJoystickRescan(TetrimoneApp* app);
void updateJoystickInfo(TetrimoneApp* app);
void onJoystickMapApply(TetrimoneApp* app);
void onJoystickMapReset(TetrimoneApp* app);
void onBackgroundImageDialog(TetrimoneApp* app);
void onBackgroundToggled(TetrimoneApp* app, bool enabled);
void onBackgroundOpacityDialog(TetrimoneApp* app);
void onOpacityValueChanged(int value, TetrimoneApp* app);
void updateSizeValueLabel(int value, TetrimoneApp* app);
void onBackgroundZipDialog(TetrimoneApp* app);
void onVolumeDialog(TetrimoneApp* app);
void onVolumeValueChanged(int value, TetrimoneApp* app);
void onMusicVolumeValueChanged(int value, TetrimoneApp* app);
void onTrackToggled(TetrimoneApp* app, int trackIndex, bool enabled);
void onBlockSizeRulesChanged(TetrimoneApp* app, int mode);
void onGameSizeDialog(TetrimoneApp* app);
void onGridLinesToggled(TetrimoneApp* app, bool enabled);
void updateWidthValueLabel(int value, TetrimoneApp* app);
void updateHeightValueLabel(int value, TetrimoneApp* app);
void onGhostPieceToggled(TetrimoneApp* app, bool enabled);
void onViewHighScores(TetrimoneApp* app);
void onBackgroundImagesDialog(TetrimoneApp* app);
void onSimpleBlocksToggled(TetrimoneApp* app, bool enabled);
void onRetroMusicToggled(TetrimoneApp* app, bool enabled);
void onTestSound(TetrimoneApp* app);
void onGameSetupDialog(TetrimoneApp* app);
void onResetSettings(TetrimoneApp* app);
void onThemeChanged(TetrimoneApp* app, int themeIndex);
void onBlockTrailsToggled(TetrimoneApp* app, bool enabled);
void onBlockTrailsConfig(TetrimoneApp* app);
void onTrailOpacityChanged(int value, TetrimoneApp* app);
void onTrailDurationChanged(int value, TetrimoneApp* app);
void onRenderModeChanged(TetrimoneApp* app, int mode);

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
