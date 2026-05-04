#include "tetrimone_sdl2.h"
#include <algorithm>
#include <cmath>

TetrimoneBlock::TetrimoneBlock(int type) : type(type), rotation(0) {
    x = GRID_WIDTH / 2 - 2;
    y = 0;
}

void TetrimoneBlock::rotate(bool clockwise) {
    rotation = (rotation + (clockwise ? 1 : 3)) % 4;
}

int TetrimoneBlock::getRotation() const { return rotation; }
void TetrimoneBlock::move(int dx, int dy) { x += dx; y += dy; }
void TetrimoneBlock::setPosition(int newX, int newY) { x = newX; y = newY; }

std::vector<std::vector<int>> TetrimoneBlock::getShape() const {
    static const int shapes[7][4][4][4] = {
        {{{0,0,0,0},{1,1,1,1},{0,0,0,0},{0,0,0,0}},{{0,0,1,0},{0,0,1,0},{0,0,1,0},{0,0,1,0}},{{0,0,0,0},{1,1,1,1},{0,0,0,0},{0,0,0,0}},{{0,1,0,0},{0,1,0,0},{0,1,0,0},{0,1,0,0}}},
        {{{0,1,1,0},{0,1,1,0},{0,0,0,0},{0,0,0,0}},{{0,1,1,0},{0,1,1,0},{0,0,0,0},{0,0,0,0}},{{0,1,1,0},{0,1,1,0},{0,0,0,0},{0,0,0,0}},{{0,1,1,0},{0,1,1,0},{0,0,0,0},{0,0,0,0}}},
        {{{0,1,0,0},{1,1,1,0},{0,0,0,0},{0,0,0,0}},{{0,1,0,0},{0,1,1,0},{0,1,0,0},{0,0,0,0}},{{0,0,0,0},{1,1,1,0},{0,1,0,0},{0,0,0,0}},{{0,1,0,0},{1,1,0,0},{0,1,0,0},{0,0,0,0}}},
        {{{0,1,1,0},{1,1,0,0},{0,0,0,0},{0,0,0,0}},{{0,1,0,0},{0,1,1,0},{0,0,1,0},{0,0,0,0}},{{0,0,0,0},{0,1,1,0},{1,1,0,0},{0,0,0,0}},{{1,0,0,0},{1,1,0,0},{0,1,0,0},{0,0,0,0}}},
        {{{1,1,0,0},{0,1,1,0},{0,0,0,0},{0,0,0,0}},{{0,0,1,0},{0,1,1,0},{0,1,0,0},{0,0,0,0}},{{0,0,0,0},{1,1,0,0},{0,1,1,0},{0,0,0,0}},{{0,1,0,0},{1,1,0,0},{1,0,0,0},{0,0,0,0}}},
        {{{0,1,0,0},{0,1,0,0},{1,1,0,0},{0,0,0,0}},{{1,0,0,0},{1,1,1,0},{0,0,0,0},{0,0,0,0}},{{0,1,1,0},{0,1,0,0},{0,1,0,0},{0,0,0,0}},{{0,0,0,0},{1,1,1,0},{0,0,1,0},{0,0,0,0}}},
        {{{1,0,0,0},{1,0,0,0},{1,1,0,0},{0,0,0,0}},{{0,0,0,0},{1,1,1,0},{1,0,0,0},{0,0,0,0}},{{0,1,1,0},{0,1,0,0},{0,1,0,0},{0,0,0,0}},{{0,0,1,0},{1,1,1,0},{0,0,0,0},{0,0,0,0}}}
    };
    std::vector<std::vector<int>> result;
    if (type >= 0 && type < 7 && rotation >= 0 && rotation < 4) {
        for (int row = 0; row < 4; row++) {
            std::vector<int> r;
            for (int col = 0; col < 4; col++) {
                r.push_back(shapes[type][rotation][row][col]);
            }
            result.push_back(r);
        }
    }
    return result;
}

std::array<double, 3> TetrimoneBlock::getColor() const {
    static const std::array<double, 3> colors[] = {
        {0.0, 1.0, 1.0}, {1.0, 1.0, 0.0}, {1.0, 0.0, 1.0},
        {0.0, 1.0, 0.0}, {1.0, 0.0, 0.0}, {0.0, 0.0, 1.0},
        {1.0, 0.65, 0.0}
    };
    if (type >= 0 && type < 7) return colors[type];
    return {1.0, 1.0, 1.0};
}

TetrimoneBoard::TetrimoneBoard(TetrimoneApp* appPtr) {
    for (int i = 0; i < 3; i++) {
        nextPieces.push_back(std::make_unique<TetrimoneBlock>(rand() % 7));
    }
}

TetrimoneBoard::~TetrimoneBoard() {}

void TetrimoneBoard::update(uint32_t deltaTime) {
    if (currentPiece) {
        currentPiece->move(0, 1);
    }
}

void TetrimoneBoard::render(SDL_Renderer* renderer, int screenWidth, int screenHeight) {}

void TetrimoneBoard::movePiece(int dx, int dy) {
    if (currentPiece) currentPiece->move(dx, dy);
}

void TetrimoneBoard::rotatePiece(bool clockwise) {
    if (currentPiece) currentPiece->rotate(clockwise);
}

void TetrimoneBoard::hardDropPiece() {
    if (currentPiece) {
        for (int i = 0; i < GRID_HEIGHT; i++) {
            currentPiece->move(0, 1);
        }
    }
}

void TetrimoneBoard::playSound(GameSoundEvent event) {}
void TetrimoneBoard::startSmoothMovement(int newX, int newY) {}
void TetrimoneBoard::startThemeTransition(int newTheme) { isThemeTransitioning = true; }
void TetrimoneBoard::updateThemeTransition() { if (isThemeTransitioning) isThemeTransitioning = false; }
void TetrimoneBoard::startFireworksAnimation(int linesCleared) {}
