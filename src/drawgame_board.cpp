// ============================================================================
// drawgame_board.cpp - Whole-board Cairo rendering shared by the GTK3 and
// wxWidgets frontends (moved out of gtkstuff.cpp unchanged, apart from a
// void function no longer returning FALSE).
// ============================================================================

#ifdef GTK3
#include "tetrimone_gtk.h"
#endif

#ifdef WXWIDGETS
#include "tetrimone_wx.h"
#endif

#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstdio>

void drawPlacedBlocks(cairo_t *cr, TetrimoneBoard *board, TetrimoneApp *app) {
  for (int y = 0; y < GRID_HEIGHT; ++y) {
    for (int x = 0; x < GRID_WIDTH; ++x) {
      int value = board->getGridValue(x, y);
      if (value > 0) {
        // Check if this line is being cleared
        double alpha = 1.0;
        double scale = 1.0;
        double offsetX = 0.0;
        double offsetY = 0.0;
        
        if (board->isLineClearActive() && board->isLineBeingCleared(y)) {
          // Get animation progress (0.0 to 1.0)
          double progress = board->getLineClearProgress();
          
          if (board->retroModeActive) {
            // Soviet-era computer animation: Simple scan line effect
            if (progress < 0.3) {
              // Horizontal scan line sweep from left to right
              double scanProgress = progress / 0.3;
              int scanX = (int)(scanProgress * GRID_WIDTH);
              
              // Only affect blocks that have been "scanned"
              if (x <= scanX) {
                alpha = 0.3 + 0.4 * sin(progress * 20.0); // Subtle flicker
              } else {
                alpha = 1.0; // Normal until scanned
              }
              scale = 1.0;
            } else if (progress < 0.7) {
              // All blocks flash in unison (like old CRT monitors)
              double flashProgress = (progress - 0.3) / 0.4;
              alpha = 1.0 - flashProgress * 0.7;
              
              // Simulate old monitor "collapse" effect - vertical compression
              scale = 1.0;
              offsetY = flashProgress * BLOCK_SIZE * 0.3; // Slight downward compression
            } else {
              // Final "wipe" effect - blocks disappear in chunks
              double wipeProgress = (progress - 0.7) / 0.3;
              
              // Divide line into segments that disappear sequentially
              int segment = x / 3; // 3-block segments
              double segmentDelay = segment * 0.2;
              
              if (wipeProgress > segmentDelay) {
                alpha = 0.0; // Instant disappear once segment is reached
                scale = 0.0;
              } else {
                alpha = 1.0 - wipeProgress * 0.5;
                scale = 1.0;
              }
            }
          } else {
            // Modern animations - 10 different types selected randomly
            int animationType = board->getCurrentAnimationType();
            LineClearAnimValues animValues = getLineClearAnimationValues(animationType, progress, x, y);
            alpha = animValues.alpha;
            scale = animValues.scale;
            offsetX = animValues.offsetX;
            offsetY = animValues.offsetY;
          }
        }

        // Get color from tetrimoneblock colors
        auto baseColor = board->isInThemeTransition() ? 
        board->getInterpolatedColor(value - 1, board->getThemeTransitionProgress()) :
        TETRIMONEBLOCK_COLOR_THEMES[currentThemeIndex][value - 1];
        auto color = getHeatModifiedColor(baseColor, board->getHeatLevel());

        cairo_set_source_rgba(cr, color[0], color[1], color[2], alpha);

        // Calculate position with animation offsets
        double drawX = x * BLOCK_SIZE + offsetX + (BLOCK_SIZE * (1.0 - scale)) / 2;
        double drawY = y * BLOCK_SIZE + offsetY + (BLOCK_SIZE * (1.0 - scale)) / 2;
        double drawSize = BLOCK_SIZE * scale;

        if (board->retroModeActive || board->simpleBlocksActive) {
          // Simple blocks
          cairo_rectangle(cr, drawX, drawY, drawSize, drawSize);
          cairo_fill(cr);
        } else {
          // 3D blocks with scaling
          cairo_rectangle(cr, drawX + 1, drawY + 1, drawSize - 2, drawSize - 2);
          cairo_fill(cr);

          // Draw highlight (3D effect)
          cairo_set_source_rgba(cr, 1, 1, 1, 0.3 * alpha);
          cairo_move_to(cr, drawX + 1, drawY + 1);
          cairo_line_to(cr, drawX + drawSize - 1, drawY + 1);
          cairo_line_to(cr, drawX + 1, drawY + drawSize - 1);
          cairo_close_path(cr);
          cairo_fill(cr);

          // Draw shadow (3D effect)
          cairo_set_source_rgba(cr, 0, 0, 0, 0.3 * alpha);
          cairo_move_to(cr, drawX + drawSize - 1, drawY + 1);
          cairo_line_to(cr, drawX + drawSize - 1, drawY + drawSize - 1);
          cairo_line_to(cr, drawX + 1, drawY + drawSize - 1);
          cairo_close_path(cr);
          cairo_fill(cr);
        }
        
        if (!board->retroModeActive) { // Only apply effects in modern mode
          float heatLevel = board->getHeatLevel();
          
          // Get current time for animation
          auto now = std::chrono::high_resolution_clock::now();
          auto timeMs = std::chrono::duration<double, std::milli>(
              now.time_since_epoch()).count();
          
          // Draw fiery glow effect when hot
          if (heatLevel > 0.7f) {
            drawFireyGlow(cr, drawX, drawY, drawSize, heatLevel, timeMs);
            updateDisplay(app);
          }
          
          // Draw freezy effect when cold
          if (heatLevel < 0.3f) {
            drawFreezyEffect(cr, drawX, drawY, drawSize, heatLevel, timeMs);
            updateDisplay(app);
          }
        }   
      }
    }
  }
}

void OnDrawGameAreaCairo(cairo_t *cr, TetrimoneApp *app, int width, int height) {

  TetrimoneBoard *board = app->board;

  // Draw background
  drawBackground(cr, board, width, height);

  // Draw gridlines
  drawGridLines(cr, board);

  // Draw failure line
  drawFailureLine(cr);

  // Draw placed blocks with line clearing animation
  drawPlacedBlocks(cr, board, app);

  // Draw splash screen if active
  if (board->isSplashScreenActive()) {
    drawSplashScreen(cr, board, app);
    return;
  }

  // Draw propaganda messages
  drawPropagandaMessage(cr, board);

  // Draw current piece with smooth movement animation
  if (board->getCurrentPiece()) {
    drawCurrentPiece(cr, board);
  } else {
    fprintf(stderr, "WARNING: getCurrentPiece() returned null, skipping drawCurrentPiece\n");
  }

  // Draw ghost piece with interpolated position
  drawGhostPiece(cr, board);

  // Draw enhanced pause menu if paused
  drawPauseMenu(cr, board);

  // Draw game over text if needed
  drawGameOver(cr, board);


  if (board->isFireworksActive()) {
    drawFireworks(cr, board, app);
  }
 
  if (board->isTrailsEnabled() && board->isBlockTrailsActive()) {
    drawBlockTrails(cr, board);
  }
} 

void onDrawNextPieceCairo(cairo_t *cr, TetrimoneApp *app, int width, int height) {
  TetrimoneBoard *board = app->board;
  // Draw background
  cairo_set_source_rgb(cr, 0.1, 0.1, 0.1);
  cairo_rectangle(cr, 0, 0, width, height);
  cairo_fill(cr);

  if (!board->isGameOver()) {
    // Calculate section width - each piece gets exactly 1/3 of the total width
    int sectionWidth = width / 3;

    // Calculate preview block size (half of the normal block size)
    int previewBlockSize = BLOCK_SIZE / 2;

    // Draw dividers between sections
    cairo_set_source_rgb(cr, 0.3, 0.3, 0.3);
    cairo_set_line_width(cr, 2);
    cairo_move_to(cr, sectionWidth, 0);
    cairo_line_to(cr, sectionWidth, height);
    cairo_move_to(cr, sectionWidth * 2, 0);
    cairo_line_to(cr, sectionWidth * 2, height);
    cairo_stroke(cr);

    // Process each piece
    for (int pieceIndex = 0; pieceIndex < 3; pieceIndex++) {
      // Calculate the section's X position
      int sectionX = pieceIndex * sectionWidth;

      // Reserve space for the header
      int headerHeight = 25;

      // Get the piece information
      const TetrimoneBlock* piece = board->getNextPiece(pieceIndex);
      if (!piece) continue;  // Skip if piece doesn't exist yet
      
      auto shape = piece->getShape();
      auto color = piece->getColor();

      // Calculate the shape dimensions in blocks
      int pieceWidth = 0;
      for (const auto &row : shape) {
        pieceWidth = std::max(pieceWidth, (int)row.size());
      }
      int pieceHeight = shape.size();

      // Center the piece in the available space (using the preview block size)
      int availableWidth = sectionWidth;
      int offsetX =
          sectionX + (availableWidth - pieceWidth * previewBlockSize) / 2;
      int offsetY = headerHeight + (height - headerHeight -
                                    pieceHeight * previewBlockSize) /
                                       2;

      // Draw piece number label at the top of each section
      cairo_set_source_rgb(cr, 0.8, 0.8, 0.8);
      cairo_select_font_face(cr, "Sans", CAIRO_FONT_SLANT_NORMAL,
                             CAIRO_FONT_WEIGHT_BOLD);

      // Scale font size with block size
      double fontSize = std::max(10.0, std::min(16.0, previewBlockSize * 0.5));
      cairo_set_font_size(cr, fontSize);

      // Use a shorter label to avoid width issues
      char pieceLabel[10];
      snprintf(pieceLabel, sizeof(pieceLabel), "%d", pieceIndex + 1);

      cairo_text_extents_t extents;
      cairo_text_extents(cr, pieceLabel, &extents);

      // Center the text in the section
      double textX = sectionX + (sectionWidth - extents.width) / 2;
      cairo_move_to(cr, textX, headerHeight - 5);
      cairo_show_text(cr, pieceLabel);

      // Set color for drawing
      cairo_set_source_rgb(cr, color[0], color[1], color[2]);

      // Draw the piece blocks with half size
      for (size_t y = 0; y < shape.size(); ++y) {
        for (size_t x = 0; x < shape[y].size(); ++x) {
          if (shape[y][x] == 1) {
            int drawX = offsetX + x * previewBlockSize;
            int drawY = offsetY + y * previewBlockSize;
     if (board->retroModeActive || board->simpleBlocksActive) {
        // In retro mode, draw simple blocks without 3D effects
        cairo_rectangle(cr, drawX, drawY, previewBlockSize, previewBlockSize);
        cairo_fill(cr);
      } else {
            // Draw block with a small margin
            cairo_rectangle(cr, drawX + 1, drawY + 1, previewBlockSize - 2,
                            previewBlockSize - 2);
            cairo_fill(cr);

            // Draw highlight (3D effect)
            cairo_set_source_rgba(cr, 1, 1, 1, 0.3);
            cairo_move_to(cr, drawX + 1, drawY + 1);
            cairo_line_to(cr, drawX + previewBlockSize - 1, drawY + 1);
            cairo_line_to(cr, drawX + 1, drawY + previewBlockSize - 1);
            cairo_close_path(cr);
            cairo_fill(cr);

            // Draw shadow (3D effect)
            cairo_set_source_rgba(cr, 0, 0, 0, 0.3);
            cairo_move_to(cr, drawX + previewBlockSize - 1, drawY + 1);
            cairo_line_to(cr, drawX + previewBlockSize - 1,
                          drawY + previewBlockSize - 1);
            cairo_line_to(cr, drawX + 1, drawY + previewBlockSize - 1);
            cairo_close_path(cr);
            cairo_fill(cr);

            // Reset color for next block
            cairo_set_source_rgb(cr, color[0], color[1], color[2]);
       }
          }
        }
      }
    }
  }
}
