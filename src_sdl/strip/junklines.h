#ifndef JUNKLINES_H
#define JUNKLINES_H

#include <vector>

// Forward declaration
struct TetrimoneApp;
class Tetrimone;

/**
 * Public Junk Line Generation Methods
 * (Should be added to TetrimoneBoard class)
 */

/**
 * Generate junk lines at the bottom of the grid based on a percentage.
 * @param percentage Percentage of grid height to fill (0-100)
 */
void generateJunkLines(int percentage);

/**
 * Add junk lines from the bottom, shifting existing content up.
 * @param numLines Number of junk lines to add
 */
void addJunkLinesFromBottom(int numLines);

// ============================================================================
// Private Helper Methods (Framework-Agnostic Grid Operations)
// ============================================================================

/**
 * Fill a range of rows with randomly-placed junk blocks.
 * Ensures variety in block types and leaves random gaps.
 * 
 * @param startRow First row to fill (inclusive)
 * @param endRow Last row to fill (inclusive)
 */
void fillJunkRows(int startRow, int endRow);

/**
 * Generate unique random positions within a range.
 * Used for placing empty gaps in junk rows.
 * 
 * @param count Number of unique positions to generate
 * @param maxWidth Maximum value (exclusive)
 * @return Vector of unique random positions sorted
 */
std::vector<int> generateRandomPositions(int count, int maxWidth);

/**
 * Select the next block type for junk row filling.
 * Enforces constraints: max 3 consecutive blocks of same type.
 * 
 * @param currentType The previous block type (1-7)
 * @param currentTypeCount How many consecutive blocks of currentType
 * @return Next block type (1-7), different if constraint violated
 */
int selectNextBlockType(int currentType, int currentTypeCount);

/**
 * Shift all grid content up by specified rows to make room for junk.
 * 
 * @param numRows Rows to shift up
 */
void shiftGridContentUp(int numRows);

/**
 * Shift the current piece up to keep it valid during grid shifts.
 * 
 * @param numRows Rows to shift up
 */
void shiftCurrentPieceUp(int numRows);

/**
 * Reposition the current piece above junk lines at game start.
 * Uses multiple strategies to find a collision-free position.
 * 
 * @param piece Pointer to current Tetrimone piece
 * @param junkStartRow The row where junk starts
 */
void repositionPieceAboveJunk(Tetrimone* piece, int junkStartRow);

/**
 * Ensure the current piece has a valid position (no collisions).
 * Tries up, then different XY positions, then defaults to top center.
 * Sets gameOver flag if top center is also invalid.
 */
void ensureValidPiecePosition();

// ============================================================================
// Framework-Specific Handlers (Conditional Compilation)
// ============================================================================

#ifdef GTK3

/**
 * Callback for game setup dialog apply button.
 * Checks if settings changed and prompts for confirmation before restart.
 */
void onGameSetupApply(int junkPercentage, int junkPerLevel, int initialLevel, gpointer userData);

/**
 * Confirm game restart with user via GTK3 dialog.
 * 
 * @param app Application pointer
 * @return true if user clicked Yes, false otherwise
 */
bool confirmGameRestart(TetrimoneApp* app);

/**
 * Apply new game settings and restart the game.
 * Updates board settings, clears the board, and starts a new game.
 */
void applyGameSetupSettings(TetrimoneApp* app, int junkPercentage, int junkPerLevel, int initialLevel);

/**
 * Menu callback to open the game setup dialog.
 * GTK3-specific implementation.
 */
void onGameSetupDialog(GtkMenuItem* menuItem, gpointer userData);

#endif // GTK3

#endif // JUNKLINES_H
