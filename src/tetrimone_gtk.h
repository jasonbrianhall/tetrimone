#ifndef TETRIMONE_GTK4_H
#define TETRIMONE_GTK4_H

#include <gtk/gtk.h>
#include <gdk-pixbuf/gdk-pixbuf.h>
#include <SDL2/SDL.h>
#include <string>
#include "audiomanager.h"
#include "tetrimone_core.h"

// ============================================================================
// GTK4-specific callback data structures
// ============================================================================

struct BlockSizeCallbackData {
    struct TetrimoneApp* app;
    GtkWidget* label;
};

// ============================================================================
// GTK4-specific TetrimoneApp structure
//
// NOTE: GTK4 removed GtkMenuBar / GtkMenu / GtkMenuItem / GtkRadioMenuItem /
// GtkCheckMenuItem entirely. The menu bar is now a GMenu model rendered by a
// GtkPopoverMenuBar, and each menu item is backed by a GSimpleAction living
// in a GSimpleActionGroup ("game") inserted on the window. Anywhere the old
// code did gtk_widget_set_sensitive(app->fooMenuItem, ...) or
// gtk_check_menu_item_set_active(...), the GTK4 code now calls
// g_simple_action_set_enabled(app->fooAction, ...) or toggles the action's
// state via g_simple_action_set_state(...).
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

    // Menu bar (GMenu model + action group; no more GtkMenuBar/GtkMenuItem)
    GtkWidget* menuBar;                 // GtkPopoverMenuBar
    GSimpleActionGroup* actionGroup;    // inserted on window as "game"

    // Game actions (replace startMenuItem/pauseMenuItem/restartMenuItem)
    GSimpleAction* startAction;
    GSimpleAction* pauseAction;
    GSimpleAction* restartAction;
    std::string pauseActionLabel = "Pause";

    // Toggle / radio-backed actions (replace GtkCheckMenuItem / GtkRadioMenuItem)
    GSimpleAction* soundToggleAction;
    GSimpleAction* backgroundToggleAction;
    GSimpleAction* gridLinesToggleAction;
    GSimpleAction* blockTrailsToggleAction;
    GSimpleAction* simpleBlocksToggleAction;
    GSimpleAction* retroMusicToggleAction;
    GSimpleAction* ghostPieceToggleAction;
    GSimpleAction* difficultyAction;    // stateful string action
    GSimpleAction* themeAction;         // stateful int action (theme index)
    GSimpleAction* minBlockSizeAction;  // stateful int action (1-4)
    GSimpleAction* trackActions[5];     // per-track toggle actions

    GtkWidget* sequenceLabel;
    GtkWidget* controlsLabel;
    int difficulty; // 1 = Easy, 2 = Medium, 3 = Hard, 0 = Zen, 4 = Extreme, 5 = Insane
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
    GSimpleAction* renderModeAction; // stateful int action (0=Cairo, 1=OpenGL)

    // Event controllers (owned by the widgets they're attached to; kept here
    // in case other translation units need to reference them)
    GtkEventController* keyController;
    GtkEventController* focusController;
};

// ============================================================================
// GTK4-specific drawing and event functions
// ============================================================================

// Cairo drawing: GTK4 uses a draw-function instead of the "draw" signal.
void onDrawGameArea(GtkDrawingArea* widget, cairo_t* cr, int width, int height, gpointer data);
void onDrawNextPiece(GtkDrawingArea* widget, cairo_t* cr, int width, int height, gpointer data);

void drawBackground(cairo_t *cr, TetrimoneBoard *board, int width, int height);

// Input and event handling (GTK4-specific: GtkEventControllerKey signals)
gboolean onKeyPressed(GtkEventControllerKey* controller, guint keyval, guint keycode,
                       GdkModifierType state, gpointer data);
void onKeyReleased(GtkEventControllerKey* controller, guint keyval, guint keycode,
                    GdkModifierType state, gpointer data);
gboolean onKeyDownTick(gpointer userData);
gboolean onKeyLeftTick(gpointer userData);
gboolean onKeyRightTick(gpointer userData);
gboolean onTimerTick(gpointer data);
gboolean onCloseRequest(GtkWindow* window, gpointer userData);
void onWindowFocusEnter(GtkEventControllerFocus* controller, gpointer userData);
void onWindowFocusLeave(GtkEventControllerFocus* controller, gpointer userData);

// GTK Application lifecycle
void onAppActivate(GtkApplication* app, gpointer userData);

// GTK4-specific menu (GMenu / GAction) construction
void createMenu(TetrimoneApp* app);
void rebuildRenderingArea(TetrimoneApp *app);
void onRenderModeChanged(GSimpleAction* action, GVariant* param, gpointer userData);
void onStartGame(GSimpleAction* action, GVariant* param, gpointer userData);
void onPauseGame(GSimpleAction* action, GVariant* param, gpointer userData);
void onRestartGame(GSimpleAction* action, GVariant* param, gpointer userData);
void onQuitGame(GSimpleAction* action, GVariant* param, gpointer userData);
void onSoundToggled(GSimpleAction* action, GVariant* param, gpointer userData);
void onDifficultyChanged(GSimpleAction* action, GVariant* param, gpointer userData);
void onAboutDialog(GSimpleAction* action, GVariant* param, gpointer userData);
void onInstructionsDialog(GSimpleAction* action, GVariant* param, gpointer userData);

// Helpers that actually apply pause/resume/start (called directly by game
// logic as well as by the action callbacks above)
void pauseGame(TetrimoneApp* app);
void unpauseGame(TetrimoneApp* app);
void startGame(TetrimoneApp* app);
void adjustDropSpeed(TetrimoneApp* app);
void resetUI(TetrimoneApp* app);
void updateLabels(TetrimoneApp* app);
void updateDisplay(TetrimoneApp* app);
void calculateBlockSize(TetrimoneApp* app);
void rebuildGameUI(TetrimoneApp* app);
void cleanupApp(gpointer data);
void initSDL(TetrimoneApp* app);
bool checkAndRecordHighScoreWrapper(TetrimoneApp* app);
void showIdeologicalFailureDialog(TetrimoneApp* app);
void showPatrioticPerformanceDialog(TetrimoneApp* app);

// GTK-specific configuration dialogs
gboolean pollJoystick(gpointer data);
void onBlockSizeDialog(GSimpleAction* action, GVariant* param, gpointer userData);
void onBlockSizeValueChanged(GtkRange* range, gpointer data);
void onResizeWindowButtonClicked(GtkWidget* button, gpointer data);

// GTK Joystick configuration dialogs
void onJoystickConfig(GSimpleAction* action, GVariant* param, gpointer userData);
void onJoystickRescan(GtkButton* button, gpointer userData);
void updateJoystickInfo(GtkLabel* infoLabel, TetrimoneApp* app);
void onJoystickMapApply(GtkButton* button, gpointer userData);
void onJoystickMapReset(GtkButton* button, gpointer userData);

// GTK Background and rendering dialogs
void onBackgroundImageDialog(GSimpleAction* action, GVariant* param, gpointer userData);
void onBackgroundToggled(GSimpleAction* action, GVariant* state, gpointer userData);
void onBackgroundOpacityDialog(GSimpleAction* action, GVariant* param, gpointer userData);
void onOpacityValueChanged(GtkRange* range, gpointer userData);
void updateSizeValueLabel(GtkRange* range, gpointer data);
void onBackgroundZipDialog(GSimpleAction* action, GVariant* param, gpointer userData);
void onVolumeDialog(GSimpleAction* action, GVariant* param, gpointer userData);
void onVolumeValueChanged(GtkRange* range, gpointer userData);
void onMusicVolumeValueChanged(GtkRange* range, gpointer userData);
void onTrackToggled(GSimpleAction* action, GVariant* state, gpointer userData);
void onBlockSizeRulesChanged(GSimpleAction* action, GVariant* param, gpointer userData);
void onGameSizeDialog(GSimpleAction* action, GVariant* param, gpointer userData);
void onGridLinesToggled(GSimpleAction* action, GVariant* state, gpointer userData);
void updateWidthValueLabel(GtkAdjustment* adj, gpointer data);
void updateHeightValueLabel(GtkAdjustment* adj, gpointer data);

// GTK game feature dialogs
void onGhostPieceToggled(GSimpleAction* action, GVariant* state, gpointer userData);
void onViewHighScores(GSimpleAction* action, GVariant* param, gpointer userData);
void setWindowIcon(GtkWindow* window);
void onBackgroundImagesDialog(GSimpleAction* action, GVariant* param, gpointer userData);
void onSimpleBlocksToggled(GSimpleAction* action, GVariant* state, gpointer userData);
void onRetroMusicToggled(GSimpleAction* action, GVariant* state, gpointer userData);
void onTestSound(GtkButton* button, gpointer userData);
void onGameSetupDialog(GSimpleAction* action, GVariant* param, gpointer userData);
void onResetSettings(GSimpleAction* action, GVariant* param, gpointer userData);
void onThemeChanged(GSimpleAction* action, GVariant* param, gpointer userData);

// Block trails animation configuration
void onBlockTrailsToggled(GSimpleAction* action, GVariant* state, gpointer userData);
void onBlockTrailsConfig(GSimpleAction* action, GVariant* param, gpointer userData);
void onTrailOpacityChanged(GtkAdjustment* adj, gpointer data);
void onTrailDurationChanged(GtkAdjustment* adj, gpointer data);

#endif // TETRIMONE_GTK4_H
