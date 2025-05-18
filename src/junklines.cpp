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

void onGameSetupDialog(GtkMenuItem* menuItem, gpointer userData) {
  TetrimoneApp *app = static_cast<TetrimoneApp *>(userData);

  // Create dialog
  GtkWidget *dialog = gtk_dialog_new_with_buttons(
      "Game Setup", GTK_WINDOW(app->window), GTK_DIALOG_MODAL, 
      "Apply", GTK_RESPONSE_APPLY, 
      "Cancel", GTK_RESPONSE_CANCEL, 
      NULL);

  // Get content area
  GtkWidget *contentArea = gtk_dialog_get_content_area(GTK_DIALOG(dialog));
  gtk_container_set_border_width(GTK_CONTAINER(contentArea), 10);

  // Create main VBox
  GtkWidget *vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 10);
  gtk_container_add(GTK_CONTAINER(contentArea), vbox);

  // Junk Lines Percentage
  GtkWidget *junkFrame = gtk_frame_new("Initial Junk Lines");
  gtk_box_pack_start(GTK_BOX(vbox), junkFrame, TRUE, TRUE, 0);

  GtkWidget *junkBox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 5);
  gtk_container_add(GTK_CONTAINER(junkFrame), junkBox);

  // Junk lines percentage slider
  GtkAdjustment *junkAdj = gtk_adjustment_new(
      app->board->junkLinesPercentage, // Initial value
      0,        // Minimum value (0%)
      50,       // Maximum value (50%)
      1,        // Step increment
      5,        // Page increment
      0         // Page size (not used)
  );

  GtkWidget *junkScale = gtk_scale_new(GTK_ORIENTATION_HORIZONTAL, junkAdj);
  gtk_scale_set_digits(GTK_SCALE(junkScale), 0); // No decimal places
  gtk_scale_set_value_pos(GTK_SCALE(junkScale), GTK_POS_RIGHT);
  gtk_box_pack_start(GTK_BOX(junkBox), junkScale, TRUE, TRUE, 0);

  GtkWidget *junkLabel = gtk_label_new("Percentage of board to fill with junk lines (0-50%)");
  gtk_box_pack_start(GTK_BOX(junkBox), junkLabel, FALSE, FALSE, 0);

  GtkWidget *junkDescription = gtk_label_new(
      "Junk lines contain random blocks with at least 4 empty spaces per row.\n"
      "Similar colors have a higher chance of being placed adjacent to each other.");
  gtk_label_set_line_wrap(GTK_LABEL(junkDescription), TRUE);
  gtk_box_pack_start(GTK_BOX(junkBox), junkDescription, FALSE, FALSE, 5);
  
  // Junk Lines Per Level
  GtkWidget *junkPerLevelFrame = gtk_frame_new("Junk Lines Per Level");
  gtk_box_pack_start(GTK_BOX(vbox), junkPerLevelFrame, TRUE, TRUE, 0);

  GtkWidget *junkPerLevelBox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 5);
  gtk_container_add(GTK_CONTAINER(junkPerLevelFrame), junkPerLevelBox);

  // Junk lines per level slider
  GtkAdjustment *junkPerLevelAdj = gtk_adjustment_new(
      app->board->junkLinesPerLevel, // Initial value
      0,        // Minimum value (0 lines)
      5,        // Maximum value (5 lines)
      1,        // Step increment
      1,        // Page increment
      0         // Page size (not used)
  );

  GtkWidget *junkPerLevelScale = gtk_scale_new(GTK_ORIENTATION_HORIZONTAL, junkPerLevelAdj);
  gtk_scale_set_digits(GTK_SCALE(junkPerLevelScale), 0); // No decimal places
  gtk_scale_set_value_pos(GTK_SCALE(junkPerLevelScale), GTK_POS_RIGHT);
  gtk_box_pack_start(GTK_BOX(junkPerLevelBox), junkPerLevelScale, TRUE, TRUE, 0);

  GtkWidget *junkPerLevelLabel = gtk_label_new("Number of junk lines to add when advancing to a new level (0-5)");
  gtk_box_pack_start(GTK_BOX(junkPerLevelBox), junkPerLevelLabel, FALSE, FALSE, 0);

  GtkWidget *junkPerLevelDescription = gtk_label_new(
      "These junk lines will push up from the bottom of the board\n"
      "when you advance to a new level, increasing the challenge.");
  gtk_label_set_line_wrap(GTK_LABEL(junkPerLevelDescription), TRUE);
  gtk_box_pack_start(GTK_BOX(junkPerLevelBox), junkPerLevelDescription, FALSE, FALSE, 5);

  // Initial Level
  GtkWidget *levelFrame = gtk_frame_new("Starting Level");
  gtk_box_pack_start(GTK_BOX(vbox), levelFrame, TRUE, TRUE, 0);

  GtkWidget *levelBox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 5);
  gtk_container_add(GTK_CONTAINER(levelFrame), levelBox);

  // Level slider
  GtkAdjustment *levelAdj = gtk_adjustment_new(
      app->board->initialLevel, // Initial value
      1,       // Minimum value (level 1)
      100,     // Maximum value (level 100)
      1,       // Step increment
      5,       // Page increment
      0        // Page size (not used)
  );

  GtkWidget *levelScale = gtk_scale_new(GTK_ORIENTATION_HORIZONTAL, levelAdj);
  gtk_scale_set_digits(GTK_SCALE(levelScale), 0); // No decimal places
  gtk_scale_set_value_pos(GTK_SCALE(levelScale), GTK_POS_RIGHT);
  gtk_box_pack_start(GTK_BOX(levelBox), levelScale, TRUE, TRUE, 0);

  GtkWidget *levelLabel = gtk_label_new("Start at higher levels for increased difficulty and points");
  gtk_box_pack_start(GTK_BOX(levelBox), levelLabel, FALSE, FALSE, 0);

  // Warning message about restarting game
  GtkWidget *warningLabel = gtk_label_new(NULL);
  gtk_label_set_markup(GTK_LABEL(warningLabel), 
                    "<span foreground='red'>Note:</span> Applying these settings will restart the current game.");
  gtk_box_pack_start(GTK_BOX(vbox), warningLabel, FALSE, FALSE, 10);

  // Show all widgets
  gtk_widget_show_all(dialog);

  // Run dialog
  gint response = gtk_dialog_run(GTK_DIALOG(dialog));

  if (response == GTK_RESPONSE_APPLY) {
    // Get the new settings
    int newJunkPercentage = (int)gtk_adjustment_get_value(junkAdj);
    int newJunkPerLevel = (int)gtk_adjustment_get_value(junkPerLevelAdj);
    int newLevel = (int)gtk_adjustment_get_value(levelAdj);

    // Check if settings have changed
    bool settingsChanged = (newJunkPercentage != app->board->junkLinesPercentage) || 
                          (newJunkPerLevel != app->board->junkLinesPerLevel) ||
                          (newLevel != app->board->initialLevel);

    if (settingsChanged) {
      // Create confirmation dialog
      GtkWidget *confirmDialog = gtk_message_dialog_new(
          GTK_WINDOW(dialog), GTK_DIALOG_MODAL, GTK_MESSAGE_QUESTION,
          GTK_BUTTONS_YES_NO,
          "Changing game settings will restart the current game. Continue?");

      // Run confirmation dialog and get response
      gint confirmResponse = gtk_dialog_run(GTK_DIALOG(confirmDialog));
      gtk_widget_destroy(confirmDialog);

      // If user confirmed, apply changes and restart the game
      if (confirmResponse == GTK_RESPONSE_YES) {
        // Update settings
        app->board->junkLinesPercentage = newJunkPercentage;
        app->board->junkLinesPerLevel = newJunkPerLevel;
        app->board->initialLevel = newLevel;

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

  // Destroy dialog
  gtk_widget_destroy(dialog);
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
