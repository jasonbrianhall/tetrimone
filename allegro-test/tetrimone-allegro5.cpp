// Tetrimone using C++ and Allegro 5
// Compile with: g++ -o tetrimone tetrimone.cpp -lallegro -lallegro_font -lallegro_ttf -lallegro_primitives -lallegro_audio -lallegro_acodec -std=c++11

#include <allegro5/allegro.h>
#include <allegro5/allegro_font.h>
#include <allegro5/allegro_ttf.h>
#include <allegro5/allegro_primitives.h>
#include <allegro5/allegro_audio.h>
#include <allegro5/allegro_acodec.h>
#include <iostream>
#include <vector>
#include <random>
#include <ctime>

// Constants
const int SCREEN_WIDTH = 800;
const int SCREEN_HEIGHT = 600;
const int BLOCK_SIZE = 30;
const int GRID_WIDTH = 10;
const int GRID_HEIGHT = 20;
const int GRID_OFFSET_X = (SCREEN_WIDTH - GRID_WIDTH * BLOCK_SIZE) / 2;
const int GRID_OFFSET_Y = (SCREEN_HEIGHT - GRID_HEIGHT * BLOCK_SIZE) / 2;
const float FPS = 60.0;

// Tetromino shapes - Each piece has 4 rotations (even if some are duplicates)
// Fixed structure: pieces[type][rotation][row][column]
const std::vector<std::vector<std::vector<std::vector<int>>>> TETROMINOS = {
    // I piece - 4 rotations
    {
        // Rotation 0
        {
            {0, 0, 0, 0},
            {1, 1, 1, 1},
            {0, 0, 0, 0},
            {0, 0, 0, 0}
        },
        // Rotation 1
        {
            {0, 0, 1, 0},
            {0, 0, 1, 0},
            {0, 0, 1, 0},
            {0, 0, 1, 0}
        },
        // Rotation 2
        {
            {0, 0, 0, 0},
            {0, 0, 0, 0},
            {1, 1, 1, 1},
            {0, 0, 0, 0}
        },
        // Rotation 3
        {
            {0, 1, 0, 0},
            {0, 1, 0, 0},
            {0, 1, 0, 0},
            {0, 1, 0, 0}
        }
    },
    // J piece - 4 rotations
    {
        // Rotation 0
        {
            {1, 0, 0},
            {1, 1, 1},
            {0, 0, 0}
        },
        // Rotation 1
        {
            {0, 1, 1},
            {0, 1, 0},
            {0, 1, 0}
        },
        // Rotation 2
        {
            {0, 0, 0},
            {1, 1, 1},
            {0, 0, 1}
        },
        // Rotation 3
        {
            {0, 1, 0},
            {0, 1, 0},
            {1, 1, 0}
        }
    },
    // L piece - 4 rotations
    {
        // Rotation 0
        {
            {0, 0, 1},
            {1, 1, 1},
            {0, 0, 0}
        },
        // Rotation 1
        {
            {0, 1, 0},
            {0, 1, 0},
            {0, 1, 1}
        },
        // Rotation 2
        {
            {0, 0, 0},
            {1, 1, 1},
            {1, 0, 0}
        },
        // Rotation 3
        {
            {1, 1, 0},
            {0, 1, 0},
            {0, 1, 0}
        }
    },
    // O piece - 4 identical rotations for consistency
    {
        // Rotation 0
        {
            {1, 1},
            {1, 1}
        },
        // Rotation 1
        {
            {1, 1},
            {1, 1}
        },
        // Rotation 2
        {
            {1, 1},
            {1, 1}
        },
        // Rotation 3
        {
            {1, 1},
            {1, 1}
        }
    },
    // S piece - 4 rotations
    {
        // Rotation 0
        {
            {0, 1, 1},
            {1, 1, 0},
            {0, 0, 0}
        },
        // Rotation 1
        {
            {0, 1, 0},
            {0, 1, 1},
            {0, 0, 1}
        },
        // Rotation 2
        {
            {0, 0, 0},
            {0, 1, 1},
            {1, 1, 0}
        },
        // Rotation 3
        {
            {1, 0, 0},
            {1, 1, 0},
            {0, 1, 0}
        }
    },
    // T piece - 4 rotations
    {
        // Rotation 0
        {
            {0, 1, 0},
            {1, 1, 1},
            {0, 0, 0}
        },
        // Rotation 1
        {
            {0, 1, 0},
            {0, 1, 1},
            {0, 1, 0}
        },
        // Rotation 2
        {
            {0, 0, 0},
            {1, 1, 1},
            {0, 1, 0}
        },
        // Rotation 3
        {
            {0, 1, 0},
            {1, 1, 0},
            {0, 1, 0}
        }
    },
    // Z piece - 4 rotations
    {
        // Rotation 0
        {
            {1, 1, 0},
            {0, 1, 1},
            {0, 0, 0}
        },
        // Rotation 1
        {
            {0, 0, 1},
            {0, 1, 1},
            {0, 1, 0}
        },
        // Rotation 2
        {
            {0, 0, 0},
            {1, 1, 0},
            {0, 1, 1}
        },
        // Rotation 3
        {
            {0, 1, 0},
            {1, 1, 0},
            {1, 0, 0}
        }
    }
};

// Colors for each tetromino
const ALLEGRO_COLOR TETROMINO_COLORS[] = {
    al_map_rgb(0, 240, 240),   // I - Cyan
    al_map_rgb(0, 0, 240),     // J - Blue
    al_map_rgb(240, 160, 0),   // L - Orange
    al_map_rgb(240, 240, 0),   // O - Yellow
    al_map_rgb(0, 240, 0),     // S - Green
    al_map_rgb(160, 0, 240),   // T - Purple
    al_map_rgb(240, 0, 0)      // Z - Red
};

// Helper functions to extract RGB components from an ALLEGRO_COLOR
float get_allegro_r(ALLEGRO_COLOR color) {
    float r, g, b, a;
    al_unmap_rgba_f(color, &r, &g, &b, &a);
    return r;
}

float get_allegro_g(ALLEGRO_COLOR color) {
    float r, g, b, a;
    al_unmap_rgba_f(color, &r, &g, &b, &a);
    return g;
}

float get_allegro_b(ALLEGRO_COLOR color) {
    float r, g, b, a;
    al_unmap_rgba_f(color, &r, &g, &b, &a);
    return b;
}

// Game states
enum GameState {
    PLAYING,
    GAME_OVER,
    PAUSED
};

class Tetrimone {
private:
    int grid[GRID_HEIGHT][GRID_WIDTH] = {0};
    int currentPiece;
    int nextPiece;
    int currentRotation;
    int currentX, currentY;
    int score;
    int level;
    int lines;
    float fallSpeed;
    float fallTimer;
    bool gameOver;
    bool paused;
    std::mt19937 rng;

    // Initialize game values
    void init() {
        // Clear grid
        for (int y = 0; y < GRID_HEIGHT; y++) {
            for (int x = 0; x < GRID_WIDTH; x++) {
                grid[y][x] = 0;
            }
        }

        // Initialize game variables
        score = 0;
        level = 1;
        lines = 0;
        fallSpeed = 1.0;
        fallTimer = 0;
        gameOver = false;
        paused = false;
        
        // Set up random number generator for piece selection
        rng.seed(static_cast<unsigned int>(std::time(nullptr)));
        
        // Create the first piece
        nextPiece = -1;
        spawnPiece();
    }

    // Generate a random piece
    int randomPiece() {
        std::uniform_int_distribution<int> dist(0, TETROMINOS.size() - 1);
        return dist(rng);
    }

    // Create a new piece at the top of the grid
    void spawnPiece() {
        if (nextPiece == -1) {
            currentPiece = randomPiece();
            nextPiece = randomPiece();
        } else {
            currentPiece = nextPiece;
            nextPiece = randomPiece();
        }
        
        currentRotation = 0;
        
        // Position the piece at the top-center of the grid
        currentX = GRID_WIDTH / 2 - TETROMINOS[currentPiece][0][0].size() / 2;
        currentY = 0;
        
        // Check if the new piece collides immediately (game over condition)
        if (checkCollision()) {
            gameOver = true;
        }
    }

    // Check if current piece collides with borders or placed blocks
    bool checkCollision() {
        const auto& piece = TETROMINOS[currentPiece][currentRotation];
        for (size_t y = 0; y < piece.size(); y++) {
            for (size_t x = 0; x < piece[y].size(); x++) {
                if (piece[y][x]) {
                    int gridX = currentX + x;
                    int gridY = currentY + y;
                    
                    // Check boundaries
                    if (gridX < 0 || gridX >= GRID_WIDTH || gridY >= GRID_HEIGHT) {
                        return true;
                    }
                    
                    // Check collision with placed blocks (but ignore if we're still at the top)
                    if (gridY >= 0 && grid[gridY][gridX]) {
                        return true;
                    }
                }
            }
        }
        return false;
    }

    // Rotate the current piece
    void rotatePiece() {
        int oldRotation = currentRotation;
        currentRotation = (currentRotation + 1) % TETROMINOS[currentPiece].size();
        
        // If the rotation causes a collision, revert back
        if (checkCollision()) {
            currentRotation = oldRotation;
        }
    }

    // Move the current piece left, right, or down
    bool movePiece(int dx, int dy) {
        currentX += dx;
        currentY += dy;
        
        // If the move causes a collision, revert back
        if (checkCollision()) {
            currentX -= dx;
            currentY -= dy;
            
            // If we tried to move down and couldn't, lock the piece in place
            if (dy > 0) {
                lockPiece();
            }
            return false;
        }
        return true;
    }

    // Lock the current piece into the grid
    void lockPiece() {
        const auto& piece = TETROMINOS[currentPiece][currentRotation];
        for (size_t y = 0; y < piece.size(); y++) {
            for (size_t x = 0; x < piece[y].size(); x++) {
                if (piece[y][x]) {
                    int gridY = currentY + y;
                    int gridX = currentX + x;
                    
                    // Make sure we're not above the grid
                    if (gridY >= 0) {
                        // Store the piece index + 1 in the grid (0 means empty)
                        grid[gridY][gridX] = currentPiece + 1;
                    }
                }
            }
        }
        
        // Check for completed lines
        checkLines();
        
        // Spawn a new piece
        spawnPiece();
    }

    // Check for and clear completed lines
    void checkLines() {
        int linesCleared = 0;
        
        for (int y = GRID_HEIGHT - 1; y >= 0; y--) {
            bool lineComplete = true;
            
            // Check if the line is complete
            for (int x = 0; x < GRID_WIDTH; x++) {
                if (grid[y][x] == 0) {
                    lineComplete = false;
                    break;
                }
            }
            
            if (lineComplete) {
                linesCleared++;
                
                // Move all lines above down by one
                for (int moveY = y; moveY > 0; moveY--) {
                    for (int x = 0; x < GRID_WIDTH; x++) {
                        grid[moveY][x] = grid[moveY - 1][x];
                    }
                }
                
                // Clear the top line
                for (int x = 0; x < GRID_WIDTH; x++) {
                    grid[0][x] = 0;
                }
                
                // Check this line again as it now contains the line from above
                y++;
            }
        }
        
        // Update score and level
        if (linesCleared > 0) {
            // Scoring: 100 * (1 for single, 3 for double, 5 for triple, 8 for tetrimone) * level
            int multiplier;
            switch (linesCleared) {
                case 1: multiplier = 1; break;
                case 2: multiplier = 3; break;
                case 3: multiplier = 5; break;
                case 4: multiplier = 8; break;
                default: multiplier = 0;
            }
            
            score += 100 * multiplier * level;
            lines += linesCleared;
            
            // Level up every 10 lines
            level = (lines / 10) + 1;
            
            // Increase fall speed with level
            fallSpeed = 1.0 / level;
        }
    }

    // Drop the piece all the way down
    void hardDrop() {
        while (movePiece(0, 1)) {
            // Keep moving down until we hit something
        }
    }

    // Draw a single block with border
    void drawBlock(int x, int y, ALLEGRO_COLOR color) {
        int screenX = GRID_OFFSET_X + x * BLOCK_SIZE;
        int screenY = GRID_OFFSET_Y + y * BLOCK_SIZE;
        
        // Draw filled block
        al_draw_filled_rectangle(screenX, screenY, 
                                 screenX + BLOCK_SIZE, screenY + BLOCK_SIZE, 
                                 color);
        
        // Draw highlight (top and left edges)
        al_draw_line(screenX, screenY, 
                     screenX + BLOCK_SIZE, screenY, 
                     al_map_rgba(255, 255, 255, 128), 2);
        al_draw_line(screenX, screenY, 
                     screenX, screenY + BLOCK_SIZE, 
                     al_map_rgba(255, 255, 255, 128), 2);
        
        // Draw shadow (bottom and right edges)
        al_draw_line(screenX, screenY + BLOCK_SIZE, 
                     screenX + BLOCK_SIZE, screenY + BLOCK_SIZE, 
                     al_map_rgba(0, 0, 0, 128), 2);
        al_draw_line(screenX + BLOCK_SIZE, screenY, 
                     screenX + BLOCK_SIZE, screenY + BLOCK_SIZE, 
                     al_map_rgba(0, 0, 0, 128), 2);
    }

    // Draw the game grid
    void drawGrid() {
        // Draw border
        al_draw_rectangle(GRID_OFFSET_X - 1, GRID_OFFSET_Y - 1,
                         GRID_OFFSET_X + GRID_WIDTH * BLOCK_SIZE + 1,
                         GRID_OFFSET_Y + GRID_HEIGHT * BLOCK_SIZE + 1,
                         al_map_rgb(255, 255, 255), 2);
        
        // Draw grid lines
        for (int x = 0; x <= GRID_WIDTH; x++) {
            al_draw_line(GRID_OFFSET_X + x * BLOCK_SIZE, GRID_OFFSET_Y,
                        GRID_OFFSET_X + x * BLOCK_SIZE, GRID_OFFSET_Y + GRID_HEIGHT * BLOCK_SIZE,
                        al_map_rgba(80, 80, 80, 80), 1);
        }
        
        for (int y = 0; y <= GRID_HEIGHT; y++) {
            al_draw_line(GRID_OFFSET_X, GRID_OFFSET_Y + y * BLOCK_SIZE,
                        GRID_OFFSET_X + GRID_WIDTH * BLOCK_SIZE, GRID_OFFSET_Y + y * BLOCK_SIZE,
                        al_map_rgba(80, 80, 80, 80), 1);
        }
        
        // Draw placed blocks
        for (int y = 0; y < GRID_HEIGHT; y++) {
            for (int x = 0; x < GRID_WIDTH; x++) {
                if (grid[y][x]) {
                    drawBlock(x, y, TETROMINO_COLORS[grid[y][x] - 1]);
                }
            }
        }
    }

    // Draw the current falling piece
    void drawCurrentPiece() {
        const auto& piece = TETROMINOS[currentPiece][currentRotation];
        
        for (size_t y = 0; y < piece.size(); y++) {
            for (size_t x = 0; x < piece[y].size(); x++) {
                if (piece[y][x]) {
                    int gridX = currentX + x;
                    int gridY = currentY + y;
                    
                    // Only draw if the block is within the visible grid
                    if (gridY >= 0) {
                        drawBlock(gridX, gridY, TETROMINO_COLORS[currentPiece]);
                    }
                }
            }
        }
    }

    // Draw the ghost piece (preview of where the piece will land)
    void drawGhostPiece() {
        int ghostY = currentY;
        
        // Find where the piece would land
        while (true) {
            ghostY++;
            
            // Check for collision
            const auto& piece = TETROMINOS[currentPiece][currentRotation];
            bool collision = false;
            
            for (size_t y = 0; y < piece.size() && !collision; y++) {
                for (size_t x = 0; x < piece[y].size() && !collision; x++) {
                    if (piece[y][x]) {
                        int gridX = currentX + x;
                        int gridY = ghostY + y;
                        
                        // Check boundaries
                        if (gridX < 0 || gridX >= GRID_WIDTH || gridY >= GRID_HEIGHT) {
                            collision = true;
                            break;
                        }
                        
                        // Check collision with placed blocks
                        if (gridY >= 0 && grid[gridY][gridX]) {
                            collision = true;
                            break;
                        }
                    }
                }
            }
            
            if (collision) {
                ghostY--;
                break;
            }
        }
        
        // Draw the ghost piece
        if (ghostY > currentY) {
            const auto& piece = TETROMINOS[currentPiece][currentRotation];
            ALLEGRO_COLOR ghostColor = al_map_rgba(
                get_allegro_r(TETROMINO_COLORS[currentPiece]) * 255,
                get_allegro_g(TETROMINO_COLORS[currentPiece]) * 255,
                get_allegro_b(TETROMINO_COLORS[currentPiece]) * 255,
                50  // Low alpha for transparency
            );
            
            for (size_t y = 0; y < piece.size(); y++) {
                for (size_t x = 0; x < piece[y].size(); x++) {
                    if (piece[y][x]) {
                        int gridX = currentX + x;
                        int gridY = ghostY + y;
                        
                        // Only draw if the block is within the visible grid
                        if (gridY >= 0) {
                            int screenX = GRID_OFFSET_X + gridX * BLOCK_SIZE;
                            int screenY = GRID_OFFSET_Y + gridY * BLOCK_SIZE;
                            
                            al_draw_rectangle(screenX, screenY,
                                            screenX + BLOCK_SIZE, screenY + BLOCK_SIZE,
                                            ghostColor, 2);
                        }
                    }
                }
            }
        }
    }

    // Draw the next piece preview
    void drawNextPiece(ALLEGRO_FONT* font) {
        int previewX = GRID_OFFSET_X + GRID_WIDTH * BLOCK_SIZE + 30;
        int previewY = GRID_OFFSET_Y + 30;
        
        // Draw title
        al_draw_text(font, al_map_rgb(255, 255, 255), previewX, previewY, 0, "NEXT");
        previewY += 30;
        
        // Calculate preview size based on the piece
        const auto& piece = TETROMINOS[nextPiece][0]; // Use first rotation for preview
        int pieceHeight = piece.size() * BLOCK_SIZE;
        int pieceWidth = piece[0].size() * BLOCK_SIZE;
        
        // Draw background
        al_draw_filled_rectangle(previewX, previewY,
                               previewX + pieceWidth + 20, previewY + pieceHeight + 20,
                               al_map_rgb(30, 30, 30));
        
        // Draw next piece
        for (size_t y = 0; y < piece.size(); y++) {
            for (size_t x = 0; x < piece[y].size(); x++) {
                if (piece[y][x]) {
                    int blockX = previewX + 10 + x * BLOCK_SIZE;
                    int blockY = previewY + 10 + y * BLOCK_SIZE;
                    
                    // Draw filled block
                    al_draw_filled_rectangle(blockX, blockY,
                                           blockX + BLOCK_SIZE, blockY + BLOCK_SIZE,
                                           TETROMINO_COLORS[nextPiece]);
                    
                    // Draw highlight (top and left edges)
                    al_draw_line(blockX, blockY,
                                blockX + BLOCK_SIZE, blockY,
                                al_map_rgba(255, 255, 255, 128), 2);
                    al_draw_line(blockX, blockY,
                                blockX, blockY + BLOCK_SIZE,
                                al_map_rgba(255, 255, 255, 128), 2);
                    
                    // Draw shadow (bottom and right edges)
                    al_draw_line(blockX, blockY + BLOCK_SIZE,
                                blockX + BLOCK_SIZE, blockY + BLOCK_SIZE,
                                al_map_rgba(0, 0, 0, 128), 2);
                    al_draw_line(blockX + BLOCK_SIZE, blockY,
                                blockX + BLOCK_SIZE, blockY + BLOCK_SIZE,
                                al_map_rgba(0, 0, 0, 128), 2);
                }
            }
        }
    }

    // Draw score, level, and lines
    void drawStats(ALLEGRO_FONT* font) {
        int statsX = GRID_OFFSET_X + GRID_WIDTH * BLOCK_SIZE + 30;
        int statsY = GRID_OFFSET_Y + 150;
        
        al_draw_textf(font, al_map_rgb(255, 255, 255), statsX, statsY, 0, "SCORE: %d", score);
        statsY += 30;
        
        al_draw_textf(font, al_map_rgb(255, 255, 255), statsX, statsY, 0, "LEVEL: %d", level);
        statsY += 30;
        
        al_draw_textf(font, al_map_rgb(255, 255, 255), statsX, statsY, 0, "LINES: %d", lines);
    }

    // Draw game over screen
    void drawGameOver(ALLEGRO_FONT* font) {
        al_draw_filled_rectangle(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, al_map_rgba(0, 0, 0, 150));
        
        al_draw_text(font, al_map_rgb(255, 255, 255), SCREEN_WIDTH / 2, SCREEN_HEIGHT / 2 - 30,
                    ALLEGRO_ALIGN_CENTER, "GAME OVER");
        
        al_draw_textf(font, al_map_rgb(255, 255, 255), SCREEN_WIDTH / 2, SCREEN_HEIGHT / 2,
                     ALLEGRO_ALIGN_CENTER, "FINAL SCORE: %d", score);
        
        al_draw_text(font, al_map_rgb(255, 255, 255), SCREEN_WIDTH / 2, SCREEN_HEIGHT / 2 + 30,
                    ALLEGRO_ALIGN_CENTER, "PRESS ENTER TO RESTART");
    }

    // Draw pause screen
    void drawPaused(ALLEGRO_FONT* font) {
        al_draw_filled_rectangle(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, al_map_rgba(0, 0, 0, 150));
        al_draw_text(font, al_map_rgb(255, 255, 255), SCREEN_WIDTH / 2, SCREEN_HEIGHT / 2,
                    ALLEGRO_ALIGN_CENTER, "PAUSED");
    }

public:
    Tetrimone() {
        nextPiece = -1;
        init();
    }

    // Main game update function
    void update(float dt) {
        if (gameOver || paused) {
            return;
        }
        
        // Update fall timer
        fallTimer += dt;
        if (fallTimer >= fallSpeed) {
            fallTimer = 0;
            movePiece(0, 1);
        }
    }

    // Main game draw function
    void draw(ALLEGRO_FONT* font) {
        drawGrid();
        drawGhostPiece();
        drawCurrentPiece();
        drawNextPiece(font);
        drawStats(font);
        
        if (gameOver) {
            drawGameOver(font);
        } else if (paused) {
            drawPaused(font);
        }
    }

    // Handle keyboard input
    void handleInput(ALLEGRO_KEYBOARD_STATE& keyState) {
        static bool leftPressed = false;
        static bool rightPressed = false;
        static bool downPressed = false;
        static bool upPressed = false;
        static bool spacePressed = false;
        static bool pPressed = false;
        static bool enterPressed = false;
        
        // Check for key presses
        bool left = al_key_down(&keyState, ALLEGRO_KEY_LEFT);
        bool right = al_key_down(&keyState, ALLEGRO_KEY_RIGHT);
        bool down = al_key_down(&keyState, ALLEGRO_KEY_DOWN);
        bool up = al_key_down(&keyState, ALLEGRO_KEY_UP);
        bool space = al_key_down(&keyState, ALLEGRO_KEY_SPACE);
        bool p = al_key_down(&keyState, ALLEGRO_KEY_P);
        bool enter = al_key_down(&keyState, ALLEGRO_KEY_ENTER);
        
        // Handle game over
        if (gameOver) {
            if (enter && !enterPressed) {
                init();
            }
            enterPressed = enter;
            return;
        }
        
        // Toggle pause
        if (p && !pPressed) {
            paused = !paused;
        }
        pPressed = p;
        
        // If paused, don't process other inputs
        if (paused) {
            return;
        }
        
        // Move left
        if (left && !leftPressed) {
            movePiece(-1, 0);
        }
        leftPressed = left;
        
        // Move right
        if (right && !rightPressed) {
            movePiece(1, 0);
        }
        rightPressed = right;
        
        // Move down (soft drop)
        if (down && !downPressed) {
            movePiece(0, 1);
        }
        downPressed = down;
        
        // Rotate
        if (up && !upPressed) {
            rotatePiece();
        }
        upPressed = up;
        
        // Hard drop
        if (space && !spacePressed) {
            hardDrop();
        }
        spacePressed = space;
    }

    // Check if game is over
    bool isGameOver() const {
        return gameOver;
    }

    // Get current score
    int getScore() const {
        return score;
    }
};

// Main function - game entry point
int main() {
    // Initialize Allegro
    if (!al_init()) {
        std::cerr << "Failed to initialize Allegro!" << std::endl;
        return -1;
    }

    // Initialize keyboard
    if (!al_install_keyboard()) {
        std::cerr << "Failed to initialize keyboard!" << std::endl;
        return -1;
    }

    // Initialize font add-on
    if (!al_init_font_addon() || !al_init_ttf_addon()) {
        std::cerr << "Failed to initialize font add-on!" << std::endl;
        return -1;
    }

    // Initialize primitives add-on
    if (!al_init_primitives_addon()) {
        std::cerr << "Failed to initialize primitives add-on!" << std::endl;
        return -1;
    }

    // Create display
    ALLEGRO_DISPLAY* display = al_create_display(SCREEN_WIDTH, SCREEN_HEIGHT);
    if (!display) {
        std::cerr << "Failed to create display!" << std::endl;
        return -1;
    }
    al_set_window_title(display, "Tetrimone");

    // Create event queue
    ALLEGRO_EVENT_QUEUE* eventQueue = al_create_event_queue();
    if (!eventQueue) {
        std::cerr << "Failed to create event queue!" << std::endl;
        al_destroy_display(display);
        return -1;
    }

    // Create timer
    ALLEGRO_TIMER* timer = al_create_timer(1.0 / FPS);
    if (!timer) {
        std::cerr << "Failed to create timer!" << std::endl;
        al_destroy_event_queue(eventQueue);
        al_destroy_display(display);
        return -1;
    }

    // Load font
    ALLEGRO_FONT* font = al_load_font("arial.ttf", 24, 0);
    if (!font) {
        // Try to load a fallback font
        font = al_create_builtin_font();
        if (!font) {
            std::cerr << "Failed to load font!" << std::endl;
            al_destroy_timer(timer);
            al_destroy_event_queue(eventQueue);
            al_destroy_display(display);
            return -1;
        }
    }

    // Register event sources
    al_register_event_source(eventQueue, al_get_display_event_source(display));
    al_register_event_source(eventQueue, al_get_timer_event_source(timer));
    al_register_event_source(eventQueue, al_get_keyboard_event_source());

    // Start the timer
    al_start_timer(timer);

    // Create game instance
    Tetrimone tetrimone;

    // Game loop variables
    bool running = true;
    bool redraw = true;
    ALLEGRO_EVENT event;
    ALLEGRO_KEYBOARD_STATE keyState;

    // Game loop
    while (running) {
        al_wait_for_event(eventQueue, &event);

        switch (event.type) {
            case ALLEGRO_EVENT_TIMER:
                // Update game
                tetrimone.update(1.0 / FPS);
                
                // Get keyboard state
                al_get_keyboard_state(&keyState);
                tetrimone.handleInput(keyState);
                
                redraw = true;
                break;

            case ALLEGRO_EVENT_DISPLAY_CLOSE:
                running = false;
                break;

            case ALLEGRO_EVENT_KEY_DOWN:
                if (event.keyboard.keycode == ALLEGRO_KEY_ESCAPE) {
                    running = false;
                }
                break;
        }

        if (redraw && al_is_event_queue_empty(eventQueue)) {
            redraw = false;
            
            // Clear screen
            al_clear_to_color(al_map_rgb(0, 0, 0));
            
            // Draw game
            tetrimone.draw(font);
            
            // Flip display
            al_flip_display();
        }
    }

    // Clean up
    al_destroy_font(font);
    al_destroy_timer(timer);
    al_destroy_event_queue(eventQueue);
    al_destroy_display(display);

    return 0;
}
