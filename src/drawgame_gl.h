#ifndef DRAWGAME_GL_H
#define DRAWGAME_GL_H

#include <GL/glew.h>
#include <GL/gl.h>
#include <gtk/gtk.h>

// ============================================================================
// CORE OPENGL SETUP AND PROJECTION
// ============================================================================

/**
 * Set up 2D orthographic projection for Tetrimone rendering
 * Uses glMatrixMode(), glOrtho() equivalent via matrix math
 * @param width The viewport width in pixels
 * @param height The viewport height in pixels
 */
void gl_setup_2d_projection(int width, int height);

/**
 * Set the current drawing color (opaque)
 * Uses glColor3f() equivalent via shader uniforms
 * @param r Red component (0.0 - 1.0)
 * @param g Green component (0.0 - 1.0)
 * @param b Blue component (0.0 - 1.0)
 */
void gl_set_color(float r, float g, float b);

/**
 * Set the current drawing color with alpha transparency
 * Uses glColor4f() equivalent via shader uniforms
 * @param r Red component (0.0 - 1.0)
 * @param g Green component (0.0 - 1.0)
 * @param b Blue component (0.0 - 1.0)
 * @param a Alpha component (0.0 - 1.0, 0=transparent, 1=opaque)
 */
void gl_set_color_alpha(float r, float g, float b, float a);

// ============================================================================
// PRIMITIVE DRAWING FUNCTIONS
// ============================================================================

/**
 * Draw a filled rectangle
 * Uses glDrawArrays(GL_TRIANGLES) with 6 vertices (2 triangles)
 * @param x X coordinate of top-left corner
 * @param y Y coordinate of top-left corner
 * @param width Width of rectangle
 * @param height Height of rectangle
 */
void gl_draw_rect_filled(float x, float y, float width, float height);

/**
 * Draw a rectangle outline
 * Uses glDrawArrays(GL_LINE_STRIP) with 5 vertices
 * Uses glLineWidth() to set outline thickness
 * @param x X coordinate of top-left corner
 * @param y Y coordinate of top-left corner
 * @param width Width of rectangle
 * @param height Height of rectangle
 * @param line_width Width of the outline in pixels
 */
void gl_draw_rect_outline(float x, float y, float width, float height, float line_width);

/**
 * Draw a line segment
 * Uses glDrawArrays(GL_LINES) with 2 vertices
 * Uses glLineWidth() to set line thickness
 * @param x1 X coordinate of start point
 * @param y1 Y coordinate of start point
 * @param x2 X coordinate of end point
 * @param y2 Y coordinate of end point
 * @param width Width of the line in pixels
 */
void gl_draw_line(float x1, float y1, float x2, float y2, float width);

/**
 * Draw a filled circle
 * Uses glDrawArrays(GL_TRIANGLE_FAN) with (segments + 2) vertices
 * @param cx Center X coordinate
 * @param cy Center Y coordinate
 * @param radius Circle radius in pixels
 * @param segments Number of segments (higher = smoother circle)
 */
void gl_draw_circle(float cx, float cy, float radius, int segments);

/**
 * Draw a circle outline
 * Uses glDrawArrays(GL_LINE_STRIP) with (segments + 1) vertices
 * Uses glLineWidth() to set outline thickness
 * @param cx Center X coordinate
 * @param cy Center Y coordinate
 * @param radius Circle radius in pixels
 * @param line_width Width of the outline in pixels
 * @param segments Number of segments (higher = smoother circle)
 */
void gl_draw_circle_outline(float cx, float cy, float radius, float line_width, int segments);

/**
 * Draw a filled triangle
 * Uses glDrawArrays(GL_TRIANGLES) with 3 vertices
 * @param x1 X coordinate of first vertex
 * @param y1 Y coordinate of first vertex
 * @param x2 X coordinate of second vertex
 * @param y2 Y coordinate of second vertex
 * @param x3 X coordinate of third vertex
 * @param y3 Y coordinate of third vertex
 */
void gl_draw_triangle(float x1, float y1, float x2, float y2, float x3, float y3);

// ============================================================================
// TETRIMONE GAME DRAWING FUNCTIONS
// ============================================================================

/**
 * Draw the background of the game board
 * Includes main area fill and border
 * Uses glDrawArrays(GL_TRIANGLES) and GL_LINE_STRIP
 */
void drawBackground_gl(TetrimoneBoard *board, int width, int height);

/**
 * Draw grid lines for the game board
 * Conditional rendering based on board->isShowingGridLines()
 * Uses glDrawArrays(GL_LINES) for each grid line
 */
void drawGridLines_gl(TetrimoneBoard *board);

/**
 * Draw the failure/death line at the top of the board
 * Red line with glow effect
 * Uses glDrawArrays(GL_LINES) twice (main line + glow)
 */
void drawFailureLine_gl();

/**
 * Draw the splash/title screen
 * Semi-transparent overlay with tetris blocks decoration
 * Uses multiple glDrawArrays calls (GL_TRIANGLES, GL_LINE_STRIP)
 */
void drawSplashScreen_gl(TetrimoneBoard *board, TetrimoneApp *app);

/**
 * Draw all placed blocks on the board
 * Includes animations for line clears
 * Uses glDrawArrays(GL_TRIANGLES) for each block and 3D effects
 */
void drawPlacedBlocks_gl(TetrimoneBoard *board, TetrimoneApp *app);

/**
 * Draw the currently falling tetrimone piece
 * Includes 3D highlight and shadow effects
 * Uses glDrawArrays(GL_TRIANGLES) for blocks and highlights
 */
void drawCurrentPiece_gl(TetrimoneBoard *board);

/**
 * Draw the ghost/preview of where the current piece will land
 * Semi-transparent outline
 * Uses glDrawArrays(GL_LINE_STRIP) for outlines
 */
void drawGhostPiece_gl(TetrimoneBoard *board);

/**
 * Draw the GAME OVER screen
 * Semi-transparent overlay with decorative elements
 * Uses glDrawArrays(GL_TRIANGLES, GL_LINE_STRIP) for overlay, box, and circles
 */
void drawGameOver_gl(TetrimoneBoard *board);

/**
 * Draw the PAUSED overlay
 * Semi-transparent background with pause indication
 * Uses glDrawArrays(GL_TRIANGLES, GL_LINE_STRIP)
 */
void drawPauseMenu_gl(TetrimoneBoard *board);

/**
 * Draw propaganda/status message if active
 * Retro-style message box
 * Uses glDrawArrays(GL_TRIANGLES, GL_LINE_STRIP)
 */
void drawPropagandaMessage_gl(TetrimoneBoard *board);

/**
 * Draw line clear fireworks particle effects
 * Uses glDrawArrays(GL_TRIANGLE_FAN) for each particle
 */
void drawFireworks_gl(TetrimoneBoard *board);

/**
 * Draw block movement trail effects
 * Semi-transparent fading lines
 * Uses glDrawArrays(GL_LINES) for trail segments
 */
void drawBlockTrails_gl(TetrimoneBoard *board);

/**
 * Draw glowing heat effect for hot blocks
 * Orange glow with pulsing animation
 * Uses glDrawArrays(GL_TRIANGLE_FAN) twice
 */
void drawFireyGlow_gl(double x, double y, double size, float heatLevel, double time);

/**
 * Draw freezing effect for cold blocks
 * Light blue glow with icy sparkle
 * Uses glDrawArrays(GL_TRIANGLE_FAN) twice
 */
void drawFreezyEffect_gl(double x, double y, double size, float heatLevel, double time);

/**
 * Draw next piece preview box
 * Shows a single upcoming piece
 * Uses glDrawArrays(GL_TRIANGLES, GL_LINE_STRIP)
 */
void drawNextPiecePreview_gl(TetrimoneBoard *board, int previewIndex, int screenX, int screenY, int previewSize);

// ============================================================================
// GTK/OPENGL INTEGRATION
// ============================================================================

/**
 * Initialize the main game rendering callback
 * Connects to the GTK realize and render signals
 * @param gl_area The GtkGLArea widget
 */
void tetrimone_gl_init(GtkGLArea *gl_area);

/**
 * Initialize the next piece preview rendering callback
 * Connects to the GTK realize and render signals
 * @param gl_area The GtkGLArea widget for next piece display
 */
void tetrimone_gl_next_piece_init(GtkGLArea *gl_area);

#endif // DRAWGAME_GL_H
