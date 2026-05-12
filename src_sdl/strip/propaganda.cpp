#include "audiomanager.h"
#include <iostream>
#include <string>
#include <algorithm>
#include <chrono>
#include <random>
#ifdef _WIN32
#include <windows.h>
#include <commdlg.h>
#endif
#include "highscores.h"
#include "propaganda_messages.h"

// Include framework headers FIRST so TetrimoneApp is declared
#ifdef GTK3
#include "tetrimone_gtk.h"
#include "gtk3_dialog_helpers.h"
#endif

#ifdef QT5
#include "tetrimone_qt5.h"
#include <QMessageBox>
#endif

// ============================================================================
// Framework-Specific Propaganda Dialogs
// ============================================================================

#ifdef GTK3

using namespace GTK3Helpers;

void showIdeologicalFailureDialog(TetrimoneApp* app) {
    // Only show in retro mode
    if (!app->board->retroModeActive) {
        return;
    }
    
    // Configure dialog appearance
    DialogConfig dialogConfig{
        .title = "ОБЪЯСНЕНИЕ ИДЕОЛОГИЧЕСКОГО ПРОВАЛА",
        .acceptButtonLabel = "_Принять наказание",
        .width = 500,
        .height = 400
    };
    
    // Configure text elements
    std::vector<TextConfig> textElements{
        {
            .content = "",
            .markup = "<span size='large' weight='bold' foreground='red'>"
                      "ВНИМАНИЕ, ГРАЖДАНИН!\n"
                      "ВАШ ИДЕОЛОГИЧЕСКИЙ ПРОВАЛ ТРЕБУЕТ ОБЪЯСНЕНИЯ</span>",
            .isMarkup = true,
            .marginTop = 0,
            .marginBottom = 0
        },
        {
            .content = "Укажите основную причину вашего антиреволюционного поведения:\n"
                      "(Indicate the main reason for your counter-revolutionary behavior:)",
            .markup = "",
            .isMarkup = false,
            .marginTop = 0,
            .marginBottom = 10
        }
    };
    
    // Configure footer elements
    std::vector<TextConfig> footerElements{
        {
            .content = "",
            .markup = "<span style='italic' foreground='red'>"
                     "Внимание: Ваш ответ будет записан в ваше личное дело\n"
                     "(Warning: Your answer will be recorded in your personal file)</span>",
            .isMarkup = true,
            .marginTop = 10,
            .marginBottom = 0
        }
    };
    
    // Configure radio button group
    RadioGroupConfig radioConfig{
        .frameTitle = "Самокритика (Self-Criticism)",
        .options = {
            "Недостаточная преданность Партии (Insufficient Party loyalty)",
            "Буржуазные наклонности к неэффективности (Bourgeois tendencies toward inefficiency)",
            "Западный шпионаж повлиял на мои движения (Western espionage influenced my movements)",
            "Слишком много времени тратил на чтение непартийной литературы (Too much time spent reading non-Party literature)",
            "Идеологическая диверсия со стороны капиталистических блоков (Ideological subversion from capitalist blocks)",
            "Недостаточное потребление пропаганды в свободное время (Insufficient consumption of propaganda during free time)",
            "Контрреволюционное мышление (Counter-revolutionary thinking)"
        },
        .defaultSelectedIndex = 0
    };
    
    // Create and run dialog - all GTK3 calls delegated to helper
    createAndRunDialog(
        GTK_WINDOW(app->window),
        dialogConfig,
        textElements,
        &radioConfig,
        footerElements
    );
    
    // The game will restart after this dialog
    onRestartGame(GTK_MENU_ITEM(app->restartMenuItem), app);
}

#endif  // GTK3

#ifdef QT5

void showIdeologicalFailureDialog(TetrimoneApp* app) {
    if (!app || !app->board) return;
    
    // Use the propaganda messages vector
    extern const std::vector<std::string> PROPAGANDA_MESSAGES;
    
    if (PROPAGANDA_MESSAGES.empty()) {
        app->board->currentPropagandaMessage = "YOUR BLOCKS HAVE FAILED THE STATE!";
    } else {
        static std::mt19937 rng(std::chrono::system_clock::now().time_since_epoch().count());
        std::uniform_int_distribution<int> dist(0, PROPAGANDA_MESSAGES.size() - 1);
        app->board->currentPropagandaMessage = PROPAGANDA_MESSAGES[dist(rng)];
    }
    
    // Show the message
    app->board->showPropagandaMessage = true;
    
    // Force repaint of game area
    if (app->gameArea) {
        app->gameArea->update();
    }
    
    // Set timer to clear message after 2 seconds
    QTimer::singleShot(2000, [app]() {
        app->board->showPropagandaMessage = false;
        if (app->gameArea) {
            app->gameArea->update();
        }
    });
}

#endif  // QT5
