#include "tetrimone.h"
#include "audiomanager.h"
#include "gtk3_dialog_helpers.h"
#include <iostream>
#include <string>
#include <algorithm>
#ifdef _WIN32
#include <windows.h>
#include <commdlg.h>
#endif
#include "highscores.h"
#include "freedom_messages.h"

using namespace GTK3Helpers;

void showPatrioticPerformanceDialog(TetrimoneApp* app) {
    // Only show in patriotic mode
    if (!app->board->patrioticModeActive) {
        return;
    }
    
    // Configure dialog appearance
    DialogConfig dialogConfig{
        .title = "FREEDOM PERFORMANCE EVALUATION",
        .acceptButtonLabel = "_Accept Responsibility",
        .width = 550,
        .height = 450
    };
    
    // Configure text elements
    std::vector<TextConfig> textElements{
        {
            .content = "",
            .markup = "<span size='large' weight='bold' foreground='blue'>"
                      "ATTENTION, CITIZEN!\n"
                      "YOUR FREEDOM PERFORMANCE REQUIRES EVALUATION</span>",
            .isMarkup = true,
            .marginTop = 0,
            .marginBottom = 0
        },
        {
            .content = "Please indicate the primary reason for your un-American block performance:\n"
                      "(Your response helps us serve you better!)",
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
            .markup = "<span style='italic' foreground='blue'>"
                     "Remember: In America, failure is just another opportunity to succeed!\n"
                     "Your data helps us optimize your gaming experience. Privacy policy available online.</span>",
            .isMarkup = true,
            .marginTop = 10,
            .marginBottom = 0
        },
        {
            .content = "",
            .markup = "<span weight='bold' foreground='red'>"
                     "🦅 GOD BLESS AMERICA AND GOD BLESS YOUR BLOCKS! 🦅</span>",
            .isMarkup = true,
            .marginTop = 0,
            .marginBottom = 5
        }
    };
    
    // Configure radio button group
    RadioGroupConfig radioConfig{
        .frameTitle = "Personal Accountability Assessment",
        .options = {
            "🇺🇸 Insufficient consumption of freedom during gameplay",
            "🍔 Too much fast food affecting hand-eye coordination",
            "📺 Distracted by the latest Netflix series during play",
            "🏈 Still thinking about last night's football game",
            "💼 Work-life balance priorities shifted toward actual work",
            "🎬 Hollywood movies set unrealistic block-stacking expectations",
            "☕ Not enough coffee to maintain peak performance",
            "🚗 Traffic on the commute affected my gaming mindset",
            "📱 Social media notifications broke my concentration",
            "🛒 Worried about credit card bills instead of focusing on blocks",
            "🏠 HOA meeting stressed me out before playing",
            "🌮 Taco Tuesday made me hungry instead of focused"
        },
        .defaultSelectedIndex = 0
    };
    
    // Create and run dialog - all GTK3 calls are encapsulated in helpers
    GtkWidget* dialog = createDialog(
        GTK_WINDOW(app->window),
        dialogConfig,
        textElements,
        &radioConfig,
        footerElements
    );
    
    runDialog(dialog);
    
    // Restart game with renewed American spirit!
    onRestartGame(NULL, app);
}
