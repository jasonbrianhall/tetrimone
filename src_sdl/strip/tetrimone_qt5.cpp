#include "tetrimone_qt5.h"
#include "tetrimone_core.h"
#include "audiomanager.h"
#include <iostream>
#include <string>
#include <algorithm>
#include <chrono>
#include "highscores.h"
#include "freedom_messages.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QMenuBar>
#include <QMenu>
#include <QAction>
#include <QActionGroup>
#include <QPainter>
#include <QTimer>
#include <QKeyEvent>
#include <QMessageBox>
#include <QApplication>
#include <QScreen>
#include <QObject>

// Global variables for key repeat handling
extern bool keyDownPressed;
extern bool keyLeftPressed;
extern bool keyRightPressed;
extern int keyDownTimer;
extern int keyLeftTimer;
extern int keyRightTimer;
extern int keyDownDelay;
extern int keyLeftDelay;
extern int keyRightDelay;
extern int keyDownCount;
extern int keyLeftCount;
extern int keyRightCount;

// Global variables
extern int BLOCK_SIZE;
extern int currentThemeIndex;
extern int GRID_WIDTH;
extern int GRID_HEIGHT;

// ============================================================================
// Qt5 Game Area Widget
// ============================================================================

class GameAreaWidget : public QWidget {
public:
    explicit GameAreaWidget(TetrimoneBoard* board, QWidget* parent = nullptr)
        : QWidget(parent), board(board) {
        setMinimumSize(GRID_WIDTH * BLOCK_SIZE, GRID_HEIGHT * BLOCK_SIZE);
    }

protected:
    void paintEvent(QPaintEvent* event) override {
        if (!board) return;
        (void)event; // Mark as intentionally unused
        
        QPainter painter(this);
        painter.fillRect(rect(), Qt::black);
        
        // Draw board grid
        for (int y = 0; y < GRID_HEIGHT; y++) {
            for (int x = 0; x < GRID_WIDTH; x++) {
                int gridValue = board->getGridValue(x, y);
                if (gridValue > 0) {
                    drawBlock(&painter, x, y, gridValue);
                }
            }
        }
        
        // Draw ghost piece
        drawGhostPiece(&painter);
        
        // Draw current piece
        drawCurrentPiece(&painter);
        
        // Draw grid lines if enabled
        if (board->isShowingGridLines()) {
            drawGridLines(&painter);
        }
        
        // Draw game over screen
        if (board->isGameOver()) {
            drawGameOverScreen(&painter);
        }
        
        // Draw pause menu
        if (board->isPaused() && !board->isGameOver()) {
            drawPauseScreen(&painter);
        }
    }

private:
    TetrimoneBoard* board;

    void drawBlock(QPainter* painter, int x, int y, int type) {
        int px = x * BLOCK_SIZE;
        int py = y * BLOCK_SIZE;
        
        std::array<double, 3> color = getTetrimineColor(type);
        QColor blockColor(
            static_cast<int>(color[0] * 255),
            static_cast<int>(color[1] * 255),
            static_cast<int>(color[2] * 255)
        );
        
        painter->fillRect(px, py, BLOCK_SIZE, BLOCK_SIZE, blockColor);
        painter->drawRect(px, py, BLOCK_SIZE, BLOCK_SIZE);
    }

    void drawGhostPiece(QPainter* painter) {
        if (!board->isGhostPieceEnabled()) return;
        
        // Implementation would reference ghost piece from board
        // This is simplified - actual implementation depends on board methods
        (void)painter; // Mark as intentionally unused
    }

    void drawCurrentPiece(QPainter* painter) {
        // Implementation would draw current falling piece
        // This is simplified - actual implementation depends on board methods
        (void)painter; // Mark as intentionally unused
    }

    void drawGridLines(QPainter* painter) {
        painter->setPen(QPen(QColor(64, 64, 64), 1));
        
        for (int x = 0; x <= GRID_WIDTH; x++) {
            painter->drawLine(x * BLOCK_SIZE, 0, x * BLOCK_SIZE, height());
        }
        
        for (int y = 0; y <= GRID_HEIGHT; y++) {
            painter->drawLine(0, y * BLOCK_SIZE, width(), y * BLOCK_SIZE);
        }
    }

    void drawGameOverScreen(QPainter* painter) {
        painter->fillRect(rect(), QColor(0, 0, 0, 180));
        
        painter->setPen(Qt::white);
        QFont font = painter->font();
        font.setPointSize(24);
        painter->setFont(font);
        
        painter->drawText(rect(), Qt::AlignCenter, "GAME OVER");
    }

    void drawPauseScreen(QPainter* painter) {
        painter->fillRect(rect(), QColor(0, 0, 0, 180));
        
        painter->setPen(Qt::white);
        QFont font = painter->font();
        font.setPointSize(24);
        painter->setFont(font);
        
        painter->drawText(rect(), Qt::AlignCenter, "PAUSED");
    }

    std::array<double, 3> getTetrimineColor(int type) {
        // Returns RGB color for piece type
        const std::array<double, 3> colors[] = {
            {0.0, 1.0, 1.0},  // I - Cyan
            {1.0, 1.0, 0.0},  // O - Yellow
            {1.0, 0.0, 1.0},  // T - Magenta
            {0.0, 1.0, 0.0},  // S - Green
            {1.0, 0.0, 0.0},  // Z - Red
            {0.0, 0.0, 1.0},  // J - Blue
            {1.0, 0.5, 0.0}   // L - Orange
        };
        return colors[std::min(type - 1, 6)];
    }
};

// ============================================================================
// Qt5 Next Piece Widget
// ============================================================================

class NextPieceWidget : public QWidget {
public:
    explicit NextPieceWidget(TetrimoneBoard* board, QWidget* parent = nullptr)
        : QWidget(parent), board(board) {
        setMinimumSize(200, 200);
    }

protected:
    void paintEvent(QPaintEvent* event) override {
        if (!board) return;
        
        QPainter painter(this);
        painter.fillRect(rect(), Qt::black);
        
        painter.setPen(Qt::white);
        QFont font = painter.font();
        font.setPointSize(12);
        painter.setFont(font);
        
        painter.drawText(10, 25, "Next Piece:");
        
        // Draw next piece preview
        // Implementation depends on board methods for getting next pieces
        (void)event; // Mark as intentionally unused
    }

private:
    TetrimoneBoard* board;
};

// ============================================================================
// Core Game Logic (Platform-Agnostic)
// ============================================================================

void updateLabels(TetrimoneApp* app) {
    if (!app || !app->board) return;
    
    app->scoreLabel->setText(QString("Score: %1").arg(app->board->getScore()));
    app->levelLabel->setText(QString("Level: %1").arg(app->board->getLevel()));
    app->linesLabel->setText(QString("Lines: %1").arg(app->board->getLinesCleared()));
    
    QString difficulty;
    switch (app->difficulty) {
        case 0: difficulty = "Zen"; break;
        case 1: difficulty = "Easy"; break;
        case 2: difficulty = "Medium"; break;
        case 3: difficulty = "Hard"; break;
        case 4: difficulty = "Extreme"; break;
        default: difficulty = "Unknown"; break;
    }
    app->difficultyLabel->setText(QString("Difficulty: %1").arg(difficulty));
}

void startGame(TetrimoneApp* app) {
    if (!app || !app->board) return;
    
    app->board->restart();
    
    if (!app->backgroundMusicPlaying) {
        app->board->resumeBackgroundMusic();
        app->backgroundMusicPlaying = true;
    }
    
    if (app->board->junkLinesPercentage > 0) {
        app->board->generateJunkLines(app->board->junkLinesPercentage);
    }
    
    if (app->board->junkLinesPerLevel > 0) {
        app->board->addJunkLinesFromBottom(app->board->junkLinesPerLevel);
    }
}

void pauseGame(TetrimoneApp* app) {
    if (!app || !app->board) return;
    
    app->board->setPaused(!app->board->isPaused());
    
    if (app->board->isPaused() && app->backgroundMusicPlaying) {
        app->board->pauseBackgroundMusic();
    } else if (!app->board->isPaused() && app->backgroundMusicPlaying) {
        app->board->resumeBackgroundMusic();
    }
}

void resetUI(TetrimoneApp* app) {
    if (!app || !app->board) return;
    // UI reset is handled by Qt framework via repaint events
}

void cleanupApp(TetrimoneApp* app) {
    if (!app) return;
    
    if (app->board) {
        app->board->pauseBackgroundMusic();
        app->board->cleanupAudio();
        delete app->board;
        app->board = nullptr;
    }
    
    if (app->joystick) {
        SDL_JoystickClose(app->joystick);
        app->joystick = nullptr;
    }
    
    shutdownSDL(app);
}

void initSDL(TetrimoneApp* app) {
    if (!app) return;
    
    if (SDL_Init(SDL_INIT_JOYSTICK) < 0) {
        std::cerr << "Failed to initialize SDL for joystick: " << SDL_GetError() << std::endl;
        return;
    }
    
    int numJoysticks = SDL_NumJoysticks();
    if (numJoysticks > 0) {
        app->joystick = SDL_JoystickOpen(0);
        if (app->joystick) {
            app->joystickEnabled = true;
        }
    }
}

void shutdownSDL(TetrimoneApp* app) {
    if (!app) return;
    
    if (app->joystick) {
        SDL_JoystickClose(app->joystick);
        app->joystick = nullptr;
    }
    
    SDL_Quit();
}

void updateDisplay(TetrimoneApp* app) {
    if (!app || !app->gameArea || !app->nextPieceArea) return;
    
    // Qt5: Request repaint from the event loop
    app->gameArea->update();
    app->nextPieceArea->update();
}

// ============================================================================
// Event Handlers
// ============================================================================

void onKeyDownTick(TetrimoneApp* app) {
    if (!app || !app->board) return;
    
    if (!app->board->isPaused() && !app->board->isGameOver() && 
        !app->board->isSplashScreenActive() && keyDownPressed) {
        
        app->board->movePiece(0, 1);
        keyDownCount++;
        
        // Accelerate by decreasing the delay
        if (keyDownCount > 6) {
            keyDownDelay = 20;
        } else if (keyDownCount > 4) {
            keyDownDelay = 30;
        } else if (keyDownCount > 2) {
            keyDownDelay = 60;
        }
        
        updateDisplay(app);
        updateLabels(app);
    }
}

void onKeyLeftTick(TetrimoneApp* app) {
    if (!app || !app->board) return;
    
    if (!app->board->isPaused() && !app->board->isGameOver() && 
        !app->board->isSplashScreenActive() && keyLeftPressed) {
        
        app->board->movePiece(-1, 0);
        keyLeftCount++;
        
        if (keyLeftCount > 6) {
            keyLeftDelay = 30;
        } else if (keyLeftCount > 4) {
            keyLeftDelay = 50;
        } else if (keyLeftCount > 2) {
            keyLeftDelay = 100;
        }
        
        updateDisplay(app);
        updateLabels(app);
    }
}

void onKeyRightTick(TetrimoneApp* app) {
    if (!app || !app->board) return;
    
    if (!app->board->isPaused() && !app->board->isGameOver() && 
        !app->board->isSplashScreenActive() && keyRightPressed) {
        
        app->board->movePiece(1, 0);
        keyRightCount++;
        
        if (keyRightCount > 6) {
            keyRightDelay = 30;
        } else if (keyRightCount > 4) {
            keyRightDelay = 50;
        } else if (keyRightCount > 2) {
            keyRightDelay = 100;
        }
        
        updateDisplay(app);
        updateLabels(app);
    }
}

void onGameTick(TetrimoneApp* app) {
    if (!app || !app->board) return;
    
    if (!app->board->isPaused() && !app->board->isSplashScreenActive() && 
        !app->board->retroModeActive) {
        app->board->coolDown();
    }
    
    if (!app->board->isPaused()) {
        app->board->updateGame();
        
        if (app->board->isGameOver()) {
            if (!app->board->highScoreAlreadyProcessed) {
                app->board->highScoreAlreadyProcessed = true;
                bool isHighScore = app->board->checkAndRecordHighScore(app);
                
                if (isHighScore) {
                    app->board->playSound(GameSoundEvent::Excellent);
                }
            }
        }
        
        updateDisplay(app);
        updateLabels(app);
    }
    
    if (app->board->retroModeActive) {
        app->board->setHeatLevel(0.5f);
    }
}

// ============================================================================
// Key Event Handler - Qt5 Window Class
// ============================================================================

class TetrimoneWindow : public QWidget {
public:
    explicit TetrimoneWindow(TetrimoneApp* app, QWidget* parent = nullptr)
        : QWidget(parent), app(app) {
        setFocusPolicy(Qt::StrongFocus);
    }

protected:
    void keyPressEvent(QKeyEvent* event) override {
        if (event->isAutoRepeat()) {
            QWidget::keyPressEvent(event);
            return;
        }
        
        if (!app || !app->board) {
            QWidget::keyPressEvent(event);
            return;
        }
        
        int key = event->key();
        
        if (key == Qt::Key_Space) {
            // Hard drop
            if (!app->board->isPaused() && !app->board->isGameOver() && 
                !app->board->isSplashScreenActive()) {
                app->board->hardDrop();
                updateDisplay(app);
                updateLabels(app);
            }
        } else if (key == Qt::Key_Up || key == Qt::Key_W) {
            // Rotate clockwise
            if (!app->board->isPaused() && !app->board->isGameOver() && 
                !app->board->isSplashScreenActive()) {
                app->board->rotatePiece(true);
                updateDisplay(app);
                updateLabels(app);
            }
        } else if (key == Qt::Key_Control) {
            // Rotate counter-clockwise
            if (!app->board->isPaused() && !app->board->isGameOver() && 
                !app->board->isSplashScreenActive()) {
                app->board->rotatePiece(false);
                updateDisplay(app);
                updateLabels(app);
            }
        } else if (key == Qt::Key_Down || key == Qt::Key_S) {
            keyDownPressed = true;
            keyDownCount = 0;
            keyDownDelay = 150;
            onKeyDownTick(app);
        } else if (key == Qt::Key_Left || key == Qt::Key_A) {
            keyLeftPressed = true;
            keyLeftCount = 0;
            keyLeftDelay = 150;
            onKeyLeftTick(app);
        } else if (key == Qt::Key_Right || key == Qt::Key_D) {
            keyRightPressed = true;
            keyRightCount = 0;
            keyRightDelay = 150;
            onKeyRightTick(app);
        } else if (key == Qt::Key_P) {
            pauseGame(app);
            updateDisplay(app);
        } else {
            QWidget::keyPressEvent(event);
        }
    }

    void keyReleaseEvent(QKeyEvent* event) override {
        if (event->isAutoRepeat()) {
            QWidget::keyReleaseEvent(event);
            return;
        }
        
        int key = event->key();
        
        if (key == Qt::Key_Down || key == Qt::Key_S) {
            keyDownPressed = false;
            if (keyDownTimer) {
                // Cancel pending timer
                keyDownTimer = 0;
            }
        } else if (key == Qt::Key_Left || key == Qt::Key_A) {
            keyLeftPressed = false;
            if (keyLeftTimer) {
                keyLeftTimer = 0;
            }
        } else if (key == Qt::Key_Right || key == Qt::Key_D) {
            keyRightPressed = false;
            if (keyRightTimer) {
                keyRightTimer = 0;
            }
        } else {
            QWidget::keyReleaseEvent(event);
        }
    }

private:
    TetrimoneApp* app;
};

// ============================================================================
// Menu Actions - Qt5
// ============================================================================

void onStartGameAction(TetrimoneApp* app) {
    startGame(app);
    updateDisplay(app);
    updateLabels(app);
}

void onPauseGameAction(TetrimoneApp* app) {
    pauseGame(app);
    updateDisplay(app);
}

void onRestartGameAction(TetrimoneApp* app) {
    startGame(app);
    updateDisplay(app);
    updateLabels(app);
}

void onQuitGameAction(TetrimoneApp* app) {
    if (app->window) {
        app->window->close();
    }
}

void onSoundToggleAction(TetrimoneApp* app, bool enabled) {
    if (app->board) {
        app->board->setSoundEnabled(enabled);
    }
}

void onDifficultyChanged(TetrimoneApp* app, int difficulty) {
    app->difficulty = difficulty;
    // Note: difficulty affects level progression and speed, 
    // which is handled by the game logic, not a setDifficulty method
    updateLabels(app);
}

// ============================================================================
// Qt5 Helper Class for Signals/Slots
// ============================================================================

class GameConnector : public QObject {
    Q_OBJECT
public:
    explicit GameConnector(TetrimoneApp* app, QObject* parent = nullptr)
        : QObject(parent), app(app) {}

private slots:
    void onStartGame() { onStartGameAction(app); }
    void onPauseGame() { onPauseGameAction(app); }
    void onRestartGame() { onRestartGameAction(app); }
    void onQuitGame() { onQuitGameAction(app); }

public:
    void connect_actions(TetrimoneApp* a) {
        app = a;
    }

private:
    TetrimoneApp* app;
};

// ============================================================================
// Setup UI - Qt5
// ============================================================================

void setupMenuBar(TetrimoneApp* app) {
    QMenu* fileMenu = app->menuBar->addMenu("&File");
    
    app->startMenuItem = fileMenu->addAction("&Start Game");
    QObject::connect(app->startMenuItem, &QAction::triggered, [app]() {
        onStartGameAction(app);
    });
    
    app->pauseMenuItem = fileMenu->addAction("&Pause Game");
    QObject::connect(app->pauseMenuItem, &QAction::triggered, [app]() {
        onPauseGameAction(app);
    });
    
    app->restartMenuItem = fileMenu->addAction("&Restart Game");
    QObject::connect(app->restartMenuItem, &QAction::triggered, [app]() {
        onRestartGameAction(app);
    });
    
    fileMenu->addSeparator();
    
    QAction* quitAction = fileMenu->addAction("&Quit");
    QObject::connect(quitAction, &QAction::triggered, [app]() {
        onQuitGameAction(app);
    });
    
    // Game menu
    QMenu* gameMenu = app->menuBar->addMenu("&Game");
    
    QMenu* difficultyMenu = gameMenu->addMenu("&Difficulty");
    QActionGroup* difficultyGroup = new QActionGroup(difficultyMenu);
    
    const char* difficultyNames[] = {"Zen", "Easy", "Medium", "Hard", "Extreme"};
    for (int i = 0; i < 5; i++) {
        QAction* action = difficultyMenu->addAction(difficultyNames[i]);
        action->setCheckable(true);
        action->setActionGroup(difficultyGroup);
        if (i == 1) action->setChecked(true); // Default to Easy
        
        QObject::connect(action, &QAction::triggered, [app, i]() {
            onDifficultyChanged(app, i);
        });
    }
    
    // Sound menu
    QMenu* soundMenu = gameMenu->addMenu("&Sound");
    app->soundToggleMenuItem = soundMenu->addAction("&Enabled");
    app->soundToggleMenuItem->setCheckable(true);
    app->soundToggleMenuItem->setChecked(true);
    
    QObject::connect(app->soundToggleMenuItem, &QAction::triggered, [app](bool checked) {
        onSoundToggleAction(app, checked);
    });
    
    // View menu
    QMenu* viewMenu = app->menuBar->addMenu("&View");
    QAction* fullscreenAction = viewMenu->addAction("&Fullscreen");
    QObject::connect(fullscreenAction, &QAction::triggered, [app]() {
        if (app->window->isFullScreen()) {
            app->window->showNormal();
        } else {
            app->window->showFullScreen();
        }
    });
    
    // Help menu
    QMenu* helpMenu = app->menuBar->addMenu("&Help");
    QAction* aboutAction = helpMenu->addAction("&About");
    QObject::connect(aboutAction, &QAction::triggered, [app]() {
        QMessageBox::about(app->window, "About Tetrimone",
            "Tetrimone - A Qt5 Tetris Clone\n\n"
            "Controls:\n"
            "Arrow Keys or WASD - Move\n"
            "Space - Hard Drop\n"
            "Up/W - Rotate Clockwise\n"
            "Ctrl - Rotate Counter-Clockwise\n"
            "P - Pause/Resume");
    });
}

void setupGameUI(TetrimoneApp* app, int width, int height) {
    // Create main window
    app->window = new TetrimoneWindow(app);
    app->window->setWindowTitle("Tetrimone");
    app->window->resize(width, height);
    
    // Create menu bar
    app->menuBar = new QMenuBar(app->window);
    
    // Create main layout
    QVBoxLayout* mainLayout = new QVBoxLayout(app->window);
    mainLayout->setMenuBar(app->menuBar);
    
    // Create horizontal layout for game area and info
    QHBoxLayout* gameLayout = new QHBoxLayout();
    
    // Game area
    app->gameArea = new GameAreaWidget(app->board);
    gameLayout->addWidget(app->gameArea);
    
    // Info panel
    QVBoxLayout* infoLayout = new QVBoxLayout();
    
    app->scoreLabel = new QLabel("Score: 0");
    app->levelLabel = new QLabel("Level: 1");
    app->linesLabel = new QLabel("Lines: 0");
    app->difficultyLabel = new QLabel("Difficulty: Easy");
    
    infoLayout->addWidget(app->scoreLabel);
    infoLayout->addWidget(app->levelLabel);
    infoLayout->addWidget(app->linesLabel);
    infoLayout->addWidget(app->difficultyLabel);
    
    infoLayout->addSpacing(20);
    
    // Next piece area
    app->nextPieceArea = new NextPieceWidget(app->board);
    infoLayout->addWidget(new QLabel("Next Piece:"));
    infoLayout->addWidget(app->nextPieceArea);
    
    infoLayout->addStretch();
    
    gameLayout->addLayout(infoLayout);
    mainLayout->addLayout(gameLayout);
    
    // Setup menus
    setupMenuBar(app);
    
    // Setup game timer
    QTimer* gameTimer = new QTimer(app->window);
    QObject::connect(gameTimer, &QTimer::timeout, [app]() {
        onGameTick(app);
    });
    gameTimer->start(app->dropSpeed);
    app->timerId = gameTimer->timerId();
}

