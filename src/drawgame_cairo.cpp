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
