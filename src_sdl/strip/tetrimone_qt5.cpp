#include "tetrimone_qt5.h"
#include "tetrimone_core.h"
#include "audiomanager.h"
#include <iostream>
#include <string>
#include <algorithm>
#include <chrono>
#include <cmath>
#include "highscores.h"
#include "propaganda_messages.h"
#include "freedom_messages.h"
#include "commandline.h"
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
#include <QDialog>
#include <QSlider>
#include <QSpinBox>
#include <QPushButton>
#include <QFileDialog>
#include <QDesktopServices>
#include <QUrl>
#include <QFont>
#include <QFontMetrics>
#include <QCloseEvent>
#include <SDL2/SDL.h>
#include <cairo/cairo.h>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

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

// Forward declarations
void onKeyDownTick(TetrimoneApp* app);
void onKeyLeftTick(TetrimoneApp* app);
void onKeyRightTick(TetrimoneApp* app);
void onGameTick(TetrimoneApp* app);
void updateDisplay(TetrimoneApp* app);
void pauseGame(TetrimoneApp* app);
void cleanupApp(TetrimoneApp* app);
void onStartGameAction(TetrimoneApp* app);
void onPauseGameAction(TetrimoneApp* app);
void onRestartGameAction(TetrimoneApp* app);
void onQuitGameAction(TetrimoneApp* app);
void onDifficultyChanged(TetrimoneApp* app, int difficulty);
void onSoundToggleAction(TetrimoneApp* app, bool enabled);
void updateLabels(TetrimoneApp* app);

// Forward declarations for drawing helpers
void drawFireyGlow(cairo_t* cr, double x, double y, double size, float heatLevel, double time);
void drawFreezyEffect(cairo_t* cr, double x, double y, double size, float heatLevel, double time);
LineClearAnimValues getLineClearAnimationValues(int animationType, double progress, int x, int y);
std::array<double, 3> getHeatModifiedColor(const std::array<double, 3>& baseColor, float heatLevel);

// Implementation of drawPlacedBlocks (locked/placed blocks with animations)
void drawPlacedBlocks(cairo_t *cr, TetrimoneBoard *board, TetrimoneApp *app) {
  for (int y = 0; y < GRID_HEIGHT; ++y) {
    for (int x = 0; x < GRID_WIDTH; ++x) {
      int value = board->getGridValue(x, y);
      if (value > 0) {
        // Check if this line is being cleared
        double alpha = 1.0;
        double scale = 1.0;
        double offsetX = 0.0;
        double offsetY = 0.0;
        
        if (board->isLineClearActive() && board->isLineBeingCleared(y)) {
          // Get animation progress (0.0 to 1.0)
          double progress = board->getLineClearProgress();
          
          if (board->retroModeActive) {
            // Soviet-era computer animation: Simple scan line effect
            if (progress < 0.3) {
              // Horizontal scan line sweep from left to right
              double scanProgress = progress / 0.3;
              int scanX = (int)(scanProgress * GRID_WIDTH);
              
              // Only affect blocks that have been "scanned"
              if (x <= scanX) {
                alpha = 0.3 + 0.4 * sin(progress * 20.0); // Subtle flicker
              } else {
                alpha = 1.0; // Normal until scanned
              }
              scale = 1.0;
            } else if (progress < 0.7) {
              // All blocks flash in unison (like old CRT monitors)
              double flashProgress = (progress - 0.3) / 0.4;
              alpha = 1.0 - flashProgress * 0.7;
              
              // Simulate old monitor "collapse" effect - vertical compression
              scale = 1.0;
              offsetY = flashProgress * BLOCK_SIZE * 0.3; // Slight downward compression
            } else {
              // Final "wipe" effect - blocks disappear in chunks
              double wipeProgress = (progress - 0.7) / 0.3;
              
              // Divide line into segments that disappear sequentially
              int segment = x / 3; // 3-block segments
              double segmentDelay = segment * 0.2;
              
              if (wipeProgress > segmentDelay) {
                alpha = 0.0; // Instant disappear once segment is reached
                scale = 0.0;
              } else {
                alpha = 1.0 - wipeProgress * 0.5;
                scale = 1.0;
              }
            }
          } else {
            // Modern animations - 10 different types selected randomly
            int animationType = board->getCurrentAnimationType();
            LineClearAnimValues animValues = getLineClearAnimationValues(animationType, progress, x, y);
            alpha = animValues.alpha;
            scale = animValues.scale;
            offsetX = animValues.offsetX;
            offsetY = animValues.offsetY;
          }
        }

        // Get color from tetrimoneblock colors
        auto baseColor = board->isInThemeTransition() ? 
        board->getInterpolatedColor(value - 1, board->getThemeTransitionProgress()) :
        TETRIMONEBLOCK_COLOR_THEMES[currentThemeIndex][value - 1];
        auto color = getHeatModifiedColor(baseColor, board->getHeatLevel());

        cairo_set_source_rgba(cr, color[0], color[1], color[2], alpha);

        // Calculate position with animation offsets
        double drawX = x * BLOCK_SIZE + offsetX + (BLOCK_SIZE * (1.0 - scale)) / 2;
        double drawY = y * BLOCK_SIZE + offsetY + (BLOCK_SIZE * (1.0 - scale)) / 2;
        double drawSize = BLOCK_SIZE * scale;

        if (board->retroModeActive || board->simpleBlocksActive) {
          // Simple blocks
          cairo_rectangle(cr, drawX, drawY, drawSize, drawSize);
          cairo_fill(cr);
        } else {
          // 3D blocks with scaling
          cairo_rectangle(cr, drawX + 1, drawY + 1, drawSize - 2, drawSize - 2);
          cairo_fill(cr);

          // Draw highlight (3D effect)
          cairo_set_source_rgba(cr, 1, 1, 1, 0.3 * alpha);
          cairo_move_to(cr, drawX + 1, drawY + 1);
          cairo_line_to(cr, drawX + drawSize - 1, drawY + 1);
          cairo_line_to(cr, drawX + 1, drawY + drawSize - 1);
          cairo_close_path(cr);
          cairo_fill(cr);

          // Draw shadow (3D effect)
          cairo_set_source_rgba(cr, 0, 0, 0, 0.3 * alpha);
          cairo_move_to(cr, drawX + drawSize - 1, drawY + 1);
          cairo_line_to(cr, drawX + drawSize - 1, drawY + drawSize - 1);
          cairo_line_to(cr, drawX + 1, drawY + drawSize - 1);
          cairo_close_path(cr);
          cairo_fill(cr);
        }
        
        if (!board->retroModeActive) { // Only apply effects in modern mode
          float heatLevel = board->getHeatLevel();
          
          // Get current time for animation
          auto now = std::chrono::high_resolution_clock::now();
          auto timeMs = std::chrono::duration<double, std::milli>(
              now.time_since_epoch()).count();
          
          // Draw fiery glow effect when hot
          if (heatLevel > 0.7f) {
            drawFireyGlow(cr, drawX, drawY, drawSize, heatLevel, timeMs);
            updateDisplay(app);
          }
          
          // Draw freezy effect when cold
          if (heatLevel < 0.3f) {
            drawFreezyEffect(cr, drawX, drawY, drawSize, heatLevel, timeMs);
            updateDisplay(app);
          }
        }   
      }
    }
  }
}

// ============================================================================
// Qt5 Game Area Widget with Full Rendering
// ============================================================================

class GameAreaWidget : public QWidget {
public:
    explicit GameAreaWidget(TetrimoneBoard* board, TetrimoneApp* app, QWidget* parent = nullptr)
        : QWidget(parent), board(board), app(app), backgroundPixmap(nullptr) {
        int boardWidth = GRID_WIDTH * BLOCK_SIZE;
        int boardHeight = GRID_HEIGHT * BLOCK_SIZE;
        setMinimumSize(boardWidth, boardHeight);
        setMaximumSize(boardWidth, boardHeight);
        setFixedSize(boardWidth, boardHeight);
        setFocusPolicy(Qt::StrongFocus);
    }

    ~GameAreaWidget() {
        if (backgroundPixmap) delete backgroundPixmap;
    }

protected:
    void paintEvent(QPaintEvent* event) override {
        if (!board) return;
        
        // Create SDL surface for Cairo to render to
        int w = width();
        int h = height();
        SDL_Surface* surface = SDL_CreateRGBSurface(0, w, h, 32,
            0x00FF0000, 0x0000FF00, 0x000000FF, 0xFF000000);
        
        if (!surface) return;
        
        // Create Cairo surface from SDL surface
        cairo_surface_t* cairo_surface = cairo_image_surface_create_for_data(
            (unsigned char*)surface->pixels,
            CAIRO_FORMAT_ARGB32,
            w, h,
            surface->pitch
        );
        
        cairo_t* cr = cairo_create(cairo_surface);
        
        // Draw background
        cairo_set_source_rgb(cr, 0, 0, 0);
        cairo_rectangle(cr, 0, 0, w, h);
        cairo_fill(cr);
        
        // Call ONLY the existing drawing functions from drawgame_cairo.cpp
        drawGridLines(cr, board);
        drawPlacedBlocks(cr, board, app);  // Draw locked blocks with animations
        drawCurrentPiece(cr, board);
        drawGhostPiece(cr, board);
        drawBlockTrails(cr, board);
        drawGameOver(cr, board);
        drawPauseMenu(cr, board);
        if (board->isSplashScreenActive()) {
            drawSplashScreen(cr, board, app);
        }
        drawFireworks(cr, board, app);
        
        // Convert SDL surface to QImage and display
        QImage img((uchar*)surface->pixels, w, h, surface->pitch, QImage::Format_ARGB32);
        QPainter painter(this);
        painter.drawImage(0, 0, img);
        
        // Cleanup
        cairo_destroy(cr);
        cairo_surface_destroy(cairo_surface);
        SDL_FreeSurface(surface);
        
        (void)event;
    }

    void keyPressEvent(QKeyEvent* event) override {
        if (event->isAutoRepeat()) {
            QWidget::keyPressEvent(event);
            return;
        }
        
        int key = event->key();
        
        if (key == Qt::Key_Down || key == Qt::Key_S) {
            // Down - soft drop (accelerate falling)
            keyDownPressed = true;
            keyDownCount = 0;
            keyDownDelay = 50;  // Faster falling when held
            onKeyDownTick(app);
        } else if (key == Qt::Key_Up || key == Qt::Key_W) {
            // Up - rotate piece
            if (!board->isSplashScreenActive() && !board->isPaused() && !board->isGameOver()) {
                board->rotatePiece(1);
            }
            updateDisplay(app);
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
        } else if (key == Qt::Key_Space) {
            // Spacebar - hard drop
            if (board->isSplashScreenActive()) {
                board->setSplashScreenActive(false);
                startGame(app);
            } else if (!board->isPaused() && !board->isGameOver()) {
                // Hard drop - move piece all the way down
                while (board->movePiece(0, 1)) {
                    // Keep moving down until we hit something
                }
                // Lock the piece and spawn next one
                board->lockPiece();
                board->clearLines();  // Check for and clear full lines
                board->generateNewPiece();
            }
            updateDisplay(app);
        } else if (key == Qt::Key_Z) {
            // Z for counter-clockwise rotation (alternate binding)
            if (!board->isSplashScreenActive() && !board->isPaused() && !board->isGameOver()) {
                board->rotatePiece(-1);
            }
            updateDisplay(app);
        } else if (key == Qt::Key_P) {
            pauseGame(app);
            updateDisplay(app);
        } else if (key == Qt::Key_Escape) {
            if (board->isSplashScreenActive()) {
                board->setSplashScreenActive(false);
            }
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
    TetrimoneBoard* board;
    TetrimoneApp* app;
    QPixmap* backgroundPixmap;
};

// ============================================================================
// Qt5 Next Piece Widget
// ============================================================================

class NextPieceWidget : public QWidget {
public:
    explicit NextPieceWidget(TetrimoneBoard* board, QWidget* parent = nullptr)
        : QWidget(parent), board(board) {
        setMinimumSize(150, 350);
        setMaximumSize(150, 350);
    }

protected:
    void paintEvent(QPaintEvent* event) override {
        if (!board) return;
        
        QPainter painter(this);
        painter.fillRect(rect(), Qt::black);
        
        painter.setPen(Qt::white);
        QFont font = painter.font();
        font.setPointSize(10);
        font.setBold(true);
        painter.setFont(font);
        
        painter.drawText(10, 20, "Next Pieces:");
        
        // Draw three next pieces
        int previewStartY = 40;
        int spaceBetween = 110;
        int blockSize = 20;
        int previewX = 20;
        
        int validPieceCount = 0;
        for (int pieceIndex = 0; pieceIndex < 3; pieceIndex++) {
            const TetrimoneBlock* nextBlock = board->getNextPiece(pieceIndex);
            if (!nextBlock || !nextBlock->isValid()) {
                continue;  // Skip invalid or null pieces
            }
            
            int nextType = nextBlock->getType();
            std::vector<std::vector<int>> nextShape = nextBlock->getShape();
            
            int previewY = previewStartY + (validPieceCount * spaceBetween);
            
            // Draw label
            painter.setPen(Qt::white);
            painter.setFont(font);
            std::string label = "#" + std::to_string(validPieceCount + 1);
            painter.drawText(10, previewY - 10, QString::fromStdString(label));
            
            // Draw piece
            for (size_t row = 0; row < nextShape.size(); row++) {
                for (size_t col = 0; col < nextShape[row].size(); col++) {
                    if (nextShape[row][col]) {
                        int px = previewX + col * blockSize;
                        int py = previewY + row * blockSize;
                        
                        std::array<double, 3> color = getTetrimineColor(nextType);
                        QColor blockColor(
                            static_cast<int>(color[0] * 255),
                            static_cast<int>(color[1] * 255),
                            static_cast<int>(color[2] * 255)
                        );
                        
                        painter.fillRect(px, py, blockSize, blockSize, blockColor);
                        painter.setPen(QColor(0, 0, 0, 128));
                        painter.drawRect(px, py, blockSize, blockSize);
                    }
                }
            }
            validPieceCount++;
        }

        (void)event;
    }

private:
    TetrimoneBoard* board;

    std::array<double, 3> getTetrimineColor(int type) {
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
// Key Repeat Handlers - Qt5
// ============================================================================

void onKeyDownTick(TetrimoneApp* app) {
    if (!app || !app->board) return;
    
    if (!app->board->isPaused() && !app->board->isGameOver() && 
        !app->board->isSplashScreenActive() && keyDownPressed) {
        
        app->board->movePiece(0, 1);
        keyDownCount++;
        
        if (keyDownCount > 6) {
            keyDownDelay = 20;
        } else if (keyDownCount > 4) {
            keyDownDelay = 30;
        } else if (keyDownCount > 2) {
            keyDownDelay = 60;
        }
        
        // Schedule next tick using Qt timer
        QTimer::singleShot(keyDownDelay, [app]() { onKeyDownTick(app); });
        
        updateDisplay(app);
        updateLabels(app);
    } else {
        keyDownTimer = 0;
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
        
        QTimer::singleShot(keyLeftDelay, [app]() { onKeyLeftTick(app); });
        
        updateDisplay(app);
        updateLabels(app);
    } else {
        keyLeftTimer = 0;
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
        
        QTimer::singleShot(keyRightDelay, [app]() { onKeyRightTick(app); });
        
        updateDisplay(app);
        updateLabels(app);
    } else {
        keyRightTimer = 0;
    }
}

// ============================================================================
// Game Tick Handler - Qt5
// ============================================================================

void onGameTick(TetrimoneApp* app) {
    if (!app || !app->board) return;
    
    if (!app->board->isPaused() && !app->board->isGameOver() && 
        !app->board->isSplashScreenActive()) {
        
        // Try to move piece down
        if (!app->board->movePiece(0, 1)) {
            // If move failed (hit bottom or obstacle), lock the piece and spawn next
            app->board->lockPiece();
            app->board->clearLines();  // Check for and clear full lines
            app->board->generateNewPiece();
        }
        
        if (app->board->isGameOver()) {
            // Game over logic
            if (app->board->isSoundEnabled()) {
                // Play game over sound if available
            }
        }
    }
    
    updateDisplay(app);
    updateLabels(app);
}

// ============================================================================
// Core Game Functions - Qt5
// ============================================================================

void updateLabels(TetrimoneApp* app) {
    if (!app || !app->board) return;
    
    if (app->scoreLabel) {
        app->scoreLabel->setText(QString("Score: %1").arg(app->board->getScore()));
    }
    if (app->levelLabel) {
        app->levelLabel->setText(QString("Level: %1").arg(app->board->getLevel()));
    }
    if (app->linesLabel) {
        app->linesLabel->setText(QString("Lines: %1").arg(app->board->getLinesCleared()));
    }
    
    if (app->difficultyLabel) {
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
}

void updateDisplay(TetrimoneApp* app) {
    if (app && app->gameArea) {
        app->gameArea->update();
    }
    if (app && app->nextPieceArea) {
        app->nextPieceArea->update();
    }
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
    
    updateLabels(app);
    updateDisplay(app);
}

void pauseGame(TetrimoneApp* app) {
    if (!app || !app->board) return;
    
    app->board->togglePause();
}

void resetUI(TetrimoneApp* app) {
    if (!app) return;
    
    updateLabels(app);
    updateDisplay(app);
}

void cleanupApp(TetrimoneApp* app) {
    if (!app) return;
    
    if (app->board) {
        app->board->pauseBackgroundMusic();
    }
    
    // Qt handles cleanup automatically with parent/child relationships
}

// ============================================================================
// Menu Actions - Qt5
// ============================================================================

void onStartGameAction(TetrimoneApp* app) {
    if (app->board->isSplashScreenActive()) {
        app->board->setSplashScreenActive(false);
    }
    startGame(app);
    updateDisplay(app);
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
    cleanupApp(app);
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
    // Difficulty affects level progression - note: actual difficulty logic is in game loop
    updateLabels(app);
}

// ============================================================================
// Dialog Handlers - Implemented in help.cpp
// ============================================================================
// Forward declarations - actual implementations are in help.cpp
// void onAboutDialog(void* menuItem, void* userData);
// void onInstructionsDialog(void* menuItem, void* userData);

// ============================================================================
// Qt5-specific TetrimoneWindow Class
// ============================================================================

class TetrimoneWindow : public QWidget {
public:
    explicit TetrimoneWindow(TetrimoneApp* app, QWidget* parent = nullptr)
        : QWidget(parent), app(app) {
        setWindowTitle("Tetrimone");
        setAttribute(Qt::WA_DeleteOnClose);
    }

protected:
    void closeEvent(QCloseEvent* event) override {
        cleanupApp(app);
        event->accept();
    }

    void changeEvent(QEvent* event) override {
        if (event->type() == QEvent::WindowDeactivate) {
            if (app && app->board && !app->board->isPaused()) {
                pauseGame(app);
                app->pausedByFocusLoss = true;
            }
        } else if (event->type() == QEvent::WindowActivate) {
            if (app && app->pausedByFocusLoss) {
                pauseGame(app);
                app->pausedByFocusLoss = false;
            }
        }
        QWidget::changeEvent(event);
    }

private:
    TetrimoneApp* app;
};

// ============================================================================
// Application Initialization - Qt5
// ============================================================================

void onAppActivate(TetrimoneApp* app) {
    if (!app) return;
    
    // Initialize the game UI
    setupGameUI(app, 800, 600);
    
    // Show the window
    if (app->window) {
        app->window->show();
    }
    
    // Show splash screen
    if (app->board) {
        app->board->setSplashScreenActive(true);
    }
    
    updateDisplay(app);
}

void rebuildGameUI(TetrimoneApp* app) {
    if (!app || !app->window) return;
    
    // Rebuild the game area widget with new settings
    // This is called when settings like block size or grid dimensions change
    updateDisplay(app);
}

void setupMenuBar(TetrimoneApp* app) {
    if (!app || !app->menuBar) return;
    
    // File Menu
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
    
    // Game Menu
    QMenu* gameMenu = app->menuBar->addMenu("&Game");
    
    // Difficulty submenu
    QMenu* difficultyMenu = gameMenu->addMenu("&Difficulty");
    QActionGroup* difficultyGroup = new QActionGroup(difficultyMenu);
    
    const char* difficultyNames[] = {"Zen", "Easy", "Medium", "Hard", "Extreme"};
    for (int i = 0; i < 5; i++) {
        QAction* action = difficultyMenu->addAction(difficultyNames[i]);
        action->setCheckable(true);
        action->setActionGroup(difficultyGroup);
        if (i == app->difficulty) action->setChecked(true);
        
        QObject::connect(action, &QAction::triggered, [app, i]() {
            onDifficultyChanged(app, i);
        });
    }
    
    // Sound Menu
    QMenu* soundMenu = gameMenu->addMenu("&Sound");
    app->soundToggleMenuItem = soundMenu->addAction("&Enabled");
    app->soundToggleMenuItem->setCheckable(true);
    app->soundToggleMenuItem->setChecked(true);
    
    QObject::connect(app->soundToggleMenuItem, &QAction::triggered, [app](bool checked) {
        onSoundToggleAction(app, checked);
    });
    
    // View Menu
    QMenu* viewMenu = app->menuBar->addMenu("&View");
    
    QAction* fullscreenAction = viewMenu->addAction("&Fullscreen");
    QObject::connect(fullscreenAction, &QAction::triggered, [app]() {
        if (app->window->isFullScreen()) {
            app->window->showNormal();
        } else {
            app->window->showFullScreen();
        }
    });
    
    viewMenu->addSeparator();
    
    // Ghost piece toggle
    app->backgroundToggleMenuItem = viewMenu->addAction("&Ghost Piece");
    app->backgroundToggleMenuItem->setCheckable(true);
    app->backgroundToggleMenuItem->setChecked(true);
    QObject::connect(app->backgroundToggleMenuItem, &QAction::triggered, [app](bool checked) {
        if (app->board) {
            app->board->setGhostPieceEnabled(checked);
        }
        updateDisplay(app);
    });
    
    // Grid lines toggle
    QAction* gridAction = viewMenu->addAction("&Grid Lines");
    gridAction->setCheckable(true);
    gridAction->setChecked(false);
    QObject::connect(gridAction, &QAction::triggered, [app](bool checked) {
        if (app->board) {
            app->board->setShowGridLines(checked);
        }
        updateDisplay(app);
    });
    
    // Help Menu
    QMenu* helpMenu = app->menuBar->addMenu("&Help");
    
    QAction* aboutAction = helpMenu->addAction("&About");
    QObject::connect(aboutAction, &QAction::triggered, [app]() {
        onAboutDialog(nullptr, app);
    });
    
    QAction* instructionsAction = helpMenu->addAction("&Instructions");
    QObject::connect(instructionsAction, &QAction::triggered, [app]() {
        onInstructionsDialog(nullptr, app);
    });
}

void setupGameUI(TetrimoneApp* app, int width, int height) {
    if (!app) return;
    
    // Create the game board if it doesn't exist
    if (!app->board) {
        app->board = new TetrimoneBoard();
        app->board->app = app;
    }
    
    // Create main window
    app->window = new TetrimoneWindow(app);
    app->window->setWindowTitle("Tetrimone");
    app->window->resize(width, height);
    
    // Create menu bar
    app->menuBar = new QMenuBar(app->window);
    
    // Create main layout
    QVBoxLayout* mainLayout = new QVBoxLayout(app->window);
    mainLayout->setMenuBar(app->menuBar);
    mainLayout->setContentsMargins(0, 0, 0, 0);
    
    // Create horizontal layout for game area and info
    QHBoxLayout* gameLayout = new QHBoxLayout();
    gameLayout->setSpacing(10);
    
    // Game area
    app->gameArea = new GameAreaWidget(app->board, app);
    gameLayout->addWidget(app->gameArea, 1);
    
    // Info panel
    QVBoxLayout* infoLayout = new QVBoxLayout();
    
    app->scoreLabel = new QLabel("Score: 0");
    app->levelLabel = new QLabel("Level: 1");
    app->linesLabel = new QLabel("Lines: 0");
    app->difficultyLabel = new QLabel("Difficulty: Easy");
    
    QFont labelFont;
    labelFont.setPointSize(11);
    labelFont.setBold(true);
    app->scoreLabel->setFont(labelFont);
    app->levelLabel->setFont(labelFont);
    app->linesLabel->setFont(labelFont);
    app->difficultyLabel->setFont(labelFont);
    
    infoLayout->addWidget(app->scoreLabel);
    infoLayout->addWidget(app->levelLabel);
    infoLayout->addWidget(app->linesLabel);
    infoLayout->addWidget(app->difficultyLabel);
    
    infoLayout->addSpacing(20);
    
    // Next piece label
    app->controlsHeaderLabel = new QLabel("Next Piece:");
    app->controlsHeaderLabel->setFont(labelFont);
    infoLayout->addWidget(app->controlsHeaderLabel);
    
    // Next piece area
    app->nextPieceArea = new NextPieceWidget(app->board);
    infoLayout->addWidget(app->nextPieceArea);
    
    infoLayout->addSpacing(20);
    
    // Controls info
    app->controlsLabel = new QLabel(
        "Controls:\n"
        "Left/Right - Move\n"
        "Up - Rotate\n"
        "Down - Soft Drop\n"
        "Space - Hard Drop\n"
        "P - Pause"
    );
    app->controlsLabel->setStyleSheet("QLabel { font-size: 9px; }");
    infoLayout->addWidget(app->controlsLabel);
    
    infoLayout->addStretch();
    
    gameLayout->addLayout(infoLayout, 0);
    mainLayout->addLayout(gameLayout);
    
    // Setup menus
    setupMenuBar(app);
    
    // Setup game timer for piece falling
    QTimer* gameTimer = new QTimer(app->window);
    QObject::connect(gameTimer, &QTimer::timeout, [app]() {
        onGameTick(app);
    });
    gameTimer->start(app->dropSpeed);
    app->timerId = gameTimer->timerId();
}

// ============================================================================
// Application Entry Point - Qt5
// ============================================================================

int main_qt5(int argc, char* argv[], TetrimoneApp* app) {
    QApplication qapp(argc, argv);
    
    if (!app) {
        app = new TetrimoneApp();
    }
    
    app->app = &qapp;
    app->difficulty = 1; // Easy
    app->dropSpeed = 500;
    app->backgroundMusicPlaying = false;
    
    // Initialize SDL for joystick support if needed
    SDL_Init(SDL_INIT_JOYSTICK);
    
    // Setup the game UI
    setupGameUI(app, 800, 600);
    
    // Show window and splash screen
    if (app->window) {
        app->window->show();
        if (app->board) {
            app->board->setSplashScreenActive(true);
        }
        updateDisplay(app);
    }
    
    int result = qapp.exec();
    
    // Cleanup
    cleanupApp(app);
    SDL_Quit();
    
    return result;
}

// MOC not needed - using lambda connections instead of Q_OBJECT macro
