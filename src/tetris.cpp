#include "tetris.h"
#include "audiomanager.h"
#include <iostream>
#include <string>

int BLOCK_SIZE = 50;  // Default value, will be updated at runtime

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
    return TETROMINO_COLORS[type];
}

void Tetromino::setPosition(int newX, int newY) {
    x = newX;
    y = newY;
}

// TetrisBoard class implementation
TetrisBoard::TetrisBoard() : score(0), level(1), linesCleared(0), gameOver(false), paused(false) {
    // Initialize random number generator
    rng.seed(std::chrono::system_clock::now().time_since_epoch().count());
    
    // Initialize grid
    grid.resize(GRID_HEIGHT, std::vector<int>(GRID_WIDTH, 0));
    
    // Generate initial pieces
    generateNewPiece();
    generateNewPiece();
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
    if (gameOver || paused) return;
    
    // Try to move the piece down
    if (!movePiece(0, 1)) {
        // If can't move down, lock the piece
        lockPiece();
        
        // Clear any full lines
        clearLines();
        
        // Generate a new piece
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
    
    // Generate new pieces
    nextPiece.reset();
    currentPiece.reset();
    generateNewPiece();
    generateNewPiece();
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
    
    // Draw placed blocks
    for (int y = 0; y < GRID_HEIGHT; ++y) {
        for (int x = 0; x < GRID_WIDTH; ++x) {
            int value = board->getGridValue(x, y);
            if (value > 0) {
                // Get color from tetromino colors (value-1 because grid values are 1-based)
                auto color = TETROMINO_COLORS[value - 1];
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
    
    // Draw current piece
    if (!board->isGameOver() && !board->isPaused()) {
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
    
    // Draw paused text if needed
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
        double y = (GRID_HEIGHT * BLOCK_SIZE) / 2;
        
        cairo_move_to(cr, x, y);
        cairo_show_text(cr, text);
        
        // Show "Press P to continue" message
        cairo_set_font_size(cr, 16);
        const char* continueText = "Press P to continue";
        cairo_text_extents(cr, continueText, &extents);
        
        x = (GRID_WIDTH * BLOCK_SIZE - extents.width) / 2;
        y += 40;
        
        cairo_move_to(cr, x, y);
        cairo_show_text(cr, continueText);
    }
    
    return FALSE;
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
    
    switch (event->keyval) {
        case GDK_KEY_Left:
        case GDK_KEY_a:
            board->movePiece(-1, 0);
            break;
            
        case GDK_KEY_Right:
        case GDK_KEY_d:
            board->movePiece(1, 0);
            break;
            
        case GDK_KEY_Down:
        case GDK_KEY_s:
            board->movePiece(0, 1);
            break;
            
        case GDK_KEY_Up:
        case GDK_KEY_w:
            board->rotatePiece(true);
            break;

        case GDK_KEY_z:
            board->rotatePiece(false);
            break;

            
        case GDK_KEY_space:
            board->hardDrop();
            break;
            
        case GDK_KEY_p:
        case GDK_KEY_P:
            // Use menu action to ensure consistent behavior
            onPauseGame(GTK_MENU_ITEM(app->pauseMenuItem), app);
            break;
            
        case GDK_KEY_r:
        case GDK_KEY_R:
            if (board->isGameOver()) {
                onRestartGame(GTK_MENU_ITEM(app->restartMenuItem), app);
            }
            break;
            
        default:
            return FALSE;
    }
    
    // Redraw the game board
    gtk_widget_queue_draw(app->gameArea);
    gtk_widget_queue_draw(app->nextPieceArea);
    updateLabels(app);
    
    return TRUE;
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
    
    // Recalculate block size
    calculateBlockSize(app);
    
    // Resize game area
    gtk_widget_set_size_request(app->gameArea, 
                              GRID_WIDTH * BLOCK_SIZE, 
                              GRID_HEIGHT * BLOCK_SIZE);
    
    // Resize next piece area
    gtk_widget_set_size_request(app->nextPieceArea, 
                              4 * BLOCK_SIZE, 
                              4 * BLOCK_SIZE);
    
    // Redraw everything
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
        "Left/Right: Move\n"
        "Up: Rotate\n"
        "Down: Soft Drop\n"
        "Space: Hard Drop\n"
        "P: Pause\n"
        "R: Restart"
    );
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

// Function to create the menu bar with pause handling
void createMenu(TetrisApp* app) {
    GtkWidget* menuBar = gtk_menu_bar_new();
    
    // Game menu
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
    
    // Options menu
    GtkWidget* optionsMenu = gtk_menu_new();
    GtkWidget* optionsMenuItem = gtk_menu_item_new_with_label("Options");
    gtk_menu_item_set_submenu(GTK_MENU_ITEM(optionsMenuItem), optionsMenu);
    
    // Connect signals for menu activation/deactivation
    g_signal_connect(G_OBJECT(optionsMenu), "show", 
                   G_CALLBACK(onMenuActivated), app);
    g_signal_connect(G_OBJECT(optionsMenu), "hide", 
                   G_CALLBACK(onMenuDeactivated), app);
    
    // Options menu items
    app->soundToggleMenuItem = gtk_check_menu_item_new_with_label("Sound");
    gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(app->soundToggleMenuItem), TRUE);
    
    // Difficulty submenu
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
    app->easyMenuItem = gtk_radio_menu_item_new_with_label(difficultyGroup, "Easy");
    difficultyGroup = gtk_radio_menu_item_get_group(GTK_RADIO_MENU_ITEM(app->easyMenuItem));
    
    app->mediumMenuItem = gtk_radio_menu_item_new_with_label(difficultyGroup, "Medium");
    difficultyGroup = gtk_radio_menu_item_get_group(GTK_RADIO_MENU_ITEM(app->mediumMenuItem));
    
    app->hardMenuItem = gtk_radio_menu_item_new_with_label(difficultyGroup, "Hard");
    
    // Set medium as default
    gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(app->mediumMenuItem), TRUE);
    app->difficulty = 2; // Medium
    
    gtk_menu_shell_append(GTK_MENU_SHELL(difficultyMenu), app->easyMenuItem);
    gtk_menu_shell_append(GTK_MENU_SHELL(difficultyMenu), app->mediumMenuItem);
    gtk_menu_shell_append(GTK_MENU_SHELL(difficultyMenu), app->hardMenuItem);
    
    gtk_menu_shell_append(GTK_MENU_SHELL(optionsMenu), app->soundToggleMenuItem);
    gtk_menu_shell_append(GTK_MENU_SHELL(optionsMenu), difficultyMenuItem);
    
    // Help menu
    GtkWidget* helpMenu = gtk_menu_new();
    GtkWidget* helpMenuItem = gtk_menu_item_new_with_label("Help");
    gtk_menu_item_set_submenu(GTK_MENU_ITEM(helpMenuItem), helpMenu);
    
    // Connect signals for menu activation/deactivation
    g_signal_connect(G_OBJECT(helpMenu), "show", 
                   G_CALLBACK(onMenuActivated), app);
    g_signal_connect(G_OBJECT(helpMenu), "hide", 
                   G_CALLBACK(onMenuDeactivated), app);
    
    // Help menu items
    GtkWidget* aboutMenuItem = gtk_menu_item_new_with_label("About");
    GtkWidget* instructionsMenuItem = gtk_menu_item_new_with_label("Instructions");
    
    gtk_menu_shell_append(GTK_MENU_SHELL(helpMenu), instructionsMenuItem);
    gtk_menu_shell_append(GTK_MENU_SHELL(helpMenu), aboutMenuItem);
    
    // Add menus to menu bar
    gtk_menu_shell_append(GTK_MENU_SHELL(menuBar), gameMenuItem);
    gtk_menu_shell_append(GTK_MENU_SHELL(menuBar), optionsMenuItem);
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
    
    g_signal_connect(G_OBJECT(app->soundToggleMenuItem), "toggled",
                   G_CALLBACK(onSoundToggled), app);
    g_signal_connect(G_OBJECT(app->easyMenuItem), "toggled",
                   G_CALLBACK(onDifficultyChanged), app);
    g_signal_connect(G_OBJECT(app->mediumMenuItem), "toggled",
                   G_CALLBACK(onDifficultyChanged), app);
    g_signal_connect(G_OBJECT(app->hardMenuItem), "toggled",
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
    
    // Determine which difficulty was selected
    if (menuItem == GTK_RADIO_MENU_ITEM(app->easyMenuItem)) {
        app->difficulty = 1;
    } else if (menuItem == GTK_RADIO_MENU_ITEM(app->mediumMenuItem)) {
        app->difficulty = 2;
    } else if (menuItem == GTK_RADIO_MENU_ITEM(app->hardMenuItem)) {
        app->difficulty = 3;
    }
    
    // Update difficulty label
    gtk_label_set_markup(GTK_LABEL(app->difficultyLabel), 
                       getDifficultyText(app->difficulty).c_str());
    
    // Recalculate drop speed based on difficulty and level
    adjustDropSpeed(app);
    
    // Restart timer with new speed if game is running
    if (!app->board->isPaused() && !app->board->isGameOver() && app->timerId > 0) {
        g_source_remove(app->timerId);
        app->timerId = g_timeout_add(app->dropSpeed, onTimerTick, app);
    }
}

std::string getDifficultyText(int difficulty) {
    switch (difficulty) {
        case 1: return "<b>Difficulty:</b> Easy";
        case 2: return "<b>Difficulty:</b> Medium";
        case 3: return "<b>Difficulty:</b> Hard";
        default: return "<b>Difficulty:</b> Medium";
    }
}

void adjustDropSpeed(TetrisApp* app) {
    // Base speed based on level
    int baseSpeed = INITIAL_SPEED - (app->board->getLevel() - 1) * 50;
    
    // Apply difficulty modifier
    switch (app->difficulty) {
        case 1: // Easy
            app->dropSpeed = baseSpeed * 1.5;
            break;
        case 2: // Medium
            app->dropSpeed = baseSpeed;
            break;
        case 3: // Hard
            app->dropSpeed = baseSpeed * 0.7;
            break;
        default:
            app->dropSpeed = baseSpeed;
    }
    
    // Enforce minimum speed
    if (app->dropSpeed < 100) {
        app->dropSpeed = 100;
    }
}

void onAboutDialog(GtkMenuItem* menuItem, gpointer userData) {
    TetrisApp* app = static_cast<TetrisApp*>(userData);
    
    // Create and show about dialog
    GtkWidget* dialog = gtk_about_dialog_new();
    gtk_about_dialog_set_program_name(GTK_ABOUT_DIALOG(dialog), "GTK Tetris");
    gtk_about_dialog_set_version(GTK_ABOUT_DIALOG(dialog), "1.0");
    gtk_about_dialog_set_copyright(GTK_ABOUT_DIALOG(dialog), "© 2025 Jason Brian Hall");
    gtk_about_dialog_set_comments(GTK_ABOUT_DIALOG(dialog), 
                                "A simple Tetris clone written using GTK+");
    gtk_about_dialog_set_website(GTK_ABOUT_DIALOG(dialog), 
                               "https://github.com/jasonbrianhall/tetris");
    
    // Use app->window as the parent
    gtk_window_set_transient_for(GTK_WINDOW(dialog), GTK_WINDOW(app->window));
    
    // Show dialog and wait for response
    gtk_dialog_run(GTK_DIALOG(dialog));
    
    // Destroy dialog when closed
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
        "• Up Arrow or W: Rotate block\n"
        "• Down Arrow or S: Move block down (soft drop)\n"
        "• Space: Hard drop (instantly places block at bottom)\n"
        "• P: Pause/Resume game\n"
        "• R: Restart game when game over\n\n"
        "Scoring:\n"
        "• 1 line: 40 × level\n"
        "• 2 lines: 100 × level\n"
        "• 3 lines: 300 × level\n"
        "• 4 lines: 1200 × level\n\n"
        "Every 10 lines cleared increases the level and speed."
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


int main(int argc, char* argv[]) {
    GtkApplication* app;
    int status;
    
    app = gtk_application_new("org.gtk.tetris", G_APPLICATION_FLAGS_NONE);
    g_signal_connect(app, "activate", G_CALLBACK(onAppActivate), NULL);
    status = g_application_run(G_APPLICATION(app), argc, argv);
    g_object_unref(app);
    
    return status;
}
