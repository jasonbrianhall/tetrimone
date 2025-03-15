#include "tetris.h"
#include "audiomanager.h"
#include <iostream>
#include <string>


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

bool TetrisBoard::rotatePiece() {
    if (gameOver || paused) return false;
    
    currentPiece->rotate();
    
    if (checkCollision(*currentPiece)) {
        currentPiece->rotate(false);  // Rotate back if collision
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
            board->rotatePiece();
            break;
            
        case GDK_KEY_space:
            board->hardDrop();
            break;
            
        case GDK_KEY_p:
        case GDK_KEY_P:
            board->togglePause();
            if (board->isPaused()) {
                pauseGame(app);
            } else {
                startGame(app);
            }
            break;
            
        case GDK_KEY_r:
        case GDK_KEY_R:
            if (board->isGameOver()) {
                board->restart();
                resetUI(app);
                startGame(app);
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
}

void startGame(TetrisApp* app) {
    // Remove existing timer if any
    if (app->timerId > 0) {
        g_source_remove(app->timerId);
        app->timerId = 0;
    }
    
    // Resume background music if it was playing
    if (!app->backgroundMusicPlaying) {
        app->board->resumeBackgroundMusic();
        app->backgroundMusicPlaying = true;
    }
    
    // Calculate drop speed based on level (faster as level increases)
    app->dropSpeed = INITIAL_SPEED - (app->board->getLevel() - 1) * 50;
    if (app->dropSpeed < 100) {
        app->dropSpeed = 100;  // Set a minimum speed
    }
    
    // Start a new timer
    app->timerId = g_timeout_add(app->dropSpeed, onTimerTick, app);
}

void pauseGame(TetrisApp* app) {
    // Remove the timer
    if (app->timerId > 0) {
        g_source_remove(app->timerId);
        app->timerId = 0;
    }
    
    // Pause background music
    if (app->backgroundMusicPlaying) {
        app->board->pauseBackgroundMusic();
        app->backgroundMusicPlaying = false;
    }
}

void cleanupApp(gpointer data) {
    TetrisApp* app = static_cast<TetrisApp*>(data);
    
    // Stop timer if running
    if (app->timerId > 0) {
        g_source_remove(app->timerId);
        app->timerId = 0;
    }
    
    // Delete board
    delete app->board;
    
    // Delete app struct
    delete app;
}

void onAppActivate(GtkApplication* app, gpointer userData) {
    TetrisApp* tetrisApp = new TetrisApp();
    tetrisApp->app = app;
    tetrisApp->board = new TetrisBoard();
    tetrisApp->timerId = 0;
    tetrisApp->dropSpeed = INITIAL_SPEED;
    
    // Create the main window
    tetrisApp->window = gtk_application_window_new(app);
    gtk_window_set_title(GTK_WINDOW(tetrisApp->window), "Tetris");
    gtk_window_set_default_size(GTK_WINDOW(tetrisApp->window), 
                              GRID_WIDTH * BLOCK_SIZE + 200, 
                              GRID_HEIGHT * BLOCK_SIZE + 20);
    gtk_window_set_resizable(GTK_WINDOW(tetrisApp->window), FALSE);
    
    // Create main horizontal box
    tetrisApp->mainBox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 10);
    gtk_container_set_border_width(GTK_CONTAINER(tetrisApp->mainBox), 10);
    gtk_container_add(GTK_CONTAINER(tetrisApp->window), tetrisApp->mainBox);
    
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

    if (tetrisApp->board->initializeAudio()) {
        // Only play music if initialization was successful
        tetrisApp->board->playBackgroundMusic();
        tetrisApp->backgroundMusicPlaying = true;
    }
    else {
        printf("Music failed to initialize");
    }
        
    // Start the game
    startGame(tetrisApp);
}

void TetrisBoard::playBackgroundMusic() {
    if (sound_enabled_) {
        playSound(GameSoundEvent::BackgroundMusic);
    }
}

void TetrisBoard::pauseBackgroundMusic() {
    // This would require additional AudioManager functionality to pause a specific sound
    // For now, we can implement this by managing the background music state in the app
}

void TetrisBoard::resumeBackgroundMusic() {
    // Similar to pause, this would require AudioManager extensions
    // For now, we can just restart the music
    if (sound_enabled_) {
        playSound(GameSoundEvent::BackgroundMusic);
    }
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
