// Enhanced drawgame.cpp with smooth animations
#include "audiomanager.h"
#include "tetrimone.h"
#include <algorithm>
#include <iostream>
#include <string>
#ifdef _WIN32
#include <commdlg.h>
#include <windows.h>
#endif
#include "highscores.h"
#include "propaganda_messages.h"

// Define M_PI for Windows compatibility
#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

gboolean onDrawGameArea(GtkWidget *widget, cairo_t *cr, gpointer data) {
  TetrimoneApp *app = static_cast<TetrimoneApp *>(data);
  TetrimoneBoard *board = app->board;

  // Get widget dimensions
  GtkAllocation allocation;
  gtk_widget_get_allocation(widget, &allocation);

  // Draw background
  cairo_set_source_rgb(cr, 0.1, 0.1, 0.1);
  cairo_rectangle(cr, 0, 0, allocation.width, allocation.height);
  cairo_fill(cr);

  // Draw background image if enabled
  if ((board->isUsingBackgroundImage() || board->isUsingBackgroundZip()) &&
      board->getBackgroundImage() != nullptr) {
    // Save the current state
    cairo_save(cr);

    // Check if we're in a transition
    if (board->isInBackgroundTransition()) {
      // If fading out, draw old background first
      if (board->getTransitionDirection() == -1 &&
          board->getOldBackground() != nullptr) {
        // Get the image dimensions
        int imgWidth = cairo_image_surface_get_width(board->getOldBackground());
        int imgHeight =
            cairo_image_surface_get_height(board->getOldBackground());

        // Calculate scaling to fill the game area while maintaining aspect
        // ratio
        double scaleX = static_cast<double>(allocation.width) / imgWidth;
        double scaleY = static_cast<double>(allocation.height) / imgHeight;
        double scale = std::max(scaleX, scaleY);

        // Calculate position to center the image
        double x = (allocation.width - imgWidth * scale) / 2;
        double y = (allocation.height - imgHeight * scale) / 2;

        // Apply the transformation
        cairo_translate(cr, x, y);
        cairo_scale(cr, scale, scale);

        // Draw the old image with current transition opacity
        cairo_set_source_surface(cr, board->getOldBackground(), 0, 0);
        cairo_paint_with_alpha(cr, board->getTransitionOpacity());

        // Reset transformation for next drawing
        cairo_restore(cr);
        cairo_save(cr);
      } else if (board->getTransitionDirection() == 1) {
        // Fading in - draw new background with transition opacity
        // Get the image dimensions
        int imgWidth =
            cairo_image_surface_get_width(board->getBackgroundImage());
        int imgHeight =
            cairo_image_surface_get_height(board->getBackgroundImage());

        // Calculate scaling to fill the game area while maintaining aspect
        // ratio
        double scaleX = static_cast<double>(allocation.width) / imgWidth;
        double scaleY = static_cast<double>(allocation.height) / imgHeight;
        double scale = std::max(scaleX, scaleY);

        // Calculate position to center the image
        double x = (allocation.width - imgWidth * scale) / 2;
        double y = (allocation.height - imgHeight * scale) / 2;

        // Apply the transformation
        cairo_translate(cr, x, y);
        cairo_scale(cr, scale, scale);

        // Draw the new image with transition opacity
        cairo_set_source_surface(cr, board->getBackgroundImage(), 0, 0);
        cairo_paint_with_alpha(cr, board->getTransitionOpacity());
      }
    } else {
      // Normal drawing (no transition)
      // Get the image dimensions
      int imgWidth = cairo_image_surface_get_width(board->getBackgroundImage());
      int imgHeight =
          cairo_image_surface_get_height(board->getBackgroundImage());

      // Calculate scaling to fill the game area while maintaining aspect ratio
      double scaleX = static_cast<double>(allocation.width) / imgWidth;
      double scaleY = static_cast<double>(allocation.height) / imgHeight;
      double scale = std::max(scaleX, scaleY);

      // Calculate position to center the image
      double x = (allocation.width - imgWidth * scale) / 2;
      double y = (allocation.height - imgHeight * scale) / 2;

      // Apply the transformation
      cairo_translate(cr, x, y);
      cairo_scale(cr, scale, scale);

      // Draw the image with normal opacity
      cairo_set_source_surface(cr, board->getBackgroundImage(), 0, 0);
      cairo_paint_with_alpha(cr, board->getBackgroundOpacity());
    }

    // Restore the original state
    cairo_restore(cr);
  }

  if (board->isShowingGridLines()) {
    cairo_set_source_rgb(cr, 0.3, 0.3, 0.3);
    cairo_set_line_width(cr, 1);

    // Vertical lines
    for (int x = 1; x < GRID_WIDTH; ++x) {
      cairo_move_to(cr, x * BLOCK_SIZE, 0);
      cairo_line_to(cr, x * BLOCK_SIZE, GRID_HEIGHT * BLOCK_SIZE);
    }

    // Horizontal lines
    for (int y = 1; y < GRID_HEIGHT; ++y) {
      cairo_move_to(cr, 0, y * BLOCK_SIZE);
      cairo_line_to(cr, GRID_WIDTH * BLOCK_SIZE, y * BLOCK_SIZE);
    }

    cairo_stroke(cr);
  }

  int failureLineY = 2;
  cairo_set_source_rgb(cr, 1.0, 0.2, 0.2);
  cairo_set_line_width(cr, 1.0);
  cairo_move_to(cr, 0, failureLineY * BLOCK_SIZE);
  cairo_line_to(cr, GRID_WIDTH * BLOCK_SIZE, failureLineY * BLOCK_SIZE);
  cairo_stroke(cr);

  // Draw placed blocks with line clearing animation
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
              offsetY = flashProgress * BLOCK_SIZE *
                        0.3; // Slight downward compression
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

            switch (animationType) {
            case 0: // Classic Shrink & Scatter
              if (progress < 0.2) {
                alpha = 0.4 + 0.6 * sin(progress * 25.0);
                scale = 1.0;
              } else if (progress < 0.6) {
                double scaleProgress = (progress - 0.2) / 0.4;
                scale = 1.0 - scaleProgress * 0.8;
                alpha = 1.0 - scaleProgress * 0.4;
              } else {
                double fadeProgress = (progress - 0.6) / 0.4;
                alpha = (1.0 - fadeProgress * 0.6) * 0.6;
                scale = 0.2 - fadeProgress * 0.2;
                offsetX = (rand() % 8 - 4) * fadeProgress * 3;
                offsetY = (rand() % 8 - 4) * fadeProgress * 3;
              }
              break;

            case 1: // Dissolve (blocks fade randomly)
            {
              int blockSeed = (x * 31 + y * 17) % 100;
              double blockDelay = (blockSeed / 100.0) * 0.3;

              if (progress < blockDelay) {
                alpha = 1.0;
                scale = 1.0;
              } else if (progress < blockDelay + 0.4) {
                double dissolveProgress = (progress - blockDelay) / 0.4;
                alpha = 1.0 - dissolveProgress;
                scale = 1.0 - dissolveProgress * 0.3;
              } else {
                alpha = 0.0;
                scale = 0.7;
              }
            } break;

            case 2: // Ripple Wave (from center outward)
            {
              double centerX = GRID_WIDTH / 2.0;
              double distance = abs(x - centerX) / centerX;
              double waveDelay = distance * 0.3;

              if (progress < waveDelay) {
                alpha = 1.0;
                scale = 1.0;
              } else if (progress < waveDelay + 0.5) {
                double waveProgress = (progress - waveDelay) / 0.5;
                alpha = 1.0 - waveProgress;
                scale = 1.0 + sin(waveProgress * M_PI) * 0.3;
                offsetY = sin(waveProgress * M_PI * 2) * 5;
              } else {
                alpha = 0.0;
                scale = 0.0;
              }
            } break;

            case 3: // Explosion (blocks shoot outward)
              if (progress < 0.15) {
                alpha = 1.0;
                scale = 1.0 + progress * 2;
              } else if (progress < 0.6) {
                double explodeProgress = (progress - 0.15) / 0.45;
                alpha = 1.0 - explodeProgress * 0.7;
                scale = 1.3 - explodeProgress * 0.5;

                double centerX = GRID_WIDTH / 2.0;
                double forceX = (x - centerX) * explodeProgress * 15;
                double forceY = -explodeProgress * 20;
                offsetX = forceX;
                offsetY = forceY;
              } else {
                double finalProgress = (progress - 0.6) / 0.4;
                alpha = 0.3 - finalProgress * 0.3;
                scale = 0.8 - finalProgress * 0.8;

                double centerX = GRID_WIDTH / 2.0;
                double forceX = (x - centerX) * (1.0 + finalProgress) * 15;
                double forceY = -20 + finalProgress * 40;
                offsetX = forceX;
                offsetY = forceY;
              }
              break;

            case 4: // Spin & Vanish
              if (progress < 0.3) {
                alpha = 1.0;
                scale = 1.0;
                double rotationSpeed = progress * 20;
                offsetX = sin(rotationSpeed) * 3;
                offsetY = cos(rotationSpeed) * 3;
              } else if (progress < 0.8) {
                double spinProgress = (progress - 0.3) / 0.5;
                alpha = 1.0 - spinProgress * 0.8;
                scale = 1.0 - spinProgress * 0.7;

                double rotationSpeed = (progress * 40) + (spinProgress * 60);
                offsetX = sin(rotationSpeed) * (3 - spinProgress * 3);
                offsetY = cos(rotationSpeed) * (3 - spinProgress * 3);
              } else {
                double finalProgress = (progress - 0.8) / 0.2;
                alpha = 0.2 - finalProgress * 0.2;
                scale = 0.3 - finalProgress * 0.3;
                offsetX = 0;
                offsetY = 0;
              }
              break;

            case 5: // Left-to-Right Sweep
            {
              double sweepProgress = progress;
              double blockDelay = (x / (double)GRID_WIDTH) *
                                  0.4; // Spread over 40% of animation

              if (sweepProgress < blockDelay) {
                alpha = 1.0;
                scale = 1.0;
              } else if (sweepProgress < blockDelay + 0.3) {
                double localProgress = (sweepProgress - blockDelay) / 0.3;
                alpha = 1.0 - localProgress;
                scale = 1.0 - localProgress * 0.6;
                offsetX = localProgress * 20; // Slide right as they fade
              } else {
                alpha = 0.0;
                scale = 0.4;
                offsetX = 20;
              }
            } break;

            case 6: // Bounce & Pop
              if (progress < 0.2) {
                alpha = 1.0;
                scale = 1.0 + sin(progress * 15) * 0.2; // Quick bounce
              } else if (progress < 0.5) {
                double bounceProgress = (progress - 0.2) / 0.3;
                alpha = 1.0;
                scale = 1.0 + bounceProgress * 0.8; // Inflate
                offsetY = -bounceProgress * 10;     // Lift up
              } else if (progress < 0.7) {
                double popProgress = (progress - 0.5) / 0.2;
                alpha = 1.0 - popProgress * 0.9;
                scale = 1.8 + popProgress * 0.5; // Pop bigger
                offsetY = -10 - popProgress * 5;
              } else {
                double fadeProgress = (progress - 0.7) / 0.3;
                alpha = 0.1 - fadeProgress * 0.1;
                scale = 2.3 - fadeProgress * 2.3;
                offsetY = -15;
              }
              break;

            case 7: // Melt Down
              if (progress < 0.3) {
                alpha = 1.0;
                scale = 1.0;
                offsetY = progress * 5; // Start sinking slightly
              } else if (progress < 0.7) {
                double meltProgress = (progress - 0.3) / 0.4;
                alpha = 1.0 - meltProgress * 0.6;
                scale = 1.0;                     // Keep width
                offsetY = 5 + meltProgress * 15; // Sink down
              } else {
                double finalProgress = (progress - 0.7) / 0.3;
                alpha = 0.4 - finalProgress * 0.4;
                scale = 1.0;
                offsetY = 20 + finalProgress * 10;
              }
              break;

            case 8: // Zigzag Wipe
            {
              // Create zigzag pattern based on x position
              int zigzagOffset =
                  (x % 4 < 2) ? 0 : 2; // Alternating pattern every 2 blocks
              double zigzagDelay =
                  ((x + zigzagOffset) / (double)GRID_WIDTH) * 0.5;

              if (progress < zigzagDelay) {
                alpha = 1.0;
                scale = 1.0;
              } else if (progress < zigzagDelay + 0.4) {
                double wipeProgress = (progress - zigzagDelay) / 0.4;
                alpha = 1.0 - wipeProgress;
                scale = 1.0 - wipeProgress * 0.8;

                // Zigzag motion
                offsetX = sin(wipeProgress * M_PI * 4) * 8;
                offsetY = wipeProgress * 12;
              } else {
                alpha = 0.0;
                scale = 0.2;
              }
            } break;

            case 9: // Fireworks Burst
              if (progress < 0.1) {
                alpha = 1.0;
                scale = 1.0;
              } else if (progress < 0.25) {
                double chargeProgress = (progress - 0.1) / 0.15;
                alpha = 1.0;
                scale = 1.0 - chargeProgress * 0.3; // Compress before burst
                offsetY = -chargeProgress * 8;
              } else if (progress < 0.4) {
                double burstProgress = (progress - 0.25) / 0.15;
                alpha = 1.0;
                scale = 0.7 + burstProgress * 1.0; // Sudden expansion
                offsetY = -8 + burstProgress * 3;
              } else if (progress < 0.8) {
                double sparkleProgress = (progress - 0.4) / 0.4;
                alpha = 1.0 - sparkleProgress * 0.7;
                scale = 1.7 - sparkleProgress * 0.9;

                // Sparkle effect - random small movements
                int sparkSeed = (x * 23 + y * 41 + (int)(progress * 100)) % 100;
                offsetX = (sparkSeed % 20 - 10) * sparkleProgress * 0.8;
                offsetY =
                    -5 + ((sparkSeed / 20) % 20 - 10) * sparkleProgress * 0.8;
              } else {
                double fadeProgress = (progress - 0.8) / 0.2;
                alpha = 0.3 - fadeProgress * 0.3;
                scale = 0.8 - fadeProgress * 0.8;
                offsetX = 0;
                offsetY = 0;
              }
              break;
            }
          }
        }

        // Get color from tetrimoneblock colors
        /*        auto color = board->isInThemeTransition() ?
            board->getInterpolatedColor(value - 1,
           board->getThemeTransitionProgress()) :
            TETRIMONEBLOCK_COLOR_THEMES[currentThemeIndex][value - 1]; */

        auto baseColor =
            board->isInThemeTransition()
                ? board->getInterpolatedColor(
                      value - 1, board->getThemeTransitionProgress())
                : TETRIMONEBLOCK_COLOR_THEMES[currentThemeIndex][value - 1];
        auto color = getHeatModifiedColor(baseColor, board->getHeatLevel());

        /*auto baseColor = TETRIMONEBLOCK_COLOR_THEMES[themeIndex][blockType -
        1]; auto heatColor = getHeatModifiedColor(baseColor,
        app->board->getHeatLevel());

        cairo_set_source_rgba(cr, heatColor[0], heatColor[1], heatColor[2],
        alpha); */

        cairo_set_source_rgba(cr, color[0], color[1], color[2], alpha);

        // Calculate position with animation offsets
        double drawX =
            x * BLOCK_SIZE + offsetX + (BLOCK_SIZE * (1.0 - scale)) / 2;
        double drawY =
            y * BLOCK_SIZE + offsetY + (BLOCK_SIZE * (1.0 - scale)) / 2;
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
          auto timeMs =
              std::chrono::duration<double, std::milli>(now.time_since_epoch())
                  .count();

          // Draw fiery glow effect when hot
          if (heatLevel > 0.7f) {
            drawFireyGlow(cr, drawX, drawY, drawSize, heatLevel, timeMs);
            gtk_widget_queue_draw(app->gameArea);
          }

          // Draw freezy effect when cold
          if (heatLevel < 0.3f) {
            drawFreezyEffect(cr, drawX, drawY, drawSize, heatLevel, timeMs);
            gtk_widget_queue_draw(app->gameArea);
          }
        }
      }
    }
  }

  // Draw splash screen if active
  if (board->isSplashScreenActive()) {
    // Semi-transparent overlay
    cairo_set_source_rgba(cr, 0, 0, 0, 0.7);
    cairo_rectangle(cr, 0, 0, GRID_WIDTH * BLOCK_SIZE,
                    GRID_HEIGHT * BLOCK_SIZE);
    cairo_fill(cr);

    // Draw title
    cairo_select_font_face(cr, "Sans", CAIRO_FONT_SLANT_NORMAL,
                           CAIRO_FONT_WEIGHT_BOLD);
    cairo_set_font_size(cr, 40 * BLOCK_SIZE / 47);
    cairo_set_source_rgb(cr, 1, 1, 1);

    // Center the title
    cairo_text_extents_t extents;
    const char *title =
        board->retroModeActive ? "БЛОЧНАЯ РЕВОЛЮЦИЯ" : "TETRIMONE";
    cairo_text_extents(cr, title, &extents);

    double x = (GRID_WIDTH * BLOCK_SIZE - extents.width) / 2;
    double y = (GRID_HEIGHT * BLOCK_SIZE) / 3;

    cairo_move_to(cr, x, y);
    cairo_show_text(cr, title);

    // Draw colored blocks for decoration
    int blockSize = 30;
    int startX = (GRID_WIDTH * BLOCK_SIZE - 4 * BLOCK_SIZE) / 2;
    int startY = y + 20;

    // Draw I piece (cyan)
    cairo_set_source_rgb(cr, 0.0, 0.7, 0.9);
    for (int i = 0; i < 4; i++) {
      cairo_rectangle(cr, startX + i * BLOCK_SIZE, startY, BLOCK_SIZE - 2,
                      BLOCK_SIZE - 2);
      cairo_fill(cr);
    }

    // Draw T piece (purple)
    cairo_set_source_rgb(cr, 0.8, 0.0, 0.8);
    startY += BLOCK_SIZE * 1.5;
    cairo_rectangle(cr, startX + BLOCK_SIZE, startY, BLOCK_SIZE - 2,
                    BLOCK_SIZE - 2);
    cairo_fill(cr);
    cairo_rectangle(cr, startX, startY + BLOCK_SIZE, BLOCK_SIZE - 2,
                    BLOCK_SIZE - 2);
    cairo_fill(cr);
    cairo_rectangle(cr, startX + BLOCK_SIZE, startY + BLOCK_SIZE,
                    BLOCK_SIZE - 2, BLOCK_SIZE - 2);
    cairo_fill(cr);
    cairo_rectangle(cr, startX + BLOCK_SIZE * 2, startY + BLOCK_SIZE,
                    BLOCK_SIZE - 2, BLOCK_SIZE - 2);
    cairo_fill(cr);

    // Draw press space message
    cairo_set_font_size(cr, 20 * BLOCK_SIZE / 47);
    const char *startText = board->retroModeActive ? "Нажмите ПРОБЕЛ для начала"
                                                   : "Press SPACE to Start";
    cairo_text_extents(cr, startText, &extents);

    x = (GRID_WIDTH * BLOCK_SIZE - extents.width) / 2;
    y = (GRID_HEIGHT * BLOCK_SIZE) * 0.75;

    cairo_move_to(cr, x, y);
    cairo_show_text(cr, startText);

    // Draw joystick message if enabled
    if (app->joystickEnabled) {
      cairo_set_font_size(cr, 16 * BLOCK_SIZE / 47);
      const char *joystickText = board->retroModeActive
                                     ? "или Нажмите СТАРТ на контроллере"
                                     : "or Press START on Controller";
      cairo_text_extents(cr, joystickText, &extents);

      x = (GRID_WIDTH * BLOCK_SIZE - extents.width) / 2;
      y += 30;

      cairo_move_to(cr, x, y);
      cairo_show_text(cr, joystickText);
    }

    return FALSE;
  }

  if (board->retroModeActive && board->showPropagandaMessage) {
    // Semi-transparent background for message
    cairo_set_source_rgba(cr, 0.8, 0.0, 0.0, 0.8);

    // Calculate message position - center of screen
    double msgX = (GRID_WIDTH * BLOCK_SIZE) / 2;
    double msgY = (GRID_HEIGHT * BLOCK_SIZE) / 2;

    // Calculate font size based on screen width
    double screenWidth = GRID_WIDTH * BLOCK_SIZE;
    double baseFontSize = screenWidth / 25.0;
    double fontSize = std::max(14.0, std::min(26.0, baseFontSize));

    // Set font size
    cairo_select_font_face(cr, "Sans", CAIRO_FONT_SLANT_NORMAL,
                           CAIRO_FONT_WEIGHT_BOLD);
    cairo_set_font_size(cr, fontSize);

    // Get original message
    std::string originalMessage = board->currentPropagandaMessage;
    std::string formattedMessage = originalMessage;

    // Check if message is too long
    cairo_text_extents_t extents;
    cairo_text_extents(cr, originalMessage.c_str(), &extents);

    // If message is too wide for the screen, add a line break
    if (extents.width > screenWidth - 40) {
      // Find approximate middle position to break the text
      int halfLength = originalMessage.length() / 2;

      // Find the closest space to the middle
      size_t spacePos = originalMessage.rfind(' ', halfLength);
      if (spacePos != std::string::npos) {
        // Replace the space with a newline
        formattedMessage = originalMessage.substr(0, spacePos) + "\n" +
                           originalMessage.substr(spacePos + 1);
      }
    }

    // Get extents of the potentially reformatted message
    cairo_text_extents_t newExtents;
    cairo_text_extents(cr, formattedMessage.c_str(), &newExtents);

    // Make sure the message fits the screen even after reformatting
    if (newExtents.width > screenWidth - 40) {
      // If still too wide, use a smaller font
      fontSize =
          std::max(12.0, fontSize * (screenWidth - 40) / newExtents.width);
      cairo_set_font_size(cr, fontSize);
      cairo_text_extents(cr, formattedMessage.c_str(), &newExtents);
    }

    // Calculate background box size
    int msgPadding = 20;
    double boxHeight = newExtents.height + msgPadding * 2;
    // If there's a newline in the message, make the box taller
    if (formattedMessage.find('\n') != std::string::npos) {
      boxHeight = newExtents.height * 2.5 + msgPadding * 2;
    }

    // Draw message background
    cairo_rectangle(cr, msgX - newExtents.width / 2 - msgPadding,
                    msgY - boxHeight / 2, newExtents.width + msgPadding * 2,
                    boxHeight);
    cairo_fill(cr);

    // Draw white border
    cairo_set_source_rgb(cr, 1.0, 1.0, 1.0);
    cairo_set_line_width(cr, 2.0);
    cairo_rectangle(cr, msgX - newExtents.width / 2 - msgPadding,
                    msgY - boxHeight / 2, newExtents.width + msgPadding * 2,
                    boxHeight);
    cairo_stroke(cr);

    // Draw text in white
    cairo_set_source_rgb(cr, 1.0, 1.0, 1.0);

    if (formattedMessage.find('\n') != std::string::npos) {
      // If message contains a newline, draw as multiple lines
      std::string firstLine =
          formattedMessage.substr(0, formattedMessage.find('\n'));
      std::string secondLine =
          formattedMessage.substr(formattedMessage.find('\n') + 1);

      cairo_text_extents_t firstLineExtents;
      cairo_text_extents(cr, firstLine.c_str(), &firstLineExtents);

      // Draw first line
      cairo_move_to(cr, msgX - firstLineExtents.width / 2, msgY - fontSize / 2);
      cairo_show_text(cr, firstLine.c_str());

      // Draw second line
      cairo_text_extents_t secondLineExtents;
      cairo_text_extents(cr, secondLine.c_str(), &secondLineExtents);
      cairo_move_to(cr, msgX - secondLineExtents.width / 2, msgY + fontSize);
      cairo_show_text(cr, secondLine.c_str());
    } else {
      // Single line display
      cairo_move_to(cr, msgX - newExtents.width / 2,
                    msgY + newExtents.height / 2);
      cairo_show_text(cr, formattedMessage.c_str());
    }
  }

  // Draw current piece with smooth movement animation
  if (!board->isGameOver() && !board->isPaused() &&
      !board->isSplashScreenActive()) {
    const TetrimoneBlock &piece = board->getCurrentPiece();
    auto shape = piece.getShape();
    auto color = board->isInThemeTransition()
                     ? board->getInterpolatedColor(
                           piece.getType(), board->getThemeTransitionProgress())
                     : piece.getColor();

    // Get interpolated position for smooth movement
    double pieceX, pieceY;
    board->getCurrentPieceInterpolatedPosition(pieceX, pieceY);

    cairo_set_source_rgb(cr, color[0], color[1], color[2]);

    for (size_t y = 0; y < shape.size(); ++y) {
      for (size_t x = 0; x < shape[y].size(); ++x) {
        if (shape[y][x] == 1) {
          // Use interpolated position for smooth movement
          double drawX = (pieceX + x) * BLOCK_SIZE;
          double drawY = (pieceY + y) * BLOCK_SIZE;

          // Only draw if within the visible grid
          if (drawY >= -BLOCK_SIZE) {
            if (board->retroModeActive || board->simpleBlocksActive) {
              cairo_rectangle(cr, drawX, drawY, BLOCK_SIZE, BLOCK_SIZE);
              cairo_fill(cr);
            } else {
              // Regular mode with 3D effects
              cairo_rectangle(cr, drawX + 1, drawY + 1, BLOCK_SIZE - 2,
                              BLOCK_SIZE - 2);
              cairo_fill(cr);

              // Draw highlight (3D effect)
              cairo_set_source_rgba(cr, 1, 1, 1, 0.3);
              cairo_move_to(cr, drawX + 1, drawY + 1);
              cairo_line_to(cr, drawX + BLOCK_SIZE - 1, drawY + 1);
              cairo_line_to(cr, drawX + 1, drawY + BLOCK_SIZE - 1);
              cairo_close_path(cr);
              cairo_fill(cr);

              // Draw shadow (3D effect)
              cairo_set_source_rgba(cr, 0, 0, 0, 0.3);
              cairo_move_to(cr, drawX + BLOCK_SIZE - 1, drawY + 1);
              cairo_line_to(cr, drawX + BLOCK_SIZE - 1, drawY + BLOCK_SIZE - 1);
              cairo_line_to(cr, drawX + 1, drawY + BLOCK_SIZE - 1);
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

  // Draw ghost piece with interpolated position
  if (!board->isGameOver() && !board->isPaused() &&
      !board->isSplashScreenActive() && board->isGhostPieceEnabled()) {

    const TetrimoneBlock &piece = board->getCurrentPiece();
    auto shape = piece.getShape();
    auto color = piece.getColor();

    double currentPieceX, currentPieceY;
    board->getCurrentPieceInterpolatedPosition(currentPieceX, currentPieceY);
    int ghostY = board->getGhostPieceY();

    // Only draw ghost if it's in a different position than current piece
    if (ghostY > (int)currentPieceY) {
      // Set semi-transparent color for ghost piece
      cairo_set_source_rgba(cr, color[0], color[1], color[2], 0.3);

      for (size_t y = 0; y < shape.size(); ++y) {
        for (size_t x = 0; x < shape[y].size(); ++x) {
          if (shape[y][x] == 1) {
            double drawX = (currentPieceX + x) * BLOCK_SIZE;
            double drawY = (ghostY + y) * BLOCK_SIZE;

            // Only draw if within the visible grid
            if (drawY >= 0) {
              if (board->retroModeActive) {
                cairo_rectangle(cr, drawX, drawY, BLOCK_SIZE, BLOCK_SIZE);
                cairo_stroke(cr);
              } else {
                cairo_rectangle(cr, drawX + 1, drawY + 1, BLOCK_SIZE - 2,
                                BLOCK_SIZE - 2);
                cairo_stroke_preserve(cr);
                cairo_fill(cr);
              }
            }
          }
        }
      }
    }
  }

  // Draw enhanced pause menu if paused
  if (board->isPaused() && !board->isGameOver()) {
    cairo_set_source_rgba(cr, 0, 0, 0, 0.7);
    cairo_rectangle(cr, 0, 0, GRID_WIDTH * BLOCK_SIZE,
                    GRID_HEIGHT * BLOCK_SIZE);
    cairo_fill(cr);

    cairo_select_font_face(cr, "Sans", CAIRO_FONT_SLANT_NORMAL,
                           CAIRO_FONT_WEIGHT_BOLD);
    cairo_set_font_size(cr, 30);
    cairo_set_source_rgb(cr, 1, 1, 1);

    // Center the text
    cairo_text_extents_t extents;
    const char *text =
        board->retroModeActive ? "ПРИОСТАНОВЛЕНО ПО ПРИКАЗУ ПАРТИИ" : "PAUSED";
    cairo_text_extents(cr, text, &extents);

    double x = (GRID_WIDTH * BLOCK_SIZE - extents.width) / 2;
    double y = (GRID_HEIGHT * BLOCK_SIZE) / 4;

    cairo_move_to(cr, x, y);
    cairo_show_text(cr, text);

    // Draw pause menu options with Soviet bureaucracy names if in retro mode
    const int numOptions = 3;
    const char *menuOptions[numOptions];

    if (board->retroModeActive) {
      menuOptions[0] = "Продолжить Трудовой Подвиг (P)";
      menuOptions[1] = "Новая Пятилетка (N)";
      menuOptions[2] = "Дезертировать с Поля Боя (Q)";
    } else {
      menuOptions[0] = "Continue (P)";
      menuOptions[1] = "New Game (N)";
      menuOptions[2] = "Quit (Q)";
    }

    cairo_set_font_size(cr, 20);

    // Calculate center position
    y = (GRID_HEIGHT * BLOCK_SIZE) / 2;

    // Draw menu options
    for (int i = 0; i < numOptions; i++) {
      cairo_text_extents(cr, menuOptions[i], &extents);
      x = (GRID_WIDTH * BLOCK_SIZE - extents.width) / 2;

      cairo_move_to(cr, x, y);
      cairo_show_text(cr, menuOptions[i]);

      y += 40;
    }
  }

  // Draw game over text if needed
  if (board->isGameOver()) {
    cairo_set_source_rgba(cr, 0, 0, 0, 0.7);
    cairo_rectangle(cr, 0, 0, GRID_WIDTH * BLOCK_SIZE,
                    GRID_HEIGHT * BLOCK_SIZE);
    cairo_fill(cr);
    cairo_select_font_face(cr, "Sans", CAIRO_FONT_SLANT_NORMAL,
                           CAIRO_FONT_WEIGHT_BOLD);
    cairo_set_font_size(cr, 30);
    cairo_set_source_rgb(cr, 1, 0, 0);

    // Center the text
    cairo_text_extents_t extents;
    const char *text =
        board->retroModeActive ? "ИНФОРМАЦИЯ ЗАПРЕЩЕНА" : "GAME OVER";
    cairo_text_extents(cr, text, &extents);
    double x = (GRID_WIDTH * BLOCK_SIZE - extents.width) / 2;
    double y = (GRID_HEIGHT * BLOCK_SIZE) / 2;
    cairo_move_to(cr, x, y);
    cairo_show_text(cr, text);

    // Show different restart message in retro mode
    cairo_set_font_size(cr, 16);
    const char *restartText = board->retroModeActive
                                  ? "ОЖИДАЙТЕ ДОПРОСА. НЕ ДВИГАЙТЕСЬ..."
                                  : "Press R to restart";
    cairo_text_extents(cr, restartText, &extents);
    x = (GRID_WIDTH * BLOCK_SIZE - extents.width) / 2;
    y += 40;
    cairo_move_to(cr, x, y);
    cairo_show_text(cr, restartText);

    // Add a second line with translation for non-Russian speakers
    if (board->retroModeActive) {
      cairo_set_font_size(cr, 12);
      const char *translationText = "(AWAIT INTERROGATION. DO NOT MOVE...)";
      cairo_text_extents(cr, translationText, &extents);
      x = (GRID_WIDTH * BLOCK_SIZE - extents.width) / 2;
      y += 25;
      cairo_move_to(cr, x, y);
      cairo_show_text(cr, translationText);
    }
  }

  if (board->isFireworksActive()) {
    const auto &particles = app->board->getFireworkParticles();

    for (const auto &particle : particles) {
      // Set particle color with alpha based on life
      double alpha = particle.life;
      cairo_set_source_rgba(cr, particle.color[0], particle.color[1],
                            particle.color[2], alpha);

      // Draw particle as a glowing circle
      cairo_arc(cr, particle.x, particle.y, particle.size * particle.life, 0,
                2 * M_PI);
      cairo_fill(cr);

      // Add glow effect
      cairo_set_source_rgba(cr, particle.color[0], particle.color[1],
                            particle.color[2], alpha * 0.3);
      cairo_arc(cr, particle.x, particle.y, particle.size * particle.life * 2,
                0, 2 * M_PI);
      cairo_fill(cr);

      // Add bright center
      cairo_set_source_rgba(cr, 1.0, 1.0, 1.0, alpha * 0.8);
      cairo_arc(cr, particle.x, particle.y, particle.size * particle.life * 0.3,
                0, 2 * M_PI);
      cairo_fill(cr);
    }
  }

  if (board->isTrailsEnabled() && board->isBlockTrailsActive()) {
    const auto &trails = app->board->getBlockTrails();

    for (const auto &trail : trails) {
      // Set color with alpha for fading effect
      auto color = trail.color;
      cairo_set_source_rgba(cr, color[0], color[1], color[2], trail.alpha);

      // Draw each block of the trail piece
      for (size_t y = 0; y < trail.shape.size(); ++y) {
        for (size_t x = 0; x < trail.shape[y].size(); ++x) {
          if (trail.shape[y][x] == 1) {
            double drawX = (trail.x + x) * BLOCK_SIZE;
            double drawY = (trail.y + y) * BLOCK_SIZE;

            // Only draw if within the visible grid
            if (drawY >= -BLOCK_SIZE) {
              if (board->simpleBlocksActive) {
                // Simple trail blocks
                cairo_rectangle(cr, drawX, drawY, BLOCK_SIZE, BLOCK_SIZE);
                cairo_fill(cr);
              } else {
                // 3D trail blocks with reduced effect
                cairo_rectangle(cr, drawX + 1, drawY + 1, BLOCK_SIZE - 2,
                                BLOCK_SIZE - 2);
                cairo_fill(cr);

                // Subtle highlight for 3D effect
                cairo_set_source_rgba(cr, 1, 1, 1, 0.1 * trail.alpha);
                cairo_move_to(cr, drawX + 1, drawY + 1);
                cairo_line_to(cr, drawX + BLOCK_SIZE - 1, drawY + 1);
                cairo_line_to(cr, drawX + 1, drawY + BLOCK_SIZE - 1);
                cairo_close_path(cr);
                cairo_fill(cr);

                // Subtle shadow for 3D effect
                cairo_set_source_rgba(cr, 0, 0, 0, 0.1 * trail.alpha);
                cairo_move_to(cr, drawX + BLOCK_SIZE - 1, drawY + 1);
                cairo_line_to(cr, drawX + BLOCK_SIZE - 1,
                              drawY + BLOCK_SIZE - 1);
                cairo_line_to(cr, drawX + 1, drawY + BLOCK_SIZE - 1);
                cairo_close_path(cr);
                cairo_fill(cr);

                // Reset color for next block
                cairo_set_source_rgba(cr, color[0], color[1], color[2],
                                      trail.alpha);
              }
            }
          }
        }
      }
    }
  }

  return FALSE;
}

void TetrimoneBoard::getCurrentPieceInterpolatedPosition(double &x,
                                                         double &y) const {
  if (currentPiece) {
    if (smoothMovementTimer > 0 && movementProgress < 1.0) {
      // Interpolate between last position and current position
      double t = movementProgress;
      // Use easing function for smoother movement
      t = t * t * (3.0 - 2.0 * t); // Smoothstep function

      x = lastPieceX + (currentPiece->getX() - lastPieceX) * t;
      y = lastPieceY + (currentPiece->getY() - lastPieceY) * t;
    } else {
      x = currentPiece->getX();
      y = currentPiece->getY();
    }
  } else {
    x = 0;
    y = 0;
  }
}

bool TetrimoneBoard::isLineBeingCleared(int y) const {
  return std::find(linesBeingCleared.begin(), linesBeingCleared.end(), y) !=
         linesBeingCleared.end();
}

void TetrimoneBoard::startSmoothMovement(int newX, int newY) {
  if (currentPiece) {
    lastPieceX = currentPiece->getX();
    lastPieceY = currentPiece->getY();
    movementProgress = 0.0;

    // Set start time for this animation
    movementStartTime = std::chrono::high_resolution_clock::now();

    // Only start animation if there's actual movement
    if (lastPieceX != newX || lastPieceY != newY) {
      if (smoothMovementTimer > 0) {
        g_source_remove(smoothMovementTimer);
      }

      smoothMovementTimer = g_timeout_add(
          16, // ~60 FPS
          [](gpointer userData) -> gboolean {
            TetrimoneBoard *board = static_cast<TetrimoneBoard *>(userData);
            board->updateSmoothMovement();

            // FORCE SCREEN REPAINT
            if (board->app) {
              gtk_widget_queue_draw(board->app->gameArea);
            }

            return TRUE;
          },
          this);
    }
  }
}

void TetrimoneBoard::updateSmoothMovement() {
  auto now = std::chrono::high_resolution_clock::now();
  auto totalMs =
      std::chrono::duration<double, std::milli>(now - movementStartTime)
          .count();

  movementProgress = totalMs / MOVEMENT_ANIMATION_DURATION;

  if (movementProgress >= 1.0) {
    movementProgress = 1.0;
    if (smoothMovementTimer > 0) {
      g_source_remove(smoothMovementTimer);
      smoothMovementTimer = 0;
    }
  }
}

void TetrimoneBoard::startLineClearAnimation(
    const std::vector<int> &clearedLines) {
  linesBeingCleared = clearedLines;
  lineClearActive = true;
  lineClearProgress = 0.0;

  // Set start time for this animation
  lineClearStartTime = std::chrono::high_resolution_clock::now();

  // Randomly select animation type for modern mode (0-9)
  if (!retroModeActive) {
    std::uniform_int_distribution<int> animDist(0, 9);
    currentAnimationType = animDist(rng);
  }

  if (lineClearAnimationTimer > 0) {
    g_source_remove(lineClearAnimationTimer);
  }

  lineClearAnimationTimer = g_timeout_add(
      16, // ~60 FPS
      [](gpointer userData) -> gboolean {
        TetrimoneBoard *board = static_cast<TetrimoneBoard *>(userData);
        board->updateLineClearAnimation();

        // FORCE SCREEN REPAINT
        if (board->app) {
          gtk_widget_queue_draw(board->app->gameArea);
        }

        return TRUE;
      },
      this);
}

void TetrimoneBoard::updateLineClearAnimation() {
  auto now = std::chrono::high_resolution_clock::now();
  auto totalMs =
      std::chrono::duration<double, std::milli>(now - lineClearStartTime)
          .count();

  lineClearProgress = totalMs / LINE_CLEAR_ANIMATION_DURATION;

  if (lineClearProgress >= 1.0) {
    // Animation complete - stop timer first
    if (lineClearAnimationTimer > 0) {
      g_source_remove(lineClearAnimationTimer);
      lineClearAnimationTimer = 0;
    }

    // Set progress to exactly 1.0
    lineClearProgress = 1.0;

    // Sort lines in descending order (bottom to top) to remove correctly
    std::vector<int> sortedLines = linesBeingCleared;
    std::sort(sortedLines.begin(), sortedLines.end(), std::greater<int>());

    // Remove the lines from the grid (bottom to top)
    for (int lineY : sortedLines) {
      // Verify this line actually needs to be cleared
      bool lineIsFull = true;
      for (int x = 0; x < GRID_WIDTH; ++x) {
        if (grid[lineY][x] == 0) {
          lineIsFull = false;
          break;
        }
      }

      // Only remove if line is actually full
      if (lineIsFull) {
        // Move all rows above this line down by one
        for (int moveY = lineY; moveY > 0; --moveY) {
          for (int x = 0; x < GRID_WIDTH; ++x) {
            grid[moveY][x] = grid[moveY - 1][x];
          }
        }

        // Clear the top row
        for (int x = 0; x < GRID_WIDTH; ++x) {
          grid[0][x] = 0;
        }

        // After removing this line, adjust positions of remaining lines
        for (int &otherLineY : sortedLines) {
          if (otherLineY < lineY) {
            otherLineY++;
          }
        }
      }
    }

    // Clean up animation state
    linesBeingCleared.clear();
    lineClearActive = false;
    lineClearProgress = 0.0;
  }
}

gboolean onDrawNextPiece(GtkWidget *widget, cairo_t *cr, gpointer data) {
  TetrimoneApp *app = static_cast<TetrimoneApp *>(data);
  TetrimoneBoard *board = app->board;

  // Get widget dimensions
  GtkAllocation allocation;
  gtk_widget_get_allocation(widget, &allocation);

  // Draw background
  cairo_set_source_rgb(cr, 0.1, 0.1, 0.1);
  cairo_rectangle(cr, 0, 0, allocation.width, allocation.height);
  cairo_fill(cr);

  if (!board->isGameOver()) {
    // Calculate section width - each piece gets exactly 1/3 of the total width
    int sectionWidth = allocation.width / 3;

    // Calculate preview block size (half of the normal block size)
    int previewBlockSize = BLOCK_SIZE / 2;

    // Draw dividers between sections
    cairo_set_source_rgb(cr, 0.3, 0.3, 0.3);
    cairo_set_line_width(cr, 2);
    cairo_move_to(cr, sectionWidth, 0);
    cairo_line_to(cr, sectionWidth, allocation.height);
    cairo_move_to(cr, sectionWidth * 2, 0);
    cairo_line_to(cr, sectionWidth * 2, allocation.height);
    cairo_stroke(cr);

    // Process each piece
    for (int pieceIndex = 0; pieceIndex < 3; pieceIndex++) {
      // Calculate the section's X position
      int sectionX = pieceIndex * sectionWidth;

      // Reserve space for the header
      int headerHeight = 25;

      // Get the piece information
      const TetrimoneBlock &piece = board->getNextPiece(pieceIndex);
      auto shape = piece.getShape();
      auto color = piece.getColor();

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
      int offsetY = headerHeight + (allocation.height - headerHeight -
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
              cairo_rectangle(cr, drawX, drawY, previewBlockSize,
                              previewBlockSize);
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

  return FALSE;
}

void TetrimoneBoard::startThemeTransition(int targetTheme) {
  // Don't start transition if already transitioning to the same theme
  if (isThemeTransitioning && newThemeIndex == targetTheme) {
    return;
  }

  // Cancel any existing transition
  if (themeTransitionTimer > 0) {
    g_source_remove(themeTransitionTimer);
  }

  // Set up transition
  oldThemeIndex = currentThemeIndex;
  newThemeIndex = targetTheme;
  isThemeTransitioning = true;
  themeTransitionProgress = 0.0;

  // Set start time for this animation
  themeStartTime = std::chrono::high_resolution_clock::now();

  themeTransitionTimer = g_timeout_add(
      16, // ~60 FPS
      [](gpointer userData) -> gboolean {
        TetrimoneBoard *board = static_cast<TetrimoneBoard *>(userData);
        board->updateThemeTransition();

        // FORCE SCREEN REPAINT
        if (board->app) {
          gtk_widget_queue_draw(board->app->gameArea);
          gtk_widget_queue_draw(
              board->app->nextPieceArea); // For theme color changes
        }

        return TRUE;
      },
      this);
}

void TetrimoneBoard::updateThemeTransition() {
  auto now = std::chrono::high_resolution_clock::now();
  auto totalMs =
      std::chrono::duration<double, std::milli>(now - themeStartTime).count();

  themeTransitionProgress = totalMs / THEME_TRANSITION_DURATION;

  if (themeTransitionProgress >= 1.0) {
    // Transition complete
    themeTransitionProgress = 1.0;
    currentThemeIndex = newThemeIndex;

    // Clean up
    if (themeTransitionTimer > 0) {
      g_source_remove(themeTransitionTimer);
      themeTransitionTimer = 0;
    }

    isThemeTransitioning = false;
  }
}

void TetrimoneBoard::cancelThemeTransition() {
  if (themeTransitionTimer > 0) {
    g_source_remove(themeTransitionTimer);
    themeTransitionTimer = 0;
  }

  isThemeTransitioning = false;
  themeTransitionProgress = 0.0;
}

std::array<double, 3>
TetrimoneBoard::getInterpolatedColor(int blockType, double progress) const {
  if (!isThemeTransitioning) {
    return TETRIMONEBLOCK_COLOR_THEMES[currentThemeIndex][blockType];
  }

  // Get colors from old and new themes
  auto oldColor = TETRIMONEBLOCK_COLOR_THEMES[oldThemeIndex][blockType];
  auto newColor = TETRIMONEBLOCK_COLOR_THEMES[newThemeIndex][blockType];

  // Use a more gentle easing function for the longer transition
  // Cubic ease-in-out for smoother, more noticeable transitions
  double t = themeTransitionProgress;
  if (t < 0.5) {
    t = 4 * t * t * t; // Ease-in cubic
  } else {
    t = 1 - pow(-2 * t + 2, 3) / 2; // Ease-out cubic
  }

  // Interpolate between colors
  std::array<double, 3> interpolatedColor;
  for (int i = 0; i < 3; i++) {
    interpolatedColor[i] = oldColor[i] + (newColor[i] - oldColor[i]) * t;
  }

  return interpolatedColor;
}
