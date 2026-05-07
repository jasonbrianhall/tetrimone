// ============================================================================
// Generic Junk Line Generation Logic (Framework-Agnostic)
// ============================================================================

#include "audiomanager.h"
#include "highscores.h"
#include <iostream>
#include <string>
#include <algorithm>

#ifdef GTK3
#include "tetrimone_gtk.h"
#include "gtk3_dialog_helpers.h"
using namespace GTK3Helpers;
#endif

#ifdef QT5
#include "tetrimone_qt5.h"
#include "qt5_dialog_helpers.h"
#endif

#ifdef _WIN32
#include <windows.h>
#include <commdlg.h>
#endif

// ============================================================================
// Pure Logic: Junk Line Generation
// ============================================================================

// ============================================================================
// Public Junk Line Generation Methods
// ============================================================================

/**
 * Generate junk lines at the bottom of the grid.
 * This function is framework-agnostic and contains only game logic.
 * 
 * @param percentage Percentage of grid height to fill with junk (0-100)
 */
void TetrimoneBoard::generateJunkLines(int percentage) {
  // Calculate how many lines to fill with junk
  int junkLines = (GRID_HEIGHT * percentage) / 100;
  
  // Ensure we don't fill more than the grid height
  junkLines = std::min(junkLines, GRID_HEIGHT - 0); // Leave at least 5 rows empty at top
  
  if (junkLines <= 0) return;
  
  fillJunkRows(GRID_HEIGHT - junkLines, GRID_HEIGHT - 1);
  
  // If we have a current piece, make sure it doesn't collide with junk lines
  if (currentPiece) {
    repositionPieceAboveJunk(currentPiece.get(), GRID_HEIGHT - junkLines);
  }
}

/**
 * Add junk lines from the bottom, shifting existing content up.
 * This function is framework-agnostic and contains only game logic.
 * 
 * @param numLines Number of junk lines to add from the bottom
 */
void TetrimoneBoard::addJunkLinesFromBottom(int numLines) {
  if (numLines <= 0) return;
  
  // Limit to available space
  numLines = std::min(numLines, GRID_HEIGHT - 5); // Leave at least 5 rows at top
  
  // Handle current piece before shifting grid content
  if (currentPiece) {
    shiftCurrentPieceUp(numLines);
  }
  
  // Move existing content up to make room for junk lines
  shiftGridContentUp(numLines);
  
  // Fill the bottom rows with junk
  fillJunkRows(GRID_HEIGHT - numLines, GRID_HEIGHT - 1);
  
  // Reposition the current piece to avoid collisions
  if (currentPiece) {
    ensureValidPiecePosition();
  }
  
  // Play a warning sound when junk lines are added
  playSound(GameSoundEvent::LevelUp);
}

// ============================================================================
// Helper Methods: Grid Manipulation (Framework-Agnostic)
// ============================================================================

/**
 * Fill a range of rows with junk blocks, leaving random gaps.
 * 
 * @param startRow First row to fill (inclusive)
 * @param endRow Last row to fill (inclusive)
 */
void TetrimoneBoard::fillJunkRows(int startRow, int endRow) {
  for (int y = startRow; y <= endRow; y++) {
    // Create a list of empty spaces (at least 4 per row)
    int emptySpaces = 4 + std::rand() % (GRID_WIDTH / 3); // Between 4 and 1/3 of width
    std::vector<int> emptyPositions = generateRandomPositions(emptySpaces, GRID_WIDTH);
    
    // Fill the row with blocks, ensuring empty spaces where specified
    int prevType = 1 + std::rand() % 7; // Random block type (1-7)
    int typeCount = 0; // Count of current type (max 3 of each type)
    
    for (int x = 0; x < GRID_WIDTH; x++) {
      // Skip if this position should be empty
      if (std::find(emptyPositions.begin(), emptyPositions.end(), x) != emptyPositions.end()) {
        grid[y][x] = 0; // Empty space
        continue;
      }
      
      // Generate a new block, with higher probability of same type as neighbor
      int newType = selectNextBlockType(prevType, typeCount);
      
      if (newType != prevType) {
        prevType = newType;
        typeCount = 1;
      } else {
        typeCount++;
      }
      
      grid[y][x] = newType;
    }
  }
}

/**
 * Generate a list of unique random positions within a range.
 * 
 * @param count Number of positions to generate
 * @param maxWidth Maximum value for positions (exclusive)
 * @return Vector of unique random positions
 */
std::vector<int> TetrimoneBoard::generateRandomPositions(int count, int maxWidth) {
  std::vector<int> positions;
  while ((int)positions.size() < count) {
    int pos = std::rand() % maxWidth;
    if (std::find(positions.begin(), positions.end(), pos) == positions.end()) {
      positions.push_back(pos);
    }
  }
  return positions;
}

/**
 * Select the next block type, enforcing variety constraints.
 * 
 * @param currentType Previous block type
 * @param currentTypeCount How many consecutive blocks of current type
 * @return Next block type (1-7)
 */
int TetrimoneBoard::selectNextBlockType(int currentType, int currentTypeCount) {
  if (currentTypeCount >= 3) {
    // Force a new type if we already have 3 of the current type
    int newType;
    do {
      newType = 1 + std::rand() % 7;
    } while (newType == currentType);
    return newType;
  } else {
    // 70% chance to continue with same type, 30% chance for a new type
    if (std::rand() % 10 < 7) {
      return currentType;
    } else {
      int newType;
      do {
        newType = 1 + std::rand() % 7;
      } while (newType == currentType);
      return newType;
    }
  }
}

/**
 * Shift all grid content up by a specified number of rows.
 * 
 * @param numRows Number of rows to shift up
 */
void TetrimoneBoard::shiftGridContentUp(int numRows) {
  for (int y = 0; y < GRID_HEIGHT - numRows; y++) {
    for (int x = 0; x < GRID_WIDTH; x++) {
      grid[y][x] = grid[y + numRows][x];
    }
  }
}

/**
 * Shift the current piece up to make room for incoming junk lines.
 * 
 * @param numRows Number of rows to shift
 */
void TetrimoneBoard::shiftCurrentPieceUp(int numRows) {
  if (!currentPiece) return;
  
  int currentY = currentPiece->getY();
  
  // If current piece is too low, move it up by the number of junk lines
  if (currentY > GRID_HEIGHT - numRows - 4) {
    currentPiece->setPosition(currentPiece->getX(), std::max(0, currentY - numRows));
  }
}

/**
 * Reposition the current piece to avoid collisions with junk lines.
 * Tries multiple strategies to find a valid position.
 */
void TetrimoneBoard::ensureValidPiecePosition() {
  if (!currentPiece) return;
  
  int currentX = currentPiece->getX();
  int currentY = currentPiece->getY();
  
  if (!checkCollision(*currentPiece)) {
    return; // Already valid
  }
  
  bool foundValidPosition = false;
  
  // Strategy 1: Try moving up from current position
  for (int testY = currentY - 1; testY >= 0; testY--) {
    currentPiece->setPosition(currentX, testY);
    if (!checkCollision(*currentPiece)) {
      foundValidPosition = true;
      break;
    }
  }
  
  // Strategy 2: Try different X and Y positions in upper area
  if (!foundValidPosition) {
    for (int testY = 0; testY < 4; testY++) {
      for (int testX = 0; testX <= GRID_WIDTH - 4; testX++) {
        currentPiece->setPosition(testX, testY);
        if (!checkCollision(*currentPiece)) {
          foundValidPosition = true;
          break;
        }
      }
      if (foundValidPosition) break;
    }
  }
  
  // Strategy 3: Default to top center
  if (!foundValidPosition) {
    currentPiece->setPosition(GRID_WIDTH / 2 - 2, 0);
    
    // Only set game over if there's still a collision at the top center
    if (checkCollision(*currentPiece)) {
      gameOver = true;
    }
  }
}

/**
 * Reposition piece above junk lines generated at game start.
 * 
 * @param piece The current piece to reposition
 * @param junkStartRow The row where junk starts
 */
void TetrimoneBoard::repositionPieceAboveJunk(TetrimoneBlock* piece, int junkStartRow) {
  if (!piece) return;
  
  int originalX = piece->getX();
  
  // Move the piece to the top of the board
  piece->setPosition(originalX, 0);
  
  // Check collision
  if (!checkCollision(*piece)) {
    return; // Position is valid
  }
  
  // Try different positions in the top rows
  bool foundValidPosition = false;
  
  for (int testX = 0; testX <= GRID_WIDTH - 4; testX++) {
    piece->setPosition(testX, 0);
    if (!checkCollision(*piece)) {
      foundValidPosition = true;
      break;
    }
  }
  
  // If still can't find a valid position, reset to center top
  if (!foundValidPosition) {
    piece->setPosition(GRID_WIDTH / 2 - 2, 0);
  }
}

// ============================================================================
// GTK3-Specific: Game Setup Dialog Handler
// ============================================================================

#ifdef GTK3

// Forward declarations
bool confirmGameRestart(TetrimoneApp* app);
void applyGameSetupSettings(TetrimoneApp* app, int junkPercentage, int junkPerLevel, int initialLevel);

/**
 * Apply callback for game setup dialog.
 * Handles GTK3-specific confirmation dialog logic.
 */
void onGameSetupApply(int junkPercentage, int junkPerLevel, int initialLevel, gpointer userData) {
    TetrimoneApp* app = static_cast<TetrimoneApp*>(userData);
    
    // Check if settings have changed
    bool settingsChanged = (junkPercentage != app->board->junkLinesPercentage) || 
                          (junkPerLevel != app->board->junkLinesPerLevel) ||
                          (initialLevel != app->board->initialLevel);
    
    if (!settingsChanged) {
        return; // No changes, nothing to do
    }
    
    // Use framework-agnostic confirmation
    if (!confirmGameRestart(app)) {
        return; // User cancelled
    }
    
    // Apply the new settings
    applyGameSetupSettings(app, junkPercentage, junkPerLevel, initialLevel);
}

/**
 * Confirm with user before restarting game.
 * GTK3-specific implementation.
 * 
 * @return true if user confirmed, false otherwise
 */
bool confirmGameRestart(TetrimoneApp* app) {
    GtkWidget* confirmDialog = gtk_message_dialog_new(
        GTK_WINDOW(app->window),
        GTK_DIALOG_MODAL,
        GTK_MESSAGE_QUESTION,
        GTK_BUTTONS_YES_NO,
        "Changing game settings will restart the current game. Continue?"
    );
    
    gint response = gtk_dialog_run(GTK_DIALOG(confirmDialog));
    gtk_widget_destroy(confirmDialog);
    
    return (response == GTK_RESPONSE_YES);
}

/**
 * Apply game setup settings and restart the game.
 * This is framework-agnostic game logic.
 */
void applyGameSetupSettings(TetrimoneApp* app, int junkPercentage, int junkPerLevel, int initialLevel) {
    // Update settings
    app->board->junkLinesPercentage = junkPercentage;
    app->board->junkLinesPerLevel = junkPerLevel;
    app->board->initialLevel = initialLevel;
    
    // Restart the game with new settings
    app->board->restart();
    resetUI(app);
    
    // Start game with new settings
    if (app->board->isPaused()) {
        app->board->togglePause();
        gtk_menu_item_set_label(GTK_MENU_ITEM(app->pauseMenuItem), "Pause");
    }
    
    gtk_widget_set_sensitive(app->startMenuItem, FALSE);
    gtk_widget_set_sensitive(app->pauseMenuItem, true);
    
    startGame(app);
}

/**
 * Open game setup dialog from menu.
 * GTK3-specific menu callback.
 */
void onGameSetupDialog(GtkMenuItem* menuItem, gpointer userData) {
    TetrimoneApp* app = static_cast<TetrimoneApp*>(userData);
    
    GameSetupConfig config{
        .title = "Game Setup",
        .junkPercentage = app->board->junkLinesPercentage,
        .junkPerLevel = app->board->junkLinesPerLevel,
        .initialLevel = app->board->initialLevel,
        .width = 500,
        .height = 450
    };
    
    createGameSetupDialog(GTK_WINDOW(app->window), config, onGameSetupApply, app);
}

#endif // GTK3
