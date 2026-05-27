#ifndef QT5_DIALOG_HELPERS_H
#define QT5_DIALOG_HELPERS_H

#include <QDialog>
#include <QButtonGroup>
#include <vector>
#include <string>
#include "highscores.h"

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

// ============================================================================
// Qt5 Dialog Classes
// ============================================================================

class FreedomPerformanceDialog : public QDialog {
    Q_OBJECT
public:
    FreedomPerformanceDialog(QWidget* parent,
                             const DialogConfig& dialogConfig,
                             const std::vector<TextConfig>& textElements,
                             const RadioGroupConfig* radioConfig,
                             const std::vector<TextConfig>& footerElements);
    
    int getSelectedIndex() const;

private slots:
    void onRadioSelected(int id);

private:
    QButtonGroup* buttonGroup;
    int selectedIndex;
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

// Create and run high scores dialog (tabbed view of all scores)
void createScoreTabulatorDialog(
    void* parent,
    const ScoreTabulatorConfig& config
);

}  // namespace Qt5Helpers

#endif  // QT5_DIALOG_HELPERS_H
