#ifndef QT5_DIALOG_HELPERS_H
#define QT5_DIALOG_HELPERS_H

#include <QDialog>
#include <vector>
#include <string>

// Forward declarations
struct TetrimoneApp;

namespace Qt5Helpers {

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
    std::string markup;
    bool isMarkup;
    double marginTop;
    double marginBottom;
};

struct RadioGroupConfig {
    std::string frameTitle;
    std::vector<std::string> options;
    int defaultSelectedIndex;
};

// ============================================================================
// Public Interface
// ============================================================================

// High-level function: create and run dialog in one call
// Returns QDialog::Accepted or QDialog::Rejected
int createAndRunDialog(
    void* parent,  // Unused in Qt5, kept for API compatibility with GTK3 version
    const DialogConfig& dialogConfig,
    const std::vector<TextConfig>& textElements,
    const RadioGroupConfig* radioConfig = nullptr,
    const std::vector<TextConfig>& footerElements = std::vector<TextConfig>()
);

}  // namespace Qt5Helpers

#endif  // QT5_DIALOG_HELPERS_H
