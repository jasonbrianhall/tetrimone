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

void TetrimoneBlock::rotate(bool clockwise) {
  rotation = (rotation + (clockwise ? 1 : 3)) % 4;
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
                  return true; // Continue the timer
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
                    return true; // Continue the timer
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
