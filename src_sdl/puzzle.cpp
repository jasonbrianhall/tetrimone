#include "tetrimone.h"

    void TetrimoneBoard::placeBlock(int x, int y, int blockType) {
        // Validate grid coordinates
        if (x < 0 || x >= GRID_WIDTH || y < 0 || y >= GRID_HEIGHT) {
            std::cerr << "Invalid grid coordinates: (" << x << ", " << y << ")" << std::endl;
            return;
        }

        // Validate block type (1-13 correspond to block types, 0 means clear)
        if (blockType < 0 || blockType > 13) {
            std::cerr << "Invalid block type: " << blockType << std::endl;
            return;
        }

        // Place the block in the grid
        grid[y][x] = blockType;
    }

    // Optional: Method to clear a specific block or entire grid
    void TetrimoneBoard::clearGrid() {
        for (auto& row : grid) {
            std::fill(row.begin(), row.end(), 0);
        }
    }
