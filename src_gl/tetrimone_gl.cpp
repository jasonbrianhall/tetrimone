#include <SDL2/SDL.h>
#include <GL/glew.h>
#include <GL/gl.h>
#include <iostream>
#include <memory>
#include <chrono>

#include "tetrimone.h"
#include "audiomanager.h"
#include "highscores.h"
#include "propaganda_messages.h"
#include "freedom_messages.h"
#include "commandline.h"



// Global variables
bool g_quit = false;
bool g_paused = false;
SDL_Window* g_window = nullptr;
SDL_GLContext g_gl_context = nullptr;
std::unique_ptr<TetrimoneBoard> g_board = nullptr;
SDL_Joystick* g_joystick = nullptr;
Uint32 g_last_drop_time = 0;
Uint32 g_last_render_time = 0;
int g_drop_speed = 500; // milliseconds

// Window dimensions
const int WINDOW_WIDTH = 1200;
const int WINDOW_HEIGHT = 800;

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
  /*if (loadBackgroundImagesFromZip("background.zip")) {
    std::cout << "Successfully loaded background images from background.zip"
              << std::endl;
    // Background should be enabled by default if successfully loaded
    useBackgroundImage = true;
    useBackgroundZip = true;
  } else {
    std::cout << "Could not load background.zip, backgrounds will need to be "
                 "loaded manually"
              << std::endl;
  }*/
  for (int i = 0; i < 5; i++) {
    enabledTracks[i] = true;
  }
}

TetrimoneBoard::~TetrimoneBoard() {
    // Cancel any ongoing transition and clean up resources
    //cancelBackgroundTransition();

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
    //cleanupBackgroundImages();

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
        //addJunkLinesFromBottom(junkLinesPerLevel);
    }
    
    // Only change theme if retro mode is not enabled
    if (!retroModeActive  && !patrioticModeActive) {
        // Calculate next theme with wrap-around
        int nextTheme = (currentThemeIndex + 1) % NUM_COLOR_THEMES;
        
        // Start smooth theme transition instead of immediate change
        startThemeTransition(nextTheme);
        
        // Add background transition if using background zip
        /*if (useBackgroundZip && !backgroundImages.empty()) {
            startBackgroundTransition();
        }*/
    }
    else if (patrioticModeActive) {
        /*if (useBackgroundZip && !backgroundImages.empty()) {
            startBackgroundTransition();
        }*/
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
    //generateJunkLines(junkLinesPercentage);
  }

  // Generate new pieces
  generateNewPiece();

  // Select a random background if using background images from ZIP
  /*if (useBackgroundZip && !backgroundImages.empty()) {
    // Start a smooth background transition
    startBackgroundTransition();
  }*/

  consecutiveClears = 0;
  maxConsecutiveClears = 0;
  lastClearCount = 0;
  sequenceActive = false;
  highScoreAlreadyProcessed = false;
}

// TetrimoneBlock class implementation
TetrimoneBlock::TetrimoneBlock(int type) : type(type), rotation(0) {
  // Start pieces centered at top
  x = GRID_WIDTH / 2 - 2;
  y = 0;
}

// ============================================================================
// INITIALIZATION
// ============================================================================

bool initSDLGL() {
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_JOYSTICK) < 0) {
        std::cerr << "SDL initialization failed: " << SDL_GetError() << std::endl;
        return false;
    }

    // Set up OpenGL attributes
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
    SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);
    SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 8);

    // Create window with OpenGL support
    g_window = SDL_CreateWindow(
        "Tetrimone",
        SDL_WINDOWPOS_CENTERED,
        SDL_WINDOWPOS_CENTERED,
        WINDOW_WIDTH,
        WINDOW_HEIGHT,
        SDL_WINDOW_OPENGL | SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE
    );

    if (!g_window) {
        std::cerr << "Window creation failed: " << SDL_GetError() << std::endl;
        SDL_Quit();
        return false;
    }

    // Create OpenGL context
    g_gl_context = SDL_GL_CreateContext(g_window);
    if (!g_gl_context) {
        std::cerr << "OpenGL context creation failed: " << SDL_GetError() << std::endl;
        SDL_DestroyWindow(g_window);
        SDL_Quit();
        return false;
    }

    // Enable VSync
    SDL_GL_SetSwapInterval(1);

    // Initialize GLEW
    glewExperimental = GL_TRUE;
    GLenum glewStatus = glewInit();
    if (glewStatus != GLEW_OK) {
        std::cerr << "GLEW initialization failed: " << glewGetErrorString(glewStatus) << std::endl;
        SDL_GL_DeleteContext(g_gl_context);
        SDL_DestroyWindow(g_window);
        SDL_Quit();
        return false;
    }

    // Setup OpenGL state
    glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glEnable(GL_LINE_SMOOTH);
    glHint(GL_LINE_SMOOTH_HINT, GL_NICEST);
    glEnable(GL_MULTISAMPLE);

    std::cout << "OpenGL " << glGetString(GL_VERSION) << std::endl;

    return true;
}

bool initAudio() {
    // AudioManager uses singleton pattern - get the instance
    // NOTE: Adjust this based on your actual AudioManager implementation
    try {
        // If AudioManager has a getInstance() method:
        // AudioManager& audio = AudioManager::getInstance();
        
        // If it initializes on first use, no special initialization needed
        std::cout << "Audio manager initialized" << std::endl;
        return true;
    } catch (const std::exception& e) {
        std::cerr << "Audio initialization error: " << e.what() << std::endl;
        return false;
    }
}

bool initJoystick() {
    int joystick_count = SDL_NumJoysticks();
    if (joystick_count > 0) {
        g_joystick = SDL_JoystickOpen(0);
        if (g_joystick) {
            std::cout << "Joystick initialized: " << SDL_JoystickName(g_joystick) << std::endl;
            return true;
        }
    }
    std::cout << "No joystick detected" << std::endl;
    return true; // Not fatal
}

bool initGame() {
    // Create game board with default settings
    // TetrimoneBoard constructor takes no parameters
    g_board = std::make_unique<TetrimoneBoard>();
    if (!g_board) {
        std::cerr << "Game board initialization failed" << std::endl;
        return false;
    }

    g_last_drop_time = SDL_GetTicks();
    g_last_render_time = SDL_GetTicks();

    return true;
}

void shutdownSDL() {
    if (g_joystick) {
        SDL_JoystickClose(g_joystick);
    }
    if (g_gl_context) {
        SDL_GL_DeleteContext(g_gl_context);
    }
    if (g_window) {
        SDL_DestroyWindow(g_window);
    }
    SDL_Quit();
}

void shutdown() {
    g_board.reset();
    shutdownSDL();
}

// ============================================================================
// INPUT HANDLING
// ============================================================================

void handleKeyPress(SDL_KeyboardEvent& key) {
    if (key.repeat) return; // Ignore auto-repeat

    switch (key.keysym.sym) {
        case SDLK_LEFT:
            if (!g_paused && !g_board->isGameOver() && !g_board->isSplashScreenActive()) {
                g_board->movePiece(-1, 0);
            }
            break;
        case SDLK_RIGHT:
            if (!g_paused && !g_board->isGameOver() && !g_board->isSplashScreenActive()) {
                g_board->movePiece(1, 0);
            }
            break;
        case SDLK_DOWN:
            if (!g_paused && !g_board->isGameOver() && !g_board->isSplashScreenActive()) {
                g_board->movePiece(0, 1);
            }
            break;
        case SDLK_UP:
        case SDLK_z:
            if (!g_paused && !g_board->isGameOver() && !g_board->isSplashScreenActive()) {
                g_board->rotatePiece(true);
            }
            break;
        case SDLK_x:
        case SDLK_LCTRL:
            if (!g_paused && !g_board->isGameOver() && !g_board->isSplashScreenActive()) {
                g_board->rotatePiece(false);
            }
            break;
        case SDLK_SPACE:
            if (!g_paused && !g_board->isGameOver() && !g_board->isSplashScreenActive()) {
                g_board->hardDrop();
            }
            break;
        case SDLK_p:
        case SDLK_PAUSE:
            g_paused = !g_paused;
            // NOTE: Adjust these calls based on your actual AudioManager methods
            // if (g_paused) {
            //     // Pause audio if available
            // } else {
            //     // Resume audio if available
            // }
            break;
        case SDLK_ESCAPE:
            g_quit = true;
            break;
        default:
            break;
    }
}

void handleJoystickInput() {
    if (!g_joystick) return;

    // Axis input (left stick for movement)
    int x_axis = SDL_JoystickGetAxis(g_joystick, 0);
    int y_axis = SDL_JoystickGetAxis(g_joystick, 1);

    const int JOYSTICK_DEADZONE = 8000;

    if (!g_paused && !g_board->isGameOver() && !g_board->isSplashScreenActive()) {
        // Horizontal movement
        if (x_axis < -JOYSTICK_DEADZONE) {
            g_board->movePiece(-1, 0);
        } else if (x_axis > JOYSTICK_DEADZONE) {
            g_board->movePiece(1, 0);
        }

        // Vertical movement
        if (y_axis > JOYSTICK_DEADZONE) {
            g_board->movePiece(0, 1);
        }
    }

    // Button input
    for (int i = 0; i < SDL_JoystickNumButtons(g_joystick); ++i) {
        if (SDL_JoystickGetButton(g_joystick, i)) {
            if (i == 0) { // A button - rotate CW
                if (!g_paused && !g_board->isGameOver() && !g_board->isSplashScreenActive()) {
                    g_board->rotatePiece(true);
                }
            } else if (i == 1) { // B button - rotate CCW
                if (!g_paused && !g_board->isGameOver() && !g_board->isSplashScreenActive()) {
                    g_board->rotatePiece(false);
                }
            } else if (i == 2) { // X button - hard drop
                if (!g_paused && !g_board->isGameOver() && !g_board->isSplashScreenActive()) {
                    g_board->hardDrop();
                }
            } else if (i == 3) { // Y button - pause
                g_paused = !g_paused;
            }
        }
    }
}

// ============================================================================
// GAME LOOP
// ============================================================================

void update() {
    if (g_paused || g_board->isGameOver()) {
        return;
    }

    Uint32 current_time = SDL_GetTicks();
    if (current_time - g_last_drop_time >= static_cast<Uint32>(g_drop_speed)) {
        // NOTE: Your TetrimoneBoard may not have dropPiece()
        // Check what methods exist and call the appropriate one
        // Possibilities: update(), tick(), step(), or automatic in movePiece()
        
        // For now, we'll try calling the piece movement down
        // Replace this with the actual method your board uses
        g_board->movePiece(0, 1);  // Move down
        
        g_last_drop_time = current_time;
    }
}

void render() {
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    int window_width, window_height;
    SDL_GetWindowSize(g_window, &window_width, &window_height);

    // Draw game - use your existing OpenGL rendering functions
    // These should be in your drawgame_gl.cpp
    
    // Set up projection
    gl_setup_2d_projection(window_width, window_height);
    
    // Draw game components
    drawBackground_gl(g_board.get(), window_width, window_height);
    drawGridLines_gl(g_board.get());
    drawPlacedBlocks_gl(g_board.get(), nullptr);
    drawCurrentPiece_gl(g_board.get());
    drawGhostPiece_gl(g_board.get());
    
    // Optional effects
    if (g_board->isFireworksActive()) {
        drawFireworks_gl(g_board.get());
    }
    if (g_board->isBlockTrailsActive()) {
        drawBlockTrails_gl(g_board.get());
    }

    // Draw UI overlays
    if (g_board->isPaused()) {
        drawPauseMenu_gl(g_board.get());
    }

    if (g_board->isGameOver()) {
        drawGameOver_gl(g_board.get());
    }

    if (g_board->isShowingPropagandaMessage()) {
        drawPropagandaMessage_gl(g_board.get());
    }

    SDL_GL_SwapWindow(g_window);
}

// ============================================================================
// MAIN
// ============================================================================

int main([[maybe_unused]] int argc, [[maybe_unused]] char* argv[]) {
    // Initialize SDL and OpenGL
    TetrimoneApp* app;
    if (!initSDLGL()) {
        std::cerr << "SDL initialization failed" << std::endl;
        return 1;
    }

    // Initialize audio
    if (!initAudio()) {
        std::cerr << "Audio initialization failed (non-fatal)" << std::endl;
        // Continue anyway - audio not essential
    }

    // Initialize joystick
    initJoystick();

    // Initialize game
    if (!initGame()) {
        std::cerr << "Game initialization failed" << std::endl;
        shutdown();
        return 1;
    }

    // Main loop
    bool running = true;
    SDL_Event event;

    auto frame_start = std::chrono::high_resolution_clock::now();
    const float target_fps = 60.0f;
    const float frame_time = 1.0f / target_fps;

    while (running && !g_quit) {
        auto frame_time_start = std::chrono::high_resolution_clock::now();

        // Event handling
        while (SDL_PollEvent(&event)) {
            switch (event.type) {
                case SDL_QUIT:
                    running = false;
                    break;
                case SDL_KEYDOWN:
                    handleKeyPress(event.key);
                    break;
                case SDL_WINDOWEVENT:
                    if (event.window.event == SDL_WINDOWEVENT_CLOSE) {
                        running = false;
                    } else if (event.window.event == SDL_WINDOWEVENT_RESIZED) {
                        int new_width = event.window.data1;
                        int new_height = event.window.data2;
                        glViewport(0, 0, new_width, new_height);
                    }
                    break;
            }
        }

        // Joystick input (continuous polling)
        handleJoystickInput();

        // Update game state
        update();

        // Render
        render();

        // Frame rate limiting
        auto frame_time_end = std::chrono::high_resolution_clock::now();
        float elapsed = std::chrono::duration<float>(frame_time_end - frame_time_start).count();
        float sleep_time = frame_time - elapsed;

        if (sleep_time > 0) {
            SDL_Delay(static_cast<Uint32>(sleep_time * 1000.0f));
        }
    }

    shutdown();
    return 0;
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
