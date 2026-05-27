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
#include <QDialog>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QRadioButton>
#include <QButtonGroup>
#include <QGroupBox>
#include <QTimer>

void showIdeologicalFailureDialog(TetrimoneApp* app) {
    if (!app || !app->board) return;
    
    QDialog dialog(app->window);
    dialog.setWindowTitle("ОБЪЯСНЕНИЕ ИДЕОЛОГИЧЕСКОГО ПРОВАЛА");
    dialog.setMinimumSize(500, 400);
    
    QVBoxLayout* mainLayout = new QVBoxLayout(&dialog);
    mainLayout->setContentsMargins(15, 15, 15, 15);
    mainLayout->setSpacing(15);
    
    // Title
    QLabel* titleLabel = new QLabel(&dialog);
    titleLabel->setText("ВНИМАНИЕ, ГРАЖДАНИН!\nВАШ ИДЕОЛОГИЧЕСКИЙ ПРОВАЛ ТРЕБУЕТ ОБЪЯСНЕНИЯ");
    QFont titleFont = titleLabel->font();
    titleFont.setPointSize(12);
    titleFont.setBold(true);
    titleLabel->setFont(titleFont);
    titleLabel->setStyleSheet("color: red;");
    titleLabel->setAlignment(Qt::AlignCenter);
    mainLayout->addWidget(titleLabel);
    
    // Instruction
    QLabel* instructionLabel = new QLabel("Укажите основную причину вашего антиреволюционного поведения:\n"
                                          "(Indicate the main reason for your counter-revolutionary behavior:)", &dialog);
    mainLayout->addWidget(instructionLabel);
    
    // Radio buttons
    QGroupBox* optionsGroup = new QGroupBox("Самокритика (Self-Criticism)", &dialog);
    QVBoxLayout* optionsLayout = new QVBoxLayout(optionsGroup);
    
    QButtonGroup* buttonGroup = new QButtonGroup(&dialog);
    
    std::vector<QString> options = {
        "Недостаточная преданность Партии (Insufficient Party loyalty)",
        "Буржуазные наклонности к неэффективности (Bourgeois tendencies toward inefficiency)",
        "Западный шпионаж повлиял на мои движения (Western espionage influenced my movements)",
        "Слишком много времени тратил на чтение непартийной литературы (Too much time spent reading non-Party literature)",
        "Идеологическая диверсия со стороны капиталистических блоков (Ideological subversion from capitalist blocks)",
        "Недостаточное потребление пропаганды в свободное время (Insufficient consumption of propaganda during free time)",
        "Контрреволюционное мышление (Counter-revolutionary thinking)"
    };
    
    for (size_t i = 0; i < options.size(); ++i) {
        QRadioButton* radio = new QRadioButton(options[i], &dialog);
        if (i == 0) radio->setChecked(true);
        buttonGroup->addButton(radio, i);
        optionsLayout->addWidget(radio);
    }
    
    mainLayout->addWidget(optionsGroup);
    
    // Footer
    QLabel* footerLabel = new QLabel("Внимание: Ваш ответ будет записан в ваше личное дело\n"
                                      "(Warning: Your answer will be recorded in your personal file)", &dialog);
    footerLabel->setStyleSheet("color: red; font-style: italic;");
    footerLabel->setAlignment(Qt::AlignCenter);
    mainLayout->addWidget(footerLabel);
    
    mainLayout->addStretch();
    
    // Buttons
    QHBoxLayout* buttonLayout = new QHBoxLayout();
    buttonLayout->addStretch();
    
    QPushButton* acceptBtn = new QPushButton("Принять наказание (Accept)", &dialog);
    QPushButton* cancelBtn = new QPushButton("Cancel", &dialog);
    
    QObject::connect(acceptBtn, &QPushButton::clicked, [&dialog]() {
        dialog.accept();
    });
    
    QObject::connect(cancelBtn, &QPushButton::clicked, &dialog, &QDialog::reject);
    
    buttonLayout->addWidget(acceptBtn);
    buttonLayout->addWidget(cancelBtn);
    mainLayout->addLayout(buttonLayout);
    
    dialog.exec();
}

#endif  // QT5
