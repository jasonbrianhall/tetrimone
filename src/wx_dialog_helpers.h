// ============================================================================
// wx_dialog_helpers.h - wxWidgets implementation of the dialog-helper API
//
// Same configuration structs and function names as gtk3_dialog_helpers.h so
// the shared dialog code (help, propaganda, freedom, high scores) can be used
// by both frontends. Parent windows are wxWindow* instead of GtkWindow*.
// ============================================================================

#ifndef WX_DIALOG_HELPERS_H
#define WX_DIALOG_HELPERS_H

#include <functional>
#include <string>
#include <vector>
#include "highscores.h"

class wxWindow;

namespace WXHelpers {

// ============================================================================
// Configuration Structures (identical to GTK3Helpers)
// ============================================================================

struct DialogConfig {
    std::string title;
    std::string acceptButtonLabel;
    int width;
    int height;
};

struct TextConfig {
    std::string content;
    std::string markup;  // Pango-style markup; wx understands the common subset
    bool isMarkup;
    double marginTop;
    double marginBottom;
};

struct RadioGroupConfig {
    std::string frameTitle;
    std::vector<std::string> options;
    int defaultSelectedIndex;
};

struct ScrolledTextConfig {
    std::string content;
    std::string fontDescription;  // e.g. "Monospace 10" or "Sans 11"
    int width;
    int height;
};

struct ScoreEntryConfig {
    std::string title;
    int score;
    std::string difficulty;
    std::string gridSize;
    std::string junkInfo;
};

struct ScoreTabData {
    std::string tabName;
    std::vector<Score> scores;
};

struct ScoreTabulatorConfig {
    std::string title;
    std::vector<ScoreTabData> tabs;
    int width;
    int height;
};

struct OpacitySliderConfig {
    std::string title;
    double minValue;
    double maxValue;
    double stepValue;
    double currentValue;
    int width;
    int height;
};

struct JoystickMappingConfig {
    std::string title;
    int numButtons;
    int numAxes;
    int rotate_cw;
    int rotate_ccw;
    int hard_drop;
    int pause_button;
    int x_axis;
    int y_axis;
    bool invert_x;
    bool invert_y;
    int width;
    int height;
};

struct GameSetupConfig {
    std::string title;
    int junkPercentage;
    int junkPerLevel;
    int initialLevel;
    int width;
    int height;
};

struct VolumeControlConfig {
    std::string title;
    std::string okButtonLabel;
    bool isRetroMode;
    int sfxVolume;
    int musicVolume;
    int width;
    int height;
};

// Dialog response codes (mirror the GTK values the shared code may compare to)
enum Response {
    RESPONSE_NONE   = -1,
    RESPONSE_OK     = -5,
    RESPONSE_CANCEL = -6,
    RESPONSE_CLOSE  = -7,
    RESPONSE_YES    = -8,
    RESPONSE_NO     = -9,
    RESPONSE_APPLY  = -10
};

// ============================================================================
// File dialogs
// ============================================================================

class WXFileDialog {
public:
    explicit WXFileDialog(wxWindow* parent) : parentWindow(parent) {}

    // filter is a glob such as "*.zip" or "*.png;*.jpg;*.jpeg"
    std::string openFile(const std::string& title,
                         const std::string& filter,
                         const std::string& filterDescription);

    std::vector<std::string> openFiles(const std::string& title,
                                       const std::string& filter,
                                       const std::string& filterDescription);

    void showError(const std::string& title, const std::string& message);

private:
    wxWindow* parentWindow;
};

// ============================================================================
// Dialogs
// ============================================================================

// Generic informational dialog: stacked text, optional radio group, footer text.
// Returns RESPONSE_OK when accepted, RESPONSE_CANCEL if closed.
int createAndRunDialog(
    wxWindow* parent,
    const DialogConfig& dialogConfig,
    const std::vector<TextConfig>& textElements,
    const RadioGroupConfig* radioConfig = nullptr,
    const std::vector<TextConfig>& footerElements = std::vector<TextConfig>());

// Read-only scrolled text (instructions)
void createAndRunScrolledTextDialog(
    wxWindow* parent,
    const DialogConfig& dialogConfig,
    const ScrolledTextConfig& textConfig);

// Returns the player's name, "Anonymous" for a blank entry, "" if cancelled
std::string createScoreEntryDialog(wxWindow* parent, const ScoreEntryConfig& config);

// Tabbed, sortable high score tables
void createScoreTabulatorDialog(wxWindow* parent, const ScoreTabulatorConfig& config);

// Live-updating opacity slider; onValueChanged receives the new value
void createOpacitySliderDialog(
    wxWindow* parent,
    const OpacitySliderConfig& config,
    std::function<void(double)> onValueChanged);

// Joystick mapping; onApply fires when the "Apply Mapping" button is pressed
typedef std::function<void(int rotate_cw, int rotate_ccw, int hard_drop, int pause_btn,
                           int x_axis, int y_axis, bool invert_x, bool invert_y)>
    JoystickApplyCallback;

void createJoystickMappingDialog(
    wxWindow* parent,
    const JoystickMappingConfig& config,
    JoystickApplyCallback onApply);

// Game setup (junk lines, starting level); onApply fires on "Apply"
typedef std::function<void(int junkPercentage, int junkPerLevel, int initialLevel)>
    GameSetupApplyCallback;

void createGameSetupDialog(
    wxWindow* parent,
    const GameSetupConfig& config,
    GameSetupApplyCallback onApply);

// Volume sliders with live feedback (values are 0-100)
void createVolumeControlDialog(
    wxWindow* parent,
    const VolumeControlConfig& config,
    std::function<void(int)> onSfxVolumeChanged,
    std::function<void(int)> onMusicVolumeChanged);

// Simple message helpers
bool askYesNo(wxWindow* parent, const std::string& message, const std::string& title = "Question");
void showInfo(wxWindow* parent, const std::string& message, const std::string& title = "Information");
void showError(wxWindow* parent, const std::string& message, const std::string& title = "Error");

// Generic "one or more sliders + Apply/Cancel" dialog, used for block size,
// game size and block trail settings. Values are updated in place when the
// user presses Apply (return true); untouched on Cancel (return false).
struct SliderSpec {
    std::string frameTitle;    // group box title; empty = no box
    double minValue;
    double maxValue;
    double step;
    double value;              // in: initial, out: chosen
    std::string valueFormat;   // printf format for the readout, e.g. "Width: %.0f"
    std::string leftCaption;   // optional captions under the slider
    std::string rightCaption;
};

bool createSliderSettingsDialog(
    wxWindow* parent,
    const std::string& title,
    const std::string& intro,
    std::vector<SliderSpec>& sliders,
    const std::string& note,
    int width,
    int height);

// Converts GTK-style mnemonics ("_OK") to wx style ("&OK")
std::string convertMnemonic(const std::string& gtkLabel);

// Plain-text version of a markup string (tags removed, entities decoded)
std::string stripMarkup(const std::string& markup);

}  // namespace WXHelpers

#endif  // WX_DIALOG_HELPERS_H
