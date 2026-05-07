#ifdef GTK3
#include "tetrimone_gtk.h"
#include "gtk3_dialog_helpers.h"
#endif

#ifdef QT5
#include "tetrimone_qt5.h"
#include "qt5_dialog_helpers.h"
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

#ifdef GTK3
using namespace GTK3Helpers;
#endif

// Method to generate junk lines
void TetrimoneBoard::generateJunkLines(int percentage) {
  // Calculate how many lines to fill with junk
  int junkLines = (GRID_HEIGHT * percentage) / 100;
  
  // Ensure we don't fill more than the grid height
  junkLines = std::min(junkLines, GRID_HEIGHT - 0); // Leave at least 5 rows empty at top
  
  if (junkLines <= 0) return;
  
  // Fill bottom rows with junk
  for (int y = GRID_HEIGHT - 1; y >= GRID_HEIGHT - junkLines; y--) {
    // Create a list of empty spaces (at least 4 per row)
    int emptySpaces = 4 + std::rand() % (GRID_WIDTH / 3); // Between 4 and 1/3 of width
    std::vector<int> emptyPositions;
    
    // Generate positions for empty spaces
    while (emptyPositions.size() < emptySpaces) {
      int pos = std::rand() % GRID_WIDTH;
      // Check if this position is already in the list
      if (std::find(emptyPositions.begin(), emptyPositions.end(), pos) == emptyPositions.end()) {
        emptyPositions.push_back(pos);
      }
    }
    
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
      int newType;
      if (typeCount >= 3) {
        // Force a new type if we already have 3 of the current type
        do {
          newType = 1 + std::rand() % 7;
        } while (newType == prevType);
      } else {
        // 70% chance to continue with same type, 30% chance for a new type
        if (std::rand() % 10 < 7) {
          newType = prevType;
          typeCount++;
        } else {
          do {
            newType = 1 + std::rand() % 7;
          } while (newType == prevType);
          
          // Reset type count for the new type
          prevType = newType;
          typeCount = 1;
        }
      }
      
      grid[y][x] = newType;
    }
  }
  
  // If we have a current piece, make sure it doesn't collide with junk lines
  if (currentPiece) {
    // Store original position
    int originalX = currentPiece->getX();
    int originalY = currentPiece->getY();
    
    // Move the piece to the top of the board
    currentPiece->setPosition(originalX, 0);
    
    // Check collision
    if (checkCollision(*currentPiece)) {
      // Try different positions in the top row
      bool foundValidPosition = false;
      
      // Try each possible X position in the top row
      for (int testX = 0; testX <= GRID_WIDTH - 4; testX++) {
        currentPiece->setPosition(testX, 0);
        if (!checkCollision(*currentPiece)) {
          foundValidPosition = true;
          break;
        }
      }
      
      // If still can't find a valid position, reset game state but don't end game
      if (!foundValidPosition) {
        currentPiece->setPosition(GRID_WIDTH / 2 - 2, 0);
      }
    }
  }
}

// Callback for game setup dialog
void onGameSetupApply(int junkPercentage, int junkPerLevel, int initialLevel, gpointer userData) {
    TetrimoneApp* app = static_cast<TetrimoneApp*>(userData);
    
    // Check if settings have changed
    bool settingsChanged = (junkPercentage != app->board->junkLinesPercentage) || 
                          (junkPerLevel != app->board->junkLinesPerLevel) ||
                          (initialLevel != app->board->initialLevel);
    
    if (settingsChanged) {
        // Create confirmation dialog
        GtkWidget* confirmDialog = gtk_message_dialog_new(
            GTK_WINDOW(app->window), GTK_DIALOG_MODAL, GTK_MESSAGE_QUESTION,
            GTK_BUTTONS_YES_NO,
            "Changing game settings will restart the current game. Continue?");
        
        gint confirmResponse = gtk_dialog_run(GTK_DIALOG(confirmDialog));
        gtk_widget_destroy(confirmDialog);
        
        if (confirmResponse == GTK_RESPONSE_YES) {
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
            gtk_widget_set_sensitive(app->pauseMenuItem, TRUE);
            
            startGame(app);
        }
    }
}

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


void TetrimoneBoard::addJunkLinesFromBottom(int numLines) {
  if (numLines <= 0) return;
  
  // Limit to available space
  numLines = std::min(numLines, GRID_HEIGHT - 5); // Leave at least 5 rows at top
  
  // Store current piece position if it exists
  int currentX = 0;
  int currentY = 0;
  int currentType = 0;
  int currentRotation = 0;
  bool hasPiece = false;
  
  if (currentPiece) {
    hasPiece = true;
    currentX = currentPiece->getX();
    currentY = currentPiece->getY();
    currentType = currentPiece->getType();
    currentRotation = currentPiece->getRotation();
    
    // If current piece is too low, move it up by the number of junk lines
    if (currentY > GRID_HEIGHT - numLines - 4) {
      currentY = std::max(0, currentY - numLines);
    }
  }
  
  // Move existing content up to make room for junk lines
  for (int y = 0; y < GRID_HEIGHT - numLines; y++) {
    for (int x = 0; x < GRID_WIDTH; x++) {
      grid[y][x] = grid[y + numLines][x];
    }
  }
  
  // Fill the bottom rows with junk
  for (int y = GRID_HEIGHT - numLines; y < GRID_HEIGHT; y++) {
    // Create a list of empty spaces (at least 4 per row)
    int emptySpaces = 4 + std::rand() % (GRID_WIDTH / 3); // Between 4 and 1/3 of width
    std::vector<int> emptyPositions;
    
    // Generate positions for empty spaces
    while (emptyPositions.size() < emptySpaces) {
      int pos = std::rand() % GRID_WIDTH;
      // Check if this position is already in the list
      if (std::find(emptyPositions.begin(), emptyPositions.end(), pos) == emptyPositions.end()) {
        emptyPositions.push_back(pos);
      }
    }
    
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
      int newType;
      if (typeCount >= 3) {
        // Force a new type if we already have 3 of the current type
        do {
          newType = 1 + std::rand() % 7;
        } while (newType == prevType);
      } else {
        // 70% chance to continue with same type, 30% chance for a new type
        if (std::rand() % 10 < 7) {
          newType = prevType;
          typeCount++;
        } else {
          do {
            newType = 1 + std::rand() % 7;
          } while (newType == prevType);
          
          // Reset type count for the new type
          prevType = newType;
          typeCount = 1;
        }
      }
      
      grid[y][x] = newType;
    }
  }
  
  // Reposition the current piece to avoid collisions
  if (hasPiece) {
    // Set piece to the stored position (which we've already moved up)
    currentPiece->setPosition(currentX, currentY);
    
    // Check for collision
    if (checkCollision(*currentPiece)) {
      // Try to find a valid position for the piece
      bool foundValidPosition = false;
      
      // First try moving up
      for (int testY = currentY - 1; testY >= 0; testY--) {
        currentPiece->setPosition(currentX, testY);
        if (!checkCollision(*currentPiece)) {
          foundValidPosition = true;
          break;
        }
      }
      
      // If still colliding, try different X positions
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
      
      // If still can't find a position, just place at top center
      if (!foundValidPosition) {
        currentPiece->setPosition(GRID_WIDTH / 2 - 2, 0);
        
        // Only set game over if there's still a collision at the top center
        if (checkCollision(*currentPiece)) {
          gameOver = true;
        }
      }
    }
  }
  
  // Play a warning sound when junk lines are added
  playSound(GameSoundEvent::LevelUp);
}
