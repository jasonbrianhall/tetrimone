// Enhanced drawgame.cpp with smooth animations
#include "tetrimone.h"
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

// Define M_PI for Windows compatibility
#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

void TetrimoneBoard::getCurrentPieceInterpolatedPosition(double &x, double &y) const {
  if (currentPiece) {
    if (smoothMovementTimer > 0 && movementProgress < 1.0) {
      // Interpolate between last position and current position
      double t = movementProgress;
      // Use easing function for smoother movement
      t = t * t * (3.0 - 2.0 * t); // Smoothstep function
      
      x = lastPieceX + (currentPiece->getX() - lastPieceX) * t;
      y = lastPieceY + (currentPiece->getY() - lastPieceY) * t;
    } else {
      x = currentPiece->getX();
      y = currentPiece->getY();
    }
  } else {
    x = 0;
    y = 0;
  }
}

bool TetrimoneBoard::isLineBeingCleared(int y) const {
  return std::find(linesBeingCleared.begin(), linesBeingCleared.end(), y) != linesBeingCleared.end();
}

void TetrimoneBoard::startSmoothMovement(int newX, int newY) {
  if (currentPiece) {
    lastPieceX = currentPiece->getX();
    lastPieceY = currentPiece->getY();
    movementProgress = 0.0;
    
    // Set start time for this animation
    movementStartTime = std::chrono::high_resolution_clock::now();
    
    // Only start animation if there's actual movement
    if (lastPieceX != newX || lastPieceY != newY) {
      if (smoothMovementTimer > 0) {
        g_source_remove(smoothMovementTimer);
      }
      
      smoothMovementTimer = g_timeout_add(16, // ~60 FPS
        [](gpointer userData) -> gboolean {
          TetrimoneBoard* board = static_cast<TetrimoneBoard*>(userData);
          board->updateSmoothMovement();
          
          // FORCE SCREEN REPAINT
          if (board->app) {
            gtk_widget_queue_draw(board->app->gameArea);
          }
          
          return TRUE;
        }, this);
    }
  }
}


void TetrimoneBoard::updateSmoothMovement() {
  auto now = std::chrono::high_resolution_clock::now();
  auto totalMs = std::chrono::duration<double, std::milli>(now - movementStartTime).count();
  
  movementProgress = totalMs / MOVEMENT_ANIMATION_DURATION;
  
  if (movementProgress >= 1.0) {
    movementProgress = 1.0;
    if (smoothMovementTimer > 0) {
      g_source_remove(smoothMovementTimer);
      smoothMovementTimer = 0;
    }
  }
}

void TetrimoneBoard::startLineClearAnimation(const std::vector<int> &clearedLines) {
  linesBeingCleared = clearedLines;
  lineClearActive = true;
  lineClearProgress = 0.0;
  
  // Set start time for this animation
  lineClearStartTime = std::chrono::high_resolution_clock::now();
  
  // Randomly select animation type for modern mode (0-9)
  if (!retroModeActive) {
    std::uniform_int_distribution<int> animDist(0, 9);
    currentAnimationType = animDist(rng);
  }
  
  if (lineClearAnimationTimer > 0) {
    g_source_remove(lineClearAnimationTimer);
  }
  
  lineClearAnimationTimer = g_timeout_add(16, // ~60 FPS
    [](gpointer userData) -> gboolean {
      TetrimoneBoard* board = static_cast<TetrimoneBoard*>(userData);
      board->updateLineClearAnimation();
      
      // FORCE SCREEN REPAINT
      if (board->app) {
        gtk_widget_queue_draw(board->app->gameArea);
      }
      
      return TRUE;
    }, this);
}


void TetrimoneBoard::updateLineClearAnimation() {
  auto now = std::chrono::high_resolution_clock::now();
  auto totalMs = std::chrono::duration<double, std::milli>(now - lineClearStartTime).count();
  
  lineClearProgress = totalMs / LINE_CLEAR_ANIMATION_DURATION;
  
  if (lineClearProgress >= 1.0) {
    // Animation complete - stop timer first
    if (lineClearAnimationTimer > 0) {
      g_source_remove(lineClearAnimationTimer);
      lineClearAnimationTimer = 0;
    }
    
    // Set progress to exactly 1.0
    lineClearProgress = 1.0;
    
    // Sort lines in descending order (bottom to top) to remove correctly
    std::vector<int> sortedLines = linesBeingCleared;
    std::sort(sortedLines.begin(), sortedLines.end(), std::greater<int>());
    
    // Remove the lines from the grid (bottom to top)
    for (int lineY : sortedLines) {
      // Verify this line actually needs to be cleared
      bool lineIsFull = true;
      for (int x = 0; x < GRID_WIDTH; ++x) {
        if (grid[lineY][x] == 0) {
          lineIsFull = false;
          break;
        }
      }
      
      // Only remove if line is actually full
      if (lineIsFull) {
        // Move all rows above this line down by one
        for (int moveY = lineY; moveY > 0; --moveY) {
          for (int x = 0; x < GRID_WIDTH; ++x) {
            grid[moveY][x] = grid[moveY - 1][x];
          }
        }
        
        // Clear the top row
        for (int x = 0; x < GRID_WIDTH; ++x) {
          grid[0][x] = 0;
        }
        
        // After removing this line, adjust positions of remaining lines
        for (int& otherLineY : sortedLines) {
          if (otherLineY < lineY) {
            otherLineY++;
          }
        }
      }
    }
    
    // Clean up animation state
    linesBeingCleared.clear();
    lineClearActive = false;
    lineClearProgress = 0.0;
  }
}

void TetrimoneBoard::startThemeTransition(int targetTheme) {
    // Don't start transition if already transitioning to the same theme
    if (isThemeTransitioning && newThemeIndex == targetTheme) {
        return;
    }
    
    // Cancel any existing transition
    if (themeTransitionTimer > 0) {
        g_source_remove(themeTransitionTimer);
    }
    
    // Set up transition
    oldThemeIndex = currentThemeIndex;
    newThemeIndex = targetTheme;
    isThemeTransitioning = true;
    themeTransitionProgress = 0.0;
    
    // Set start time for this animation
    themeStartTime = std::chrono::high_resolution_clock::now();
    
    themeTransitionTimer = g_timeout_add(16, // ~60 FPS
        [](gpointer userData) -> gboolean {
            TetrimoneBoard* board = static_cast<TetrimoneBoard*>(userData);
            board->updateThemeTransition();
            
            // FORCE SCREEN REPAINT
            if (board->app) {
                gtk_widget_queue_draw(board->app->gameArea);
                gtk_widget_queue_draw(board->app->nextPieceArea); // For theme color changes
            }
            
            return TRUE;
        }, this);
}

void TetrimoneBoard::updateThemeTransition() {
    auto now = std::chrono::high_resolution_clock::now();
    auto totalMs = std::chrono::duration<double, std::milli>(now - themeStartTime).count();
    
    themeTransitionProgress = totalMs / THEME_TRANSITION_DURATION;
    
    if (themeTransitionProgress >= 1.0) {
        // Transition complete
        themeTransitionProgress = 1.0;
        currentThemeIndex = newThemeIndex;
        
        // Clean up
        if (themeTransitionTimer > 0) {
            g_source_remove(themeTransitionTimer);
            themeTransitionTimer = 0;
        }
        
        isThemeTransitioning = false;
    }
}

void TetrimoneBoard::cancelThemeTransition() {
    if (themeTransitionTimer > 0) {
        g_source_remove(themeTransitionTimer);
        themeTransitionTimer = 0;
    }
    
    isThemeTransitioning = false;
    themeTransitionProgress = 0.0;
}

std::array<double, 3> TetrimoneBoard::getInterpolatedColor(int blockType, double progress) const {
    if (!isThemeTransitioning) {
        return TETRIMONEBLOCK_COLOR_THEMES[currentThemeIndex][blockType];
    }
    
    // Get colors from old and new themes
    auto oldColor = TETRIMONEBLOCK_COLOR_THEMES[oldThemeIndex][blockType];
    auto newColor = TETRIMONEBLOCK_COLOR_THEMES[newThemeIndex][blockType];
    
    // Use a more gentle easing function for the longer transition
    // Cubic ease-in-out for smoother, more noticeable transitions
    double t = themeTransitionProgress;
    if (t < 0.5) {
        t = 4 * t * t * t; // Ease-in cubic
    } else {
        t = 1 - pow(-2 * t + 2, 3) / 2; // Ease-out cubic
    }
    
    // Interpolate between colors
    std::array<double, 3> interpolatedColor;
    for (int i = 0; i < 3; i++) {
        interpolatedColor[i] = oldColor[i] + (newColor[i] - oldColor[i]) * t;
    }
    
    return interpolatedColor;
}

