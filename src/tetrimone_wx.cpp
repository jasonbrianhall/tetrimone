// ============================================================================
// tetrimone_wx.cpp - wxWidgets frontend for Tetrimone
//
// Feature-for-feature port of tetrimone_gtk3.cpp / gtkstuff.cpp /
// background.cpp / joystick_gtk.cpp. Rendering still goes through the shared
// Cairo drawing code: each frame is drawn into an offscreen Cairo image
// surface and blitted to the window, so drawgame_cairo.cpp is unchanged and
// the game looks identical on every platform.
// ============================================================================

#include "tetrimone_wx.h"
#include "wx_dialog_helpers.h"

#include <wx/wx.h>
#include <wx/artprov.h>
#include <wx/display.h>
#include <wx/filename.h>
#include <wx/mstream.h>
#include <wx/statbmp.h>
#include <wx/stdpaths.h>

#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstring>
#include <functional>
#include <iostream>
#include <map>
#include <random>
#include <string>

#include "audiomanager.h"
#include "commandline.h"
#include "freedom_messages.h"
#include "highscores.h"
#include "propaganda_messages.h"

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

using namespace WXHelpers;

// Key repeat state (defined in tetrimone.cpp)
extern bool keyDownPressed;
extern bool keyLeftPressed;
extern bool keyRightPressed;
extern int keyDownTimer;
extern int keyLeftTimer;
extern int keyRightTimer;
extern int keyDownDelay;
extern int keyLeftDelay;
extern int keyRightDelay;
extern int keyDownCount;
extern int keyLeftCount;
extern int keyRightCount;

extern int BLOCK_SIZE;
extern int currentThemeIndex;
extern int GRID_WIDTH;
extern int GRID_HEIGHT;

// Joystick core (joystick_core.cpp)
extern bool processJoystickButtons(
    TetrimoneApp* app,
    void (*onPauseCallback)(TetrimoneApp*, bool shouldPause),
    void (*onRotateCallback)(TetrimoneApp*, bool clockwise),
    void (*onHardDropCallback)(TetrimoneApp*));
extern void getJoystickAnalogMovement(TetrimoneApp* app, int* outX, int* outY);

// Shared Cairo scene drawing (drawgame_board.cpp)
void OnDrawGameAreaCairo(cairo_t* cr, TetrimoneApp* app, int width, int height);
void onDrawNextPieceCairo(cairo_t* cr, TetrimoneApp* app, int width, int height);

namespace {

wxString U(const std::string& s) { return wxString::FromUTF8(s.c_str()); }
wxString U(const char* s) { return wxString::FromUTF8(s); }

// ----------------------------------------------------------------------------
// Side-panel text
// ----------------------------------------------------------------------------

const char* CONTROLS_TEXT =
    "Keyboard Controls:\n"
    "• Left/Right/A/D: Move block\n"
    "• Up/W: Rotate clockwise\n"
    "• Z: Rotate counter-clockwise\n"
    "• Down/S: Soft drop\n"
    "• Space: Hard drop\n"
    "• P: Pause/Resume game\n"
    "• R: Restart game\n"
    "• M: Toggle music\n\n"
    "Controller support is available.\n"
    "Configure in Controls menu.";

const char* CONTROLS_TEXT_RETRO =
    "Управление клавиатурой:\n"
    "• Влево/Вправо/A/D: Перемещение блока\n"
    "• Вверх/W: Поворот по часовой стрелке\n"
    "• Z: Поворот против часовой стрелки\n"
    "• Вниз/S: Мягкое падение\n"
    "• Пробел: Быстрое падение\n"
    "• P: Пауза/Продолжение игры\n"
    "• R: Перезапуск игры\n"
    "• M: Переключение музыки\n\n"
    "Поддержка контроллера доступна.\n"
    "Настройка в меню Управление.";

const char* CONTROLS_TEXT_PATRIOTIC =
    "Freedom Controls:\n"
    "• Left/Right/A/D: Exercise your right to move blocks\n"
    "• Up/W: Rotate clockwise (like freedom)\n"
    "• Z: Rotate counter-clockwise (constitutional right)\n"
    "• Down/S: Soft drop (gentle like democracy)\n"
    "• Space: Hard drop (decisive like America)\n"
    "• P: Pause/Resume (work-life balance)\n"
    "• R: Restart (second chances, American dream)\n"
    "• M: Toggle music (freedom of choice)\n\n"
    "Controller support available.\n"
    "Configure in Controls menu.\n"
    "🇺🇸 GOD BLESS AMERICA! 🦅";

const char* THEME_NAMES[] = {
    "Watercolor", "Neon", "Pastel", "Earth Tones", "Monochrome Blue",
    "Monochrome Green", "Sunset", "Ocean", "Grayscale", "Candy",
    "Neon Dark", "Jewel Tones", "Retro Gaming", "Autumn", "Winter",
    "Spring", "Summer", "Monochrome Purple", "Desert", "Rainbow",
    "Art Deco", "Northern Lights", "Moroccan Tiles", "Bioluminescence", "Fossil",
    "Silk Road", "Digital Glitch", "Botanical", "Jazz Age", "Steampunk",
    "USA", "Soviet Retro"
};
const int NUM_THEME_NAMES = sizeof(THEME_NAMES) / sizeof(THEME_NAMES[0]);

int themeCount() {
    return std::min<int>({static_cast<int>(NUM_COLOR_THEMES), NUM_THEME_NAMES,
                          TetrimoneApp::MAX_THEME_MENU_ITEMS});
}

// ----------------------------------------------------------------------------
// Menu ids
// ----------------------------------------------------------------------------

enum {
    ID_START = wxID_HIGHEST + 1,
    ID_PAUSE,
    ID_RESTART,
    ID_HIGH_SCORES,
    ID_DIFFICULTY_FIRST,                          // 6 radio items
    ID_DIFFICULTY_LAST = ID_DIFFICULTY_FIRST + 5,
    ID_BLOCK_SIZE,
    ID_BG_IMAGE,
    ID_BG_ZIP,
    ID_BG_OPACITY,
    ID_BG_TOGGLE,
    ID_GRID_LINES,
    ID_BLOCK_TRAILS,
    ID_BLOCK_TRAILS_CONFIG,
    ID_GHOST_PIECE,
    ID_SIMPLE_BLOCKS,
    ID_FULLSCREEN,
    ID_SOUND_TOGGLE,
    ID_VOLUME,
    ID_RETRO_MUSIC,
    ID_TRACK_FIRST,
    ID_TRACK_LAST = ID_TRACK_FIRST + 4,
    ID_JOYSTICK_CONFIG,
    ID_MIN_BLOCK_FIRST,
    ID_MIN_BLOCK_LAST = ID_MIN_BLOCK_FIRST + 3,
    ID_GAME_SIZE,
    ID_GAME_SETUP,
    ID_INSTRUCTIONS,
    ID_THEME_FIRST,
    ID_THEME_LAST = ID_THEME_FIRST + TetrimoneApp::MAX_THEME_MENU_ITEMS - 1
};

// ----------------------------------------------------------------------------
// Small helpers
// ----------------------------------------------------------------------------

TetrimoneApp* g_app = nullptr;  // the one running game (for event routing)

void runLater(guint ms, std::function<void()> fn) {
    auto* heap = new std::function<void()>(std::move(fn));
    g_timeout_add(ms, [](gpointer data) -> gboolean {
        auto* f = static_cast<std::function<void()>*>(data);
        (*f)();
        delete f;
        return FALSE;
    }, heap);
}

void checkItem(wxMenuItem* item, bool checked) {
    if (item && item->IsCheckable()) item->Check(checked);
}

void enableItem(wxMenuItem* item, bool enabled) {
    if (item) item->Enable(enabled);
}

// Returns true if the label text actually changed
bool setMarkup(wxStaticText* label, const std::string& markup) {
    if (!label) return false;
    // Labels are refreshed on every game tick; only touch them on change to
    // avoid flicker and needless relayout.
    static std::map<wxStaticText*, std::string> lastMarkup;
    auto it = lastMarkup.find(label);
    if (it != lastMarkup.end() && it->second == markup) return false;
    lastMarkup[label] = markup;

    wxString text = U(markup);
    if (!label->SetLabelMarkup(text)) label->SetLabelText(U(stripMarkup(markup)));
    return true;
}

// fit=false only grows the window when the side panel no longer fits (cheap
// enough to call on every score change). fit=true resizes the window so the
// board is drawn at the current BLOCK_SIZE (after Block Size / Game Size).
void relayoutWindow(TetrimoneApp* app, bool fit = false);

// Previous theme when toggling retro/patriotic modes with '.' and ','
int savedThemeIndex = 0;

} // namespace

// ============================================================================
// Cairo <-> wx image bridging
// ============================================================================

namespace {

// Converts a wxImage to a Cairo ARGB32 (premultiplied) surface
cairo_surface_t* surfaceFromImage(const wxImage& image) {
    if (!image.IsOk()) return nullptr;

    int w = image.GetWidth();
    int h = image.GetHeight();
    cairo_surface_t* surface = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, w, h);
    if (cairo_surface_status(surface) != CAIRO_STATUS_SUCCESS) {
        cairo_surface_destroy(surface);
        return nullptr;
    }

    cairo_surface_flush(surface);
    unsigned char* dst = cairo_image_surface_get_data(surface);
    int stride = cairo_image_surface_get_stride(surface);
    const unsigned char* rgb = image.GetData();
    const unsigned char* alpha = image.HasAlpha() ? image.GetAlpha() : nullptr;

    for (int y = 0; y < h; ++y) {
        uint32_t* row = reinterpret_cast<uint32_t*>(dst + y * stride);
        for (int x = 0; x < w; ++x) {
            size_t i = static_cast<size_t>(y) * w + x;
            uint32_t a = alpha ? alpha[i] : 255;
            uint32_t r = rgb[i * 3 + 0] * a / 255;
            uint32_t g = rgb[i * 3 + 1] * a / 255;
            uint32_t b = rgb[i * 3 + 2] * a / 255;
            row[x] = (a << 24) | (r << 16) | (g << 8) | b;
        }
    }
    cairo_surface_mark_dirty(surface);
    return surface;
}

// A window whose contents are painted by a Cairo callback via an offscreen
// image surface. Handles HiDPI by rendering at the content scale factor.
class CairoCanvas : public wxWindow {
public:
    typedef std::function<void(cairo_t*, int, int)> DrawFn;

    CairoCanvas(wxWindow* parent, const wxSize& size, DrawFn draw)
        : wxWindow(parent, wxID_ANY, wxDefaultPosition, size,
                   wxWANTS_CHARS | wxFULL_REPAINT_ON_RESIZE | wxBORDER_NONE),
          draw_(std::move(draw)) {
        SetBackgroundStyle(wxBG_STYLE_PAINT);
        SetMinSize(size);
        Bind(wxEVT_PAINT, &CairoCanvas::onPaint, this);
    }

    ~CairoCanvas() override {
        if (surface_) cairo_surface_destroy(surface_);
    }

    void setFixedSize(const wxSize& size) {
        SetMinSize(size);
        SetMaxSize(size);
        SetSize(size);
    }

    // No focus rectangle, but we do want keyboard focus so arrow keys reach us
    bool AcceptsFocus() const override { return true; }

private:
    void onPaint(wxPaintEvent&) {
        wxPaintDC dc(this);
        wxSize size = GetClientSize();
        if (size.x <= 0 || size.y <= 0) return;

        double scale = GetContentScaleFactor();
        if (scale <= 0) scale = 1.0;
        int pw = static_cast<int>(std::ceil(size.x * scale));
        int ph = static_cast<int>(std::ceil(size.y * scale));

        if (!surface_ || pw != surfaceW_ || ph != surfaceH_) {
            if (surface_) cairo_surface_destroy(surface_);
            surface_ = cairo_image_surface_create(CAIRO_FORMAT_RGB24, pw, ph);
            surfaceW_ = pw;
            surfaceH_ = ph;
            image_.Create(pw, ph, false);
        }

        cairo_t* cr = cairo_create(surface_);
        cairo_set_source_rgb(cr, 0.1, 0.1, 0.1);
        cairo_paint(cr);
        cairo_scale(cr, scale, scale);
        if (draw_) draw_(cr, size.x, size.y);
        cairo_destroy(cr);
        cairo_surface_flush(surface_);

        // RGB24 is xRGB in native-endian 32-bit words
        const unsigned char* src = cairo_image_surface_get_data(surface_);
        int stride = cairo_image_surface_get_stride(surface_);
        unsigned char* dst = image_.GetData();
        for (int y = 0; y < ph; ++y) {
            const uint32_t* row = reinterpret_cast<const uint32_t*>(src + y * stride);
            unsigned char* out = dst + static_cast<size_t>(y) * pw * 3;
            for (int x = 0; x < pw; ++x) {
                uint32_t p = row[x];
                out[0] = (p >> 16) & 0xFF;
                out[1] = (p >> 8) & 0xFF;
                out[2] = p & 0xFF;
                out += 3;
            }
        }

        wxBitmap bitmap(image_, -1, scale);
        dc.DrawBitmap(bitmap, 0, 0, false);
    }

    DrawFn draw_;
    cairo_surface_t* surface_ = nullptr;
    int surfaceW_ = 0;
    int surfaceH_ = 0;
    wxImage image_;
};

wxSize gameAreaSize() { return wxSize(GRID_WIDTH * BLOCK_SIZE, GRID_HEIGHT * BLOCK_SIZE); }

// Smallest block size the board shrinks to when the window is made small
const int SMALLEST_BLOCK_SIZE = 8;

wxSize nextPieceAreaSize() {
    int previewBlockSize = BLOCK_SIZE / 2;
    return wxSize(3 * 4 * previewBlockSize, 4 * previewBlockSize + 30);
}

} // namespace

// Image loading used by the shared background code. wxImage handles PNG and
// JPEG on every platform (and Unicode paths on Windows, which cairo's own PNG
// loader does not), so no libjpeg/gdk-pixbuf dependency is needed.
cairo_surface_t* cairo_image_surface_create_from_jpeg(const char* filename) {
    wxImage image;
    wxLogNull quiet;
    if (!filename || !image.LoadFile(wxString::FromUTF8(filename), wxBITMAP_TYPE_ANY)) {
        return nullptr;
    }
    return surfaceFromImage(image);
}

cairo_surface_t* cairo_image_surface_create_from_memory(const void* data, size_t length) {
    if (!data || length == 0) return nullptr;
    wxMemoryInputStream stream(data, length);
    wxImage image;
    wxLogNull quiet;
    if (!image.LoadFile(stream, wxBITMAP_TYPE_ANY)) return nullptr;
    return surfaceFromImage(image);
}

static cairo_surface_t* loadImageSurface(const wxString& path) {
    wxImage image;
    wxLogNull quiet;
    if (!image.LoadFile(path, wxBITMAP_TYPE_ANY)) return nullptr;
    return surfaceFromImage(image);
}

// ============================================================================
// Main window
// ============================================================================

class TetrimoneFrame : public wxFrame {
public:
    explicit TetrimoneFrame(TetrimoneApp* app);

    void buildMenu();
    void buildContent();
    void rebuildContent();

    wxPanel* panel() const { return panel_; }

private:
    void onClose(wxCloseEvent& event);
    void onActivate(wxActivateEvent& event);
    void onCharHook(wxKeyEvent& event);
    void onMenuOpen(wxMenuEvent& event);
    void onMenuClose(wxMenuEvent& event);
    void onAnyMenuCommand(wxCommandEvent& event);
    void finishMenuPause();

    TetrimoneApp* app_;
    wxPanel* panel_ = nullptr;
    wxStaticBoxSizer* nextBox_ = nullptr;
    bool pausedByMenu_ = false;
};

// ============================================================================
// Board animations that live in the frontend (same as tetrimone_gtk3.cpp)
// ============================================================================

void TetrimoneBoard::startFireworksAnimation(int linesCleared) {
    if (linesCleared != 4) return;  // Only for Tetrimone (4 lines)

    fireworksActive = true;
    fireworksType = 1;
    fireworkParticles.clear();
    fireworksStartTime = std::chrono::high_resolution_clock::now();

    for (int i = 0; i < 5; i++) {
        double x = (rng() % GRID_WIDTH) * BLOCK_SIZE + BLOCK_SIZE / 2;
        double y = (rng() % 4 + GRID_HEIGHT - 8) * BLOCK_SIZE + BLOCK_SIZE / 2;
        int colorIndex = rng() % 7;
        std::array<double, 3> color = TETRIMONEBLOCK_COLOR_THEMES[currentThemeIndex][colorIndex];
        createFireworkBurst(x, y, color, 15 + rng() % 10);
    }

    if (fireworksTimer > 0) {
        g_source_remove(fireworksTimer);
    }

    fireworksTimer = g_timeout_add(16, [](gpointer userData) -> gboolean {
        TetrimoneBoard* board = static_cast<TetrimoneBoard*>(userData);
        board->updateFireworksAnimation();
        return board->isFireworksActive();
    }, this);
}

void TetrimoneBoard::createFireworkBurst(double centerX, double centerY,
                                         const std::array<double, 3>& baseColor,
                                         int particleCount) {
    for (int i = 0; i < particleCount; i++) {
        FireworkParticle particle;
        double angle = (2.0 * M_PI * i) / particleCount + (rng() % 100 - 50) * 0.01;
        double speed = 2.0 + (rng() % 100) * 0.03;

        particle.x = centerX;
        particle.y = centerY;
        particle.vx = cos(angle) * speed;
        particle.vy = sin(angle) * speed;
        particle.life = 1.0;
        particle.maxLife = 1.0 + (rng() % 100) * 0.01;
        particle.size = 3.0 + (rng() % 3);
        particle.gravity = 0.1 + (rng() % 5) * 0.01;
        particle.fade = 0.008 + (rng() % 5) * 0.001;

        particle.color = baseColor;
        for (int c = 0; c < 3; c++) {
            particle.color[c] += (rng() % 40 - 20) * 0.01;
            particle.color[c] = std::max(0.0, std::min(1.0, particle.color[c]));
        }
        fireworkParticles.push_back(particle);
    }
}

void TetrimoneBoard::updateFireworksAnimation() {
    auto now = std::chrono::high_resolution_clock::now();
    auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(now - fireworksStartTime).count();

    if (elapsed > 200 && elapsed < 1500 && (elapsed % 300) < 50) {
        double x = (rng() % GRID_WIDTH) * BLOCK_SIZE + BLOCK_SIZE / 2;
        double y = (rng() % 6 + GRID_HEIGHT - 10) * BLOCK_SIZE + BLOCK_SIZE / 2;
        int colorIndex = rng() % 7;
        std::array<double, 3> color = TETRIMONEBLOCK_COLOR_THEMES[currentThemeIndex][colorIndex];
        createFireworkBurst(x, y, color, 12 + rng() % 8);
    }

    for (auto it = fireworkParticles.begin(); it != fireworkParticles.end();) {
        FireworkParticle& p = *it;
        p.x += p.vx;
        p.y += p.vy;
        p.vy += p.gravity;
        p.life -= p.fade;
        p.vx *= 0.98;
        p.vy *= 0.98;
        if (p.life <= 0.0) {
            it = fireworkParticles.erase(it);
        } else {
            ++it;
        }
    }

    if (app) {
        updateDisplay(app);
    }

    if (elapsed >= FIREWORKS_DURATION || fireworkParticles.empty()) {
        fireworksActive = false;
        fireworkParticles.clear();
        if (fireworksTimer > 0) {
            g_source_remove(fireworksTimer);
            fireworksTimer = 0;
        }
    }
}

// ============================================================================
// Keyboard repeat timers (accelerating, same curve as GTK3/joystick)
// ============================================================================

static bool gameAcceptsInput(TetrimoneApp* app) {
    return !app->board->isPaused() && !app->board->isGameOver() &&
           !app->board->isSplashScreenActive();
}

gboolean onKeyDownTick(gpointer userData) {
    TetrimoneApp* app = static_cast<TetrimoneApp*>(userData);
    if (gameAcceptsInput(app) && keyDownPressed) {
        app->board->movePiece(0, 1);
        keyDownCount++;
        if (keyDownCount > 6) keyDownDelay = 20;
        else if (keyDownCount > 4) keyDownDelay = 30;
        else if (keyDownCount > 2) keyDownDelay = 60;
        keyDownTimer = g_timeout_add(keyDownDelay, onKeyDownTick, app);
        updateDisplay(app);
        updateLabels(app);
        return FALSE;
    }
    keyDownTimer = 0;
    return FALSE;
}

gboolean onKeyLeftTick(gpointer userData) {
    TetrimoneApp* app = static_cast<TetrimoneApp*>(userData);
    if (gameAcceptsInput(app) && keyLeftPressed) {
        app->board->movePiece(-1, 0);
        keyLeftCount++;
        if (keyLeftCount > 6) keyLeftDelay = 30;
        else if (keyLeftCount > 4) keyLeftDelay = 50;
        else if (keyLeftCount > 2) keyLeftDelay = 100;
        keyLeftTimer = g_timeout_add(keyLeftDelay, onKeyLeftTick, app);
        updateDisplay(app);
        updateLabels(app);
        return FALSE;
    }
    keyLeftTimer = 0;
    return FALSE;
}

gboolean onKeyRightTick(gpointer userData) {
    TetrimoneApp* app = static_cast<TetrimoneApp*>(userData);
    if (gameAcceptsInput(app) && keyRightPressed) {
        app->board->movePiece(1, 0);
        keyRightCount++;
        if (keyRightCount > 6) keyRightDelay = 30;
        else if (keyRightCount > 4) keyRightDelay = 50;
        else if (keyRightCount > 2) keyRightDelay = 100;
        keyRightTimer = g_timeout_add(keyRightDelay, onKeyRightTick, app);
        updateDisplay(app);
        updateLabels(app);
        return FALSE;
    }
    keyRightTimer = 0;
    return FALSE;
}

static void stopKeyRepeat() {
    keyDownPressed = keyLeftPressed = keyRightPressed = false;
    if (keyDownTimer > 0) { g_source_remove(keyDownTimer); keyDownTimer = 0; }
    if (keyLeftTimer > 0) { g_source_remove(keyLeftTimer); keyLeftTimer = 0; }
    if (keyRightTimer > 0) { g_source_remove(keyRightTimer); keyRightTimer = 0; }
}

// ============================================================================
// Retro / patriotic mode toggles ('.' and ',')
// ============================================================================

static void applyModeLabels(TetrimoneApp* app, const char* controlsHeader, const char* controlsText) {
    setMarkup(app->difficultyLabel, app->board->getDifficultyText(app->difficulty));
    setMarkup(app->controlsHeaderLabel, controlsHeader);
    if (app->controlsLabel) app->controlsLabel->SetLabelText(U(controlsText));
    updateLabels(app);
    relayoutWindow(app, false);
}

static void toggleRetroMode(TetrimoneApp* app) {
    TetrimoneBoard* board = app->board;
    board->retroModeActive = !board->retroModeActive;
    board->patrioticModeActive = false;

    if (board->retroModeActive) {
        savedThemeIndex = currentThemeIndex;
        ui_set_window_title(app, "БЛОЧНАЯ РЕВОЛЮЦИЯ");
        currentThemeIndex = NUM_COLOR_THEMES - 1;  // Soviet Retro
        applyModeLabels(app, "<b>ПАРТИЙНЫЕ ДИРЕКТИВЫ</b>", CONTROLS_TEXT_RETRO);
        if (board->isUsingBackgroundImage() || board->isUsingBackgroundZip()) {
            board->setUseBackgroundImage(false);
            board->setUseBackgroundZip(false);
            checkItem(app->backgroundToggleMenuItem, false);
        }
        board->playSound(GameSoundEvent::Select);
    } else {
        ui_set_window_title(app, "Tetrimone");
        currentThemeIndex = savedThemeIndex;
        applyModeLabels(app, "<b>Controls</b>", CONTROLS_TEXT);
        if (board->getBackgroundImage() != nullptr) {
            board->setUseBackgroundImage(true);
            if (!board->backgroundZipPath.empty()) {
                board->setUseBackgroundZip(true);
                board->startBackgroundTransition();
            }
            checkItem(app->backgroundToggleMenuItem, true);
        }
        if (!app->backgroundMusicPlaying && board->sound_enabled_) {
            board->resumeBackgroundMusic();
            app->backgroundMusicPlaying = true;
        }
        std::cout << "Retro mode OFF" << std::endl;
    }
    ui_set_active_theme(app, currentThemeIndex);
    updateLabels(app);
    updateDisplay(app);

    // Restart the music so the right playlist is used
    board->pauseBackgroundMusic();
    board->resumeBackgroundMusic();
}

static void togglePatrioticMode(TetrimoneApp* app) {
    TetrimoneBoard* board = app->board;
    board->retroModeActive = false;
    board->patrioticModeActive = !board->patrioticModeActive;

    if (board->patrioticModeActive) {
        savedThemeIndex = currentThemeIndex;
        ui_set_window_title(app, "FREEDOM BLOCKS - GOD BLESS AMERICA");
        currentThemeIndex = NUM_COLOR_THEMES - 2;  // USA
        applyModeLabels(app, "<b>FREEDOM COMMANDS</b>", CONTROLS_TEXT_PATRIOTIC);
        board->playSound(GameSoundEvent::Select);
        std::cout << "Patriotic mode ON - FREEDOM ACTIVATED!" << std::endl;
    } else {
        ui_set_window_title(app, "Tetrimone");
        currentThemeIndex = savedThemeIndex;
        applyModeLabels(app, "<b>Controls</b>", CONTROLS_TEXT);
        if (!app->backgroundMusicPlaying && board->sound_enabled_) {
            board->resumeBackgroundMusic();
            app->backgroundMusicPlaying = true;
        }
        std::cout << "Patriotic mode OFF" << std::endl;
    }

    if (board->getBackgroundImage() != nullptr) {
        board->setUseBackgroundImage(true);
        if (!board->backgroundZipPath.empty()) {
            board->setUseBackgroundZip(true);
        }
        checkItem(app->backgroundToggleMenuItem, true);
    }
    board->startBackgroundTransition();

    ui_set_active_theme(app, currentThemeIndex);
    updateLabels(app);
    updateDisplay(app);

    board->pauseBackgroundMusic();
    board->resumeBackgroundMusic();
}

// ============================================================================
// Keyboard handling
// ============================================================================

// Returns true if the key was consumed
static bool handleKeyDown(TetrimoneApp* app, int key) {
    TetrimoneBoard* board = app->board;
    bool handled = false;

    // Space dismisses the splash screen first
    if (key == WXK_SPACE && board->isSplashScreenActive()) {
        board->dismissSplashScreen();
        updateDisplay(app);
        updateLabels(app);
        return true;
    }

    if (gameAcceptsInput(app)) {
        switch (key) {
            case WXK_LEFT: case WXK_NUMPAD_LEFT: case 'A':
                if (!keyLeftPressed) {
                    keyLeftPressed = true;
                    keyLeftCount = 0;
                    keyLeftDelay = 150;
                    board->movePiece(-1, 0);
                    if (keyLeftTimer == 0) keyLeftTimer = g_timeout_add(keyLeftDelay, onKeyLeftTick, app);
                }
                handled = true;
                break;
            case WXK_RIGHT: case WXK_NUMPAD_RIGHT: case 'D':
                if (!keyRightPressed) {
                    keyRightPressed = true;
                    keyRightCount = 0;
                    keyRightDelay = 150;
                    board->movePiece(1, 0);
                    if (keyRightTimer == 0) keyRightTimer = g_timeout_add(keyRightDelay, onKeyRightTick, app);
                }
                handled = true;
                break;
            case WXK_DOWN: case WXK_NUMPAD_DOWN: case 'S':
                if (!keyDownPressed) {
                    keyDownPressed = true;
                    keyDownCount = 0;
                    keyDownDelay = 150;
                    board->movePiece(0, 1);
                    if (keyDownTimer == 0) keyDownTimer = g_timeout_add(keyDownDelay, onKeyDownTick, app);
                }
                handled = true;
                break;
            case WXK_UP: case WXK_NUMPAD_UP: case 'W':
                board->rotatePiece(true);
                handled = true;
                break;
            case 'Z':
                board->rotatePiece(false);
                handled = true;
                break;
            case WXK_SPACE:
                board->hardDrop();
                handled = true;
                break;
        }
    }

    switch (key) {
        case 'P':
            if (!board->isSplashScreenActive()) onPauseGame(nullptr, app);
            handled = true;
            break;
        case 'M':
            if (board->musicPaused) board->resumeBackgroundMusic();
            else board->pauseBackgroundMusic();
            handled = true;
            break;
        case 'N':
            if (board->isPaused()) onRestartGame(nullptr, app);
            handled = true;
            break;
        case 'Q':
            if (board->isPaused()) onQuitGame(nullptr, app);
            handled = true;
            break;
        case 'R':
            if (board->isGameOver()) onRestartGame(nullptr, app);
            handled = true;
            break;
        case WXK_ESCAPE:
            if (board->isPaused() && !board->isGameOver()) onPauseGame(nullptr, app);
            handled = true;
            break;
        case '.': case WXK_NUMPAD_DECIMAL:
            toggleRetroMode(app);
            handled = true;
            break;
        case ',':
            togglePatrioticMode(app);
            handled = true;
            break;
        case WXK_SPACE:
            handled = true;  // never let Space "click" anything
            break;
    }

    if (handled) {
        updateDisplay(app);
        updateLabels(app);
    }
    return handled;
}

static void handleKeyUp(TetrimoneApp* app, int key) {
    switch (key) {
        case WXK_DOWN: case WXK_NUMPAD_DOWN: case 'S':
            keyDownPressed = false;
            if (keyDownTimer > 0) { g_source_remove(keyDownTimer); keyDownTimer = 0; }
            break;
        case WXK_LEFT: case WXK_NUMPAD_LEFT: case 'A':
            keyLeftPressed = false;
            if (keyLeftTimer > 0) { g_source_remove(keyLeftTimer); keyLeftTimer = 0; }
            break;
        case WXK_RIGHT: case WXK_NUMPAD_RIGHT: case 'D':
            keyRightPressed = false;
            if (keyRightTimer > 0) { g_source_remove(keyRightTimer); keyRightTimer = 0; }
            break;
        default:
            return;
    }
    updateDisplay(app);
}

// ============================================================================
// Game timer
// ============================================================================

static void showTransientPopup(TetrimoneApp* app, const wxString& message, bool warning, guint ms) {
    wxDialog* popup = new wxDialog(app->window, wxID_ANY, wxEmptyString, wxDefaultPosition,
                                   wxDefaultSize, wxCAPTION | wxSTAY_ON_TOP);
    wxBoxSizer* row = new wxBoxSizer(wxHORIZONTAL);
    row->Add(new wxStaticBitmap(popup, wxID_ANY,
                                wxArtProvider::GetBitmap(warning ? wxART_WARNING : wxART_INFORMATION,
                                                         wxART_MESSAGE_BOX)),
             0, wxALIGN_CENTER_VERTICAL | wxALL, popup->FromDIP(12));
    wxStaticText* text = new wxStaticText(popup, wxID_ANY, message);
    wxFont font = text->GetFont();
    font.SetPointSize(font.GetPointSize() + 2);
    font.MakeBold();
    text->SetFont(font);
    row->Add(text, 0, wxALIGN_CENTER_VERTICAL | wxTOP | wxBOTTOM | wxRIGHT, popup->FromDIP(12));
    popup->SetSizerAndFit(row);
    popup->CentreOnParent();
    popup->ShowWithoutActivating();

    runLater(ms, [popup]() { popup->Destroy(); });
}

gboolean onTimerTick(gpointer data) {
    TetrimoneApp* app = static_cast<TetrimoneApp*>(data);
    TetrimoneBoard* board = app->board;

    if (!board->isPaused() && !board->isSplashScreenActive() && !board->retroModeActive) {
        board->coolDown();
    }

    if (!board->isPaused()) {
        board->updateGame();

        if (board->isGameOver() && !board->highScoreAlreadyProcessed) {
            board->highScoreAlreadyProcessed = true;
            bool isHighScore = board->checkAndRecordHighScore(app);
            if (isHighScore) {
                board->playSound(GameSoundEvent::Excellent);
            }
            if (board->retroModeActive) {
                runLater(1500, [app]() { showIdeologicalFailureDialog(app); });
            }
            if (board->patrioticModeActive) {
                runLater(1500, [app]() { showPatrioticPerformanceDialog(app); });
            }
        }
        updateDisplay(app);
        updateLabels(app);
    }

    if (board->retroModeActive) board->setHeatLevel(0.5);

    if (!board->isPaused() && !board->isGameOver() && board->retroModeActive) {
        // 1 in 1000 chance of KGB inspection
        static std::mt19937 rng(std::chrono::system_clock::now().time_since_epoch().count());
        std::uniform_int_distribution<int> dist(1, 1000);
        if (dist(rng) == 1) {
            board->setPaused(true);
            showTransientPopup(app, U("КГБ ИНСПЕКЦИЯ В ПРОЦЕССЕ...\n(KGB INSPECTION IN PROGRESS...)"),
                               true, 2000);
            runLater(2100, [app]() { app->board->setPaused(false); });
        }
    } else if (!board->isPaused() && !board->isGameOver() && board->patrioticModeActive) {
        // 1 in 1776 chance of a Freedom Inspection
        static std::mt19937 rng(std::chrono::system_clock::now().time_since_epoch().count());
        std::uniform_int_distribution<int> dist(1, 1776);
        if (dist(rng) == 1) {
            board->setPaused(true);
            static const char* freedomInspections[] = {
                "🇺🇸 FREEDOM INSPECTION IN PROGRESS! 🦅\n(Checking your liberty levels...)",
                "📺 COMMERCIAL BREAK! 🍔\n(This freedom brought to you by sponsors!)",
                "🏈 TOUCHDOWN! AMERICA SCORES! 🎯\n(Brief patriotic celebration pause!)",
                "☕ COFFEE BREAK TIME! ⏰\n(Even freedom fighters need caffeine!)",
                "📱 SOCIAL MEDIA NOTIFICATION! 💬\n(Someone liked your freedom post!)",
                "🛒 FLASH SALE ALERT! 💳\n(50% off freedom accessories!)",
                "🎬 MOVIE TRAILER PREVIEW! 🍿\n(Coming soon: BLOCKS 2: FREEDOM EDITION!)",
                "🚗 TRAFFIC UPDATE! 🛣️\n(Highway to freedom temporarily slowed!)",
                "🌮 FOOD TRUCK ALERT! 🚚\n(Taco Tuesday freedom fuel available!)",
                "📺 BREAKING NEWS! 📰\n(Local gamer achieves blocks and liberty!)"
            };
            std::uniform_int_distribution<int> msgDist(0, 9);
            showTransientPopup(app, U(freedomInspections[msgDist(rng)]), false, 2500);
            runLater(2600, [app]() { app->board->setPaused(false); });
        }
    }
    return TRUE;
}

// ============================================================================
// Labels and UI state
// ============================================================================

void updateLabels(TetrimoneApp* app) {
    TetrimoneBoard* board = app->board;
    if (!board || !app->scoreLabel) return;

    bool changed = false;
    std::string scoreText = board->retroModeActive
        ? "<b>Партийная Лояльность:</b> " + std::to_string(board->getScore()) + "%"
        : "<b>Score:</b> " + std::to_string(board->getScore());
    changed |= setMarkup(app->scoreLabel, scoreText);

    std::string levelText = board->retroModeActive
        ? "<b>Пятилетка:</b> " + std::to_string(board->getLevel())
        : "<b>Level:</b> " + std::to_string(board->getLevel());
    changed |= setMarkup(app->levelLabel, levelText);

    std::string linesText = board->retroModeActive
        ? "<b>Уничтожено врагов народа:</b> " + std::to_string(board->getLinesCleared())
        : "<b>Lines:</b> " + std::to_string(board->getLinesCleared());
    changed |= setMarkup(app->linesLabel, linesText);

    std::string sequenceText = board->retroModeActive
        ? "<b>Коллективная эффективность:</b> " + std::to_string(board->getConsecutiveClears()) +
              " (Рекорд: " + std::to_string(board->getMaxConsecutiveClears()) + ")"
        : "<b>Sequence:</b> " + std::to_string(board->getConsecutiveClears()) +
              " (Max: " + std::to_string(board->getMaxConsecutiveClears()) + ")";
    if (board->isSequenceActive() && board->getConsecutiveClears() > 1) {
        sequenceText = "<span foreground='#00AA00'>" + sequenceText + "</span>";
    }
    changed |= setMarkup(app->sequenceLabel, sequenceText);

    if (changed) relayoutWindow(app, false);
}

void resetUI(TetrimoneApp* app) {
    updateDisplay(app);
    setMarkup(app->scoreLabel, "<b>Score:</b> 0");
    setMarkup(app->levelLabel, "<b>Level:</b> 1");
    setMarkup(app->linesLabel, "<b>Lines:</b> 0");
    enableItem(app->startMenuItem, false);
    enableItem(app->pauseMenuItem, true);
    ui_set_pause_menu_label(app, "Pause");
}

void updateDisplay(TetrimoneApp* app) {
    if (!app) return;
    if (app->gameArea) app->gameArea->Refresh(false);
    if (app->nextPieceArea) app->nextPieceArea->Refresh(false);
}

void drawBoard(TetrimoneBoard* board) {
    if (board && board->app && board->app->gameArea) board->app->gameArea->Refresh(false);
}

void drawNextPieceArea(TetrimoneBoard* board) {
    if (board && board->app && board->app->nextPieceArea) board->app->nextPieceArea->Refresh(false);
}

namespace {

wxSize smallestGameAreaSize() {
    return wxSize(GRID_WIDTH * SMALLEST_BLOCK_SIZE, GRID_HEIGHT * SMALLEST_BLOCK_SIZE);
}

// Keep the whole window on the screen it's on (title bar and bottom row
// included), letting the board shrink to fit.
void clampToDisplay(wxFrame* frame) {
    int index = wxDisplay::GetFromWindow(frame);
    wxDisplay display(index == wxNOT_FOUND ? 0u : static_cast<unsigned>(index));
    wxRect area = display.GetClientArea();
    wxRect rect = frame->GetRect();
    wxSize size(std::min(rect.width, area.width), std::min(rect.height, area.height));
    if (size != rect.GetSize()) frame->SetSize(size);
    rect = frame->GetRect();
    if (!area.Contains(rect)) {
        int x = std::clamp(rect.x, area.x, std::max(area.x, area.GetRight() - rect.width + 1));
        int y = std::clamp(rect.y, area.y, std::max(area.y, area.GetBottom() - rect.height + 1));
        frame->Move(x, y);
    }
}

void relayoutWindow(TetrimoneApp* app, bool fit) {
    TetrimoneFrame* frame = static_cast<TetrimoneFrame*>(app->window);
    if (!frame || !frame->panel() || !frame->panel()->GetSizer() || !app->gameArea) return;
    wxPanel* panel = frame->panel();
    bool userSized = frame->IsFullScreen() || frame->IsMaximized();

    if (fit && !userSized) {
        // Ask for exactly GRID * BLOCK_SIZE, size the window around it...
        app->gameArea->SetMinSize(gameAreaSize());
        panel->InvalidateBestSize();
        frame->SetMinSize(wxDefaultSize);
        frame->SetClientSize(panel->GetSizer()->GetMinSize());
        clampToDisplay(frame);
    }

    // ...then let the user shrink or stretch it freely from there
    app->gameArea->SetMinSize(smallestGameAreaSize());
    wxSize minClient = panel->GetSizer()->GetMinSize();
    frame->SetMinClientSize(minClient);
    if (!userSized) {
        wxSize client = frame->GetClientSize();
        if (client.x < minClient.x || client.y < minClient.y) {
            frame->SetClientSize(wxSize(std::max(client.x, minClient.x), std::max(client.y, minClient.y)));
        }
    }
    panel->Layout();
}

// The board scales with the window: pick the largest block size that fits
void fitBlockSizeToCanvas(TetrimoneApp* app) {
    if (!app->gameArea) return;
    wxSize size = app->gameArea->GetClientSize();
    if (size.x <= 0 || size.y <= 0) return;
    int blockSize = std::max(SMALLEST_BLOCK_SIZE, std::min(size.x / GRID_WIDTH, size.y / GRID_HEIGHT));
    if (blockSize != BLOCK_SIZE) {
        BLOCK_SIZE = blockSize;
        updateDisplay(app);
    }
}

} // namespace

void ui_set_active_theme(TetrimoneApp* app, int index) {
    if (index >= 0 && index < themeCount()) checkItem(app->themeMenuItems[index], true);
}

void ui_window_fullscreen(TetrimoneApp* app) {
    if (app->window) static_cast<wxFrame*>(app->window)->ShowFullScreen(true, wxFULLSCREEN_NOBORDER | wxFULLSCREEN_NOCAPTION);
    if (app->menuBar) app->menuBar->Check(ID_FULLSCREEN, true);
}

void ui_set_sound_enabled(TetrimoneApp* app, bool enabled) {
    checkItem(app->soundToggleMenuItem, enabled);
}

void ui_set_sound_enabled(TetrimoneApp* app) {
    checkItem(app->soundToggleMenuItem, app->board->sound_enabled_);
}

void ui_set_isusingbackgroundimage_enabled(TetrimoneApp* app) {
    checkItem(app->backgroundToggleMenuItem, app->board->isUsingBackgroundImage());
}

void ui_set_background_enabled(TetrimoneApp* app, bool enabled) {
    checkItem(app->backgroundToggleMenuItem, enabled);
}

void set_difficulty_menu(TetrimoneApp* app, int difficulty) {
    wxMenuItem* items[] = {app->zenMenuItem, app->easyMenuItem, app->mediumMenuItem,
                           app->hardMenuItem, app->extremeMenuItem, app->insaneMenuItem};
    if (difficulty < 0 || difficulty > 5) {
        printf("DEBUG: Invalid difficulty %d\n", difficulty);
        return;
    }
    checkItem(items[difficulty], true);
}

void ui_set_mediumMenuItem_enabled(TetrimoneApp* app, bool enabled) {
    checkItem(app->mediumMenuItem, enabled);
}

void ui_set_window_title(TetrimoneApp* app, const char* title) {
    if (app->window) static_cast<wxFrame*>(app->window)->SetTitle(U(title));
}

void ui_set_difficulty_label(TetrimoneApp* app, const char* markup) {
    setMarkup(app->difficultyLabel, markup);
}

void ui_set_pause_menu_label(TetrimoneApp* app, const char* text) {
    if (app->pauseMenuItem) {
        // Keep the "P" accelerator hint visible in the menu
        app->pauseMenuItem->SetItemLabel(U(text) + "\tP");
    }
}

void ui_update_track_menu(TetrimoneApp* app) {
    for (int i = 0; i < 5; i++) checkItem(app->trackMenuItems[i], app->board->enabledTracks[i]);
}

void app_set_track_items_active(TetrimoneApp* app, int count, bool active) {
    for (int i = 0; i < count && i < 5; i++) checkItem(app->trackMenuItems[i], active);
}

// ============================================================================
// Game flow
// ============================================================================

void adjustDropSpeed(TetrimoneApp* app) {
    int baseSpeed = INITIAL_SPEED - (app->board->getLevel() - 1) * 50;
    switch (app->difficulty) {
        case 0: app->dropSpeed = 1000; break;                 // Zen
        case 1: app->dropSpeed = baseSpeed * 1.5; break;      // Easy
        case 2: app->dropSpeed = baseSpeed; break;            // Medium
        case 3: app->dropSpeed = baseSpeed * 0.7; break;      // Hard
        case 4: app->dropSpeed = baseSpeed * 0.3; break;      // Extreme
        case 5: app->dropSpeed = baseSpeed * 0.1; break;      // Insane
        default: app->dropSpeed = baseSpeed;
    }
    if (app->dropSpeed < 10) app->dropSpeed = 10;
}

void startGame(TetrimoneApp* app) {
    if (app->timerId > 0) {
        g_source_remove(app->timerId);
        app->timerId = 0;
    }

    if (!app->backgroundMusicPlaying && app->board->sound_enabled_) {
        app->board->resumeBackgroundMusic();
        app->backgroundMusicPlaying = true;
    }

    adjustDropSpeed(app);
    if (app->board->junkLinesPercentage > 0) {
        app->board->generateJunkLines(app->board->junkLinesPercentage);
    }
    if (app->board->junkLinesPerLevel > 0) {
        app->board->addJunkLinesFromBottom(app->board->junkLinesPerLevel);
    }

    app->timerId = g_timeout_add(app->dropSpeed, onTimerTick, app);

    enableItem(app->startMenuItem, false);
    enableItem(app->pauseMenuItem, true);
    ui_set_pause_menu_label(app, "Pause");
}

void pauseGame(TetrimoneApp* app) {
    if (app->timerId > 0) {
        g_source_remove(app->timerId);
        app->timerId = 0;
    }
    stopKeyRepeat();

    if (app->backgroundMusicPlaying && app->board->sound_enabled_) {
        app->board->pauseBackgroundMusic();
        app->backgroundMusicPlaying = false;
    }

    enableItem(app->startMenuItem, true);
    ui_set_pause_menu_label(app, "Resume");
}

// Common tail for every "settings changed, restart the game" path
static void restartWithCurrentSettings(TetrimoneApp* app) {
    app->board->restart();
    resetUI(app);
    if (app->board->isPaused()) {
        app->board->togglePause();
        ui_set_pause_menu_label(app, "Pause");
    }
    enableItem(app->startMenuItem, false);
    enableItem(app->pauseMenuItem, true);
    startGame(app);
    updateDisplay(app);
    updateLabels(app);
}

void onStartGame(GtkMenuItem*, gpointer userData) {
    TetrimoneApp* app = static_cast<TetrimoneApp*>(userData);
    if (app->board->isGameOver()) {
        app->board->restart();
        resetUI(app);
    }
    app->board->setPaused(false);
    startGame(app);
    enableItem(app->startMenuItem, false);
    enableItem(app->pauseMenuItem, true);
    updateDisplay(app);
    updateLabels(app);
}

void onPauseGame(GtkMenuItem*, gpointer userData) {
    TetrimoneApp* app = static_cast<TetrimoneApp*>(userData);
    if (app->board->isGameOver()) return;

    app->board->togglePause();
    if (app->board->isPaused()) {
        pauseGame(app);
        ui_set_pause_menu_label(app, "Resume");
        enableItem(app->startMenuItem, true);
    } else {
        startGame(app);
        ui_set_pause_menu_label(app, "Pause");
        enableItem(app->startMenuItem, false);
    }
    updateDisplay(app);
}

void onRestartGame(GtkMenuItem*, gpointer userData) {
    restartWithCurrentSettings(static_cast<TetrimoneApp*>(userData));
}

void onQuitGame(GtkMenuItem*, gpointer userData) {
    TetrimoneApp* app = static_cast<TetrimoneApp*>(userData);
    if (app->window) app->window->Close();
}

// Pause while a dialog is up, resume afterwards (only if we paused it)
class ScopedDialogPause {
public:
    explicit ScopedDialogPause(TetrimoneApp* app) : app_(app) {
        if (!app_->board->isPaused() && !app_->board->isGameOver() &&
            !app_->board->isSplashScreenActive()) {
            onPauseGame(nullptr, app_);
            paused_ = true;
        }
    }
    ~ScopedDialogPause() {
        if (paused_ && app_->board->isPaused() && !app_->board->isGameOver()) {
            onPauseGame(nullptr, app_);
        }
    }
private:
    TetrimoneApp* app_;
    bool paused_ = false;
};

void calculateBlockSize(TetrimoneApp* app) {
    int index = wxNOT_FOUND;
    if (app && app->window) index = wxDisplay::GetFromWindow(app->window);
    wxDisplay display(index == wxNOT_FOUND ? 0u : static_cast<unsigned>(index));
    wxRect workarea = display.GetClientArea();

    // Title bar, menu bar and margins; generous because some window managers
    // report the whole screen as the work area (the window is resizable anyway)
    int availableHeight = workarea.height - 150;
    int availableWidth = workarea.width - 300;    // side panel
    int heightBasedSize = availableHeight / GRID_HEIGHT;
    int widthBasedSize = availableWidth / GRID_WIDTH;

    BLOCK_SIZE = std::min(heightBasedSize, widthBasedSize);
    BLOCK_SIZE = std::max(BLOCK_SIZE, MIN_BLOCK_SIZE);
    BLOCK_SIZE = std::min(BLOCK_SIZE, MAX_BLOCK_SIZE);
}

void rebuildGameUI(TetrimoneApp* app) {
    TetrimoneFrame* frame = static_cast<TetrimoneFrame*>(app->window);
    if (frame) frame->rebuildContent();
}

void cleanupApp(TetrimoneApp* app) {
    if (!app) return;
    if (app->timerId > 0) {
        g_source_remove(app->timerId);
        app->timerId = 0;
    }
    if (app->joystickTimerId > 0) {
        g_source_remove(app->joystickTimerId);
        app->joystickTimerId = 0;
    }
    if (app->joystick) {
        SDL_JoystickClose(app->joystick);
        app->joystick = nullptr;
    }
    if (app->joystickEnabled) {
        SDL_Quit();
        app->joystickEnabled = false;
    }
    delete app->board;
    app->board = nullptr;
    delete app;
}

// ============================================================================
// Menu handlers (wx counterparts of the GTK3 callbacks)
// ============================================================================

static void onDifficultySelected(TetrimoneApp* app, int newDifficulty) {
    int previousDifficulty = app->difficulty;

    if (newDifficulty != previousDifficulty) {
        if (!app->board->isSplashScreenActive() && !app->board->isGameOver()) {
            if (!askYesNo(app->window, "Changing difficulty will start a new game. Continue?")) {
                set_difficulty_menu(app, previousDifficulty);
                return;
            }
        }
        app->difficulty = newDifficulty;
        setMarkup(app->difficultyLabel, app->board->getDifficultyText(app->difficulty));
        adjustDropSpeed(app);
        restartWithCurrentSettings(app);
    } else if (!app->board->isPaused() && !app->board->isGameOver() && app->timerId > 0) {
        g_source_remove(app->timerId);
        app->timerId = g_timeout_add(app->dropSpeed, onTimerTick, app);
    }
}

static void onMinBlockSizeSelected(TetrimoneApp* app, int index) {
    int newMinBlockSize = index + 1;  // item order matches the GTK3 menu
    int currentMinBlockSize = app->board->getMinBlockSize();
    if (newMinBlockSize == currentMinBlockSize) return;

    if (!askYesNo(app->window, "Changing the minimum block size will restart the game. Continue?")) {
        if (currentMinBlockSize >= 1 && currentMinBlockSize <= 4) {
            checkItem(app->blockSizeRuleMenuItems[currentMinBlockSize - 1], true);
        }
        return;
    }
    app->board->setMinBlockSize(newMinBlockSize);
    restartWithCurrentSettings(app);
}

static void onThemeSelected(TetrimoneApp* app, int newThemeIndex) {
    if (newThemeIndex == currentThemeIndex) return;
    app->board->startThemeTransition(newThemeIndex);
    if (app->board->isUsingBackgroundZip() && !app->board->backgroundImages.empty()) {
        app->board->startBackgroundTransition();
    }
    updateDisplay(app);
}

static void onTrackToggled(TetrimoneApp* app, int trackIndex, bool enabled) {
    app->board->enabledTracks[trackIndex] = enabled;
    bool anyEnabled = false;
    for (int i = 0; i < 5; i++) anyEnabled = anyEnabled || app->board->enabledTracks[i];
    if (!anyEnabled) {
        app->board->enabledTracks[trackIndex] = true;
        checkItem(app->trackMenuItems[trackIndex], true);
    }
}

static void onSoundToggled(TetrimoneApp* app, bool enabled) {
    app->board->sound_enabled_ = enabled;
    if (enabled) {
        if (!app->board->initializeAudio()) {
            checkItem(app->soundToggleMenuItem, false);
            app->board->sound_enabled_ = false;
            return;
        }
        if (!app->board->isPaused() && !app->board->isGameOver()) {
            app->board->resumeBackgroundMusic();
            app->backgroundMusicPlaying = true;
        }
    } else {
        app->board->pauseBackgroundMusic();
        app->backgroundMusicPlaying = false;
        app->board->cleanupAudio();
    }
}

static void onRetroMusicToggled(TetrimoneApp* app, bool enabled) {
    app->board->retroMusicActive = enabled;
    // Stop the current track and start the other playlist right away.
    // (pause + play left the music paused and muted, so nothing played.)
    // While the game is paused the new choice is picked up on resume.
    if (app->board->sound_enabled_ && !app->board->isPaused()) {
        app->board->pauseBackgroundMusic();
        app->board->resumeBackgroundMusic();
        app->backgroundMusicPlaying = true;
    }
}

static void onBackgroundToggled(TetrimoneApp* app, bool useBackground) {
    app->board->setUseBackgroundImage(useBackground);
    if (app->board->isUsingBackgroundZip()) {
        app->board->setUseBackgroundZip(useBackground);
    }
    updateDisplay(app);
}

static void onBackgroundOpacityDialog(TetrimoneApp* app) {
    OpacitySliderConfig config{
        .title = "Background Opacity",
        .minValue = 0.0,
        .maxValue = 1.0,
        .stepValue = 0.05,
        .currentValue = app->board->getBackgroundOpacity(),
        .width = 300,
        .height = 150
    };
    createOpacitySliderDialog(app->window, config, [app](double opacity) {
        if (!app->board->isUsingBackgroundImage() || app->board->getBackgroundImage() == nullptr) {
            return;
        }
        cairo_status_t status = cairo_surface_status(
            static_cast<cairo_surface_t*>(const_cast<void*>(app->board->getBackgroundImage())));
        if (status != CAIRO_STATUS_SUCCESS) {
            std::cerr << "Invalid background image surface during opacity change: "
                      << cairo_status_to_string(status) << std::endl;
            return;
        }
        app->board->setBackgroundOpacity(opacity);
        updateDisplay(app);
    });
}

static void onBackgroundImageDialog(TetrimoneApp* app) {
    ScopedDialogPause pause(app);

    wxFileDialog dialog(app->window, "Select Background Images", wxEmptyString, wxEmptyString,
                        "All Image Files (*.png;*.jpg;*.jpeg)|*.png;*.jpg;*.jpeg|"
                        "PNG Images (*.png)|*.png|"
                        "JPEG Images (*.jpg;*.jpeg)|*.jpg;*.jpeg|"
                        "All Files (*.*)|*.*",
                        wxFD_OPEN | wxFD_FILE_MUST_EXIST | wxFD_MULTIPLE);
    if (dialog.ShowModal() != wxID_OK) {
        updateDisplay(app);
        return;
    }

    wxArrayString paths;
    dialog.GetPaths(paths);
    if (paths.empty()) return;

    app->board->cleanupBackgroundImages();
    bool imagesLoaded = false;
    for (const wxString& path : paths) {
        cairo_surface_t* surface = loadImageSurface(path);
        if (surface && cairo_surface_status(surface) == CAIRO_STATUS_SUCCESS) {
            app->board->backgroundImages.push_back(surface);
            imagesLoaded = true;
        } else {
            std::cerr << "Failed to load image: " << path.utf8_str() << std::endl;
            if (surface) cairo_surface_destroy(surface);
        }
    }

    if (imagesLoaded) {
        app->board->useBackgroundZip = true;
        app->board->useBackgroundImage = true;
        app->board->selectRandomBackground();
        checkItem(app->backgroundToggleMenuItem, true);
        updateDisplay(app);
        onBackgroundOpacityDialog(app);
    } else {
        showError(app->window, "No valid image files could be loaded.");
    }
    updateDisplay(app);
}

static void onBackgroundZipDialog(TetrimoneApp* app) {
    ScopedDialogPause pause(app);

    WXFileDialog fileDialog(app->window);
    std::string filePath = fileDialog.openFile("Select Background Images ZIP File", "*.zip", "ZIP Files");
    if (!filePath.empty()) {
        if (app->board->loadBackgroundImagesFromZip(filePath)) {
            checkItem(app->backgroundToggleMenuItem, true);
            updateDisplay(app);
            onBackgroundOpacityDialog(app);
        } else {
            fileDialog.showError("Error Loading Background",
                                 "Failed to load background images from ZIP: " + filePath);
        }
    }
    updateDisplay(app);
}

// Volume dialog (volume.cpp's GTK version keeps backups because AudioManager
// can report 0 before it's initialised; keep the same safeguard here)
static float s_lastSfxVolume = 0.50f;
static float s_lastMusicVolume = 0.50f;

static void onVolumeDialog(TetrimoneApp* app) {
    bool isRetroMode = app->board->retroModeActive;

    float currentVolume = AudioManager::getInstance().getVolume();
    if (currentVolume > 0.0f) s_lastSfxVolume = currentVolume;
    else if (s_lastSfxVolume > 0.0f) {
        currentVolume = s_lastSfxVolume;
        AudioManager::getInstance().setVolume(currentVolume);
    }

    float currentMusicVolume = AudioManager::getInstance().getMusicVolume();
    if (currentMusicVolume > 0.0f) s_lastMusicVolume = currentMusicVolume;
    else if (s_lastMusicVolume > 0.0f) {
        currentMusicVolume = s_lastMusicVolume;
        AudioManager::getInstance().setMusicVolume(currentMusicVolume);
    }

    VolumeControlConfig config{
        .title = isRetroMode ? "ЦЕНТРАЛЬНЫЙ КОНТРОЛЬ ЗВУКА" : "Volume Control",
        .okButtonLabel = isRetroMode ? "_ПРИНЯТО" : "_OK",
        .isRetroMode = isRetroMode,
        .sfxVolume = static_cast<int>(currentVolume * 100.0f + 0.5f),
        .musicVolume = static_cast<int>(currentMusicVolume * 100.0f + 0.5f),
        .width = 350,
        .height = 250
    };

    createVolumeControlDialog(
        app->window, config,
        [](int percent) {
            s_lastSfxVolume = percent / 100.0f;
            AudioManager::getInstance().setVolume(percent / 100.0f);
        },
        [](int percent) {
            s_lastMusicVolume = percent / 100.0f;
            AudioManager::getInstance().setMusicVolume(percent / 100.0f);
        });
}

static void onBlockSizeDialog(TetrimoneApp* app) {
    ScopedDialogPause pause(app);

    std::vector<SliderSpec> sliders{
        {"", static_cast<double>(MIN_BLOCK_SIZE), static_cast<double>(MAX_BLOCK_SIZE), 1.0,
         static_cast<double>(BLOCK_SIZE), "Current size: %.0f", "Small", "Large"}
    };
    if (createSliderSettingsDialog(app->window, "Block Size", "Adjust block size:", sliders,
                                   "Click Apply to set the new block size.\nThis will reset the game UI.",
                                   300, 150)) {
        BLOCK_SIZE = static_cast<int>(sliders[0].value + 0.5);
        rebuildGameUI(app);
    }
}

static void onGameSizeDialog(TetrimoneApp* app) {
    std::vector<SliderSpec> sliders{
        {"Width", static_cast<double>(MIN_GRID_WIDTH), static_cast<double>(MAX_GRID_WIDTH), 1.0,
         static_cast<double>(GRID_WIDTH), "Width: %.0f", "", ""},
        {"Height", static_cast<double>(MIN_GRID_HEIGHT), static_cast<double>(MAX_GRID_HEIGHT), 1.0,
         static_cast<double>(GRID_HEIGHT), "Height: %.0f", "", ""}
    };
    if (!createSliderSettingsDialog(app->window, "Game Size Settings", "", sliders,
                                    "Note: Changing game size will restart the current game.",
                                    300, 200)) {
        return;
    }
    int newWidth = static_cast<int>(sliders[0].value + 0.5);
    int newHeight = static_cast<int>(sliders[1].value + 0.5);
    if (newWidth == GRID_WIDTH && newHeight == GRID_HEIGHT) return;

    if (askYesNo(app->window, "Changing the game size will restart the current game. Continue?")) {
        GRID_WIDTH = newWidth;
        GRID_HEIGHT = newHeight;
        calculateBlockSize(app);
        rebuildGameUI(app);
        restartWithCurrentSettings(app);
    }
}

static void onBlockTrailsConfig(TetrimoneApp* app) {
    std::vector<SliderSpec> sliders{
        {"Trail Opacity", 0.1, 1.0, 0.05, app->board->getTrailOpacity(), "Opacity: %.2f", "", ""},
        {"Trail Duration", 0.05, 1.0, 0.05, app->board->getTrailDuration(), "Duration: %.2f seconds", "", ""}
    };
    if (createSliderSettingsDialog(app->window, "Block Trails Settings", "", sliders, "", 300, 200)) {
        app->board->setTrailOpacity(sliders[0].value);
        app->board->setTrailDuration(sliders[1].value);
        updateDisplay(app);
    }
}

static void onGameSetupDialog(TetrimoneApp* app) {
    GameSetupConfig config{
        .title = "Game Setup",
        .junkPercentage = app->board->junkLinesPercentage,
        .junkPerLevel = app->board->junkLinesPerLevel,
        .initialLevel = app->board->initialLevel,
        .width = 500,
        .height = 450
    };
    createGameSetupDialog(app->window, config, [app](int junkPercentage, int junkPerLevel, int initialLevel) {
        bool changed = junkPercentage != app->board->junkLinesPercentage ||
                       junkPerLevel != app->board->junkLinesPerLevel ||
                       initialLevel != app->board->initialLevel;
        if (!changed) return;
        if (!askYesNo(app->window, "Changing game settings will restart the current game. Continue?")) {
            return;
        }
        app->board->junkLinesPercentage = junkPercentage;
        app->board->junkLinesPerLevel = junkPerLevel;
        app->board->initialLevel = initialLevel;
        restartWithCurrentSettings(app);
    });
}

// ============================================================================
// Joystick
// ============================================================================

namespace {

struct DirectionalControl {
    bool active;
    int direction;
    Uint32 lastMoveTime;
    Uint32 repeatDelay;
    int moveCount;
};

const Uint32 AXIS_REPEAT_DELAY_MS = 150;

void onJoystickPause(TetrimoneApp* app, bool shouldPause) {
    if (app->board->isSplashScreenActive()) {
        app->board->dismissSplashScreen();
        startGame(app);
        updateDisplay(app);
        updateLabels(app);
    } else if (app->board->isGameOver()) {
        onRestartGame(nullptr, app);
    } else if (shouldPause) {
        onPauseGame(nullptr, app);
    }
}

void onJoystickRotate(TetrimoneApp* app, bool clockwise) {
    app->board->rotatePiece(clockwise);
    updateDisplay(app);
}

void onJoystickHardDrop(TetrimoneApp* app) {
    app->board->hardDrop();
    updateDisplay(app);
    updateLabels(app);
}

void startJoystickPolling(TetrimoneApp* app) {
    if (app->joystickTimerId > 0) {
        g_source_remove(app->joystickTimerId);
        app->joystickTimerId = 0;
    }
    if (app->joystickEnabled && app->joystick) {
        loadJoystickMapping(app);
        app->joystickTimerId = g_timeout_add(16, pollJoystick, app);
    }
}

} // namespace

gboolean pollJoystick(gpointer data) {
    TetrimoneApp* app = static_cast<TetrimoneApp*>(data);
    if (!app || !app->joystickEnabled || !app->joystick || !app->board) {
        if (app) app->joystickTimerId = 0;
        return FALSE;
    }

    SDL_JoystickUpdate();
    processJoystickButtons(app, onJoystickPause, onJoystickRotate, onJoystickHardDrop);

    if (app->board->isGameOver() || app->board->isPaused()) return TRUE;

    int moveX = 0, moveY = 0;
    getJoystickAnalogMovement(app, &moveX, &moveY);

    static DirectionalControl horizontal = {false, 0, 0, AXIS_REPEAT_DELAY_MS, 0};
    static DirectionalControl vertical = {false, 0, 0, AXIS_REPEAT_DELAY_MS, 0};
    Uint32 now = SDL_GetTicks();

    if (moveX != 0) {
        if (!horizontal.active || horizontal.direction != moveX) {
            horizontal = {true, moveX, now, AXIS_REPEAT_DELAY_MS, 0};
            app->board->movePiece(moveX, 0);
            updateDisplay(app);
            updateLabels(app);
        } else if (now - horizontal.lastMoveTime > horizontal.repeatDelay) {
            horizontal.moveCount++;
            int moves = 1 + std::min(5, horizontal.moveCount / 10);
            for (int i = 0; i < moves; i++) app->board->movePiece(moveX, 0);
            horizontal.lastMoveTime = now;
            updateDisplay(app);
            updateLabels(app);
        }
    } else {
        horizontal.active = false;
        horizontal.direction = 0;
    }

    if (moveY != 0) {
        if (!vertical.active || vertical.direction != moveY) {
            vertical = {true, moveY, now, AXIS_REPEAT_DELAY_MS, 0};
            if (moveY > 0) app->board->movePiece(0, 1);
            updateDisplay(app);
            updateLabels(app);
        } else if (now - vertical.lastMoveTime > vertical.repeatDelay) {
            vertical.moveCount++;
            int drops = 1 + std::min(5, vertical.moveCount / 10);
            if (moveY > 0) {
                for (int i = 0; i < drops; i++) app->board->movePiece(0, 1);
            }
            vertical.lastMoveTime = now;
            updateDisplay(app);
            updateLabels(app);
        }
    } else {
        vertical.active = false;
        vertical.direction = 0;
    }
    return TRUE;
}

static void onJoystickConfig(TetrimoneApp* app) {
    if (!app->joystick) {
        // A controller may have been plugged in since start-up
        initSDL(app);
        startJoystickPolling(app);
    }
    if (!app->joystick) {
        showError(app->window, "No joystick connected!");
        return;
    }

    ScopedDialogPause pause(app);

    JoystickMappingConfig config{
        .title = "Joystick Configuration - Mapping",
        .numButtons = std::min(16, SDL_JoystickNumButtons(app->joystick)),
        .numAxes = std::min(6, SDL_JoystickNumAxes(app->joystick)),
        .rotate_cw = app->joystickMapping.rotate_cw_button,
        .rotate_ccw = app->joystickMapping.rotate_ccw_button,
        .hard_drop = app->joystickMapping.hard_drop_button,
        .pause_button = app->joystickMapping.pause_button,
        .x_axis = app->joystickMapping.x_axis,
        .y_axis = app->joystickMapping.y_axis,
        .invert_x = app->joystickMapping.invert_x,
        .invert_y = app->joystickMapping.invert_y,
        .width = 500,
        .height = 400
    };

    createJoystickMappingDialog(app->window, config,
        [app](int rotate_cw, int rotate_ccw, int hard_drop, int pause_btn,
              int x_axis, int y_axis, bool invert_x, bool invert_y) {
            app->joystickMapping.rotate_cw_button = rotate_cw;
            app->joystickMapping.rotate_ccw_button = rotate_ccw;
            app->joystickMapping.hard_drop_button = hard_drop;
            app->joystickMapping.pause_button = pause_btn;
            app->joystickMapping.x_axis = x_axis;
            app->joystickMapping.y_axis = y_axis;
            app->joystickMapping.invert_x = invert_x;
            app->joystickMapping.invert_y = invert_y;
            saveJoystickMapping(app);
        });
}

// ============================================================================
// TetrimoneFrame
// ============================================================================

TetrimoneFrame::TetrimoneFrame(TetrimoneApp* app)
    : wxFrame(nullptr, wxID_ANY, "Tetrimone", wxDefaultPosition, wxDefaultSize,
              wxDEFAULT_FRAME_STYLE),
      app_(app) {
    app_->window = this;
    setWindowIcon(this);

    Bind(wxEVT_CLOSE_WINDOW, &TetrimoneFrame::onClose, this);
    Bind(wxEVT_ACTIVATE, &TetrimoneFrame::onActivate, this);
    Bind(wxEVT_CHAR_HOOK, &TetrimoneFrame::onCharHook, this);
    Bind(wxEVT_MENU_OPEN, &TetrimoneFrame::onMenuOpen, this);
    Bind(wxEVT_MENU_CLOSE, &TetrimoneFrame::onMenuClose, this);
}

void TetrimoneFrame::buildMenu() {
    TetrimoneApp* app = app_;
    wxMenuBar* menuBar = new wxMenuBar();

    // *** GAME ***
    wxMenu* gameMenu = new wxMenu();
    app->startMenuItem = gameMenu->Append(ID_START, "Start");
    app->pauseMenuItem = gameMenu->Append(ID_PAUSE, "Pause\tP");
    app->restartMenuItem = gameMenu->Append(ID_RESTART, "Restart");
    gameMenu->Append(ID_HIGH_SCORES, "High Scores");
    gameMenu->AppendSeparator();
    gameMenu->Append(wxID_EXIT, "Quit");

    // *** DIFFICULTY ***
    wxMenu* difficultyMenu = new wxMenu();
    const char* difficultyNames[] = {"Zen", "Easy", "Medium", "Hard", "Extreme", "Insane"};
    wxMenuItem** difficultyItems[] = {&app->zenMenuItem, &app->easyMenuItem, &app->mediumMenuItem,
                                      &app->hardMenuItem, &app->extremeMenuItem, &app->insaneMenuItem};
    for (int i = 0; i < 6; i++) {
        *difficultyItems[i] = difficultyMenu->AppendRadioItem(ID_DIFFICULTY_FIRST + i, difficultyNames[i]);
    }
    checkItem(app->mediumMenuItem, true);
    app->difficulty = 2;

    // *** GRAPHICS ***
    wxMenu* graphicsMenu = new wxMenu();
    graphicsMenu->Append(ID_BLOCK_SIZE, "Block Size...");

    wxMenu* backgroundMenu = new wxMenu();
    backgroundMenu->Append(ID_BG_IMAGE, "Set Background Image ...");
    backgroundMenu->Append(ID_BG_ZIP, "Set Background Images from ZIP...");
    backgroundMenu->Append(ID_BG_OPACITY, "Background Opacity...");
    app->backgroundToggleMenuItem = backgroundMenu->AppendCheckItem(ID_BG_TOGGLE, "Enable Background Image");
    checkItem(app->backgroundToggleMenuItem, true);
    graphicsMenu->AppendSubMenu(backgroundMenu, "Background");

    app->gridLinesMenuItem = graphicsMenu->AppendCheckItem(ID_GRID_LINES, "Show Grid Lines");
    checkItem(app->gridLinesMenuItem, app->board->isShowingGridLines());
    app->blockTrailsMenuItem = graphicsMenu->AppendCheckItem(ID_BLOCK_TRAILS, "Block Trails");
    checkItem(app->blockTrailsMenuItem, app->board->isTrailsEnabled());
    graphicsMenu->Append(ID_BLOCK_TRAILS_CONFIG, "Block Trails Settings...");

    wxMenu* themeMenu = new wxMenu();
    for (int i = 0; i < themeCount(); i++) {
        app->themeMenuItems[i] = themeMenu->AppendRadioItem(ID_THEME_FIRST + i, THEME_NAMES[i]);
    }
    ui_set_active_theme(app, currentThemeIndex);
    graphicsMenu->AppendSubMenu(themeMenu, "Color Themes");

    app->ghostPieceMenuItem = graphicsMenu->AppendCheckItem(ID_GHOST_PIECE, "Show Ghost Piece");
    checkItem(app->ghostPieceMenuItem, app->board->isGhostPieceEnabled());
    app->simpleBlocksMenuItem = graphicsMenu->AppendCheckItem(ID_SIMPLE_BLOCKS, "Simple Blocks (No 3D Effect)");
    checkItem(app->simpleBlocksMenuItem, app->board->simpleBlocksActive);
    graphicsMenu->AppendSeparator();
    graphicsMenu->AppendCheckItem(ID_FULLSCREEN, "Full Screen\tF11");

    // *** SOUND ***
    wxMenu* soundMenu = new wxMenu();
    app->soundToggleMenuItem = soundMenu->AppendCheckItem(ID_SOUND_TOGGLE, "Enable Sound");
    checkItem(app->soundToggleMenuItem, true);
    soundMenu->Append(ID_VOLUME, "Volume Settings...");
    wxMenu* musicMenu = new wxMenu();
    for (int i = 0; i < 5; i++) {
        app->trackMenuItems[i] = musicMenu->AppendCheckItem(ID_TRACK_FIRST + i, wxString::Format("Track %d", i + 1));
        checkItem(app->trackMenuItems[i], true);
    }
    soundMenu->AppendSubMenu(musicMenu, "Music Tracks");
    app->retroMusicMenuItem = soundMenu->AppendCheckItem(ID_RETRO_MUSIC, "Use Retro Music");
    checkItem(app->retroMusicMenuItem, app->board->retroMusicActive);

    // *** CONTROLS ***
    wxMenu* controlsMenu = new wxMenu();
    controlsMenu->Append(ID_JOYSTICK_CONFIG, "Configure Joystick...");

    // *** RULES ***
    wxMenu* rulesMenu = new wxMenu();
    wxMenu* minBlockMenu = new wxMenu();
    const char* minBlockLabels[] = {
        "4:  Single, Double, Triple, and Quadruple Blocks",
        "3:  No Single Blocks",
        "2:   No Single or Double Blocks",
        "1:  No Single, Double, or Triple Blocks; only Quadruple Blocks"
    };
    for (int i = 0; i < 4; i++) {
        app->blockSizeRuleMenuItems[i] = minBlockMenu->AppendRadioItem(ID_MIN_BLOCK_FIRST + i, minBlockLabels[i]);
    }
    int minBlock = app->board->getMinBlockSize();
    if (minBlock >= 1 && minBlock <= 4) checkItem(app->blockSizeRuleMenuItems[minBlock - 1], true);
    rulesMenu->AppendSubMenu(minBlockMenu, "Minimum Block Size");
    rulesMenu->Append(ID_GAME_SIZE, "Game Size");
    rulesMenu->Append(ID_GAME_SETUP, "Game Setup");

    // *** HELP ***
    wxMenu* helpMenu = new wxMenu();
    helpMenu->Append(ID_INSTRUCTIONS, "Instructions");
    helpMenu->Append(wxID_ABOUT, "About");

    menuBar->Append(gameMenu, "&Game");
    menuBar->Append(difficultyMenu, "&Difficulty");
    menuBar->Append(graphicsMenu, "G&raphics");
    menuBar->Append(soundMenu, "&Sound");
    menuBar->Append(controlsMenu, "&Controls");
    menuBar->Append(rulesMenu, "R&ules");
    menuBar->Append(helpMenu, "&Help");
    SetMenuBar(menuBar);
    app->menuBar = menuBar;

    // --- handlers ---
    auto on = [this](int id, std::function<void(wxCommandEvent&)> fn) {
        Bind(wxEVT_MENU, [fn](wxCommandEvent& e) { fn(e); }, id);
    };

    on(ID_START, [app](wxCommandEvent&) { onStartGame(nullptr, app); });
    on(ID_PAUSE, [app](wxCommandEvent&) { onPauseGame(nullptr, app); });
    on(ID_RESTART, [app](wxCommandEvent&) { onRestartGame(nullptr, app); });
    on(ID_HIGH_SCORES, [app](wxCommandEvent&) { onViewHighScores(nullptr, app); });
    on(wxID_EXIT, [app](wxCommandEvent&) { onQuitGame(nullptr, app); });

    Bind(wxEVT_MENU, [app](wxCommandEvent& e) {
        onDifficultySelected(app, e.GetId() - ID_DIFFICULTY_FIRST);
    }, ID_DIFFICULTY_FIRST, ID_DIFFICULTY_LAST);

    on(ID_BLOCK_SIZE, [app](wxCommandEvent&) { onBlockSizeDialog(app); });
    on(ID_BG_IMAGE, [app](wxCommandEvent&) { onBackgroundImageDialog(app); });
    on(ID_BG_ZIP, [app](wxCommandEvent&) { onBackgroundZipDialog(app); });
    on(ID_BG_OPACITY, [app](wxCommandEvent&) { onBackgroundOpacityDialog(app); });
    on(ID_BG_TOGGLE, [app](wxCommandEvent& e) { onBackgroundToggled(app, e.IsChecked()); });
    on(ID_GRID_LINES, [app](wxCommandEvent& e) {
        app->board->setShowGridLines(e.IsChecked());
        updateDisplay(app);
    });
    on(ID_BLOCK_TRAILS, [app](wxCommandEvent& e) {
        app->board->setTrailsEnabled(e.IsChecked());
        updateDisplay(app);
    });
    on(ID_BLOCK_TRAILS_CONFIG, [app](wxCommandEvent&) { onBlockTrailsConfig(app); });
    on(ID_GHOST_PIECE, [app](wxCommandEvent& e) {
        app->board->setGhostPieceEnabled(e.IsChecked());
        updateDisplay(app);
    });
    on(ID_FULLSCREEN, [this](wxCommandEvent& e) {
        ShowFullScreen(e.IsChecked(), wxFULLSCREEN_NOBORDER | wxFULLSCREEN_NOCAPTION);
    });
    on(ID_SIMPLE_BLOCKS, [app](wxCommandEvent& e) {
        app->board->simpleBlocksActive = e.IsChecked();
        updateDisplay(app);
    });
    Bind(wxEVT_MENU, [app](wxCommandEvent& e) {
        onThemeSelected(app, e.GetId() - ID_THEME_FIRST);
    }, ID_THEME_FIRST, ID_THEME_LAST);

    on(ID_SOUND_TOGGLE, [app](wxCommandEvent& e) { onSoundToggled(app, e.IsChecked()); });
    on(ID_VOLUME, [app](wxCommandEvent&) { onVolumeDialog(app); });
    on(ID_RETRO_MUSIC, [app](wxCommandEvent& e) { onRetroMusicToggled(app, e.IsChecked()); });
    Bind(wxEVT_MENU, [app](wxCommandEvent& e) {
        onTrackToggled(app, e.GetId() - ID_TRACK_FIRST, e.IsChecked());
    }, ID_TRACK_FIRST, ID_TRACK_LAST);

    on(ID_JOYSTICK_CONFIG, [app](wxCommandEvent&) { onJoystickConfig(app); });

    Bind(wxEVT_MENU, [app](wxCommandEvent& e) {
        onMinBlockSizeSelected(app, e.GetId() - ID_MIN_BLOCK_FIRST);
    }, ID_MIN_BLOCK_FIRST, ID_MIN_BLOCK_LAST);
    on(ID_GAME_SIZE, [app](wxCommandEvent&) { onGameSizeDialog(app); });
    on(ID_GAME_SETUP, [app](wxCommandEvent&) { onGameSetupDialog(app); });

    on(ID_INSTRUCTIONS, [app](wxCommandEvent&) { onInstructionsDialog(nullptr, app); });
    on(wxID_ABOUT, [app](wxCommandEvent&) { onAboutDialog(nullptr, app); });

    // Bound last so it runs first: settle the menu-open pause before any
    // command handler looks at the paused state.
    Bind(wxEVT_MENU, &TetrimoneFrame::onAnyMenuCommand, this);
}

void TetrimoneFrame::buildContent() {
    TetrimoneApp* app = app_;
    panel_ = new wxPanel(this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxWANTS_CHARS);

    wxBoxSizer* mainBox = new wxBoxSizer(wxHORIZONTAL);

    // The board is drawn at BLOCK_SIZE, centred in whatever space the window
    // gives it; BLOCK_SIZE follows the window size (fitBlockSizeToCanvas).
    app->gameArea = new CairoCanvas(panel_, gameAreaSize(), [app](cairo_t* cr, int w, int h) {
        if (!app->board) return;
        int boardW = GRID_WIDTH * BLOCK_SIZE;
        int boardH = GRID_HEIGHT * BLOCK_SIZE;
        cairo_save(cr);
        cairo_translate(cr, std::max(0, (w - boardW) / 2), std::max(0, (h - boardH) / 2));
        cairo_rectangle(cr, 0, 0, boardW, boardH);
        cairo_clip(cr);
        OnDrawGameAreaCairo(cr, app, boardW, boardH);
        cairo_restore(cr);
    });
    mainBox->Add(app->gameArea, 1, wxEXPAND | wxALL, FromDIP(10));
    app->gameArea->Bind(wxEVT_SIZE, [app](wxSizeEvent& e) {
        e.Skip();
        fitBlockSizeToCanvas(app);
    });

    wxBoxSizer* side = new wxBoxSizer(wxVERTICAL);
    app->sidePanel = side;

    nextBox_ = new wxStaticBoxSizer(wxVERTICAL, panel_, "Next Pieces");
    app->nextPieceArea = new CairoCanvas(nextBox_->GetStaticBox(), nextPieceAreaSize(),
                                         [app](cairo_t* cr, int w, int h) {
        if (!app->board) return;
        // The preview keeps its own scale (width = 12 half-size blocks) even
        // while the board grows or shrinks with the window.
        int boardBlockSize = BLOCK_SIZE;
        BLOCK_SIZE = std::max(2, 2 * (w / 12));
        onDrawNextPieceCairo(cr, app, w, h);
        BLOCK_SIZE = boardBlockSize;
    });
    nextBox_->Add(app->nextPieceArea, 0, wxALL, FromDIP(2));
    side->Add(nextBox_, 0, wxBOTTOM, FromDIP(10));

    auto addLabel = [&](wxStaticText*& label, int flags, int border) {
        label = new wxStaticText(panel_, wxID_ANY, wxEmptyString);
        side->Add(label, 0, flags, border);
    };

    addLabel(app->scoreLabel, wxBOTTOM, FromDIP(4));
    addLabel(app->levelLabel, wxBOTTOM, FromDIP(4));
    addLabel(app->linesLabel, wxBOTTOM, FromDIP(4));
    addLabel(app->sequenceLabel, wxBOTTOM, FromDIP(4));
    addLabel(app->difficultyLabel, wxBOTTOM, FromDIP(4));
    addLabel(app->controlsHeaderLabel, wxTOP | wxBOTTOM, FromDIP(10));
    addLabel(app->controlsLabel, 0, 0);

    setMarkup(app->difficultyLabel, app->board->getDifficultyText(app->difficulty));
    bool retro = app->board->retroModeActive;
    setMarkup(app->controlsHeaderLabel, retro ? "<b>ПАРТИЙНЫЕ ДИРЕКТИВЫ</b>" : "<b>Controls</b>");
    app->controlsLabel->SetLabelText(U(retro ? CONTROLS_TEXT_RETRO : CONTROLS_TEXT));
    updateLabels(app);

    mainBox->Add(side, 0, wxTOP | wxRIGHT | wxBOTTOM, FromDIP(10));
    panel_->SetSizer(mainBox);

    // Clicking anywhere in the window gives the game the keyboard back
    auto refocus = [app](wxMouseEvent& e) { if (app->gameArea) app->gameArea->SetFocus(); e.Skip(); };
    panel_->Bind(wxEVT_LEFT_DOWN, refocus);
    app->gameArea->Bind(wxEVT_LEFT_DOWN, refocus);

    relayoutWindow(app, true);
}

void TetrimoneFrame::rebuildContent() {
    TetrimoneApp* app = app_;
    if (!panel_) return;
    static_cast<CairoCanvas*>(app->nextPieceArea)->setFixedSize(nextPieceAreaSize());

    setMarkup(app->difficultyLabel, app->board->getDifficultyText(app->difficulty));
    bool retro = app->board->retroModeActive;
    bool patriotic = app->board->patrioticModeActive;
    setMarkup(app->controlsHeaderLabel, retro ? "<b>ПАРТИЙНЫЕ ДИРЕКТИВЫ</b>"
                                       : patriotic ? "<b>FREEDOM COMMANDS</b>" : "<b>Controls</b>");
    app->controlsLabel->SetLabelText(U(retro ? CONTROLS_TEXT_RETRO
                                             : patriotic ? CONTROLS_TEXT_PATRIOTIC : CONTROLS_TEXT));
    updateLabels(app);
    relayoutWindow(app, true);
    updateDisplay(app);
}

void TetrimoneFrame::onClose(wxCloseEvent& event) {
    TetrimoneApp* app = app_;
    if (app->timerId > 0) {
        g_source_remove(app->timerId);
        app->timerId = 0;
    }
    if (app->joystickTimerId > 0) {
        g_source_remove(app->joystickTimerId);
        app->joystickTimerId = 0;
    }
    stopKeyRepeat();

    if (app->board) {
        if (app->backgroundMusicPlaying) {
            app->board->pauseBackgroundMusic();
            app->backgroundMusicPlaying = false;
        }
        app->board->cleanupAudio();
    }
    if (app->joystick) {
        SDL_JoystickClose(app->joystick);
        app->joystick = nullptr;
    }
    if (app->joystickEnabled) {
        SDL_Quit();
        app->joystickEnabled = false;
    }

    // Nothing may fire into a half-destroyed window
    wx_glib_compat_remove_all_sources();
    event.Skip();  // default handler destroys the frame
}

void TetrimoneFrame::onActivate(wxActivateEvent& event) {
    TetrimoneApp* app = app_;
    event.Skip();
    if (!app->board) return;

    if (!event.GetActive()) {
        // Releasing keys while unfocused would never reach us
        stopKeyRepeat();
        if (gameAcceptsInput(app)) {
            app->pausedByFocusLoss = true;
            onPauseGame(nullptr, app);
        }
    } else {
        if (app->gameArea) app->gameArea->SetFocus();
        if (app->pausedByFocusLoss) {
            app->pausedByFocusLoss = false;
            if (app->board->isPaused() && !app->board->isGameOver()) {
                onPauseGame(nullptr, app);
            }
        }
    }
}

void TetrimoneFrame::onCharHook(wxKeyEvent& event) {
    // Let Alt/Ctrl combinations through (Alt+F4, menu mnemonics, ...)
    if (event.HasAnyModifiers() && event.GetModifiers() != wxMOD_SHIFT) {
        event.Skip();
        return;
    }
    int key = event.GetKeyCode();
    // Punctuation key codes depend on the keyboard layout; use the character
    wxChar ch = event.GetUnicodeKey();
    if (ch == '.' || ch == ',') key = ch;

    if (!handleKeyDown(app_, key)) {
        event.Skip();
    }
}

void TetrimoneFrame::onMenuOpen(wxMenuEvent& event) {
    event.Skip();
    wxMenu* menu = event.GetMenu();
    if (menu && menu->GetParent()) return;  // submenus don't change anything

    app_->openMenuDepth++;
    if (app_->openMenuDepth == 1 && gameAcceptsInput(app_) && !pausedByMenu_) {
        onPauseGame(nullptr, app_);
        // Keep "Pause" showing, as GTK3 does: this pause is temporary
        ui_set_pause_menu_label(app_, "Pause");
        pausedByMenu_ = true;
    }
}

void TetrimoneFrame::onMenuClose(wxMenuEvent& event) {
    event.Skip();
    wxMenu* menu = event.GetMenu();
    if (menu && menu->GetParent()) return;

    app_->openMenuDepth = std::max(0, app_->openMenuDepth - 1);
    // Moving the mouse between top-level menus closes one and opens the next;
    // wait a moment before deciding the menu bar is really closed.
    CallAfter([this]() {
        if (app_->openMenuDepth == 0) finishMenuPause();
    });
}

void TetrimoneFrame::finishMenuPause() {
    app_->openMenuDepth = 0;
    if (!pausedByMenu_) return;
    pausedByMenu_ = false;
    if (app_->board->isPaused() && !app_->board->isGameOver()) {
        onPauseGame(nullptr, app_);
    }
}

void TetrimoneFrame::onAnyMenuCommand(wxCommandEvent& event) {
    finishMenuPause();
    event.Skip();
}

// ============================================================================
// Application
// ============================================================================

class TetrimoneWxApp : public wxApp {
public:
    TetrimoneWxApp(const CommandLineArgs* args) : args_(args) {}

    bool OnInit() override;
    int OnExit() override;
    int FilterEvent(wxEvent& event) override;

    // Tetrimone parses its own command line in main()
    bool OnCmdLineParsed(wxCmdLineParser&) override { return true; }
    void OnInitCmdLine(wxCmdLineParser&) override {}

private:
    const CommandLineArgs* args_;
    TetrimoneApp* app_ = nullptr;
};

bool TetrimoneWxApp::OnInit() {
    SetAppName("tetrimone");
    SetAppDisplayName("Tetrimone");
    wxInitAllImageHandlers();

    // Resources (sound.zip, background.zip) are looked up relative to the
    // working directory. When launched from Explorer or a shortcut that isn't
    // the install folder, so fall back to the executable's directory.
    if (!wxFileExists("sound.zip") && !wxFileExists("background.zip")) {
        wxFileName exe(wxStandardPaths::Get().GetExecutablePath());
        wxString exeDir = exe.GetPath();
        if (wxFileExists(exeDir + wxFILE_SEP_PATH + "sound.zip") ||
            wxFileExists(exeDir + wxFILE_SEP_PATH + "background.zip")) {
            wxSetWorkingDirectory(exeDir);
        }
    }

    TetrimoneApp* app = new TetrimoneApp();
    app_ = app;
    g_app = app;
    app->cmdlineArgs = args_;
    app->board = new TetrimoneBoard();
    app->board->setApp(app);
    app->timerId = 0;
    app->dropSpeed = INITIAL_SPEED;
    app->difficulty = 2;

    if (args_ && args_->gridWidth != -1) GRID_WIDTH = args_->gridWidth;
    if (args_ && args_->gridHeight != -1) GRID_HEIGHT = args_->gridHeight;
    if (!args_ || args_->blockSize == -1) {
        calculateBlockSize(app);
    } else {
        BLOCK_SIZE = args_->blockSize;
    }

    TetrimoneFrame* frame = new TetrimoneFrame(app);
    frame->buildMenu();
    frame->buildContent();

    enableItem(app->startMenuItem, false);
    enableItem(app->pauseMenuItem, true);

    if (app->board->initializeAudio()) {
        app->board->playBackgroundMusic();
        app->backgroundMusicPlaying = true;
    } else {
        printf("Music failed to initialize\n");
        checkItem(app->soundToggleMenuItem, false);
    }

    app->joystick = nullptr;
    app->joystickEnabled = false;
    app->joystickTimerId = 0;

    if (args_) {
        applyCommandLineArgs(app, *args_);
        // Block size / grid may have changed
        frame->rebuildContent();
    }

    initSDL(app);
    startJoystickPolling(app);

    frame->Centre();
    frame->Show();
    app->gameArea->SetFocus();

    startGame(app);
    return true;
}

int TetrimoneWxApp::OnExit() {
    wx_glib_compat_remove_all_sources();
    if (app_) {
        TetrimoneApp* app = app_;
        app_ = nullptr;
        g_app = nullptr;
        app->window = nullptr;
        app->gameArea = nullptr;
        app->nextPieceArea = nullptr;
        cleanupApp(app);
    }
    return wxApp::OnExit();
}

int TetrimoneWxApp::FilterEvent(wxEvent& event) {
    // Key releases aren't routed through wxEVT_CHAR_HOOK, so catch them here
    // for any window inside the main frame.
    if (event.GetEventType() == wxEVT_KEY_UP && app_ && app_->window) {
        wxWindow* win = wxDynamicCast(event.GetEventObject(), wxWindow);
        if (win && wxGetTopLevelParent(win) == app_->window) {
            handleKeyUp(app_, static_cast<wxKeyEvent&>(event).GetKeyCode());
        }
    }
    return Event_Skip;
}

// Declared in tetrimone_core.h; called from main() in tetrimone_main.cpp
int ui_run_application(int argc, char* argv[], TetrimoneApp* /*app*/, const CommandLineArgs* args) {
    wxApp::SetInstance(new TetrimoneWxApp(args));
    return wxEntry(argc, argv);
}
