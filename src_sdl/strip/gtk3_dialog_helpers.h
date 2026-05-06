#ifndef GTK3_DIALOG_HELPERS_H
#define GTK3_DIALOG_HELPERS_H

#include <gtk/gtk.h>
#include <vector>
#include <string>
#include "highscores.h"

namespace GTK3Helpers {

// ============================================================================
// Configuration Structures
// ============================================================================

struct DialogConfig {
    std::string title;
    std::string acceptButtonLabel;
    int width;
    int height;
};

struct TextConfig {
    std::string content;
    std::string markup;  // Use this for Pango markup instead of plain content
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
    std::string fontDescription;  // e.g., "Monospace 10" or "Sans 11"
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

// ============================================================================
// File Dialog Interface (framework-agnostic)
// ============================================================================

class FileDialogInterface {
public:
    virtual ~FileDialogInterface() = default;
    
    virtual std::string openFile(
        const std::string& title,
        const std::string& filter,
        const std::string& filterDescription
    ) = 0;
    
    virtual std::vector<std::string> openFiles(
        const std::string& title,
        const std::string& filter,
        const std::string& filterDescription
    ) = 0;
    
    virtual void showError(
        const std::string& title,
        const std::string& message
    ) = 0;
};

// ============================================================================
// GTK3 Implementation of FileDialogInterface
// ============================================================================

class GTK3FileDialog : public FileDialogInterface {
private:
    GtkWindow* parentWindow;

public:
    explicit GTK3FileDialog(GtkWindow* parent) : parentWindow(parent) {}
    
    std::string openFile(
        const std::string& title,
        const std::string& filter,
        const std::string& filterDescription
    ) override;
    
    std::vector<std::string> openFiles(
        const std::string& title,
        const std::string& filter,
        const std::string& filterDescription
    ) override;
    
    void showError(
        const std::string& title,
        const std::string& message
    ) override;
};

// ============================================================================
// Public Interface
// ============================================================================

// Build a labeled text widget
GtkWidget* createTextLabel(const TextConfig& config);

// Build a radio button group and return the group list
GSList* createRadioGroup(GtkWidget* container, const RadioGroupConfig& config);

// Build a scrolled text view
GtkWidget* createScrolledTextView(const ScrolledTextConfig& config);

// Create a score entry dialog and return the player name (empty string if cancelled)
std::string createScoreEntryDialog(
    GtkWindow* parent,
    const ScoreEntryConfig& config
);

// Create a tabbed score viewer dialog
void createScoreTabulatorDialog(
    GtkWindow* parent,
    const ScoreTabulatorConfig& config
);

// Build a complete dialog with title, content area, and buttons
GtkWidget* createDialog(
    GtkWindow* parent,
    const DialogConfig& dialogConfig,
    const std::vector<TextConfig>& textElements,
    const RadioGroupConfig* radioConfig = nullptr,
    const std::vector<TextConfig>& footerElements = std::vector<TextConfig>()
);

// Run dialog and handle cleanup
gint runDialog(GtkWidget* dialog);

// High-level function: create and run dialog in one call
gint createAndRunDialog(
    GtkWindow* parent,
    const DialogConfig& dialogConfig,
    const std::vector<TextConfig>& textElements,
    const RadioGroupConfig* radioConfig = nullptr,
    const std::vector<TextConfig>& footerElements = std::vector<TextConfig>()
);

// Create an opacity slider dialog
void createOpacitySliderDialog(
    GtkWindow* parent,
    const OpacitySliderConfig& config,
    GCallback onValueChanged,
    gpointer userData
);

}  // namespace GTK3Helpers

#endif  // GTK3_DIALOG_HELPERS_H
