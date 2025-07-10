// Tetrimone using C++ and Allegro 4 with Color Themes, Extended Pieces, and ZIP Audio
// Compile with: g++ -o tetrimone.exe tetrimone.cpp zip_audio_loader.cpp -lalleg

#include <iostream>
#include <vector>
#include <cstdlib>
#include <ctime>
#include <array>
#include <algorithm>
#include <allegro.h>

// Include block shapes, themes, and ZIP audio
#include "tetrimoneblock.h"
#include "themes.h"
#include "zip_audio_loader.h"

// Constants
const int SCREEN_WIDTH = 800;
const int SCREEN_HEIGHT = 600;
const int BLOCK_SIZE = 30;
const int GRID_WIDTH = 10;
const int GRID_HEIGHT = 20;
const int GRID_OFFSET_X = (SCREEN_WIDTH - GRID_WIDTH * BLOCK_SIZE) / 2;
const int GRID_OFFSET_Y = (SCREEN_HEIGHT - GRID_HEIGHT * BLOCK_SIZE) / 2;

class Tetrimone {
private:
    int grid[GRID_HEIGHT][GRID_WIDTH];
    int currentPiece;
    int nextPiece;
    int currentRotation;
    int currentX, currentY;
    int score;
    int level;
    int lines;
    int currentTheme;
    float fallSpeed;
    float fallTimer;
    bool gameOver;
    bool paused;
    BITMAP* buffer;
    FONT* gameFont;
    ZipSoundManager* soundManager;
    
    // Color conversion cache
    std::vector<int> themeColors;

    // Convert RGB to Allegro color
    int rgbToAllegroColor(double r, double g, double b) {
        // Convert 0-1 range to 0-255 range and clamp
        int red = std::max(0, std::min(255, (int)(r * 255)));
        int green = std::max(0, std::min(255, (int)(g * 255)));
        int blue = std::max(0, std::min(255, (int)(b * 255)));
        
        // Allegro 4 makecol function
        return makecol(red, green, blue);
    }

    // Update theme colors based on current theme
    void updateThemeColors() {
        themeColors.clear();
        if (currentTheme < NUM_COLOR_THEMES) {
            for (const auto& color : TETRIMONEBLOCK_COLOR_THEMES[currentTheme]) {
                themeColors.push_back(rgbToAllegroColor(color[0], color[1], color[2]));
            }
        } else {
            // Fallback colors if theme index is out of bounds
            for (int i = 0; i < 14; i++) {
                themeColors.push_back(makecol(128 + i * 8, 128 + i * 8, 255));
            }
        }
    }

    // Get theme name for display
    std::string getThemeName() const {
        const std::vector<std::string> themeNames = {
            "Watercolor", "Neon", "Pastel", "Earth Tones", "Monochrome Blue",
            "Monochrome Green", "Sunset", "Ocean", "Grayscale", "Candy",
            "Neon Dark", "Jewel Tones", "Retro Gaming", "Autumn", "Winter",
            "Spring", "Summer", "Monochrome Purple", "Desert", "Rainbow",
            "Art Deco", "Northern Lights", "Moroccan Tiles", "Bioluminescence", "Fossil",
            "Silk Road", "Digital Glitch", "Botanical", "Jazz Age", "Steampunk",
            "USA", "Soviet Retro"
        };
        
        if (currentTheme < themeNames.size()) {
            return themeNames[currentTheme];
        }
        return "Theme " + std::to_string(currentTheme + 1);
    }

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
        currentTheme = 0;
        fallSpeed = 1.0;
        fallTimer = 0;
        gameOver = false;
        paused = false;
        
        // Initialize theme colors
        updateThemeColors();
        
        // Seed random number generator
        srand(time(NULL));
        
        // Create the first piece
        nextPiece = -1;
        spawnPiece();
        
        // Play game start sound and music
        if (soundManager) {
            soundManager->playGameStart();
            soundManager->playThemeMusic(currentTheme);
        }
    }

    // Generate a random piece
    int randomPiece() {
        return rand() % TETRIMONEBLOCK_SHAPES.size();
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
        if (currentPiece < TETRIMONEBLOCK_SHAPES.size()) {
            currentX = GRID_WIDTH / 2 - 2; // Center it roughly
            currentY = 0;
        }
        
        // Check if the new piece collides immediately (game over condition)
        if (checkCollision()) {
            gameOver = true;
            if (soundManager) {
                soundManager->stopMusic();
                soundManager->playGameOver();
            }
        }
    }

    // Check if current piece collides with borders or placed blocks
    bool checkCollision() {
        if (currentPiece >= TETRIMONEBLOCK_SHAPES.size()) return true;
        
        const auto& piece = TETRIMONEBLOCK_SHAPES[currentPiece][currentRotation];
        for (size_t y = 0; y < piece.size(); y++) {
            for (size_t x = 0; x < piece[y].size(); x++) {
                if (piece[y][x]) {
                    int gridX = currentX + x;
                    int gridY = currentY + y;
                    
                    // Check boundaries
                    if (gridX < 0 || gridX >= GRID_WIDTH || gridY >= GRID_HEIGHT) {
                        return true;
                    }
                    
                    // Check collision with placed blocks
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
        if (currentPiece >= TETRIMONEBLOCK_SHAPES.size()) return;
        
        int oldRotation = currentRotation;
        currentRotation = (currentRotation + 1) % TETRIMONEBLOCK_SHAPES[currentPiece].size();
        
        // If the rotation causes a collision, revert back
        if (checkCollision()) {
            currentRotation = oldRotation;
        } else {
            // Play rotation sound
            if (soundManager) {
                soundManager->playPieceRotate();
            }
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
        } else {
            // Play movement sound for lateral moves
            if (dx != 0 && soundManager) {
                soundManager->playPieceMove();
            }
        }
        return true;
    }

    // Lock the current piece into the grid
    void lockPiece() {
        if (currentPiece >= TETRIMONEBLOCK_SHAPES.size()) return;
        
        const auto& piece = TETRIMONEBLOCK_SHAPES[currentPiece][currentRotation];
        for (size_t y = 0; y < piece.size(); y++) {
            for (size_t x = 0; x < piece[y].size(); x++) {
                if (piece[y][x]) {
                    int gridY = currentY + y;
                    int gridX = currentX + x;
                    
                    // Make sure we're not above the grid
                    if (gridY >= 0 && gridY < GRID_HEIGHT && gridX >= 0 && gridX < GRID_WIDTH) {
                        // Store the piece index + 1 in the grid (0 means empty)
                        grid[gridY][gridX] = currentPiece + 1;
                    }
                }
            }
        }
        
        // Play piece drop sound
        if (soundManager) {
            soundManager->playPieceDrop();
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
                
                // Check this line again
                y++;
            }
        }
        
        // Update score and level
        if (linesCleared > 0) {
            // Play line clear sound
            if (soundManager) {
                soundManager->playLineClear(linesCleared);
            }
            
            // Scoring: 100 * multiplier * level
            int multiplier;
            switch (linesCleared) {
                case 1: multiplier = 1; break;
                case 2: multiplier = 3; break;
                case 3: multiplier = 5; break;
                case 4: multiplier = 8; break;
                default: multiplier = linesCleared;
            }
            
            score += 100 * multiplier * level;
            lines += linesCleared;
            
            // Level up every 10 lines
            int newLevel = (lines / 10) + 1;
            if (newLevel != level) {
                level = newLevel;
                
                // Play level up sound and change music
                if (soundManager) {
                    soundManager->playLevelUp();
                    soundManager->playThemeMusic(currentTheme);
                }
                
                // Change theme every level (cycle through available themes)
                currentTheme = (level - 1) % NUM_COLOR_THEMES;
                updateThemeColors();
            }
            
            // Increase fall speed with level (but cap it)
            fallSpeed = std::max(0.1f, 1.0f / level);
        }
    }

    // Drop the piece all the way down
    void hardDrop() {
        while (movePiece(0, 1)) {
            // Keep moving down until we hit something
        }
    }

    // Draw a single block with border
    void drawBlock(BITMAP* bmp, int x, int y, int color) {
        int screenX = GRID_OFFSET_X + x * BLOCK_SIZE;
        int screenY = GRID_OFFSET_Y + y * BLOCK_SIZE;
        
        // Draw filled block
        rectfill(bmp, screenX, screenY, 
                 screenX + BLOCK_SIZE - 1, screenY + BLOCK_SIZE - 1, 
                 color);
        
        // Draw highlight (top and left edges) - lighter version of the color
        int highlightColor = makecol(
            std::min(255, getr(color) + 60),
            std::min(255, getg(color) + 60),
            std::min(255, getb(color) + 60)
        );
        line(bmp, screenX, screenY, screenX + BLOCK_SIZE - 1, screenY, highlightColor);
        line(bmp, screenX, screenY, screenX, screenY + BLOCK_SIZE - 1, highlightColor);
        
        // Draw shadow (bottom and right edges) - darker version of the color
        int shadowColor = makecol(
            std::max(0, getr(color) - 60),
            std::max(0, getg(color) - 60),
            std::max(0, getb(color) - 60)
        );
        line(bmp, screenX, screenY + BLOCK_SIZE - 1, 
             screenX + BLOCK_SIZE - 1, screenY + BLOCK_SIZE - 1, shadowColor);
        line(bmp, screenX + BLOCK_SIZE - 1, screenY, 
             screenX + BLOCK_SIZE - 1, screenY + BLOCK_SIZE - 1, shadowColor);
    }

    // Draw a transparent block for ghost pieces
    void drawTransparentBlock(BITMAP* bmp, int x, int y, int color) {
        int screenX = GRID_OFFSET_X + x * BLOCK_SIZE;
        int screenY = GRID_OFFSET_Y + y * BLOCK_SIZE;
        
        // Create a semi-transparent version of the color (about 30% opacity)
        int transparentColor = makecol(
            getr(color) * 30 / 100,
            getg(color) * 30 / 100,
            getb(color) * 30 / 100
        );
        
        // Draw filled transparent block
        rectfill(bmp, screenX, screenY, 
                 screenX + BLOCK_SIZE - 1, screenY + BLOCK_SIZE - 1, 
                 transparentColor);
        
        // Draw subtle outline
        int outlineColor = makecol(
            getr(color) * 50 / 100,
            getg(color) * 50 / 100,
            getb(color) * 50 / 100
        );
        rect(bmp, screenX, screenY,
             screenX + BLOCK_SIZE - 1, screenY + BLOCK_SIZE - 1,
             outlineColor);
    }

    // Draw the game grid
    void drawGrid() {
        // Draw border only
        rect(buffer, GRID_OFFSET_X - 1, GRID_OFFSET_Y - 1,
             GRID_OFFSET_X + GRID_WIDTH * BLOCK_SIZE,
             GRID_OFFSET_Y + GRID_HEIGHT * BLOCK_SIZE, makecol(255, 255, 255));
        
        // Draw placed blocks
        for (int y = 0; y < GRID_HEIGHT; y++) {
            for (int x = 0; x < GRID_WIDTH; x++) {
                if (grid[y][x]) {
                    int pieceIndex = grid[y][x] - 1;
                    if (pieceIndex < themeColors.size()) {
                        drawBlock(buffer, x, y, themeColors[pieceIndex]);
                    } else {
                        // Fallback color
                        drawBlock(buffer, x, y, makecol(128, 128, 255));
                    }
                }
            }
        }
    }

    // Draw the current falling piece
    void drawCurrentPiece() {
        if (currentPiece >= TETRIMONEBLOCK_SHAPES.size()) return;
        
        const auto& piece = TETRIMONEBLOCK_SHAPES[currentPiece][currentRotation];
        
        for (size_t y = 0; y < piece.size(); y++) {
            for (size_t x = 0; x < piece[y].size(); x++) {
                if (piece[y][x]) {
                    int gridX = currentX + x;
                    int gridY = currentY + y;
                    
                    // Only draw if the block is within the visible grid
                    if (gridY >= 0 && gridX >= 0 && gridX < GRID_WIDTH) {
                        int color;
                        if (currentPiece < themeColors.size()) {
                            color = themeColors[currentPiece];
                        } else {
                            color = makecol(255, 255, 255); // fallback
                        }
                        drawBlock(buffer, gridX, gridY, color);
                    }
                }
            }
        }
    }

    // Draw the ghost piece (preview of where the piece will land)
    void drawGhostPiece() {
        if (currentPiece >= TETRIMONEBLOCK_SHAPES.size()) return;
        
        int ghostY = currentY;
        
        // Find where the piece would land
        while (true) {
            ghostY++;
            
            // Check for collision
            const auto& piece = TETRIMONEBLOCK_SHAPES[currentPiece][currentRotation];
            bool collision = false;
            
            for (size_t y = 0; y < piece.size() && !collision; y++) {
                for (size_t x = 0; x < piece[y].size() && !collision; x++) {
                    if (piece[y][x]) {
                        int gridX = currentX + x;
                        int gridY = ghostY + y;
                        
                        if (gridX < 0 || gridX >= GRID_WIDTH || gridY >= GRID_HEIGHT) {
                            collision = true;
                            break;
                        }
                        
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
        
        // Draw the ghost piece with transparency
        if (ghostY > currentY) {
            const auto& piece = TETRIMONEBLOCK_SHAPES[currentPiece][currentRotation];
            
            for (size_t y = 0; y < piece.size(); y++) {
                for (size_t x = 0; x < piece[y].size(); x++) {
                    if (piece[y][x]) {
                        int gridX = currentX + x;
                        int gridY = ghostY + y;
                        
                        if (gridY >= 0 && gridX >= 0 && gridX < GRID_WIDTH) {
                            int color;
                            if (currentPiece < themeColors.size()) {
                                color = themeColors[currentPiece];
                            } else {
                                color = makecol(128, 128, 128);
                            }
                            
                            // Draw transparent ghost block
                            drawTransparentBlock(buffer, gridX, gridY, color);
                        }
                    }
                }
            }
        }
    }

    // Draw the next piece preview
    void drawNextPiece() {
        if (nextPiece >= TETRIMONEBLOCK_SHAPES.size()) return;
        
        int previewX = GRID_OFFSET_X + GRID_WIDTH * BLOCK_SIZE + 30;
        int previewY = GRID_OFFSET_Y + 30;
        
        // Draw title
        textout_ex(buffer, gameFont, "NEXT", previewX, previewY, makecol(255, 255, 255), -1);
        previewY += 30;
        
        // Calculate preview size
        const auto& piece = TETRIMONEBLOCK_SHAPES[nextPiece][0];
        int pieceHeight = piece.size() * BLOCK_SIZE;
        int pieceWidth = piece[0].size() * BLOCK_SIZE;
        
        // Draw background
        rectfill(buffer, previewX, previewY,
                 previewX + pieceWidth + 20, previewY + pieceHeight + 20, makecol(32, 32, 32));
        
        // Draw next piece
        for (size_t y = 0; y < piece.size(); y++) {
            for (size_t x = 0; x < piece[y].size(); x++) {
                if (piece[y][x]) {
                    int blockX = previewX + 10 + x * BLOCK_SIZE;
                    int blockY = previewY + 10 + y * BLOCK_SIZE;
                    
                    int color;
                    if (nextPiece < themeColors.size()) {
                        color = themeColors[nextPiece];
                    } else {
                        color = makecol(255, 255, 255);
                    }
                    
                    // Draw filled block
                    rectfill(buffer, blockX, blockY,
                             blockX + BLOCK_SIZE - 1, blockY + BLOCK_SIZE - 1,
                             color);
                    
                    // Draw highlight
                    int highlightColor = makecol(
                        std::min(255, getr(color) + 60),
                        std::min(255, getg(color) + 60),
                        std::min(255, getb(color) + 60)
                    );
                    line(buffer, blockX, blockY, blockX + BLOCK_SIZE - 1, blockY, highlightColor);
                    line(buffer, blockX, blockY, blockX, blockY + BLOCK_SIZE - 1, highlightColor);
                    
                    // Draw shadow
                    int shadowColor = makecol(
                        std::max(0, getr(color) - 60),
                        std::max(0, getg(color) - 60),
                        std::max(0, getb(color) - 60)
                    );
                    line(buffer, blockX, blockY + BLOCK_SIZE - 1,
                         blockX + BLOCK_SIZE - 1, blockY + BLOCK_SIZE - 1, shadowColor);
                    line(buffer, blockX + BLOCK_SIZE - 1, blockY,
                         blockX + BLOCK_SIZE - 1, blockY + BLOCK_SIZE - 1, shadowColor);
                }
            }
        }
    }

    // Draw score, level, lines, and theme
    void drawStats() {
        int statsX = GRID_OFFSET_X + GRID_WIDTH * BLOCK_SIZE + 30;
        int statsY = GRID_OFFSET_Y + 250;
        int textColor = makecol(255, 255, 255);
        
        textprintf_ex(buffer, gameFont, statsX, statsY, textColor, -1, "SCORE: %d", score);
        statsY += 25;
        
        textprintf_ex(buffer, gameFont, statsX, statsY, textColor, -1, "LEVEL: %d", level);
        statsY += 25;
        
        textprintf_ex(buffer, gameFont, statsX, statsY, textColor, -1, "LINES: %d", lines);
        statsY += 25;
        
        // Display current theme
        textprintf_ex(buffer, gameFont, statsX, statsY, textColor, -1, "THEME:");
        statsY += 20;
        textprintf_ex(buffer, gameFont, statsX, statsY, textColor, -1, "%s", getThemeName().c_str());
        
        statsY += 40;
        
        // Display audio status
        if (soundManager) {
            textprintf_ex(buffer, gameFont, statsX, statsY, textColor, -1, "MUSIC: %s (M)", 
                         soundManager->isMusicEnabled() ? "ON" : "OFF");
            statsY += 20;
            textprintf_ex(buffer, gameFont, statsX, statsY, textColor, -1, "SOUND: %s (S)", 
                         soundManager->isSoundEnabled() ? "ON" : "OFF");
        }
    }

    // Draw game over screen
    void drawGameOver() {
        rectfill(buffer, 0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, makecol(0, 0, 0));
        
        int textColor = makecol(255, 255, 255);
        textout_centre_ex(buffer, gameFont, "GAME OVER", SCREEN_WIDTH / 2, SCREEN_HEIGHT / 2 - 30, textColor, -1);
        
        textprintf_centre_ex(buffer, gameFont, SCREEN_WIDTH / 2, SCREEN_HEIGHT / 2,
                            textColor, -1, "FINAL SCORE: %d", score);
        
        textprintf_centre_ex(buffer, gameFont, SCREEN_WIDTH / 2, SCREEN_HEIGHT / 2 + 30,
                            textColor, -1, "REACHED LEVEL: %d", level);
        
        textout_centre_ex(buffer, gameFont, "PRESS ENTER TO RESTART", SCREEN_WIDTH / 2, SCREEN_HEIGHT / 2 + 60, textColor, -1);
    }

    // Draw pause screen
    void drawPaused() {
        rectfill(buffer, 0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, makecol(0, 0, 0));
        textout_centre_ex(buffer, gameFont, "PAUSED", SCREEN_WIDTH / 2, SCREEN_HEIGHT / 2, makecol(255, 255, 255), -1);
    }

public:
    Tetrimone() {
        buffer = create_bitmap(SCREEN_WIDTH, SCREEN_HEIGHT);
        gameFont = font;
        nextPiece = -1;
        currentTheme = 0;
        
        // Initialize sound manager
        soundManager = new ZipSoundManager();
        if (soundManager->initialize()) {
            if (soundManager->loadAudioZip("sound.zip")) {
                std::cout << "Audio loaded from ZIP successfully!" << std::endl;
            } else {
                std::cout << "Failed to load audio ZIP - continuing without audio" << std::endl;
            }
        } else {
            delete soundManager;
            soundManager = nullptr;
            std::cout << "Sound disabled - continuing without audio" << std::endl;
        }
        
        init();
    }

    ~Tetrimone() {
        destroy_bitmap(buffer);
        if (soundManager) {
            delete soundManager;
        }
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
    void draw() {
        // Clear buffer
        clear_to_color(buffer, makecol(0, 0, 0));
        
        drawGrid();
        drawGhostPiece();
        drawCurrentPiece();
        drawNextPiece();
        drawStats();
        
        if (gameOver) {
            drawGameOver();
        } else if (paused) {
            drawPaused();
        }
        
        // Draw buffer to screen
        blit(buffer, screen, 0, 0, 0, 0, SCREEN_WIDTH, SCREEN_HEIGHT);
    }

    // Handle keyboard input
    void handleInput() {
        static bool leftPressed = false;
        static bool rightPressed = false;
        static bool downPressed = false;
        static bool upPressed = false;
        static bool spacePressed = false;
        static bool pPressed = false;
        static bool enterPressed = false;
        static bool mPressed = false; // For music toggle
        static bool sPressed = false; // For sound toggle
        
        // Check for key presses
        bool left = key[KEY_LEFT];
        bool right = key[KEY_RIGHT];
        bool down = key[KEY_DOWN];
        bool up = key[KEY_UP];
        bool space = key[KEY_SPACE];
        bool p = key[KEY_P];
        bool enter = key[KEY_ENTER];
        bool m = key[KEY_M];
        bool s = key[KEY_S];
        
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
            if (soundManager) {
                if (paused) {
                    soundManager->pauseMusic();
                } else {
                    soundManager->resumeMusic();
                }
                soundManager->playMenuSelect();
            }
        }
        pPressed = p;
        
        // Toggle music
        if (m && !mPressed && soundManager) {
            soundManager->setMusicEnabled(!soundManager->isMusicEnabled());
            soundManager->playMenuSelect();
        }
        mPressed = m;
        
        // Toggle sound effects
        if (s && !sPressed && soundManager) {
            soundManager->setSoundEnabled(!soundManager->isSoundEnabled());
            if (soundManager->isSoundEnabled()) {
                soundManager->playMenuSelect();
            }
        }
        sPressed = s;
        
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

// Main function
int main() {
    // Initialize Allegro
    if (allegro_init() != 0) {
        std::cerr << "Failed to initialize Allegro!" << std::endl;
        return -1;
    }

    // Install keyboard
    if (install_keyboard() != 0) {
        std::cerr << "Failed to install keyboard!" << std::endl;
        return -1;
    }

    // Install timer
    if (install_timer() != 0) {
        std::cerr << "Failed to install timer!" << std::endl;
        return -1;
    }

    // Set color depth
    set_color_depth(32); // 32-bit for better color support

    // Set graphics mode
    if (set_gfx_mode(GFX_AUTODETECT_WINDOWED, SCREEN_WIDTH, SCREEN_HEIGHT, 0, 0) != 0) {
        if (set_gfx_mode(GFX_AUTODETECT, SCREEN_WIDTH, SCREEN_HEIGHT, 0, 0) != 0) {
            std::cerr << "Failed to set graphics mode!" << std::endl;
            return -1;
        }
    }

    // Set window title
    set_window_title("Tetrimone - Extended Pieces, Themes & ZIP Audio");

    // Create game instance
    Tetrimone tetrimone;

    // Game loop variables
    bool running = true;
    clock_t lastTime = clock();

    // Game loop
    while (running && !key[KEY_ESC]) {
        // Calculate delta time
        clock_t currentTime = clock();
        float dt = (float)(currentTime - lastTime) / CLOCKS_PER_SEC;
        lastTime = currentTime;

        // Handle input
        tetrimone.handleInput();

        // Update game
        tetrimone.update(dt);

        // Draw game
        tetrimone.draw();

        // Small delay to prevent 100% CPU usage
        rest(16); // ~60 FPS
    }

    return 0;
}
END_OF_MAIN()
