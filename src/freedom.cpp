#include "audiomanager.h"
#include <iostream>
#include <string>
#include <algorithm>
#include <chrono>
#include <random>

#ifdef QT5
#include "qt5_dialog_helpers.h"
#include "tetrimone_qt5.h"
#include <QApplication>
#include <QMessageBox>
#endif

#ifdef GTK3
#include "gtk3_dialog_helpers.h"
#include "tetrimone_gtk.h"
#ifdef _WIN32
#include <windows.h>
#include <commdlg.h>
#endif
#include <gtk/gtk.h>
#endif

#include "highscores.h"
#include "freedom_messages.h"

#ifdef GTK3
using namespace GTK3Helpers;
#endif

#ifdef QT5
using namespace Qt5Helpers;
#endif

#ifdef GTK3

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
    
    // Create and run dialog - works with GTK3
    createAndRunDialog(
        GTK_WINDOW(app->window),
        dialogConfig,
        textElements,
        &radioConfig,
        footerElements
    );
    
    // Restart game with renewed American spirit!
    app->board->restart();
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

void showPatrioticPerformanceDialog(TetrimoneApp* app) {
    if (!app || !app->board) return;
    
    QDialog dialog(app->window);
    dialog.setWindowTitle("FREEDOM PERFORMANCE EVALUATION");
    dialog.setMinimumSize(550, 500);
    
    QVBoxLayout* mainLayout = new QVBoxLayout(&dialog);
    mainLayout->setContentsMargins(15, 15, 15, 15);
    mainLayout->setSpacing(15);
    
    // Title
    QLabel* titleLabel = new QLabel(&dialog);
    titleLabel->setText("ATTENTION, CITIZEN!\nYOUR FREEDOM PERFORMANCE REQUIRES EVALUATION");
    QFont titleFont = titleLabel->font();
    titleFont.setPointSize(12);
    titleFont.setBold(true);
    titleLabel->setFont(titleFont);
    titleLabel->setStyleSheet("color: blue;");
    titleLabel->setAlignment(Qt::AlignCenter);
    mainLayout->addWidget(titleLabel);
    
    // Instruction
    QLabel* instructionLabel = new QLabel("Please indicate the primary reason for your un-American block performance:\n"
                                          "(Your response helps us serve you better!)", &dialog);
    mainLayout->addWidget(instructionLabel);
    
    // Radio buttons
    QGroupBox* optionsGroup = new QGroupBox("Personal Accountability Assessment", &dialog);
    QVBoxLayout* optionsLayout = new QVBoxLayout(optionsGroup);
    
    QButtonGroup* buttonGroup = new QButtonGroup(&dialog);
    
    std::vector<QString> options = {
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
    };
    
    for (size_t i = 0; i < options.size(); ++i) {
        QRadioButton* radio = new QRadioButton(options[i], &dialog);
        if (i == 0) radio->setChecked(true);
        buttonGroup->addButton(radio, i);
        optionsLayout->addWidget(radio);
    }
    
    mainLayout->addWidget(optionsGroup);
    
    // Footer
    QLabel* footerLabel = new QLabel("Remember: In America, failure is just another opportunity to succeed!\n"
                                      "Your data helps us optimize your gaming experience. Privacy policy available online.", &dialog);
    footerLabel->setStyleSheet("color: blue; font-style: italic;");
    footerLabel->setAlignment(Qt::AlignCenter);
    mainLayout->addWidget(footerLabel);
    
    QLabel* patriotLabel = new QLabel("🦅 GOD BLESS AMERICA AND GOD BLESS YOUR BLOCKS! 🦅", &dialog);
    patriotLabel->setStyleSheet("color: red; font-weight: bold;");
    patriotLabel->setAlignment(Qt::AlignCenter);
    mainLayout->addWidget(patriotLabel);
    
    mainLayout->addStretch();
    
    // Buttons
    QHBoxLayout* buttonLayout = new QHBoxLayout();
    buttonLayout->addStretch();
    
    QPushButton* acceptBtn = new QPushButton("Accept Responsibility", &dialog);
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
