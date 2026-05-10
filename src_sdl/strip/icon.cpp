#include <vector>
#include <algorithm>

#ifdef GTK3
#include "tetrimone_gtk.h"
#include <gdk-pixbuf/gdk-pixbuf.h>
#endif

#ifdef QT5
#include "tetrimone_qt5.h"
#include <QIcon>
#include <QPixmap>
#include <QPainter>
#endif

// ============================================================================
// Core Icon Data (Framework-Independent)
// ============================================================================

struct IconBlock {
    int x, y, size;
    unsigned char r, g, b;
};

// Get T-shaped tetrimone blocks (purple)
std::vector<IconBlock> getTBlocksData() {
    return {
        {24, 8, 12, 128, 0, 128},   // Center top
        {12, 20, 12, 148, 0, 148},  // Left middle
        {24, 20, 12, 148, 0, 148},  // Center middle
        {36, 20, 12, 148, 0, 148}   // Right middle
    };
}

// Get I-shaped tetrimone blocks (cyan)
std::vector<IconBlock> getIBlocksData() {
    return {
        {8, 40, 12, 0, 255, 255},   // Left
        {20, 40, 12, 0, 235, 235},  // Middle-left
        {32, 40, 12, 0, 235, 235},  // Middle-right
        {44, 40, 12, 0, 255, 255}   // Right
    };
}

// ============================================================================
// GTK3 Implementation
// ============================================================================

#ifdef GTK3

void drawIconBlock(guchar* pixels, int rowstride, int width, int height, const IconBlock& block) {
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
            pixel[0] = std::min(255, (int)block.r + 70);  // Lighten
            pixel[1] = std::min(255, (int)block.g + 70);
            pixel[2] = std::min(255, (int)block.b + 70);
            pixel[3] = 255;
        }
        
        // Left edge highlight
        if ((block.y + i) >= 0 && (block.y + i) < height && block.x >= 0 && block.x < width) {
            guchar* pixel = pixels + (block.y + i) * rowstride + block.x * 4;
            pixel[0] = std::min(255, (int)block.r + 50);  // Lighten
            pixel[1] = std::min(255, (int)block.g + 50);
            pixel[2] = std::min(255, (int)block.b + 50);
            pixel[3] = 255;
        }
    }
    
    // Add shadow (3D effect) - bottom and right edges
    for (int i = 0; i < block.size; i++) {
        // Bottom edge shadow
        int bottomY = block.y + block.size - 1;
        if (bottomY >= 0 && bottomY < height && (block.x + i) >= 0 && (block.x + i) < width) {
            guchar* pixel = pixels + bottomY * rowstride + (block.x + i) * 4;
            pixel[0] = std::max(0, (int)block.r - 50);  // Darken
            pixel[1] = std::max(0, (int)block.g - 50);
            pixel[2] = std::max(0, (int)block.b - 50);
            pixel[3] = 255;
        }
        
        // Right edge shadow
        int rightX = block.x + block.size - 1;
        if ((block.y + i) >= 0 && (block.y + i) < height && rightX >= 0 && rightX < width) {
            guchar* pixel = pixels + (block.y + i) * rowstride + rightX * 4;
            pixel[0] = std::max(0, (int)block.r - 40);  // Darken
            pixel[1] = std::max(0, (int)block.g - 40);
            pixel[2] = std::max(0, (int)block.b - 40);
            pixel[3] = 255;
        }
    }
}

// Function to create a Tetrimone icon programmatically (GTK3)
void setWindowIcon(GtkWindow* window) {
    // Create a 64x64 pixel icon with RGBA format (32 bits)
    int width = 64;
    int height = 64;
    
    // Create a pixbuf with an alpha channel (transparency)
    GdkPixbuf* icon = gdk_pixbuf_new(GDK_COLORSPACE_RGB, true, 8, width, height);
    
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
    
    // Draw all the blocks
    auto tBlocks = getTBlocksData();
    auto iBlocks = getIBlocksData();
    
    for (const auto& block : tBlocks) {
        drawIconBlock(pixels, rowstride, width, height, block);
    }
    for (const auto& block : iBlocks) {
        drawIconBlock(pixels, rowstride, width, height, block);
    }
    
    // Set the icon
    gtk_window_set_icon(window, icon);
    
    // Clean up
    g_object_unref(icon);
}

#endif  // GTK3

// ============================================================================
// Qt5 Implementation
// ============================================================================

#ifdef QT5

void setWindowIcon(void* window) {
    // Create a 64x64 pixel icon with alpha channel
    int width = 64;
    int height = 64;
    
    QPixmap pixmap(width, height);
    pixmap.fill(Qt::transparent);  // Start with transparent background
    
    QPainter painter(&pixmap);
    
    // Helper lambda to draw a block with 3D effect
    auto drawBlock = [&painter](const IconBlock& block) {
        // Main block fill
        painter.fillRect(block.x, block.y, block.size, block.size,
                        QColor(block.r, block.g, block.b));
        
        // Highlight (top and left edges)
        int lighter_r = std::min(255, (int)block.r + 70);
        int lighter_g = std::min(255, (int)block.g + 70);
        int lighter_b = std::min(255, (int)block.b + 70);
        
        painter.setPen(QColor(lighter_r, lighter_g, lighter_b));
        painter.drawLine(block.x, block.y, block.x + block.size - 1, block.y);  // Top
        painter.drawLine(block.x, block.y, block.x, block.y + block.size - 1);  // Left
        
        // Shadow (bottom and right edges)
        int darker_r = std::max(0, (int)block.r - 50);
        int darker_g = std::max(0, (int)block.g - 50);
        int darker_b = std::max(0, (int)block.b - 50);
        
        painter.setPen(QColor(darker_r, darker_g, darker_b));
        painter.drawLine(block.x, block.y + block.size - 1, 
                        block.x + block.size - 1, block.y + block.size - 1);  // Bottom
        painter.drawLine(block.x + block.size - 1, block.y, 
                        block.x + block.size - 1, block.y + block.size - 1);  // Right
    };
    
    // Draw all the blocks
    auto tBlocks = getTBlocksData();
    auto iBlocks = getIBlocksData();
    
    for (const auto& block : tBlocks) {
        drawBlock(block);
    }
    for (const auto& block : iBlocks) {
        drawBlock(block);
    }
    
    painter.end();
    
    // Set the icon on the window (cast to QMainWindow or QWidget as needed)
    // Note: window pointer type varies by application; casting here depends on context
    // This is a simplified version - adjust based on your actual Qt5 window type
    // QMainWindow* mainWindow = static_cast<QMainWindow*>(window);
    // mainWindow->setWindowIcon(QIcon(pixmap));
}

#endif  // QT5
