#include "tetrimone_sdl2.h"
#include <cmath>
#include <sstream>
#include <iomanip>

SDL_Color rgb_to_sdl(double r, double g, double b) {
    return SDL_Color{
        static_cast<Uint8>(r * 255),
        static_cast<Uint8>(g * 255),
        static_cast<Uint8>(b * 255),
        255
    };
}

void draw_rect_filled(SDL_Renderer* renderer, int x, int y, int w, int h, SDL_Color color) {
    SDL_SetRenderDrawColor(renderer, color.r, color.g, color.b, color.a);
    SDL_Rect rect = {x, y, w, h};
    SDL_RenderFillRect(renderer, &rect);
}

void draw_rect_outline(SDL_Renderer* renderer, int x, int y, int w, int h, SDL_Color color, int thickness) {
    SDL_SetRenderDrawColor(renderer, color.r, color.g, color.b, color.a);
    for (int i = 0; i < thickness; i++) {
        SDL_Rect rect = {x + i, y + i, w - 2 * i, h - 2 * i};
        SDL_RenderDrawRect(renderer, &rect);
    }
}

void draw_filled_rect_with_border(SDL_Renderer* renderer, int x, int y, int w, int h, SDL_Color fill, SDL_Color border) {
    draw_rect_filled(renderer, x, y, w, h, fill);
    draw_rect_outline(renderer, x, y, w, h, border, 2);
}

void draw_circle(SDL_Renderer* renderer, int centerX, int centerY, int radius, SDL_Color color, bool filled) {
    SDL_SetRenderDrawColor(renderer, color.r, color.g, color.b, color.a);
    
    if (filled) {
        for (int y = -radius; y <= radius; y++) {
            for (int x = -radius; x <= radius; x++) {
                if (x * x + y * y <= radius * radius) {
                    SDL_RenderDrawPoint(renderer, centerX + x, centerY + y);
                }
            }
        }
    } else {
        int x = radius;
        int y = 0;
        int d = 3 - 2 * radius;
        
        while (x >= y) {
            SDL_RenderDrawPoint(renderer, centerX + x, centerY + y);
            SDL_RenderDrawPoint(renderer, centerX - x, centerY + y);
            SDL_RenderDrawPoint(renderer, centerX + x, centerY - y);
            SDL_RenderDrawPoint(renderer, centerX - x, centerY - y);
            SDL_RenderDrawPoint(renderer, centerX + y, centerY + x);
            SDL_RenderDrawPoint(renderer, centerX - y, centerY + x);
            SDL_RenderDrawPoint(renderer, centerX + y, centerY - x);
            SDL_RenderDrawPoint(renderer, centerX - y, centerY - x);
            
            if (d < 0) {
                d = d + 4 * y + 6;
            } else {
                d = d + 4 * (y - x) + 10;
                x--;
            }
            y++;
        }
    }
}

void draw_block(SDL_Renderer* renderer, int x, int y, int size, SDL_Color color, bool is3d) {
    if (is3d) {
        // Draw 3D block with shading
        SDL_Color darkColor = {
            static_cast<Uint8>(color.r * 0.6),
            static_cast<Uint8>(color.g * 0.6),
            static_cast<Uint8>(color.b * 0.6),
            255
        };
        SDL_Color lightColor = {
            static_cast<Uint8>(std::min(255, (int)(color.r * 1.4))),
            static_cast<Uint8>(std::min(255, (int)(color.g * 1.4))),
            static_cast<Uint8>(std::min(255, (int)(color.b * 1.4))),
            255
        };

        // Main body
        draw_rect_filled(renderer, x, y, size, size, color);
        
        // Top highlight
        SDL_SetRenderDrawColor(renderer, lightColor.r, lightColor.g, lightColor.b, lightColor.a);
        SDL_RenderDrawLine(renderer, x, y, x + size - 1, y);
        SDL_RenderDrawLine(renderer, x, y, x, y + size - 1);
        
        // Bottom shadow
        SDL_SetRenderDrawColor(renderer, darkColor.r, darkColor.g, darkColor.b, darkColor.a);
        SDL_RenderDrawLine(renderer, x + size - 1, y, x + size - 1, y + size - 1);
        SDL_RenderDrawLine(renderer, x, y + size - 1, x + size - 1, y + size - 1);
    } else {
        // Simple flat block
        draw_rect_filled(renderer, x, y, size, size, color);
        draw_rect_outline(renderer, x, y, size, size, {0, 0, 0, 255}, 1);
    }
}

void render_text(SDL_Renderer* renderer, TTF_Font* font, const std::string& text, int x, int y, SDL_Color color) {
    if (!font) return;

    SDL_Surface* surface = TTF_RenderText_Solid(font, text.c_str(), color);
    if (!surface) return;

    SDL_Texture* texture = SDL_CreateTextureFromSurface(renderer, surface);
    if (texture) {
        SDL_Rect dst = {x, y, surface->w, surface->h};
        SDL_RenderCopy(renderer, texture, nullptr, &dst);
        SDL_DestroyTexture(texture);
    }

    SDL_FreeSurface(surface);
}

void render_text_centered(SDL_Renderer* renderer, TTF_Font* font, const std::string& text, int x, int y, int w, SDL_Color color) {
    if (!font) return;

    SDL_Surface* surface = TTF_RenderText_Solid(font, text.c_str(), color);
    if (!surface) return;

    SDL_Texture* texture = SDL_CreateTextureFromSurface(renderer, surface);
    if (texture) {
        SDL_Rect dst = {x + (w - surface->w) / 2, y, surface->w, surface->h};
        SDL_RenderCopy(renderer, texture, nullptr, &dst);
        SDL_DestroyTexture(texture);
    }

    SDL_FreeSurface(surface);
}

void render_grid_lines(SDL_Renderer* renderer, int x, int y, int gridW, int gridH, int blockSize) {
    SDL_SetRenderDrawColor(renderer, 100, 100, 100, 100);
    
    for (int i = 0; i <= gridW; i++) {
        SDL_RenderDrawLine(renderer, x + i * blockSize, y, x + i * blockSize, y + gridH * blockSize);
    }
    
    for (int i = 0; i <= gridH; i++) {
        SDL_RenderDrawLine(renderer, x, y + i * blockSize, x + gridW * blockSize, y + i * blockSize);
    }
}

void render_blocks(SDL_Renderer* renderer, TetrimoneApp& app, int x, int y) {
    TetrimoneBoard* board = app.board;
    const auto& grid = board->getGrid();
    int gridW = board->getGridWidth();
    int gridH = board->getGridHeight();

    for (int gridY = 0; gridY < gridH; gridY++) {
        for (int gridX = 0; gridX < gridW; gridX++) {
            int blockType = grid[gridY][gridX];
            if (blockType > 0) {
                blockType--;  // Convert from 1-7 to 0-6
                SDL_Color color = rgb_to_sdl(1.0, 1.0, 1.0);  // TODO: Use actual color
                int posX = x + gridX * BLOCK_SIZE;
                int posY = y + gridY * BLOCK_SIZE;
                draw_block(renderer, posX, posY, BLOCK_SIZE, color);
            }
        }
    }
}

void render_current_piece(SDL_Renderer* renderer, TetrimoneApp& app, int x, int y) {
    TetrimoneBoard* board = app.board;
    const auto* piece = board->getCurrentPiece();
    
    if (!piece) return;

    double pieceX, pieceY;
    board->getCurrentPieceInterpolatedPosition(pieceX, pieceY);

    auto shape = piece->getShape();
    SDL_Color color = rgb_to_sdl(1.0, 1.0, 1.0);  // TODO: Use actual color

    for (int shapeY = 0; shapeY < (int)shape.size(); shapeY++) {
        for (int shapeX = 0; shapeX < (int)shape[shapeY].size(); shapeX++) {
            if (shape[shapeY][shapeX]) {
                int gridX = (int)pieceX + shapeX;
                int gridY = (int)pieceY + shapeY;
                int posX = x + gridX * BLOCK_SIZE;
                int posY = y + gridY * BLOCK_SIZE;
                draw_block(renderer, posX, posY, BLOCK_SIZE, color);
            }
        }
    }
}

void render_ghost_piece(SDL_Renderer* renderer, TetrimoneApp& app, int x, int y) {
    TetrimoneBoard* board = app.board;
    const auto* piece = board->getCurrentPiece();
    
    if (!piece) return;

    int ghostY = board->getGhostPieceY();
    if (ghostY < 0) return;

    auto shape = piece->getShape();
    SDL_Color color = {100, 100, 100, 128};

    for (int shapeY = 0; shapeY < (int)shape.size(); shapeY++) {
        for (int shapeX = 0; shapeX < (int)shape[shapeY].size(); shapeX++) {
            if (shape[shapeY][shapeX]) {
                int gridX = piece->getX() + shapeX;
                int gridY = ghostY + shapeY;
                int posX = x + gridX * BLOCK_SIZE;
                int posY = y + gridY * BLOCK_SIZE;
                draw_rect_outline(renderer, posX, posY, BLOCK_SIZE, BLOCK_SIZE, color, 1);
            }
        }
    }
}

void render_next_pieces(SDL_Renderer* renderer, TetrimoneApp& app, int x, int y, int w, int h) {
    TetrimoneBoard* board = app.board;
    const auto& nextPieces = board->getNextPieces();

    render_text(renderer, app.mainFont, "NEXT", x, y, {255, 255, 255, 255});

    int previewY = y + 30;
    for (int i = 0; i < std::min((int)nextPieces.size(), 3); i++) {
        const auto& piece = nextPieces[i];
        auto shape = piece->getShape();
        
        SDL_Color color = rgb_to_sdl(1.0, 1.0, 1.0);  // TODO: Use actual color
        
        int previewSize = BLOCK_SIZE * 0.6;
        int startX = x + 10;
        int startY = previewY + i * (previewSize * 4 + 10);

        for (int shapeY = 0; shapeY < (int)shape.size(); shapeY++) {
            for (int shapeX = 0; shapeX < (int)shape[shapeY].size(); shapeX++) {
                if (shape[shapeY][shapeX]) {
                    int drawX = startX + shapeX * previewSize;
                    int drawY = startY + shapeY * previewSize;
                    draw_block(renderer, drawX, drawY, (int)previewSize, color);
                }
            }
        }
    }
}

void render_game_area(SDL_Renderer* renderer, TetrimoneApp& app, int x, int y, int w, int h) {
    TetrimoneBoard* board = app.board;

    // Draw background
    draw_rect_filled(renderer, x, y, w, h, {40, 40, 45, 255});

    // Draw grid lines if enabled
    if (board->gridLinesEnabled) {
        render_grid_lines(renderer, x, y, board->getGridWidth(), board->getGridHeight(), BLOCK_SIZE);
    }

    // Draw placed blocks
    render_blocks(renderer, app, x, y);

    // Draw ghost piece
    render_ghost_piece(renderer, app, x, y);

    // Draw current piece
    render_current_piece(renderer, app, x, y);

    // Draw border
    draw_rect_outline(renderer, x, y, w, h, {200, 200, 200, 255}, 2);
}

void render_sidebar(SDL_Renderer* renderer, TetrimoneApp& app, int x, int y, int w, int h) {
    TetrimoneBoard* board = app.board;

    // Draw background
    draw_rect_filled(renderer, x, y, w, h, {35, 35, 40, 255});

    // Draw info
    int infoY = y + 20;
    
    std::stringstream scoreStr;
    scoreStr << "Score: " << board->getScore();
    render_text(renderer, app.mainFont, scoreStr.str(), x + 10, infoY, {255, 255, 255, 255});

    infoY += 30;
    std::stringstream levelStr;
    levelStr << "Level: " << board->getLevel();
    render_text(renderer, app.mainFont, levelStr.str(), x + 10, infoY, {255, 255, 255, 255});

    infoY += 30;
    std::stringstream linesStr;
    linesStr << "Lines: " << board->getLinesCleared();
    render_text(renderer, app.mainFont, linesStr.str(), x + 10, infoY, {255, 255, 255, 255});

    // Draw next pieces preview
    int nextY = infoY + 50;
    render_next_pieces(renderer, app, x, nextY, w, h - nextY);

    // Draw game status
    int statusY = y + h - 60;
    if (board->isPaused()) {
        render_text_centered(renderer, app.largeFont, "PAUSED", x, statusY, w, {255, 200, 0, 255});
    }

    if (board->isGameOver()) {
        render_text_centered(renderer, app.largeFont, "GAME OVER", x, statusY, w, {255, 0, 0, 255});
    }

    // Draw FPS (debug)
    std::stringstream fpsStr;
    fpsStr << std::fixed << std::setprecision(1) << app.fps << " FPS";
    render_text(renderer, app.smallFont, fpsStr.str(), x + 10, y + h - 20, {150, 150, 150, 255});
}

void render_menu(SDL_Renderer* renderer, TetrimoneApp& app) {
    int windowWidth, windowHeight;
    SDL_GetWindowSize(app.window, &windowWidth, &windowHeight);

    // Semi-transparent overlay
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 200);
    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
    SDL_Rect overlay = {0, 0, windowWidth, windowHeight};
    SDL_RenderFillRect(renderer, &overlay);

    // Menu box
    int menuWidth = 300;
    int menuHeight = 300;
    int menuX = (windowWidth - menuWidth) / 2;
    int menuY = (windowHeight - menuHeight) / 2;

    draw_filled_rect_with_border(renderer, menuX, menuY, menuWidth, menuHeight, {50, 50, 60, 255}, {200, 200, 200, 255});

    // Title
    render_text_centered(renderer, app.largeFont, "TETRIMONE", menuX, menuY + 20, menuWidth, {0, 200, 255, 255});

    // Menu items
    int itemY = menuY + 80;
    render_text_centered(renderer, app.mainFont, "[SPACE] Start Game", menuX, itemY, menuWidth, {255, 255, 255, 255});
    itemY += 40;
    render_text_centered(renderer, app.mainFont, "[S] Settings", menuX, itemY, menuWidth, {255, 255, 255, 255});
    itemY += 40;
    render_text_centered(renderer, app.mainFont, "[H] High Scores", menuX, itemY, menuWidth, {255, 255, 255, 255});
    itemY += 40;
    render_text_centered(renderer, app.mainFont, "[Q] Quit", menuX, itemY, menuWidth, {255, 255, 255, 255});
}

void render_pause_menu(SDL_Renderer* renderer, TetrimoneApp& app) {
    int windowWidth, windowHeight;
    SDL_GetWindowSize(app.window, &windowWidth, &windowHeight);

    // Semi-transparent overlay
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 200);
    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
    SDL_Rect overlay = {0, 0, windowWidth, windowHeight};
    SDL_RenderFillRect(renderer, &overlay);

    // Menu box
    int menuWidth = 300;
    int menuHeight = 250;
    int menuX = (windowWidth - menuWidth) / 2;
    int menuY = (windowHeight - menuHeight) / 2;

    draw_filled_rect_with_border(renderer, menuX, menuY, menuWidth, menuHeight, {50, 50, 60, 255}, {200, 200, 200, 255});

    // Title
    render_text_centered(renderer, app.largeFont, "PAUSED", menuX, menuY + 20, menuWidth, {255, 200, 0, 255});

    // Menu items
    int itemY = menuY + 80;
    render_text_centered(renderer, app.mainFont, "[P] Resume", menuX, itemY, menuWidth, {255, 255, 255, 255});
    itemY += 40;
    render_text_centered(renderer, app.mainFont, "[R] Restart", menuX, itemY, menuWidth, {255, 255, 255, 255});
    itemY += 40;
    render_text_centered(renderer, app.mainFont, "[ESC] Menu", menuX, itemY, menuWidth, {255, 255, 255, 255});
}

void render_game_over_screen(SDL_Renderer* renderer, TetrimoneApp& app) {
    int windowWidth, windowHeight;
    SDL_GetWindowSize(app.window, &windowWidth, &windowHeight);

    // Semi-transparent overlay
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 200);
    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
    SDL_Rect overlay = {0, 0, windowWidth, windowHeight};
    SDL_RenderFillRect(renderer, &overlay);

    // Menu box
    int menuWidth = 350;
    int menuHeight = 300;
    int menuX = (windowWidth - menuWidth) / 2;
    int menuY = (windowHeight - menuHeight) / 2;

    draw_filled_rect_with_border(renderer, menuX, menuY, menuWidth, menuHeight, {50, 50, 60, 255}, {255, 0, 0, 255});

    // Title
    render_text_centered(renderer, app.largeFont, "GAME OVER", menuX, menuY + 20, menuWidth, {255, 0, 0, 255});

    // Stats
    TetrimoneBoard* board = app.board;
    int statY = menuY + 80;
    
    std::stringstream scoreStr;
    scoreStr << "Final Score: " << board->getScore();
    render_text_centered(renderer, app.mainFont, scoreStr.str(), menuX, statY, menuWidth, {255, 255, 255, 255});
    
    statY += 35;
    std::stringstream levelStr;
    levelStr << "Level: " << board->getLevel();
    render_text_centered(renderer, app.mainFont, levelStr.str(), menuX, statY, menuWidth, {255, 255, 255, 255});
    
    statY += 35;
    std::stringstream linesStr;
    linesStr << "Lines: " << board->getLinesCleared();
    render_text_centered(renderer, app.mainFont, linesStr.str(), menuX, statY, menuWidth, {255, 255, 255, 255});

    // Options
    statY += 60;
    render_text_centered(renderer, app.mainFont, "[R] Play Again  [ESC] Menu", menuX, statY, menuWidth, {255, 200, 0, 255});
}

void show_main_menu(TetrimoneApp& app) {
    app.activeDialog = TetrimoneApp::DialogState::MAIN_MENU;
}

void show_pause_menu(TetrimoneApp& app) {
    app.activeDialog = TetrimoneApp::DialogState::PAUSE_MENU;
}

void show_game_over_dialog(TetrimoneApp& app) {
    app.activeDialog = TetrimoneApp::DialogState::GAME_OVER;
}

void show_settings_dialog(TetrimoneApp& app) {
    app.activeDialog = TetrimoneApp::DialogState::SETTINGS;
}

void show_high_scores_dialog(TetrimoneApp& app) {
    app.activeDialog = TetrimoneApp::DialogState::HIGH_SCORES;
}

void show_game_setup_dialog(TetrimoneApp& app) {
    app.activeDialog = TetrimoneApp::DialogState::GAME_SETUP;
}

void show_volume_dialog(TetrimoneApp& app) {
    // Dialog implementation
}

void show_about_dialog(TetrimoneApp& app) {
    // Dialog implementation
}
