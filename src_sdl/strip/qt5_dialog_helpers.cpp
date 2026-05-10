#include "qt5_dialog_helpers.h"
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
#include <string>
#include <vector>

namespace Qt5Helpers {

// Qt5 Implementation
class FreedomPerformanceDialog : public QDialog {
    Q_OBJECT
public:
    FreedomPerformanceDialog(QWidget* parent,
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
            
            connect(buttonGroup, QOverload<int>::of(&QButtonGroup::buttonClicked),
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
    
    int getSelectedIndex() const { return selectedIndex; }

private slots:
    void onRadioSelected(int id) {
        selectedIndex = id;
    }

private:
    QButtonGroup* buttonGroup;
    int selectedIndex;
};

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

}  // namespace Qt5Helpers

#include "qt5_dialog_helpers_moc.cpp"

