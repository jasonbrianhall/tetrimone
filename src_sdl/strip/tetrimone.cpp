#ifdef GTK3
#include "tetrimone_gtk.h"
#endif

#ifdef QT5
#include "tetrimone_qt5.h"
#endif

#include "audiomanager.h"
#include <iostream>
#include <string>
#include <algorithm>
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

// Global variables
int BLOCK_SIZE = 30;
int currentThemeIndex = 0;
int GRID_WIDTH = 10;
int GRID_HEIGHT = 22;

int keyDownTimer = 0;
int keyLeftTimer = 0;
int keyRightTimer = 0;
int keyDownDelay = 150;
int keyLeftDelay = 150;
int keyRightDelay = 150;
int keyDownCount = 0;
int keyLeftCount = 0;
int keyRightCount = 0;
bool keyDownPressed = false;
bool keyLeftPressed = false;
bool keyRightPressed = false;

// ============================================================================
// TetrimoneBlock class implementation
// ============================================================================
TetrimoneBlock::TetrimoneBlock(int type) : type(type), rotation(0) {
  // Start pieces centered at top
  x = GRID_WIDTH / 2 - 2;
  y = 0;
}

int TetrimoneBlock::getRotation() const { return rotation; }

void TetrimoneBlock::rotate(bool clockwise) {
  rotation = (rotation + (clockwise ? 1 : 3)) % 4;
}

std::vector<std::vector<int>> TetrimoneBlock::getShape() const {
  return TETRIMONEBLOCK_SHAPES[type][rotation];
}

std::array<double, 3> TetrimoneBlock::getColor() const {
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

// ============================================================================
// TetrimoneBoard class implementation
// ============================================================================
TetrimoneBoard::TetrimoneBoard()
    : score(0), level(1), linesCleared(0), gameOver(false), paused(false),
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
  propagandaMessageDuration = 2000;

  fireworksActive = false;
  fireworksTimer = 0;
  fireworksType = 0;

  trailsEnabled = true;
  maxTrailSegments = 3;
  trailOpacity = 0.6;
  trailDuration = 0.1;
  trailUpdateTimer = 0;
  lastTrailTime = std::chrono::high_resolution_clock::now();

  heatLevel = 0.5f;
  heatDecayTimer = 0;
  grid.resize(MAX_GRID_HEIGHT, std::vector<int>(MAX_GRID_WIDTH, 0));
  nextPieces.resize(3);
  generateNewPiece();

  if (loadBackgroundImagesFromZip("background.zip")) {
    std::cout << "Successfully loaded background images from background.zip" << std::endl;
    useBackgroundImage = true;
    useBackgroundZip = true;
  } else {
    std::cout << "Could not load background.zip, backgrounds will need to be loaded manually" << std::endl;
  }
  for (int i = 0; i < 5; i++) {
    enabledTracks[i] = true;
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

void TetrimoneBoard::restart() {
  // Clear the grid
  for (auto &row : grid) {
    std::fill(row.begin(), row.end(), 0);
  }
  heatLevel = 0.5f;
#ifdef GTK3
  heatDecayTimer = 0;
#endif
#ifdef QT5
  if (heatDecayTimer != nullptr) {
    heatDecayTimer->stop();
    delete heatDecayTimer;
    heatDecayTimer = nullptr;
  }
#endif
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

TetrimoneBoard::~TetrimoneBoard() {
    // Cancel any ongoing transition and clean up resources
    cancelBackgroundTransition();

    // Cancel propaganda message timers
#ifdef GTK3
    if (propagandaTimerId > 0) {
        g_source_remove(propagandaTimerId);
        propagandaTimerId = 0;
    }
    
    if (propagandaScaleTimerId > 0) {
        g_source_remove(propagandaScaleTimerId);
        propagandaScaleTimerId = 0;
    }
#endif

#ifdef QT5
    if (propagandaTimerId != nullptr) {
        propagandaTimerId->stop();
        delete propagandaTimerId;
        propagandaTimerId = nullptr;
    }
    
    if (propagandaScaleTimerId != nullptr) {
        propagandaScaleTimerId->stop();
        delete propagandaScaleTimerId;
        propagandaScaleTimerId = nullptr;
    }
#endif

    if (backgroundImage != nullptr) {
        cairo_surface_destroy(backgroundImage);
        backgroundImage = nullptr;
    }

#ifdef GTK3
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

    if (fireworksTimer > 0) {
        g_source_remove(fireworksTimer);
        fireworksTimer = 0;
    }

    if (trailUpdateTimer > 0) {
        g_source_remove(trailUpdateTimer);
        trailUpdateTimer = 0;
    }
#endif

#ifdef QT5
    if (themeTransitionTimer != nullptr) {
        themeTransitionTimer->stop();
        delete themeTransitionTimer;
        themeTransitionTimer = nullptr;
    }

    if (lineClearAnimationTimer != nullptr) {
        lineClearAnimationTimer->stop();
        delete lineClearAnimationTimer;
        lineClearAnimationTimer = nullptr;
    }

    if (smoothMovementTimer != nullptr) {
        smoothMovementTimer->stop();
        delete smoothMovementTimer;
        smoothMovementTimer = nullptr;
    }

    if (fireworksTimer != nullptr) {
        fireworksTimer->stop();
        delete fireworksTimer;
        fireworksTimer = nullptr;
    }

    if (trailUpdateTimer != nullptr) {
        trailUpdateTimer->stop();
        delete trailUpdateTimer;
        trailUpdateTimer = nullptr;
    }
#endif

    // Clean up any background images from ZIP
    cleanupBackgroundImages();
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
      
#ifdef GTK3
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
                  return true; // Continue the timer
              }
              // Stop the timer if message is no longer showing
              board->propagandaScaleTimerId = 0;
              return FALSE;
          }, 
          this);
#endif

#ifdef QT5
      // Cancel existing timer if any
      if (propagandaTimerId != nullptr) {
          propagandaTimerId->stop();
          delete propagandaTimerId;
      }
      
      // Set timer to hide message after duration
      propagandaTimerId = new QTimer();
      QObject::connect(propagandaTimerId, &QTimer::timeout, [this]() {
          this->showPropagandaMessage = false;
          this->propagandaTimerId->stop();
          delete this->propagandaTimerId;
          this->propagandaTimerId = nullptr;
      });
      propagandaTimerId->setSingleShot(true);
      propagandaTimerId->start(propagandaMessageDuration);
          
      // Set timer for pulsing animation effect
      propagandaMessageScale = 0.7; // Start smaller and grow
      propagandaScalingUp = true;
      if (propagandaScaleTimerId != nullptr) {
          propagandaScaleTimerId->stop();
          delete propagandaScaleTimerId;
      }
      propagandaScaleTimerId = new QTimer();
      QObject::connect(propagandaScaleTimerId, &QTimer::timeout, [this]() {
          if (this->showPropagandaMessage) {
              // Update scale for pulsing effect
              if (this->propagandaScalingUp) {
                  this->propagandaMessageScale += 0.04;
                  if (this->propagandaMessageScale >= 1.2) {
                      this->propagandaScalingUp = false;
                  }
              } else {
                  this->propagandaMessageScale -= 0.04;
                  if (this->propagandaMessageScale <= 0.8) {
                      this->propagandaScalingUp = true;
                  }
              }
          } else {
              // Stop the timer if message is no longer showing
              this->propagandaScaleTimerId->stop();
              delete this->propagandaScaleTimerId;
              this->propagandaScaleTimerId = nullptr;
          }
      });
      propagandaScaleTimerId->start(50);
#endif
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
        
#ifdef GTK3
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
                    return true; // Continue the timer
                }
                // Stop the timer if message is no longer showing
                board->propagandaScaleTimerId = 0;
                return FALSE;
            }, 
            this);
#endif

#ifdef QT5
        // Cancel existing timer if any
        if (propagandaTimerId != nullptr) {
            propagandaTimerId->stop();
            delete propagandaTimerId;
        }
        
        // Set timer to hide message after duration
        propagandaTimerId = new QTimer();
        QObject::connect(propagandaTimerId, &QTimer::timeout, [this]() {
            this->showPropagandaMessage = false;
            this->propagandaTimerId->stop();
            delete this->propagandaTimerId;
            this->propagandaTimerId = nullptr;
        });
        propagandaTimerId->setSingleShot(true);
        propagandaTimerId->start(propagandaMessageDuration);
            
        // Set timer for patriotic pulsing animation effect
        propagandaMessageScale = 0.8; // Start slightly smaller for freedom effect
        propagandaScalingUp = true;
        if (propagandaScaleTimerId != nullptr) {
            propagandaScaleTimerId->stop();
            delete propagandaScaleTimerId;
        }
        propagandaScaleTimerId = new QTimer();
        QObject::connect(propagandaScaleTimerId, &QTimer::timeout, [this]() {
            if (this->showPropagandaMessage) {
                // Update scale for patriotic pulsing effect
                if (this->propagandaScalingUp) {
                    this->propagandaMessageScale += 0.03; // Slightly gentler scaling
                    if (this->propagandaMessageScale >= 1.15) { // Less extreme scaling
                        this->propagandaScalingUp = false;
                    }
                } else {
                    this->propagandaMessageScale -= 0.03;
                    if (this->propagandaMessageScale <= 0.85) { // More conservative minimum
                        this->propagandaScalingUp = true;
                    }
                }
            } else {
                // Stop the timer if message is no longer showing
                this->propagandaScaleTimerId->stop();
                delete this->propagandaScaleTimerId;
                this->propagandaScaleTimerId = nullptr;
            }
        });
        propagandaScaleTimerId->start(60);
#endif
    }
    
    // Award points based on lines cleared
    int points = 0;
    switch (linesCleared) {
      case 1:
        points = 100;
        playSound(GameSoundEvent::Single);
        break;
      case 2:
        points = 300;
        playSound(GameSoundEvent::Double);
        break;
      case 3:
        points = 500;
        playSound(GameSoundEvent::Triple);
        break;
      case 4:
        points = 800;
        playSound(GameSoundEvent::Tetrimone);
        break;
    }
    
    score += points;
  }
  
  return linesCleared;
}

bool TetrimoneBoard::isGameOver() const {
  // If this is the first time checking game over status since it became true,
  // play the game over sound
  bool soundPlayed = false;
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

void TetrimoneBlock::move(int dx, int dy) {
  x += dx;
  y += dy;
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
#ifdef GTK3
        trailUpdateTimer = g_timeout_add(TRAIL_UPDATE_INTERVAL,
            [](gpointer userData) -> gboolean {
                TetrimoneBoard* board = static_cast<TetrimoneBoard*>(userData);
                board->updateBlockTrails();
                
                // Force redraw
                if (board->app) {
                    drawBoard(board);
                }
                
                return true; // Keep timer running
            }, this);
#endif // GTK3

#ifdef QT5
        TetrimoneApp* qtApp = static_cast<TetrimoneApp*>(app);
        if (qtApp) {
            QTimer* timer = new QTimer();
            timer->setInterval(TRAIL_UPDATE_INTERVAL);
            QObject::connect(timer, &QTimer::timeout, [this]() {
                this->updateBlockTrails();
                if (this->app) {
                    drawBoard(this);
                }
            });
            timer->start();
            trailUpdateTimer = 1; // Non-zero to indicate timer is running
        }
#endif // QT5
    }
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

void TetrimoneBoard::updateBlockTrails() {
    if (!trailsEnabled) {
        blockTrails.clear();
        
#ifdef GTK3
        // GTK3: Remove g_source timer
        if (trailUpdateTimer != 0) {
            g_source_remove(trailUpdateTimer);
            trailUpdateTimer = 0;
        }
#endif
        return;
    }
    
    double deltaTime = TRAIL_UPDATE_INTERVAL / 1000.0;
    
    for (auto it = blockTrails.begin(); it != blockTrails.end();) {
        BlockTrail& trail = *it;
        trail.life -= deltaTime;
        trail.alpha = (trail.life / trail.maxLife) * trailOpacity;
        
        if (trail.life <= 0.0 || trail.alpha <= 0.05) {
            it = blockTrails.erase(it);
        } else {
            ++it;
        }
    }
    
    if (blockTrails.empty() && trailUpdateTimer != 0) {
#ifdef GTK3
        // GTK3: Remove timer
        g_source_remove(trailUpdateTimer);
        trailUpdateTimer = 0;
#endif
    }
}


int TetrimoneBoard::getGridValue(int x, int y) const {
  if (x < 0 || x >= GRID_WIDTH || y < 0 || y >= GRID_HEIGHT) {
    return 0;
  }
  return grid[y][x];
}

void drawBoard(TetrimoneBoard *board) {
#ifdef GTK3
     gtk_widget_queue_draw(board->app->gameArea);
#endif

#ifdef QT5
    board->app->gameArea->update();
#endif
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

