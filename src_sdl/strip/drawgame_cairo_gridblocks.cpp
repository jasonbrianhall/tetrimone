// Grid block drawing for locked pieces
#include "tetrimone_core.h"
#include <cairo/cairo.h>
#include <cmath>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

void drawGridBlocks(cairo_t *cr, TetrimoneBoard *board) {
    if (!board) return;
    
    for (int y = 0; y < GRID_HEIGHT; y++) {
        for (int x = 0; x < GRID_WIDTH; x++) {
            int gridValue = board->getGridValue(x, y);
            if (gridValue <= 0) continue;
            
            // Check if this line is being cleared (animated)
            bool isClearing = board->isLineBeingCleared(y);
            
            int px = x * BLOCK_SIZE;
            int py = y * BLOCK_SIZE;
            
            // Get block color with theme interpolation support
            std::array<double, 3> color = board->getInterpolatedColor(gridValue, 0.0);
            
            if (isClearing) {
                // Draw line clear animation - blocks fade out and scale
                double clearProgress = 0.0; // This would come from the board's animation state
                
                // Create fade effect during line clear
                double alpha = 1.0 - clearProgress;
                cairo_set_source_rgba(cr, color[0], color[1], color[2], alpha);
                
                // Optional: scale effect
                double scale = 1.0 - (clearProgress * 0.5);
                int centerX = px + BLOCK_SIZE / 2;
                int centerY = py + BLOCK_SIZE / 2;
                
                cairo_save(cr);
                cairo_translate(cr, centerX, centerY);
                cairo_scale(cr, scale, scale);
                cairo_translate(cr, -centerX, -centerY);
                
                cairo_rectangle(cr, px, py, BLOCK_SIZE, BLOCK_SIZE);
                cairo_fill(cr);
                
                cairo_restore(cr);
            } else {
                // Normal block rendering - solid fill
                cairo_set_source_rgb(cr, color[0], color[1], color[2]);
                cairo_rectangle(cr, px, py, BLOCK_SIZE, BLOCK_SIZE);
                cairo_fill(cr);
            }
            
            // Draw black border
            cairo_set_source_rgb(cr, 0, 0, 0);
            cairo_set_line_width(cr, 1);
            cairo_rectangle(cr, px, py, BLOCK_SIZE, BLOCK_SIZE);
            cairo_stroke(cr);
        }
    }
}
