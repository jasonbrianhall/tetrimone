#include "tetris.h"
#include "audiomanager.h"
#include <iostream>
#include <string>
#ifdef _WIN32
#include <windows.h>
#include <commdlg.h>
#endif

int BLOCK_SIZE = 30;  // Default value, will be updated at runtime
int currentThemeIndex = 0;

// Tetromino class implementation
Tetromino::Tetromino(int type) : type(type), rotation(0) {
    // Start pieces centered at top
    x = GRID_WIDTH / 2 - 2;
    y = 0;
}

void Tetromino::rotate(bool clockwise) {
    rotation = (rotation + (clockwise ? 1 : 3)) % 4;
}

void Tetromino::move(int dx, int dy) {
    x += dx;
    y += dy;
}

std::vector<std::vector<int>> Tetromino::getShape() const {
    return TETROMINO_SHAPES[type][rotation];
}

std::array<double, 3> Tetromino::getColor() const {
    // Get the current level (need to add a way to pass this info to the Tetromino)
    int themeIndex=currentThemeIndex;

    // Cap at the max theme index
    if (themeIndex >= TETROMINO_COLOR_THEMES.size()) {
        themeIndex = TETROMINO_COLOR_THEMES.size() - 1;
    }
    
    return TETROMINO_COLOR_THEMES[themeIndex][type];
}

void Tetromino::setPosition(int newX, int newY) {
    x = newX;
    y = newY;
}

// TetrisBoard class implementation
TetrisBoard::TetrisBoard() : score(0), level(1), linesCleared(0), gameOver(false), 
                            paused(false), splashScreenActive(true),
                            backgroundImage(nullptr), useBackgroundImage(false),
                            backgroundOpacity(0.3), useBackgroundZip(false), 
                            currentBackgroundIndex(0), isTransitioning(false),
                            transitionOpacity(0.0), transitionDirection(0),
                            oldBackground(nullptr), transitionTimerId(0) {
    rng.seed(std::chrono::system_clock::now().time_since_epoch().count());
    grid.resize(GRID_HEIGHT, std::vector<int>(GRID_WIDTH, 0));
    generateNewPiece();
    generateNewPiece();
    
    // Try to load background.zip by default
    if (loadBackgroundImagesFromZip("background.zip")) {
        std::cout << "Successfully loaded background images from background.zip" << std::endl;
        // Background should be enabled by default if successfully loaded
        useBackgroundImage = true;
        useBackgroundZip = true;
    } else {
        std::cout << "Could not load background.zip, backgrounds will need to be loaded manually" << std::endl;
    }
    for (int i = 0; i < 5; i++) {
        enabledTracks[i] = true;
    }
}

TetrisBoard::~TetrisBoard() {
    // Cancel any ongoing transition and clean up resources
    cancelBackgroundTransition();
    
    if (backgroundImage != nullptr) {
        cairo_surface_destroy(backgroundImage);
        backgroundImage = nullptr;
    }
    
    // Clean up any background images from ZIP
    cleanupBackgroundImages();
}

bool TetrisBoard::movePiece(int dx, int dy) {
    if (gameOver || paused) return false;
    
    currentPiece->move(dx, dy);
    
    if (checkCollision(*currentPiece)) {
        currentPiece->move(-dx, -dy);  // Move back if collision
        return false;
    }
    
    return true;
}

bool TetrisBoard::rotatePiece(bool clockwise) {
    if (gameOver || paused) return false;
    
    currentPiece->rotate(clockwise);
    
    if (checkCollision(*currentPiece)) {
        currentPiece->rotate(!clockwise);  // Rotate back in opposite direction
        return false;
    }
    
    return true;
}

bool TetrisBoard::checkCollision(const Tetromino& piece) const {
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

void TetrisBoard::lockPiece() {
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
                
                if (gridY >= 0 && gridY < GRID_HEIGHT && gridX >= 0 && gridX < GRID_WIDTH) {
                    grid[gridY][gridX] = pieceType + 1;  // +1 so that empty is 0 and pieces are 1-7
                }
            }
        }
    }
}

int TetrisBoard::clearLines() {
    int linesCleared = 0;
    int currentlevel = (this->linesCleared / 10) + 1;
    // Check each row from bottom to top
    for (int y = GRID_HEIGHT - 1; y >= 0; --y) {
        bool isFullLine = true;
        
        // Check if row is full
        for (int x = 0; x < GRID_WIDTH; ++x) {
            if (grid[y][x] == 0) {
                isFullLine = false;
                break;
            }
        }
        
        if (isFullLine) {
            linesCleared++;
            
            // Move all rows above down
            for (int moveY = y; moveY > 0; --moveY) {
                for (int x = 0; x < GRID_WIDTH; ++x) {
                    grid[moveY][x] = grid[moveY - 1][x];
                }
            }
            
            // Clear top row
            for (int x = 0; x < GRID_WIDTH; ++x) {
                grid[0][x] = 0;
            }
            
            // We need to check this row again since we moved everything down
            y++;
        }
    }
    
    // Update score based on lines cleared
    if (linesCleared > 0) {
        // Play appropriate sound based on number of lines cleared
        if (linesCleared == 4) {
            playSound(GameSoundEvent::Excellent); // Play Tetris/Excellent sound for 4 lines
        } else if (linesCleared > 0) {
            playSound(GameSoundEvent::Clear); // Play normal clear sound for 1-3 lines
            if (linesCleared == 1) { playSound(GameSoundEvent::Single); }
            if (linesCleared == 2) { playSound(GameSoundEvent::Double); }
            if (linesCleared == 3) { playSound(GameSoundEvent::Triple); }
            
        }
        // Classic Tetris scoring
        switch (linesCleared) {
            case 1:
                score += 40 * level;
                break;
            case 2:
                score += 100 * level;
                break;
            case 3:
                score += 300 * level;
                break;
            case 4:
                score += 1200 * level;
                break;
        }
        
        // Update total lines cleared
        this->linesCleared += linesCleared;
        
        // Update level every 10 lines
        level = (this->linesCleared / 10) + 1;
    }
    
    if (level > currentlevel) {
        playSound(GameSoundEvent::LevelUp);
        // Every Level, change theme; wrap around at 20
        currentThemeIndex = (currentThemeIndex + 1) % 20;
        
        // Add this section to change background on level up
        if (useBackgroundZip && !backgroundImages.empty()) {
            // Change to a random background on level up if using background zip
            startBackgroundTransition();
        }
    }
    
    return linesCleared;
}

void TetrisBoard::generateNewPiece() {
    // Move next piece to current
    currentPiece = std::move(nextPiece);
    
    // Generate new next piece
    std::uniform_int_distribution<int> dist(0, 6);
    int nextType = dist(rng);
    nextPiece = std::make_unique<Tetromino>(nextType);
    
    // If this is the first piece (current was null)
    if (!currentPiece) {
        currentPiece = std::make_unique<Tetromino>(nextType);
    }
    
    // Check if the new piece collides immediately - game over
    if (checkCollision(*currentPiece)) {
        gameOver = true;
    }
}

void TetrisBoard::updateGame() {
    if (gameOver || paused || splashScreenActive) return;
    
    // Existing update code...
    if (!movePiece(0, 1)) {
        lockPiece();
        clearLines();
        generateNewPiece();
    }
}

void TetrisBoard::hardDrop() {
    if (gameOver || paused) return;
    
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

int TetrisBoard::getGridValue(int x, int y) const {
    if (x < 0 || x >= GRID_WIDTH || y < 0 || y >= GRID_HEIGHT) {
        return 0;
    }
    return grid[y][x];
}

void TetrisBoard::restart() {
    // Clear the grid
    for (auto& row : grid) {
        std::fill(row.begin(), row.end(), 0);
    }
    
    // Reset game state
    score = 0;
    level = 1;
    linesCleared = 0;
    gameOver = false;
    paused = false;
    splashScreenActive = true;  // Show splash screen on restart
    
    // Generate new pieces
    nextPiece.reset();
    currentPiece.reset();
    generateNewPiece();
    generateNewPiece();
    
    // Select a random background if using background images from ZIP
    if (useBackgroundZip && !backgroundImages.empty()) {
        // Start a smooth background transition
        startBackgroundTransition();
    }
}

// GTK+ callback functions
gboolean onDrawGameArea(GtkWidget* widget, cairo_t* cr, gpointer data) {
    TetrisApp* app = static_cast<TetrisApp*>(data);
    TetrisBoard* board = app->board;
    
    // Get widget dimensions
    GtkAllocation allocation;
    gtk_widget_get_allocation(widget, &allocation);
    
    // Draw background
    cairo_set_source_rgb(cr, 0.1, 0.1, 0.1);
    cairo_rectangle(cr, 0, 0, allocation.width, allocation.height);
    cairo_fill(cr);

    // Draw background image if enabled
if ((board->isUsingBackgroundImage() || board->isUsingBackgroundZip()) && board->getBackgroundImage() != nullptr) {
    // Save the current state
    cairo_save(cr);
    
    // Check if we're in a transition
    if (board->isInBackgroundTransition()) {
        // If fading out, draw old background first
        if (board->getTransitionDirection() == -1 && board->getOldBackground() != nullptr) {
            // Get the image dimensions
            int imgWidth = cairo_image_surface_get_width(board->getOldBackground());
            int imgHeight = cairo_image_surface_get_height(board->getOldBackground());
            
            // Calculate scaling to fill the game area while maintaining aspect ratio
            double scaleX = static_cast<double>(allocation.width) / imgWidth;
            double scaleY = static_cast<double>(allocation.height) / imgHeight;
            double scale = std::max(scaleX, scaleY);
            
            // Calculate position to center the image
            double x = (allocation.width - imgWidth * scale) / 2;
            double y = (allocation.height - imgHeight * scale) / 2;
            
            // Apply the transformation
            cairo_translate(cr, x, y);
            cairo_scale(cr, scale, scale);
            
            // Draw the old image with current transition opacity
            cairo_set_source_surface(cr, board->getOldBackground(), 0, 0);
            cairo_paint_with_alpha(cr, board->getTransitionOpacity());
            
            // Reset transformation for next drawing
            cairo_restore(cr);
            cairo_save(cr);
        } else if (board->getTransitionDirection() == 1) {
            // Fading in - draw new background with transition opacity
            // Get the image dimensions
            int imgWidth = cairo_image_surface_get_width(board->getBackgroundImage());
            int imgHeight = cairo_image_surface_get_height(board->getBackgroundImage());
            
            // Calculate scaling to fill the game area while maintaining aspect ratio
            double scaleX = static_cast<double>(allocation.width) / imgWidth;
            double scaleY = static_cast<double>(allocation.height) / imgHeight;
            double scale = std::max(scaleX, scaleY);
            
            // Calculate position to center the image
            double x = (allocation.width - imgWidth * scale) / 2;
            double y = (allocation.height - imgHeight * scale) / 2;
            
            // Apply the transformation
            cairo_translate(cr, x, y);
            cairo_scale(cr, scale, scale);
            
            // Draw the new image with transition opacity
            cairo_set_source_surface(cr, board->getBackgroundImage(), 0, 0);
            cairo_paint_with_alpha(cr, board->getTransitionOpacity());
        }
    } else {
        // Normal drawing (no transition)
        // Get the image dimensions
        int imgWidth = cairo_image_surface_get_width(board->getBackgroundImage());
        int imgHeight = cairo_image_surface_get_height(board->getBackgroundImage());
        
        // Calculate scaling to fill the game area while maintaining aspect ratio
        double scaleX = static_cast<double>(allocation.width) / imgWidth;
        double scaleY = static_cast<double>(allocation.height) / imgHeight;
        double scale = std::max(scaleX, scaleY);
        
        // Calculate position to center the image
        double x = (allocation.width - imgWidth * scale) / 2;
        double y = (allocation.height - imgHeight * scale) / 2;
        
        // Apply the transformation
        cairo_translate(cr, x, y);
        cairo_scale(cr, scale, scale);
        
        // Draw the image with normal opacity
        cairo_set_source_surface(cr, board->getBackgroundImage(), 0, 0);
        cairo_paint_with_alpha(cr, board->getBackgroundOpacity());
    }
    
    // Restore the original state
    cairo_restore(cr);
}
    

    
    // Draw grid lines
    cairo_set_source_rgb(cr, 0.3, 0.3, 0.3);
    cairo_set_line_width(cr, 1);
    
    // Vertical lines
    for (int x = 1; x < GRID_WIDTH; ++x) {
        cairo_move_to(cr, x * BLOCK_SIZE, 0);
        cairo_line_to(cr, x * BLOCK_SIZE, GRID_HEIGHT * BLOCK_SIZE);
    }
    
    // Horizontal lines
    for (int y = 1; y < GRID_HEIGHT; ++y) {
        cairo_move_to(cr, 0, y * BLOCK_SIZE);
        cairo_line_to(cr, GRID_WIDTH * BLOCK_SIZE, y * BLOCK_SIZE);
    }
    
    cairo_stroke(cr);

    int failureLineY = 2; // Position the line at the second row
    cairo_set_source_rgb(cr, 1.0, 0.2, 0.2); // Bright red for visibility
    cairo_set_line_width(cr, 1.0); 
    cairo_move_to(cr, 0, failureLineY * BLOCK_SIZE);
    cairo_line_to(cr, GRID_WIDTH * BLOCK_SIZE, failureLineY * BLOCK_SIZE);
    cairo_stroke(cr);
    
    // Draw placed blocks
    for (int y = 0; y < GRID_HEIGHT; ++y) {
        for (int x = 0; x < GRID_WIDTH; ++x) {
            int value = board->getGridValue(x, y);
            if (value > 0) {
                // Get color from tetromino colors (value-1 because grid values are 1-based)
               auto color = TETROMINO_COLOR_THEMES[currentThemeIndex][value - 1];
               cairo_set_source_rgb(cr, color[0], color[1], color[2]);
                
                // Draw block with a small margin
                cairo_rectangle(cr, 
                    x * BLOCK_SIZE + 1, 
                    y * BLOCK_SIZE + 1, 
                    BLOCK_SIZE - 2, 
                    BLOCK_SIZE - 2);
                cairo_fill(cr);
                
                // Draw highlight (3D effect)
                cairo_set_source_rgba(cr, 1, 1, 1, 0.3);
                cairo_move_to(cr, x * BLOCK_SIZE + 1, y * BLOCK_SIZE + 1);
                cairo_line_to(cr, x * BLOCK_SIZE + BLOCK_SIZE - 1, y * BLOCK_SIZE + 1);
                cairo_line_to(cr, x * BLOCK_SIZE + 1, y * BLOCK_SIZE + BLOCK_SIZE - 1);
                cairo_close_path(cr);
                cairo_fill(cr);
                
                // Draw shadow (3D effect)
                cairo_set_source_rgba(cr, 0, 0, 0, 0.3);
                cairo_move_to(cr, x * BLOCK_SIZE + BLOCK_SIZE - 1, y * BLOCK_SIZE + 1);
                cairo_line_to(cr, x * BLOCK_SIZE + BLOCK_SIZE - 1, y * BLOCK_SIZE + BLOCK_SIZE - 1);
                cairo_line_to(cr, x * BLOCK_SIZE + 1, y * BLOCK_SIZE + BLOCK_SIZE - 1);
                cairo_close_path(cr);
                cairo_fill(cr);
            }
        }
    }
    
    // Draw splash screen if active
    if (board->isSplashScreenActive()) {
        // Semi-transparent overlay
        cairo_set_source_rgba(cr, 0, 0, 0, 0.7);
        cairo_rectangle(cr, 0, 0, GRID_WIDTH * BLOCK_SIZE, GRID_HEIGHT * BLOCK_SIZE);
        cairo_fill(cr);
        
        // Draw title
        cairo_select_font_face(cr, "Sans", CAIRO_FONT_SLANT_NORMAL, CAIRO_FONT_WEIGHT_BOLD);
        cairo_set_font_size(cr, 40);
        cairo_set_source_rgb(cr, 1, 1, 1);
        
        // Center the title
        cairo_text_extents_t extents;
        const char* title = "TETRIS";
        cairo_text_extents(cr, title, &extents);
        
        double x = (GRID_WIDTH * BLOCK_SIZE - extents.width) / 2;
        double y = (GRID_HEIGHT * BLOCK_SIZE) / 3;
        
        cairo_move_to(cr, x, y);
        cairo_show_text(cr, title);
        
        // Draw colored blocks for decoration
        int blockSize = 30;
        int startX = (GRID_WIDTH * BLOCK_SIZE - 4 * blockSize) / 2;
        int startY = y + 20;
        
        // Draw I piece (cyan)
        cairo_set_source_rgb(cr, 0.0, 0.7, 0.9);
        for (int i = 0; i < 4; i++) {
            cairo_rectangle(cr, startX + i * blockSize, startY, blockSize - 2, blockSize - 2);
            cairo_fill(cr);
        }
        
        // Draw T piece (purple)
        cairo_set_source_rgb(cr, 0.8, 0.0, 0.8);
        startY += blockSize * 1.5;
        cairo_rectangle(cr, startX + blockSize, startY, blockSize - 2, blockSize - 2);
        cairo_fill(cr);
        cairo_rectangle(cr, startX, startY + blockSize, blockSize - 2, blockSize - 2);
        cairo_fill(cr);
        cairo_rectangle(cr, startX + blockSize, startY + blockSize, blockSize - 2, blockSize - 2);
        cairo_fill(cr);
        cairo_rectangle(cr, startX + blockSize * 2, startY + blockSize, blockSize - 2, blockSize - 2);
        cairo_fill(cr);
        
        // Draw press space message
        cairo_set_font_size(cr, 20);
        const char* startText = "Press SPACE to Start";
        cairo_text_extents(cr, startText, &extents);
        
        x = (GRID_WIDTH * BLOCK_SIZE - extents.width) / 2;
        y = (GRID_HEIGHT * BLOCK_SIZE) * 0.75;
        
        cairo_move_to(cr, x, y);
        cairo_show_text(cr, startText);
        
        // Draw joystick message if enabled
        if (app->joystickEnabled) {
            cairo_set_font_size(cr, 16);
            const char* joystickText = "or Press START on Controller";
            cairo_text_extents(cr, joystickText, &extents);
            
            x = (GRID_WIDTH * BLOCK_SIZE - extents.width) / 2;
            y += 30;
            
            cairo_move_to(cr, x, y);
            cairo_show_text(cr, joystickText);
        }
        
        return FALSE;  // Skip drawing the rest
    }
    
    // Draw current piece if game is active
    if (!board->isGameOver() && !board->isPaused() && !board->isSplashScreenActive()) {
        const Tetromino& piece = board->getCurrentPiece();
        auto shape = piece.getShape();
        auto color = piece.getColor();
        int pieceX = piece.getX();
        int pieceY = piece.getY();
        
        cairo_set_source_rgb(cr, color[0], color[1], color[2]);
        
        for (size_t y = 0; y < shape.size(); ++y) {
            for (size_t x = 0; x < shape[y].size(); ++x) {
                if (shape[y][x] == 1) {
                    int drawX = (pieceX + x) * BLOCK_SIZE;
                    int drawY = (pieceY + y) * BLOCK_SIZE;
                    
                    // Only draw if within the visible grid
                    if (drawY >= 0) {
                        // Draw block with a small margin
                        cairo_rectangle(cr,
                            drawX + 1, 
                            drawY + 1, 
                            BLOCK_SIZE - 2, 
                            BLOCK_SIZE - 2);
                        cairo_fill(cr);
                        
                        // Draw highlight (3D effect)
                        cairo_set_source_rgba(cr, 1, 1, 1, 0.3);
                        cairo_move_to(cr, drawX + 1, drawY + 1);
                        cairo_line_to(cr, drawX + BLOCK_SIZE - 1, drawY + 1);
                        cairo_line_to(cr, drawX + 1, drawY + BLOCK_SIZE - 1);
                        cairo_close_path(cr);
                        cairo_fill(cr);
                        
                        // Draw shadow (3D effect)
                        cairo_set_source_rgba(cr, 0, 0, 0, 0.3);
                        cairo_move_to(cr, drawX + BLOCK_SIZE - 1, drawY + 1);
                        cairo_line_to(cr, drawX + BLOCK_SIZE - 1, drawY + BLOCK_SIZE - 1);
                        cairo_line_to(cr, drawX + 1, drawY + BLOCK_SIZE - 1);
                        cairo_close_path(cr);
                        cairo_fill(cr);
                        
                        // Reset color for next block
                        cairo_set_source_rgb(cr, color[0], color[1], color[2]);
                    }
                }
            }
        }
    }
    
    // Draw enhanced pause menu if paused
    if (board->isPaused() && !board->isGameOver()) {
        cairo_set_source_rgba(cr, 0, 0, 0, 0.7);
        cairo_rectangle(cr, 0, 0, GRID_WIDTH * BLOCK_SIZE, GRID_HEIGHT * BLOCK_SIZE);
        cairo_fill(cr);
        
        cairo_select_font_face(cr, "Sans", CAIRO_FONT_SLANT_NORMAL, CAIRO_FONT_WEIGHT_BOLD);
        cairo_set_font_size(cr, 30);
        cairo_set_source_rgb(cr, 1, 1, 1);
        
        // Center the text
        cairo_text_extents_t extents;
        const char* text = "PAUSED";
        cairo_text_extents(cr, text, &extents);
        
        double x = (GRID_WIDTH * BLOCK_SIZE - extents.width) / 2;
        double y = (GRID_HEIGHT * BLOCK_SIZE) / 4;
        
        cairo_move_to(cr, x, y);
        cairo_show_text(cr, text);
        
        // Draw pause menu options
        const int numOptions = 3;
        const char* menuOptions[numOptions] = {
            "Continue (P)",
            "New Game (N)",
            "Quit (Q)"
        };
        
        cairo_set_font_size(cr, 20);
        
        // Calculate center position
        y = (GRID_HEIGHT * BLOCK_SIZE) / 2;
        
        // Draw menu options
        for (int i = 0; i < numOptions; i++) {
            cairo_text_extents(cr, menuOptions[i], &extents);
            x = (GRID_WIDTH * BLOCK_SIZE - extents.width) / 2;
            
            cairo_move_to(cr, x, y);
            cairo_show_text(cr, menuOptions[i]);
            
            y += 40;
        }
    }
    
    // Draw game over text if needed
    if (board->isGameOver()) {
        cairo_set_source_rgba(cr, 0, 0, 0, 0.7);
        cairo_rectangle(cr, 0, 0, GRID_WIDTH * BLOCK_SIZE, GRID_HEIGHT * BLOCK_SIZE);
        cairo_fill(cr);
        
        cairo_select_font_face(cr, "Sans", CAIRO_FONT_SLANT_NORMAL, CAIRO_FONT_WEIGHT_BOLD);
        cairo_set_font_size(cr, 30);
        cairo_set_source_rgb(cr, 1, 0, 0);
        
        // Center the text
        cairo_text_extents_t extents;
        const char* text = "GAME OVER";
        cairo_text_extents(cr, text, &extents);
        
        double x = (GRID_WIDTH * BLOCK_SIZE - extents.width) / 2;
        double y = (GRID_HEIGHT * BLOCK_SIZE) / 2;
        
        cairo_move_to(cr, x, y);
        cairo_show_text(cr, text);
        
        // Show "Press R to restart" message
        cairo_set_font_size(cr, 16);
        const char* restartText = "Press R to restart";
        cairo_text_extents(cr, restartText, &extents);
        
        x = (GRID_WIDTH * BLOCK_SIZE - extents.width) / 2;
        y += 40;
        
        cairo_move_to(cr, x, y);
        cairo_show_text(cr, restartText);
    }
    
    return FALSE;
}

void onBlockSizeDialog(GtkMenuItem* menuItem, gpointer userData) {
    TetrisApp* app = static_cast<TetrisApp*>(userData);
    
    // Pause the game if it's running
    bool wasPaused = app->board->isPaused();
    if (!wasPaused && !app->board->isGameOver() && !app->board->isSplashScreenActive()) {
        onPauseGame(GTK_MENU_ITEM(app->pauseMenuItem), app);
    }
    
    // Store original block size in case user cancels
    int originalBlockSize = BLOCK_SIZE;
    int newBlockSize = BLOCK_SIZE; // Working value
    
    // Create dialog with Apply and Cancel buttons
    GtkWidget* dialog = gtk_dialog_new_with_buttons(
        "Block Size",
        GTK_WINDOW(app->window),
        GTK_DIALOG_MODAL,
        "_Cancel", GTK_RESPONSE_CANCEL,
        "_Apply", GTK_RESPONSE_APPLY,
        NULL
    );
    
    // Make it a reasonable size
    gtk_window_set_default_size(GTK_WINDOW(dialog), 300, 150);
    
    // Create content area
    GtkWidget* contentArea = gtk_dialog_get_content_area(GTK_DIALOG(dialog));
    gtk_container_set_border_width(GTK_CONTAINER(contentArea), 10);
    
    // Create a vertical box for content
    GtkWidget* vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 6);
    gtk_container_add(GTK_CONTAINER(contentArea), vbox);
    
    // Add a label
    GtkWidget* label = gtk_label_new("Adjust block size:");
    gtk_widget_set_halign(label, GTK_ALIGN_START);
    gtk_box_pack_start(GTK_BOX(vbox), label, FALSE, FALSE, 0);
    
    // Create a horizontal scale (slider)
    GtkWidget* scale = gtk_scale_new_with_range(GTK_ORIENTATION_HORIZONTAL, 
                                             MIN_BLOCK_SIZE, MAX_BLOCK_SIZE, 1);
    gtk_range_set_value(GTK_RANGE(scale), BLOCK_SIZE);
    gtk_scale_set_digits(GTK_SCALE(scale), 0); // No decimal places
    gtk_scale_set_value_pos(GTK_SCALE(scale), GTK_POS_RIGHT);
    gtk_box_pack_start(GTK_BOX(vbox), scale, FALSE, FALSE, 0);
    
    // Add min/max labels
    GtkWidget* rangeBox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
    gtk_box_pack_start(GTK_BOX(vbox), rangeBox, FALSE, FALSE, 0);
    
    GtkWidget* minLabel = gtk_label_new("Small");
    gtk_widget_set_halign(minLabel, GTK_ALIGN_START);
    gtk_box_pack_start(GTK_BOX(rangeBox), minLabel, TRUE, TRUE, 0);
    
    GtkWidget* maxLabel = gtk_label_new("Large");
    gtk_widget_set_halign(maxLabel, GTK_ALIGN_END);
    gtk_box_pack_end(GTK_BOX(rangeBox), maxLabel, TRUE, TRUE, 0);
    
    // Current value label that updates as the slider moves
    GtkWidget* currentValueLabel = gtk_label_new(NULL);
    char valueBuf[32];
    snprintf(valueBuf, sizeof(valueBuf), "Current size: %d", BLOCK_SIZE);
    gtk_label_set_text(GTK_LABEL(currentValueLabel), valueBuf);
    gtk_widget_set_halign(currentValueLabel, GTK_ALIGN_START);
    gtk_box_pack_start(GTK_BOX(vbox), currentValueLabel, FALSE, FALSE, 0);
    
    // Note about application
    GtkWidget* noteLabel = gtk_label_new("Click Apply to set the new block size.\nThis will reset the game UI.");
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
    if (!wasPaused && !app->board->isGameOver() && !app->board->isSplashScreenActive()) {
        onPauseGame(GTK_MENU_ITEM(app->pauseMenuItem), app);
    }
}

gboolean onDrawNextPiece(GtkWidget* widget, cairo_t* cr, gpointer data) {
    TetrisApp* app = static_cast<TetrisApp*>(data);
    TetrisBoard* board = app->board;
    
    // Get widget dimensions
    GtkAllocation allocation;
    gtk_widget_get_allocation(widget, &allocation);
    
    // Draw background
    cairo_set_source_rgb(cr, 0.1, 0.1, 0.1);
    // Draw background
    cairo_set_source_rgb(cr, 0.1, 0.1, 0.1);
    cairo_rectangle(cr, 0, 0, allocation.width, allocation.height);
    cairo_fill(cr);
    
    if (!board->isGameOver()) {
        // Draw next piece
        const Tetromino& piece = board->getNextPiece();
        auto shape = piece.getShape();
        auto color = piece.getColor();
        
        cairo_set_source_rgb(cr, color[0], color[1], color[2]);
        
        // Center the piece in the preview area
        int offsetX = (allocation.width - 4 * BLOCK_SIZE) / 2;
        int offsetY = (allocation.height - 4 * BLOCK_SIZE) / 2;
        
        for (size_t y = 0; y < shape.size(); ++y) {
            for (size_t x = 0; x < shape[y].size(); ++x) {
                if (shape[y][x] == 1) {
                    int drawX = offsetX + x * BLOCK_SIZE;
                    int drawY = offsetY + y * BLOCK_SIZE;
                    
                    // Draw block with a small margin
                    cairo_rectangle(cr,
                        drawX + 1, 
                        drawY + 1, 
                        BLOCK_SIZE - 2, 
                        BLOCK_SIZE - 2);
                    cairo_fill(cr);
                    
                    // Draw highlight (3D effect)
                    cairo_set_source_rgba(cr, 1, 1, 1, 0.3);
                    cairo_move_to(cr, drawX + 1, drawY + 1);
                    cairo_line_to(cr, drawX + BLOCK_SIZE - 1, drawY + 1);
                    cairo_line_to(cr, drawX + 1, drawY + BLOCK_SIZE - 1);
                    cairo_close_path(cr);
                    cairo_fill(cr);
                    
                    // Draw shadow (3D effect)
                    cairo_set_source_rgba(cr, 0, 0, 0, 0.3);
                    cairo_move_to(cr, drawX + BLOCK_SIZE - 1, drawY + 1);
                    cairo_line_to(cr, drawX + BLOCK_SIZE - 1, drawY + BLOCK_SIZE - 1);
                    cairo_line_to(cr, drawX + 1, drawY + BLOCK_SIZE - 1);
                    cairo_close_path(cr);
                    cairo_fill(cr);
                    
                    // Reset color for next block
                    cairo_set_source_rgb(cr, color[0], color[1], color[2]);
                }
            }
        }
    }
    
    return FALSE;
}

gboolean onKeyPress(GtkWidget* widget, GdkEventKey* event, gpointer data) {
    TetrisApp* app = static_cast<TetrisApp*>(data);
    TetrisBoard* board = app->board;
    
    // Handle space to dismiss splash screen first
    if (event->keyval == GDK_KEY_space && board->isSplashScreenActive()) {
        board->dismissSplashScreen();
        gtk_widget_queue_draw(app->gameArea);
        gtk_widget_queue_draw(app->nextPieceArea);
        updateLabels(app);
        return TRUE;
    }
    
    // Handle game control keys only when game is active
    if (!board->isPaused() && !board->isGameOver() && !board->isSplashScreenActive()) {
        switch (event->keyval) {
            case GDK_KEY_Left:
            case GDK_KEY_a:
            case GDK_KEY_A:
                board->movePiece(-1, 0);
                break;
                
            case GDK_KEY_Right:
            case GDK_KEY_d:
            case GDK_KEY_D:
                board->movePiece(1, 0);
                break;
                
            case GDK_KEY_Down:
            case GDK_KEY_s:
            case GDK_KEY_S:
                board->movePiece(0, 1);
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
            
        default:
            // Don't return FALSE here as it prevents redrawing
            break;
    }
    
    // Always redraw and update after any key press
    gtk_widget_queue_draw(app->gameArea);
    gtk_widget_queue_draw(app->nextPieceArea);
    updateLabels(app);
    
    return TRUE;  // Always claim we handled the key event
}

gboolean onTimerTick(gpointer data) {
    TetrisApp* app = static_cast<TetrisApp*>(data);
    TetrisBoard* board = app->board;
    
    if (!board->isGameOver() && !board->isPaused()) {
        board->updateGame();
        gtk_widget_queue_draw(app->gameArea);
        gtk_widget_queue_draw(app->nextPieceArea);
        updateLabels(app);
    }
    
    return TRUE;  // Keep the timer running
}

void updateLabels(TetrisApp* app) {
    TetrisBoard* board = app->board;
    
    // Update score label
    std::string score_text = "<b>Score:</b> " + std::to_string(board->getScore());
    gtk_label_set_markup(GTK_LABEL(app->scoreLabel), score_text.c_str());
    
    // Update level label
    std::string level_text = "<b>Level:</b> " + std::to_string(board->getLevel());
    gtk_label_set_markup(GTK_LABEL(app->levelLabel), level_text.c_str());
    
    // Update lines label
    std::string lines_text = "<b>Lines:</b> " + std::to_string(board->getLinesCleared());
    gtk_label_set_markup(GTK_LABEL(app->linesLabel), lines_text.c_str());
}

void resetUI(TetrisApp* app) {
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
    TetrisApp* app = static_cast<TetrisApp*>(data);
    
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

void onScreenSizeChanged(GtkWidget* widget, GdkRectangle* allocation, gpointer userData) {
    TetrisApp* app = static_cast<TetrisApp*>(userData);
    
    // We intentionally avoid recalculating the block size here
    // so that manual resize doesn't affect the game area
    
    // Just redraw everything with the current block size
    gtk_widget_queue_draw(app->gameArea);
    gtk_widget_queue_draw(app->nextPieceArea);
}

void onAppActivate(GtkApplication* app, gpointer userData) {
    TetrisApp* tetrisApp = new TetrisApp();
    tetrisApp->app = app;
    tetrisApp->board = new TetrisBoard();
    tetrisApp->timerId = 0;
    tetrisApp->dropSpeed = INITIAL_SPEED;
    tetrisApp->difficulty = 2; // Default to Medium
    
    // Calculate block size based on screen resolution
    calculateBlockSize(tetrisApp);
    
    // Create the main window
    tetrisApp->window = gtk_application_window_new(app);
    gtk_window_set_title(GTK_WINDOW(tetrisApp->window), "Tetris");

g_signal_connect(G_OBJECT(tetrisApp->window), "delete-event",
               G_CALLBACK(onDeleteEvent), tetrisApp);
    
    // Use the calculated block size for window dimensions
    gtk_window_set_default_size(GTK_WINDOW(tetrisApp->window), 
                              GRID_WIDTH * BLOCK_SIZE + 200, 
                              GRID_HEIGHT * BLOCK_SIZE + 40);
    gtk_window_set_resizable(GTK_WINDOW(tetrisApp->window), TRUE);
    
    // Create main vertical box
    GtkWidget* mainVBox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
    gtk_container_add(GTK_CONTAINER(tetrisApp->window), mainVBox);
    
    // Create menu
    createMenu(tetrisApp);
    gtk_box_pack_start(GTK_BOX(mainVBox), tetrisApp->menuBar, FALSE, FALSE, 0);
    
    // Create main horizontal box for game contents
    tetrisApp->mainBox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 10);
    gtk_container_set_border_width(GTK_CONTAINER(tetrisApp->mainBox), 10);
    gtk_box_pack_start(GTK_BOX(mainVBox), tetrisApp->mainBox, TRUE, TRUE, 0);

    g_signal_connect(G_OBJECT(tetrisApp->window), "size-allocate",
               G_CALLBACK(onScreenSizeChanged), tetrisApp);
    
    // Create the game area (drawing area)
    tetrisApp->gameArea = gtk_drawing_area_new();
    gtk_widget_set_size_request(tetrisApp->gameArea, 
                              GRID_WIDTH * BLOCK_SIZE, 
                              GRID_HEIGHT * BLOCK_SIZE);
    g_signal_connect(G_OBJECT(tetrisApp->gameArea), "draw",
                   G_CALLBACK(onDrawGameArea), tetrisApp);
    gtk_box_pack_start(GTK_BOX(tetrisApp->mainBox), tetrisApp->gameArea, FALSE, FALSE, 0);
    
    // Create the side panel (vertical box)
    GtkWidget* sideBox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 10);
    gtk_box_pack_start(GTK_BOX(tetrisApp->mainBox), sideBox, FALSE, FALSE, 0);
    
    // Create the next piece preview frame
    GtkWidget* nextPieceFrame = gtk_frame_new("Next Piece");
    gtk_box_pack_start(GTK_BOX(sideBox), nextPieceFrame, FALSE, FALSE, 0);
    
    // Create the next piece drawing area
    tetrisApp->nextPieceArea = gtk_drawing_area_new();
    gtk_widget_set_size_request(tetrisApp->nextPieceArea, 4 * BLOCK_SIZE, 4 * BLOCK_SIZE);
    g_signal_connect(G_OBJECT(tetrisApp->nextPieceArea), "draw",
                   G_CALLBACK(onDrawNextPiece), tetrisApp);
    gtk_container_add(GTK_CONTAINER(nextPieceFrame), tetrisApp->nextPieceArea);
    
    // Create score, level, and lines labels
    tetrisApp->scoreLabel = gtk_label_new(NULL);
    gtk_label_set_markup(GTK_LABEL(tetrisApp->scoreLabel), "<b>Score:</b> 0");
    gtk_widget_set_halign(tetrisApp->scoreLabel, GTK_ALIGN_START);
    gtk_box_pack_start(GTK_BOX(sideBox), tetrisApp->scoreLabel, FALSE, FALSE, 0);
    
    tetrisApp->levelLabel = gtk_label_new(NULL);
    gtk_label_set_markup(GTK_LABEL(tetrisApp->levelLabel), "<b>Level:</b> 1");
    gtk_widget_set_halign(tetrisApp->levelLabel, GTK_ALIGN_START);
    gtk_box_pack_start(GTK_BOX(sideBox), tetrisApp->levelLabel, FALSE, FALSE, 0);
    
    tetrisApp->linesLabel = gtk_label_new(NULL);
    gtk_label_set_markup(GTK_LABEL(tetrisApp->linesLabel), "<b>Lines:</b> 0");
    gtk_widget_set_halign(tetrisApp->linesLabel, GTK_ALIGN_START);
    gtk_box_pack_start(GTK_BOX(sideBox), tetrisApp->linesLabel, FALSE, FALSE, 0);
    
    // Add difficulty label
    tetrisApp->difficultyLabel = gtk_label_new(NULL);
    gtk_label_set_markup(GTK_LABEL(tetrisApp->difficultyLabel), 
                       getDifficultyText(tetrisApp->difficulty).c_str());
    gtk_widget_set_halign(tetrisApp->difficultyLabel, GTK_ALIGN_START);
    gtk_box_pack_start(GTK_BOX(sideBox), tetrisApp->difficultyLabel, FALSE, FALSE, 0);
    
    // Add controls info
    GtkWidget* controlsLabel = gtk_label_new(NULL);
    gtk_label_set_markup(GTK_LABEL(controlsLabel), "<b>Controls</b>");
    gtk_widget_set_halign(controlsLabel, GTK_ALIGN_START);
    gtk_box_pack_start(GTK_BOX(sideBox), controlsLabel, FALSE, FALSE, 10);
    
    GtkWidget* controls = gtk_label_new(
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

    gtk_widget_set_halign(controls, GTK_ALIGN_START);
    gtk_box_pack_start(GTK_BOX(sideBox), controls, FALSE, FALSE, 0);
    
    // Set up key press events
    gtk_widget_add_events(tetrisApp->window, GDK_KEY_PRESS_MASK);
    g_signal_connect(G_OBJECT(tetrisApp->window), "key-press-event",
                   G_CALLBACK(onKeyPress), tetrisApp);
    
    // Connect cleanup function
    g_object_set_data_full(G_OBJECT(tetrisApp->window), "app-data", 
                          tetrisApp, cleanupApp);
    
    // Show all widgets
    gtk_widget_show_all(tetrisApp->window);

    // Initialize the menu state
    gtk_widget_set_sensitive(tetrisApp->startMenuItem, FALSE);
    gtk_widget_set_sensitive(tetrisApp->pauseMenuItem, TRUE);

    if (tetrisApp->board->initializeAudio()) {
        // Only play music if initialization was successful
        tetrisApp->board->playBackgroundMusic();
        tetrisApp->backgroundMusicPlaying = true;
    }
    else {
        printf("Music failed to initialize");
        // Disable sound menu item
        gtk_check_menu_item_set_active(
            GTK_CHECK_MENU_ITEM(tetrisApp->soundToggleMenuItem), FALSE);
    }

    tetrisApp->joystick = NULL;
    tetrisApp->joystickEnabled = false;
    tetrisApp->joystickTimerId = 0;
    
    // Try to initialize SDL
    initSDL(tetrisApp);
        
    // Start the game
    startGame(tetrisApp);
}

// Function to create the menu bar with a better organization
void createMenu(TetrisApp* app) {
    GtkWidget* menuBar = gtk_menu_bar_new();
    
    // *** GAME MENU ***
    GtkWidget* gameMenu = gtk_menu_new();
    GtkWidget* gameMenuItem = gtk_menu_item_new_with_label("Game");
    gtk_menu_item_set_submenu(GTK_MENU_ITEM(gameMenuItem), gameMenu);
    
    // Connect signals for menu activation/deactivation
    g_signal_connect(G_OBJECT(gameMenu), "show", 
                   G_CALLBACK(onMenuActivated), app);
    g_signal_connect(G_OBJECT(gameMenu), "hide", 
                   G_CALLBACK(onMenuDeactivated), app);
    
    // Game menu items
    app->startMenuItem = gtk_menu_item_new_with_label("Start");
    app->pauseMenuItem = gtk_menu_item_new_with_label("Pause");
    app->restartMenuItem = gtk_menu_item_new_with_label("Restart");
    GtkWidget* quitMenuItem = gtk_menu_item_new_with_label("Quit");
    
    gtk_menu_shell_append(GTK_MENU_SHELL(gameMenu), app->startMenuItem);
    gtk_menu_shell_append(GTK_MENU_SHELL(gameMenu), app->pauseMenuItem);
    gtk_menu_shell_append(GTK_MENU_SHELL(gameMenu), app->restartMenuItem);
    gtk_menu_shell_append(GTK_MENU_SHELL(gameMenu), gtk_separator_menu_item_new());
    gtk_menu_shell_append(GTK_MENU_SHELL(gameMenu), quitMenuItem);
    
    // *** DIFFICULTY MENU ***
    GtkWidget* difficultyMenu = gtk_menu_new();
    GtkWidget* difficultyMenuItem = gtk_menu_item_new_with_label("Difficulty");
    gtk_menu_item_set_submenu(GTK_MENU_ITEM(difficultyMenuItem), difficultyMenu);
    
    // Connect signals for difficulty submenu
    g_signal_connect(G_OBJECT(difficultyMenu), "show", 
                   G_CALLBACK(onMenuActivated), app);
    g_signal_connect(G_OBJECT(difficultyMenu), "hide", 
                   G_CALLBACK(onMenuDeactivated), app);
    
    // Create difficulty radio menu items
    GSList* difficultyGroup = NULL;
    app->zenMenuItem = gtk_radio_menu_item_new_with_label(difficultyGroup, "Zen");
    difficultyGroup = gtk_radio_menu_item_get_group(GTK_RADIO_MENU_ITEM(app->zenMenuItem));

    app->easyMenuItem = gtk_radio_menu_item_new_with_label(difficultyGroup, "Easy");
    difficultyGroup = gtk_radio_menu_item_get_group(GTK_RADIO_MENU_ITEM(app->easyMenuItem));    

    app->mediumMenuItem = gtk_radio_menu_item_new_with_label(difficultyGroup, "Medium");
    difficultyGroup = gtk_radio_menu_item_get_group(GTK_RADIO_MENU_ITEM(app->mediumMenuItem));
    
    app->hardMenuItem = gtk_radio_menu_item_new_with_label(difficultyGroup, "Hard");
    difficultyGroup = gtk_radio_menu_item_get_group(GTK_RADIO_MENU_ITEM(app->hardMenuItem));
    
    app->extremeMenuItem = gtk_radio_menu_item_new_with_label(difficultyGroup, "Extreme");
    difficultyGroup = gtk_radio_menu_item_get_group(GTK_RADIO_MENU_ITEM(app->extremeMenuItem));
    
    app->insaneMenuItem = gtk_radio_menu_item_new_with_label(difficultyGroup, "Insane");
    
    // Set medium as default
    gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(app->mediumMenuItem), TRUE);
    app->difficulty = 2; // Medium
    
    gtk_menu_shell_append(GTK_MENU_SHELL(difficultyMenu), app->zenMenuItem);
    gtk_menu_shell_append(GTK_MENU_SHELL(difficultyMenu), app->easyMenuItem);
    gtk_menu_shell_append(GTK_MENU_SHELL(difficultyMenu), app->mediumMenuItem);
    gtk_menu_shell_append(GTK_MENU_SHELL(difficultyMenu), app->hardMenuItem);
    gtk_menu_shell_append(GTK_MENU_SHELL(difficultyMenu), app->extremeMenuItem);
    gtk_menu_shell_append(GTK_MENU_SHELL(difficultyMenu), app->insaneMenuItem);

    // *** GRAPHICS MENU ***
    GtkWidget* graphicsMenu = gtk_menu_new();
    GtkWidget* graphicsMenuItem = gtk_menu_item_new_with_label("Graphics");
    gtk_menu_item_set_submenu(GTK_MENU_ITEM(graphicsMenuItem), graphicsMenu);
    
    // Connect signals for menu activation/deactivation
    g_signal_connect(G_OBJECT(graphicsMenu), "show", 
                   G_CALLBACK(onMenuActivated), app);
    g_signal_connect(G_OBJECT(graphicsMenu), "hide", 
                   G_CALLBACK(onMenuDeactivated), app);
    
    // Block size menu item
    GtkWidget* blockSizeMenuItem = gtk_menu_item_new_with_label("Block Size...");
    gtk_menu_shell_append(GTK_MENU_SHELL(graphicsMenu), blockSizeMenuItem);
    g_signal_connect(G_OBJECT(blockSizeMenuItem), "activate",
                   G_CALLBACK(onBlockSizeDialog), app);
    
    // Background submenu
    GtkWidget* backgroundMenu = gtk_menu_new();
    GtkWidget* backgroundMenuItem = gtk_menu_item_new_with_label("Background");
    gtk_menu_item_set_submenu(GTK_MENU_ITEM(backgroundMenuItem), backgroundMenu);
    gtk_menu_shell_append(GTK_MENU_SHELL(graphicsMenu), backgroundMenuItem);
    
    // Background menu items
    GtkWidget* setBackgroundMenuItem = gtk_menu_item_new_with_label("Set Background Image (PNG)...");
    gtk_menu_shell_append(GTK_MENU_SHELL(backgroundMenu), setBackgroundMenuItem);
    g_signal_connect(G_OBJECT(setBackgroundMenuItem), "activate",
                  G_CALLBACK(onBackgroundImageDialog), app);

    GtkWidget* backgroundZipMenuItem = gtk_menu_item_new_with_label("Set Background Images from ZIP...");
    gtk_menu_shell_append(GTK_MENU_SHELL(backgroundMenu), backgroundZipMenuItem);
    g_signal_connect(G_OBJECT(backgroundZipMenuItem), "activate",
                  G_CALLBACK(onBackgroundZipDialog), app);
    
    GtkWidget* backgroundOpacityMenuItem = gtk_menu_item_new_with_label("Background Opacity...");
    gtk_menu_shell_append(GTK_MENU_SHELL(backgroundMenu), backgroundOpacityMenuItem);
    g_signal_connect(G_OBJECT(backgroundOpacityMenuItem), "activate",
                  G_CALLBACK(onBackgroundOpacityDialog), app);
    
    // Add background toggle checkbox
    app->backgroundToggleMenuItem = gtk_check_menu_item_new_with_label("Enable Background Image");
    gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(app->backgroundToggleMenuItem), TRUE);
    gtk_menu_shell_append(GTK_MENU_SHELL(backgroundMenu), app->backgroundToggleMenuItem);
    g_signal_connect(G_OBJECT(app->backgroundToggleMenuItem), "toggled",
                  G_CALLBACK(onBackgroundToggled), app);

    // *** SOUND MENU ***
    GtkWidget* soundMenu = gtk_menu_new();
    GtkWidget* soundMenuItem = gtk_menu_item_new_with_label("Sound");
    gtk_menu_item_set_submenu(GTK_MENU_ITEM(soundMenuItem), soundMenu);
    
    // Connect signals for menu activation/deactivation
    g_signal_connect(G_OBJECT(soundMenu), "show", 
                   G_CALLBACK(onMenuActivated), app);
    g_signal_connect(G_OBJECT(soundMenu), "hide", 
                   G_CALLBACK(onMenuDeactivated), app);
    
    // Sound menu items
    app->soundToggleMenuItem = gtk_check_menu_item_new_with_label("Enable Sound");
    gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(app->soundToggleMenuItem), TRUE);
    gtk_menu_shell_append(GTK_MENU_SHELL(soundMenu), app->soundToggleMenuItem);
    g_signal_connect(G_OBJECT(app->soundToggleMenuItem), "toggled",
                   G_CALLBACK(onSoundToggled), app);
    
    GtkWidget* volumeMenuItem = gtk_menu_item_new_with_label("Volume Settings...");
    gtk_menu_shell_append(GTK_MENU_SHELL(soundMenu), volumeMenuItem);
    g_signal_connect(G_OBJECT(volumeMenuItem), "activate",
                   G_CALLBACK(onVolumeDialog), app);
    
    // Music submenu
    GtkWidget* musicMenu = gtk_menu_new();
    GtkWidget* musicMenuItem = gtk_menu_item_new_with_label("Music Tracks");
    gtk_menu_item_set_submenu(GTK_MENU_ITEM(musicMenuItem), musicMenu);
    gtk_menu_shell_append(GTK_MENU_SHELL(soundMenu), musicMenuItem);
    
    // Create checkbox for each track
    for (int i = 0; i < 5; i++) {
        char label[20];
        sprintf(label, "Track %d", i+1);
        app->trackMenuItems[i] = gtk_check_menu_item_new_with_label(label);
        
        // Set all checked by default
        gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(app->trackMenuItems[i]), TRUE);
        
        // Store track index in the widget data
        g_object_set_data(G_OBJECT(app->trackMenuItems[i]), "track-index", GINT_TO_POINTER(i));
        
        // Connect signal
        g_signal_connect(G_OBJECT(app->trackMenuItems[i]), "toggled", 
                       G_CALLBACK(onTrackToggled), app);
                       
        gtk_menu_shell_append(GTK_MENU_SHELL(musicMenu), app->trackMenuItems[i]);
    }

    // *** CONTROLS MENU ***
    GtkWidget* controlsMenu = gtk_menu_new();
    GtkWidget* controlsMenuItem = gtk_menu_item_new_with_label("Controls");
    gtk_menu_item_set_submenu(GTK_MENU_ITEM(controlsMenuItem), controlsMenu);
    
    // Connect signals for menu activation/deactivation
    g_signal_connect(G_OBJECT(controlsMenu), "show", 
                   G_CALLBACK(onMenuActivated), app);
    g_signal_connect(G_OBJECT(controlsMenu), "hide", 
                   G_CALLBACK(onMenuDeactivated), app);
    
    // Joystick config menu item
    GtkWidget* joystickConfigMenuItem = gtk_menu_item_new_with_label("Configure Joystick...");
    gtk_menu_shell_append(GTK_MENU_SHELL(controlsMenu), joystickConfigMenuItem);
    g_signal_connect(G_OBJECT(joystickConfigMenuItem), "activate",
                   G_CALLBACK(onJoystickConfig), app);

    // *** HELP MENU ***
    GtkWidget* helpMenu = gtk_menu_new();
    GtkWidget* helpMenuItem = gtk_menu_item_new_with_label("Help");
    gtk_menu_item_set_submenu(GTK_MENU_ITEM(helpMenuItem), helpMenu);
    
    // Connect signals for menu activation/deactivation
    g_signal_connect(G_OBJECT(helpMenu), "show", 
                   G_CALLBACK(onMenuActivated), app);
    g_signal_connect(G_OBJECT(helpMenu), "hide", 
                   G_CALLBACK(onMenuDeactivated), app);
    
    // Help menu items
    GtkWidget* instructionsMenuItem = gtk_menu_item_new_with_label("Instructions");
    GtkWidget* aboutMenuItem = gtk_menu_item_new_with_label("About");
    
    gtk_menu_shell_append(GTK_MENU_SHELL(helpMenu), instructionsMenuItem);
    gtk_menu_shell_append(GTK_MENU_SHELL(helpMenu), aboutMenuItem);
    
    // Add menus to menu bar
    gtk_menu_shell_append(GTK_MENU_SHELL(menuBar), gameMenuItem);
    gtk_menu_shell_append(GTK_MENU_SHELL(menuBar), difficultyMenuItem);
    gtk_menu_shell_append(GTK_MENU_SHELL(menuBar), graphicsMenuItem);
    gtk_menu_shell_append(GTK_MENU_SHELL(menuBar), soundMenuItem);
    gtk_menu_shell_append(GTK_MENU_SHELL(menuBar), controlsMenuItem);
    gtk_menu_shell_append(GTK_MENU_SHELL(menuBar), helpMenuItem);
    
    // Add menu signal handlers
    g_signal_connect(G_OBJECT(app->startMenuItem), "activate",
                   G_CALLBACK(onStartGame), app);
    g_signal_connect(G_OBJECT(app->pauseMenuItem), "activate",
                   G_CALLBACK(onPauseGame), app);
    g_signal_connect(G_OBJECT(app->restartMenuItem), "activate",
                   G_CALLBACK(onRestartGame), app);
    g_signal_connect(G_OBJECT(quitMenuItem), "activate",
                   G_CALLBACK(onQuitGame), app);
    
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

// Menu callback functions
void onStartGame(GtkMenuItem* menuItem, gpointer userData) {
    TetrisApp* app = static_cast<TetrisApp*>(userData);
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

void pauseGame(TetrisApp* app) {
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

void onRestartGame(GtkMenuItem* menuItem, gpointer userData) {
    TetrisApp* app = static_cast<TetrisApp*>(userData);
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

void onQuitGame(GtkMenuItem* menuItem, gpointer userData) {
    TetrisApp* app = static_cast<TetrisApp*>(userData);
    gtk_window_close(GTK_WINDOW(app->window));
}

gboolean onDeleteEvent(GtkWidget* widget, GdkEvent* event, gpointer userData) {
#ifdef DEBUG
    std::cerr << "DEBUG: onDeleteEvent called" << std::endl;
#endif    
    TetrisApp* app = static_cast<TetrisApp*>(userData);
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
    std::cerr << "DEBUG: Returning FALSE to let GTK handle window destruction" << std::endl;
#endif
    // Now allow the window to close
    return FALSE;  // Let GTK handle the window destruction
}


void onSoundToggled(GtkCheckMenuItem* menuItem, gpointer userData) {
    TetrisApp* app = static_cast<TetrisApp*>(userData);
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

void onDifficultyChanged(GtkRadioMenuItem* menuItem, gpointer userData) {
    TetrisApp* app = static_cast<TetrisApp*>(userData);
    
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
            GtkWidget* dialog = gtk_message_dialog_new(
                GTK_WINDOW(app->window),
                GTK_DIALOG_MODAL,
                GTK_MESSAGE_QUESTION,
                GTK_BUTTONS_YES_NO,
                "Changing difficulty will start a new game. Continue?"
            );
            
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
                        gtk_check_menu_item_set_active(
                            GTK_CHECK_MENU_ITEM(app->zenMenuItem), TRUE);
                        break;
                    case 1:
                        gtk_check_menu_item_set_active(
                            GTK_CHECK_MENU_ITEM(app->easyMenuItem), TRUE);
                        break;
                    case 2:
                        gtk_check_menu_item_set_active(
                            GTK_CHECK_MENU_ITEM(app->mediumMenuItem), TRUE);
                        break;
                    case 3:
                        gtk_check_menu_item_set_active(
                            GTK_CHECK_MENU_ITEM(app->hardMenuItem), TRUE);
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
                           getDifficultyText(app->difficulty).c_str());
        
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
        // If difficulty didn't change, just restart timer with new speed if game is running
        if (!app->board->isPaused() && !app->board->isGameOver() && app->timerId > 0) {
            g_source_remove(app->timerId);
            app->timerId = g_timeout_add(app->dropSpeed, onTimerTick, app);
        }
    }
}

std::string getDifficultyText(int difficulty) {
    switch (difficulty) {
        case 0: return "<b>Difficulty:</b> Zen";
        case 1: return "<b>Difficulty:</b> Easy";
        case 2: return "<b>Difficulty:</b> Medium";
        case 3: return "<b>Difficulty:</b> Hard";
        case 4: return "<b>Difficulty:</b> Extreme";
        case 5: return "<b>Difficulty:</b> Insane";
        default: return "<b>Difficulty:</b> Medium";
    }
}

void adjustDropSpeed(TetrisApp* app) {
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
    if(app->dropSpeed < 10) {
        app->dropSpeed = 10;
    }
}

void onAboutDialog(GtkMenuItem* menuItem, gpointer userData) {
    TetrisApp* app = static_cast<TetrisApp*>(userData);
    
    // Create a custom dialog
    GtkWidget* dialog = gtk_dialog_new_with_buttons(
        "About GTK Tetris",
        GTK_WINDOW(app->window),
        GTK_DIALOG_MODAL,
        "_OK", GTK_RESPONSE_OK,
        NULL
    );
    
    // Get the content area
    GtkWidget* contentArea = gtk_dialog_get_content_area(GTK_DIALOG(dialog));
    gtk_container_set_border_width(GTK_CONTAINER(contentArea), 15);
    
    // Create a vertical box for layout
    GtkWidget* vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 10);
    gtk_container_add(GTK_CONTAINER(contentArea), vbox);
    
    // Add an image (optional)
    GtkWidget* image = gtk_image_new_from_icon_name("applications-games", GTK_ICON_SIZE_DIALOG);
    gtk_box_pack_start(GTK_BOX(vbox), image, FALSE, FALSE, 0);
    
    // Add program name
    GtkWidget* nameLabel = gtk_label_new(NULL);
    gtk_label_set_markup(GTK_LABEL(nameLabel), "<span size='x-large' weight='bold'>GTK Tetris</span>");
    gtk_box_pack_start(GTK_BOX(vbox), nameLabel, FALSE, FALSE, 5);
    
    // Add version
    GtkWidget* versionLabel = gtk_label_new("Version 1.0");
    gtk_box_pack_start(GTK_BOX(vbox), versionLabel, FALSE, FALSE, 0);
    
    // Add description
    GtkWidget* descLabel = gtk_label_new(
        "A feature-rich Tetris implementation with advanced graphics,\n"
        "multiple difficulty levels, theme progression, and comprehensive\n"
        "control options including joystick support."
    );
    gtk_box_pack_start(GTK_BOX(vbox), descLabel, FALSE, FALSE, 10);
    
    // Add license info
    GtkWidget* licenseLabel = gtk_label_new("This software is released under the MIT License.");
    gtk_box_pack_start(GTK_BOX(vbox), licenseLabel, FALSE, FALSE, 5);
    
    // Add acknowledgment heading
    GtkWidget* ackHeadingLabel = gtk_label_new(NULL);
    gtk_label_set_markup(GTK_LABEL(ackHeadingLabel), "<span weight='bold'>Acknowledgments:</span>");
    gtk_widget_set_halign(ackHeadingLabel, GTK_ALIGN_START);
    gtk_box_pack_start(GTK_BOX(vbox), ackHeadingLabel, FALSE, FALSE, 5);
    
    // Add acknowledgment text
    GtkWidget* ackLabel = gtk_label_new(
        "This game is based on Tetris®, created by Alexey Pajitnov in 1984.\n"
        "Tetris® is a registered trademark of The Tetris Company, LLC.\n"
        "Original game published by ELORG."
    );
    gtk_box_pack_start(GTK_BOX(vbox), ackLabel, FALSE, FALSE, 0);
    
    // Add website button
    GtkWidget* websiteButton = gtk_link_button_new_with_label(
        "https://github.com/jasonbrianhall/tetris", 
        "Website"
    );
    gtk_box_pack_start(GTK_BOX(vbox), websiteButton, FALSE, FALSE, 10);
    
    // Add copyright
    GtkWidget* copyrightLabel = gtk_label_new("© 2025 Jason Brian Hall");
    gtk_box_pack_start(GTK_BOX(vbox), copyrightLabel, FALSE, FALSE, 5);
    
    // Add no warranty disclaimer
    GtkWidget* disclaimerLabel = gtk_label_new("This program comes with absolutely no warranty.");
    gtk_box_pack_start(GTK_BOX(vbox), disclaimerLabel, FALSE, FALSE, 0);
    
    GtkWidget* licenseInfoLabel = gtk_label_new(NULL);
    gtk_label_set_markup(GTK_LABEL(licenseInfoLabel), "See the <a href='https://opensource.org/licenses/MIT'>MIT License</a> for details.");
    gtk_box_pack_start(GTK_BOX(vbox), licenseInfoLabel, FALSE, FALSE, 0);
    
    // Show all content
    gtk_widget_show_all(dialog);
    
    // Run the dialog
    gtk_dialog_run(GTK_DIALOG(dialog));
    
    // Destroy the dialog when closed
    gtk_widget_destroy(dialog);
}

void onInstructionsDialog(GtkMenuItem* menuItem, gpointer userData) {
    TetrisApp* app = static_cast<TetrisApp*>(userData);
    
    // Create dialog
    GtkWidget* dialog = gtk_dialog_new_with_buttons(
        "Instructions",
        GTK_WINDOW(app->window),
        GTK_DIALOG_MODAL,
        "_OK", GTK_RESPONSE_OK,
        NULL
    );
    
    // Create content area
    GtkWidget* contentArea = gtk_dialog_get_content_area(GTK_DIALOG(dialog));
    
    // Create label with instructions
    GtkWidget* label = gtk_label_new(
        "Tetris Instructions:\n\n"
        "Goal: Arrange falling blocks to complete lines.\n\n"
        "Controls:\n"
        "• Left/Right Arrow or A/D: Move block left/right\n"
        "• Up Arrow or W: Rotate block clockwise\n"
        "• Z: Rotate block counter-clockwise\n"
        "• Down Arrow or S: Move block down (soft drop)\n"
        "• Space: Hard drop (instantly places block at bottom)\n"
        "• P: Pause/Resume game\n"
        "• R: Restart game when game over\n"
        "• N: New game (when paused)\n"
        "• Q: Quit game (when paused)\n\n"
        "Controller Support:\n"
        "• D-pad/Analog: Move piece\n"
        "• A/B buttons: Rotate piece\n"
        "• X button: Hard drop\n"
        "• Start: Pause/Resume\n"
        "• Custom mapping available in Options menu\n\n"
        "Scoring:\n"
        "• 1 line: 40 × level\n"
        "• 2 lines: 100 × level\n"
        "• 3 lines: 300 × level\n"
        "• 4 lines: 1200 × level\n"
        "• Hard drops: 2 points per cell\n\n"
        "Levels:\n"
        "• Every 10 lines cleared increases the level\n"
        "• Higher levels increase speed and points\n"
        "• Color themes change with level progression\n"
        "• Difficulty can be adjusted in Options menu\n\n"
        "Tips:\n"
        "• Keep the stack low and even\n"
        "• Save I-pieces for Tetris clears (4 lines)\n"
        "• Watch the preview for the next piece\n"
        "• Red line indicates the game over zone"
    );    
    gtk_label_set_line_wrap(GTK_LABEL(label), TRUE);
    gtk_widget_set_margin_start(label, 20);
    gtk_widget_set_margin_end(label, 20);
    gtk_widget_set_margin_top(label, 20);
    gtk_widget_set_margin_bottom(label, 20);
    
    gtk_container_add(GTK_CONTAINER(contentArea), label);
    gtk_widget_show_all(dialog);
    
    // Set minimum width for dialog
    gtk_widget_set_size_request(dialog, 400, -1);
    
    // Run dialog
    gtk_dialog_run(GTK_DIALOG(dialog));
    
    // Destroy dialog when closed
    gtk_widget_destroy(dialog);
}

// Add to startGame function
void startGame(TetrisApp* app) {
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
    
    // Start a new timer
    app->timerId = g_timeout_add(app->dropSpeed, onTimerTick, app);
    
    // Update menu items
    gtk_widget_set_sensitive(app->startMenuItem, FALSE);
    gtk_widget_set_sensitive(app->pauseMenuItem, TRUE);
    gtk_menu_item_set_label(GTK_MENU_ITEM(app->pauseMenuItem), "Pause");
}

void onPauseGame(GtkMenuItem* menuItem, gpointer userData) {
    TetrisApp* app = static_cast<TetrisApp*>(userData);
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

void onMenuActivated(GtkWidget* widget, gpointer userData) {
    TetrisApp* app = static_cast<TetrisApp*>(userData);
    
    // Store current menu item label
    const char* currentLabel = gtk_menu_item_get_label(GTK_MENU_ITEM(app->pauseMenuItem));
    
    // Call onPauseGame to properly pause the game if it's not already paused
    if (!app->board->isPaused() && !app->board->isGameOver()) {
        onPauseGame(GTK_MENU_ITEM(app->pauseMenuItem), app);
        
        // Restore the menu label so the visual state remains consistent
        gtk_menu_item_set_label(GTK_MENU_ITEM(app->pauseMenuItem), currentLabel);
    }
}

void onMenuDeactivated(GtkWidget* widget, gpointer userData) {
    TetrisApp* app = static_cast<TetrisApp*>(userData);
    
    // Call onPauseGame to resume if we weren't manually paused before
    if (app->board->isPaused() && !app->board->isGameOver() && 
        strcmp(gtk_menu_item_get_label(GTK_MENU_ITEM(app->pauseMenuItem)), "Resume") != 0) {
        onPauseGame(GTK_MENU_ITEM(app->pauseMenuItem), app);
    }
}

bool TetrisBoard::isGameOver() const {
    // If this is the first time checking game over status since it became true,
    // play the game over sound
    static bool soundPlayed = false;
    if (gameOver && !soundPlayed) {
        // Cast away const to allow calling non-const member function
        TetrisBoard* nonConstThis = const_cast<TetrisBoard*>(this);
        nonConstThis->playSound(GameSoundEvent::Gameover);
        soundPlayed = true;
    }
    
    // If game is no longer over, reset the sound played flag
    if (!gameOver) {
        soundPlayed = false;
    }
    
    return gameOver;
}

void calculateBlockSize(TetrisApp* app) {
    // Get the screen dimensions
    GdkRectangle workarea = {0};
    GdkMonitor* monitor = gdk_display_get_primary_monitor(gdk_display_get_default());
    gdk_monitor_get_workarea(monitor, &workarea);
    
    // Calculate available height and width (accounting for menu and side panel)
    int availableHeight = workarea.height - 100;  // Allow for window decorations and menu
    int availableWidth = workarea.width - 300;    // Allow for side panel and margins
    
    // Calculate block size based on available space and grid dimensions
    int heightBasedSize = availableHeight / GRID_HEIGHT;
    int widthBasedSize = availableWidth / GRID_WIDTH;
    
    // Use the smaller of the two to ensure the game fits on screen
    BLOCK_SIZE = std::min(heightBasedSize, widthBasedSize);
    
    // Constrain to min/max values for usability
    BLOCK_SIZE = std::max(BLOCK_SIZE, MIN_BLOCK_SIZE);
    BLOCK_SIZE = std::min(BLOCK_SIZE, MAX_BLOCK_SIZE);
    
}

void onBlockSizeValueChanged(GtkRange* range, gpointer data) {
    // Extract the app pointer and label from the data
    BlockSizeCallbackData* cbData = static_cast<BlockSizeCallbackData*>(data);
    TetrisApp* app = cbData->app;
    GtkWidget* label = cbData->label;
    
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

void rebuildGameUI(TetrisApp* app) {
    // Remove and destroy existing game area and next piece area
    if (app->gameArea != NULL) {
        gtk_widget_destroy(app->gameArea);
    }
    
    if (app->nextPieceArea != NULL) {
        gtk_widget_destroy(app->nextPieceArea);
    }
    
    // Resize the window to match the new block size
    gtk_window_resize(GTK_WINDOW(app->window), 
                    GRID_WIDTH * BLOCK_SIZE + 200, 
                    GRID_HEIGHT * BLOCK_SIZE + 40);
    
    // Create new game area with correct size
    app->gameArea = gtk_drawing_area_new();
    gtk_widget_set_size_request(app->gameArea, 
                              GRID_WIDTH * BLOCK_SIZE, 
                              GRID_HEIGHT * BLOCK_SIZE);
    g_signal_connect(G_OBJECT(app->gameArea), "draw",
                   G_CALLBACK(onDrawGameArea), app);
    
    // Add game area back to its container
    // First, let's find and empty the mainBox (keep the side panel)
    GList* children = gtk_container_get_children(GTK_CONTAINER(app->mainBox));
    for (GList* child = children; child != NULL; child = child->next) {
        gtk_container_remove(GTK_CONTAINER(app->mainBox), GTK_WIDGET(child->data));
    }
    g_list_free(children);
    
    // Recreate the main box contents
    gtk_box_pack_start(GTK_BOX(app->mainBox), app->gameArea, FALSE, FALSE, 0);
    
    // Create the side panel (vertical box)
    GtkWidget* sideBox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 10);
    gtk_box_pack_start(GTK_BOX(app->mainBox), sideBox, FALSE, FALSE, 0);
    
    // Create the next piece preview frame
    GtkWidget* nextPieceFrame = gtk_frame_new("Next Piece");
    gtk_box_pack_start(GTK_BOX(sideBox), nextPieceFrame, FALSE, FALSE, 0);
    
    // Create the next piece drawing area
    app->nextPieceArea = gtk_drawing_area_new();
    gtk_widget_set_size_request(app->nextPieceArea, 4 * BLOCK_SIZE, 4 * BLOCK_SIZE);
    g_signal_connect(G_OBJECT(app->nextPieceArea), "draw",
                   G_CALLBACK(onDrawNextPiece), app);
    gtk_container_add(GTK_CONTAINER(nextPieceFrame), app->nextPieceArea);
    
    // Recreate score, level, and lines labels
    // (We need to recreate these because we destroyed their container)
    app->scoreLabel = gtk_label_new(NULL);
    std::string score_text = "<b>Score:</b> " + std::to_string(app->board->getScore());
    gtk_label_set_markup(GTK_LABEL(app->scoreLabel), score_text.c_str());
    gtk_widget_set_halign(app->scoreLabel, GTK_ALIGN_START);
    gtk_box_pack_start(GTK_BOX(sideBox), app->scoreLabel, FALSE, FALSE, 0);
    
    app->levelLabel = gtk_label_new(NULL);
    std::string level_text = "<b>Level:</b> " + std::to_string(app->board->getLevel());
    gtk_label_set_markup(GTK_LABEL(app->levelLabel), level_text.c_str());
    gtk_widget_set_halign(app->levelLabel, GTK_ALIGN_START);
    gtk_box_pack_start(GTK_BOX(sideBox), app->levelLabel, FALSE, FALSE, 0);
    
    app->linesLabel = gtk_label_new(NULL);
    std::string lines_text = "<b>Lines:</b> " + std::to_string(app->board->getLinesCleared());
    gtk_label_set_markup(GTK_LABEL(app->linesLabel), lines_text.c_str());
    gtk_widget_set_halign(app->linesLabel, GTK_ALIGN_START);
    gtk_box_pack_start(GTK_BOX(sideBox), app->linesLabel, FALSE, FALSE, 0);
    
    // Recreate difficulty label
    app->difficultyLabel = gtk_label_new(NULL);
    gtk_label_set_markup(GTK_LABEL(app->difficultyLabel), 
                       getDifficultyText(app->difficulty).c_str());
    gtk_widget_set_halign(app->difficultyLabel, GTK_ALIGN_START);
    gtk_box_pack_start(GTK_BOX(sideBox), app->difficultyLabel, FALSE, FALSE, 0);
    
    // Add controls info
    GtkWidget* controlsLabel = gtk_label_new(NULL);
    gtk_label_set_markup(GTK_LABEL(controlsLabel), "<b>Controls</b>");
    gtk_widget_set_halign(controlsLabel, GTK_ALIGN_START);
    gtk_box_pack_start(GTK_BOX(sideBox), controlsLabel, FALSE, FALSE, 10);
    
    GtkWidget* controls = gtk_label_new(
        "Left/Right/A/D: Move\n"
        "Up/W/Z: Rotate\n"
        "Down/S: Soft Drop\n"
        "Space: Hard Drop\n"
        "P: Pause\n"
        "R: Restart"
    );
    gtk_widget_set_halign(controls, GTK_ALIGN_START);
    gtk_box_pack_start(GTK_BOX(sideBox), controls, FALSE, FALSE, 0);
    
    // Show all the new widgets
    gtk_widget_show_all(app->mainBox);
}

void onResizeWindowButtonClicked(GtkWidget* button, gpointer data) {
    TetrisApp* app = static_cast<TetrisApp*>(data);
    
    // Rebuild UI with current block size
    rebuildGameUI(app);
}

bool TetrisBoard::loadBackgroundImage(const std::string& imagePath) {
    // Clean up previous image if it exists
    if (backgroundImage != nullptr) {
        cairo_surface_destroy(backgroundImage);
        backgroundImage = nullptr;
    }
    
    // Try to load the new image
    backgroundImage = cairo_image_surface_create_from_png(imagePath.c_str());
    
    // Check if image loaded successfully
    cairo_status_t status = cairo_surface_status(backgroundImage);
    if (status != CAIRO_STATUS_SUCCESS) {
        std::cerr << "Failed to load background image: " 
                  << cairo_status_to_string(status) << std::endl;
        cairo_surface_destroy(backgroundImage);
        backgroundImage = nullptr;
        return false;
    }
    
    // Store the path of successfully loaded image
    backgroundImagePath = imagePath;
    useBackgroundImage = true;
    return true;
}

void onBackgroundOpacityDialog(GtkMenuItem* menuItem, gpointer userData) {
    TetrisApp* app = static_cast<TetrisApp*>(userData);
    
    // Create dialog
    GtkWidget* dialog = gtk_dialog_new_with_buttons(
        "Background Opacity",
        GTK_WINDOW(app->window),
        GTK_DIALOG_MODAL,
        "_OK", GTK_RESPONSE_OK,
        NULL
    );
    
    // Make it a reasonable size
    gtk_window_set_default_size(GTK_WINDOW(dialog), 300, 150);
    
    // Create content area
    GtkWidget* contentArea = gtk_dialog_get_content_area(GTK_DIALOG(dialog));
    gtk_container_set_border_width(GTK_CONTAINER(contentArea), 10);
    
    // Create a vertical box for content
    GtkWidget* vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 6);
    gtk_container_add(GTK_CONTAINER(contentArea), vbox);
    
    // Add a label
    GtkWidget* label = gtk_label_new("Adjust background opacity:");
    gtk_widget_set_halign(label, GTK_ALIGN_START);
    gtk_box_pack_start(GTK_BOX(vbox), label, FALSE, FALSE, 0);
    
    // Create a horizontal scale (slider)
    GtkWidget* scale = gtk_scale_new_with_range(GTK_ORIENTATION_HORIZONTAL, 
                                             0.0, 1.0, 0.05);
    gtk_range_set_value(GTK_RANGE(scale), app->board->getBackgroundOpacity());
    gtk_scale_set_digits(GTK_SCALE(scale), 2); // 2 decimal places
    gtk_scale_set_value_pos(GTK_SCALE(scale), GTK_POS_RIGHT);
    gtk_box_pack_start(GTK_BOX(vbox), scale, FALSE, FALSE, 0);
    
    // Add min/max labels
    GtkWidget* rangeBox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
    gtk_box_pack_start(GTK_BOX(vbox), rangeBox, FALSE, FALSE, 0);
    
    GtkWidget* minLabel = gtk_label_new("Transparent");
    gtk_widget_set_halign(minLabel, GTK_ALIGN_START);
    gtk_box_pack_start(GTK_BOX(rangeBox), minLabel, TRUE, TRUE, 0);
    
    GtkWidget* maxLabel = gtk_label_new("Opaque");
    gtk_widget_set_halign(maxLabel, GTK_ALIGN_END);
    gtk_box_pack_end(GTK_BOX(rangeBox), maxLabel, TRUE, TRUE, 0);
    
    // Connect value-changed signal to update the opacity in real-time
    g_signal_connect(G_OBJECT(scale), "value-changed", 
                   G_CALLBACK(onOpacityValueChanged), app);
    
    // Show all dialog widgets
    gtk_widget_show_all(dialog);
    
    // Run the dialog
    gtk_dialog_run(GTK_DIALOG(dialog));
    
    // Destroy dialog
    gtk_widget_destroy(dialog);
}

void onOpacityValueChanged(GtkRange* range, gpointer userData) {
    TetrisApp* app = static_cast<TetrisApp*>(userData);
    
    // Update the opacity in the board
    double opacity = gtk_range_get_value(range);
    
    // Early return if the background image isn't valid
    if (!app->board->isUsingBackgroundImage() || app->board->getBackgroundImage() == nullptr) {
        return;
    }
    
    // Check surface status before attempting to draw
    cairo_status_t status = cairo_surface_status(app->board->getBackgroundImage());
    if (status != CAIRO_STATUS_SUCCESS) {
        std::cerr << "Invalid background image surface during opacity change: " 
                  << cairo_status_to_string(status) << std::endl;
        return;
    }
    
    app->board->setBackgroundOpacity(opacity);
    
    // Queue a redraw rather than forcing immediate redraw
    gtk_widget_queue_draw(app->gameArea);
}

void updateSizeValueLabel(GtkRange* range, gpointer data) {
    // Extract the label widget from the data
    GtkWidget* label = static_cast<GtkWidget*>(data);
    
    // Get the new block size from the slider
    int newSize = (int)gtk_range_get_value(range);
    
    // Update the label text
    char buf[32];
    snprintf(buf, sizeof(buf), "Current size: %d", newSize);
    gtk_label_set_text(GTK_LABEL(label), buf);
}

void onTrackToggled(GtkCheckMenuItem* menuItem, gpointer userData) {
    TetrisApp* app = static_cast<TetrisApp*>(userData);
    
    // Get the track index
    int trackIndex = GPOINTER_TO_INT(g_object_get_data(G_OBJECT(menuItem), "track-index"));
    
    // Update the enabled state in the board
    app->board->enabledTracks[trackIndex] = gtk_check_menu_item_get_active(menuItem);
    
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

int main(int argc, char* argv[]) {
    GtkApplication* app;
    int status;
#ifdef DEBUG 
    freopen("debug_output.log", "w", stdout);
    freopen("debug_output.log", "a", stderr);
#endif    
    app = gtk_application_new("org.gtk.tetris", G_APPLICATION_FLAGS_NONE);
    g_signal_connect(app, "activate", G_CALLBACK(onAppActivate), NULL);
    status = g_application_run(G_APPLICATION(app), argc, argv);
    g_object_unref(app);
    
    return status;
}
