// ============================================================================
// tetrimone_wx.h - wxWidgets frontend for Tetrimone
//
// Mirrors tetrimone_gtk.h: defines the frontend-specific TetrimoneApp and the
// callbacks the shared game code expects. wx types are only forward-declared
// here so shared .cpp files can include this header without pulling in all of
// wxWidgets.
// ============================================================================

#ifndef TETRIMONE_WX_H
#define TETRIMONE_WX_H

#include <SDL2/SDL.h>
#include <cairo/cairo.h>
#include "wx_glib_compat.h"
#include "audiomanager.h"
#include "tetrimone_core.h"

class wxWindow;
class wxMenuBar;
class wxMenuItem;
class wxStaticText;
class wxSizer;

// ============================================================================
// GTK-signature compatibility
//
// Several shared files (help.cpp, propaganda.cpp, freedom.cpp, highscores.cpp)
// contain dialog code written as GTK callbacks, e.g.
//     void onAboutDialog(GtkMenuItem*, gpointer) { ... GTK_WINDOW(app->window) ... }
// That code only talks to the dialog-helper API, which wx_dialog_helpers
// re-implements, so these shims let it compile unchanged for wxWidgets.
// ============================================================================

struct GtkMenuItem;  // opaque, never defined - menu handlers ignore it
#define GTK_WINDOW(w)    (w)
#define GTK_MENU_ITEM(w) (static_cast<GtkMenuItem*>(nullptr))

// ============================================================================
// wxWidgets-specific TetrimoneApp structure
// ============================================================================

struct TetrimoneApp {
    wxWindow*     window = nullptr;          // the main TetrimoneFrame
    wxWindow*     gameArea = nullptr;        // GameCanvas
    wxWindow*     nextPieceArea = nullptr;   // NextPieceCanvas
    wxSizer*      sidePanel = nullptr;

    wxStaticText* scoreLabel = nullptr;
    wxStaticText* levelLabel = nullptr;
    wxStaticText* linesLabel = nullptr;
    wxStaticText* sequenceLabel = nullptr;
    wxStaticText* difficultyLabel = nullptr;
    wxStaticText* controlsHeaderLabel = nullptr;
    wxStaticText* controlsLabel = nullptr;

    bool            backgroundMusicPlaying = false;
    TetrimoneBoard* board = nullptr;

    guint timerId = 0;
    int   dropSpeed = INITIAL_SPEED;
    int   difficulty = 2;  // 0=Zen, 1=Easy, 2=Medium, 3=Hard, 4=Extreme, 5=Insane

    // Menu
    wxMenuBar*  menuBar = nullptr;
    wxMenuItem* startMenuItem = nullptr;
    wxMenuItem* pauseMenuItem = nullptr;
    wxMenuItem* restartMenuItem = nullptr;
    wxMenuItem* soundToggleMenuItem = nullptr;
    wxMenuItem* backgroundToggleMenuItem = nullptr;
    wxMenuItem* gridLinesMenuItem = nullptr;
    wxMenuItem* blockTrailsMenuItem = nullptr;
    wxMenuItem* ghostPieceMenuItem = nullptr;
    wxMenuItem* simpleBlocksMenuItem = nullptr;
    wxMenuItem* retroMusicMenuItem = nullptr;
    wxMenuItem* zenMenuItem = nullptr;
    wxMenuItem* easyMenuItem = nullptr;
    wxMenuItem* mediumMenuItem = nullptr;
    wxMenuItem* hardMenuItem = nullptr;
    wxMenuItem* extremeMenuItem = nullptr;
    wxMenuItem* insaneMenuItem = nullptr;
    wxMenuItem* trackMenuItems[5] = {nullptr};
    static const int MAX_THEME_MENU_ITEMS = 64;  // NUM_COLOR_THEMES is runtime (vector size)
    wxMenuItem* themeMenuItems[MAX_THEME_MENU_ITEMS] = {nullptr};
    wxMenuItem* blockSizeRuleMenuItems[4] = {nullptr};
    int         openMenuDepth = 0;

    // Joystick
    SDL_Joystick*   joystick = nullptr;
    bool            joystickEnabled = false;
    guint           joystickTimerId = 0;
    JoystickMapping joystickMapping{};

    bool pausedByFocusLoss = false;

    // Kept for parity with the GTK3 struct (only Cairo is implemented)
    enum RenderingMode { RENDER_CAIRO = 0, RENDER_OPENGL = 1 };
    RenderingMode renderingMode = RENDER_CAIRO;

    const CommandLineArgs* cmdlineArgs = nullptr;
};

// ============================================================================
// Callbacks shared with the common game code (GTK-compatible signatures)
// ============================================================================

void onStartGame(GtkMenuItem* menuItem, gpointer userData);
void onPauseGame(GtkMenuItem* menuItem, gpointer userData);
void onRestartGame(GtkMenuItem* menuItem, gpointer userData);
void onQuitGame(GtkMenuItem* menuItem, gpointer userData);
void onAboutDialog(GtkMenuItem* menuItem, gpointer userData);         // help.cpp
void onInstructionsDialog(GtkMenuItem* menuItem, gpointer userData);  // help.cpp
void onViewHighScores(GtkMenuItem* menuItem, gpointer userData);      // highscores.cpp

gboolean onTimerTick(gpointer data);
gboolean onKeyDownTick(gpointer userData);
gboolean onKeyLeftTick(gpointer userData);
gboolean onKeyRightTick(gpointer userData);
gboolean pollJoystick(gpointer data);

void applyCommandLineArgs(TetrimoneApp* app, const CommandLineArgs& args);  // tetrimone_main.cpp
void setWindowIcon(wxWindow* window);                                       // icon.cpp

#endif // TETRIMONE_WX_H
