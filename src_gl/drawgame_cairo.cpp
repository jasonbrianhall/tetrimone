#include "tetrimone.h"
#include "audiomanager.h"
#include <iostream>
#include <string>
#include <algorithm>
#ifdef _WIN32
#include <windows.h>
#include <commdlg.h>
#endif
#include "highscores.h"
#include "propaganda_messages.h"
#include "zip.h"

// Define M_PI for Windows compatibility
#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

void drawSplashScreen(cairo_t *cr, TetrimoneBoard *board, TetrimoneApp *app) {
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
  const char *title = board->retroModeActive ? 
      "БЛОЧНАЯ РЕВОЛЮЦИЯ" : "TETRIMONE";
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
  const char *startText = board->retroModeActive ? 
      "Нажмите ПРОБЕЛ для начала" : "Press SPACE to Start";
  cairo_text_extents(cr, startText, &extents);

  x = (GRID_WIDTH * BLOCK_SIZE - extents.width) / 2;
  y = (GRID_HEIGHT * BLOCK_SIZE) * 0.75;

  cairo_move_to(cr, x, y);
  cairo_show_text(cr, startText);

  // Draw joystick message if enabled
  if (app->joystickEnabled) {
    cairo_set_font_size(cr, 16 * BLOCK_SIZE / 47);
    const char *joystickText = board->retroModeActive ? 
        "или Нажмите СТАРТ на контроллере" : 
        "or Press START on Controller";
    cairo_text_extents(cr, joystickText, &extents);

    x = (GRID_WIDTH * BLOCK_SIZE - extents.width) / 2;
    y += 30;

    cairo_move_to(cr, x, y);
    cairo_show_text(cr, joystickText);
  }
}

void drawBackground(cairo_t *cr, TetrimoneBoard *board, const GtkAllocation &allocation) {
  // Draw solid background color
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
}

void drawGridLines(cairo_t *cr, TetrimoneBoard *board) {
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
}

// Animation value struct
struct LineClearAnimValues {
  double alpha;
  double scale;
  double offsetX;
  double offsetY;
};

// Animation 0: Classic Shrink & Scatter
LineClearAnimValues anim0_ClassicShrinkScatter(double progress, int x, int y) {
  LineClearAnimValues result = {1.0, 1.0, 0.0, 0.0};
  
  if (progress < 0.2) {
    result.alpha = 0.4 + 0.6 * sin(progress * 25.0);
    result.scale = 1.0;
  } else if (progress < 0.6) {
    double scaleProgress = (progress - 0.2) / 0.4;
    result.scale = 1.0 - scaleProgress * 0.8;
    result.alpha = 1.0 - scaleProgress * 0.4;
  } else {
    double fadeProgress = (progress - 0.6) / 0.4;
    result.alpha = (1.0 - fadeProgress * 0.6) * 0.6;
    result.scale = 0.2 - fadeProgress * 0.2;
    result.offsetX = (rand() % 8 - 4) * fadeProgress * 3;
    result.offsetY = (rand() % 8 - 4) * fadeProgress * 3;
  }
  
  return result;
}

// Animation 1: Dissolve (blocks fade randomly)
LineClearAnimValues anim1_Dissolve(double progress, int x, int y) {
  LineClearAnimValues result = {1.0, 1.0, 0.0, 0.0};
  
  int blockSeed = (x * 31 + y * 17) % 100;
  double blockDelay = (blockSeed / 100.0) * 0.3;
  
  if (progress < blockDelay) {
    result.alpha = 1.0;
    result.scale = 1.0;
  } else if (progress < blockDelay + 0.4) {
    double dissolveProgress = (progress - blockDelay) / 0.4;
    result.alpha = 1.0 - dissolveProgress;
    result.scale = 1.0 - dissolveProgress * 0.3;
  } else {
    result.alpha = 0.0;
    result.scale = 0.7;
  }
  
  return result;
}

// Animation 2: Ripple Wave (from center outward)
LineClearAnimValues anim2_RippleWave(double progress, int x, int y) {
  LineClearAnimValues result = {1.0, 1.0, 0.0, 0.0};
  
  double centerX = GRID_WIDTH / 2.0;
  double distance = abs(x - centerX) / centerX;
  double waveDelay = distance * 0.3;
  
  if (progress < waveDelay) {
    result.alpha = 1.0;
    result.scale = 1.0;
  } else if (progress < waveDelay + 0.5) {
    double waveProgress = (progress - waveDelay) / 0.5;
    result.alpha = 1.0 - waveProgress;
    result.scale = 1.0 + sin(waveProgress * M_PI) * 0.3;
    result.offsetY = sin(waveProgress * M_PI * 2) * 5;
  } else {
    result.alpha = 0.0;
    result.scale = 0.0;
  }
  
  return result;
}

// Animation 3: Explosion (blocks shoot outward)
LineClearAnimValues anim3_Explosion(double progress, int x, int y) {
  LineClearAnimValues result = {1.0, 1.0, 0.0, 0.0};
  
  if (progress < 0.15) {
    result.alpha = 1.0;
    result.scale = 1.0 + progress * 2;
  } else if (progress < 0.6) {
    double explodeProgress = (progress - 0.15) / 0.45;
    result.alpha = 1.0 - explodeProgress * 0.7;
    result.scale = 1.3 - explodeProgress * 0.5;
    
    double centerX = GRID_WIDTH / 2.0;
    double forceX = (x - centerX) * explodeProgress * 15;
    double forceY = -explodeProgress * 20;
    result.offsetX = forceX;
    result.offsetY = forceY;
  } else {
    double finalProgress = (progress - 0.6) / 0.4;
    result.alpha = 0.3 - finalProgress * 0.3;
    result.scale = 0.8 - finalProgress * 0.8;
    
    double centerX = GRID_WIDTH / 2.0;
    double forceX = (x - centerX) * (1.0 + finalProgress) * 15;
    double forceY = -20 + finalProgress * 40;
    result.offsetX = forceX;
    result.offsetY = forceY;
  }
  
  return result;
}

// Animation 4: Spin & Vanish
LineClearAnimValues anim4_SpinVanish(double progress, int x, int y) {
  LineClearAnimValues result = {1.0, 1.0, 0.0, 0.0};
  
  if (progress < 0.3) {
    result.alpha = 1.0;
    result.scale = 1.0;
    double rotationSpeed = progress * 20;
    result.offsetX = sin(rotationSpeed) * 3;
    result.offsetY = cos(rotationSpeed) * 3;
  } else if (progress < 0.8) {
    double spinProgress = (progress - 0.3) / 0.5;
    result.alpha = 1.0 - spinProgress * 0.8;
    result.scale = 1.0 - spinProgress * 0.7;
    
    double rotationSpeed = (progress * 40) + (spinProgress * 60);
    result.offsetX = sin(rotationSpeed) * (3 - spinProgress * 3);
    result.offsetY = cos(rotationSpeed) * (3 - spinProgress * 3);
  } else {
    double finalProgress = (progress - 0.8) / 0.2;
    result.alpha = 0.2 - finalProgress * 0.2;
    result.scale = 0.3 - finalProgress * 0.3;
    result.offsetX = 0;
    result.offsetY = 0;
  }
  
  return result;
}

// Animation 5: Left-to-Right Sweep
LineClearAnimValues anim5_LeftToRightSweep(double progress, int x, int y) {
  LineClearAnimValues result = {1.0, 1.0, 0.0, 0.0};
  
  double sweepProgress = progress;
  double blockDelay = (x / (double)GRID_WIDTH) * 0.4; // Spread over 40% of animation
  
  if (sweepProgress < blockDelay) {
    result.alpha = 1.0;
    result.scale = 1.0;
  } else if (sweepProgress < blockDelay + 0.3) {
    double localProgress = (sweepProgress - blockDelay) / 0.3;
    result.alpha = 1.0 - localProgress;
    result.scale = 1.0 - localProgress * 0.6;
    result.offsetX = localProgress * 20; // Slide right as they fade
  } else {
    result.alpha = 0.0;
    result.scale = 0.4;
    result.offsetX = 20;
  }
  
  return result;
}

// Animation 6: Bounce & Pop
LineClearAnimValues anim6_BouncePop(double progress, int x, int y) {
  LineClearAnimValues result = {1.0, 1.0, 0.0, 0.0};
  
  if (progress < 0.2) {
    result.alpha = 1.0;
    result.scale = 1.0 + sin(progress * 15) * 0.2; // Quick bounce
  } else if (progress < 0.5) {
    double bounceProgress = (progress - 0.2) / 0.3;
    result.alpha = 1.0;
    result.scale = 1.0 + bounceProgress * 0.8; // Inflate
    result.offsetY = -bounceProgress * 10; // Lift up
  } else if (progress < 0.7) {
    double popProgress = (progress - 0.5) / 0.2;
    result.alpha = 1.0 - popProgress * 0.9;
    result.scale = 1.8 + popProgress * 0.5; // Pop bigger
    result.offsetY = -10 - popProgress * 5;
  } else {
    double fadeProgress = (progress - 0.7) / 0.3;
    result.alpha = 0.1 - fadeProgress * 0.1;
    result.scale = 2.3 - fadeProgress * 2.3;
    result.offsetY = -15;
  }
  
  return result;
}

// Animation 7: Melt Down
LineClearAnimValues anim7_MeltDown(double progress, int x, int y) {
  LineClearAnimValues result = {1.0, 1.0, 0.0, 0.0};
  
  if (progress < 0.3) {
    result.alpha = 1.0;
    result.scale = 1.0;
    result.offsetY = progress * 5; // Start sinking slightly
  } else if (progress < 0.7) {
    double meltProgress = (progress - 0.3) / 0.4;
    result.alpha = 1.0 - meltProgress * 0.6;
    result.scale = 1.0; // Keep width
    result.offsetY = 5 + meltProgress * 15; // Sink down
  } else {
    double finalProgress = (progress - 0.7) / 0.3;
    result.alpha = 0.4 - finalProgress * 0.4;
    result.scale = 1.0;
    result.offsetY = 20 + finalProgress * 10;
  }
  
  return result;
}

// Animation 8: Zigzag Wipe
LineClearAnimValues anim8_ZigzagWipe(double progress, int x, int y) {
  LineClearAnimValues result = {1.0, 1.0, 0.0, 0.0};
  
  // Create zigzag pattern based on x position
  int zigzagOffset = (x % 4 < 2) ? 0 : 2; // Alternating pattern every 2 blocks
  double zigzagDelay = ((x + zigzagOffset) / (double)GRID_WIDTH) * 0.5;
  
  if (progress < zigzagDelay) {
    result.alpha = 1.0;
    result.scale = 1.0;
  } else if (progress < zigzagDelay + 0.4) {
    double wipeProgress = (progress - zigzagDelay) / 0.4;
    result.alpha = 1.0 - wipeProgress;
    result.scale = 1.0 - wipeProgress * 0.8;
    
    // Zigzag motion
    result.offsetX = sin(wipeProgress * M_PI * 4) * 8;
    result.offsetY = wipeProgress * 12;
  } else {
    result.alpha = 0.0;
    result.scale = 0.2;
  }
  
  return result;
}

// Animation 9: Fireworks Burst
LineClearAnimValues anim9_FireworksBurst(double progress, int x, int y) {
  LineClearAnimValues result = {1.0, 1.0, 0.0, 0.0};
  
  if (progress < 0.1) {
    result.alpha = 1.0;
    result.scale = 1.0;
  } else if (progress < 0.25) {
    double chargeProgress = (progress - 0.1) / 0.15;
    result.alpha = 1.0;
    result.scale = 1.0 - chargeProgress * 0.3; // Compress before burst
    result.offsetY = -chargeProgress * 8;
  } else if (progress < 0.4) {
    double burstProgress = (progress - 0.25) / 0.15;
    result.alpha = 1.0;
    result.scale = 0.7 + burstProgress * 1.0; // Sudden expansion
    result.offsetY = -8 + burstProgress * 3;
  } else if (progress < 0.8) {
    double sparkleProgress = (progress - 0.4) / 0.4;
    result.alpha = 1.0 - sparkleProgress * 0.7;
    result.scale = 1.7 - sparkleProgress * 0.9;
    
    // Sparkle effect - random small movements
    int sparkSeed = (x * 23 + y * 41 + (int)(progress * 100)) % 100;
    result.offsetX = (sparkSeed % 20 - 10) * sparkleProgress * 0.8;
    result.offsetY = -5 + ((sparkSeed / 20) % 20 - 10) * sparkleProgress * 0.8;
  } else {
    double fadeProgress = (progress - 0.8) / 0.2;
    result.alpha = 0.3 - fadeProgress * 0.3;
    result.scale = 0.8 - fadeProgress * 0.8;
    result.offsetX = 0;
    result.offsetY = 0;
  }
  
  return result;
}

// Get animation values based on animation type
LineClearAnimValues getLineClearAnimationValues(int animationType, double progress, int x, int y) {
  switch (animationType) {
    case 0: return anim0_ClassicShrinkScatter(progress, x, y);
    case 1: return anim1_Dissolve(progress, x, y);
    case 2: return anim2_RippleWave(progress, x, y);
    case 3: return anim3_Explosion(progress, x, y);
    case 4: return anim4_SpinVanish(progress, x, y);
    case 5: return anim5_LeftToRightSweep(progress, x, y);
    case 6: return anim6_BouncePop(progress, x, y);
    case 7: return anim7_MeltDown(progress, x, y);
    case 8: return anim8_ZigzagWipe(progress, x, y);
    case 9: return anim9_FireworksBurst(progress, x, y);
    default: return {1.0, 1.0, 0.0, 0.0};
  }
}

void drawGameOver(cairo_t *cr, TetrimoneBoard *board) {
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
    const char *text = board->retroModeActive ? "ИНФОРМАЦИЯ ЗАПРЕЩЕНА" : "GAME OVER";
    cairo_text_extents(cr, text, &extents);
    double x = (GRID_WIDTH * BLOCK_SIZE - extents.width) / 2;
    double y = (GRID_HEIGHT * BLOCK_SIZE) / 2;
    cairo_move_to(cr, x, y);
    cairo_show_text(cr, text);
    
    // Show different restart message in retro mode
    cairo_set_font_size(cr, 16);
    const char *restartText = board->retroModeActive ? 
                              "ОЖИДАЙТЕ ДОПРОСА. НЕ ДВИГАЙТЕСЬ..." : 
                              "Press R to restart";
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
}

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
}

void drawFireworks(cairo_t *cr, TetrimoneBoard *board, TetrimoneApp *app) {
  if (board->isFireworksActive()) {
    const auto& particles = app->board->getFireworkParticles();
    
    for (const auto& particle : particles) {
        // Set particle color with alpha based on life
        double alpha = particle.life;
        cairo_set_source_rgba(cr, particle.color[0], particle.color[1], particle.color[2], alpha);
        
        // Draw particle as a glowing circle
        cairo_arc(cr, particle.x, particle.y, particle.size * particle.life, 0, 2 * M_PI);
        cairo_fill(cr);
        
        // Add glow effect
        cairo_set_source_rgba(cr, particle.color[0], particle.color[1], particle.color[2], alpha * 0.3);
        cairo_arc(cr, particle.x, particle.y, particle.size * particle.life * 2, 0, 2 * M_PI);
        cairo_fill(cr);
        
        // Add bright center
        cairo_set_source_rgba(cr, 1.0, 1.0, 1.0, alpha * 0.8);
        cairo_arc(cr, particle.x, particle.y, particle.size * particle.life * 0.3, 0, 2 * M_PI);
        cairo_fill(cr);
    }
  }
}

void drawBlockTrails(cairo_t *cr, TetrimoneBoard *board) {
  if (board->isTrailsEnabled() && board->isBlockTrailsActive()) {
    const auto& trails = board->getBlockTrails();
    
    for (const auto& trail : trails) {
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
                            cairo_rectangle(cr, drawX + 1, drawY + 1, BLOCK_SIZE - 2, BLOCK_SIZE - 2);
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
                            cairo_line_to(cr, drawX + BLOCK_SIZE - 1, drawY + BLOCK_SIZE - 1);
                            cairo_line_to(cr, drawX + 1, drawY + BLOCK_SIZE - 1);
                            cairo_close_path(cr);
                            cairo_fill(cr);
                            
                            // Reset color for next block
                            cairo_set_source_rgba(cr, color[0], color[1], color[2], trail.alpha);
                        }
                    }
                }
            }
        }
    }
  }
}

void drawFailureLine(cairo_t *cr) {
  int failureLineY = 2;
  cairo_set_source_rgb(cr, 1.0, 0.2, 0.2);
  cairo_set_line_width(cr, 1.0);
  cairo_move_to(cr, 0, failureLineY * BLOCK_SIZE);
  cairo_line_to(cr, GRID_WIDTH * BLOCK_SIZE, failureLineY * BLOCK_SIZE);
  cairo_stroke(cr);
}

void drawPropagandaMessage(cairo_t *cr, TetrimoneBoard *board) {
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
    cairo_select_font_face(cr, "Sans", CAIRO_FONT_SLANT_NORMAL, CAIRO_FONT_WEIGHT_BOLD);
    cairo_set_font_size(cr, fontSize);
    
    // Get original message
    std::string originalMessage = board->currentPropagandaMessage;
    std::string formattedMessage = originalMessage;
    
    // Check if message is too long
    cairo_text_extents_t extents;
    cairo_text_extents(cr, originalMessage.c_str(), &extents);
    
    // If message is too wide for the screen, add a line break
    if (extents.width > screenWidth - 40) {
        int halfLength = originalMessage.length() / 2;
        size_t spacePos = originalMessage.rfind(' ', halfLength);
        if (spacePos != std::string::npos) {
            formattedMessage = originalMessage.substr(0, spacePos) + 
                              "\n" + 
                              originalMessage.substr(spacePos + 1);
        }
    }
    
    // Get extents of the potentially reformatted message
    cairo_text_extents_t newExtents;
    cairo_text_extents(cr, formattedMessage.c_str(), &newExtents);
    
    // Make sure the message fits the screen even after reformatting
    if (newExtents.width > screenWidth - 40) {
        fontSize = std::max(12.0, fontSize * (screenWidth - 40) / newExtents.width);
        cairo_set_font_size(cr, fontSize);
        cairo_text_extents(cr, formattedMessage.c_str(), &newExtents);
    }
    
    // Calculate background box size
    int msgPadding = 20;
    double boxHeight = newExtents.height + msgPadding*2;
    if (formattedMessage.find('\n') != std::string::npos) {
        boxHeight = newExtents.height * 2.5 + msgPadding*2;
    }
    
    // Draw message background
    cairo_rectangle(cr, msgX - newExtents.width/2 - msgPadding,
                    msgY - boxHeight/2,
                    newExtents.width + msgPadding*2,
                    boxHeight);
    cairo_fill(cr);
    
    // Draw white border
    cairo_set_source_rgb(cr, 1.0, 1.0, 1.0);
    cairo_set_line_width(cr, 2.0);
    cairo_rectangle(cr, msgX - newExtents.width/2 - msgPadding,
                    msgY - boxHeight/2,
                    newExtents.width + msgPadding*2,
                    boxHeight);
    cairo_stroke(cr);
    
    // Draw text in white
    cairo_set_source_rgb(cr, 1.0, 1.0, 1.0);
    
    if (formattedMessage.find('\n') != std::string::npos) {
        std::string firstLine = formattedMessage.substr(0, formattedMessage.find('\n'));
        std::string secondLine = formattedMessage.substr(formattedMessage.find('\n') + 1);
        
        cairo_text_extents_t firstLineExtents;
        cairo_text_extents(cr, firstLine.c_str(), &firstLineExtents);
        
        cairo_move_to(cr, msgX - firstLineExtents.width/2, msgY - fontSize/2);
        cairo_show_text(cr, firstLine.c_str());
        
        cairo_text_extents_t secondLineExtents;
        cairo_text_extents(cr, secondLine.c_str(), &secondLineExtents);
        cairo_move_to(cr, msgX - secondLineExtents.width/2, msgY + fontSize);
        cairo_show_text(cr, secondLine.c_str());
    } else {
        cairo_move_to(cr, msgX - newExtents.width/2, msgY + newExtents.height/2);
        cairo_show_text(cr, formattedMessage.c_str());
    }
  }
  else if (board->patrioticModeActive && board->showPropagandaMessage) {
    // Patriotic blue background
    cairo_set_source_rgba(cr, 0.0, 0.2, 0.7, 0.85);
    
    double msgX = (GRID_WIDTH * BLOCK_SIZE) / 2;
    double msgY = (GRID_HEIGHT * BLOCK_SIZE) / 2;
    double screenWidth = GRID_WIDTH * BLOCK_SIZE;
    double baseFontSize = screenWidth / 25.0;
    double fontSize = std::max(14.0, std::min(26.0, baseFontSize));
    
    cairo_select_font_face(cr, "Sans", CAIRO_FONT_SLANT_NORMAL, CAIRO_FONT_WEIGHT_BOLD);
    cairo_set_font_size(cr, fontSize);
    
    std::string originalMessage = board->currentPropagandaMessage;
    std::string formattedMessage = originalMessage;
    
    cairo_text_extents_t extents;
    cairo_text_extents(cr, originalMessage.c_str(), &extents);
    
    if (extents.width > screenWidth - 40) {
        int halfLength = originalMessage.length() / 2;
        size_t spacePos = originalMessage.rfind(' ', halfLength);
        if (spacePos != std::string::npos) {
            formattedMessage = originalMessage.substr(0, spacePos) + 
                              "\n" + 
                              originalMessage.substr(spacePos + 1);
        }
    }
    
    cairo_text_extents_t newExtents;
    cairo_text_extents(cr, formattedMessage.c_str(), &newExtents);
    
    if (newExtents.width > screenWidth - 40) {
        fontSize = std::max(12.0, fontSize * (screenWidth - 40) / newExtents.width);
        cairo_set_font_size(cr, fontSize);
        cairo_text_extents(cr, formattedMessage.c_str(), &newExtents);
    }
    
    int msgPadding = 25;
    double boxHeight = newExtents.height + msgPadding*2;
    if (formattedMessage.find('\n') != std::string::npos) {
        boxHeight = newExtents.height * 2.5 + msgPadding*2;
    }
    
    double cornerRadius = 8.0;
    double boxX = msgX - newExtents.width/2 - msgPadding;
    double boxY = msgY - boxHeight/2;
    double boxWidth = newExtents.width + msgPadding*2;
    
    // Draw rounded rectangle
    cairo_new_sub_path(cr);
    cairo_arc(cr, boxX + cornerRadius, boxY + cornerRadius, cornerRadius, M_PI, 3 * M_PI / 2);
    cairo_arc(cr, boxX + boxWidth - cornerRadius, boxY + cornerRadius, cornerRadius, 3 * M_PI / 2, 0);
    cairo_arc(cr, boxX + boxWidth - cornerRadius, boxY + boxHeight - cornerRadius, cornerRadius, 0, M_PI / 2);
    cairo_arc(cr, boxX + cornerRadius, boxY + boxHeight - cornerRadius, cornerRadius, M_PI / 2, M_PI);
    cairo_close_path(cr);
    cairo_fill(cr);
    
    // Draw patriotic border
    cairo_set_line_width(cr, 3.0);
    cairo_set_source_rgb(cr, 0.8, 0.0, 0.0);
    cairo_new_sub_path(cr);
    cairo_arc(cr, boxX + cornerRadius, boxY + cornerRadius, cornerRadius, M_PI, 3 * M_PI / 2);
    cairo_arc(cr, boxX + boxWidth - cornerRadius, boxY + cornerRadius, cornerRadius, 3 * M_PI / 2, 0);
    cairo_arc(cr, boxX + boxWidth - cornerRadius, boxY + boxHeight - cornerRadius, cornerRadius, 0, M_PI / 2);
    cairo_arc(cr, boxX + cornerRadius, boxY + boxHeight - cornerRadius, cornerRadius, M_PI / 2, M_PI);
    cairo_close_path(cr);
    cairo_stroke(cr);
    
    cairo_set_source_rgb(cr, 1.0, 1.0, 1.0);
    cairo_set_line_width(cr, 1.5);
    cairo_new_sub_path(cr);
    cairo_arc(cr, boxX + cornerRadius, boxY + cornerRadius, cornerRadius, M_PI, 3 * M_PI / 2);
    cairo_arc(cr, boxX + boxWidth - cornerRadius, boxY + cornerRadius, cornerRadius, 3 * M_PI / 2, 0);
    cairo_arc(cr, boxX + boxWidth - cornerRadius, boxY + boxHeight - cornerRadius, cornerRadius, 0, M_PI / 2);
    cairo_arc(cr, boxX + cornerRadius, boxY + boxHeight - cornerRadius, cornerRadius, M_PI / 2, M_PI);
    cairo_close_path(cr);
    cairo_stroke(cr);
    
    // Draw text with shadow
    cairo_set_source_rgba(cr, 0.0, 0.0, 0.0, 0.3);
    
    if (formattedMessage.find('\n') != std::string::npos) {
        std::string firstLine = formattedMessage.substr(0, formattedMessage.find('\n'));
        std::string secondLine = formattedMessage.substr(formattedMessage.find('\n') + 1);
        
        cairo_text_extents_t firstLineExtents;
        cairo_text_extents(cr, firstLine.c_str(), &firstLineExtents);
        
        cairo_move_to(cr, msgX - firstLineExtents.width/2 + 1, msgY - fontSize/2 + 1);
        cairo_show_text(cr, firstLine.c_str());
        
        cairo_text_extents_t secondLineExtents;
        cairo_text_extents(cr, secondLine.c_str(), &secondLineExtents);
        cairo_move_to(cr, msgX - secondLineExtents.width/2 + 1, msgY + fontSize + 1);
        cairo_show_text(cr, secondLine.c_str());
    } else {
        cairo_move_to(cr, msgX - newExtents.width/2 + 1, msgY + newExtents.height/2 + 1);
        cairo_show_text(cr, formattedMessage.c_str());
    }
    
    // Draw main text
    cairo_set_source_rgb(cr, 1.0, 1.0, 1.0);
    
    if (formattedMessage.find('\n') != std::string::npos) {
        std::string firstLine = formattedMessage.substr(0, formattedMessage.find('\n'));
        std::string secondLine = formattedMessage.substr(formattedMessage.find('\n') + 1);
        
        cairo_text_extents_t firstLineExtents;
        cairo_text_extents(cr, firstLine.c_str(), &firstLineExtents);
        
        cairo_move_to(cr, msgX - firstLineExtents.width/2, msgY - fontSize/2);
        cairo_show_text(cr, firstLine.c_str());
        
        cairo_text_extents_t secondLineExtents;
        cairo_text_extents(cr, secondLine.c_str(), &secondLineExtents);
        cairo_move_to(cr, msgX - secondLineExtents.width/2, msgY + fontSize);
        cairo_show_text(cr, secondLine.c_str());
    } else {
        cairo_move_to(cr, msgX - newExtents.width/2, msgY + newExtents.height/2);
        cairo_show_text(cr, formattedMessage.c_str());
    }
  }
}

void drawCurrentPiece(cairo_t *cr, TetrimoneBoard *board) {
  if (!board->isGameOver() && !board->isPaused() && !board->isSplashScreenActive()) {
    const TetrimoneBlock &piece = board->getCurrentPiece();
    auto shape = piece.getShape();
    auto color = board->isInThemeTransition() ? 
    board->getInterpolatedColor(piece.getType(), board->getThemeTransitionProgress()) :
    piece.getColor();
    
    double pieceX, pieceY;
    board->getCurrentPieceInterpolatedPosition(pieceX, pieceY);

    cairo_set_source_rgb(cr, color[0], color[1], color[2]);

    for (size_t y = 0; y < shape.size(); ++y) {
      for (size_t x = 0; x < shape[y].size(); ++x) {
        if (shape[y][x] == 1) {
          double drawX = (pieceX + x) * BLOCK_SIZE;
          double drawY = (pieceY + y) * BLOCK_SIZE;

          if (drawY >= -BLOCK_SIZE) {
            if (board->retroModeActive || board->simpleBlocksActive) {
              cairo_rectangle(cr, drawX, drawY, BLOCK_SIZE, BLOCK_SIZE);
              cairo_fill(cr);
            } else {
              cairo_rectangle(cr, drawX + 1, drawY + 1, BLOCK_SIZE - 2, BLOCK_SIZE - 2);
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
}

void drawGhostPiece(cairo_t *cr, TetrimoneBoard *board) {
  if (!board->isGameOver() && !board->isPaused() && 
      !board->isSplashScreenActive() && board->isGhostPieceEnabled()) {
    
    const TetrimoneBlock &piece = board->getCurrentPiece();
    auto shape = piece.getShape();
    auto color = piece.getColor();
    
    double currentPieceX, currentPieceY;
    board->getCurrentPieceInterpolatedPosition(currentPieceX, currentPieceY);
    int ghostY = board->getGhostPieceY();

    if (ghostY > (int)currentPieceY) {
      cairo_set_source_rgba(cr, color[0], color[1], color[2], 0.3);

      for (size_t y = 0; y < shape.size(); ++y) {
        for (size_t x = 0; x < shape[y].size(); ++x) {
          if (shape[y][x] == 1) {
            double drawX = (currentPieceX + x) * BLOCK_SIZE;
            double drawY = (ghostY + y) * BLOCK_SIZE;

            if (drawY >= 0) {
              if (board->retroModeActive) {
                cairo_rectangle(cr, drawX, drawY, BLOCK_SIZE, BLOCK_SIZE);
                cairo_stroke(cr);
              } else {
                cairo_rectangle(cr, drawX + 1, drawY + 1, BLOCK_SIZE - 2, BLOCK_SIZE - 2);
                cairo_stroke_preserve(cr);
                cairo_fill(cr);
              }
            }
          }
        }
      }
    }
  }
}

void drawPauseMenu(cairo_t *cr, TetrimoneBoard *board) {
  if (board->isPaused() && !board->isGameOver()) {
    cairo_set_source_rgba(cr, 0, 0, 0, 0.7);
    cairo_rectangle(cr, 0, 0, GRID_WIDTH * BLOCK_SIZE,
                    GRID_HEIGHT * BLOCK_SIZE);
    cairo_fill(cr);

    cairo_select_font_face(cr, "Sans", CAIRO_FONT_SLANT_NORMAL,
                          CAIRO_FONT_WEIGHT_BOLD);
    cairo_set_font_size(cr, 30);
    cairo_set_source_rgb(cr, 1, 1, 1);

    cairo_text_extents_t extents;
    const char *text = board->retroModeActive ? "ПРИОСТАНОВЛЕНО ПО ПРИКАЗУ ПАРТИИ" : "PAUSED";
    cairo_text_extents(cr, text, &extents);

    double x = (GRID_WIDTH * BLOCK_SIZE - extents.width) / 2;
    double y = (GRID_HEIGHT * BLOCK_SIZE) / 4;

    cairo_move_to(cr, x, y);
    cairo_show_text(cr, text);

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
    y = (GRID_HEIGHT * BLOCK_SIZE) / 2;

    for (int i = 0; i < numOptions; i++) {
        cairo_text_extents(cr, menuOptions[i], &extents);
        x = (GRID_WIDTH * BLOCK_SIZE - extents.width) / 2;

        cairo_move_to(cr, x, y);
        cairo_show_text(cr, menuOptions[i]);

        y += 40;
    }
  }
}

gboolean onDrawGameArea(GtkWidget *widget, cairo_t *cr, gpointer data) {
  TetrimoneApp *app = static_cast<TetrimoneApp *>(data);
  TetrimoneBoard *board = app->board;

  // Get widget dimensions
  GtkAllocation allocation;
  gtk_widget_get_allocation(widget, &allocation);

  // Draw background
  drawBackground(cr, board, allocation);

  // Draw gridlines
  drawGridLines(cr, board);

  // Draw failure line
  drawFailureLine(cr);

  // Draw placed blocks with line clearing animation
  drawPlacedBlocks(cr, board, app);

  // Draw splash screen if active
  if (board->isSplashScreenActive()) {
    drawSplashScreen(cr, board, app);
    return FALSE;
  }

  // Draw propaganda messages
  drawPropagandaMessage(cr, board);

  // Draw current piece with smooth movement animation
  drawCurrentPiece(cr, board);

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
  
  return FALSE;
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

  return FALSE;
}

void drawFireyGlow(cairo_t* cr, double x, double y, double size, float heatLevel, double time) {
   if (heatLevel <= 0.7f) return;
   
   // Calculate glow intensity based on heat level
   float glowIntensity = (heatLevel - 0.7f) / 0.3f; // 0.0 to 1.0 range
   
   // Create animated glow with time-based pulsing (faster pulse at higher heat)
   double pulseSpeed = 1.0 + glowIntensity * 2.0; // Pulse faster when hotter
   double pulseTime = fmod(time * pulseSpeed, 2000.0) / 2000.0; // 2-second base cycle
   double pulse = 0.5 + 0.5 * sin(pulseTime * 2 * M_PI); // 0.0 to 1.0 pulse
   
   // Combine intensity with pulse for final glow strength
   double finalGlowStrength = glowIntensity * (0.6 + 0.4 * pulse);
   
   // Save the current state
   cairo_save(cr);
   
   // Create multiple glow layers for depth (more layers at higher heat)
   int numLayers = 3 + (int)(glowIntensity * 2); // 3-5 layers based on heat
   for (int layer = 0; layer < numLayers; layer++) {
       double layerSize = size + (layer + 1) * 6 * finalGlowStrength;
       double layerAlpha = finalGlowStrength * (0.5 - layer * 0.08);
       
       if (layerAlpha <= 0) continue;
       
       // Prevent division by zero for radius
       double radius = layerSize / 2;
       if (radius <= 0) radius = 1.0;
       
       // Create radial gradient for each glow layer
       cairo_pattern_t* gradient = cairo_pattern_create_radial(
           x + size/2, y + size/2, 0,
           x + size/2, y + size/2, radius
       );
       
       // Color shifts based on heat intensity
       float redIntensity = 1.0f;
       float greenIntensity = 0.3f + glowIntensity * 0.4f;
       float blueIntensity = glowIntensity > 0.8f ? 0.2f : 0.0f; // Add blue for extreme heat
       
       // Inner glow color (varies with intensity)
       cairo_pattern_add_color_stop_rgba(gradient, 0.0, 
           redIntensity, greenIntensity, blueIntensity, layerAlpha);
       
       // Outer glow color (red fading to transparent)
       cairo_pattern_add_color_stop_rgba(gradient, 1.0, 
           1.0, 0.0, 0.0, 0.0);
       
       cairo_set_source(cr, gradient);
       
       // Draw the glow circle
       cairo_arc(cr, x + size/2, y + size/2, radius, 0, 2 * M_PI);
       cairo_fill(cr);
       
       cairo_pattern_destroy(gradient);
   }
   
   // Add flickering fire particles for extra effect at high heat
   if (heatLevel > 0.85f) {
       cairo_set_source_rgba(cr, 1.0, 0.8, 0.0, 0.4 * pulse * glowIntensity);
       
       // More particles at higher heat
       int numParticles = 6 + (int)(glowIntensity * 4);
       if (numParticles <= 0) numParticles = 1; // Prevent division by zero
       
       for (int i = 0; i < numParticles; i++) {
           double angle = (i / (double)numParticles) * 2 * M_PI + pulseTime * 4 * M_PI;
           
           // Ensure rand() % doesn't get 0
           int randRange = 5;
           if (randRange <= 0) randRange = 1;
           
           double radius = size/2 + 5 + (rand() % randRange) * glowIntensity;
           double particleX = x + size/2 + cos(angle) * radius;
           double particleY = y + size/2 + sin(angle) * radius;
           
           double particleRadius = 1 + glowIntensity * 2;
           if (particleRadius <= 0) particleRadius = 1.0;
           
           cairo_arc(cr, particleX, particleY, particleRadius, 0, 2 * M_PI);
           cairo_fill(cr);
       }
   }
   
   cairo_restore(cr);
}

// New function for drawing freezy effect
void drawFreezyEffect(cairo_t* cr, double x, double y, double size, float heatLevel, double time) {
   if (heatLevel >= 0.3f) return;
   
   // Calculate freeze intensity (higher when colder)
   float freezeIntensity = (0.3f - heatLevel) / 0.3f; // 0.0 to 1.0 range
   
   // Create subtle shimmer effect for ice (slower when colder)
   double shimmerSpeed = 0.5 + (1.0 - freezeIntensity) * 0.5; // Slower shimmer when colder
   double shimmerTime = fmod(time * shimmerSpeed, 3000.0) / 3000.0; // 3-second base cycle
   double shimmer = 0.3 + 0.2 * sin(shimmerTime * 2 * M_PI);
   
   cairo_save(cr);
   
   // Draw ice crystal overlay (more opaque when colder)
   double iceOpacity = 0.15 + 0.25 * freezeIntensity;
   cairo_set_source_rgba(cr, 0.7, 0.9, 1.0, iceOpacity * shimmer);
   cairo_rectangle(cr, x, y, size, size);
   cairo_fill(cr);
   
   // Create multiple layers of sparkly stars
   for (int layer = 0; layer < 3; layer++) {
       // Different timing for each layer creates depth
       double layerTime = shimmerTime + (layer * 0.3);
       double layerShimmer = 0.4 + 0.6 * sin(layerTime * 2 * M_PI);
       
       // Star brightness varies by layer and freeze intensity
       double starAlpha = (0.3 + 0.7 * freezeIntensity) * layerShimmer * (1.0 - layer * 0.2);
       cairo_set_source_rgba(cr, 1.0, 1.0, 1.0, starAlpha);
       
       // Number of stars increases with freeze intensity
       int numStars = 2 + layer + (int)(freezeIntensity * 4);
       
       for (int i = 0; i < numStars; i++) {
           // Use deterministic positioning based on block coordinates and layer
           int seed = (int)(x + y * 100 + layer * 1000 + i * 137);
           srand(seed); // Temporary seed for consistent positioning
           
           // Fix: Ensure we don't get division by zero or negative modulo
           int maxX = (int)(size - 8);
           int maxY = (int)(size - 8);
           if (maxX <= 0) maxX = 1;
           if (maxY <= 0) maxY = 1;
           
           double starX = x + 4 + (rand() % maxX);
           double starY = y + 4 + (rand() % maxY);
           
           // Star size varies with intensity and layer
           double starSize = 1.5 + freezeIntensity * 2 + layer * 0.5;
           
           // Draw 4-pointed star
           double armLength = starSize + layerShimmer * 0.5;
           
           // Horizontal arm
           cairo_move_to(cr, starX - armLength, starY);
           cairo_line_to(cr, starX + armLength, starY);
           
           // Vertical arm
           cairo_move_to(cr, starX, starY - armLength);
           cairo_line_to(cr, starX, starY + armLength);
           
           cairo_set_line_width(cr, 0.8 + layer * 0.2);
           cairo_stroke(cr);
           
           // Add diagonal arms for bigger stars at higher freeze intensity
           if (freezeIntensity > 0.5f && layer == 0) {
               double diagLength = armLength * 0.7;
               
               // Diagonal arms
               cairo_move_to(cr, starX - diagLength, starY - diagLength);
               cairo_line_to(cr, starX + diagLength, starY + diagLength);
               cairo_move_to(cr, starX - diagLength, starY + diagLength);
               cairo_line_to(cr, starX + diagLength, starY - diagLength);
               
               cairo_set_line_width(cr, 0.6);
               cairo_stroke(cr);
           }
       }
   }
   
   // Add floating sparkle particles around the block for extreme cold
   if (heatLevel < 0.1f) {
       cairo_set_source_rgba(cr, 0.9, 0.95, 1.0, 0.4 * shimmer);
       
       // Draw small sparkle points floating around the block
       for (int i = 0; i < 6; i++) {
           double angle = (i / 6.0) * 2 * M_PI + shimmerTime * M_PI;
           double distance = size/2 + 4 + sin(shimmerTime * 4 + i) * 2;
           double sparkleX = x + size/2 + cos(angle) * distance;
           double sparkleY = y + size/2 + sin(angle) * distance;
           
           // Draw tiny 4-pointed stars
           double tinySize = 1.0 + sin(shimmerTime * 3 + i * 2) * 0.5;
           
           cairo_move_to(cr, sparkleX - tinySize, sparkleY);
           cairo_line_to(cr, sparkleX + tinySize, sparkleY);
           cairo_move_to(cr, sparkleX, sparkleY - tinySize);
           cairo_line_to(cr, sparkleX, sparkleY + tinySize);
           
           cairo_set_line_width(cr, 0.5);
           cairo_stroke(cr);
       }
   }
   
   cairo_restore(cr);
}

void TetrimoneBoard::cleanupBackgroundImages() {
    // Clean up regular background images
    for (auto surface : backgroundImages) {
        if (surface != nullptr) {
            cairo_surface_destroy(surface);
        }
    }
    backgroundImages.clear();
    
    // Clean up patriot background images
    for (auto surface : patriotBackgroundImages) {
        if (surface != nullptr) {
            cairo_surface_destroy(surface);
        }
    }
    patriotBackgroundImages.clear();
}

cairo_surface_t* cairo_image_surface_create_from_jpeg(const char* filename) {
    // Read the file into memory first
    GFile* file = g_file_new_for_path(filename);
    GError* error = NULL;
    
    // Get file content as bytes
    GBytes* bytes = NULL;
    GFileInputStream* stream = g_file_read(file, NULL, &error);
    
    if (!stream) {
        if (error) {
            std::cerr << "Failed to open JPEG file: " << error->message << std::endl;
            g_error_free(error);
        }
        g_object_unref(file);
        return NULL;
    }
    
    // Load file content into memory
    error = NULL;
    bytes = g_input_stream_read_bytes(G_INPUT_STREAM(stream), 10 * 1024 * 1024, NULL, &error); // 10MB max
    
    if (!bytes) {
        if (error) {
            std::cerr << "Failed to read JPEG file: " << error->message << std::endl;
            g_error_free(error);
        }
        g_object_unref(stream);
        g_object_unref(file);
        return NULL;
    }
    
    // Get the data and size
    gsize size;
    const void* data = g_bytes_get_data(bytes, &size);
    
    // Use the existing from_memory function that works with ZIP files
    cairo_surface_t* surface = cairo_image_surface_create_from_memory(data, size);
    
    // Clean up
    g_bytes_unref(bytes);
    g_object_unref(stream);
    g_object_unref(file);
    
    return surface;
}

cairo_surface_t* cairo_image_surface_create_from_memory(const void* data, size_t length) {
    GError* error = NULL;
    
    // Create a memory input stream from the data
    GInputStream* stream = g_memory_input_stream_new_from_data(data, length, NULL);
    if (!stream) {
        std::cerr << "Failed to create memory stream" << std::endl;
        return NULL;
    }
    
    // Load the image using GdkPixbuf
    GdkPixbuf* pixbuf = gdk_pixbuf_new_from_stream(stream, NULL, &error);
    g_object_unref(stream);
    
    if (!pixbuf) {
        if (error) {
            std::cerr << "Failed to load image from memory: " << error->message << std::endl;
            g_error_free(error);
        }
        return NULL;
    }
    
    // Create a cairo surface of the same size
    cairo_surface_t* surface = cairo_image_surface_create(
        CAIRO_FORMAT_ARGB32,
        gdk_pixbuf_get_width(pixbuf),
        gdk_pixbuf_get_height(pixbuf)
    );
    
    // Create a cairo context
    cairo_t* cr = cairo_create(surface);
    
    // Draw the pixbuf onto the surface
    gdk_cairo_set_source_pixbuf(cr, pixbuf, 0, 0);
    cairo_paint(cr);
    
    // Clean up
    cairo_destroy(cr);
    g_object_unref(pixbuf);
    
    return surface;
}

bool TetrimoneBoard::loadBackgroundImage(const std::string& imagePath) {
    // Clean up previous image if it exists
    if (backgroundImage != nullptr) {
        cairo_surface_destroy(backgroundImage);
        backgroundImage = nullptr;
    }
    
    // Get file extension
    std::string extension = imagePath.substr(imagePath.find_last_of(".") + 1);
    std::transform(extension.begin(), extension.end(), extension.begin(), ::tolower);
    
    // Log the attempt to load
    std::cout << "Attempting to load background image: " << imagePath 
              << " (type: " << extension << ")" << std::endl;
    
    if (extension == "jpg" || extension == "jpeg") {
        // Load JPEG file
        backgroundImage = cairo_image_surface_create_from_jpeg(imagePath.c_str());
    } else {
        // Load PNG file (or attempt to) using existing method
        backgroundImage = cairo_image_surface_create_from_png(imagePath.c_str());
    }
    
    // Check if image loaded successfully
    if (backgroundImage == nullptr) {
        std::cerr << "Failed to load background image (null pointer): " << imagePath << std::endl;
        return false;
    }
    
    cairo_status_t status = cairo_surface_status(backgroundImage);
    if (status != CAIRO_STATUS_SUCCESS) {
        std::cerr << "Failed to load background image (status error): " << imagePath 
                  << " - " << cairo_status_to_string(status) << std::endl;
        
        cairo_surface_destroy(backgroundImage);
        backgroundImage = nullptr;
        return false;
    }
    
    // Log successful loading
    std::cout << "Successfully loaded background image: " << imagePath 
              << " (" << cairo_image_surface_get_width(backgroundImage) << "x" 
              << cairo_image_surface_get_height(backgroundImage) << ")" << std::endl;
    
    // Store the path and set flag
    backgroundImagePath = imagePath;
    useBackgroundImage = true;
    return true;
}

void TetrimoneBoard::selectRandomBackground() {
    // Determine which image collection to use based on patriotic mode
    std::vector<cairo_surface_t*>* imageCollection = nullptr;
    
    printf("patriot mode %i\n", patrioticModeActive);
    printf("patriot images count: %zu\n", patriotBackgroundImages.size());
    printf("regular images count: %zu\n", backgroundImages.size());
    
    if (patrioticModeActive && !patriotBackgroundImages.empty()) {
        // Use patriot images when in patriotic mode and they exist
        imageCollection = &patriotBackgroundImages;
        printf("Using patriot image collection\n");
    } else if (!backgroundImages.empty()) {
        // Use regular images as fallback or when not in patriotic mode
        imageCollection = &backgroundImages;
        printf("Using regular image collection\n");
    } else {
        // No images available
        printf("No images available\n");
        return;
    }
    
    // Generate a random index for the selected collection
    printf("Collection size: %zu, range: 0 to %zu\n", imageCollection->size(), imageCollection->size() - 1);
    std::uniform_int_distribution<int> dist(0, imageCollection->size() - 1);
    int randomIndex = dist(rng);
    printf("Generated random index: %d\n", randomIndex);
    
    // Update the current background image
    if (backgroundImage != nullptr) {
        cairo_surface_destroy(backgroundImage);
    }
    
    // Clone the selected surface to avoid double-free issues
    cairo_surface_t* selectedSurface = (*imageCollection)[randomIndex];
    int width = cairo_image_surface_get_width(selectedSurface);
    int height = cairo_image_surface_get_height(selectedSurface);
    
    // Create a new surface and copy the data
    backgroundImage = cairo_image_surface_create(
        cairo_image_surface_get_format(selectedSurface),
        width, height);
    
    // Copy the surface data
    cairo_t* cr = cairo_create(backgroundImage);
    cairo_set_source_surface(cr, selectedSurface, 0, 0);
    cairo_paint(cr);
    cairo_destroy(cr);
    
    // Set the single-image mode to use our new image
    useBackgroundImage = true;
    
    // Store which collection and index we're using for tracking
    if (imageCollection == &patriotBackgroundImages) {
        // We're using a patriot image - store the index for transitions
        currentPatriotBackgroundIndex = randomIndex;
        std::cout << "Selected random patriot background image (index " << randomIndex << ")" << std::endl;
    } else {
        // We're using a regular image
        currentBackgroundIndex = randomIndex;
        std::cout << "Selected random background image (index " << randomIndex << ")" << std::endl;
    }
}

void onBackgroundImageDialog(GtkMenuItem* menuItem, gpointer userData) {
    TetrimoneApp* app = static_cast<TetrimoneApp*>(userData);
    
    // Pause the game if it's running
    bool wasPaused = app->board->isPaused();
    if (!wasPaused && !app->board->isGameOver() && !app->board->isSplashScreenActive()) {
        onPauseGame(GTK_MENU_ITEM(app->pauseMenuItem), app);
    }
    
    std::vector<std::string> filePaths;
    
#ifdef _WIN32
    // Use Windows native dialog with multi-select support
    OPENFILENAME ofn;
    char szFile[4096] = {0};  // Increased buffer size to support multiple files
    
    ZeroMemory(&ofn, sizeof(ofn));
    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner = NULL;  // Ideally get the HWND from GTK window
    ofn.lpstrFile = szFile;
    ofn.nMaxFile = sizeof(szFile);
    ofn.lpstrFilter = "Image Files\0*.png;*.jpg;*.jpeg\0PNG Images\0*.png\0JPEG Images\0*.jpg;*.jpeg\0All Files\0*.*\0";
    ofn.nFilterIndex = 1;
    ofn.lpstrFileTitle = NULL;
    ofn.nMaxFileTitle = 0;
    ofn.lpstrInitialDir = NULL;
    ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST | OFN_ALLOWMULTISELECT | OFN_EXPLORER;
    
    if (GetOpenFileName(&ofn)) {
        // Parse multiple file selection
        char* filePtr = ofn.lpstrFile;
        
        // First string is the directory
        std::string directory(filePtr);
        filePtr += directory.length() + 1;
        
        // If no files selected after directory, it means only one file was chosen
        if (*filePtr == '\0') {
            filePaths.push_back(directory);
        } else {
            // Multiple files selected
            while (*filePtr != '\0') {
                std::string filename(filePtr);
                std::string fullPath = directory + "\\" + filename;
                filePaths.push_back(fullPath);
                
                // Move to next filename
                filePtr += filename.length() + 1;
            }
        }
    }
#else
    // Use GTK dialog on other platforms
    GtkWidget* dialog = gtk_file_chooser_dialog_new(
        "Select Background Images",
        GTK_WINDOW(app->window),
        GTK_FILE_CHOOSER_ACTION_OPEN,
        "_Cancel", GTK_RESPONSE_CANCEL,
        "_Open", GTK_RESPONSE_ACCEPT,
        NULL
    );
    
    // Allow multiple file selection
    gtk_file_chooser_set_select_multiple(GTK_FILE_CHOOSER(dialog), TRUE);
    
    // Add filter for image files
    GtkFileFilter* filterAll = gtk_file_filter_new();
    gtk_file_filter_set_name(filterAll, "All Image Files");
    gtk_file_filter_add_mime_type(filterAll, "image/png");
    gtk_file_filter_add_mime_type(filterAll, "image/jpeg");
    gtk_file_chooser_add_filter(GTK_FILE_CHOOSER(dialog), filterAll);
    
    // Add filter for PNG files
    GtkFileFilter* filterPng = gtk_file_filter_new();
    gtk_file_filter_set_name(filterPng, "PNG Images");
    gtk_file_filter_add_pattern(filterPng, "*.png");
    gtk_file_filter_add_mime_type(filterPng, "image/png");
    gtk_file_chooser_add_filter(GTK_FILE_CHOOSER(dialog), filterPng);
    
    // Add filter for JPEG files
    GtkFileFilter* filterJpeg = gtk_file_filter_new();
    gtk_file_filter_set_name(filterJpeg, "JPEG Images");
    gtk_file_filter_add_pattern(filterJpeg, "*.jpg");
    gtk_file_filter_add_pattern(filterJpeg, "*.jpeg");
    gtk_file_filter_add_mime_type(filterJpeg, "image/jpeg");
    gtk_file_chooser_add_filter(GTK_FILE_CHOOSER(dialog), filterJpeg);
    
    // Run the dialog
    if (gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_ACCEPT) {
        // Get the selected filenames
        GSList* filenames = gtk_file_chooser_get_filenames(GTK_FILE_CHOOSER(dialog));
        
        // Convert GSList to vector of strings
        for (GSList* list = filenames; list != NULL; list = list->next) {
            char* filename = static_cast<char*>(list->data);
            filePaths.push_back(filename);
            g_free(filename);
        }
        
        g_slist_free(filenames);
    }
    
    gtk_widget_destroy(dialog);
#endif
    
    // Process the selected file paths
    if (!filePaths.empty()) {
        // Clean up existing background images
        app->board->cleanupBackgroundImages();
        
        // Flag to track successful image loading
        bool imagesLoaded = false;
        
        // Process each selected file
        for (const auto& filepath : filePaths) {
            // Get file extension
            std::string extension = filepath.substr(filepath.find_last_of(".") + 1);
            std::transform(extension.begin(), extension.end(), extension.begin(), ::tolower);
            
            cairo_surface_t* surface = nullptr;
            
            if (extension == "jpg" || extension == "jpeg") {
                // Load JPEG
                surface = cairo_image_surface_create_from_jpeg(filepath.c_str());
            } else {
                // Default to PNG
                surface = cairo_image_surface_create_from_png(filepath.c_str());
            }
            
            // Check if the surface was created successfully
            if (surface && cairo_surface_status(surface) == CAIRO_STATUS_SUCCESS) {
                app->board->backgroundImages.push_back(surface);
                imagesLoaded = true;
            } else {
                std::cerr << "Failed to load image: " << filepath << std::endl;
                if (surface) cairo_surface_destroy(surface);
            }
        }
        
        if (imagesLoaded) {
            // Set background modes
            app->board->useBackgroundZip = true;
            app->board->useBackgroundImage = true;
            
            // Select initial random background
            app->board->selectRandomBackground();
            
            // Activate background toggle
            gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(app->backgroundToggleMenuItem), TRUE);
            
            // Process any pending events before showing opacity dialog
            while (gtk_events_pending())
                gtk_main_iteration();
                
            // Show opacity dialog
            onBackgroundOpacityDialog(NULL, app);
        } else {
            // No images loaded
            GtkWidget* errorDialog = gtk_message_dialog_new(
                GTK_WINDOW(app->window),
                GTK_DIALOG_MODAL,
                GTK_MESSAGE_ERROR,
                GTK_BUTTONS_OK,
                "No valid image files could be loaded."
            );
            gtk_dialog_run(GTK_DIALOG(errorDialog));
            gtk_widget_destroy(errorDialog);
        }
    }
    
    // Redraw the game area
    gtk_widget_queue_draw(app->gameArea);
    
    // Resume the game if it wasn't paused before
    if (!wasPaused && !app->board->isGameOver() && !app->board->isSplashScreenActive()) {
        onPauseGame(GTK_MENU_ITEM(app->pauseMenuItem), app);
    }
}

void TetrimoneBoard::startBackgroundTransition() {
    if (!useBackgroundZip || !useBackgroundImage) {
        return; // Only perform transitions when using background images from ZIP
    }
    
    // Check if we have appropriate images for the current mode
    if (patrioticModeActive && patriotBackgroundImages.empty()) {
        return; // No patriot images available in patriotic mode
    }
    if (!patrioticModeActive && backgroundImages.empty()) {
        return; // No regular images available in normal mode
    }
    
    // If already transitioning, cancel the current transition
    if (isTransitioning && transitionTimerId > 0) {
        g_source_remove(transitionTimerId);
        transitionTimerId = 0;
    }
    
    // Store the current background for the fade out effect
    if (oldBackground != nullptr) {
        cairo_surface_destroy(oldBackground);
    }
    
    // Clone the current background
    if (backgroundImage != nullptr) {
        int width = cairo_image_surface_get_width(backgroundImage);
        int height = cairo_image_surface_get_height(backgroundImage);
        
        oldBackground = cairo_image_surface_create(
            cairo_image_surface_get_format(backgroundImage),
            width, height);
        
        cairo_t* cr = cairo_create(oldBackground);
        cairo_set_source_surface(cr, backgroundImage, 0, 0);
        cairo_paint(cr);
        cairo_destroy(cr);
    }
    
    // Start with the current opacity
    transitionOpacity = backgroundOpacity;
    
    // Set up transition state
    isTransitioning = true;
    transitionDirection = -1; // Start by fading out
    
    // Select a new random background - this will be applied during the transition
    // We'll call selectRandomBackground() when we're fully faded out
    
    // Start the transition timer - update 20 times per second
    transitionTimerId = g_timeout_add(50, 
        [](gpointer data) -> gboolean {
            TetrimoneBoard* board = static_cast<TetrimoneBoard*>(data);
            board->updateBackgroundTransition();
            return TRUE; // Keep the timer running
        }, 
        this);
}

void TetrimoneBoard::updateBackgroundTransition() {
    if (!isTransitioning) {
        return;
    }
    
    // Update opacity based on direction
    const double TRANSITION_SPEED = 0.02; // Change this to adjust fade speed
    transitionOpacity += transitionDirection * TRANSITION_SPEED;
    
    // Check for direction change (from fade-out to fade-in)
    if (transitionDirection == -1 && transitionOpacity <= 0.0) {
        transitionOpacity = 0.0;
        transitionDirection = 1; // Change to fade in
        
        // Now select a new random background using the existing function
        // This will automatically choose patriot vs regular based on current mode
        selectRandomBackground();
        
        std::cout << "Background transition: selected new random background" << std::endl;
    }
    
    // Check if transition is complete
    if (transitionDirection == 1 && transitionOpacity >= backgroundOpacity) {
        transitionOpacity = backgroundOpacity;
        isTransitioning = false;
        
        // Clean up the old background
        if (oldBackground != nullptr) {
            cairo_surface_destroy(oldBackground);
            oldBackground = nullptr;
        }
        
        // Clean up the timer
        if (transitionTimerId > 0) {
            g_source_remove(transitionTimerId);
            transitionTimerId = 0;
        }
    }
}

void TetrimoneBoard::cancelBackgroundTransition() {
    if (isTransitioning && transitionTimerId > 0) {
        g_source_remove(transitionTimerId);
        transitionTimerId = 0;
    }
    
    isTransitioning = false;
    
    // Clean up the old background
    if (oldBackground != nullptr) {
        cairo_surface_destroy(oldBackground);
        oldBackground = nullptr;
    }
}

bool TetrimoneBoard::loadBackgroundImagesFromZip(const std::string& zipPath) {
    // Clean up existing background images first
    cleanupBackgroundImages();
    
    // Also clean up patriot background images
    for (auto surface : patriotBackgroundImages) {
        if (surface != nullptr) {
            cairo_surface_destroy(surface);
        }
    }
    patriotBackgroundImages.clear();
    
    // Store the ZIP path
    backgroundZipPath = zipPath;
    
    int errCode = 0;
    zip_t *archive = zip_open(zipPath.c_str(), 0, &errCode);
    
    if (!archive) {
        zip_error_t zipError;
        zip_error_init_with_code(&zipError, errCode);
        std::cerr << "Failed to open ZIP archive: " << zip_error_strerror(&zipError) << std::endl;
        zip_error_fini(&zipError);
        return false;
    }
    
    // Get the number of entries in the archive
    zip_int64_t numEntries = zip_get_num_entries(archive, 0);
    if (numEntries <= 0) {
        std::cerr << "No files found in ZIP archive" << std::endl;
        zip_close(archive);
        return false;
    }
    
    // Count how many image files we found
    int imageCount = 0;
    int patriotImageCount = 0;
    
    // Process each file in the archive
    for (zip_int64_t i = 0; i < numEntries; i++) {
        // Get file info
        zip_stat_t stat;
        if (zip_stat_index(archive, i, 0, &stat) < 0) {
            std::cerr << "Failed to get file stats at index " << i << std::endl;
            continue;
        }
        
        // Check if it's an image file (PNG or JPEG)
        std::string filename = stat.name;
        std::string extension = "";
        size_t dotPos = filename.find_last_of('.');
        if (dotPos != std::string::npos) {
            extension = filename.substr(dotPos + 1);
            std::transform(extension.begin(), extension.end(), extension.begin(), 
                [](unsigned char c) { return std::tolower(c); });
        }
        
        if (extension != "png" && extension != "jpg" && extension != "jpeg") {
            continue;  // Skip non-image files
        }
        
        // Check if filename contains "patriot" (case-insensitive)
        std::string filenameLower = filename;
        std::transform(filenameLower.begin(), filenameLower.end(), filenameLower.begin(), 
            [](unsigned char c) { return std::tolower(c); });
        bool isPatriotImage = filenameLower.find("patriot") != std::string::npos;
        
        // Open the file in the archive
        zip_file_t *file = zip_fopen_index(archive, i, 0);
        if (!file) {
            std::cerr << "Failed to open file in ZIP archive: " << zip_strerror(archive) << std::endl;
            continue;
        }
        
        // Allocate memory for the file content
        std::vector<uint8_t> fileData(stat.size);
        
        // Read the file content
        zip_int64_t bytesRead = zip_fread(file, fileData.data(), stat.size);
        if (bytesRead < 0 || static_cast<zip_uint64_t>(bytesRead) != stat.size) {
            std::cerr << "Failed to read file: " << zip_file_strerror(file) << std::endl;
            zip_fclose(file);
            continue;
        }
        
        // Close the file
        zip_fclose(file);
        
        cairo_surface_t* surface = nullptr;
        
        if (extension == "png") {
            // Use existing PNG loading from memory code
            struct PngReadData {
                const std::vector<uint8_t>* data;
                size_t offset;
            };
            
            PngReadData readData = { &fileData, 0 };
            
            surface = cairo_image_surface_create_from_png_stream(
                [](void* closure, unsigned char* data, unsigned int length) -> cairo_status_t {
                    PngReadData* readData = static_cast<PngReadData*>(closure);
                    
                    // Check if we've reached the end of our data
                    if (readData->offset >= readData->data->size()) {
                        return CAIRO_STATUS_READ_ERROR;
                    }
                    
                    // Calculate how much we can read
                    size_t remaining = readData->data->size() - readData->offset;
                    size_t toRead = (length < remaining) ? length : remaining;
                    
                    // Copy the data
                    memcpy(data, readData->data->data() + readData->offset, toRead);
                    readData->offset += toRead;
                    
                    return CAIRO_STATUS_SUCCESS;
                },
                &readData
            );
        } else if (extension == "jpg" || extension == "jpeg") {
            // Load JPEG from memory using GdkPixbuf
            surface = cairo_image_surface_create_from_memory(fileData.data(), fileData.size());
        }
        
        // Check if the surface was created successfully
        if (surface && cairo_surface_status(surface) == CAIRO_STATUS_SUCCESS) {
            // Add the surface to the appropriate collection based on filename
            if (isPatriotImage) {
                patriotBackgroundImages.push_back(surface);
                patriotImageCount++;
                std::cout << "Loaded patriot background image: " << filename << std::endl;
            } else {
                backgroundImages.push_back(surface);
                imageCount++;
                std::cout << "Loaded regular background image: " << filename << std::endl;
            }
        } else {
            std::cerr << "Failed to load image from ZIP: " << filename << std::endl;
            if (surface) cairo_surface_destroy(surface);
        }
    }
    
    // Close the archive
    zip_close(archive);
    
    // Check if we loaded any images
    if (imageCount == 0 && patriotImageCount == 0) {
        std::cerr << "No valid image files found in ZIP archive" << std::endl;
        return false;
    }
    
    // Set the current background to a random one from regular images if available
    // If no regular images but patriot images exist, you might want to handle this case
    if (imageCount > 0) {
        useBackgroundZip = true;
        useBackgroundImage = true;
        selectRandomBackground();
    } else if (patriotImageCount > 0) {
        // Handle case where only patriot images are available
        // This depends on your game logic - you might want to set patriotic mode
        // or handle this differently
        std::cout << "Only patriot background images found. Consider enabling patriotic mode." << std::endl;
    }
    
    std::cout << "Successfully loaded " << imageCount << " regular background images and " 
              << patriotImageCount << " patriot background images from ZIP" << std::endl;
    return true;
}

void onOpacityValueChanged(GtkRange *range, gpointer userData) {
  TetrimoneApp *app = static_cast<TetrimoneApp *>(userData);

  // Update the opacity in the board
  double opacity = gtk_range_get_value(range);

  // Early return if the background image isn't valid
  if (!app->board->isUsingBackgroundImage() ||
      app->board->getBackgroundImage() == nullptr) {
    return;
  }

  // Check surface status before attempting to draw
  cairo_status_t status =
      cairo_surface_status(app->board->getBackgroundImage());
  if (status != CAIRO_STATUS_SUCCESS) {
    std::cerr << "Invalid background image surface during opacity change: "
              << cairo_status_to_string(status) << std::endl;
    return;
  }

  app->board->setBackgroundOpacity(opacity);

  // Queue a redraw rather than forcing immediate redraw
  gtk_widget_queue_draw(app->gameArea);
}
