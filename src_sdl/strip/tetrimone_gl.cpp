#include "tetrimone.h"
#include "audiomanager.h"
#include <iostream>
#include <string>
#include <algorithm>
#include <cmath>
#ifdef _WIN32
#include <windows.h>
#include <commdlg.h>
#endif
#include "highscores.h"
#include "propaganda_messages.h"
#include "freedom_messages.h"
#include "commandline.h"

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

int BLOCK_SIZE = 30; // Default value, will be updated at runtime
int currentThemeIndex = 0;

int GRID_WIDTH = 10;
int GRID_HEIGHT = 22;

static bool keyDownPressed = false;
static bool keyLeftPressed = false;
static bool keyRightPressed = false;
static guint keyDownTimer = 0;
static guint keyLeftTimer = 0;
static guint keyRightTimer = 0;
static int keyDownDelay = 150;
static int keyLeftDelay = 150;
static int keyRightDelay = 150;
static int keyDownCount = 0;
static int keyLeftCount = 0;
static int keyRightCount = 0;

gboolean onKeyDownTick(gpointer userData) {
  TetrimoneApp *app = static_cast<TetrimoneApp *>(userData);
  
  // Only process if the game is active and key is still pressed
  if (!app->board->isPaused() && !app->board->isGameOver() && 
      !app->board->isSplashScreenActive() && keyDownPressed) {
    
    // Move the piece down with acceleration
    app->board->movePiece(0, 1);
    keyDownCount++;
    
    // Accelerate by decreasing the delay (matches joystick.cpp)
    if (keyDownCount > 6) {
      keyDownDelay = 20; // Very fast (matches joystick)
    } else if (keyDownCount > 4) {
      keyDownDelay = 30; // Fast (matches joystick)
    } else if (keyDownCount > 2) {
      keyDownDelay = 60; // Medium (matches joystick)
    }
    
    // Update the timer with the new delay
    keyDownTimer = g_timeout_add(keyDownDelay, onKeyDownTick, app);
    
    // Update the display
    gtk_widget_queue_draw(app->gameArea);
    gtk_widget_queue_draw(app->nextPieceArea);
    updateLabels(app);
    
    // Stop this timer instance (we created a new one above)
    return FALSE;
  }
  
  // If game is paused or key is released, stop the timer
  keyDownTimer = 0;
  return FALSE;
}

gboolean onKeyLeftTick(gpointer userData) {
  TetrimoneApp *app = static_cast<TetrimoneApp *>(userData);
  
  if (!app->board->isPaused() && !app->board->isGameOver() && 
      !app->board->isSplashScreenActive() && keyLeftPressed) {
    
    // Move the piece left with acceleration
    app->board->movePiece(-1, 0);
    keyLeftCount++;
    
    // Accelerate by decreasing the delay (matches joystick.cpp horizontal movement)
    if (keyLeftCount > 6) {
      keyLeftDelay = 30; // Very fast
    } else if (keyLeftCount > 4) {
      keyLeftDelay = 50; // Fast
    } else if (keyLeftCount > 2) {
      keyLeftDelay = 100; // Medium
    }
    
    // Update the timer with the new delay
    keyLeftTimer = g_timeout_add(keyLeftDelay, onKeyLeftTick, app);
    
    // Update the display
    gtk_widget_queue_draw(app->gameArea);
    gtk_widget_queue_draw(app->nextPieceArea);
    updateLabels(app);
    
    // Stop this timer instance
    return FALSE;
  }
  
  keyLeftTimer = 0;
  return FALSE;
}

gboolean onKeyRightTick(gpointer userData) {
  TetrimoneApp *app = static_cast<TetrimoneApp *>(userData);
  
  if (!app->board->isPaused() && !app->board->isGameOver() && 
      !app->board->isSplashScreenActive() && keyRightPressed) {
    
    // Move the piece right with acceleration
    app->board->movePiece(1, 0);
    keyRightCount++;
    
    // Accelerate by decreasing the delay (matches joystick.cpp horizontal movement)
    if (keyRightCount > 6) {
      keyRightDelay = 30; // Very fast
    } else if (keyRightCount > 4) {
      keyRightDelay = 50; // Fast
    } else if (keyRightCount > 2) {
      keyRightDelay = 100; // Medium
    }
    
    // Update the timer with the new delay
    keyRightTimer = g_timeout_add(keyRightDelay, onKeyRightTick, app);
    
    // Update the display
    gtk_widget_queue_draw(app->gameArea);
    gtk_widget_queue_draw(app->nextPieceArea);
    updateLabels(app);
    
    // Stop this timer instance
    return FALSE;
  }
  
  keyRightTimer = 0;
  return FALSE;
}

// TetrimoneBlock class implementation
TetrimoneBlock::TetrimoneBlock(int type) : type(type), rotation(0) {
  // Start pieces centered at top
  x = GRID_WIDTH / 2 - 2;
  y = 0;
}

int TetrimoneBlock::getRotation() const { return rotation; }

void TetrimoneBlock::rotate(bool clockwise) {
  rotation = (rotation + (clockwise ? 1 : 3)) % 4;
}

void TetrimoneBlock::move(int dx, int dy) {
  x += dx;
  y += dy;
}

std::vector<std::vector<int>> TetrimoneBlock::getShape() const {
  return TETRIMONEBLOCK_SHAPES[type][rotation];
}

std::array<double, 3> TetrimoneBlock::getColor() const {
    // This is a bit tricky since TetrimoneBlock doesn't have access to TetrimoneBoard
    // We'll need to modify this differently - see the drawing code changes below
    int themeIndex = currentThemeIndex;
    if (themeIndex >= TETRIMONEBLOCK_COLOR_THEMES.size()) {
        themeIndex = TETRIMONEBLOCK_COLOR_THEMES.size() - 1;
    }
    return TETRIMONEBLOCK_COLOR_THEMES[themeIndex][type];
}

void TetrimoneBlock::setPosition(int newX, int newY) {
  x = newX;
  y = newY;
}

// TetrimoneBoard class implementation
TetrimoneBoard::TetrimoneBoard()
    : score(0), level(1), linesCleared(0), gameOver(false),paused(false),
      ghostPieceEnabled(true), splashScreenActive(true),
      backgroundImage(nullptr), useBackgroundImage(false),
      backgroundOpacity(0.3), useBackgroundZip(false),
      currentBackgroundIndex(0), isTransitioning(false), transitionOpacity(0.0),
      transitionDirection(0), oldBackground(nullptr), transitionTimerId(0),
      consecutiveClears(0), maxConsecutiveClears(0), lastClearCount(0),
      sequenceActive(false), lineClearActive(false), lineClearProgress(0.0), lineClearAnimationTimer(0),
currentPieceInterpolatedX(0), currentPieceInterpolatedY(0),
lastPieceX(0), lastPieceY(0), smoothMovementTimer(0), movementProgress(0.0),
      isThemeTransitioning(false), oldThemeIndex(0), newThemeIndex(0),
      themeTransitionProgress(0.0), themeTransitionTimer(0) {
  rng.seed(std::chrono::system_clock::now().time_since_epoch().count());

    showPropagandaMessage = false;
    propagandaTimerId = 0;
    propagandaMessageDuration = 2000; // 2 seconds display time

fireworksActive = false;
fireworksTimer = 0;
fireworksType = 0;

trailsEnabled = true;          // Enabled by default
maxTrailSegments = 3;          // Keep only 3 trail segments (reduced)
trailOpacity = 0.6;            // Default opacity
trailDuration = 0.1;           // Default duration (seconds)
trailUpdateTimer = 0;
lastTrailTime = std::chrono::high_resolution_clock::now();


heatLevel = 0.5f;
heatDecayTimer = 0;
  // Initialize grid with maximum possible dimensions to avoid reallocation
  grid.resize(MAX_GRID_HEIGHT, std::vector<int>(MAX_GRID_WIDTH, 0));

  // Initialize with 3 next pieces instead of just one
  nextPieces.resize(3);

  // Generate initial pieces
  generateNewPiece();

  // Try to load background.zip by default
  if (loadBackgroundImagesFromZip("background.zip")) {
    std::cout << "Successfully loaded background images from background.zip"
              << std::endl;
    // Background should be enabled by default if successfully loaded
    useBackgroundImage = true;
    useBackgroundZip = true;
  } else {
    std::cout << "Could not load background.zip, backgrounds will need to be "
                 "loaded manually"
              << std::endl;
  }
  for (int i = 0; i < 5; i++) {
    enabledTracks[i] = true;
  }
}

TetrimoneBoard::~TetrimoneBoard() {
    // Cancel any ongoing transition and clean up resources
    cancelBackgroundTransition();

    // Cancel propaganda message timers
    if (propagandaTimerId > 0) {
        g_source_remove(propagandaTimerId);
        propagandaTimerId = 0;
    }
    
    if (propagandaScaleTimerId > 0) {
        g_source_remove(propagandaScaleTimerId);
        propagandaScaleTimerId = 0;
    }

    if (backgroundImage != nullptr) {
        cairo_surface_destroy(backgroundImage);
        backgroundImage = nullptr;
    }

    if (themeTransitionTimer > 0) {
        g_source_remove(themeTransitionTimer);
        themeTransitionTimer = 0;
    }

if (lineClearAnimationTimer > 0) {
    g_source_remove(lineClearAnimationTimer);
    lineClearAnimationTimer = 0;
}

if (smoothMovementTimer > 0) {
    g_source_remove(smoothMovementTimer);
    smoothMovementTimer = 0;
}

    // Clean up any background images from ZIP
    cleanupBackgroundImages();

if (fireworksTimer > 0) {
    g_source_remove(fireworksTimer);
    fireworksTimer = 0;
}

if (trailUpdateTimer > 0) {
    g_source_remove(trailUpdateTimer);
    trailUpdateTimer = 0;
}

}

void TetrimoneBoard::createBlockTrail() {
    if (!trailsEnabled || retroModeActive || !currentPiece) return;
    
    // Don't create trails if game is paused or over
    if (isPaused() || isGameOver()) return;
    
    auto now = std::chrono::high_resolution_clock::now();
    auto timeSinceLastTrail = std::chrono::duration<double, std::milli>(now - lastTrailTime).count();
    
    // Only create trails if enough time has passed
    if (timeSinceLastTrail < TRAIL_SPAWN_DELAY) return;
    lastTrailTime = now;
    
    // Create a new trail segment
    BlockTrail trail;
    trail.x = currentPiece->getX();
    trail.y = currentPiece->getY();
    trail.rotation = currentPiece->getRotation();
    trail.pieceType = currentPiece->getType();
    trail.shape = currentPiece->getShape();
    trail.color = currentPiece->getColor();
    trail.maxLife = trailDuration; // Use configurable duration
    trail.life = trail.maxLife;
    trail.alpha = trailOpacity; // Use configurable opacity
    
    blockTrails.push_back(trail);
    
    // Remove old trails if we have too many
    while (blockTrails.size() > maxTrailSegments) {
        blockTrails.erase(blockTrails.begin());
    }
    
    // Start update timer if not running
    if (trailUpdateTimer == 0) {
        trailUpdateTimer = g_timeout_add(TRAIL_UPDATE_INTERVAL,
            [](gpointer userData) -> gboolean {
                TetrimoneBoard* board = static_cast<TetrimoneBoard*>(userData);
                board->updateBlockTrails();
                
                // Force redraw
                if (board->app) {
                    gtk_widget_queue_draw(board->app->gameArea);
                }
                
                return TRUE; // Keep timer running
            }, this);
    }
}

void TetrimoneBoard::updateBlockTrails() {
    if (!trailsEnabled) {
        blockTrails.clear();
        if (trailUpdateTimer > 0) {
            g_source_remove(trailUpdateTimer);
            trailUpdateTimer = 0;
        }
        return;
    }
    
    double deltaTime = TRAIL_UPDATE_INTERVAL / 1000.0; // Convert to seconds
    
    // Update existing trail segments
    for (auto it = blockTrails.begin(); it != blockTrails.end();) {
        BlockTrail& trail = *it;
        
        // Update life and alpha
        trail.life -= deltaTime;
        trail.alpha = (trail.life / trail.maxLife) * trailOpacity; // Use configurable opacity
        
        // Remove dead trails
        if (trail.life <= 0.0 || trail.alpha <= 0.05) {
            it = blockTrails.erase(it);
        } else {
            ++it;
        }
    }
    
    // Stop timer if no trails left
    if (blockTrails.empty() && trailUpdateTimer > 0) {
        g_source_remove(trailUpdateTimer);
        trailUpdateTimer = 0;
    }
}


void TetrimoneBoard::startFireworksAnimation(int linesCleared) {
    if (linesCleared != 4) return; // Only for Tetrimone (4 lines)
    
    fireworksActive = true;
    fireworksType = 1; // Tetrimone fireworks
    fireworkParticles.clear();
    fireworksStartTime = std::chrono::high_resolution_clock::now();
    
    // Create multiple firework bursts across the cleared lines
    for (int i = 0; i < 5; i++) {
        double x = (rng() % GRID_WIDTH) * BLOCK_SIZE + BLOCK_SIZE / 2;
        double y = (rng() % 4 + GRID_HEIGHT - 8) * BLOCK_SIZE + BLOCK_SIZE / 2;
        
        // Use theme colors for fireworks
        int colorIndex = rng() % 7; // Use tetrimone block colors
        std::array<double, 3> color = TETRIMONEBLOCK_COLOR_THEMES[currentThemeIndex][colorIndex];
        
        createFireworkBurst(x, y, color, 15 + rng() % 10);
    }
    
    // Start animation timer
    if (fireworksTimer > 0) {
        g_source_remove(fireworksTimer);
    }
    
    fireworksTimer = g_timeout_add(16, // ~60 FPS
        [](gpointer userData) -> gboolean {
            TetrimoneBoard* board = static_cast<TetrimoneBoard*>(userData);
            board->updateFireworksAnimation();
            return board->isFireworksActive();
        }, 
        this);
}

void TetrimoneBoard::createFireworkBurst(double centerX, double centerY, 
                                        const std::array<double, 3>& baseColor, 
                                        int particleCount) {
    for (int i = 0; i < particleCount; i++) {
        FireworkParticle particle;
        
        // Random angle and speed
        double angle = (2.0 * M_PI * i) / particleCount + (rng() % 100 - 50) * 0.01;
        double speed = 2.0 + (rng() % 100) * 0.03;
        
        particle.x = centerX;
        particle.y = centerY;
        particle.vx = cos(angle) * speed;
        particle.vy = sin(angle) * speed;
        particle.life = 1.0;
        particle.maxLife = 1.0 + (rng() % 100) * 0.01; // Slight variation
        particle.size = 3.0 + (rng() % 3);
        particle.gravity = 0.1 + (rng() % 5) * 0.01;
        particle.fade = 0.008 + (rng() % 5) * 0.001;
        
        // Color variation
        particle.color = baseColor;
        for (int c = 0; c < 3; c++) {
            particle.color[c] += (rng() % 40 - 20) * 0.01;
            particle.color[c] = std::max(0.0, std::min(1.0, particle.color[c]));
        }
        
        fireworkParticles.push_back(particle);
    }
}

void TetrimoneBoard::updateFireworksAnimation() {
    auto now = std::chrono::high_resolution_clock::now();
    auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(now - fireworksStartTime).count();
    
    // Add new bursts over time for more spectacular effect
    if (elapsed > 200 && elapsed < 1500 && (elapsed % 300) < 50) {
        double x = (rng() % GRID_WIDTH) * BLOCK_SIZE + BLOCK_SIZE / 2;
        double y = (rng() % 6 + GRID_HEIGHT - 10) * BLOCK_SIZE + BLOCK_SIZE / 2;
        
        int colorIndex = rng() % 7;
        std::array<double, 3> color = TETRIMONEBLOCK_COLOR_THEMES[currentThemeIndex][colorIndex];
        
        createFireworkBurst(x, y, color, 12 + rng() % 8);
    }
    
    // Update existing particles
    for (auto it = fireworkParticles.begin(); it != fireworkParticles.end();) {
        FireworkParticle& p = *it;
        
        // Update physics
        p.x += p.vx;
        p.y += p.vy;
        p.vy += p.gravity; // Apply gravity
        p.life -= p.fade;
        
        // Add some air resistance
        p.vx *= 0.98;
        p.vy *= 0.98;
        
        // Remove dead particles
        if (p.life <= 0.0) {
            it = fireworkParticles.erase(it);
        } else {
            ++it;
        }
    }
    
    // FORCE REDRAW - This is crucial!
    if (app) {
        gtk_widget_queue_draw(app->gameArea);
    }
    
    // End animation when time is up or no particles left
    if (elapsed >= FIREWORKS_DURATION || fireworkParticles.empty()) {
        fireworksActive = false;
        fireworkParticles.clear();
        if (fireworksTimer > 0) {
            g_source_remove(fireworksTimer);
            fireworksTimer = 0;
        }
    }
}

bool TetrimoneBoard::movePiece(int dx, int dy) {
    if (gameOver || paused)
        return false;

    int oldX = currentPiece->getX();
    int oldY = currentPiece->getY();
    
    currentPiece->move(dx, dy);

    if (checkCollision(*currentPiece)) {
        currentPiece->move(-dx, -dy); // Move back if collision
        return false;
    }
    
    // Create block trail only for horizontal movement (less intrusive)
    if (trailsEnabled && !retroModeActive && dx != 0) {
        createBlockTrail();
    }
    
    // Start smooth movement animation
    startSmoothMovement(oldX, oldY);
    
    return true;
}


bool TetrimoneBoard::rotatePiece(bool clockwise) {
    if (gameOver || paused)
        return false;

    currentPiece->rotate(clockwise);

    if (checkCollision(*currentPiece)) {
        currentPiece->rotate(!clockwise); // Rotate back in opposite direction
        return false;
    }

    if (trailsEnabled && !retroModeActive) {
         createBlockTrail();
    }

    return true;
}

bool TetrimoneBoard::checkCollision(const TetrimoneBlock &piece) const {
  auto shape = piece.getShape();
  int pieceX = piece.getX();
  int pieceY = piece.getY();

  for (size_t y = 0; y < shape.size(); ++y) {
    for (size_t x = 0; x < shape[y].size(); ++x) {
      if (shape[y][x] == 1) {
        int gridX = pieceX + x;
        int gridY = pieceY + y;

        // Check boundaries
        if (gridX < 0 || gridX >= GRID_WIDTH || gridY >= GRID_HEIGHT) {
          return true;
        }

        // Check collision with placed blocks
        if (gridY >= 0 && grid[gridY][gridX] != 0) {
          return true;
        }
      }
    }
  }
  return false;
}

void TetrimoneBoard::lockPiece() {
  auto shape = currentPiece->getShape();
  int pieceX = currentPiece->getX();
  int pieceY = currentPiece->getY();
  int pieceType = currentPiece->getType();

  // Play drop sound when piece locks into place
  playSound(GameSoundEvent::Drop);

  for (size_t y = 0; y < shape.size(); ++y) {
    for (size_t x = 0; x < shape[y].size(); ++x) {
      if (shape[y][x] == 1) {
        int gridX = pieceX + x;
        int gridY = pieceY + y;

        if (gridY >= 0 && gridY < GRID_HEIGHT && gridX >= 0 &&
            gridX < GRID_WIDTH) {
          grid[gridY][gridX] =
              pieceType + 1; // +1 so that empty is 0 and pieces are 1-7
        }
      }
    }
  }
}

int TetrimoneBoard::clearLines() {
  std::vector<int> linesToClear;
  int currentlevel = (this->linesCleared / 10) + initialLevel;
  
  // Check each row from bottom to top to find full lines
  for (int y = GRID_HEIGHT - 1; y >= 0; --y) {
    bool isFullLine = true;
    
    for (int x = 0; x < GRID_WIDTH; ++x) {
      if (grid[y][x] == 0) {
        isFullLine = false;
        break;
      }
    }
    
    if (isFullLine) {
      linesToClear.push_back(y);
    }
  }
  
  int linesCleared = linesToClear.size();
  
  if (linesCleared > 0) {
    // Start the line clearing animation instead of immediately removing lines
    startLineClearAnimation(linesToClear);
    // Show propaganda message in retro mode
    if (retroModeActive) {
      // Select a random propaganda message
      std::uniform_int_distribution<int> dist(0, PROPAGANDA_MESSAGES.size() - 1);
      int msgIndex = dist(rng);
      
      // For 4-line clears (Tetrimone), show a special message
      std::string message;
      if (linesCleared == 4) {
          message = TETRIMONE_EXCELLENCE_MESSAGE;
      } else {
          message = PROPAGANDA_MESSAGES[msgIndex];
      }
      
      // Output to console for debugging
      std::cout << message << std::endl;

      // Display message in the GUI
      currentPropagandaMessage = message;
      showPropagandaMessage = true;
      
      // Cancel existing timer if any
      if (propagandaTimerId > 0) {
          g_source_remove(propagandaTimerId);
      }
      
      // Set timer to hide message after duration
      propagandaTimerId = g_timeout_add(propagandaMessageDuration, 
          [](gpointer userData) -> gboolean {
              TetrimoneBoard* board = static_cast<TetrimoneBoard*>(userData);
              board->showPropagandaMessage = false;
              board->propagandaTimerId = 0;
              return FALSE; // Don't repeat the timer
          }, 
          this);
          
      // Set timer for pulsing animation effect
      propagandaMessageScale = 0.7; // Start smaller and grow
      propagandaScalingUp = true;
      if (propagandaScaleTimerId > 0) {
          g_source_remove(propagandaScaleTimerId);
      }
      propagandaScaleTimerId = g_timeout_add(50, 
          [](gpointer userData) -> gboolean {
              TetrimoneBoard* board = static_cast<TetrimoneBoard*>(userData);
              if (board->showPropagandaMessage) {
                  // Update scale for pulsing effect
                  if (board->propagandaScalingUp) {
                      board->propagandaMessageScale += 0.04;
                      if (board->propagandaMessageScale >= 1.2) {
                          board->propagandaScalingUp = false;
                      }
                  } else {
                      board->propagandaMessageScale -= 0.04;
                      if (board->propagandaMessageScale <= 0.8) {
                          board->propagandaScalingUp = true;
                      }
                  }
                  return TRUE; // Continue the timer
              }
              // Stop the timer if message is no longer showing
              board->propagandaScaleTimerId = 0;
              return FALSE;
          }, 
          this);
    }
    else if (patrioticModeActive) {
        // Select a random freedom message
        std::uniform_int_distribution<int> dist(0, AMERICAN_PROPAGANDA_MESSAGES.size() - 1);
        int msgIndex = dist(rng);
        
        // For 4-line clears (Tetrimone), show a special patriotic message
        std::string message;
        if (linesCleared == 4) {
            message = AMERICAN_EXCELLENCE_MESSAGE;
        } else {
            message = AMERICAN_PROPAGANDA_MESSAGES[msgIndex];
        }
        
        // Output to console for debugging
        std::cout << message << std::endl;
        // Display message in the GUI
        currentPropagandaMessage = message;
        showPropagandaMessage = true;
        
        // Cancel existing timer if any
        if (propagandaTimerId > 0) {
            g_source_remove(propagandaTimerId);
        }
        
        // Set timer to hide message after duration
        propagandaTimerId = g_timeout_add(propagandaMessageDuration, 
            [](gpointer userData) -> gboolean {
                TetrimoneBoard* board = static_cast<TetrimoneBoard*>(userData);
                board->showPropagandaMessage = false;
                board->propagandaTimerId = 0;
                return FALSE; // Don't repeat the timer
            }, 
            this);
            
        // Set timer for patriotic pulsing animation effect
        propagandaMessageScale = 0.8; // Start slightly smaller for freedom effect
        propagandaScalingUp = true;
        if (propagandaScaleTimerId > 0) {
            g_source_remove(propagandaScaleTimerId);
        }
        propagandaScaleTimerId = g_timeout_add(60, // Slightly slower for more dignified American pulse
            [](gpointer userData) -> gboolean {
                TetrimoneBoard* board = static_cast<TetrimoneBoard*>(userData);
                if (board->showPropagandaMessage) {
                    // Update scale for patriotic pulsing effect
                    if (board->propagandaScalingUp) {
                        board->propagandaMessageScale += 0.03; // Slightly gentler scaling
                        if (board->propagandaMessageScale >= 1.15) { // Less extreme scaling
                            board->propagandaScalingUp = false;
                        }
                    } else {
                        board->propagandaMessageScale -= 0.03;
                        if (board->propagandaMessageScale <= 0.85) { // More conservative minimum
                            board->propagandaScalingUp = true;
                        }
                    }
                    return TRUE; // Continue the timer
                }
                // Stop the timer if message is no longer showing
                board->propagandaScaleTimerId = 0;
                return FALSE;
            }, 
            this);
    }

    // Play appropriate sound based on number of lines cleared
    if (linesCleared == 4) {
      playSound(GameSoundEvent::Excellent); // Play Tetrimone/Excellent sound for 4 lines
      heatLevel+=0.4;
      startFireworksAnimation(linesCleared);
    } else if (linesCleared > 0) {
      playSound(GameSoundEvent::Clear); // Play normal clear sound for 1-3 lines
      if (linesCleared == 1) {
        playSound(GameSoundEvent::Single);
        heatLevel+=0.1;
      }
      if (linesCleared == 2) {
        playSound(GameSoundEvent::Double);
        heatLevel+=0.2;

      }
      if (linesCleared == 3) {
        playSound(GameSoundEvent::Triple);
        heatLevel+=0.3;

      }
    }
    if (heatLevel>1.0) {
         heatLevel=1.0;
    }
    // Classic Tetrimone scoring
    int baseScore = 0;
    switch (linesCleared) {
    case 1:
      baseScore = 40 * level;
      break;
    case 2:
      baseScore = 100 * level;
      break;
    case 3:
      baseScore = 300 * level;
      break;
    case 4:
      baseScore = 1200 * level;
      break;
    }

    // Calculate sequence bonus
    int sequenceBonus = 0;
    if (linesCleared > 0) {
      if (lastClearCount > 0) {
        // Player cleared lines in consecutive moves
        consecutiveClears++;
        maxConsecutiveClears = std::max(maxConsecutiveClears, consecutiveClears);

        // Bonus increases with each consecutive clear
        sequenceBonus = baseScore * (consecutiveClears * 0.1); // 10% bonus per consecutive clear
        if (consecutiveClears>=2) {
            heatLevel += 0.1 * (consecutiveClears-1);
            //printf("Heat level increased by %f due to sequence\n", 0.1 * (consecutiveClears-1));
            if(heatLevel>=1.0) {
                 heatLevel=1.0;
            } 
        }        
        // Extra bonus for maintaining the same number of lines cleared
        if (linesCleared == lastClearCount) {
          sequenceBonus += baseScore * 0.2; // 20% bonus for consistent clears
          playSound(retroModeActive ? GameSoundEvent::LevelUpRetro : GameSoundEvent::LevelUp); // Special sound for consistent sequence
        }

        // Notify player about sequence
        sequenceActive = true;
      } else {
        // First clear after a move with no clears
        consecutiveClears = 1;
        sequenceActive = false;
      }

      lastClearCount = linesCleared;
    } else {
      // Reset sequence when a move doesn't clear any lines
      consecutiveClears = 0;
      lastClearCount = 0;
      sequenceActive = false;
    }

    // Apply total score with bonus
    score += baseScore + sequenceBonus;

    // Update total lines cleared
    this->linesCleared += linesCleared;

    // Update level every 10 lines
    level = (this->linesCleared / 10) + initialLevel;
  } else {
    // No lines cleared in this move
    lastClearCount = 0;
    consecutiveClears = 0;
    sequenceActive = false;
  }

if (level > currentlevel) {
    updateHeat();
    playSound(retroModeActive ? GameSoundEvent::LevelUpRetro : GameSoundEvent::LevelUp);
    if (junkLinesPerLevel > 0) {
        addJunkLinesFromBottom(junkLinesPerLevel);
    }
    
    // Only change theme if retro mode is not enabled
    if (!retroModeActive  && !patrioticModeActive) {
        // Calculate next theme with wrap-around
        int nextTheme = (currentThemeIndex + 1) % NUM_COLOR_THEMES;
        
        // Start smooth theme transition instead of immediate change
        startThemeTransition(nextTheme);
        
        // Add background transition if using background zip
        if (useBackgroundZip && !backgroundImages.empty()) {
            startBackgroundTransition();
        }
    }
    else if (patrioticModeActive) {
        if (useBackgroundZip && !backgroundImages.empty()) {
            startBackgroundTransition();
        }
    }    
}

  return linesCleared;
}

void TetrimoneBoard::generateNewPiece() {
  // Move first next piece to current
  if (!nextPieces[0]) {
    // First time initialization - create all next pieces
    for (int i = 0; i < 3; i++) {
      // Use the existing piece generation logic to create each piece
      std::vector<int> validPieces;
        // Otherwise use the regular minBlockSize rules
        switch (minBlockSize) {
        case 1: // All pieces
          for (int j = 0; j < 14; ++j) {
            validPieces.push_back(j);
          }
          break;
        case 2: // Triomones and Tetromones
          for (int j = 0; j <= 10; ++j) {
            validPieces.push_back(j);
          }
          break;
        case 3: // Tetromones only
          for (int j = 0; j <= 6; ++j) {
            validPieces.push_back(j);
          }
          break;
        case 4: // Tetromones only, but ensure at least 4 blocks
          for (int j = 0; j <= 6; ++j) {
            int blockCount = 0;
            for (const auto &row : TETRIMONEBLOCK_SHAPES[j][0]) {
              for (int cell : row) {
                if (cell == 1)
                  blockCount++;
              }
            }
            // Only add if block count is exactly 4
            if (blockCount == 4) {
              validPieces.push_back(j);
            }
          }
          break;
        default:
          // Fallback to standard tetromones
          for (int j = 0; j <= 6; ++j) {
            validPieces.push_back(j);
          }
          break;
        }

      // If no valid pieces found, fallback to standard Tetrimones
      if (validPieces.empty()) {
        for (int j = 0; j <= 6; ++j) {
          validPieces.push_back(j);
        }
      }

      // Use uniform distribution over valid pieces
      std::uniform_int_distribution<int> dist(0, validPieces.size() - 1);
      int nextIndex = dist(rng);
      int nextType = validPieces[nextIndex];

      nextPieces[i] = std::make_unique<TetrimoneBlock>(nextType);
    }

    // Create the current piece
    currentPiece = std::make_unique<TetrimoneBlock>(nextPieces[0]->getType());
  } else {
    // Move first next piece to current
    currentPiece = std::move(nextPieces[0]);

    // Shift remaining pieces
    for (int i = 0; i < 2; i++) {
      nextPieces[i] = std::move(nextPieces[i + 1]);
    }

    // Generate a new piece for the last position
    std::vector<int> validPieces;

      // Otherwise use the regular minBlockSize rules
      switch (minBlockSize) {
      case 1: // All pieces
        for (int i = 0; i < 14; ++i) {
          validPieces.push_back(i);
        }
        break;
      case 2: // Triomones and Tetromones
        for (int i = 0; i <= 10; ++i) {
          validPieces.push_back(i);
        }
        break;
      case 3: // Tetromones only
        for (int i = 0; i <= 6; ++i) {
          validPieces.push_back(i);
        }
        break;
      case 4: // Tetromones only, but ensure at least 4 blocks
        for (int i = 0; i <= 6; ++i) {
          int blockCount = 0;
          for (const auto &row : TETRIMONEBLOCK_SHAPES[i][0]) {
            for (int cell : row) {
              if (cell == 1)
                blockCount++;
            }
          }
          // Only add if block count is exactly 4
          if (blockCount == 4) {
            validPieces.push_back(i);
          }
        }
        break;
      default:
        // Fallback to standard tetromones
        for (int i = 0; i <= 6; ++i) {
          validPieces.push_back(i);
        }
        break;
      }

    // If no valid pieces found, fallback to standard Tetromones
    if (validPieces.empty()) {
      for (int i = 0; i <= 6; ++i) {
        validPieces.push_back(i);
      }
    }

    // Use uniform distribution over valid pieces
    std::uniform_int_distribution<int> dist(0, validPieces.size() - 1);
    int nextIndex = dist(rng);
    int nextType = validPieces[nextIndex];

    nextPieces[2] = std::make_unique<TetrimoneBlock>(nextType);
  }

  // Check if the new piece collides immediately - game over
  if (checkCollision(*currentPiece)) {
    gameOver = true;
  }
}

void TetrimoneBoard::updateGame() {
  if (gameOver || paused || splashScreenActive)
    return;

  // Existing update code...
  if (!movePiece(0, 1)) {
    lockPiece();
    clearLines();
    generateNewPiece();
  }
}

void TetrimoneBoard::hardDrop() {
  if (gameOver || paused)
    return;

  // Move the piece down until collision
  while (movePiece(0, 1)) {
    // Give extra points for hard drop
    score += 2;
  }

  // Lock the piece
  lockPiece();

  // Clear any full lines
  clearLines();

  // Generate a new piece
  generateNewPiece();
}

int TetrimoneBoard::getGridValue(int x, int y) const {
  if (x < 0 || x >= GRID_WIDTH || y < 0 || y >= GRID_HEIGHT) {
    return 0;
  }
  return grid[y][x];
}

void TetrimoneBoard::restart() {
  // Clear the grid
  for (auto &row : grid) {
    std::fill(row.begin(), row.end(), 0);
  }
  heatLevel = 0.5f;
  heatDecayTimer = 0;
  // Reset game state
  score = 0;
  level = initialLevel; // Use initialLevel instead of hardcoded 1
  linesCleared = 0;
  gameOver = false;
  paused = false;
  splashScreenActive = true; // Show splash screen on restart

  // Reset pieces
  currentPiece.reset();
  for (auto &piece : nextPieces) {
    piece.reset();
  }

  // Generate junk lines if percentage > 0
  if (junkLinesPercentage > 0) {
    generateJunkLines(junkLinesPercentage);
  }

  // Generate new pieces
  generateNewPiece();

  // Select a random background if using background images from ZIP
  if (useBackgroundZip && !backgroundImages.empty()) {
    // Start a smooth background transition
    startBackgroundTransition();
  }

  consecutiveClears = 0;
  maxConsecutiveClears = 0;
  lastClearCount = 0;
  sequenceActive = false;
  highScoreAlreadyProcessed = false;
}

gboolean onKeyPress(GtkWidget *widget, GdkEventKey *event, gpointer data) {
  TetrimoneApp *app = static_cast<TetrimoneApp *>(data);
  TetrimoneBoard *board = app->board;
  
  // Handle key press events
  if (event->type == GDK_KEY_PRESS) {
    // Handle space to dismiss splash screen first
    if (event->keyval == GDK_KEY_space && board->isSplashScreenActive()) {
      board->dismissSplashScreen();
      gtk_widget_queue_draw(app->gameArea);
      gtk_widget_queue_draw(app->nextPieceArea);
      updateLabels(app);
      return TRUE;
    }

    // Handle game control keys only when game is active
    if (!board->isPaused() && !board->isGameOver() &&
        !board->isSplashScreenActive()) {
      switch (event->keyval) {
      case GDK_KEY_Left:
      case GDK_KEY_a:
      case GDK_KEY_A:
        // Start left acceleration if not already active
        if (!keyLeftPressed) {
          keyLeftPressed = true;
          keyLeftCount = 0;
          keyLeftDelay = 150; // Reset to initial delay
          
          // Immediate move
          board->movePiece(-1, 0);
          
          // Start the repeat timer
          if (keyLeftTimer == 0) {
            keyLeftTimer = g_timeout_add(keyLeftDelay, onKeyLeftTick, app);
          }
        }
        break;


      case GDK_KEY_Right:
      case GDK_KEY_d:
      case GDK_KEY_D:
        // Start right acceleration if not already active
        if (!keyRightPressed) {
          keyRightPressed = true;
          keyRightCount = 0;
          keyRightDelay = 150; // Reset to initial delay
          
          // Immediate move
          board->movePiece(1, 0);
          
          // Start the repeat timer
          if (keyRightTimer == 0) {
            keyRightTimer = g_timeout_add(keyRightDelay, onKeyRightTick, app);
          }
        }
        break;

      case GDK_KEY_Down:
      case GDK_KEY_s:
      case GDK_KEY_S:
        // Start down acceleration if not already active
        if (!keyDownPressed) {
          keyDownPressed = true;
          keyDownCount = 0;
          keyDownDelay = 150; // Reset to initial delay
          
          // Immediate move
          board->movePiece(0, 1);
          
          // Start the repeat timer
          if (keyDownTimer == 0) {
            keyDownTimer = g_timeout_add(keyDownDelay, onKeyDownTick, app);
          }
        }
        break;

      case GDK_KEY_Up:
      case GDK_KEY_w:
      case GDK_KEY_W:
        board->rotatePiece(true);
        break;

      case GDK_KEY_z:
      case GDK_KEY_Z:
        board->rotatePiece(false);
        break;

      case GDK_KEY_space:
        board->hardDrop();
        break;
      }
    }

    // Handle global control keys regardless of game state
    switch (event->keyval) {
    case GDK_KEY_p:
    case GDK_KEY_P:
      if (!board->isSplashScreenActive()) {
        onPauseGame(GTK_MENU_ITEM(app->pauseMenuItem), app);
      }
      break;
    case GDK_KEY_m:
    case GDK_KEY_M:
      if (app->board->musicPaused) {
        app->board->resumeBackgroundMusic();
      } else {
        app->board->pauseBackgroundMusic();
      }
      break;

    case GDK_KEY_n:
    case GDK_KEY_N:
      if (board->isPaused()) {
        onRestartGame(GTK_MENU_ITEM(app->restartMenuItem), app);
      }
      break;

    case GDK_KEY_q:
    case GDK_KEY_Q:
      if (board->isPaused()) {
        onQuitGame(GTK_MENU_ITEM(NULL), app);
      }
      break;

    case GDK_KEY_r:
    case GDK_KEY_R:
      if (board->isGameOver()) {
        onRestartGame(GTK_MENU_ITEM(app->restartMenuItem), app);
      }
      break;

    case GDK_KEY_Escape:
      // Emergency unpause if somehow stuck
      if (board->isPaused() && !board->isGameOver()) {
        onPauseGame(GTK_MENU_ITEM(app->pauseMenuItem), app);
      }
      break;

case GDK_KEY_period:
    // Toggle retro mode with just the period key
    {
        // Save the current theme index when entering retro mode
        static int savedThemeIndex = 0;
        
        // Toggle the retro mode flag
        board->retroModeActive = !board->retroModeActive;
        board->patrioticModeActive = false;       


if (board->retroModeActive) {
    // Store current theme before switching to retro mode
    savedThemeIndex = currentThemeIndex;
    gtk_window_set_title(GTK_WINDOW(app->window), "БЛОЧНАЯ РЕВОЛЮЦИЯ");
    // Set to Soviet Retro theme (last theme in the list)
    currentThemeIndex = NUM_COLOR_THEMES - 1;
    gtk_label_set_markup(GTK_LABEL(app->difficultyLabel),
                app->board->getDifficultyText(app->difficulty).c_str());
    gtk_label_set_markup(GTK_LABEL(app->controlsHeaderLabel), "<b>ПАРТИЙНЫЕ ДИРЕКТИВЫ</b>");
    // Disable background image
    if (board->isUsingBackgroundImage() || board->isUsingBackgroundZip()) {
        board->setUseBackgroundImage(false);
        board->setUseBackgroundZip(false);
        gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(app->backgroundToggleMenuItem), FALSE);
    }
    
    // Play a special sound effect
    board->playSound(GameSoundEvent::Select);
} else {
    gtk_window_set_title(GTK_WINDOW(app->window), "Tetrimone");
    // Restore previous theme
    currentThemeIndex = savedThemeIndex;
    gtk_label_set_markup(GTK_LABEL(app->difficultyLabel),
                app->board->getDifficultyText(app->difficulty).c_str());
    gtk_label_set_markup(GTK_LABEL(app->controlsHeaderLabel), "<b>Controls</b>");
    // Re-enable background if it was enabled before
    if (board->getBackgroundImage() != nullptr) {
        board->setUseBackgroundImage(true);
        // Also restore the useBackgroundZip flag if background images were loaded from ZIP
        if (!board->backgroundZipPath.empty()) {
            board->setUseBackgroundZip(true);
            // Trigger a background transition for a nice effect when returning from retro mode
            board->startBackgroundTransition();
        }
        gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(app->backgroundToggleMenuItem), TRUE);
    }
    
    // Re-enable music if sound is enabled
    if (!app->backgroundMusicPlaying && board->sound_enabled_) {
        board->resumeBackgroundMusic();
        app->backgroundMusicPlaying = true;
    }
    
    std::cout << "Retro mode OFF" << std::endl;
}
        
        // Update controls text
        gtk_label_set_text(GTK_LABEL(app->controlsLabel), 
            board->retroModeActive ? 
            "Управление клавиатурой:\n"
            "• Влево/Вправо/A/D: Перемещение блока\n"
            "• Вверх/W: Поворот по часовой стрелке\n"
            "• Z: Поворот против часовой стрелки\n"
            "• Вниз/S: Мягкое падение\n"
            "• Пробел: Быстрое падение\n"
            "• P: Пауза/Продолжение игры\n"
            "• R: Перезапуск игры\n"
            "• M: Переключение музыки\n\n"
            "Поддержка контроллера доступна.\n"
            "Настройка в меню Управление." :
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
            "Configure in Controls menu.");
        
        // Redraw game area to show theme change
        gtk_widget_queue_draw(app->gameArea);
        gtk_widget_queue_draw(app->nextPieceArea);
    }
        app->board->pauseBackgroundMusic();
        app->board->resumeBackgroundMusic();

    break;


case GDK_KEY_comma:
    // Toggle patriotic mode with comma key
    {
        // Save the current theme index when entering patriotic mode
        static int savedThemeIndex = 0;
        
        // Toggle the patriotic mode flag
        board->retroModeActive = false;
        board->patrioticModeActive = !board->patrioticModeActive;       

        
        if (board->patrioticModeActive) {
            // Store current theme before switching to patriotic mode
            savedThemeIndex = currentThemeIndex;
            gtk_window_set_title(GTK_WINDOW(app->window), "FREEDOM BLOCKS - GOD BLESS AMERICA");
            // Set to American Patriotic theme (theme index 31)
            currentThemeIndex = NUM_COLOR_THEMES - 2;
            gtk_label_set_markup(GTK_LABEL(app->difficultyLabel),
                        app->board->getDifficultyText(app->difficulty).c_str());
            gtk_label_set_markup(GTK_LABEL(app->controlsHeaderLabel), "<b>FREEDOM COMMANDS</b>");
            
            // Re-enable background if it was enabled before
            if (board->getBackgroundImage() != nullptr) {
                board->setUseBackgroundImage(true);
                // Also restore the useBackgroundZip flag if background images were loaded from ZIP
                if (!board->backgroundZipPath.empty()) {
                    board->setUseBackgroundZip(true);
                    // Trigger a background transition for a nice effect when returning from patriotic mode
                }
                gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(app->backgroundToggleMenuItem), TRUE);
            }
            board->startBackgroundTransition();
            
            // Play a special patriotic sound effect
            board->playSound(GameSoundEvent::Select);
            std::cout << "Patriotic mode ON - FREEDOM ACTIVATED!" << std::endl;
        } else {
            gtk_window_set_title(GTK_WINDOW(app->window), "Tetrimone");
            // Restore previous theme
            currentThemeIndex = savedThemeIndex;
            gtk_label_set_markup(GTK_LABEL(app->difficultyLabel),
                        app->board->getDifficultyText(app->difficulty).c_str());
            gtk_label_set_markup(GTK_LABEL(app->controlsHeaderLabel), "<b>Controls</b>");
            
            // Re-enable background if it was enabled before
            if (board->getBackgroundImage() != nullptr) {
                board->setUseBackgroundImage(true);
                // Also restore the useBackgroundZip flag if background images were loaded from ZIP
                if (!board->backgroundZipPath.empty()) {
                    board->setUseBackgroundZip(true);
                    // Trigger a background transition for a nice effect when returning from patriotic mode
                }
                gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(app->backgroundToggleMenuItem), TRUE);
            }
            board->startBackgroundTransition();
            
            // Re-enable music if sound is enabled
            if (!app->backgroundMusicPlaying && board->sound_enabled_) {
                board->resumeBackgroundMusic();
                app->backgroundMusicPlaying = true;
            }
            
            std::cout << "Patriotic mode OFF" << std::endl;
        }
        
        // Update controls text
        gtk_label_set_text(GTK_LABEL(app->controlsLabel), 
            board->patrioticModeActive ? 
            "Freedom Controls:\n"
            "• Left/Right/A/D: Exercise your right to move blocks\n"
            "• Up/W: Rotate clockwise (like freedom)\n"
            "• Z: Rotate counter-clockwise (constitutional right)\n"
            "• Down/S: Soft drop (gentle like democracy)\n"
            "• Space: Hard drop (decisive like America)\n"
            "• P: Pause/Resume (work-life balance)\n"
            "• R: Restart (second chances, American dream)\n"
            "• M: Toggle music (freedom of choice)\n\n"
            "Controller support available.\n"
            "Configure in Controls menu.\n"
            "🇺🇸 GOD BLESS AMERICA! 🦅" :
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
            "Configure in Controls menu.");
        
        // Redraw game area to show theme change
        gtk_widget_queue_draw(app->gameArea);
        gtk_widget_queue_draw(app->nextPieceArea);
    }
        app->board->pauseBackgroundMusic();
        app->board->resumeBackgroundMusic();

    break;

    default:
      // Don't return FALSE here as it prevents redrawing
      break;
    }
  } 
  // Handle key release events
  else if (event->type == GDK_KEY_RELEASE) {
    switch (event->keyval) {
      case GDK_KEY_Down:
      case GDK_KEY_s:
      case GDK_KEY_S:
        // Stop down acceleration
        keyDownPressed = false;
        
        // Cancel the timer if it exists
        if (keyDownTimer > 0) {
          g_source_remove(keyDownTimer);
          keyDownTimer = 0;
        }
        break;
        
      case GDK_KEY_Left:
      case GDK_KEY_a:
      case GDK_KEY_A:
        // Stop left acceleration
        keyLeftPressed = false;
        
        // Cancel the timer if it exists
        if (keyLeftTimer > 0) {
          g_source_remove(keyLeftTimer);
          keyLeftTimer = 0;
        }
        break;
        
      case GDK_KEY_Right:
      case GDK_KEY_d:
      case GDK_KEY_D:
        // Stop right acceleration
        keyRightPressed = false;
        
        // Cancel the timer if it exists
        if (keyRightTimer > 0) {
          g_source_remove(keyRightTimer);
          keyRightTimer = 0;
        }
        break;
    }
  }

  // Always redraw and update after any key press
  gtk_widget_queue_draw(app->gameArea);
  gtk_widget_queue_draw(app->nextPieceArea);
  updateLabels(app);

  return TRUE; // Always claim we handled the key event
}


gboolean onTimerTick(gpointer data) {
  TetrimoneApp *app = static_cast<TetrimoneApp *>(data);
  TetrimoneBoard *board = app->board;

if (!board->isPaused() && !board->isSplashScreenActive() && !board->retroModeActive) {
    board->coolDown();
}

  if (!board->isPaused()) {
    board->updateGame();

    // If the game just ended after this update, check for high score
    if (board->isGameOver()) {
      if (!board->highScoreAlreadyProcessed) {
           board->highScoreAlreadyProcessed=true;
           bool isHighScore = board->checkAndRecordHighScore(app);
      
           // If it's a high score, play a special sound
           if (isHighScore) {
                board->playSound(GameSoundEvent::Excellent);
           }
           
           if (board->retroModeActive) {
               // Delay slightly for dramatic effect
               g_timeout_add(1500, [](gpointer userData) -> gboolean {
                   TetrimoneApp *app = static_cast<TetrimoneApp*>(userData);
                   showIdeologicalFailureDialog(app);
                   return FALSE; // One-time call
               }, app);
           }
           if (board->patrioticModeActive) {
               // Delay slightly for dramatic effect
               g_timeout_add(1500, [](gpointer userData) -> gboolean {
                   TetrimoneApp *app = static_cast<TetrimoneApp*>(userData);
                   showPatrioticPerformanceDialog(app);
                   return FALSE; // One-time call
               }, app);
           }

         }
    }
    gtk_widget_queue_draw(app->gameArea);
    gtk_widget_queue_draw(app->nextPieceArea);
    updateLabels(app);
  }

  if(board->retroModeActive) { board->setHeatLevel(0.5);}

  if (!board->isPaused() && !board->isGameOver() && board->retroModeActive) {
    // 1 in 1000 chance of KGB inspection
    static std::mt19937 rng(std::chrono::system_clock::now().time_since_epoch().count());
    std::uniform_int_distribution<int> dist(1, 1000);

    if (dist(rng) == 1) {
      // Pause the game briefly
      app->board->setPaused(true);
      
      // Create popup message
      GtkWidget *dialog = gtk_message_dialog_new(
          GTK_WINDOW(app->window),
          GTK_DIALOG_MODAL,
          GTK_MESSAGE_WARNING,
          GTK_BUTTONS_NONE,
          "КГБ ИНСПЕКЦИЯ В ПРОЦЕССЕ...\n(KGB INSPECTION IN PROGRESS...)");
      
      // Auto-close after 2 seconds
      g_timeout_add(2000, 
        [](gpointer user_data) -> gboolean {
          GtkWidget *dialog = static_cast<GtkWidget*>(user_data);
          gtk_widget_destroy(dialog);
          return G_SOURCE_REMOVE;
        }, 
        dialog);
      
      // Show dialog
      gtk_widget_show_all(dialog);
      
      // Resume the game after 2 seconds
      g_timeout_add(2100, 
        [](gpointer user_data) -> gboolean {
          TetrimoneApp *app = static_cast<TetrimoneApp *>(user_data);
          app->board->setPaused(false);
          return G_SOURCE_REMOVE;
        }, 
        app);
    }
  }
  else if (!board->isPaused() && !board->isGameOver() && board->patrioticModeActive) {
    // 1 in 1776 chance of Freedom Inspection (in honor of 1776!)
    static std::mt19937 rng(std::chrono::system_clock::now().time_since_epoch().count());
    std::uniform_int_distribution<int> dist(1, 1776);

    if (dist(rng) == 1) {
      // Pause the game briefly for patriotic interruption
      app->board->setPaused(true);
      
      // Create random patriotic popup messages
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
      
      // Select random message
      std::uniform_int_distribution<int> msgDist(0, 9);
      const char* selectedMessage = freedomInspections[msgDist(rng)];
      
      // Create popup message with American style
      GtkWidget *dialog = gtk_message_dialog_new(
          GTK_WINDOW(app->window),
          GTK_DIALOG_MODAL,
          GTK_MESSAGE_INFO, // Info instead of warning - more positive!
          GTK_BUTTONS_NONE,
          "%s", selectedMessage);
      
      // Auto-close after 2.5 seconds (slightly longer for American attention spans)
      g_timeout_add(2500, 
        [](gpointer user_data) -> gboolean {
          GtkWidget *dialog = static_cast<GtkWidget*>(user_data);
          gtk_widget_destroy(dialog);
          return G_SOURCE_REMOVE;
        }, 
        dialog);
      
      // Show dialog
      gtk_widget_show_all(dialog);
      
      // Resume the game after 2.6 seconds
      g_timeout_add(2600, 
        [](gpointer user_data) -> gboolean {
          TetrimoneApp *app = static_cast<TetrimoneApp *>(user_data);
          app->board->setPaused(false);
          return G_SOURCE_REMOVE;
        }, 
        app);
    }
  }
  return TRUE; // Keep the timer running
}

void updateLabels(TetrimoneApp *app) {
  TetrimoneBoard *board = app->board;

  // Update score label with Soviet-style text in retro mode
  std::string score_text;
  if (board->retroModeActive) {
    score_text = "<b>Партийная Лояльность:</b> " + std::to_string(board->getScore()) + "%";
  } else {
    score_text = "<b>Score:</b> " + std::to_string(board->getScore());
  }
  gtk_label_set_markup(GTK_LABEL(app->scoreLabel), score_text.c_str());

  // Update level label with Soviet-style text in retro mode
  std::string level_text;
  if (board->retroModeActive) {
    level_text = "<b>Пятилетка:</b> " + std::to_string(board->getLevel());
  } else {
    level_text = "<b>Level:</b> " + std::to_string(board->getLevel());
  }
  gtk_label_set_markup(GTK_LABEL(app->levelLabel), level_text.c_str());

  // Update lines label with Soviet propaganda in retro mode
  std::string lines_text;
  if (board->retroModeActive) {
    lines_text = "<b>Уничтожено врагов народа:</b> " + std::to_string(board->getLinesCleared());
  } else {
    lines_text = "<b>Lines:</b> " + std::to_string(board->getLinesCleared());
  }
  gtk_label_set_markup(GTK_LABEL(app->linesLabel), lines_text.c_str());

  // Update sequence label
  std::string sequence_text;
  if (board->isSequenceActive() && board->getConsecutiveClears() > 1) {
    if (board->retroModeActive) {
      sequence_text = "<b>Коллективная эффективность:</b> " + 
        std::to_string(board->getConsecutiveClears()) + " (Рекорд: " + 
        std::to_string(board->getMaxConsecutiveClears()) + ")";
    } else {
      sequence_text = "<b>Sequence:</b> " + std::to_string(board->getConsecutiveClears()) +
        " (Max: " + std::to_string(board->getMaxConsecutiveClears()) + ")";
    }
    // Make it stand out when active
    sequence_text = "<span foreground='#00AA00'>" + sequence_text + "</span>";
  } else {
    if (board->retroModeActive) {
      sequence_text = "<b>Коллективная эффективность:</b> " + 
        std::to_string(board->getConsecutiveClears()) + " (Рекорд: " + 
        std::to_string(board->getMaxConsecutiveClears()) + ")";
    } else {
      sequence_text = "<b>Sequence:</b> " + std::to_string(board->getConsecutiveClears()) +
        " (Max: " + std::to_string(board->getMaxConsecutiveClears()) + ")";
    }
  }
  gtk_label_set_markup(GTK_LABEL(app->sequenceLabel), sequence_text.c_str());
}

void resetUI(TetrimoneApp *app) {
  // Reset all UI elements to their initial state
  gtk_widget_queue_draw(app->gameArea);
  gtk_widget_queue_draw(app->nextPieceArea);

  // Reset labels
  gtk_label_set_markup(GTK_LABEL(app->scoreLabel), "<b>Score:</b> 0");
  gtk_label_set_markup(GTK_LABEL(app->levelLabel), "<b>Level:</b> 1");
  gtk_label_set_markup(GTK_LABEL(app->linesLabel), "<b>Lines:</b> 0");

  // Update menu state
  gtk_widget_set_sensitive(app->startMenuItem, FALSE);
  gtk_widget_set_sensitive(app->pauseMenuItem, TRUE);
  gtk_menu_item_set_label(GTK_MENU_ITEM(app->pauseMenuItem), "Pause");
}

void cleanupApp(gpointer data) {
  TetrimoneApp *app = static_cast<TetrimoneApp *>(data);

  // Stop timers first to prevent any race conditions
  if (app->timerId > 0) {
    g_source_remove(app->timerId);
    app->timerId = 0;
  }

  // Stop joystick timer before closing the joystick
  if (app->joystickTimerId > 0) {
    g_source_remove(app->joystickTimerId);
    app->joystickTimerId = 0;
  }

  // Close joystick properly
  if (app->joystick != NULL) {
    SDL_JoystickClose(app->joystick);
    app->joystick = NULL;
  }

  // Quit SDL
  if (app->joystickEnabled) {
    SDL_Quit();
    app->joystickEnabled = false;
  }

  // Delete board after all timers are stopped
  delete app->board;
  app->board = NULL;

  // Finally delete the app struct
  delete app;
}

void onScreenSizeChanged(GtkWidget *widget, GdkRectangle *allocation,
                         gpointer userData) {
  TetrimoneApp *app = static_cast<TetrimoneApp *>(userData);

  // Recalculate BLOCK_SIZE based on actual space available
  if (app->gameArea) {
    int gameWidth = gtk_widget_get_allocated_width(app->gameArea);
    int gameHeight = gtk_widget_get_allocated_height(app->gameArea);
    
    if (gameWidth > 0 && gameHeight > 0) {
      // Calculate block size to fit allocated space
      int blockSizeW = gameWidth / GRID_WIDTH;
      int blockSizeH = gameHeight / GRID_HEIGHT;
      int newBlockSize = std::min(blockSizeW, blockSizeH);
      
      // Only update if significantly different
      if (newBlockSize > 10 && abs(newBlockSize - BLOCK_SIZE) > 2) {
        BLOCK_SIZE = newBlockSize;
        printf("DEBUG: BLOCK_SIZE resized to %d\n", BLOCK_SIZE);
        
        // Recalculate board dimensions with new BLOCK_SIZE
        int newBoardWidth = GRID_WIDTH * BLOCK_SIZE;
        int newBoardHeight = GRID_HEIGHT * BLOCK_SIZE;
        
        // Update game area size to new board dimensions
        gtk_widget_set_hexpand(app->gameArea, FALSE);
        gtk_widget_set_vexpand(app->gameArea, FALSE);
        gtk_widget_set_size_request(app->gameArea, newBoardWidth, newBoardHeight);
        printf("DEBUG: Board resized to %d x %d\n", newBoardWidth, newBoardHeight);
        
        // Also update preview pieces to scale with board
        gtk_widget_set_size_request(app->nextPieceArea, BLOCK_SIZE * 5, BLOCK_SIZE * 4);
        
      }
    }
  }

  // Redraw at new size
  gtk_widget_queue_draw(app->gameArea);
  gtk_widget_queue_draw(app->nextPieceArea);
}

void onAppActivate(GtkApplication *app, gpointer userData) {
  TetrimoneApp *tetrimoneApp = new TetrimoneApp();
  tetrimoneApp->app = app;
  tetrimoneApp->board = new TetrimoneBoard();
  tetrimoneApp->board->setApp(tetrimoneApp);
  tetrimoneApp->timerId = 0;
  tetrimoneApp->dropSpeed = INITIAL_SPEED;
  tetrimoneApp->difficulty = 2; // Default to Medium

  // Calculate block size based on screen resolution
  calculateBlockSize(tetrimoneApp);

  // Create the main window
  tetrimoneApp->window = gtk_application_window_new(app);
  gtk_window_set_title(GTK_WINDOW(tetrimoneApp->window), "Tetrimone");

  setWindowIcon(GTK_WINDOW(tetrimoneApp->window));

    // Get command line arguments
    CommandLineArgs* args = static_cast<CommandLineArgs*>(
        g_object_get_data(G_OBJECT(app), "cmdline-args"));

    // Apply grid dimensions before calculating block size
    if (args && args->gridWidth != -1) {
        GRID_WIDTH = args->gridWidth;
    }
    if (args && args->gridHeight != -1) {
        GRID_HEIGHT = args->gridHeight;
    }

    // Calculate block size based on screen resolution (unless overridden)
    if (!args || args->blockSize == -1) {
        calculateBlockSize(tetrimoneApp);
    } else {
        BLOCK_SIZE = args->blockSize;
    }

  g_signal_connect(G_OBJECT(tetrimoneApp->window), "delete-event",
                   G_CALLBACK(onDeleteEvent), tetrimoneApp);

  g_signal_connect(G_OBJECT(tetrimoneApp->window), "focus-in-event",
                 G_CALLBACK(onWindowFocusChanged), tetrimoneApp);
  g_signal_connect(G_OBJECT(tetrimoneApp->window), "focus-out-event",
                 G_CALLBACK(onWindowFocusChanged), tetrimoneApp);

  // Use the calculated block size for window dimensions
  // Set FIXED window size at the red line (visible gray border)
  gtk_window_set_default_size(GTK_WINDOW(tetrimoneApp->window),
                              GRID_WIDTH * BLOCK_SIZE + 220,
                              GRID_HEIGHT * BLOCK_SIZE + 60);
  // Allow resizing for maximize button (Wayland support)
  gtk_window_set_resizable(GTK_WINDOW(tetrimoneApp->window), TRUE);

  // Create main vertical box
  GtkWidget *mainVBox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
  gtk_container_set_border_width(GTK_CONTAINER(mainVBox), 0);  // NO border
  gtk_container_add(GTK_CONTAINER(tetrimoneApp->window), mainVBox);

  // Create menu
  createMenu(tetrimoneApp);
  gtk_box_pack_start(GTK_BOX(mainVBox), tetrimoneApp->menuBar, FALSE, FALSE, 0);

  // Create main horizontal box for game contents
  tetrimoneApp->mainBox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);  // NO spacing between widgets
  gtk_container_set_border_width(GTK_CONTAINER(tetrimoneApp->mainBox), 0);  // NO border
  gtk_box_pack_start(GTK_BOX(mainVBox), tetrimoneApp->mainBox, TRUE, TRUE, 0);

  g_signal_connect(G_OBJECT(tetrimoneApp->window), "size-allocate",
                   G_CALLBACK(onScreenSizeChanged), tetrimoneApp);

  // Create the game area (drawing area)
  tetrimoneApp->gameArea = gtk_drawing_area_new();
  // Set EXACT size - the board is GRID_WIDTH * BLOCK_SIZE, nothing more
  int boardWidth = GRID_WIDTH * BLOCK_SIZE;
  int boardHeight = GRID_HEIGHT * BLOCK_SIZE;
  printf("BW %i HW %i\n", boardWidth, boardHeight);
  gtk_widget_set_size_request(tetrimoneApp->gameArea, boardWidth, boardHeight);
  printf("3 Game area is %i %i\n", boardWidth, boardHeight);

  // EXPAND on resize - allows onScreenSizeChanged to recalculate block size
  gtk_widget_set_hexpand(tetrimoneApp->gameArea, TRUE);
  gtk_widget_set_vexpand(tetrimoneApp->gameArea, TRUE);
  g_signal_connect(G_OBJECT(tetrimoneApp->gameArea), "draw",
                   G_CALLBACK(onDrawGameArea), tetrimoneApp);
  // Pack with expansion so resize events fire
  gtk_box_pack_start(GTK_BOX(tetrimoneApp->mainBox), tetrimoneApp->gameArea,
                     TRUE, TRUE, 0);

  // Create the side panel (vertical box)
  GtkWidget *sideBox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);  // NO spacing
  gtk_container_set_border_width(GTK_CONTAINER(sideBox), 0);  // NO border
  // Side panel stays fixed size
  gtk_widget_set_hexpand(sideBox, FALSE);
  gtk_widget_set_vexpand(sideBox, FALSE);
  gtk_box_pack_start(GTK_BOX(tetrimoneApp->mainBox), sideBox, FALSE, FALSE, 0);

  // Create the next piece preview frame
  GtkWidget *nextPieceFrame = gtk_frame_new("Next Piece");
  gtk_container_set_border_width(GTK_CONTAINER(nextPieceFrame), 0);  // NO border
  gtk_box_pack_start(GTK_BOX(sideBox), nextPieceFrame, FALSE, FALSE, 0);

  // Create the next piece drawing area - MUCH SMALLER
  tetrimoneApp->nextPieceArea = gtk_drawing_area_new();
  gtk_widget_set_size_request(tetrimoneApp->nextPieceArea, BLOCK_SIZE * 5,
                              BLOCK_SIZE * 4);
  g_signal_connect(G_OBJECT(tetrimoneApp->nextPieceArea), "draw",
                   G_CALLBACK(onDrawNextPiece), tetrimoneApp);
  gtk_container_add(GTK_CONTAINER(nextPieceFrame), tetrimoneApp->nextPieceArea);

  // Create score, level, and lines labels
  tetrimoneApp->scoreLabel = gtk_label_new(NULL);
  gtk_label_set_markup(GTK_LABEL(tetrimoneApp->scoreLabel), "<b>Score:</b> 0");
  gtk_widget_set_halign(tetrimoneApp->scoreLabel, GTK_ALIGN_START);
  gtk_box_pack_start(GTK_BOX(sideBox), tetrimoneApp->scoreLabel, FALSE, FALSE,
                     0);

  tetrimoneApp->levelLabel = gtk_label_new(NULL);
  gtk_label_set_markup(GTK_LABEL(tetrimoneApp->levelLabel), "<b>Level:</b> 1");
  gtk_widget_set_halign(tetrimoneApp->levelLabel, GTK_ALIGN_START);
  gtk_box_pack_start(GTK_BOX(sideBox), tetrimoneApp->levelLabel, FALSE, FALSE,
                     0);

  tetrimoneApp->linesLabel = gtk_label_new(NULL);
  gtk_label_set_markup(GTK_LABEL(tetrimoneApp->linesLabel), "<b>Lines:</b> 0");
  gtk_widget_set_halign(tetrimoneApp->linesLabel, GTK_ALIGN_START);
  gtk_box_pack_start(GTK_BOX(sideBox), tetrimoneApp->linesLabel, FALSE, FALSE,
                     0);

  tetrimoneApp->sequenceLabel = gtk_label_new(NULL);
  gtk_label_set_markup(GTK_LABEL(tetrimoneApp->sequenceLabel),
                       "<b>Sequence:</b> 0 (Max: 0)");
  gtk_widget_set_halign(tetrimoneApp->sequenceLabel, GTK_ALIGN_START);
  gtk_box_pack_start(GTK_BOX(sideBox), tetrimoneApp->sequenceLabel, FALSE,
                     FALSE, 0);

  // Add difficulty label
  tetrimoneApp->difficultyLabel = gtk_label_new(NULL);
gtk_label_set_markup(GTK_LABEL(tetrimoneApp->difficultyLabel),
                     tetrimoneApp->board->getDifficultyText(tetrimoneApp->difficulty).c_str());
                     
  gtk_widget_set_halign(tetrimoneApp->difficultyLabel, GTK_ALIGN_START);
  gtk_box_pack_start(GTK_BOX(sideBox), tetrimoneApp->difficultyLabel, FALSE,
                     FALSE, 0);

  // Add controls info
GtkWidget *controlsLabel = gtk_label_new(NULL);
if (tetrimoneApp->board->retroModeActive) {
    gtk_label_set_markup(GTK_LABEL(controlsLabel), "<b>ПАРТИЙНЫЕ ДИРЕКТИВЫ</b>");
} else {
    gtk_label_set_markup(GTK_LABEL(controlsLabel), "<b>Controls</b>");
}   
  gtk_widget_set_halign(controlsLabel, GTK_ALIGN_START);
  gtk_box_pack_start(GTK_BOX(sideBox), controlsLabel, FALSE, FALSE, 10);


  tetrimoneApp->controlsLabel = gtk_label_new(
    tetrimoneApp->board->retroModeActive ? 
    "Управление клавиатурой:\n"
    "• Влево/Вправо/A/D: Перемещение блока\n"
    "• Вверх/W: Поворот по часовой стрелке\n"
    "• Z: Поворот против часовой стрелки\n"
    "• Вниз/S: Мягкое падение\n"
    "• Пробел: Быстрое падение\n"
    "• P: Пауза/Продолжение игры\n"
    "• R: Перезапуск игры\n"
    "• M: Переключение музыки\n\n"
    "Поддержка контроллера доступна.\n"
    "Настройка в меню Управление." :
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
    "Configure in Controls menu.");

  tetrimoneApp->controlsHeaderLabel = controlsLabel;
  gtk_widget_set_halign(tetrimoneApp->controlsLabel, GTK_ALIGN_START);
  gtk_box_pack_start(GTK_BOX(sideBox), tetrimoneApp->controlsLabel, FALSE, FALSE, 0);

  // Set up key press events
  gtk_widget_add_events(tetrimoneApp->window, GDK_KEY_PRESS_MASK | GDK_KEY_RELEASE_MASK);
  g_signal_connect(G_OBJECT(tetrimoneApp->window), "key-press-event",
                 G_CALLBACK(onKeyPress), tetrimoneApp);
  g_signal_connect(G_OBJECT(tetrimoneApp->window), "key-release-event", 
                 G_CALLBACK(onKeyPress), tetrimoneApp);  // Connect cleanup function
    g_object_set_data_full(G_OBJECT(tetrimoneApp->window), "app-data",
                         tetrimoneApp, cleanupApp);

  // Show all widgets
  gtk_widget_show_all(tetrimoneApp->window);

  // Initialize the menu state
  gtk_widget_set_sensitive(tetrimoneApp->startMenuItem, FALSE);
  gtk_widget_set_sensitive(tetrimoneApp->pauseMenuItem, TRUE);

  if (tetrimoneApp->board->initializeAudio()) {
    // Only play music if initialization was successful
    tetrimoneApp->board->playBackgroundMusic();
    tetrimoneApp->backgroundMusicPlaying = true;
  } else {
    printf("Music failed to initialize");
    // Disable sound menu item
    gtk_check_menu_item_set_active(
        GTK_CHECK_MENU_ITEM(tetrimoneApp->soundToggleMenuItem), FALSE);
  }

  tetrimoneApp->joystick = NULL;
  tetrimoneApp->joystickEnabled = false;
  tetrimoneApp->joystickTimerId = 0;

   if (args) {
        applyCommandLineArgs(tetrimoneApp, *args);
    }


  // Try to initialize SDL
  initSDL(tetrimoneApp);

  // Start the game
  startGame(tetrimoneApp);
}

void TetrimoneBoard::setLevel(int newLevel) {
    if (newLevel >= 1) {
        level = newLevel;
    }
}

void TetrimoneBoard::setMinBlock(int size) {
    if (size >= 1 && size <=4) {
        minBlockSize=size;
    }
}


// Function to create the menu bar with a better organization
void createMenu(TetrimoneApp *app) {
  GtkWidget *menuBar = gtk_menu_bar_new();

  // *** GAME MENU ***
  GtkWidget *gameMenu = gtk_menu_new();
  GtkWidget *gameMenuItem = gtk_menu_item_new_with_label("Game");
  gtk_menu_item_set_submenu(GTK_MENU_ITEM(gameMenuItem), gameMenu);

  // Connect signals for menu activation/deactivation
  g_signal_connect(G_OBJECT(gameMenu), "show", G_CALLBACK(onMenuActivated),
                   app);
  g_signal_connect(G_OBJECT(gameMenu), "hide", G_CALLBACK(onMenuDeactivated),
                   app);

  // Game menu items
  app->startMenuItem = gtk_menu_item_new_with_label("Start");
  app->pauseMenuItem = gtk_menu_item_new_with_label("Pause");
  app->restartMenuItem = gtk_menu_item_new_with_label("Restart");
  GtkWidget *quitMenuItem = gtk_menu_item_new_with_label("Quit");

  gtk_menu_shell_append(GTK_MENU_SHELL(gameMenu), app->startMenuItem);
  gtk_menu_shell_append(GTK_MENU_SHELL(gameMenu), app->pauseMenuItem);
  gtk_menu_shell_append(GTK_MENU_SHELL(gameMenu), app->restartMenuItem);

  GtkWidget *highScoresMenuItem = gtk_menu_item_new_with_label("High Scores");
  gtk_menu_shell_append(GTK_MENU_SHELL(gameMenu), highScoresMenuItem);
  g_signal_connect(G_OBJECT(highScoresMenuItem), "activate",
                   G_CALLBACK(onViewHighScores), app);
  gtk_menu_shell_append(GTK_MENU_SHELL(gameMenu),
                        gtk_separator_menu_item_new());

  gtk_menu_shell_append(GTK_MENU_SHELL(gameMenu), quitMenuItem);

  // *** DIFFICULTY MENU ***
  GtkWidget *difficultyMenu = gtk_menu_new();
  GtkWidget *difficultyMenuItem = gtk_menu_item_new_with_label("Difficulty");
  gtk_menu_item_set_submenu(GTK_MENU_ITEM(difficultyMenuItem), difficultyMenu);

  // Connect signals for difficulty submenu
  g_signal_connect(G_OBJECT(difficultyMenu), "show",
                   G_CALLBACK(onMenuActivated), app);
  g_signal_connect(G_OBJECT(difficultyMenu), "hide",
                   G_CALLBACK(onMenuDeactivated), app);

  // Create difficulty radio menu items
  GSList *difficultyGroup = NULL;
  app->zenMenuItem = gtk_radio_menu_item_new_with_label(difficultyGroup, "Zen");
  difficultyGroup =
      gtk_radio_menu_item_get_group(GTK_RADIO_MENU_ITEM(app->zenMenuItem));

  app->easyMenuItem =
      gtk_radio_menu_item_new_with_label(difficultyGroup, "Easy");
  difficultyGroup =
      gtk_radio_menu_item_get_group(GTK_RADIO_MENU_ITEM(app->easyMenuItem));

  app->mediumMenuItem =
      gtk_radio_menu_item_new_with_label(difficultyGroup, "Medium");
  difficultyGroup =
      gtk_radio_menu_item_get_group(GTK_RADIO_MENU_ITEM(app->mediumMenuItem));

  app->hardMenuItem =
      gtk_radio_menu_item_new_with_label(difficultyGroup, "Hard");
  difficultyGroup =
      gtk_radio_menu_item_get_group(GTK_RADIO_MENU_ITEM(app->hardMenuItem));

  app->extremeMenuItem =
      gtk_radio_menu_item_new_with_label(difficultyGroup, "Extreme");
  difficultyGroup =
      gtk_radio_menu_item_get_group(GTK_RADIO_MENU_ITEM(app->extremeMenuItem));

  app->insaneMenuItem =
      gtk_radio_menu_item_new_with_label(difficultyGroup, "Insane");

  // Set medium as default
  gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(app->mediumMenuItem),
                                 TRUE);
  app->difficulty = 2; // Medium

  gtk_menu_shell_append(GTK_MENU_SHELL(difficultyMenu), app->zenMenuItem);
  gtk_menu_shell_append(GTK_MENU_SHELL(difficultyMenu), app->easyMenuItem);
  gtk_menu_shell_append(GTK_MENU_SHELL(difficultyMenu), app->mediumMenuItem);
  gtk_menu_shell_append(GTK_MENU_SHELL(difficultyMenu), app->hardMenuItem);
  gtk_menu_shell_append(GTK_MENU_SHELL(difficultyMenu), app->extremeMenuItem);
  gtk_menu_shell_append(GTK_MENU_SHELL(difficultyMenu), app->insaneMenuItem);

  // *** GRAPHICS MENU ***
  GtkWidget *graphicsMenu = gtk_menu_new();
  GtkWidget *graphicsMenuItem = gtk_menu_item_new_with_label("Graphics");
  gtk_menu_item_set_submenu(GTK_MENU_ITEM(graphicsMenuItem), graphicsMenu);

  // Connect signals for menu activation/deactivation
  g_signal_connect(G_OBJECT(graphicsMenu), "show", G_CALLBACK(onMenuActivated),
                   app);
  g_signal_connect(G_OBJECT(graphicsMenu), "hide",
                   G_CALLBACK(onMenuDeactivated), app);

  // Block size menu item
  GtkWidget *blockSizeMenuItem = gtk_menu_item_new_with_label("Block Size...");
  gtk_menu_shell_append(GTK_MENU_SHELL(graphicsMenu), blockSizeMenuItem);
  g_signal_connect(G_OBJECT(blockSizeMenuItem), "activate",
                   G_CALLBACK(onBlockSizeDialog), app);

  // Background submenu
  GtkWidget *backgroundMenu = gtk_menu_new();
  GtkWidget *backgroundMenuItem = gtk_menu_item_new_with_label("Background");
  gtk_menu_item_set_submenu(GTK_MENU_ITEM(backgroundMenuItem), backgroundMenu);
  gtk_menu_shell_append(GTK_MENU_SHELL(graphicsMenu), backgroundMenuItem);

  // Background menu items
  GtkWidget *setBackgroundMenuItem =
      gtk_menu_item_new_with_label("Set Background Image ...");
  gtk_menu_shell_append(GTK_MENU_SHELL(backgroundMenu), setBackgroundMenuItem);
  g_signal_connect(G_OBJECT(setBackgroundMenuItem), "activate",
                   G_CALLBACK(onBackgroundImageDialog), app);

  GtkWidget *backgroundZipMenuItem =
      gtk_menu_item_new_with_label("Set Background Images from ZIP...");
  gtk_menu_shell_append(GTK_MENU_SHELL(backgroundMenu), backgroundZipMenuItem);
  g_signal_connect(G_OBJECT(backgroundZipMenuItem), "activate",
                   G_CALLBACK(onBackgroundZipDialog), app);

  GtkWidget *backgroundOpacityMenuItem =
      gtk_menu_item_new_with_label("Background Opacity...");
  gtk_menu_shell_append(GTK_MENU_SHELL(backgroundMenu),
                        backgroundOpacityMenuItem);
  g_signal_connect(G_OBJECT(backgroundOpacityMenuItem), "activate",
                   G_CALLBACK(onBackgroundOpacityDialog), app);

  // Add background toggle checkbox
  app->backgroundToggleMenuItem =
      gtk_check_menu_item_new_with_label("Enable Background Image");
  gtk_check_menu_item_set_active(
      GTK_CHECK_MENU_ITEM(app->backgroundToggleMenuItem), TRUE);
  gtk_menu_shell_append(GTK_MENU_SHELL(backgroundMenu),
                        app->backgroundToggleMenuItem);
  g_signal_connect(G_OBJECT(app->backgroundToggleMenuItem), "toggled",
                   G_CALLBACK(onBackgroundToggled), app);

  GtkWidget* gridLinesMenuItem = gtk_check_menu_item_new_with_label("Show Grid Lines");
  gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(gridLinesMenuItem), 
                               app->board->isShowingGridLines());
  gtk_menu_shell_append(GTK_MENU_SHELL(graphicsMenu), gridLinesMenuItem);
  g_signal_connect(G_OBJECT(gridLinesMenuItem), "toggled",
                   G_CALLBACK(onGridLinesToggled), app);

GtkWidget* blockTrailsMenuItem = gtk_check_menu_item_new_with_label("Block Trails");
gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(blockTrailsMenuItem),
                               app->board->isTrailsEnabled());
gtk_menu_shell_append(GTK_MENU_SHELL(graphicsMenu), blockTrailsMenuItem);
g_signal_connect(G_OBJECT(blockTrailsMenuItem), "toggled",
                 G_CALLBACK(onBlockTrailsToggled), app);

GtkWidget* blockTrailsConfigMenuItem = gtk_menu_item_new_with_label("Block Trails Settings...");
gtk_menu_shell_append(GTK_MENU_SHELL(graphicsMenu), blockTrailsConfigMenuItem);
g_signal_connect(G_OBJECT(blockTrailsConfigMenuItem), "activate",
                 G_CALLBACK(onBlockTrailsConfig), app);


  // *** SOUND MENU ***
  GtkWidget *soundMenu = gtk_menu_new();
  GtkWidget *soundMenuItem = gtk_menu_item_new_with_label("Sound");
  gtk_menu_item_set_submenu(GTK_MENU_ITEM(soundMenuItem), soundMenu);

  // Connect signals for menu activation/deactivation
  g_signal_connect(G_OBJECT(soundMenu), "show", G_CALLBACK(onMenuActivated),
                   app);
  g_signal_connect(G_OBJECT(soundMenu), "hide", G_CALLBACK(onMenuDeactivated),
                   app);

  GtkWidget *themeMenu = gtk_menu_new();
  GtkWidget *themeMenuItem = gtk_menu_item_new_with_label("Color Themes");
  gtk_menu_item_set_submenu(GTK_MENU_ITEM(themeMenuItem), themeMenu);
  gtk_menu_shell_append(GTK_MENU_SHELL(graphicsMenu), themeMenuItem);

  // Connect signals for theme submenu
  g_signal_connect(G_OBJECT(themeMenu), "show", G_CALLBACK(onMenuActivated), app);
  g_signal_connect(G_OBJECT(themeMenu), "hide", G_CALLBACK(onMenuDeactivated), app);

  // Create radio group for theme selection
  GSList *themeGroup = NULL;

  // Theme names (matching the order in themes.h)
  const char* themeNames[] = {
    "Watercolor",           // 0
    "Neon",                 // 1
    "Pastel",               // 2
    "Earth Tones",          // 3
    "Monochrome Blue",      // 4
    "Monochrome Green",     // 5
    "Sunset",               // 6
    "Ocean",                // 7
    "Grayscale",            // 8
    "Candy",                // 9
    "Neon Dark",            // 10
    "Jewel Tones",          // 11
    "Retro Gaming",         // 12
    "Autumn",               // 13
    "Winter",               // 14
    "Spring",               // 15
    "Summer",               // 16
    "Monochrome Purple",    // 17
    "Desert",               // 18
    "Rainbow",              // 19
    "Art Deco",             // 20
    "Northern Lights",      // 21
    "Moroccan Tiles",       // 22
    "Bioluminescence",      // 23
    "Fossil",               // 24
    "Silk Road",            // 25
    "Digital Glitch",       // 26
    "Botanical",            // 27
    "Jazz Age",             // 28
    "Steampunk",            // 29
    "USA",                  // 30
    "Soviet Retro"          // 31 - This should be the last one
  };

  // Create radio menu items for each theme
  for (int i = 0; i < NUM_COLOR_THEMES; i++) {
    app->themeMenuItems[i] = gtk_radio_menu_item_new_with_label(themeGroup, themeNames[i]);
    themeGroup = gtk_radio_menu_item_get_group(GTK_RADIO_MENU_ITEM(app->themeMenuItems[i]));
    
    // Set the current theme as active
    if (i == currentThemeIndex) {
      gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(app->themeMenuItems[i]), TRUE);
    }
    
    gtk_menu_shell_append(GTK_MENU_SHELL(themeMenu), app->themeMenuItems[i]);
    
    // Connect signal - store theme index in widget data
    g_object_set_data(G_OBJECT(app->themeMenuItems[i]), "theme-index", GINT_TO_POINTER(i));
    g_signal_connect(G_OBJECT(app->themeMenuItems[i]), "toggled",
                     G_CALLBACK(onThemeChanged), app);
  }


  // Sound menu items
  app->soundToggleMenuItem = gtk_check_menu_item_new_with_label("Enable Sound");
  gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(app->soundToggleMenuItem),
                                 TRUE);
  gtk_menu_shell_append(GTK_MENU_SHELL(soundMenu), app->soundToggleMenuItem);
  g_signal_connect(G_OBJECT(app->soundToggleMenuItem), "toggled",
                   G_CALLBACK(onSoundToggled), app);

  GtkWidget *volumeMenuItem =
      gtk_menu_item_new_with_label("Volume Settings...");
  gtk_menu_shell_append(GTK_MENU_SHELL(soundMenu), volumeMenuItem);
  g_signal_connect(G_OBJECT(volumeMenuItem), "activate",
                   G_CALLBACK(onVolumeDialog), app);

  // Music submenu
  GtkWidget *musicMenu = gtk_menu_new();
  GtkWidget *musicMenuItem = gtk_menu_item_new_with_label("Music Tracks");
  gtk_menu_item_set_submenu(GTK_MENU_ITEM(musicMenuItem), musicMenu);
  gtk_menu_shell_append(GTK_MENU_SHELL(soundMenu), musicMenuItem);

  // Create checkbox for each track
  for (int i = 0; i < 5; i++) {
    char label[20];
    sprintf(label, "Track %d", i + 1);
    app->trackMenuItems[i] = gtk_check_menu_item_new_with_label(label);

    // Set all checked by default
    gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(app->trackMenuItems[i]),
                                   TRUE);

    // Store track index in the widget data
    g_object_set_data(G_OBJECT(app->trackMenuItems[i]), "track-index",
                      GINT_TO_POINTER(i));

    // Connect signal
    g_signal_connect(G_OBJECT(app->trackMenuItems[i]), "toggled",
                     G_CALLBACK(onTrackToggled), app);

    gtk_menu_shell_append(GTK_MENU_SHELL(musicMenu), app->trackMenuItems[i]);
  }

  GtkWidget *ghostPieceMenuItem =
      gtk_check_menu_item_new_with_label("Show Ghost Piece");
  gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(ghostPieceMenuItem),
                                 app->board->isGhostPieceEnabled());
  gtk_menu_shell_append(GTK_MENU_SHELL(graphicsMenu), ghostPieceMenuItem);
  g_signal_connect(G_OBJECT(ghostPieceMenuItem), "toggled",
                   G_CALLBACK(onGhostPieceToggled), app);

GtkWidget* simpleBlocksMenuItem = 
    gtk_check_menu_item_new_with_label("Simple Blocks (No 3D Effect)");
gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(simpleBlocksMenuItem),
                             app->board->simpleBlocksActive);
gtk_menu_shell_append(GTK_MENU_SHELL(graphicsMenu), simpleBlocksMenuItem);
g_signal_connect(G_OBJECT(simpleBlocksMenuItem), "toggled",
                 G_CALLBACK(onSimpleBlocksToggled), app);

// In the Sound menu section, after the trackMenuItems section:
GtkWidget* retroMusicMenuItem = 
    gtk_check_menu_item_new_with_label("Use Retro Music");
gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(retroMusicMenuItem),
                             app->board->retroMusicActive);
gtk_menu_shell_append(GTK_MENU_SHELL(soundMenu), retroMusicMenuItem);
g_signal_connect(G_OBJECT(retroMusicMenuItem), "toggled",
                 G_CALLBACK(onRetroMusicToggled), app);

  // *** CONTROLS MENU ***
  GtkWidget *controlsMenu = gtk_menu_new();
  GtkWidget *controlsMenuItem = gtk_menu_item_new_with_label("Controls");
  gtk_menu_item_set_submenu(GTK_MENU_ITEM(controlsMenuItem), controlsMenu);

  // Connect signals for menu activation/deactivation
  g_signal_connect(G_OBJECT(controlsMenu), "show", G_CALLBACK(onMenuActivated),
                   app);
  g_signal_connect(G_OBJECT(controlsMenu), "hide",
                   G_CALLBACK(onMenuDeactivated), app);

  // Joystick config menu item
  GtkWidget *joystickConfigMenuItem =
      gtk_menu_item_new_with_label("Configure Joystick...");
  gtk_menu_shell_append(GTK_MENU_SHELL(controlsMenu), joystickConfigMenuItem);
  g_signal_connect(G_OBJECT(joystickConfigMenuItem), "activate",
                   G_CALLBACK(onJoystickConfig), app);

  // *** RULES MENU ***
  GtkWidget *rulesMenu = gtk_menu_new();
  GtkWidget *rulesMenuItem = gtk_menu_item_new_with_label("Rules");
  gtk_menu_item_set_submenu(GTK_MENU_ITEM(rulesMenuItem), rulesMenu);

  // Connect signals for menu activation/deactivation
  g_signal_connect(G_OBJECT(rulesMenu), "show", G_CALLBACK(onMenuActivated),
                   app);
  g_signal_connect(G_OBJECT(rulesMenu), "hide", G_CALLBACK(onMenuDeactivated),
                   app);

  // Minimum Block Size Submenu
  GtkWidget *blockSizeRulesMenu = gtk_menu_new();
  GtkWidget *blockSizeRulesMenuItem =
      gtk_menu_item_new_with_label("Minimum Block Size");
  gtk_menu_item_set_submenu(GTK_MENU_ITEM(blockSizeRulesMenuItem),
                            blockSizeRulesMenu);
  gtk_menu_shell_append(GTK_MENU_SHELL(rulesMenu), blockSizeRulesMenuItem);

  // Create radio group for block size selection
  GSList *blockSizeGroup = NULL;

  // Create radio menu items for each block size
  GtkWidget *blockSize1MenuItem = gtk_radio_menu_item_new_with_label(
      blockSizeGroup, "4:  Single, Double, Triple, and Quadruple Blocks");
  blockSizeGroup =
      gtk_radio_menu_item_get_group(GTK_RADIO_MENU_ITEM(blockSize1MenuItem));

  GtkWidget *blockSize2MenuItem = gtk_radio_menu_item_new_with_label(
      blockSizeGroup, "3:  No Single Blocks");
  blockSizeGroup =
      gtk_radio_menu_item_get_group(GTK_RADIO_MENU_ITEM(blockSize2MenuItem));

  GtkWidget *blockSize3MenuItem = gtk_radio_menu_item_new_with_label(
      blockSizeGroup, "2:   No Single or Double Blocks");
  blockSizeGroup =
      gtk_radio_menu_item_get_group(GTK_RADIO_MENU_ITEM(blockSize3MenuItem));

  GtkWidget *blockSize4MenuItem = gtk_radio_menu_item_new_with_label(
      blockSizeGroup,
      "1:  No Single, Double, or Triple Blocks; only Quadruple Blocks");

  // Set the default selection based on current min block size
  switch (app->board->getMinBlockSize()) {
  case 1:
    gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(blockSize1MenuItem),
                                   TRUE);
    break;
  case 2:
    gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(blockSize2MenuItem),
                                   TRUE);
    break;
  case 3:
    gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(blockSize3MenuItem),
                                   TRUE);
    break;
  case 4:
    gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(blockSize4MenuItem),
                                   TRUE);
    break;
  }

  // Add block size menu items to submenu
  gtk_menu_shell_append(GTK_MENU_SHELL(blockSizeRulesMenu), blockSize1MenuItem);
  gtk_menu_shell_append(GTK_MENU_SHELL(blockSizeRulesMenu), blockSize2MenuItem);
  gtk_menu_shell_append(GTK_MENU_SHELL(blockSizeRulesMenu), blockSize3MenuItem);
  gtk_menu_shell_append(GTK_MENU_SHELL(blockSizeRulesMenu), blockSize4MenuItem);

  // Connect signals for block size selection
  g_signal_connect(G_OBJECT(blockSize1MenuItem), "toggled",
                   G_CALLBACK(onBlockSizeRulesChanged), app);
  g_signal_connect(G_OBJECT(blockSize2MenuItem), "toggled",
                   G_CALLBACK(onBlockSizeRulesChanged), app);
  g_signal_connect(G_OBJECT(blockSize3MenuItem), "toggled",
                   G_CALLBACK(onBlockSizeRulesChanged), app);
  g_signal_connect(G_OBJECT(blockSize4MenuItem), "toggled",
                   G_CALLBACK(onBlockSizeRulesChanged), app);

  gtk_menu_item_set_submenu(GTK_MENU_ITEM(rulesMenuItem), rulesMenu);
  gtk_menu_shell_append(GTK_MENU_SHELL(app->menuBar), rulesMenuItem);

  // Game Size option
  GtkWidget *gameSizeMenuItem = gtk_menu_item_new_with_label("Game Size");
  g_signal_connect(gameSizeMenuItem, "activate", G_CALLBACK(onGameSizeDialog),
                   app);
  gtk_menu_shell_append(GTK_MENU_SHELL(rulesMenu), gameSizeMenuItem);

  GtkWidget *gameSetupMenuItem = gtk_menu_item_new_with_label("Game Setup");
  g_signal_connect(gameSetupMenuItem, "activate", G_CALLBACK(onGameSetupDialog), app);
  gtk_menu_shell_append(GTK_MENU_SHELL(rulesMenu), gameSetupMenuItem);

  // *** HELP MENU ***
  GtkWidget *helpMenu = gtk_menu_new();
  GtkWidget *helpMenuItem = gtk_menu_item_new_with_label("Help");
  gtk_menu_item_set_submenu(GTK_MENU_ITEM(helpMenuItem), helpMenu);

  // Connect signals for menu activation/deactivation
  g_signal_connect(G_OBJECT(helpMenu), "show", G_CALLBACK(onMenuActivated),
                   app);
  g_signal_connect(G_OBJECT(helpMenu), "hide", G_CALLBACK(onMenuDeactivated),
                   app);

  // Help menu items
  GtkWidget *instructionsMenuItem =
      gtk_menu_item_new_with_label("Instructions");
  GtkWidget *aboutMenuItem = gtk_menu_item_new_with_label("About");

  gtk_menu_shell_append(GTK_MENU_SHELL(helpMenu), instructionsMenuItem);
  gtk_menu_shell_append(GTK_MENU_SHELL(helpMenu), aboutMenuItem);

  // Add menus to menu bar
  gtk_menu_shell_append(GTK_MENU_SHELL(menuBar), gameMenuItem);
  gtk_menu_shell_append(GTK_MENU_SHELL(menuBar), difficultyMenuItem);
  gtk_menu_shell_append(GTK_MENU_SHELL(menuBar), graphicsMenuItem);
  gtk_menu_shell_append(GTK_MENU_SHELL(menuBar), soundMenuItem);
  gtk_menu_shell_append(GTK_MENU_SHELL(menuBar), controlsMenuItem);
  gtk_menu_shell_append(GTK_MENU_SHELL(menuBar), rulesMenuItem);
  gtk_menu_shell_append(GTK_MENU_SHELL(menuBar), helpMenuItem);

  // Add menu signal handlers
  g_signal_connect(G_OBJECT(app->startMenuItem), "activate",
                   G_CALLBACK(onStartGame), app);
  g_signal_connect(G_OBJECT(app->pauseMenuItem), "activate",
                   G_CALLBACK(onPauseGame), app);
  g_signal_connect(G_OBJECT(app->restartMenuItem), "activate",
                   G_CALLBACK(onRestartGame), app);
  g_signal_connect(G_OBJECT(quitMenuItem), "activate", G_CALLBACK(onQuitGame),
                   app);

  g_signal_connect(G_OBJECT(app->zenMenuItem), "toggled",
                   G_CALLBACK(onDifficultyChanged), app);
  g_signal_connect(G_OBJECT(app->easyMenuItem), "toggled",
                   G_CALLBACK(onDifficultyChanged), app);
  g_signal_connect(G_OBJECT(app->mediumMenuItem), "toggled",
                   G_CALLBACK(onDifficultyChanged), app);
  g_signal_connect(G_OBJECT(app->hardMenuItem), "toggled",
                   G_CALLBACK(onDifficultyChanged), app);
  g_signal_connect(G_OBJECT(app->extremeMenuItem), "toggled",
                   G_CALLBACK(onDifficultyChanged), app);
  g_signal_connect(G_OBJECT(app->insaneMenuItem), "toggled",
                   G_CALLBACK(onDifficultyChanged), app);

  g_signal_connect(G_OBJECT(aboutMenuItem), "activate",
                   G_CALLBACK(onAboutDialog), app);
  g_signal_connect(G_OBJECT(instructionsMenuItem), "activate",
                   G_CALLBACK(onInstructionsDialog), app);

  // Store menu bar in app structure
  app->menuBar = menuBar;
}

void onGridLinesToggled(GtkCheckMenuItem* menuItem, gpointer userData) {
    TetrimoneApp* app = static_cast<TetrimoneApp*>(userData);
    bool showLines = gtk_check_menu_item_get_active(menuItem);
    
    app->board->setShowGridLines(showLines);
    
    // Redraw the game area
    gtk_widget_queue_draw(app->gameArea);
}

// Replace the current onThemeChanged function in tetrimone.cpp with this simplified version:

void onThemeChanged(GtkRadioMenuItem *menuItem, gpointer userData) {
  TetrimoneApp *app = static_cast<TetrimoneApp *>(userData);

  // Only proceed if the item is active (selected)
  if (!gtk_check_menu_item_get_active(GTK_CHECK_MENU_ITEM(menuItem))) {
    return;
  }

  // Get the theme index from the widget data
  int newThemeIndex = GPOINTER_TO_INT(g_object_get_data(G_OBJECT(menuItem), "theme-index"));
  
  // Don't change if it's the same theme
  if (newThemeIndex == currentThemeIndex) {
    return;
  }

  // Start smooth theme transition
  app->board->startThemeTransition(newThemeIndex);
  
  // Also trigger background transition if using background zip
  if (app->board->isUsingBackgroundZip() && !app->board->backgroundImages.empty()) {
    app->board->startBackgroundTransition();
  }
  
  // Redraw the game area to show the transition
  gtk_widget_queue_draw(app->gameArea);
  gtk_widget_queue_draw(app->nextPieceArea);
}

void onSimpleBlocksToggled(GtkCheckMenuItem* menuItem, gpointer userData) {
    TetrimoneApp* app = static_cast<TetrimoneApp*>(userData);
    app->board->simpleBlocksActive = gtk_check_menu_item_get_active(menuItem);
    
    // Redraw the game area to reflect the change
    gtk_widget_queue_draw(app->gameArea);
    gtk_widget_queue_draw(app->nextPieceArea);
}

void onRetroMusicToggled(GtkCheckMenuItem* menuItem, gpointer userData) {
    TetrimoneApp* app = static_cast<TetrimoneApp*>(userData);
    app->board->retroMusicActive = gtk_check_menu_item_get_active(menuItem);
    
    // If music is playing, restart it to apply the change
    if (app->backgroundMusicPlaying && app->board->sound_enabled_) {
        app->board->pauseBackgroundMusic();
        app->board->playBackgroundMusic();  // This will use retroMusicActive
    }
}


void onBlockSizeRulesChanged(GtkRadioMenuItem *menuItem, gpointer userData) {
  TetrimoneApp *app = static_cast<TetrimoneApp *>(userData);

  // Only proceed if the item is active (selected)
  if (!gtk_check_menu_item_get_active(GTK_CHECK_MENU_ITEM(menuItem))) {
    return;
  }

  int newMinBlockSize = 1;

  // Get the parent menu
  GtkWidget *parentMenu = gtk_widget_get_parent(GTK_WIDGET(menuItem));

  // Find the index of this item in the parent menu
  GList *children = gtk_container_get_children(GTK_CONTAINER(parentMenu));
  int index = g_list_index(children, menuItem);
  g_list_free(children);

  // Determine the block size based on the index
  switch (index) {
  case 3: // Last item (4 blocks)
    newMinBlockSize = 4;
    break;
  case 2: // Third item (3 blocks)
    newMinBlockSize = 3;
    break;
  case 1: // Second item (2 blocks)
    newMinBlockSize = 2;
    break;
  case 0: // First item (1 block)
  default:
    newMinBlockSize = 1;
    break;
  }

  // Retrieve the current min block size
  int currentMinBlockSize = app->board->getMinBlockSize();

  // Proceed only if block size is different
  if (newMinBlockSize != currentMinBlockSize) {
    // Create confirmation dialog
    GtkWidget *dialog = gtk_message_dialog_new(
        GTK_WINDOW(app->window), GTK_DIALOG_MODAL, GTK_MESSAGE_QUESTION,
        GTK_BUTTONS_YES_NO,
        "Changing the minimum block size will restart the game. Continue?");

    // Run dialog and get response
    gint response = gtk_dialog_run(GTK_DIALOG(dialog));
    gtk_widget_destroy(dialog);

    // If user clicked "No", revert to original radio button and return
    if (response != GTK_RESPONSE_YES) {
      // Block signals to avoid recursion
      g_signal_handlers_block_by_func(G_OBJECT(menuItem),
                                      (gpointer)onBlockSizeRulesChanged, app);

      // Reselect the previous block size menu item
      GList *children = gtk_container_get_children(GTK_CONTAINER(parentMenu));
      GtkWidget *previousItem =
          GTK_WIDGET(g_list_nth_data(children, currentMinBlockSize - 1));
      g_list_free(children);

      if (previousItem) {
        gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(previousItem), TRUE);
      }

      // Unblock signals
      g_signal_handlers_unblock_by_func(G_OBJECT(menuItem),
                                        (gpointer)onBlockSizeRulesChanged, app);

      return;
    }

    // Apply the new minimum block size
    app->board->setMinBlockSize(newMinBlockSize);

    // Restart the game with new settings
    app->board->restart();
    resetUI(app);

    // Start game with new settings
    if (app->board->isPaused()) {
      app->board->togglePause();
      gtk_menu_item_set_label(GTK_MENU_ITEM(app->pauseMenuItem), "Pause");
    }

    gtk_widget_set_sensitive(app->startMenuItem, FALSE);
    gtk_widget_set_sensitive(app->pauseMenuItem, TRUE);

    startGame(app);
    gtk_widget_queue_draw(app->gameArea);
    gtk_widget_queue_draw(app->nextPieceArea);
    updateLabels(app);
  }
}

// Menu callback functions
void onStartGame(GtkMenuItem *menuItem, gpointer userData) {
  TetrimoneApp *app = static_cast<TetrimoneApp *>(userData);
  if (app->board->isGameOver()) {
    app->board->restart();
    resetUI(app);
  }
  app->board->setPaused(false);
  startGame(app);

  gtk_widget_set_sensitive(app->startMenuItem, FALSE);
  gtk_widget_set_sensitive(app->pauseMenuItem, TRUE);

  gtk_widget_queue_draw(app->gameArea);
  gtk_widget_queue_draw(app->nextPieceArea);
  updateLabels(app);
}

void pauseGame(TetrimoneApp *app) {
  // Remove the timer
  if (app->timerId > 0) {
    g_source_remove(app->timerId);
    app->timerId = 0;
  }

  // Pause background music if enabled
  if (app->backgroundMusicPlaying && app->board->sound_enabled_) {
    app->board->pauseBackgroundMusic();
    app->backgroundMusicPlaying = false;
  }

  // Update menu state
  gtk_widget_set_sensitive(app->startMenuItem, TRUE);
  gtk_menu_item_set_label(GTK_MENU_ITEM(app->pauseMenuItem), "Resume");
}

void onRestartGame(GtkMenuItem *menuItem, gpointer userData) {
  TetrimoneApp *app = static_cast<TetrimoneApp *>(userData);
  app->board->restart();
  resetUI(app);

  if (app->board->isPaused()) {
    app->board->togglePause();
    gtk_menu_item_set_label(GTK_MENU_ITEM(app->pauseMenuItem), "Pause");
  }

  gtk_widget_set_sensitive(app->startMenuItem, FALSE);
  gtk_widget_set_sensitive(app->pauseMenuItem, TRUE);

  startGame(app);
  gtk_widget_queue_draw(app->gameArea);
  gtk_widget_queue_draw(app->nextPieceArea);
  updateLabels(app);
}

void onQuitGame(GtkMenuItem *menuItem, gpointer userData) {
  TetrimoneApp *app = static_cast<TetrimoneApp *>(userData);
  gtk_window_close(GTK_WINDOW(app->window));
}

gboolean onDeleteEvent(GtkWidget *widget, GdkEvent *event, gpointer userData) {
#ifdef DEBUG
  std::cerr << "DEBUG: onDeleteEvent called" << std::endl;
#endif
  TetrimoneApp *app = static_cast<TetrimoneApp *>(userData);
#ifdef DEBUG
  std::cerr << "DEBUG: Stopping timers" << std::endl;
#endif
  // Stop timers first to prevent any callbacks during cleanup
  if (app->timerId > 0) {
    g_source_remove(app->timerId);
    app->timerId = 0;
  }

  if (app->joystickTimerId > 0) {
    g_source_remove(app->joystickTimerId);
    app->joystickTimerId = 0;
  }

#ifdef DEBUG
  std::cerr << "DEBUG: Stopping audio" << std::endl;
#endif
  // Stop and cleanup audio
  if (app->board) {
    if (app->backgroundMusicPlaying) {
#ifdef DEBUG
      std::cerr << "DEBUG: Pausing background music" << std::endl;
#endif
      app->board->pauseBackgroundMusic();
      app->backgroundMusicPlaying = false;
    }
#ifdef DEBUG
    std::cerr << "DEBUG: Cleaning up audio" << std::endl;
#endif
    app->board->cleanupAudio();
  }
#ifdef DEBUG
  std::cerr << "DEBUG: Cleaning up joystick" << std::endl;
#endif
  // Close joystick
  if (app->joystick != NULL) {
#ifdef DEBUG
    std::cerr << "DEBUG: Closing joystick" << std::endl;
#endif
    SDL_JoystickClose(app->joystick);
    app->joystick = NULL;
  }

  // Quit SDL
  if (app->joystickEnabled) {
#ifdef DEBUG
    std::cerr << "DEBUG: Quitting SDL" << std::endl;
#endif
    SDL_Quit();
    app->joystickEnabled = false;
  }

#ifdef DEBUG
  std::cerr << "DEBUG: Returning FALSE to let GTK handle window destruction"
            << std::endl;
#endif
  // Now allow the window to close
  return FALSE; // Let GTK handle the window destruction
}

void onSoundToggled(GtkCheckMenuItem *menuItem, gpointer userData) {
  TetrimoneApp *app = static_cast<TetrimoneApp *>(userData);
  bool isSoundEnabled = gtk_check_menu_item_get_active(menuItem);

  app->board->sound_enabled_ = isSoundEnabled;

  if (isSoundEnabled) {
    // If sound is being turned on, we need to initialize the audio system
    if (!app->board->initializeAudio()) {
      // If initialization fails, update menu item to reflect actual state
      gtk_check_menu_item_set_active(menuItem, false);
      return;
    }

    if (!app->board->isPaused() && !app->board->isGameOver()) {
      app->board->resumeBackgroundMusic();
      app->backgroundMusicPlaying = true;
    }
  } else {
    // When disabling sound, pause the music and clean up audio resources
    app->board->pauseBackgroundMusic();
    app->backgroundMusicPlaying = false;
    app->board->cleanupAudio();
  }
}

void onDifficultyChanged(GtkRadioMenuItem *menuItem, gpointer userData) {
  TetrimoneApp *app = static_cast<TetrimoneApp *>(userData);

  // Only proceed if the item is active (selected)
  if (!gtk_check_menu_item_get_active(GTK_CHECK_MENU_ITEM(menuItem))) {
    return;
  }

  // Store the current difficulty level
  int previousDifficulty = app->difficulty;
  int newDifficulty = previousDifficulty; // Default to no change

  // Determine which difficulty was selected
  if (menuItem == GTK_RADIO_MENU_ITEM(app->easyMenuItem)) {
    newDifficulty = 1;
  } else if (menuItem == GTK_RADIO_MENU_ITEM(app->mediumMenuItem)) {
    newDifficulty = 2;
  } else if (menuItem == GTK_RADIO_MENU_ITEM(app->hardMenuItem)) {
    newDifficulty = 3;
  } else if (menuItem == GTK_RADIO_MENU_ITEM(app->extremeMenuItem)) {
    newDifficulty = 4;
  } else if (menuItem == GTK_RADIO_MENU_ITEM(app->insaneMenuItem)) {
    newDifficulty = 5;
  } else if (menuItem == GTK_RADIO_MENU_ITEM(app->zenMenuItem)) {
    newDifficulty = 0;
  }

  // Only prompt if the difficulty actually changed
  if (newDifficulty != previousDifficulty) {
    // Don't prompt if we're at the splash screen or game over
    if (!app->board->isSplashScreenActive() && !app->board->isGameOver()) {
      // Create confirmation dialog
      GtkWidget *dialog = gtk_message_dialog_new(
          GTK_WINDOW(app->window), GTK_DIALOG_MODAL, GTK_MESSAGE_QUESTION,
          GTK_BUTTONS_YES_NO,
          "Changing difficulty will start a new game. Continue?");

      // Run dialog and get response
      gint response = gtk_dialog_run(GTK_DIALOG(dialog));
      gtk_widget_destroy(dialog);

      // If user clicked "No", revert to original radio button and return
      if (response != GTK_RESPONSE_YES) {
        // Block signals to avoid recursion
        g_signal_handlers_block_by_func(G_OBJECT(menuItem),
                                        (gpointer)onDifficultyChanged, app);

        // Reselect the previous difficulty menu item
        switch (previousDifficulty) {
        case 0:
          gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(app->zenMenuItem),
                                         TRUE);
          break;
        case 1:
          gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(app->easyMenuItem),
                                         TRUE);
          break;
        case 2:
          gtk_check_menu_item_set_active(
              GTK_CHECK_MENU_ITEM(app->mediumMenuItem), TRUE);
          break;
        case 3:
          gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(app->hardMenuItem),
                                         TRUE);
          break;
        case 4:
          gtk_check_menu_item_set_active(
              GTK_CHECK_MENU_ITEM(app->extremeMenuItem), TRUE);
          break;
        case 5:
          gtk_check_menu_item_set_active(
              GTK_CHECK_MENU_ITEM(app->insaneMenuItem), TRUE);
          break;
        }

        // Unblock signals
        g_signal_handlers_unblock_by_func(G_OBJECT(menuItem),
                                          (gpointer)onDifficultyChanged, app);

        return;
      }
    }

    // Apply the new difficulty level
    app->difficulty = newDifficulty;

    // Update difficulty label
gtk_label_set_markup(GTK_LABEL(app->difficultyLabel),
                     app->board->getDifficultyText(app->difficulty).c_str());
    // Recalculate drop speed based on difficulty and level
    adjustDropSpeed(app);

    // Restart the game with new difficulty
    app->board->restart();
    resetUI(app);

    // Start game with new settings
    if (app->board->isPaused()) {
      app->board->togglePause();
      gtk_menu_item_set_label(GTK_MENU_ITEM(app->pauseMenuItem), "Pause");
    }

    gtk_widget_set_sensitive(app->startMenuItem, FALSE);
    gtk_widget_set_sensitive(app->pauseMenuItem, TRUE);

    startGame(app);
    gtk_widget_queue_draw(app->gameArea);
    gtk_widget_queue_draw(app->nextPieceArea);
    updateLabels(app);
  } else {
    // If difficulty didn't change, just restart timer with new speed if game is
    // running
    if (!app->board->isPaused() && !app->board->isGameOver() &&
        app->timerId > 0) {
      g_source_remove(app->timerId);
      app->timerId = g_timeout_add(app->dropSpeed, onTimerTick, app);
    }
  }
}

std::string TetrimoneBoard::getDifficultyText(int difficulty) const {
  if (retroModeActive) {
    switch (difficulty) {
    case 0:
      return "<b>Сложность:</b> Санаторий для Партийной Элиты"; // Luxury Sanatorium for Party Elite
    case 1:
      return "<b>Сложность:</b> Стахановское Движение для Начинающих"; // Stakhanovite Movement for Beginners
    case 2:
      return "<b>Сложность:</b> Стандартный Рабочий Режим"; // Standard Worker Mode
    case 3:
      return "<b>Сложность:</b> Ударный Труд"; // Shock Work
    case 4:
      return "<b>Сложность:</b> Сибирская Зима"; // Siberian Winter
    case 5:
      return "<b>Сложность:</b> ГУЛАГ"; // GULAG
    default:
      return "<b>Сложность:</b> Стандартный Рабочий Режим";
    }
  } else {
    switch (difficulty) {
    case 0:
      return "<b>Difficulty:</b> Zen";
    case 1:
      return "<b>Difficulty:</b> Easy";
    case 2:
      return "<b>Difficulty:</b> Medium";
    case 3:
      return "<b>Difficulty:</b> Hard";
    case 4:
      return "<b>Difficulty:</b> Extreme";
    case 5:
      return "<b>Difficulty:</b> Insane";
    default:
      return "<b>Difficulty:</b> Medium";
    }
  }
}

void adjustDropSpeed(TetrimoneApp *app) {
  // Base speed based on level
  int baseSpeed = INITIAL_SPEED - (app->board->getLevel() - 1) * 50;

  // Apply difficulty modifier
  switch (app->difficulty) {
  case 0: // Zen
    app->dropSpeed = 1000;
    break;
  case 1: // Easy
    app->dropSpeed = baseSpeed * 1.5;
    break;
  case 2: // Medium
    app->dropSpeed = baseSpeed;
    break;
  case 3: // Hard
    app->dropSpeed = baseSpeed * 0.7;
    break;
  case 4: // Extreme
    app->dropSpeed = baseSpeed * 0.3;
    break;
  case 5: // Insane
    app->dropSpeed = baseSpeed * 0.1;
    break;

  default:
    app->dropSpeed = baseSpeed;
  }

  // Enforce minimum speed
  if (app->dropSpeed < 10) {
    app->dropSpeed = 10;
  }
}



// Add to startGame function
void startGame(TetrimoneApp *app) {
  // Remove existing timer if any
  if (app->timerId > 0) {
    g_source_remove(app->timerId);
    app->timerId = 0;
  }

  // Resume background music if it was playing
  if (!app->backgroundMusicPlaying && app->board->sound_enabled_) {
    app->board->resumeBackgroundMusic();
    app->backgroundMusicPlaying = true;
  }

  // Calculate drop speed based on level and difficulty
  adjustDropSpeed(app);
    if (app->board->junkLinesPercentage > 0) {
       app->board->generateJunkLines(app->board->junkLinesPercentage);
    }

    if (app->board->junkLinesPerLevel > 0) {
        app->board->addJunkLinesFromBottom(app->board->junkLinesPerLevel);
    }
  // Start a new timer
  app->timerId = g_timeout_add(app->dropSpeed, onTimerTick, app);

  // Update menu items
  gtk_widget_set_sensitive(app->startMenuItem, FALSE);
  gtk_widget_set_sensitive(app->pauseMenuItem, TRUE);
  gtk_menu_item_set_label(GTK_MENU_ITEM(app->pauseMenuItem), "Pause");
}

void onPauseGame(GtkMenuItem *menuItem, gpointer userData) {
  TetrimoneApp *app = static_cast<TetrimoneApp *>(userData);
  if (!app->board->isGameOver()) {
    bool isPaused = app->board->isPaused();
    app->board->togglePause();

    if (app->board->isPaused()) {
      pauseGame(app);
      gtk_menu_item_set_label(GTK_MENU_ITEM(app->pauseMenuItem), "Resume");
      gtk_widget_set_sensitive(app->startMenuItem, TRUE);
    } else {
      startGame(app);
      gtk_menu_item_set_label(GTK_MENU_ITEM(app->pauseMenuItem), "Pause");
      gtk_widget_set_sensitive(app->startMenuItem, FALSE);
    }

    gtk_widget_queue_draw(app->gameArea);
  }
}

void onMenuActivated(GtkWidget *widget, gpointer userData) {
  TetrimoneApp *app = static_cast<TetrimoneApp *>(userData);

  // Store current menu item label
  const char *currentLabel =
      gtk_menu_item_get_label(GTK_MENU_ITEM(app->pauseMenuItem));

  // Call onPauseGame to properly pause the game if it's not already paused
  if (!app->board->isPaused() && !app->board->isGameOver()) {
    onPauseGame(GTK_MENU_ITEM(app->pauseMenuItem), app);

    // Restore the menu label so the visual state remains consistent
    gtk_menu_item_set_label(GTK_MENU_ITEM(app->pauseMenuItem), currentLabel);
  }
}

void onMenuDeactivated(GtkWidget *widget, gpointer userData) {
  TetrimoneApp *app = static_cast<TetrimoneApp *>(userData);

  // Call onPauseGame to resume if we weren't manually paused before
  if (app->board->isPaused() && !app->board->isGameOver() &&
      strcmp(gtk_menu_item_get_label(GTK_MENU_ITEM(app->pauseMenuItem)),
             "Resume") != 0) {
    onPauseGame(GTK_MENU_ITEM(app->pauseMenuItem), app);
  }
}

bool TetrimoneBoard::isGameOver() const {
  // If this is the first time checking game over status since it became true,
  // play the game over sound
  static bool soundPlayed = false;
  if (gameOver && !soundPlayed) {
    // Cast away const to allow calling non-const member function
    TetrimoneBoard *nonConstThis = const_cast<TetrimoneBoard *>(this);
    if (retroModeActive) {
        nonConstThis->playSound(GameSoundEvent::GameoverRetro);
    } else {
        nonConstThis->playSound(GameSoundEvent::Gameover);
    }    
    
    soundPlayed = true;
  }

  // If game is no longer over, reset the sound played flag
  if (!gameOver) {
    soundPlayed = false;
  }

  return gameOver;
}

void calculateBlockSize(TetrimoneApp *app) {
  // Get the screen dimensions
  GdkRectangle workarea = {0};
  GdkMonitor *monitor =
      gdk_display_get_primary_monitor(gdk_display_get_default());
  gdk_monitor_get_workarea(monitor, &workarea);

  // Calculate available height and width (accounting for menu and side panel)
  int availableHeight =
      workarea.height - 100; // Allow for window decorations and menu
  int availableWidth = workarea.width - 300; // Allow for side panel and margins

  // Calculate block size based on available space and grid dimensions
  int heightBasedSize = availableHeight / GRID_HEIGHT;
  int widthBasedSize = availableWidth / GRID_WIDTH;

  // Use the smaller of the two to ensure the game fits on screen
  BLOCK_SIZE = std::min(heightBasedSize, widthBasedSize);

  // Constrain to min/max values for usability
  BLOCK_SIZE = std::max(BLOCK_SIZE, MIN_BLOCK_SIZE);
  BLOCK_SIZE = std::min(BLOCK_SIZE, MAX_BLOCK_SIZE);
}

void onBlockSizeValueChanged(GtkRange *range, gpointer data) {
  // Extract the app pointer and label from the data
  BlockSizeCallbackData *cbData = static_cast<BlockSizeCallbackData *>(data);
  TetrimoneApp *app = cbData->app;
  GtkWidget *label = cbData->label;

  // Get the new block size from the slider
  int newBlockSize = (int)gtk_range_get_value(range);

  // Update the displayed value
  char buf[32];
  snprintf(buf, sizeof(buf), "Current size: %d", newBlockSize);
  gtk_label_set_text(GTK_LABEL(label), buf);

  // Store the current game state before rebuilding UI
  bool gameWasPaused = app->board->isPaused();
  bool gameWasOver = app->board->isGameOver();

  // Update the global block size
  BLOCK_SIZE = newBlockSize;

  // Tear down and rebuild UI components
  rebuildGameUI(app);

  // Restore game state if needed
  if (gameWasPaused && !gameWasOver) {
    app->board->setPaused(true);
  }

  // Update menu state
  if (app->board->isPaused()) {
    gtk_menu_item_set_label(GTK_MENU_ITEM(app->pauseMenuItem), "Resume");
    gtk_widget_set_sensitive(app->startMenuItem, TRUE);
  } else {
    gtk_menu_item_set_label(GTK_MENU_ITEM(app->pauseMenuItem), "Pause");
    gtk_widget_set_sensitive(app->startMenuItem, FALSE);
  }

  // Redraw everything
  gtk_widget_queue_draw(app->gameArea);
  gtk_widget_queue_draw(app->nextPieceArea);
  updateLabels(app);
}

void rebuildGameUI(TetrimoneApp *app) {
  // Remove and destroy existing game area and next piece area
  if (app->gameArea != NULL) {
    gtk_widget_destroy(app->gameArea);
  }

  if (app->nextPieceArea != NULL) {
    gtk_widget_destroy(app->nextPieceArea);
  }

  // Resize the window to match the new block size
  // Use fixed padding for consistent gray border
  gtk_window_set_default_size(GTK_WINDOW(app->window), 
                              GRID_WIDTH * BLOCK_SIZE + 220,
                              GRID_HEIGHT * BLOCK_SIZE + 60);

  // Create new game area with exact board size
  app->gameArea = gtk_drawing_area_new();
  // Set EXACT size - the board is GRID_WIDTH * BLOCK_SIZE, nothing more
  int boardWidth = GRID_WIDTH * BLOCK_SIZE;
  int boardHeight = GRID_HEIGHT * BLOCK_SIZE;
  gtk_widget_set_size_request(app->gameArea, boardWidth, boardHeight);
  printf("1 Game area is %i %i\n", boardWidth, boardHeight);
  // EXPAND on resize - allows onScreenSizeChanged to recalculate block size
  gtk_widget_set_hexpand(app->gameArea, TRUE);
  gtk_widget_set_vexpand(app->gameArea, TRUE);
  g_signal_connect(G_OBJECT(app->gameArea), "draw", G_CALLBACK(onDrawGameArea),
                   app);

  // Add game area back to its container
  // First, find and empty the mainBox (keep the side panel)
  GList *children = gtk_container_get_children(GTK_CONTAINER(app->mainBox));
  for (GList *child = children; child != NULL; child = child->next) {
    gtk_container_remove(GTK_CONTAINER(app->mainBox), GTK_WIDGET(child->data));
  }
  g_list_free(children);

  // Recreate the main box contents with expansion so resize events fire
  gtk_box_pack_start(GTK_BOX(app->mainBox), app->gameArea, TRUE, TRUE, 0);

  // Create the side panel (vertical box)
  GtkWidget *sideBox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);  // NO spacing
  gtk_container_set_border_width(GTK_CONTAINER(sideBox), 0);  // NO border
  // Side panel stays fixed size
  gtk_widget_set_hexpand(sideBox, FALSE);
  gtk_widget_set_vexpand(sideBox, FALSE);
  gtk_box_pack_start(GTK_BOX(app->mainBox), sideBox, FALSE, FALSE, 0);

  // Create the next piece preview frame
  GtkWidget *nextPieceFrame = gtk_frame_new("Next Pieces");
  gtk_container_set_border_width(GTK_CONTAINER(nextPieceFrame), 0);  // NO border
  gtk_box_pack_start(GTK_BOX(sideBox), nextPieceFrame, FALSE, FALSE, 0);

  // Create the next piece drawing area - MUCH SMALLER
  app->nextPieceArea = gtk_drawing_area_new();
  gtk_widget_set_size_request(app->nextPieceArea, BLOCK_SIZE * 5, BLOCK_SIZE * 4);
  printf("2 Game area is %i %i\n", BLOCK_SIZE*5, BLOCK_SIZE*4);

  g_signal_connect(G_OBJECT(app->nextPieceArea), "draw",
                   G_CALLBACK(onDrawNextPiece), app);
  gtk_container_add(GTK_CONTAINER(nextPieceFrame), app->nextPieceArea);

  // Create score, level, and lines labels
  app->scoreLabel = gtk_label_new(NULL);
  std::string score_text;
  if (app->board->retroModeActive) {
    score_text = "<b>Партийная Лояльность:</b> " + std::to_string(app->board->getScore()) + "%";
  } else {
    score_text = "<b>Score:</b> " + std::to_string(app->board->getScore());
  }
  gtk_label_set_markup(GTK_LABEL(app->scoreLabel), score_text.c_str());
  gtk_widget_set_halign(app->scoreLabel, GTK_ALIGN_START);
  gtk_box_pack_start(GTK_BOX(sideBox), app->scoreLabel, FALSE, FALSE, 0);

  app->levelLabel = gtk_label_new(NULL);
  std::string level_text;
  if (app->board->retroModeActive) {
    level_text = "<b>Пятилетка:</b> " + std::to_string(app->board->getLevel());
  } else {
    level_text = "<b>Level:</b> " + std::to_string(app->board->getLevel());
  }
  gtk_label_set_markup(GTK_LABEL(app->levelLabel), level_text.c_str());
  gtk_widget_set_halign(app->levelLabel, GTK_ALIGN_START);
  gtk_box_pack_start(GTK_BOX(sideBox), app->levelLabel, FALSE, FALSE, 0);

  app->linesLabel = gtk_label_new(NULL);
  std::string lines_text;
  if (app->board->retroModeActive) {
    lines_text = "<b>Уничтожено врагов народа:</b> " + std::to_string(app->board->getLinesCleared());
  } else {
    lines_text = "<b>Lines:</b> " + std::to_string(app->board->getLinesCleared());
  }
  gtk_label_set_markup(GTK_LABEL(app->linesLabel), lines_text.c_str());
  gtk_widget_set_halign(app->linesLabel, GTK_ALIGN_START);
  gtk_box_pack_start(GTK_BOX(sideBox), app->linesLabel, FALSE, FALSE, 0);

  app->sequenceLabel = gtk_label_new(NULL);
  std::string sequence_text;
  if (app->board->isSequenceActive() && app->board->getConsecutiveClears() > 1) {
    if (app->board->retroModeActive) {
      sequence_text = "<b>Коллективная эффективность:</b> " + 
        std::to_string(app->board->getConsecutiveClears()) + " (Рекорд: " + 
        std::to_string(app->board->getMaxConsecutiveClears()) + ")";
    } else {
      sequence_text = "<b>Sequence:</b> " + std::to_string(app->board->getConsecutiveClears()) +
        " (Max: " + std::to_string(app->board->getMaxConsecutiveClears()) + ")";
    }
    // Make it stand out when active
    sequence_text = "<span foreground='#00AA00'>" + sequence_text + "</span>";
  } else {
    if (app->board->retroModeActive) {
      sequence_text = "<b>Коллективная эффективность:</b> " + 
        std::to_string(app->board->getConsecutiveClears()) + " (Рекорд: " + 
        std::to_string(app->board->getMaxConsecutiveClears()) + ")";
    } else {
      sequence_text = "<b>Sequence:</b> " + std::to_string(app->board->getConsecutiveClears()) +
        " (Max: " + std::to_string(app->board->getMaxConsecutiveClears()) + ")";
    }
  }
  gtk_label_set_markup(GTK_LABEL(app->sequenceLabel), sequence_text.c_str());
  gtk_widget_set_halign(app->sequenceLabel, GTK_ALIGN_START);
  gtk_box_pack_start(GTK_BOX(sideBox), app->sequenceLabel, FALSE, FALSE, 0);

  // Recreate difficulty label
  app->difficultyLabel = gtk_label_new(NULL);
  gtk_label_set_markup(GTK_LABEL(app->difficultyLabel),
                     app->board->getDifficultyText(app->difficulty).c_str());
  gtk_widget_set_halign(app->difficultyLabel, GTK_ALIGN_START);
  gtk_box_pack_start(GTK_BOX(sideBox), app->difficultyLabel, FALSE, FALSE, 0);

  // Add controls info
  app->controlsHeaderLabel = gtk_label_new(NULL);
  if (app->board->retroModeActive) {
    gtk_label_set_markup(GTK_LABEL(app->controlsHeaderLabel), "<b>ПАРТИЙНЫЕ ДИРЕКТИВЫ</b>");
  } else {
    gtk_label_set_markup(GTK_LABEL(app->controlsHeaderLabel), "<b>Controls</b>");
  }
  gtk_widget_set_halign(app->controlsHeaderLabel, GTK_ALIGN_START);
  gtk_box_pack_start(GTK_BOX(sideBox), app->controlsHeaderLabel, FALSE, FALSE, 10);

  app->controlsLabel = gtk_label_new(
    app->board->retroModeActive ? 
    "Управление клавиатурой:\n"
    "• Влево/Вправо/A/D: Перемещение блока\n"
    "• Вверх/W: Поворот по часовой стрелке\n"
    "• Z: Поворот против часовой стрелки\n"
    "• Вниз/S: Мягкое падение\n"
    "• Пробел: Быстрое падение\n"
    "• P: Пауза/Продолжение игры\n"
    "• R: Перезапуск игры\n"
    "• M: Переключение музыки\n\n"
    "Поддержка контроллера доступна.\n"
    "Настройка в меню Управление." :
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
    "Configure in Controls menu.");
  gtk_widget_set_halign(app->controlsLabel, GTK_ALIGN_START);
  gtk_box_pack_start(GTK_BOX(sideBox), app->controlsLabel, FALSE, FALSE, 0);

  // Show all the new widgets
  gtk_widget_show_all(app->mainBox);
}

void onResizeWindowButtonClicked(GtkWidget *button, gpointer data) {
  TetrimoneApp *app = static_cast<TetrimoneApp *>(data);

  // Rebuild UI with current block size
  rebuildGameUI(app);
}

void updateSizeValueLabel(GtkRange *range, gpointer data) {
  // Extract the label widget from the data
  GtkWidget *label = static_cast<GtkWidget *>(data);

  // Get the new block size from the slider
  int newSize = (int)gtk_range_get_value(range);

  // Update the label text
  char buf[32];
  snprintf(buf, sizeof(buf), "Current size: %d", newSize);
  gtk_label_set_text(GTK_LABEL(label), buf);
}

void onTrackToggled(GtkCheckMenuItem *menuItem, gpointer userData) {
  TetrimoneApp *app = static_cast<TetrimoneApp *>(userData);

  // Get the track index
  int trackIndex =
      GPOINTER_TO_INT(g_object_get_data(G_OBJECT(menuItem), "track-index"));

  // Update the enabled state in the board
  app->board->enabledTracks[trackIndex] =
      gtk_check_menu_item_get_active(menuItem);

  // Make sure at least one track is enabled
  bool anyEnabled = false;
  for (int i = 0; i < 5; i++) {
    if (app->board->enabledTracks[i]) {
      anyEnabled = true;
      break;
    }
  }

  // If no tracks are enabled, re-enable this one
  if (!anyEnabled) {
    app->board->enabledTracks[trackIndex] = true;
    gtk_check_menu_item_set_active(menuItem, TRUE);
  }
}

void updateWidthValueLabel(GtkAdjustment *adj, gpointer data) {
  GtkWidget *label = GTK_WIDGET(data);
  int value = (int)gtk_adjustment_get_value(adj);
  char text[50];
  sprintf(text, "Width: %d", value);
  gtk_label_set_text(GTK_LABEL(label), text);
}

void updateHeightValueLabel(GtkAdjustment *adj, gpointer data) {
  GtkWidget *label = GTK_WIDGET(data);
  int value = (int)gtk_adjustment_get_value(adj);
  char text[50];
  sprintf(text, "Height: %d", value);
  gtk_label_set_text(GTK_LABEL(label), text);
}

void onBlockSizeDialog(GtkMenuItem *menuItem, gpointer userData) {
  TetrimoneApp *app = static_cast<TetrimoneApp *>(userData);

  // Pause the game if it's running
  bool wasPaused = app->board->isPaused();
  if (!wasPaused && !app->board->isGameOver() &&
      !app->board->isSplashScreenActive()) {
    onPauseGame(GTK_MENU_ITEM(app->pauseMenuItem), app);
  }

  // Store original block size in case user cancels
  int originalBlockSize = BLOCK_SIZE;
  int newBlockSize = BLOCK_SIZE; // Working value

  // Create dialog with Apply and Cancel buttons
  GtkWidget *dialog = gtk_dialog_new_with_buttons(
      "Block Size", GTK_WINDOW(app->window), GTK_DIALOG_MODAL, "_Cancel",
      GTK_RESPONSE_CANCEL, "_Apply", GTK_RESPONSE_APPLY, NULL);

  // Make it a reasonable size
  gtk_window_set_default_size(GTK_WINDOW(dialog), 300, 150);

  // Create content area
  GtkWidget *contentArea = gtk_dialog_get_content_area(GTK_DIALOG(dialog));
  gtk_container_set_border_width(GTK_CONTAINER(contentArea), 10);

  // Create a vertical box for content
  GtkWidget *vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 6);
  gtk_container_add(GTK_CONTAINER(contentArea), vbox);

  // Add a label
  GtkWidget *label = gtk_label_new("Adjust block size:");
  gtk_widget_set_halign(label, GTK_ALIGN_START);
  gtk_box_pack_start(GTK_BOX(vbox), label, FALSE, FALSE, 0);

  // Create a horizontal scale (slider)
  GtkWidget *scale = gtk_scale_new_with_range(
      GTK_ORIENTATION_HORIZONTAL, MIN_BLOCK_SIZE, MAX_BLOCK_SIZE, 1);
  gtk_range_set_value(GTK_RANGE(scale), BLOCK_SIZE);
  gtk_scale_set_digits(GTK_SCALE(scale), 0); // No decimal places
  gtk_scale_set_value_pos(GTK_SCALE(scale), GTK_POS_RIGHT);
  gtk_box_pack_start(GTK_BOX(vbox), scale, FALSE, FALSE, 0);

  // Add min/max labels
  GtkWidget *rangeBox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
  gtk_box_pack_start(GTK_BOX(vbox), rangeBox, FALSE, FALSE, 0);

  GtkWidget *minLabel = gtk_label_new("Small");
  gtk_widget_set_halign(minLabel, GTK_ALIGN_START);
  gtk_box_pack_start(GTK_BOX(rangeBox), minLabel, TRUE, TRUE, 0);

  GtkWidget *maxLabel = gtk_label_new("Large");
  gtk_widget_set_halign(maxLabel, GTK_ALIGN_END);
  gtk_box_pack_end(GTK_BOX(rangeBox), maxLabel, TRUE, TRUE, 0);

  // Current value label that updates as the slider moves
  GtkWidget *currentValueLabel = gtk_label_new(NULL);
  char valueBuf[32];
  snprintf(valueBuf, sizeof(valueBuf), "Current size: %d", BLOCK_SIZE);
  gtk_label_set_text(GTK_LABEL(currentValueLabel), valueBuf);
  gtk_widget_set_halign(currentValueLabel, GTK_ALIGN_START);
  gtk_box_pack_start(GTK_BOX(vbox), currentValueLabel, FALSE, FALSE, 0);

  // Note about application
  GtkWidget *noteLabel = gtk_label_new(
      "Click Apply to set the new block size.\nThis will reset the game UI.");
  gtk_widget_set_halign(noteLabel, GTK_ALIGN_START);
  gtk_box_pack_start(GTK_BOX(vbox), noteLabel, FALSE, FALSE, 10);

  // Connect value-changed signal to update the value label
  g_signal_connect(G_OBJECT(scale), "value-changed",
                   G_CALLBACK(updateSizeValueLabel), currentValueLabel);

  // Show all dialog widgets
  gtk_widget_show_all(dialog);

  // Run the dialog
  int response = gtk_dialog_run(GTK_DIALOG(dialog));

  if (response == GTK_RESPONSE_APPLY) {
    // Get the final block size value from the slider
    newBlockSize = (int)gtk_range_get_value(GTK_RANGE(scale));

    // Apply the new block size
    BLOCK_SIZE = newBlockSize;

    // Rebuild the UI with the new block size
    rebuildGameUI(app);
  } else {
    // Reset to original if canceled
    BLOCK_SIZE = originalBlockSize;
  }

  // Destroy dialog
  gtk_widget_destroy(dialog);

  // Resume the game if it wasn't paused before
  if (!wasPaused && !app->board->isGameOver() &&
      !app->board->isSplashScreenActive()) {
    onPauseGame(GTK_MENU_ITEM(app->pauseMenuItem), app);
  }
}

void onGameSizeDialog(GtkMenuItem *menuItem, gpointer userData) {
  TetrimoneApp *app = static_cast<TetrimoneApp *>(userData);

  // Create dialog
  GtkWidget *dialog = gtk_dialog_new_with_buttons(
      "Game Size Settings", GTK_WINDOW(app->window), GTK_DIALOG_MODAL, "Apply",
      GTK_RESPONSE_APPLY, "Cancel", GTK_RESPONSE_CANCEL, NULL);

  // Get content area
  GtkWidget *contentArea = gtk_dialog_get_content_area(GTK_DIALOG(dialog));
  gtk_container_set_border_width(GTK_CONTAINER(contentArea), 10);

  // Create main VBox
  GtkWidget *vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 8);
  gtk_container_add(GTK_CONTAINER(contentArea), vbox);

  // Width settings
  GtkWidget *widthFrame = gtk_frame_new("Width");
  gtk_box_pack_start(GTK_BOX(vbox), widthFrame, TRUE, TRUE, 0);

  GtkWidget *widthBox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 4);
  gtk_container_add(GTK_CONTAINER(widthFrame), widthBox);

  // Width adjustment
  GtkAdjustment *widthAdj = gtk_adjustment_new(GRID_WIDTH,     // Initial value
                                               MIN_GRID_WIDTH, // Minimum value
                                               MAX_GRID_WIDTH, // Maximum value
                                               1,              // Step increment
                                               5,              // Page increment
                                               0 // Page size (not used)
  );

  GtkWidget *widthScale = gtk_scale_new(GTK_ORIENTATION_HORIZONTAL, widthAdj);
  gtk_scale_set_digits(GTK_SCALE(widthScale), 0); // No decimal places
  gtk_box_pack_start(GTK_BOX(widthBox), widthScale, TRUE, TRUE, 0);

  GtkWidget *widthLabel = gtk_label_new("");
  gtk_box_pack_start(GTK_BOX(widthBox), widthLabel, FALSE, FALSE, 0);

  // Height settings
  GtkWidget *heightFrame = gtk_frame_new("Height");
  gtk_box_pack_start(GTK_BOX(vbox), heightFrame, TRUE, TRUE, 0);

  GtkWidget *heightBox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 4);
  gtk_container_add(GTK_CONTAINER(heightFrame), heightBox);

  // Height adjustment
  GtkAdjustment *heightAdj =
      gtk_adjustment_new(GRID_HEIGHT,     // Initial value
                         MIN_GRID_HEIGHT, // Minimum value
                         MAX_GRID_HEIGHT, // Maximum value
                         1,               // Step increment
                         5,               // Page increment
                         0                // Page size (not used)
      );

  GtkWidget *heightScale = gtk_scale_new(GTK_ORIENTATION_HORIZONTAL, heightAdj);
  gtk_scale_set_digits(GTK_SCALE(heightScale), 0); // No decimal places
  gtk_box_pack_start(GTK_BOX(heightBox), heightScale, TRUE, TRUE, 0);

  GtkWidget *heightLabel = gtk_label_new("");
  gtk_box_pack_start(GTK_BOX(heightBox), heightLabel, FALSE, FALSE, 0);

  // Update labels initially
  char widthText[50];
  sprintf(widthText, "Width: %d", GRID_WIDTH);
  gtk_label_set_text(GTK_LABEL(widthLabel), widthText);

  char heightText[50];
  sprintf(heightText, "Height: %d", GRID_HEIGHT);
  gtk_label_set_text(GTK_LABEL(heightLabel), heightText);

  // Connect value changed signals to update labels - using regular functions
  // instead of lambdas
  g_signal_connect(widthAdj, "value-changed", G_CALLBACK(updateWidthValueLabel),
                   widthLabel);
  g_signal_connect(heightAdj, "value-changed",
                   G_CALLBACK(updateHeightValueLabel), heightLabel);

  // Warning message about restarting game
  GtkWidget *warningLabel =
      gtk_label_new("Note: Changing game size will restart the current game.");
  gtk_box_pack_start(GTK_BOX(vbox), warningLabel, FALSE, FALSE, 10);

  // Show all widgets
  gtk_widget_show_all(dialog);

  // Run dialog
  gint response = gtk_dialog_run(GTK_DIALOG(dialog));

  if (response == GTK_RESPONSE_APPLY) {
    // Get the new grid dimensions
    int newWidth = (int)gtk_adjustment_get_value(widthAdj);
    int newHeight = (int)gtk_adjustment_get_value(heightAdj);

    // Only proceed if dimensions have changed
    if (newWidth != GRID_WIDTH || newHeight != GRID_HEIGHT) {
      // Create confirmation dialog
      GtkWidget *confirmDialog = gtk_message_dialog_new(
          GTK_WINDOW(app->window), GTK_DIALOG_MODAL, GTK_MESSAGE_QUESTION,
          GTK_BUTTONS_YES_NO,
          "Changing the game size will restart the current game. Continue?");

      // Run confirmation dialog and get response
      gint confirmResponse = gtk_dialog_run(GTK_DIALOG(confirmDialog));
      gtk_widget_destroy(confirmDialog);

      // If user confirmed, apply changes and restart the game
      if (confirmResponse == GTK_RESPONSE_YES) {
        // Update the global grid dimensions
        GRID_WIDTH = newWidth;
        GRID_HEIGHT = newHeight;

        // First recalculate block size based on the new grid dimensions
        calculateBlockSize(app);

        // Rebuild UI to match new dimensions
        rebuildGameUI(app);

        // Restart the game with the new dimensions
        app->board->restart();
        resetUI(app);

        // Start game with new settings
        if (app->board->isPaused()) {
          app->board->togglePause();
          gtk_menu_item_set_label(GTK_MENU_ITEM(app->pauseMenuItem), "Pause");
        }

        gtk_widget_set_sensitive(app->startMenuItem, FALSE);
        gtk_widget_set_sensitive(app->pauseMenuItem, TRUE);

        startGame(app);
      }
    }
  }

  // Destroy dialog
  gtk_widget_destroy(dialog);
}

gboolean onWindowFocusChanged(GtkWidget *widget, GdkEventFocus *event, gpointer userData) {
  TetrimoneApp *app = static_cast<TetrimoneApp *>(userData);
  
  // Only process if the game is currently active (not already paused or game over)
  if (!app->board->isPaused() && !app->board->isGameOver() && 
      !app->board->isSplashScreenActive()) {
    
    // If focus is lost (in_event is FALSE), pause the game
    if (!event->in) {
      // Store the pause state before pausing
      app->pausedByFocusLoss = true;
      
      // Call the existing pause function to ensure proper behavior
      onPauseGame(GTK_MENU_ITEM(app->pauseMenuItem), app);
      
      // Don't update the menu item text to show this is a special pause
      gtk_menu_item_set_label(GTK_MENU_ITEM(app->pauseMenuItem), "Resume");
    }
    // If focus returns (in_event is TRUE) and pause was caused by focus loss, resume the game
    else if (event->in && app->pausedByFocusLoss) {
      app->pausedByFocusLoss = false;
      
      // Only resume if still paused (user might have manually interacted with pause)
      if (app->board->isPaused()) {
        onPauseGame(GTK_MENU_ITEM(app->pauseMenuItem), app);
      }
    }
  }
  
  return FALSE; // Continue event propagation
}

void onBlockTrailsToggled(GtkCheckMenuItem* menuItem, gpointer userData) {
    TetrimoneApp* app = static_cast<TetrimoneApp*>(userData);
    app->board->setTrailsEnabled(gtk_check_menu_item_get_active(menuItem));
    
    // Redraw to show/hide trails
    gtk_widget_queue_draw(app->gameArea);
}

// Helper callback functions for the sliders
void onTrailOpacityChanged(GtkAdjustment* adj, gpointer data) {
    GtkWidget* label = GTK_WIDGET(data);
    double value = gtk_adjustment_get_value(adj);
    char text[50];
    sprintf(text, "Opacity: %.2f", value);
    gtk_label_set_text(GTK_LABEL(label), text);
}

void onTrailDurationChanged(GtkAdjustment* adj, gpointer data) {
    GtkWidget* label = GTK_WIDGET(data);
    double value = gtk_adjustment_get_value(adj);
    char text[50];
    sprintf(text, "Duration: %.2f seconds", value);
    gtk_label_set_text(GTK_LABEL(label), text);
}

void onBlockTrailsConfig(GtkMenuItem* menuItem, gpointer userData) {
    TetrimoneApp* app = static_cast<TetrimoneApp*>(userData);
    
    // Create dialog
    GtkWidget* dialog = gtk_dialog_new_with_buttons(
        "Block Trails Settings", GTK_WINDOW(app->window), GTK_DIALOG_MODAL,
        "_Apply", GTK_RESPONSE_APPLY,
        "_Cancel", GTK_RESPONSE_CANCEL, NULL);
    
    // Get content area
    GtkWidget* contentArea = gtk_dialog_get_content_area(GTK_DIALOG(dialog));
    gtk_container_set_border_width(GTK_CONTAINER(contentArea), 10);
    
    // Create main VBox
    GtkWidget* vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 8);
    gtk_container_add(GTK_CONTAINER(contentArea), vbox);
    
    // Opacity settings
    GtkWidget* opacityFrame = gtk_frame_new("Trail Opacity");
    gtk_box_pack_start(GTK_BOX(vbox), opacityFrame, TRUE, TRUE, 0);
    
    GtkWidget* opacityBox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 4);
    gtk_container_add(GTK_CONTAINER(opacityFrame), opacityBox);
    
    // Opacity slider
    GtkAdjustment* opacityAdj = gtk_adjustment_new(
        app->board->getTrailOpacity(), // Current value
        0.1,  // Minimum
        1.0,  // Maximum
        0.05, // Step
        0.1,  // Page increment
        0     // Page size
    );
    
    GtkWidget* opacityScale = gtk_scale_new(GTK_ORIENTATION_HORIZONTAL, opacityAdj);
    gtk_scale_set_digits(GTK_SCALE(opacityScale), 2);
    gtk_box_pack_start(GTK_BOX(opacityBox), opacityScale, TRUE, TRUE, 0);
    
    GtkWidget* opacityLabel = gtk_label_new("");
    gtk_box_pack_start(GTK_BOX(opacityBox), opacityLabel, FALSE, FALSE, 0);
    
    // Duration settings
    GtkWidget* durationFrame = gtk_frame_new("Trail Duration");
    gtk_box_pack_start(GTK_BOX(vbox), durationFrame, TRUE, TRUE, 0);
    
    GtkWidget* durationBox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 4);
    gtk_container_add(GTK_CONTAINER(durationFrame), durationBox);
    
    // Duration slider
    GtkAdjustment* durationAdj = gtk_adjustment_new(
        app->board->getTrailDuration(), // Current value
        0.05, // Minimum (50ms)
        1.0,  // Maximum (1 second)
        0.05, // Step
        0.1,  // Page increment
        0     // Page size
    );
    
    GtkWidget* durationScale = gtk_scale_new(GTK_ORIENTATION_HORIZONTAL, durationAdj);
    gtk_scale_set_digits(GTK_SCALE(durationScale), 2);
    gtk_box_pack_start(GTK_BOX(durationBox), durationScale, TRUE, TRUE, 0);
    
    GtkWidget* durationLabel = gtk_label_new("");
    gtk_box_pack_start(GTK_BOX(durationBox), durationLabel, FALSE, FALSE, 0);
    
    // Update labels initially
    char opacityText[50];
    sprintf(opacityText, "Opacity: %.2f", app->board->getTrailOpacity());
    gtk_label_set_text(GTK_LABEL(opacityLabel), opacityText);
    
    char durationText[50];
    sprintf(durationText, "Duration: %.2f seconds", app->board->getTrailDuration());
    gtk_label_set_text(GTK_LABEL(durationLabel), durationText);
    
    // Connect value changed signals to update labels
    g_signal_connect(opacityAdj, "value-changed", G_CALLBACK(onTrailOpacityChanged), opacityLabel);
    g_signal_connect(durationAdj, "value-changed", G_CALLBACK(onTrailDurationChanged), durationLabel);
    
    // Show all widgets
    gtk_widget_show_all(dialog);
    
    // Run dialog
    gint response = gtk_dialog_run(GTK_DIALOG(dialog));
    
    if (response == GTK_RESPONSE_APPLY) {
        // Apply the new settings
        double newOpacity = gtk_adjustment_get_value(opacityAdj);
        double newDuration = gtk_adjustment_get_value(durationAdj);
        
        app->board->setTrailOpacity(newOpacity);
        app->board->setTrailDuration(newDuration);
        
        // Redraw to show changes
        gtk_widget_queue_draw(app->gameArea);
    }
    
    // Destroy dialog
    gtk_widget_destroy(dialog);
}
