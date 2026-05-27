#include "qt5_dialog_helpers.h"
#include "highscores.h"
#include <QDialog>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QRadioButton>
#include <QButtonGroup>
#include <QGroupBox>
#include <QScrollArea>
#include <QFont>
#include <QApplication>
#include <QScreen>
#include <QTabWidget>
#include <QTableWidget>
#include <QTableWidgetItem>
#include <QHeaderView>
#include <string>
#include <vector>

namespace Qt5Helpers {

// Constructor implementation
FreedomPerformanceDialog::FreedomPerformanceDialog(QWidget* parent,
                             const DialogConfig& dialogConfig,
                             const std::vector<TextConfig>& textElements,
                             const RadioGroupConfig* radioConfig,
                             const std::vector<TextConfig>& footerElements)
    : QDialog(parent), selectedIndex(-1) {
        setWindowTitle(QString::fromStdString(dialogConfig.title));
        setFixedSize(dialogConfig.width, dialogConfig.height);
        
        QVBoxLayout* mainLayout = new QVBoxLayout(this);
        mainLayout->setContentsMargins(15, 15, 15, 15);
        mainLayout->setSpacing(10);
        
        // Add text elements
        for (const auto& textConfig : textElements) {
            QLabel* label = new QLabel(this);
            if (textConfig.isMarkup) {
                label->setText(QString::fromStdString(textConfig.markup));
            } else {
                label->setText(QString::fromStdString(textConfig.content));
            }
            label->setWordWrap(true);
            label->setMargin(static_cast<int>(textConfig.marginTop));
            mainLayout->addSpacing(static_cast<int>(textConfig.marginTop));
            mainLayout->addWidget(label);
            mainLayout->addSpacing(static_cast<int>(textConfig.marginBottom));
        }
        
        // Add radio button group if provided
        if (radioConfig) {
            QGroupBox* groupBox = new QGroupBox(QString::fromStdString(radioConfig->frameTitle), this);
            QVBoxLayout* radioLayout = new QVBoxLayout(groupBox);
            radioLayout->setSpacing(8);
            
            buttonGroup = new QButtonGroup(this);
            for (size_t i = 0; i < radioConfig->options.size(); ++i) {
                QRadioButton* radioBtn = new QRadioButton(
                    QString::fromStdString(radioConfig->options[i]),
                    this
                );
                if (static_cast<int>(i) == radioConfig->defaultSelectedIndex) {
                    radioBtn->setChecked(true);
                    selectedIndex = i;
                }
                buttonGroup->addButton(radioBtn, i);
                radioLayout->addWidget(radioBtn);
            }
            
            connect(buttonGroup, QOverload<int>::of(&QButtonGroup::idClicked),
                    this, &FreedomPerformanceDialog::onRadioSelected);
            
            mainLayout->addWidget(groupBox);
        }
        
        // Add footer elements
        for (const auto& footerConfig : footerElements) {
            QLabel* footer = new QLabel(this);
            if (footerConfig.isMarkup) {
                footer->setText(QString::fromStdString(footerConfig.markup));
            } else {
                footer->setText(QString::fromStdString(footerConfig.content));
            }
            footer->setWordWrap(true);
            footer->setMargin(static_cast<int>(footerConfig.marginTop));
            mainLayout->addSpacing(static_cast<int>(footerConfig.marginTop));
            mainLayout->addWidget(footer);
        }
        
        mainLayout->addStretch();
        
        // Add button
        QHBoxLayout* buttonLayout = new QHBoxLayout();
        buttonLayout->addStretch();
        QPushButton* acceptBtn = new QPushButton(QString::fromStdString(dialogConfig.acceptButtonLabel), this);
        connect(acceptBtn, &QPushButton::clicked, this, &QDialog::accept);
        buttonLayout->addWidget(acceptBtn);
        mainLayout->addLayout(buttonLayout);
    }

int FreedomPerformanceDialog::getSelectedIndex() const {
    return selectedIndex;
}

void FreedomPerformanceDialog::onRadioSelected(int id) {
    selectedIndex = id;
}

int createAndRunDialog(
    void* parent,
    const DialogConfig& dialogConfig,
    const std::vector<TextConfig>& textElements,
    const RadioGroupConfig* radioConfig,
    const std::vector<TextConfig>& footerElements) {
    
    // In Qt5 mode, parent is unused (we use QApplication's active window)
    (void)parent;
    
    FreedomPerformanceDialog dialog(nullptr, dialogConfig, textElements, radioConfig, footerElements);
    return dialog.exec();
}

void createScoreTabulatorDialog(
    void* parent,
    const ScoreTabulatorConfig& config) {
    
    (void)parent;  // Unused in Qt5
    
    QDialog dialog(nullptr);
    dialog.setWindowTitle(QString::fromStdString(config.title));
    dialog.setMinimumSize(config.width, config.height);
    
    QVBoxLayout* mainLayout = new QVBoxLayout(&dialog);
    mainLayout->setContentsMargins(10, 10, 10, 10);
    
    // Create tab widget
    QTabWidget* tabWidget = new QTabWidget(&dialog);
    
    // Create a tab for each score set
    for (const auto& tabData : config.tabs) {
        QTableWidget* table = new QTableWidget(&dialog);
        
        // Set up columns
        table->setColumnCount(5);
        table->setHorizontalHeaderLabels({"Name", "Score", "Difficulty", "Grid Size", "Junk Lines"});
        table->horizontalHeader()->setStretchLastSection(true);
        
        // Populate rows
        table->setRowCount(tabData.scores.size());
        for (size_t i = 0; i < tabData.scores.size(); ++i) {
            const auto& score = tabData.scores[i];
            
            // Name
            table->setItem(i, 0, new QTableWidgetItem(QString::fromStdString(score.name)));
            
            // Score
            table->setItem(i, 1, new QTableWidgetItem(QString::number(score.score)));
            
            // Difficulty
            table->setItem(i, 2, new QTableWidgetItem(QString::fromStdString(score.difficulty)));
            
            // Grid Size
            QString gridSize = QString("%1 x %2").arg(score.width).arg(score.height);
            table->setItem(i, 3, new QTableWidgetItem(gridSize));
            
            // Junk Settings
            QString junkInfo = QString("Init: %1%%, Level: %2")
                .arg(score.initialJunkPercent)
                .arg(score.junkLinesPerLevel);
            table->setItem(i, 4, new QTableWidgetItem(junkInfo));
        }
        
        table->setSelectionBehavior(QAbstractItemView::SelectRows);
        table->setSelectionMode(QAbstractItemView::SingleSelection);
        
        // Add tab
        QString tabLabel = QString::fromStdString(tabData.tabName) + 
                          QString(" (%1)").arg(tabData.scores.size());
        tabWidget->addTab(table, tabLabel);
    }
    
    mainLayout->addWidget(tabWidget);
    
    // Add close button
    QHBoxLayout* buttonLayout = new QHBoxLayout();
    buttonLayout->addStretch();
    QPushButton* closeBtn = new QPushButton("Close", &dialog);
    QObject::connect(closeBtn, &QPushButton::clicked, &dialog, &QDialog::accept);
    buttonLayout->addWidget(closeBtn);
    mainLayout->addLayout(buttonLayout);
    
    dialog.exec();
}

}  // namespace Qt5Helpers

