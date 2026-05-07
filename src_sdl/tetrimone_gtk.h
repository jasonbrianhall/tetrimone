#ifndef TETRIMONE_GTK3_H
#define TETRIMONE_GTK3_H

#include <gtk/gtk.h>
#include <gdk-pixbuf/gdk-pixbuf.h>
#include <SDL2/SDL.h>
#include "audiomanager.h"
#include "tetrimone_core.h"

// ============================================================================
// GTK3-specific callback data structures
// ============================================================================

struct BlockSizeCallbackData {
    struct TetrimoneApp* app;
    GtkWidget* label;
};

// ============================================================================
// GTK3-specific TetrimoneApp structure
// ============================================================================

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
    
    // Rendering mode selection
    enum RenderingMode {
        RENDER_CAIRO = 0,
        RENDER_OPENGL = 1
    };
    RenderingMode renderingMode;
    GtkWidget* renderModeMenuItems[2];  // Radio menu items for Cairo and OpenGL

};

// ============================================================================
// GTK3-specific drawing and event functions
// ============================================================================

// Cairo drawing (legacy support)
gboolean onDrawGameArea(GtkWidget* widget, cairo_t* cr, gpointer data);
gboolean onDrawNextPiece(GtkWidget* widget, cairo_t* cr, gpointer data);

void drawBackground(cairo_t *cr, TetrimoneBoard *board, int width, int height);

// Input and event handling (GTK3-specific)
gboolean onKeyPress(GtkWidget* widget, GdkEventKey* event, gpointer data);
gboolean onKeyDownTick(gpointer userData);
gboolean onKeyLeftTick(gpointer userData);
gboolean onKeyRightTick(gpointer userData);
gboolean onTimerTick(gpointer data);
gboolean onDeleteEvent(GtkWidget* widget, GdkEvent* event, gpointer userData);
gboolean onWindowFocusChanged(GtkWidget *widget, GdkEventFocus *event, gpointer userData);

// GTK Application lifecycle
void onAppActivate(GtkApplication* app, gpointer userData);
void onMenuActivated(GtkWidget* widget, gpointer userData);
void onMenuDeactivated(GtkWidget* widget, gpointer userData);

// GTK-specific menu callbacks
void createMenu(TetrimoneApp* app);
void rebuildRenderingArea(TetrimoneApp *app);
void onRenderModeChanged(GtkRadioMenuItem *menuItem, gpointer userData);
void onStartGame(GtkMenuItem* menuItem, gpointer userData);
void onPauseGame(GtkMenuItem* menuItem, gpointer userData);
void onRestartGame(GtkMenuItem* menuItem, gpointer userData);
void onQuitGame(GtkMenuItem* menuItem, gpointer userData);
void onSoundToggled(GtkCheckMenuItem* menuItem, gpointer userData);
void onDifficultyChanged(GtkRadioMenuItem* menuItem, gpointer userData);
void onAboutDialog(GtkMenuItem* menuItem, gpointer userData);
void onInstructionsDialog(GtkMenuItem* menuItem, gpointer userData);

// GTK-specific configuration dialogs
void adjustDropSpeed(TetrimoneApp* app);
void calculateBlockSize(TetrimoneApp* app);
gboolean pollJoystick(gpointer data);
void onBlockSizeDialog(GtkMenuItem* menuItem, gpointer userData);
void onBlockSizeValueChanged(GtkRange* range, gpointer data);
void onResizeWindowButtonClicked(GtkWidget* button, gpointer data);

// GTK Joystick configuration dialogs
void onJoystickConfig(GtkMenuItem* menuItem, gpointer userData);
void onJoystickRescan(GtkButton* button, gpointer userData);
void updateJoystickInfo(GtkLabel* infoLabel, TetrimoneApp* app);
void onJoystickMapApply(GtkButton* button, gpointer userData);
void onJoystickMapReset(GtkButton* button, gpointer userData);

// GTK Background and rendering dialogs
void onBackgroundImageDialog(GtkMenuItem* menuItem, gpointer userData);
void onBackgroundToggled(GtkCheckMenuItem* menuItem, gpointer userData);
void onBackgroundOpacityDialog(GtkMenuItem* menuItem, gpointer userData);
void onOpacityValueChanged(GtkRange* range, gpointer userData);
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

// GTK JPEG image loading utilities
cairo_surface_t* cairo_image_surface_create_from_jpeg(const char* filename);
cairo_surface_t* cairo_image_surface_create_from_memory(const void* data, size_t length);
int currentPatriotBackgroundIndex;

// GTK game feature dialogs
void onGhostPieceToggled(GtkCheckMenuItem* menuItem, gpointer userData);
void onViewHighScores(GtkMenuItem* menuItem, gpointer userData);
void setWindowIcon(GtkWindow* window);
void onBackgroundImagesDialog(GtkMenuItem* menuItem, gpointer userData);
void onSimpleBlocksToggled(GtkCheckMenuItem* menuItem, gpointer userData);
void onRetroMusicToggled(GtkCheckMenuItem* menuItem, gpointer userData);
void onTestSound(GtkButton* button, gpointer userData);
void onGameSetupDialog(GtkMenuItem* menuItem, gpointer userData);
void onResetSettings(GtkMenuItem* menuItem, gpointer userData);
void onThemeChanged(GtkRadioMenuItem *menuItem, gpointer userData);

// Heat effect functions - Cairo versions (legacy)
void drawFreezyEffect(cairo_t* cr, double x, double y, double size, float heatLevel, double time);
void drawFireyGlow(cairo_t* cr, double x, double y, double size, float heatLevel, double time);

// Block trails animation configuration
void onBlockTrailsToggled(GtkCheckMenuItem* menuItem, gpointer userData);
void onBlockTrailsConfig(GtkMenuItem* menuItem, gpointer userData);
void onTrailOpacityChanged(GtkAdjustment* adj, gpointer data);
void onTrailDurationChanged(GtkAdjustment* adj, gpointer data);

#endif // TETRIMONE_GTK3_H
