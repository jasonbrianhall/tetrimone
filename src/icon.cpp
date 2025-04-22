#include "tetrimone.h"

// Function to create a Tetrimone icon programmatically
void setWindowIcon(GtkWindow* window) {
    // Create a 64x64 pixel icon with RGBA format (32 bits)
    int width = 64;
    int height = 64;
    
    // Create a pixbuf with an alpha channel (transparency)
    GdkPixbuf* icon = gdk_pixbuf_new(GDK_COLORSPACE_RGB, TRUE, 8, width, height);
    
    // Get a pointer to the pixel data
    guchar* pixels = gdk_pixbuf_get_pixels(icon);
    int rowstride = gdk_pixbuf_get_rowstride(icon);
    
    // Clear the entire image with transparent black
    for (int y = 0; y < height; y++) {
        guchar* row = pixels + y * rowstride;
        for (int x = 0; x < width; x++) {
            guchar* pixel = row + x * 4;
            pixel[0] = 0;   // R
            pixel[1] = 0;   // G
            pixel[2] = 0;   // B
            pixel[3] = 0;   // Alpha (0 = transparent)
        }
    }
    
    // Define Tetrimone blocks to draw
    struct Block {
        int x, y, size;
        guchar r, g, b;
    };
    
    // Create T-shaped tetrimone (purple)
    std::vector<Block> tBlocks = {
        {24, 8, 12, 128, 0, 128},   // Center top
        {12, 20, 12, 148, 0, 148},  // Left middle
        {24, 20, 12, 148, 0, 148},  // Center middle
        {36, 20, 12, 148, 0, 148}   // Right middle
    };
    
    // Create I-shaped tetrimone (cyan)
    std::vector<Block> iBlocks = {
        {8, 40, 12, 0, 255, 255},   // Left
        {20, 40, 12, 0, 235, 235},  // Middle-left
        {32, 40, 12, 0, 235, 235},  // Middle-right
        {44, 40, 12, 0, 255, 255}   // Right
    };
    
    // Helper function to draw a filled block with a 3D effect
    auto drawBlock = [&](const Block& block) {
        // Main block fill
        for (int y = 0; y < block.size; y++) {
            for (int x = 0; x < block.size; x++) {
                int posX = block.x + x;
                int posY = block.y + y;
                
                if (posX >= 0 && posX < width && posY >= 0 && posY < height) {
                    guchar* pixel = pixels + posY * rowstride + posX * 4;
                    pixel[0] = block.r;  // R
                    pixel[1] = block.g;  // G
                    pixel[2] = block.b;  // B
                    pixel[3] = 255;      // Alpha (fully opaque)
                }
            }
        }
        
        // Add highlight (3D effect) - top and left edges
        for (int i = 0; i < block.size; i++) {
            // Top edge highlight
            if (block.y >= 0 && block.y < height && (block.x + i) >= 0 && (block.x + i) < width) {
                guchar* pixel = pixels + block.y * rowstride + (block.x + i) * 4;
                pixel[0] = std::min(255, block.r + 70);  // Lighten
                pixel[1] = std::min(255, block.g + 70);
                pixel[2] = std::min(255, block.b + 70);
                pixel[3] = 255;
            }
            
            // Left edge highlight
            if ((block.y + i) >= 0 && (block.y + i) < height && block.x >= 0 && block.x < width) {
                guchar* pixel = pixels + (block.y + i) * rowstride + block.x * 4;
                pixel[0] = std::min(255, block.r + 50);  // Lighten
                pixel[1] = std::min(255, block.g + 50);
                pixel[2] = std::min(255, block.b + 50);
                pixel[3] = 255;
            }
        }
        
        // Add shadow (3D effect) - bottom and right edges
        for (int i = 0; i < block.size; i++) {
            // Bottom edge shadow
            int bottomY = block.y + block.size - 1;
            if (bottomY >= 0 && bottomY < height && (block.x + i) >= 0 && (block.x + i) < width) {
                guchar* pixel = pixels + bottomY * rowstride + (block.x + i) * 4;
                pixel[0] = std::max(0, block.r - 50);  // Darken
                pixel[1] = std::max(0, block.g - 50);
                pixel[2] = std::max(0, block.b - 50);
                pixel[3] = 255;
            }
            
            // Right edge shadow
            int rightX = block.x + block.size - 1;
            if ((block.y + i) >= 0 && (block.y + i) < height && rightX >= 0 && rightX < width) {
                guchar* pixel = pixels + (block.y + i) * rowstride + rightX * 4;
                pixel[0] = std::max(0, block.r - 40);  // Darken
                pixel[1] = std::max(0, block.g - 40);
                pixel[2] = std::max(0, block.b - 40);
                pixel[3] = 255;
            }
        }
    };
    
    // Draw all the blocks
    for (const auto& block : tBlocks) {
        drawBlock(block);
    }
    for (const auto& block : iBlocks) {
        drawBlock(block);
    }
    
    // Set the icon
    gtk_window_set_icon(window, icon);
    
    // Clean up
    g_object_unref(icon);
}
