#include "tetrimone_qt5.h"
#include "tetrimone_core.h"
#include "audiomanager.h"
#include <iostream>
#include <string>
#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstring>
#include "highscores.h"
#include "qt5_dialog_helpers.h"
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
#include <QDebug>
#include <fstream>
#include <SDL2/SDL.h>
#include <cairo/cairo.h>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

// ============================================================================
// SDLCairoRenderer Implementation
// ============================================================================

SDLCairoRenderer::SDLCairoRenderer(int w, int h)
    : window(nullptr), sdlRenderer(nullptr), texture(nullptr),
      cairoSurface(nullptr), cairoContext(nullptr),
      width(w), height(h), pixelFormat(SDL_PIXELFORMAT_ARGB8888),
      pitch(0), pixelBuffer(nullptr) {}

bool SDLCairoRenderer::init(const char* title, Uint32 flags) {
    if (SDL_InitSubSystem(SDL_INIT_VIDEO) < 0) {
        std::cerr << "SDL video init failed: " << SDL_GetError() << std::endl;
        return false;
    }

    window = SDL_CreateWindow(
        title,
        SDL_WINDOWPOS_CENTERED,
        SDL_WINDOWPOS_CENTERED,
        width,
        height,
        flags
    );

    if (!window) {
        std::cerr << "Failed to create SDL window: " << SDL_GetError() << std::endl;
        SDL_QuitSubSystem(SDL_INIT_VIDEO);
        return false;
    }

    sdlRenderer = SDL_CreateRenderer(
        window, -1,
        SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC
    );

    if (!sdlRenderer) {
        std::cerr << "Failed to create SDL renderer: " << SDL_GetError() << std::endl;
        SDL_DestroyWindow(window);
        SDL_QuitSubSystem(SDL_INIT_VIDEO);
        return false;
    }

    texture = SDL_CreateTexture(
        sdlRenderer,
        pixelFormat,
        SDL_TEXTUREACCESS_STREAMING,
        width,
        height
    );

    if (!texture) {
        std::cerr << "Failed to create SDL texture: " << SDL_GetError() << std::endl;
        SDL_DestroyRenderer(sdlRenderer);
        SDL_DestroyWindow(window);
        SDL_QuitSubSystem(SDL_INIT_VIDEO);
        return false;
    }

    pitch = width * 4;
    pixelBuffer = malloc(pitch * height);

    if (!pixelBuffer) {
        std::cerr << "Failed to allocate pixel buffer" << std::endl;
        SDL_DestroyTexture(texture);
        SDL_DestroyRenderer(sdlRenderer);
        SDL_DestroyWindow(window);
        SDL_QuitSubSystem(SDL_INIT_VIDEO);
        return false;
    }

    cairoSurface = cairo_image_surface_create_for_data(
        (unsigned char*)pixelBuffer,
        CAIRO_FORMAT_ARGB32,
        width,
        height,
        pitch
    );

    if (cairo_surface_status(cairoSurface) != CAIRO_STATUS_SUCCESS) {
        std::cerr << "Failed to create cairo surface" << std::endl;
        free(pixelBuffer);
        SDL_DestroyTexture(texture);
        SDL_DestroyRenderer(sdlRenderer);
        SDL_DestroyWindow(window);
        SDL_QuitSubSystem(SDL_INIT_VIDEO);
        return false;
    }

    cairoContext = cairo_create(cairoSurface);

    if (cairo_status(cairoContext) != CAIRO_STATUS_SUCCESS) {
        std::cerr << "Failed to create cairo context" << std::endl;
        cairo_surface_destroy(cairoSurface);
        free(pixelBuffer);
        SDL_DestroyTexture(texture);
        SDL_DestroyRenderer(sdlRenderer);
        SDL_DestroyWindow(window);
        SDL_QuitSubSystem(SDL_INIT_VIDEO);
        return false;
    }

    return true;
}

void SDLCairoRenderer::clearCairoSurface(double r, double g, double b, double a) {
    cairo_set_source_rgba(cairoContext, r, g, b, a);
    cairo_paint(cairoContext);
}

void SDLCairoRenderer::syncSurfaceToTexture() {
    if (!texture || !pixelBuffer) return;
    cairo_surface_flush(cairoSurface);
    SDL_UpdateTexture(texture, nullptr, pixelBuffer, pitch);
}

void SDLCairoRenderer::present() {
    if (!sdlRenderer) return;
    SDL_SetRenderDrawColor(sdlRenderer, 0, 0, 0, 255);
    SDL_RenderClear(sdlRenderer);
    SDL_RenderCopy(sdlRenderer, texture, nullptr, nullptr);
    SDL_RenderPresent(sdlRenderer);
}

bool SDLCairoRenderer::resize(int newWidth, int newHeight) {
    if (newWidth <= 0 || newHeight <= 0) return false;

    if (cairoContext) {
        cairo_destroy(cairoContext);
        cairoContext = nullptr;
    }
    if (cairoSurface) {
        cairo_surface_destroy(cairoSurface);
        cairoSurface = nullptr;
    }
    if (pixelBuffer) {
        free(pixelBuffer);
        pixelBuffer = nullptr;
    }
    if (texture) {
        SDL_DestroyTexture(texture);
        texture = nullptr;
    }

    width = newWidth;
    height = newHeight;

    SDL_SetWindowSize(window, width, height);

    texture = SDL_CreateTexture(
        sdlRenderer,
        pixelFormat,
        SDL_TEXTUREACCESS_STREAMING,
        width,
        height
    );

    if (!texture) {
        std::cerr << "Failed to recreate texture" << std::endl;
        return false;
    }

    pitch = width * 4;
    pixelBuffer = malloc(pitch * height);

    if (!pixelBuffer) {
        SDL_DestroyTexture(texture);
        texture = nullptr;
        return false;
    }

    cairoSurface = cairo_image_surface_create_for_data(
        (unsigned char*)pixelBuffer,
        CAIRO_FORMAT_ARGB32,
        width,
        height,
        pitch
    );

    if (cairo_surface_status(cairoSurface) != CAIRO_STATUS_SUCCESS) {
        free(pixelBuffer);
        pixelBuffer = nullptr;
        return false;
    }

    cairoContext = cairo_create(cairoSurface);

    if (cairo_status(cairoContext) != CAIRO_STATUS_SUCCESS) {
        cairo_surface_destroy(cairoSurface);
        free(pixelBuffer);
        pixelBuffer = nullptr;
        return false;
    }

    return true;
}

bool SDLCairoRenderer::saveFrameToPNG(const char* filename) {
    if (!cairoSurface) return false;
    return cairo_surface_write_to_png(cairoSurface, filename) == CAIRO_STATUS_SUCCESS;
}

void SDLCairoRenderer::cleanup() {
    if (cairoContext) {
        cairo_destroy(cairoContext);
        cairoContext = nullptr;
    }
    if (cairoSurface) {
        cairo_surface_destroy(cairoSurface);
        cairoSurface = nullptr;
    }
    if (pixelBuffer) {
        free(pixelBuffer);
        pixelBuffer = nullptr;
    }
    if (texture) {
        SDL_DestroyTexture(texture);
        texture = nullptr;
    }
    if (sdlRenderer) {
        SDL_DestroyRenderer(sdlRenderer);
        sdlRenderer = nullptr;
    }
    if (window) {
        SDL_DestroyWindow(window);
        window = nullptr;
    }
    SDL_QuitSubSystem(SDL_INIT_VIDEO);
}

SDLCairoRenderer::~SDLCairoRenderer() {
    cleanup();
}

// ============================================================================
// Global variables for key repeat handling
// ============================================================================

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

void drawFireyGlow(cairo_t* cr, double x, double y, double size, float heatLevel, double time);
void drawFreezyEffect(cairo_t* cr, double x, double y, double size, float heatLevel, double time);
LineClearAnimValues getLineClearAnimationValues(int animationType, double progress, int x, int y);
std::array<double, 3> getHeatModifiedColor(const std::array<double, 3>& baseColor, float heatLevel);

// ============================================================================
// Implementation of drawPlacedBlocks (locked/placed blocks with animations)
// ============================================================================

void drawPlacedBlocks(cairo_t *cr, TetrimoneBoard *board, TetrimoneApp *app) {
  for (int y = 0; y < GRID_HEIGHT; ++y) {
    for (int x = 0; x < GRID_WIDTH; ++x) {
      int value = board->getGridValue(x, y);
      if (value > 0) {
        double alpha = 1.0;
        double scale = 1.0;
        double offsetX = 0.0;
        double offsetY = 0.0;
        
        if (board->isLineClearActive() && board->isLineBeingCleared(y)) {
          double progress = board->getLineClearProgress();
          
          if (board->retroModeActive) {
            if (progress < 0.3) {
              double scanProgress = progress / 0.3;
              int scanX = (int)(scanProgress * GRID_WIDTH);
              if (x <= scanX) {
                alpha = 0.3 + 0.4 * sin(progress * 20.0);
              } else {
                alpha = 1.0;
              }
              scale = 1.0;
            } else if (progress < 0.7) {
              double flashProgress = (progress - 0.3) / 0.4;
              alpha = 1.0 - flashProgress * 0.7;
              scale = 1.0;
              offsetY = flashProgress * BLOCK_SIZE * 0.3;
            } else {
              double wipeProgress = (progress - 0.7) / 0.3;
              int segment = x / 3;
              double segmentDelay = segment * 0.2;
              
              if (wipeProgress > segmentDelay) {
                alpha = 0.0;
                scale = 0.0;
              } else {
                alpha = 1.0 - wipeProgress * 0.5;
                scale = 1.0;
              }
            }
          } else {
            int animationType = board->getCurrentAnimationType();
            LineClearAnimValues animValues = getLineClearAnimationValues(animationType, progress, x, y);
            alpha = animValues.alpha;
            scale = animValues.scale;
            offsetX = animValues.offsetX;
            offsetY = animValues.offsetY;
          }
        }

        auto baseColor = board->isInThemeTransition() ? 
        board->getInterpolatedColor(value - 1, board->getThemeTransitionProgress()) :
        TETRIMONEBLOCK_COLOR_THEMES[currentThemeIndex][value - 1];
        auto color = getHeatModifiedColor(baseColor, board->getHeatLevel());

        cairo_set_source_rgba(cr, color[0], color[1], color[2], alpha);

        double drawX = x * BLOCK_SIZE + offsetX + (BLOCK_SIZE * (1.0 - scale)) / 2;
        double drawY = y * BLOCK_SIZE + offsetY + (BLOCK_SIZE * (1.0 - scale)) / 2;
        double drawSize = BLOCK_SIZE * scale;

        if (board->retroModeActive || board->simpleBlocksActive) {
          cairo_rectangle(cr, drawX, drawY, drawSize, drawSize);
          cairo_fill(cr);
        } else {
          cairo_rectangle(cr, drawX + 1, drawY + 1, drawSize - 2, drawSize - 2);
          cairo_fill(cr);

          cairo_set_source_rgba(cr, 1, 1, 1, 0.3 * alpha);
          cairo_move_to(cr, drawX + 1, drawY + 1);
          cairo_line_to(cr, drawX + drawSize - 1, drawY + 1);
          cairo_line_to(cr, drawX + 1, drawY + drawSize - 1);
          cairo_close_path(cr);
          cairo_fill(cr);

          cairo_set_source_rgba(cr, 0, 0, 0, 0.3 * alpha);
          cairo_move_to(cr, drawX + drawSize - 1, drawY + 1);
          cairo_line_to(cr, drawX + drawSize - 1, drawY + drawSize - 1);
          cairo_line_to(cr, drawX + 1, drawY + drawSize - 1);
          cairo_close_path(cr);
          cairo_fill(cr);
        }
        
        if (!board->retroModeActive) {
          float heatLevel = board->getHeatLevel();
          auto now = std::chrono::high_resolution_clock::now();
          auto timeMs = std::chrono::duration<double, std::milli>(
              now.time_since_epoch()).count();
          
          if (heatLevel > 0.7f) {
            drawFireyGlow(cr, drawX, drawY, drawSize, heatLevel, timeMs);
          }
          
          if (heatLevel < 0.3f) {
            drawFreezyEffect(cr, drawX, drawY, drawSize, heatLevel, timeMs);
          }
        }   
      }
    }
  }
}

// ============================================================================
// Qt5 Game Area Widget with Full SDL Rendering (Dynamic Scaling)
// ============================================================================

class GameAreaWidget : public QWidget {
public:
    explicit GameAreaWidget(TetrimoneBoard* board, TetrimoneApp* app, QWidget* parent = nullptr)
        : QWidget(parent), board(board), app(app), backgroundPixmap(nullptr) {
        setFocusPolicy(Qt::StrongFocus);
        setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
        setMinimumSize(280, 560);
        setMaximumSize(500, 1200);  // Larger to stretch and fill screen
    }

    ~GameAreaWidget() {
        if (backgroundPixmap) delete backgroundPixmap;
    }

    QSize sizeHint() const override {
        // Will expand to fill available space
        return QSize(400, 880);
    }

protected:
    void paintEvent(QPaintEvent* event) override {
        if (!board || !app) return;
        
        int w = width();
        int h = height();
        
        if (w <= 0 || h <= 0) return;
        
        // Calculate scaling factor - make blocks fill the HEIGHT (22 blocks)
        int blockSize = h / 22;  // Fill the full height with 22 blocks
        
        double scale = blockSize / 30.0;  // 30 is the base block size
        
        // Use GPU renderer if available
        if (app->sdlCairoRenderer) {
            cairo_t* cr = app->sdlCairoRenderer->getCairoContext();
            
            // Draw background image with transition support
            if (board->useBackgroundImage && board->getBackgroundImage() != nullptr) {
                cairo_surface_t* bgImage = (cairo_surface_t*)board->getBackgroundImage();
                
                // Draw old background during transition (fade out)
                if (board->isInBackgroundTransition() && board->getOldBackground() != nullptr) {
                    cairo_surface_t* oldBg = (cairo_surface_t*)board->getOldBackground();
                    cairo_set_source_surface(cr, oldBg, 0, 0);
                    cairo_paint_with_alpha(cr, board->getTransitionOpacity());
                    
                    // Draw new background on top (fade in)
                    cairo_set_source_surface(cr, bgImage, 0, 0);
                    cairo_paint_with_alpha(cr, 1.0 - board->getTransitionOpacity());
                } else {
                    // Normal drawing without transition
                    cairo_set_source_surface(cr, bgImage, 0, 0);
                    cairo_paint(cr);
                }
            } else {
                // Fallback to black background
                app->sdlCairoRenderer->clearCairoSurface(0, 0, 0, 1.0);
            }
            
            if (cr) {
                cairo_scale(cr, scale, scale);
                
                drawGridLines(cr, board);
                drawPlacedBlocks(cr, board, app);
                drawCurrentPiece(cr, board);
                drawGhostPiece(cr, board);
                drawBlockTrails(cr, board);
                drawGameOver(cr, board);
                drawPauseMenu(cr, board);
                if (board->isSplashScreenActive()) {
                    drawSplashScreen(cr, board, app);
                }
                drawFireworks(cr, board, app);
                drawPropagandaMessage(cr, board);
                
                app->sdlCairoRenderer->syncSurfaceToTexture();
                app->sdlCairoRenderer->present();
            }
        } else {
            // Fallback: CPU rendering with Qt
            SDL_Surface* surface = SDL_CreateRGBSurface(0, w, h, 32,
                0x00FF0000, 0x0000FF00, 0x000000FF, 0xFF000000);
            
            if (!surface) return;
            
            cairo_surface_t* cairo_surface = cairo_image_surface_create_for_data(
                (unsigned char*)surface->pixels,
                CAIRO_FORMAT_ARGB32,
                w, h,
                surface->pitch
            );
            
            cairo_t* cr = cairo_create(cairo_surface);
            
            // Draw background image with transition support
            if (board->useBackgroundImage && board->getBackgroundImage() != nullptr) {
                cairo_surface_t* bgImage = (cairo_surface_t*)board->getBackgroundImage();
                
                // Draw old background during transition (fade out)
                if (board->isInBackgroundTransition() && board->getOldBackground() != nullptr) {
                    cairo_surface_t* oldBg = (cairo_surface_t*)board->getOldBackground();
                    cairo_set_source_surface(cr, oldBg, 0, 0);
                    cairo_paint_with_alpha(cr, board->getTransitionOpacity());
                    
                    // Draw new background on top (fade in)
                    cairo_set_source_surface(cr, bgImage, 0, 0);
                    cairo_paint_with_alpha(cr, 1.0 - board->getTransitionOpacity());
                } else {
                    // Normal drawing without transition
                    cairo_set_source_surface(cr, bgImage, 0, 0);
                    cairo_paint(cr);
                }
            } else {
                // Fallback to black background
                cairo_set_source_rgb(cr, 0, 0, 0);
                cairo_rectangle(cr, 0, 0, w, h);
                cairo_fill(cr);
            }
            
            cairo_scale(cr, scale, scale);
            
            drawGridLines(cr, board);
            drawPlacedBlocks(cr, board, app);
            drawCurrentPiece(cr, board);
            drawGhostPiece(cr, board);
            drawBlockTrails(cr, board);
            drawGameOver(cr, board);
            drawPauseMenu(cr, board);
            if (board->isSplashScreenActive()) {
                drawSplashScreen(cr, board, app);
            }
            drawFireworks(cr, board, app);
            drawPropagandaMessage(cr, board);
            
            QImage img((uchar*)surface->pixels, w, h, surface->pitch, QImage::Format_ARGB32);
            QPainter painter(this);
            painter.drawImage(0, 0, img);
            
            cairo_destroy(cr);
            cairo_surface_destroy(cairo_surface);
            SDL_FreeSurface(surface);
        }
        
        (void)event;
    }

    void resizeEvent(QResizeEvent* event) override {
        QWidget::resizeEvent(event);
        update();
    }

    void keyPressEvent(QKeyEvent* event) override {
        if (event->isAutoRepeat()) {
            QWidget::keyPressEvent(event);
            return;
        }
        
        int key = event->key();
        
        if (key == Qt::Key_Down || key == Qt::Key_S) {
            keyDownPressed = true;
            keyDownCount = 0;
            keyDownDelay = 50;
            onKeyDownTick(app);
        } else if (key == Qt::Key_Up || key == Qt::Key_W) {
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
            if (board->isSplashScreenActive()) {
                board->setSplashScreenActive(false);
                startGame(app);
            } else if (!board->isPaused() && !board->isGameOver()) {
                while (board->movePiece(0, 1)) {
                }
                board->lockPiece();
                board->clearLines();
                board->generateNewPiece();
            }
            updateDisplay(app);
        } else if (key == Qt::Key_Z) {
            if (!board->isSplashScreenActive() && !board->isPaused() && !board->isGameOver()) {
                board->rotatePiece(-1);
            }
            updateDisplay(app);
        } else if (key == Qt::Key_P) {
            pauseGame(app);
            updateDisplay(app);
        } else if (key == Qt::Key_R) {
            // Restart game on 'r' key
            if (board->isGameOver()) {
                onRestartGameAction(app);
                updateDisplay(app);
            }
        } else if (key == Qt::Key_Comma) {
            // Toggle retro/propaganda mode with comma key
            board->retroModeActive = !board->retroModeActive;
            board->patrioticModeActive = false;
            if (board->retroModeActive) {
                std::cout << "✓ Retro mode ACTIVATED - БЛОЧНАЯ РЕВОЛЮЦИЯ" << std::endl;
                app->window->setWindowTitle("БЛОЧНАЯ РЕВОЛЮЦИЯ");
                board->setHeatLevel(0.5);  // Set heat level for retro mode
                // Switch to Soviet Retro theme (last theme in the list)
                extern int currentThemeIndex;
                extern const size_t NUM_COLOR_THEMES;
                currentThemeIndex = NUM_COLOR_THEMES - 1;
            } else {
                std::cout << "✓ Retro mode deactivated" << std::endl;
                app->window->setWindowTitle("Tetrimone");
                board->setHeatLevel(0.0);  // Reset heat level
            }
            updateDisplay(app);
        } else if (key == Qt::Key_Period) {
            // Toggle patriot mode with period key
            board->patrioticModeActive = !board->patrioticModeActive;
            board->retroModeActive = false;
            if (board->patrioticModeActive) {
                std::cout << "✓ Patriot mode ACTIVATED - GOD BLESS AMERICA" << std::endl;
                app->window->setWindowTitle("FREEDOM BLOCKS - GOD BLESS AMERICA");
                // Switch to American Patriotic theme (second-to-last theme in the list)
                extern int currentThemeIndex;
                extern const size_t NUM_COLOR_THEMES;
                currentThemeIndex = NUM_COLOR_THEMES - 2;
            } else {
                std::cout << "✓ Patriot mode deactivated" << std::endl;
                app->window->setWindowTitle("Tetrimone");
            }
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
// Qt5 Next Piece Widget with SDL Rendering (3D Blocks)
// ============================================================================

class NextPieceWidget : public QWidget {
public:
    explicit NextPieceWidget(TetrimoneBoard* board, TetrimoneApp* app, QWidget* parent = nullptr)
        : QWidget(parent), board(board), app(app) {
        setMinimumSize(150, 350);
        setMaximumSize(150, 350);
    }

protected:
    void paintEvent(QPaintEvent* event) override {
        if (!board) return;
        
        int w = width();
        int h = height();
        
        // Use GPU renderer if available
        if (app && app->sdlCairoRenderer) {
            app->sdlCairoRenderer->clearCairoSurface(0.15, 0.15, 0.2, 1.0);
            cairo_t* cr = app->sdlCairoRenderer->getCairoContext();
            
            // Draw title
            cairo_set_source_rgb(cr, 1, 1, 1);
            cairo_select_font_face(cr, "monospace", CAIRO_FONT_SLANT_NORMAL, CAIRO_FONT_WEIGHT_BOLD);
            cairo_set_font_size(cr, 16);
            cairo_move_to(cr, 10, 25);
            cairo_show_text(cr, "Next:");
            
            int previewStartY = 50;
            int spaceBetween = 80;
            int blockSize = 15;
            int previewX = 25;
            
            int validPieceCount = 0;
            for (int pieceIndex = 0; pieceIndex < 3; pieceIndex++) {
                const TetrimoneBlock* nextBlock = board->getNextPiece(pieceIndex);
                if (!nextBlock || !nextBlock->isValid()) {
                    continue;
                }
                
                int nextType = nextBlock->getType();
                std::vector<std::vector<int>> nextShape = nextBlock->getShape();
                
                int previewY = previewStartY + (validPieceCount * spaceBetween);
                
                // Draw label
                cairo_set_source_rgb(cr, 0.6, 0.6, 0.6);
                cairo_set_font_size(cr, 10);
                std::string label = "#" + std::to_string(validPieceCount + 1);
                cairo_move_to(cr, 10, previewY + 5);
                cairo_show_text(cr, label.c_str());
                
                // Draw piece with 3D effect
                for (size_t row = 0; row < nextShape.size(); row++) {
                    for (size_t col = 0; col < nextShape[row].size(); col++) {
                        if (nextShape[row][col]) {
                            int px = previewX + col * blockSize;
                            int py = previewY + row * blockSize;
                            
                            std::array<double, 3> color = getTetrimineColor(nextType);
                            
                            // Main block
                            cairo_set_source_rgb(cr, color[0], color[1], color[2]);
                            cairo_rectangle(cr, px + 1, py + 1, blockSize - 2, blockSize - 2);
                            cairo_fill(cr);
                            
                            // 3D highlight
                            cairo_set_source_rgba(cr, 1, 1, 1, 0.3);
                            cairo_move_to(cr, px + 1, py + 1);
                            cairo_line_to(cr, px + blockSize - 1, py + 1);
                            cairo_line_to(cr, px + 1, py + blockSize - 1);
                            cairo_close_path(cr);
                            cairo_fill(cr);
                            
                            // 3D shadow
                            cairo_set_source_rgba(cr, 0, 0, 0, 0.3);
                            cairo_move_to(cr, px + blockSize - 1, py + 1);
                            cairo_line_to(cr, px + blockSize - 1, py + blockSize - 1);
                            cairo_line_to(cr, px + 1, py + blockSize - 1);
                            cairo_close_path(cr);
                            cairo_fill(cr);
                        }
                    }
                }
                validPieceCount++;
            }
            
            app->sdlCairoRenderer->syncSurfaceToTexture();
            app->sdlCairoRenderer->present();
        } else {
            // Fallback: Qt rendering
            SDL_Surface* surface = SDL_CreateRGBSurface(0, w, h, 32,
                0x00FF0000, 0x0000FF00, 0x000000FF, 0xFF000000);
            
            if (!surface) return;
            
            cairo_surface_t* cairo_surface = cairo_image_surface_create_for_data(
                (unsigned char*)surface->pixels,
                CAIRO_FORMAT_ARGB32,
                w, h,
                surface->pitch
            );
            
            cairo_t* cr = cairo_create(cairo_surface);
            
            cairo_set_source_rgb(cr, 0.15, 0.15, 0.2);
            cairo_rectangle(cr, 0, 0, w, h);
            cairo_fill(cr);
            
            cairo_set_source_rgb(cr, 1, 1, 1);
            cairo_select_font_face(cr, "monospace", CAIRO_FONT_SLANT_NORMAL, CAIRO_FONT_WEIGHT_BOLD);
            cairo_set_font_size(cr, 16);
            cairo_move_to(cr, 10, 25);
            cairo_show_text(cr, "Next:");
            
            int previewStartY = 50;
            int spaceBetween = 80;
            int blockSize = 15;
            int previewX = 25;
            
            int validPieceCount = 0;
            for (int pieceIndex = 0; pieceIndex < 3; pieceIndex++) {
                const TetrimoneBlock* nextBlock = board->getNextPiece(pieceIndex);
                if (!nextBlock || !nextBlock->isValid()) {
                    continue;
                }
                
                int nextType = nextBlock->getType();
                std::vector<std::vector<int>> nextShape = nextBlock->getShape();
                
                int previewY = previewStartY + (validPieceCount * spaceBetween);
                
                cairo_set_source_rgb(cr, 0.6, 0.6, 0.6);
                cairo_set_font_size(cr, 10);
                std::string label = "#" + std::to_string(validPieceCount + 1);
                cairo_move_to(cr, 10, previewY + 5);
                cairo_show_text(cr, label.c_str());
                
                for (size_t row = 0; row < nextShape.size(); row++) {
                    for (size_t col = 0; col < nextShape[row].size(); col++) {
                        if (nextShape[row][col]) {
                            int px = previewX + col * blockSize;
                            int py = previewY + row * blockSize;
                            
                            std::array<double, 3> color = getTetrimineColor(nextType);
                            
                            cairo_set_source_rgb(cr, color[0], color[1], color[2]);
                            cairo_rectangle(cr, px + 1, py + 1, blockSize - 2, blockSize - 2);
                            cairo_fill(cr);
                            
                            cairo_set_source_rgba(cr, 1, 1, 1, 0.3);
                            cairo_move_to(cr, px + 1, py + 1);
                            cairo_line_to(cr, px + blockSize - 1, py + 1);
                            cairo_line_to(cr, px + 1, py + blockSize - 1);
                            cairo_close_path(cr);
                            cairo_fill(cr);
                            
                            cairo_set_source_rgba(cr, 0, 0, 0, 0.3);
                            cairo_move_to(cr, px + blockSize - 1, py + 1);
                            cairo_line_to(cr, px + blockSize - 1, py + blockSize - 1);
                            cairo_line_to(cr, px + 1, py + blockSize - 1);
                            cairo_close_path(cr);
                            cairo_fill(cr);
                        }
                    }
                }
                validPieceCount++;
            }
            
            QImage img((uchar*)surface->pixels, w, h, surface->pitch, QImage::Format_ARGB32);
            QPainter painter(this);
            painter.drawImage(0, 0, img);
            
            cairo_destroy(cr);
            cairo_surface_destroy(cairo_surface);
            SDL_FreeSurface(surface);
        }
        
        (void)event;
    }

private:
    TetrimoneBoard* board;
    TetrimoneApp* app;

    std::array<double, 3> getTetrimineColor(int type) {
        const std::array<double, 3> colors[] = {
            {0.0, 1.0, 1.0},
            {1.0, 1.0, 0.0},
            {1.0, 0.0, 1.0},
            {0.0, 1.0, 0.0},
            {1.0, 0.0, 0.0},
            {0.0, 0.0, 1.0},
            {1.0, 0.5, 0.0}
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
    
    // Update background transition if active
    if (app->board->isInBackgroundTransition()) {
        app->board->updateBackgroundTransition();
    }
    
    if (!app->board->isPaused() && !app->board->isGameOver() && 
        !app->board->isSplashScreenActive()) {
        
        if (!app->board->movePiece(0, 1)) {
            app->board->lockPiece();
            int linesCleared = app->board->clearLines();
            app->board->generateNewPiece();
            
            // Show propaganda/freedom messages when lines are cleared
            if (linesCleared > 0) {
                if (app->board->retroModeActive) {
                    QTimer::singleShot(1500, [app]() {
                        showIdeologicalFailureDialog(app);
                    });
                }
                if (app->board->patrioticModeActive) {
                    QTimer::singleShot(1500, [app]() {
                        showPatrioticPerformanceDialog(app);
                    });
                }
            }
        }
        
        // Random KGB inspection during retro mode
        if (!app->board->isPaused() && !app->board->isGameOver() && app->board->retroModeActive) {
            static std::mt19937 rng(std::chrono::system_clock::now().time_since_epoch().count());
            std::uniform_int_distribution<int> dist(1, 1000);
            
            if (dist(rng) == 1) {
                app->board->setPaused(true);
                
                QMessageBox* dialog = new QMessageBox(QMessageBox::Warning,
                    "КГБ ИНСПЕКЦИЯ",
                    "КГБ ИНСПЕКЦИЯ В ПРОЦЕССЕ...\n(KGB INSPECTION IN PROGRESS...)",
                    QMessageBox::NoButton,
                    qobject_cast<QWidget*>(app->window));
                dialog->show();
                
                QTimer::singleShot(2000, [dialog]() {
                    dialog->close();
                    dialog->deleteLater();
                });
                
                QTimer::singleShot(2100, [app]() {
                    app->board->setPaused(false);
                });
            }
        }
        
        // Random freedom inspection during patriotic mode
        if (!app->board->isPaused() && !app->board->isGameOver() && app->board->patrioticModeActive) {
            static std::mt19937 rng(std::chrono::system_clock::now().time_since_epoch().count());
            std::uniform_int_distribution<int> dist(1, 1776);
            
            if (dist(rng) == 1) {
                app->board->setPaused(true);
                
                const char* freedomInspections[] = {
                    "🇺🇸 FREEDOM INSPECTION IN PROGRESS! 🦅\n(Checking your liberty levels...)",
                    "📺 COMMERCIAL BREAK! 🍔\n(This freedom brought to you by sponsors!)",
                    "🏈 TOUCHDOWN! AMERICA SCORES! 🎯\n(Brief patriotic celebration pause!)",
                    "☕ COFFEE BREAK TIME! ⏰\n(Even freedom fighters need caffeine!)",
                    "📱 SOCIAL MEDIA NOTIFICATION! 💬\n(Someone liked your freedom post!)",
                    "🛒 FLASH SALE ALERT! 💳\n(50% off freedom accessories!)",
                    "🎬 MOVIE TRAILER PREVIEW! 🍿\n(Coming soon: BLOCKS 2: FREEDOM EDITION!)",
                    "🚗 TRAFFIC UPDATE! 🛣️\n(Highway to freedom temporarily slowed!)",
                    "🌮 FOOD TRUCK ALERT! 🚚\n(Taco Tuesday freedom fuel available!)",
                    "📺 BREAKING NEWS! 📰\n(Local gamer achieves blocks and liberty!)"
                };
                
                std::uniform_int_distribution<int> msgDist(0, 9);
                const char* selectedMessage = freedomInspections[msgDist(rng)];
                
                QMessageBox* dialog = new QMessageBox(QMessageBox::Information,
                    "FREEDOM INSPECTION",
                    selectedMessage,
                    QMessageBox::NoButton,
                    qobject_cast<QWidget*>(app->window));
                dialog->show();
                
                QTimer::singleShot(2500, [dialog]() {
                    dialog->close();
                    dialog->deleteLater();
                });
                
                QTimer::singleShot(2600, [app]() {
                    app->board->setPaused(false);
                });
            }
        }
        
        if (app->board->isGameOver()) {
            if (app->board->isSoundEnabled()) {
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
    
    // Update score label with Soviet-style text in retro mode
    QString scoreText;
    if (app->board->retroModeActive) {
        scoreText = QString("Партийная Лояльность: %1%").arg(app->board->getScore());
    } else {
        scoreText = QString("Score: %1").arg(app->board->getScore());
    }
    if (app->scoreLabel) {
        app->scoreLabel->setText(scoreText);
    }
    
    // Update level label with Soviet-style text in retro mode
    QString levelText;
    if (app->board->retroModeActive) {
        levelText = QString("Пятилетка: %1").arg(app->board->getLevel());
    } else {
        levelText = QString("Level: %1").arg(app->board->getLevel());
    }
    if (app->levelLabel) {
        app->levelLabel->setText(levelText);
    }
    
    // Update lines label with Soviet propaganda in retro mode
    QString linesText;
    if (app->board->retroModeActive) {
        linesText = QString("Уничтожено врагов народа: %1").arg(app->board->getLinesCleared());
    } else {
        linesText = QString("Lines: %1").arg(app->board->getLinesCleared());
    }
    if (app->linesLabel) {
        app->linesLabel->setText(linesText);
    }
    
    // Update sequence label
    QString sequenceText;
    if (app->board->isSequenceActive() && app->board->getConsecutiveClears() > 1) {
        if (app->board->retroModeActive) {
            sequenceText = QString("Коллективная эффективность: %1 (Рекорд: %2)")
                .arg(app->board->getConsecutiveClears())
                .arg(app->board->getMaxConsecutiveClears());
        } else {
            sequenceText = QString("Sequence: %1 (Max: %2)")
                .arg(app->board->getConsecutiveClears())
                .arg(app->board->getMaxConsecutiveClears());
        }
    } else {
        if (app->board->retroModeActive) {
            sequenceText = QString("Коллективная эффективность: %1 (Рекорд: %2)")
                .arg(app->board->getConsecutiveClears())
                .arg(app->board->getMaxConsecutiveClears());
        } else {
            sequenceText = QString("Sequence: %1 (Max: %2)")
                .arg(app->board->getConsecutiveClears())
                .arg(app->board->getMaxConsecutiveClears());
        }
    }
    if (app->sequenceLabel) {
        app->sequenceLabel->setText(sequenceText);
    }
    
    // Update difficulty label
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
    
    // Update controls label with all stats and controls like GTK3
    if (app->controlsLabel) {
        QString difficulty;
        switch (app->difficulty) {
            case 0: difficulty = "Zen"; break;
            case 1: difficulty = "Easy"; break;
            case 2: difficulty = "Medium"; break;
            case 3: difficulty = "Hard"; break;
            case 4: difficulty = "Extreme"; break;
            default: difficulty = "Unknown"; break;
        }
        
        QString scoreText;
        if (app->board->retroModeActive) {
            scoreText = QString("Партийная Лояльность: %1%").arg(app->board->getScore());
        } else {
            scoreText = QString("Score: %1").arg(app->board->getScore());
        }
        
        QString levelText;
        if (app->board->retroModeActive) {
            levelText = QString("Пятилетка: %1").arg(app->board->getLevel());
        } else {
            levelText = QString("Level: %1").arg(app->board->getLevel());
        }
        
        QString linesText;
        if (app->board->retroModeActive) {
            linesText = QString("Уничтожено врагов народа: %1").arg(app->board->getLinesCleared());
        } else {
            linesText = QString("Lines: %1").arg(app->board->getLinesCleared());
        }
        
        QString sequenceText;
        if (app->board->retroModeActive) {
            sequenceText = QString("Коллективная эффективность: %1 (Рекорд: %2)")
                .arg(app->board->getConsecutiveClears())
                .arg(app->board->getMaxConsecutiveClears());
        } else {
            sequenceText = QString("Sequence: %1 (Max: %2)")
                .arg(app->board->getConsecutiveClears())
                .arg(app->board->getMaxConsecutiveClears());
        }
        
        QString controlsText;
        if (app->board->retroModeActive) {
            controlsText = QString(
                "%1\n"
                "%2\n"
                "%3\n"
                "%4\n"
                "Партийные Директивы: %5\n\n"
                "Управление клавиатурой:\n"
                "• Влево/Вправо/A/D: Переместить блок\n"
                "• Вверх/W: Повернуть по часовой\n"
                "• Z: Повернуть против часовой\n"
                "• Вниз/S: Мягкое падение\n"
                "• Пробел: Жёсткое падение\n"
                "• P: Пауза/Возобновить\n"
                "• R: Перезагрузить игру\n"
                "• M: Включить музыку\n\n"
                "Поддержка контроллера доступна.\n"
                "Настройте в меню управления."
            ).arg(scoreText, levelText, linesText, sequenceText, difficulty);
        } else {
            controlsText = QString(
                "%1\n"
                "%2\n"
                "%3\n"
                "%4\n"
                "Difficulty: %5\n\n"
                "Keyboard Controls:\n"
                "• Left/Right/A/D: Move block\n"
                "• Up/W: Rotate clockwise\n"
                "• Z: Rotate counter-clockwise\n"
                "• Down/S: Soft drop\n"
                "• Space: Hard drop\n"
                "• P: Pause/Resume game\n"
                "• R: Restart game\n"
                "• M: Toggle music\n\n"
                "Controller support is available.\n"
                "Configure in Controls menu."
            ).arg(scoreText, levelText, linesText, sequenceText, difficulty);
        }
        app->controlsLabel->setText(controlsText);
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
    
    // Adjust drop speed based on difficulty and level
    adjustDropSpeed(app);
    
    // Update the game timer interval with new speed
    if (app->gameTimer) {
        app->gameTimer->setInterval(app->dropSpeed);
    }
    
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
    
    if (app->sdlCairoRenderer) {
        app->sdlCairoRenderer->cleanup();
        delete app->sdlCairoRenderer;
        app->sdlCairoRenderer = nullptr;
    }
}

// ============================================================================
// Menu Actions - Qt5
// ============================================================================

void onStartGameAction(TetrimoneApp* app) {
    app->board->setSplashScreenActive(true);
    updateDisplay(app);
}

void onPauseGameAction(TetrimoneApp* app) {
    pauseGame(app);
    updateDisplay(app);
}

void onRestartGameAction(TetrimoneApp* app) {
    app->board->setSplashScreenActive(true);
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
        std::cout << (enabled ? "✓ Sound enabled" : "✓ Sound disabled") << std::endl;
    }
}

void onDifficultyChanged(TetrimoneApp* app, int difficulty) {
    if (!app || !app->board) return;
    
    int previousDifficulty = app->difficulty;
    
    // Only proceed if difficulty actually changed
    if (difficulty != previousDifficulty) {
        // Apply the new difficulty
        app->difficulty = difficulty;
        app->board->setDifficultyFromGUI(difficulty);
        
        // Adjust drop speed based on new difficulty and current level
        adjustDropSpeed(app);
        
        // Show splash screen and reset game
        app->board->setSplashScreenActive(true);
        app->board->restart();
        resetUI(app);
        
        // Update labels and display
        updateLabels(app);
        updateDisplay(app);
    }
}

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
// GPU Rendering Functions
// ============================================================================

void initGPURenderer(TetrimoneApp* app, int width, int height) {
    if (!app || !app->useGPUAcceleration) return;
    
    if (app->sdlCairoRenderer) {
        delete app->sdlCairoRenderer;
    }
    
    app->sdlCairoRenderer = new SDLCairoRenderer(width, height);
    
    if (app->sdlCairoRenderer->init("Tetrimone - GPU Accelerated", 
                                     SDL_WINDOW_OPENGL | SDL_WINDOW_SHOWN)) {
        std::cout << "✓ GPU-accelerated rendering enabled (OpenGL)" << std::endl;
    } else {
        std::cerr << "Failed to initialize GPU renderer, falling back to CPU" << std::endl;
        delete app->sdlCairoRenderer;
        app->sdlCairoRenderer = nullptr;
        app->useGPUAcceleration = false;
    }
}

void shutdownGPURenderer(TetrimoneApp* app) {
    if (!app) return;
    
    if (app->sdlCairoRenderer) {
        app->sdlCairoRenderer->cleanup();
        delete app->sdlCairoRenderer;
        app->sdlCairoRenderer = nullptr;
    }
}

void renderFrameGPU(TetrimoneApp* app) {
    if (!app || !app->sdlCairoRenderer || !app->board) return;
    
    app->sdlCairoRenderer->clearCairoSurface(0, 0, 0, 1.0);
    cairo_t* cr = app->sdlCairoRenderer->getCairoContext();
    
    drawGridLines(cr, app->board);
    drawPlacedBlocks(cr, app->board, app);
    drawCurrentPiece(cr, app->board);
    drawGhostPiece(cr, app->board);
    drawBlockTrails(cr, app->board);
    drawGameOver(cr, app->board);
    drawPauseMenu(cr, app->board);
    if (app->board->isSplashScreenActive()) {
        drawSplashScreen(cr, app->board, app);
    }
    drawFireworks(cr, app->board, app);
    
    app->sdlCairoRenderer->syncSurfaceToTexture();
    app->sdlCairoRenderer->present();
}

// ============================================================================
// Application Initialization - Qt5
// ============================================================================

void onAppActivate(TetrimoneApp* app) {
    if (!app) return;
    
    setupGameUI(app, 800, 600);
    
    // Initialize audio system after board is created
    if (app->board) {
        std::cout << "\n=== AUDIO INITIALIZATION ===" << std::endl;
        
        // First enable sound if it's not already
        std::cout << "Step 1: Enabling sound..." << std::endl;
        app->board->setSoundEnabled(true);
        std::cout << "  Sound enabled state: " << (app->board->isSoundEnabled() ? "TRUE" : "FALSE") << std::endl;
        
        // Get sound zip path from command line args if available
        std::string soundZipPath = "sound.zip";
        if (app->cmdlineArgs && !app->cmdlineArgs->soundZip.empty()) {
            soundZipPath = app->cmdlineArgs->soundZip;
            std::cout << "Step 2: Using sound.zip from command line: " << soundZipPath << std::endl;
        } else {
            std::cout << "Step 2: No command line arg, using default: " << soundZipPath << std::endl;
        }
        
        // Check if file exists
        std::ifstream checkFile(soundZipPath);
        if (checkFile.good()) {
            std::cout << "Step 3: ✓ sound.zip file exists" << std::endl;
            checkFile.close();
        } else {
            std::cout << "Step 3: ✗ sound.zip file NOT found at: " << soundZipPath << std::endl;
        }
        
        // Set the sounds zip path
        std::cout << "Step 4: Setting sounds zip path..." << std::endl;
        std::cout << "  Before setSoundsZipPath - sound enabled: " << (app->board->isSoundEnabled() ? "TRUE" : "FALSE") << std::endl;
        std::cout << "  Calling setSoundsZipPath(\"" << soundZipPath << "\")..." << std::endl;
        
        bool setPathResult = app->board->setSoundsZipPath(soundZipPath);
        std::cout << "  setSoundsZipPath returned: " << (setPathResult ? "TRUE" : "FALSE") << std::endl;
        
        if (setPathResult) {
            std::cout << "Step 4: ✓ setSoundsZipPath succeeded" << std::endl;
        } else {
            std::cout << "Step 4: ✗ setSoundsZipPath failed" << std::endl;
            std::cout << "  This usually means initializeAudio() failed inside setSoundsZipPath" << std::endl;
        }
        
        // Initialize audio
        std::cout << "Step 5: Calling initializeAudio()..." << std::endl;
        bool audioInit = app->board->initializeAudio();
        std::cout << "  initializeAudio returned: " << (audioInit ? "TRUE" : "FALSE") << std::endl;
        
        if (audioInit) {
            std::cout << "Step 5: ✓ Audio system initialized successfully" << std::endl;
        } else {
            std::cerr << "Step 5: ✗ Audio initialization failed" << std::endl;
            std::cerr << "  Check if sound.zip is valid and contains required audio files" << std::endl;
        }
        
        std::cout << "=== AUDIO INITIALIZATION COMPLETE ===" << std::endl << std::endl;
    }
    
    if (app->window) {
        app->window->show();
    }
    
    if (app->board) {
        app->board->setSplashScreenActive(true);
    }
    
    updateDisplay(app);
}

void rebuildGameUI(TetrimoneApp* app) {
    if (!app || !app->window) return;
    
    updateDisplay(app);
}

void setupMenuBar(TetrimoneApp* app) {
    if (!app || !app->menuBar) return;
    
    // ===== FILE MENU =====
    QMenu* fileMenu = app->menuBar->addMenu("&File");
    
    app->startMenuItem = fileMenu->addAction("&New Game");
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
    
    // ===== GAME MENU =====
    QMenu* gameMenu = app->menuBar->addMenu("&Game");
    
    // Difficulty submenu
    QMenu* difficultyMenu = gameMenu->addMenu("&Difficulty");
    QActionGroup* difficultyGroup = new QActionGroup(difficultyMenu);
    
    const char* difficultyNames[] = {"Zen", "Easy", "Medium", "Hard", "Extreme", "Insane"};
    app->zenMenuItem = difficultyMenu->addAction("Zen");
    app->zenMenuItem->setCheckable(true);
    app->zenMenuItem->setActionGroup(difficultyGroup);
    QObject::connect(app->zenMenuItem, &QAction::triggered, [app]() {
        onDifficultyChanged(app, 0);
    });
    
    app->easyMenuItem = difficultyMenu->addAction("Easy");
    app->easyMenuItem->setCheckable(true);
    app->easyMenuItem->setActionGroup(difficultyGroup);
    QObject::connect(app->easyMenuItem, &QAction::triggered, [app]() {
        onDifficultyChanged(app, 1);
    });
    
    app->mediumMenuItem = difficultyMenu->addAction("Medium");
    app->mediumMenuItem->setCheckable(true);
    app->mediumMenuItem->setActionGroup(difficultyGroup);
    app->mediumMenuItem->setChecked(true);
    QObject::connect(app->mediumMenuItem, &QAction::triggered, [app]() {
        onDifficultyChanged(app, 2);
    });
    
    app->hardMenuItem = difficultyMenu->addAction("Hard");
    app->hardMenuItem->setCheckable(true);
    app->hardMenuItem->setActionGroup(difficultyGroup);
    QObject::connect(app->hardMenuItem, &QAction::triggered, [app]() {
        onDifficultyChanged(app, 3);
    });
    
    app->extremeMenuItem = difficultyMenu->addAction("Extreme");
    app->extremeMenuItem->setCheckable(true);
    app->extremeMenuItem->setActionGroup(difficultyGroup);
    QObject::connect(app->extremeMenuItem, &QAction::triggered, [app]() {
        onDifficultyChanged(app, 4);
    });
    
    app->insaneMenuItem = difficultyMenu->addAction("Insane");
    app->insaneMenuItem->setCheckable(true);
    app->insaneMenuItem->setActionGroup(difficultyGroup);
    QObject::connect(app->insaneMenuItem, &QAction::triggered, [app]() {
        onDifficultyChanged(app, 5);
    });
    
    gameMenu->addSeparator();
    
    app->highScoresMenuItem = gameMenu->addAction("&High Scores");
    QObject::connect(app->highScoresMenuItem, &QAction::triggered, [app]() {
        onViewHighScores(app);
    });
    
    gameMenu->addSeparator();
    
    app->gameSetupMenuItem = gameMenu->addAction("&Game Setup");
    QObject::connect(app->gameSetupMenuItem, &QAction::triggered, [app]() {
        onGameSetupDialog(app);
    });
    
    app->resetSettingsMenuItem = gameMenu->addAction("&Reset Settings");
    QObject::connect(app->resetSettingsMenuItem, &QAction::triggered, [app]() {
        onResetSettings(app);
    });
    
    // ===== GRAPHICS MENU =====
    QMenu* graphicsMenu = app->menuBar->addMenu("&Graphics");
    
    // Block Size
    app->blockSizeMenuItem = graphicsMenu->addAction("&Block Size...");
    QObject::connect(app->blockSizeMenuItem, &QAction::triggered, [app]() {
        onBlockSizeDialog(app);
    });
    
    // Game Size
    app->gameSizeMenuItem = graphicsMenu->addAction("&Game Size...");
    QObject::connect(app->gameSizeMenuItem, &QAction::triggered, [app]() {
        onGameSizeDialog(app);
    });
    
    graphicsMenu->addSeparator();
    
    // Background submenu
    QMenu* backgroundMenu = graphicsMenu->addMenu("&Background");
    
    app->backgroundImageMenuItem = backgroundMenu->addAction("&Set Image...");
    QObject::connect(app->backgroundImageMenuItem, &QAction::triggered, [app]() {
        onBackgroundImageDialog(app);
    });
    
    app->backgroundImagesMenuItem = backgroundMenu->addAction("&Browse Images...");
    QObject::connect(app->backgroundImagesMenuItem, &QAction::triggered, [app]() {
        onBackgroundImagesDialog(app);
    });
    
    app->backgroundZipMenuItem = backgroundMenu->addAction("&Load from ZIP...");
    QObject::connect(app->backgroundZipMenuItem, &QAction::triggered, [app]() {
        onBackgroundZipDialog(app);
    });
    
    app->backgroundOpacityMenuItem = backgroundMenu->addAction("&Opacity...");
    QObject::connect(app->backgroundOpacityMenuItem, &QAction::triggered, [app]() {
        onBackgroundOpacityDialog(app);
    });
    
    backgroundMenu->addSeparator();
    
    app->backgroundToggleMenuItem_Display = backgroundMenu->addAction("&Enable Background");
    app->backgroundToggleMenuItem_Display->setCheckable(true);
    app->backgroundToggleMenuItem_Display->setChecked(true);
    QObject::connect(app->backgroundToggleMenuItem_Display, &QAction::triggered, [app](bool checked) {
        onBackgroundToggled(app, checked);
    });
    
    graphicsMenu->addSeparator();
    
    // Color Themes submenu
    QMenu* themeMenu = graphicsMenu->addMenu("&Color Themes");
    QActionGroup* themeGroup = new QActionGroup(themeMenu);
    
    const char* themeNames[] = {
        "Watercolor", "Neon", "Pastel", "Earth Tones", "Monochrome Blue",
        "Monochrome Green", "Sunset", "Ocean", "Grayscale", "Candy",
        "Neon Dark", "Jewel Tones", "Retro Gaming", "Autumn", "Winter",
        "Spring", "Summer", "Monochrome Purple", "Desert", "Rainbow",
        "Art Deco", "Northern Lights", "Moroccan Tiles", "Bioluminescence", "Fossil",
        "Silk Road", "Digital Glitch", "Botanical", "Jazz Age", "Steampunk", "USA"
    };
    
    for (int i = 0; i < 31; i++) {
        app->themeMenuItems[i] = themeMenu->addAction(themeNames[i]);
        app->themeMenuItems[i]->setCheckable(true);
        app->themeMenuItems[i]->setActionGroup(themeGroup);
        if (i == currentThemeIndex) {
            app->themeMenuItems[i]->setChecked(true);
        }
        QObject::connect(app->themeMenuItems[i], &QAction::triggered, [app, i]() {
            onThemeChanged(app, i);
        });
    }
    
    graphicsMenu->addSeparator();
    
    // Display options
    app->ghostPieceMenuItem = graphicsMenu->addAction("&Show Ghost Piece");
    app->ghostPieceMenuItem->setCheckable(true);
    if (app->board) {
        app->ghostPieceMenuItem->setChecked(app->board->isGhostPieceEnabled());
    }
    QObject::connect(app->ghostPieceMenuItem, &QAction::triggered, [app](bool checked) {
        onGhostPieceToggled(app, checked);
    });
    
    app->gridLinesMenuItem = graphicsMenu->addAction("&Grid Lines");
    app->gridLinesMenuItem->setCheckable(true);
    if (app->board) {
        app->gridLinesMenuItem->setChecked(app->board->isShowingGridLines());
    }
    QObject::connect(app->gridLinesMenuItem, &QAction::triggered, [app](bool checked) {
        onGridLinesToggled(app, checked);
    });
    
    app->blockTrailsMenuItem = graphicsMenu->addAction("&Block Trails");
    app->blockTrailsMenuItem->setCheckable(true);
    if (app->board) {
        app->blockTrailsMenuItem->setChecked(app->board->isTrailsEnabled());
    }
    QObject::connect(app->blockTrailsMenuItem, &QAction::triggered, [app](bool checked) {
        onBlockTrailsToggled(app, checked);
    });
    
    app->blockTrailsConfigMenuItem = graphicsMenu->addAction("&Block Trails Settings...");
    QObject::connect(app->blockTrailsConfigMenuItem, &QAction::triggered, [app]() {
        onBlockTrailsConfig(app);
    });
    
    app->simpleBlocksMenuItem = graphicsMenu->addAction("&Simple Blocks (No 3D)");
    app->simpleBlocksMenuItem->setCheckable(true);
    if (app->board) {
        app->simpleBlocksMenuItem->setChecked(app->board->simpleBlocksActive);
    }
    QObject::connect(app->simpleBlocksMenuItem, &QAction::triggered, [app](bool checked) {
        onSimpleBlocksToggled(app, checked);
    });
    
    graphicsMenu->addSeparator();
    
    // Rendering mode (TODO)
    QMenu* renderMenu = graphicsMenu->addMenu("&Rendering Mode");
    QActionGroup* renderGroup = new QActionGroup(renderMenu);
    
    QAction* cairoAction = renderMenu->addAction("&Cairo");
    cairoAction->setCheckable(true);
    cairoAction->setActionGroup(renderGroup);
    cairoAction->setChecked(true);
    app->renderModeMenuItems[0] = cairoAction;
    
    QAction* openglAction = renderMenu->addAction("&OpenGL");
    openglAction->setCheckable(true);
    openglAction->setActionGroup(renderGroup);
    app->renderModeMenuItems[1] = openglAction;
    
    // ===== SOUND MENU =====
    QMenu* soundMenu = app->menuBar->addMenu("&Sound");
    
    app->soundToggleMenuItem = soundMenu->addAction("&Enable Sound");
    app->soundToggleMenuItem->setCheckable(true);
    app->soundToggleMenuItem->setChecked(true);
    QObject::connect(app->soundToggleMenuItem, &QAction::triggered, [app](bool checked) {
        onSoundToggleAction(app, checked);
    });
    
    app->volumeMenuItem = soundMenu->addAction("&Volume Settings...");
    QObject::connect(app->volumeMenuItem, &QAction::triggered, [app]() {
        onVolumeDialog(app);
    });
    
    soundMenu->addSeparator();
    
    // Music tracks submenu
    QMenu* musicMenu = soundMenu->addMenu("&Music Tracks");
    for (int i = 0; i < 5; i++) {
        QString label = QString("&Track %1").arg(i + 1);
        app->trackMenuItems[i] = musicMenu->addAction(label);
        app->trackMenuItems[i]->setCheckable(true);
        app->trackMenuItems[i]->setChecked(true);
        
        QObject::connect(app->trackMenuItems[i], &QAction::triggered, [app, i](bool checked) {
            onTrackToggled(app, i, checked);
        });
    }
    
    soundMenu->addSeparator();
    
    app->retroMusicMenuItem = soundMenu->addAction("&Use Retro Music");
    app->retroMusicMenuItem->setCheckable(true);
    if (app->board) {
        app->retroMusicMenuItem->setChecked(app->board->retroMusicActive);
    }
    QObject::connect(app->retroMusicMenuItem, &QAction::triggered, [app](bool checked) {
        onRetroMusicToggled(app, checked);
    });
    
    // ===== INPUT MENU (TODO) =====
    QMenu* inputMenu = app->menuBar->addMenu("&Input");
    
    app->joystickConfigMenuItem = inputMenu->addAction("&Joystick Configuration...");
    QObject::connect(app->joystickConfigMenuItem, &QAction::triggered, [app]() {
        onJoystickConfig(app);
    });
    
    // ===== HELP MENU =====
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
    
    if (!app->board) {
        app->board = new TetrimoneBoard();
        app->board->app = app;
    }
    
    app->window = new TetrimoneWindow(app);
    app->window->setWindowTitle("Tetrimone");
    
    // Get screen size
    QScreen* screen = QApplication::primaryScreen();
    QRect screenGeometry = screen->availableGeometry();
    int screenWidth = screenGeometry.width();
    int screenHeight = screenGeometry.height();
    
    // Calculate window size to stretch board to fill screen
    int windowWidth = std::min(1200, (screenWidth * 90) / 100);
    int windowHeight = std::min(1000, (screenHeight * 95) / 100);
    app->window->resize(windowWidth, windowHeight);
    
    // Center window on screen
    int x = (screenWidth - windowWidth) / 2;
    int y = (screenHeight - windowHeight) / 2;
    app->window->move(screenGeometry.left() + x, screenGeometry.top() + y);
    
    app->menuBar = new QMenuBar(app->window);
    
    QVBoxLayout* mainLayout = new QVBoxLayout(app->window);
    mainLayout->setMenuBar(app->menuBar);
    mainLayout->setContentsMargins(5, 5, 5, 5);
    mainLayout->setSpacing(0);
    
    // Center container for game
    QWidget* centerWidget = new QWidget();
    QHBoxLayout* centerLayout = new QHBoxLayout(centerWidget);
    centerLayout->setSpacing(10);
    centerLayout->setContentsMargins(0, 0, 0, 0);
    
    // LEFT: Stats + Board
    QVBoxLayout* gamePanel = new QVBoxLayout();
    gamePanel->setSpacing(10);
    gamePanel->setContentsMargins(0, 0, 0, 0);
    
    // Stats (LEFT of board)
    QHBoxLayout* statsLayout = new QHBoxLayout();
    statsLayout->setSpacing(15);
    statsLayout->setContentsMargins(0, 0, 0, 0);
    statsLayout->addStretch();  // Add stretch before stats column
    
    QVBoxLayout* statsColumn = new QVBoxLayout();
    statsColumn->setSpacing(5);
    app->scoreLabel = new QLabel("Score: 0");
    app->levelLabel = new QLabel("Level: 1");
    app->linesLabel = new QLabel("Lines: 0");
    app->difficultyLabel = new QLabel("Difficulty: Easy");
    
    QFont labelFont;
    labelFont.setPointSize(10);
    labelFont.setBold(true);
    app->scoreLabel->setFont(labelFont);
    app->levelLabel->setFont(labelFont);
    app->linesLabel->setFont(labelFont);
    app->difficultyLabel->setFont(labelFont);
    
    statsColumn->addWidget(app->scoreLabel);
    statsColumn->addWidget(app->levelLabel);
    statsColumn->addWidget(app->linesLabel);
    statsColumn->addWidget(app->difficultyLabel);
    
    // Add propaganda/freedom message label
    app->sequenceLabel = new QLabel("");
    QFont messageFont;
    messageFont.setPointSize(9);
    messageFont.setItalic(true);
    app->sequenceLabel->setFont(messageFont);
    app->sequenceLabel->setAlignment(Qt::AlignTop | Qt::AlignLeft);
    app->sequenceLabel->setWordWrap(true);
    app->sequenceLabel->setMaximumHeight(80);
    statsColumn->addWidget(app->sequenceLabel);
    statsColumn->addStretch();
    
    statsLayout->addLayout(statsColumn, 0);
    
    // Game board - centered
    app->gameArea = new GameAreaWidget(app->board, app);
    statsLayout->addWidget(app->gameArea, 0, Qt::AlignCenter);
    
    statsLayout->addStretch();  // Add stretch after board to center it
    
    gamePanel->addLayout(statsLayout, 1);
    centerLayout->addLayout(gamePanel, 1);
    
    // RIGHT: Next pieces + controls
    QVBoxLayout* rightPanel = new QVBoxLayout();
    rightPanel->setSpacing(12);
    rightPanel->setContentsMargins(0, 0, 0, 0);
    
    // Next piece area (SDL rendered)
    app->nextPieceArea = new NextPieceWidget(app->board, app);
    rightPanel->addWidget(app->nextPieceArea, 0, Qt::AlignHCenter);
    
    // Controls info + Stats - ALL in one label like GTK3
    app->controlsLabel = new QLabel(
        "Score: 0\n"
        "Level: 1\n"
        "Lines: 0\n"
        "Sequence: 0 (Max: 0)\n"
        "Difficulty: Easy\n\n"
        "Keyboard Controls:\n"
        "• Left/Right/A/D: Move block\n"
        "• Up/W: Rotate clockwise\n"
        "• Z: Rotate counter-clockwise\n"
        "• Down/S: Soft drop\n"
        "• Space: Hard drop\n"
        "• P: Pause/Resume game\n"
        "• R: Restart game\n"
        "• M: Toggle music\n\n"
        "Controller support is available.\n"
        "Configure in Controls menu."
    );
    QFont controlsFont;
    controlsFont.setPointSize(9);
    controlsFont.setFamily("Courier");
    app->controlsLabel->setFont(controlsFont);
    app->controlsLabel->setAlignment(Qt::AlignCenter);
    app->controlsLabel->setStyleSheet(
        "QLabel { "
        "background-color: white; "
        "color: black; "
        "padding: 10px; "
        "border-radius: 3px; "
        "border: 1px solid #999999; "
        "}"
    );
    rightPanel->addWidget(app->controlsLabel, 0);
    
    rightPanel->addStretch(1);
    
    centerLayout->addLayout(rightPanel, 0);
    
    mainLayout->addWidget(centerWidget, 1);
    
    // Setup menus
    setupMenuBar(app);
    
    // Setup game timer for piece falling
    QTimer* gameTimer = new QTimer(app->window);
    QObject::connect(gameTimer, &QTimer::timeout, [app]() {
        onGameTick(app);
    });
    gameTimer->start(app->dropSpeed);
    app->gameTimer = gameTimer;  // Store the pointer directly
    app->timerId = gameTimer->timerId();
}

// ============================================================================
// Application Entry Point - Qt5
// ============================================================================

// Note: main_qt5() is not used in the current build. 
// ui_run_application() in tetrimone.cpp calls onAppActivate() which handles Qt5 initialization.
// This function is kept for reference but is superseded by ui_run_application().

// ============================================================================
// TODO: Menu Callback Implementations (from GTK3)
// ============================================================================

void onBlockSizeDialog(TetrimoneApp* app) {
    // TODO: Implement block size configuration dialog
}

void onBlockSizeValueChanged(int value, TetrimoneApp* app) {
    // TODO: Implement block size value change
}

void onResizeWindowButtonClicked(TetrimoneApp* app) {
    // TODO: Implement window resize
}

void onJoystickConfig(TetrimoneApp* app) {
    // TODO: Implement joystick configuration dialog
}

void onJoystickRescan(TetrimoneApp* app) {
    // TODO: Implement joystick rescan
}

void updateJoystickInfo(TetrimoneApp* app) {
    // TODO: Update joystick info display
}

void onJoystickMapApply(TetrimoneApp* app) {
    // TODO: Apply joystick mapping
}

void onJoystickMapReset(TetrimoneApp* app) {
    // TODO: Reset joystick mapping
}

void onBackgroundImageDialog(TetrimoneApp* app) {
    // TODO: Implement background image selection dialog
}

void onBackgroundToggled(TetrimoneApp* app, bool enabled) {
    // TODO: Implement background toggle
}

void onBackgroundOpacityDialog(TetrimoneApp* app) {
    // TODO: Implement background opacity dialog
}

void onOpacityValueChanged(int value, TetrimoneApp* app) {
    // TODO: Implement opacity value change
}

void updateSizeValueLabel(int value, TetrimoneApp* app) {
    // TODO: Update size value label
}

void onBackgroundZipDialog(TetrimoneApp* app) {
    // TODO: Implement background ZIP loading dialog
}

void onVolumeDialog(TetrimoneApp* app) {
    // TODO: Implement volume settings dialog
}

void onVolumeValueChanged(int value, TetrimoneApp* app) {
    // TODO: Implement volume value change
}

void onMusicVolumeValueChanged(int value, TetrimoneApp* app) {
    // TODO: Implement music volume value change
}

void onTrackToggled(TetrimoneApp* app, int trackIndex, bool enabled) {
    // TODO: Implement music track toggle
}

void onBlockSizeRulesChanged(TetrimoneApp* app, int mode) {
    // TODO: Implement block size rules change
}

void onGameSizeDialog(TetrimoneApp* app) {
    // TODO: Implement game size configuration dialog
}

void onGridLinesToggled(TetrimoneApp* app, bool enabled) {
    if (app->board) {
        app->board->setShowGridLines(enabled);
        updateDisplay(app);
    }
}

void updateWidthValueLabel(int value, TetrimoneApp* app) {
    // TODO: Update width value label
}

void updateHeightValueLabel(int value, TetrimoneApp* app) {
    // TODO: Update height value label
}

void onGhostPieceToggled(TetrimoneApp* app, bool enabled) {
    if (app->board) {
        app->board->setGhostPieceEnabled(enabled);
        updateDisplay(app);
    }
}

void onViewHighScores(TetrimoneApp* app) {
    if (!app || !app->board) return;
    
    // Get all scores
    const std::vector<Score>& allScores = app->board->getHighScores().getScores();
    
    // Difficulty levels to create tabs for
    const std::vector<std::string> difficulties = {
        "All", "Zen", "Easy", "Medium", "Hard", "Extreme", "Insane"
    };
    
    // Build tab data
    std::vector<Qt5Helpers::ScoreTabData> tabs;
    for (const auto& difficulty : difficulties) {
        Qt5Helpers::ScoreTabData tabData;
        tabData.tabName = difficulty;
        
        if (difficulty == "All") {
            tabData.scores = allScores;
        } else {
            tabData.scores = app->board->getHighScores().getScoresByDifficulty(difficulty);
        }
        
        tabs.push_back(tabData);
    }
    
    // Configure and display tabulator
    Qt5Helpers::ScoreTabulatorConfig config{
        .title = "High Scores",
        .tabs = tabs,
        .width = 900,
        .height = 600
    };
    
    Qt5Helpers::createScoreTabulatorDialog(app->window, config);
}

void onBackgroundImagesDialog(TetrimoneApp* app) {
    // TODO: Implement background images browser dialog
}

void onSimpleBlocksToggled(TetrimoneApp* app, bool enabled) {
    if (app->board) {
        app->board->simpleBlocksActive = enabled;
        updateDisplay(app);
    }
}

void onRetroMusicToggled(TetrimoneApp* app, bool enabled) {
    if (app->board) {
        app->board->retroMusicActive = enabled;
    }
}

void onTestSound(TetrimoneApp* app) {
    // TODO: Implement test sound
}

void onGameSetupDialog(TetrimoneApp* app) {
    // TODO: Implement game setup dialog
}

void onResetSettings(TetrimoneApp* app) {
    // TODO: Implement settings reset
}

void onThemeChanged(TetrimoneApp* app, int themeIndex) {
    // TODO: Implement theme change
    currentThemeIndex = themeIndex;
    updateDisplay(app);
}

void onBlockTrailsToggled(TetrimoneApp* app, bool enabled) {
    if (app->board) {
        app->board->setTrailsEnabled(enabled);
        updateDisplay(app);
    }
}

void onBlockTrailsConfig(TetrimoneApp* app) {
    // TODO: Implement block trails configuration dialog
}

void onTrailOpacityChanged(int value, TetrimoneApp* app) {
    // TODO: Implement trail opacity value change
}

void onTrailDurationChanged(int value, TetrimoneApp* app) {
    // TODO: Implement trail duration value change
}

void onRenderModeChanged(TetrimoneApp* app, int mode) {
    // TODO: Implement rendering mode change
}
