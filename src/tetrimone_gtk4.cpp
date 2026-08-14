#ifdef GTK4
#include "tetrimone_gtk.h"
#endif

#ifdef QT5
#include "tetrimone_qt5.h"
#endif

#include "gtk4_dialog_compat.h"
#include "audiomanager.h"
#include <iostream>
#include <string>
#include <cstring>
#include <array>
#include <algorithm>
#ifdef _WIN32
#include <windows.h>
#include <commdlg.h>
#endif
#include "highscores.h"
#include "propaganda_messages.h"
#include "freedom_messages.h"
#include "commandline.h"

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

// Global variables for key repeat handling
extern bool keyDownPressed;
extern bool keyLeftPressed;
extern bool keyRightPressed;
extern int keyDownTimer;
extern int keyLeftTimer;
extern int keyRightTimer;
extern int keyDownDelay;
extern int keyLeftDelay;
extern int keyRightDelay;
extern int keyDownCount;
extern int keyLeftCount;
extern int keyRightCount;

// Global variables
extern int BLOCK_SIZE;
extern int currentThemeIndex;
extern int GRID_WIDTH;
extern int GRID_HEIGHT;
extern int fireworksTimer;

gboolean onKeyDownTick(gpointer userData) {
  TetrimoneApp *app = static_cast<TetrimoneApp *>(userData);

  // Only process if the game is active and key is still pressed
  if (!app->board->isPaused() && !app->board->isGameOver() &&
      !app->board->isSplashScreenActive() && keyDownPressed) {

    // Move the piece down with acceleration
    app->board->movePiece(0, 1);
    keyDownCount++;

    // Accelerate by decreasing the delay (matches joystick.cpp)
    if (keyDownCount > 6) {
      keyDownDelay = 20; // Very fast (matches joystick)
    } else if (keyDownCount > 4) {
      keyDownDelay = 30; // Fast (matches joystick)
    } else if (keyDownCount > 2) {
      keyDownDelay = 60; // Medium (matches joystick)
    }

    // Update the timer with the new delay
    keyDownTimer = g_timeout_add(keyDownDelay, onKeyDownTick, app);

    // Update the display
    updateDisplay(app);
    updateLabels(app);

    // Stop this timer instance (we created a new one above)
    return FALSE;
  }

  // If game is paused or key is released, stop the timer
  keyDownTimer = 0;
  return FALSE;
}

gboolean onKeyLeftTick(gpointer userData) {
  TetrimoneApp *app = static_cast<TetrimoneApp *>(userData);

  if (!app->board->isPaused() && !app->board->isGameOver() &&
      !app->board->isSplashScreenActive() && keyLeftPressed) {

    // Move the piece left with acceleration
    app->board->movePiece(-1, 0);
    keyLeftCount++;

    // Accelerate by decreasing the delay (matches joystick.cpp horizontal movement)
    if (keyLeftCount > 6) {
      keyLeftDelay = 30; // Very fast
    } else if (keyLeftCount > 4) {
      keyLeftDelay = 50; // Fast
    } else if (keyLeftCount > 2) {
      keyLeftDelay = 100; // Medium
    }

    // Update the timer with the new delay
    keyLeftTimer = g_timeout_add(keyLeftDelay, onKeyLeftTick, app);

    // Update the display
    updateDisplay(app);
    updateLabels(app);

    // Stop this timer instance
    return FALSE;
  }

  keyLeftTimer = 0;
  return FALSE;
}

gboolean onKeyRightTick(gpointer userData) {
  TetrimoneApp *app = static_cast<TetrimoneApp *>(userData);

  if (!app->board->isPaused() && !app->board->isGameOver() &&
      !app->board->isSplashScreenActive() && keyRightPressed) {

    // Move the piece right with acceleration
    app->board->movePiece(1, 0);
    keyRightCount++;

    // Accelerate by decreasing the delay (matches joystick.cpp horizontal movement)
    if (keyRightCount > 6) {
      keyRightDelay = 30; // Very fast
    } else if (keyRightCount > 4) {
      keyRightDelay = 50; // Fast
    } else if (keyRightCount > 2) {
      keyRightDelay = 100; // Medium
    }

    // Update the timer with the new delay
    keyRightTimer = g_timeout_add(keyRightDelay, onKeyRightTick, app);

    // Update the display
    updateDisplay(app);
    updateLabels(app);

    // Stop this timer instance
    return FALSE;
  }

  keyRightTimer = 0;
  return FALSE;
}

// ----------------------------------------------------------------------------
// Firework particle system - no GTK4 API usage here beyond g_timeout_add /
// g_source_remove, both of which are unchanged in GTK4 (they're GLib, not
// GTK). Left as-is.
// ----------------------------------------------------------------------------

void TetrimoneBoard::startFireworksAnimation(int linesCleared) {
    if (linesCleared != 4) return; // Only for Tetrimone (4 lines)

    fireworksActive = true;
    fireworksType = 1; // Tetrimone fireworks
    fireworkParticles.clear();
    fireworksStartTime = std::chrono::high_resolution_clock::now();

    // Create multiple firework bursts across the cleared lines
    for (int i = 0; i < 5; i++) {
        double x = (rng() % GRID_WIDTH) * BLOCK_SIZE + BLOCK_SIZE / 2;
        double y = (rng() % 4 + GRID_HEIGHT - 8) * BLOCK_SIZE + BLOCK_SIZE / 2;

        // Use theme colors for fireworks
        int colorIndex = rng() % 7; // Use tetrimone block colors
        std::array<double, 3> color = TETRIMONEBLOCK_COLOR_THEMES[currentThemeIndex][colorIndex];

        createFireworkBurst(x, y, color, 15 + rng() % 10);
    }

    // Start animation timer
    if (fireworksTimer > 0) {
        g_source_remove(fireworksTimer);
    }

    fireworksTimer = g_timeout_add(16, // ~60 FPS
        [](gpointer userData) -> gboolean {
            TetrimoneBoard* board = static_cast<TetrimoneBoard*>(userData);
            board->updateFireworksAnimation();
            return board->isFireworksActive();
        },
        this);
}

void TetrimoneBoard::createFireworkBurst(double centerX, double centerY,
                                        const std::array<double, 3>& baseColor,
                                        int particleCount) {
    for (int i = 0; i < particleCount; i++) {
        FireworkParticle particle;

        // Random angle and speed
        double angle = (2.0 * M_PI * i) / particleCount + (rng() % 100 - 50) * 0.01;
        double speed = 2.0 + (rng() % 100) * 0.03;

        particle.x = centerX;
        particle.y = centerY;
        particle.vx = cos(angle) * speed;
        particle.vy = sin(angle) * speed;
        particle.life = 1.0;
        particle.maxLife = 1.0 + (rng() % 100) * 0.01; // Slight variation
        particle.size = 3.0 + (rng() % 3);
        particle.gravity = 0.1 + (rng() % 5) * 0.01;
        particle.fade = 0.008 + (rng() % 5) * 0.001;

        // Color variation
        particle.color = baseColor;
        for (int c = 0; c < 3; c++) {
            particle.color[c] += (rng() % 40 - 20) * 0.01;
            particle.color[c] = std::max(0.0, std::min(1.0, particle.color[c]));
        }

        fireworkParticles.push_back(particle);
    }
}

void TetrimoneBoard::updateFireworksAnimation() {
    auto now = std::chrono::high_resolution_clock::now();
    auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(now - fireworksStartTime).count();

    // Add new bursts over time for more spectacular effect
    if (elapsed > 200 && elapsed < 1500 && (elapsed % 300) < 50) {
        double x = (rng() % GRID_WIDTH) * BLOCK_SIZE + BLOCK_SIZE / 2;
        double y = (rng() % 6 + GRID_HEIGHT - 10) * BLOCK_SIZE + BLOCK_SIZE / 2;

        int colorIndex = rng() % 7;
        std::array<double, 3> color = TETRIMONEBLOCK_COLOR_THEMES[currentThemeIndex][colorIndex];

        createFireworkBurst(x, y, color, 12 + rng() % 8);
    }

    // Update existing particles
    for (auto it = fireworkParticles.begin(); it != fireworkParticles.end();) {
        FireworkParticle& p = *it;

        // Update physics
        p.x += p.vx;
        p.y += p.vy;
        p.vy += p.gravity; // Apply gravity
        p.life -= p.fade;

        // Add some air resistance
        p.vx *= 0.98;
        p.vy *= 0.98;

        // Remove dead particles
        if (p.life <= 0.0) {
            it = fireworkParticles.erase(it);
        } else {
            ++it;
        }
    }

    // FORCE REDRAW - This is crucial!
    if (app) {
        updateDisplay(app);
    }

    // End animation when time is up or no particles left
    if (elapsed >= FIREWORKS_DURATION || fireworkParticles.empty()) {
        fireworksActive = false;
        fireworkParticles.clear();
        if (fireworksTimer > 0) {
            g_source_remove(fireworksTimer);
            fireworksTimer = 0;
        }
    }
}

// ----------------------------------------------------------------------------
// Key handling: GTK4 removed "key-press-event"/"key-release-event" and
// GdkEventKey entirely. Input now comes through a GtkEventControllerKey
// attached to the window, with separate "key-pressed" (returns gboolean:
// TRUE = handled, stop propagation) and "key-released" (void) signals.
// GDK_KEY_* constants are unchanged.
// ----------------------------------------------------------------------------

gboolean onKeyPressed(GtkEventControllerKey *controller, guint keyval,
                       guint keycode, GdkModifierType state, gpointer data) {
  (void)controller;
  (void)keycode;
  (void)state;
  TetrimoneApp *app = static_cast<TetrimoneApp *>(data);
  TetrimoneBoard *board = app->board;

  // Handle space to dismiss splash screen first
  if (keyval == GDK_KEY_space && board->isSplashScreenActive()) {
    board->dismissSplashScreen();
    gtk_widget_queue_draw(app->gameArea);
    gtk_widget_queue_draw(app->nextPieceArea);
    updateLabels(app);
    return true;
  }

  // Handle game control keys only when game is active
  if (!board->isPaused() && !board->isGameOver() &&
      !board->isSplashScreenActive()) {
    switch (keyval) {
    case GDK_KEY_Left:
    case GDK_KEY_a:
    case GDK_KEY_A:
      // Start left acceleration if not already active
      if (!keyLeftPressed) {
        keyLeftPressed = true;
        keyLeftCount = 0;
        keyLeftDelay = 150; // Reset to initial delay

        // Immediate move
        board->movePiece(-1, 0);

        // Start the repeat timer
        if (keyLeftTimer == 0) {
          keyLeftTimer = g_timeout_add(keyLeftDelay, onKeyLeftTick, app);
        }
      }
      break;

    case GDK_KEY_Right:
    case GDK_KEY_d:
    case GDK_KEY_D:
      // Start right acceleration if not already active
      if (!keyRightPressed) {
        keyRightPressed = true;
        keyRightCount = 0;
        keyRightDelay = 150; // Reset to initial delay

        // Immediate move
        board->movePiece(1, 0);

        // Start the repeat timer
        if (keyRightTimer == 0) {
          keyRightTimer = g_timeout_add(keyRightDelay, onKeyRightTick, app);
        }
      }
      break;

    case GDK_KEY_Down:
    case GDK_KEY_s:
    case GDK_KEY_S:
      // Start down acceleration if not already active
      if (!keyDownPressed) {
        keyDownPressed = true;
        keyDownCount = 0;
        keyDownDelay = 150; // Reset to initial delay

        // Immediate move
        board->movePiece(0, 1);

        // Start the repeat timer
        if (keyDownTimer == 0) {
          keyDownTimer = g_timeout_add(keyDownDelay, onKeyDownTick, app);
        }
      }
      break;

    case GDK_KEY_Up:
    case GDK_KEY_w:
    case GDK_KEY_W:
      board->rotatePiece(true);
      break;

    case GDK_KEY_z:
    case GDK_KEY_Z:
      board->rotatePiece(false);
      break;

    case GDK_KEY_space:
      board->hardDrop();
      break;
    }
  }

  // Handle global control keys regardless of game state
  switch (keyval) {
  case GDK_KEY_p:
  case GDK_KEY_P:
    if (!board->isSplashScreenActive()) {
      onPauseGame(app->pauseAction, nullptr, app);
    }
    break;
  case GDK_KEY_m:
  case GDK_KEY_M:
    if (app->board->musicPaused) {
      app->board->resumeBackgroundMusic();
    } else {
      app->board->pauseBackgroundMusic();
    }
    break;

  case GDK_KEY_n:
  case GDK_KEY_N:
    if (board->isPaused()) {
      onRestartGame(app->restartAction, nullptr, app);
    }
    break;

  case GDK_KEY_q:
  case GDK_KEY_Q:
    if (board->isPaused()) {
      onQuitGame(nullptr, nullptr, app);
    }
    break;

  case GDK_KEY_r:
  case GDK_KEY_R:
    if (board->isGameOver()) {
      onRestartGame(app->restartAction, nullptr, app);
    }
    break;

  case GDK_KEY_Escape:
    // Emergency unpause if somehow stuck
    if (board->isPaused() && !board->isGameOver()) {
      onPauseGame(app->pauseAction, nullptr, app);
    }
    break;

  case GDK_KEY_period: {
    // Toggle retro mode with just the period key
    // Save the current theme index when entering retro mode
    int savedThemeIndex = 0;

    // Toggle the retro mode flag
    board->retroModeActive = !board->retroModeActive;
    board->patrioticModeActive = false;

    if (board->retroModeActive) {
      // Store current theme before switching to retro mode
      savedThemeIndex = currentThemeIndex;
      gtk_window_set_title(GTK_WINDOW(app->window), "БЛОЧНАЯ РЕВОЛЮЦИЯ");
      // Set to Soviet Retro theme (last theme in the list)
      currentThemeIndex = NUM_COLOR_THEMES - 1;
      gtk_label_set_markup(GTK_LABEL(app->difficultyLabel),
                  app->board->getDifficultyText(app->difficulty).c_str());
      gtk_label_set_markup(GTK_LABEL(app->controlsHeaderLabel), "<b>ПАРТИЙНЫЕ ДИРЕКТИВЫ</b>");
      // Disable background image
      if (board->isUsingBackgroundImage() || board->isUsingBackgroundZip()) {
        board->setUseBackgroundImage(false);
        board->setUseBackgroundZip(false);
        g_simple_action_set_state(app->backgroundToggleAction, g_variant_new_boolean(FALSE));
      }

      // Play a special sound effect
      board->playSound(GameSoundEvent::Select);
    } else {
      gtk_window_set_title(GTK_WINDOW(app->window), "Tetrimone");
      // Restore previous theme
      currentThemeIndex = savedThemeIndex;
      gtk_label_set_markup(GTK_LABEL(app->difficultyLabel),
                  app->board->getDifficultyText(app->difficulty).c_str());
      gtk_label_set_markup(GTK_LABEL(app->controlsHeaderLabel), "<b>Controls</b>");
      // Re-enable background if it was enabled before
      if (board->getBackgroundImage() != nullptr) {
        board->setUseBackgroundImage(true);
        // Also restore the useBackgroundZip flag if background images were loaded from ZIP
        if (!board->backgroundZipPath.empty()) {
          board->setUseBackgroundZip(true);
          // Trigger a background transition for a nice effect when returning from retro mode
          board->startBackgroundTransition();
        }
        g_simple_action_set_state(app->backgroundToggleAction, g_variant_new_boolean(TRUE));
      }

      // Re-enable music if sound is enabled
      if (!app->backgroundMusicPlaying && board->sound_enabled_) {
        board->resumeBackgroundMusic();
        app->backgroundMusicPlaying = true;
      }

      std::cout << "Retro mode OFF" << std::endl;
    }

    // Update controls text
    gtk_label_set_text(GTK_LABEL(app->controlsLabel),
        board->retroModeActive ?
        "Управление клавиатурой:\n"
        "• Влево/Вправо/A/D: Перемещение блока\n"
        "• Вверх/W: Поворот по часовой стрелке\n"
        "• Z: Поворот против часовой стрелки\n"
        "• Вниз/S: Мягкое падение\n"
        "• Пробел: Быстрое падение\n"
        "• P: Пауза/Продолжение игры\n"
        "• R: Перезапуск игры\n"
        "• M: Переключение музыки\n\n"
        "Поддержка контроллера доступна.\n"
        "Настройка в меню Управление." :
        "Keyboard Controls:\n"
        "• Left/Right/A/D: Move block\n"
        "• Up/W: Rotate clockwise\n"
        "• Z: Rotate counter-clockwise\n"
        "• Down/S: Soft drop\n"
        "• Space: Hard drop\n"
        "• P: Pause/Resume game\n"
        "• R: Restart game\n"
        "• M: Toggle music\n\n"
        "Controller support is available.\n"
        "Configure in Controls menu.");

    // Redraw game area to show theme change
    gtk_widget_queue_draw(app->gameArea);
    gtk_widget_queue_draw(app->nextPieceArea);

    app->board->pauseBackgroundMusic();
    app->board->resumeBackgroundMusic();
    break;
  }

  case GDK_KEY_comma: {
    // Toggle patriotic mode with comma key
    // Save the current theme index when entering patriotic mode
    int savedThemeIndex = 0;

    // Toggle the patriotic mode flag
    board->retroModeActive = false;
    board->patrioticModeActive = !board->patrioticModeActive;

    if (board->patrioticModeActive) {
      // Store current theme before switching to patriotic mode
      savedThemeIndex = currentThemeIndex;
      gtk_window_set_title(GTK_WINDOW(app->window), "FREEDOM BLOCKS - GOD BLESS AMERICA");
      // Set to American Patriotic theme (theme index 31)
      currentThemeIndex = NUM_COLOR_THEMES - 2;
      gtk_label_set_markup(GTK_LABEL(app->difficultyLabel),
                  app->board->getDifficultyText(app->difficulty).c_str());
      gtk_label_set_markup(GTK_LABEL(app->controlsHeaderLabel), "<b>FREEDOM COMMANDS</b>");

      // Re-enable background if it was enabled before
      if (board->getBackgroundImage() != nullptr) {
        board->setUseBackgroundImage(true);
        // Also restore the useBackgroundZip flag if background images were loaded from ZIP
        if (!board->backgroundZipPath.empty()) {
          board->setUseBackgroundZip(true);
          // Trigger a background transition for a nice effect when returning from patriotic mode
        }
        g_simple_action_set_state(app->backgroundToggleAction, g_variant_new_boolean(TRUE));
      }
      board->startBackgroundTransition();

      // Play a special patriotic sound effect
      board->playSound(GameSoundEvent::Select);
      std::cout << "Patriotic mode ON - FREEDOM ACTIVATED!" << std::endl;
    } else {
      gtk_window_set_title(GTK_WINDOW(app->window), "Tetrimone");
      // Restore previous theme
      currentThemeIndex = savedThemeIndex;
      gtk_label_set_markup(GTK_LABEL(app->difficultyLabel),
                  app->board->getDifficultyText(app->difficulty).c_str());
      gtk_label_set_markup(GTK_LABEL(app->controlsHeaderLabel), "<b>Controls</b>");

      // Re-enable background if it was enabled before
      if (board->getBackgroundImage() != nullptr) {
        board->setUseBackgroundImage(true);
        // Also restore the useBackgroundZip flag if background images were loaded from ZIP
        if (!board->backgroundZipPath.empty()) {
          board->setUseBackgroundZip(true);
          // Trigger a background transition for a nice effect when returning from patriotic mode
        }
        g_simple_action_set_state(app->backgroundToggleAction, g_variant_new_boolean(TRUE));
      }
      board->startBackgroundTransition();

      // Re-enable music if sound is enabled
      if (!app->backgroundMusicPlaying && board->sound_enabled_) {
        board->resumeBackgroundMusic();
        app->backgroundMusicPlaying = true;
      }

      std::cout << "Patriotic mode OFF" << std::endl;
    }

    // Update controls text
    gtk_label_set_text(GTK_LABEL(app->controlsLabel),
        board->patrioticModeActive ?
        "Freedom Controls:\n"
        "• Left/Right/A/D: Exercise your right to move blocks\n"
        "• Up/W: Rotate clockwise (like freedom)\n"
        "• Z: Rotate counter-clockwise (constitutional right)\n"
        "• Down/S: Soft drop (gentle like democracy)\n"
        "• Space: Hard drop (decisive like America)\n"
        "• P: Pause/Resume (work-life balance)\n"
        "• R: Restart (second chances, American dream)\n"
        "• M: Toggle music (freedom of choice)\n\n"
        "Controller support available.\n"
        "Configure in Controls menu.\n"
        "🇺🇸 GOD BLESS AMERICA! 🦅" :
        "Keyboard Controls:\n"
        "• Left/Right/A/D: Move block\n"
        "• Up/W: Rotate clockwise\n"
        "• Z: Rotate counter-clockwise\n"
        "• Down/S: Soft drop\n"
        "• Space: Hard drop\n"
        "• P: Pause/Resume game\n"
        "• R: Restart game\n"
        "• M: Toggle music\n\n"
        "Controller support is available.\n"
        "Configure in Controls menu.");

    // Redraw game area to show theme change
    gtk_widget_queue_draw(app->gameArea);
    gtk_widget_queue_draw(app->nextPieceArea);

    app->board->pauseBackgroundMusic();
    app->board->resumeBackgroundMusic();
    break;
  }

  default:
    break;
  }

  // Always redraw and update after any key press
  gtk_widget_queue_draw(app->gameArea);
  gtk_widget_queue_draw(app->nextPieceArea);
  updateLabels(app);

  return true; // Always claim we handled the key event
}

void onKeyReleased(GtkEventControllerKey *controller, guint keyval,
                    guint keycode, GdkModifierType state, gpointer data) {
  (void)controller;
  (void)keycode;
  (void)state;
  TetrimoneApp *app = static_cast<TetrimoneApp *>(data);

  switch (keyval) {
    case GDK_KEY_Down:
    case GDK_KEY_s:
    case GDK_KEY_S:
      // Stop down acceleration
      keyDownPressed = false;
      if (keyDownTimer > 0) {
        g_source_remove(keyDownTimer);
        keyDownTimer = 0;
      }
      break;

    case GDK_KEY_Left:
    case GDK_KEY_a:
    case GDK_KEY_A:
      // Stop left acceleration
      keyLeftPressed = false;
      if (keyLeftTimer > 0) {
        g_source_remove(keyLeftTimer);
        keyLeftTimer = 0;
      }
      break;

    case GDK_KEY_Right:
    case GDK_KEY_d:
    case GDK_KEY_D:
      // Stop right acceleration
      keyRightPressed = false;
      if (keyRightTimer > 0) {
        g_source_remove(keyRightTimer);
        keyRightTimer = 0;
      }
      break;
  }

  gtk_widget_queue_draw(app->gameArea);
  gtk_widget_queue_draw(app->nextPieceArea);
  updateLabels(app);
}

gboolean onTimerTick(gpointer data) {
  TetrimoneApp *app = static_cast<TetrimoneApp *>(data);
  TetrimoneBoard *board = app->board;

  if (!board->isPaused() && !board->isSplashScreenActive() && !board->retroModeActive) {
    board->coolDown();
  }

  if (!board->isPaused()) {
    board->updateGame();

    // If the game just ended after this update, check for high score
    if (board->isGameOver()) {
      if (!board->highScoreAlreadyProcessed) {
        board->highScoreAlreadyProcessed = true;
        bool isHighScore = board->checkAndRecordHighScore(app);

        // If it's a high score, play a special sound
        if (isHighScore) {
          board->playSound(GameSoundEvent::Excellent);
        }

        if (board->retroModeActive) {
          // Delay slightly for dramatic effect
          g_timeout_add(1500, [](gpointer userData) -> gboolean {
            TetrimoneApp *app = static_cast<TetrimoneApp*>(userData);
            showIdeologicalFailureDialog(app);
            return FALSE; // One-time call
          }, app);
        }
        if (board->patrioticModeActive) {
          // Delay slightly for dramatic effect
          g_timeout_add(1500, [](gpointer userData) -> gboolean {
            TetrimoneApp *app = static_cast<TetrimoneApp*>(userData);
            showPatrioticPerformanceDialog(app);
            return FALSE; // One-time call
          }, app);
        }
      }
    }
    gtk_widget_queue_draw(app->gameArea);
    gtk_widget_queue_draw(app->nextPieceArea);
    updateLabels(app);
  }

  if (board->retroModeActive) { board->setHeatLevel(0.5); }

  if (!board->isPaused() && !board->isGameOver() && board->retroModeActive) {
    // 1 in 1000 chance of KGB inspection
    static std::mt19937 rng(std::chrono::system_clock::now().time_since_epoch().count());
    std::uniform_int_distribution<int> dist(1, 1000);

    if (dist(rng) == 1) {
      // Pause the game briefly
      app->board->setPaused(true);

      // Create popup message
      GtkWidget *dialog = gtk_message_dialog_new(
          GTK_WINDOW(app->window),
          GTK_DIALOG_MODAL,
          GTK_MESSAGE_WARNING,
          GTK_BUTTONS_NONE,
          "КГБ ИНСПЕКЦИЯ В ПРОЦЕССЕ...\n(KGB INSPECTION IN PROGRESS...)");

      // Auto-close after 2 seconds
      g_timeout_add(2000,
        [](gpointer user_data) -> gboolean {
          GtkWidget *dialog = static_cast<GtkWidget*>(user_data);
          gtk_window_destroy(GTK_WINDOW(dialog));
          return G_SOURCE_REMOVE;
        },
        dialog);

      // Show dialog (visible by default in GTK4; just present it)
      gtk_window_present(GTK_WINDOW(dialog));

      // Resume the game after 2 seconds
      g_timeout_add(2100,
        [](gpointer user_data) -> gboolean {
          TetrimoneApp *app = static_cast<TetrimoneApp *>(user_data);
          app->board->setPaused(false);
          return G_SOURCE_REMOVE;
        },
        app);
    }
  }
  else if (!board->isPaused() && !board->isGameOver() && board->patrioticModeActive) {
    // 1 in 1776 chance of Freedom Inspection (in honor of 1776!)
    static std::mt19937 rng(std::chrono::system_clock::now().time_since_epoch().count());
    std::uniform_int_distribution<int> dist(1, 1776);

    if (dist(rng) == 1) {
      // Pause the game briefly for patriotic interruption
      app->board->setPaused(true);

      // Create random patriotic popup messages
      const char* freedomInspections[] = {
        "🇺🇸 FREEDOM INSPECTION IN PROGRESS! 🦅\n(Checking your liberty levels...)",
        "📺 COMMERCIAL BREAK! 🍔\n(This freedom brought to you by sponsors!)",
        "🏈 TOUCHDOWN! AMERICA SCORES! 🎯\n(Brief patriotic celebration pause!)",
        "☕ COFFEE BREAK TIME! ⏰\n(Even freedom fighters need caffeine!)",
        "📱 SOCIAL MEDIA NOTIFICATION! 💬\n(Someone liked your freedom post!)",
        "🛒 FLASH SALE ALERT! 💳\n(50% off freedom accessories!)",
        "🎬 MOVIE TRAILER PREVIEW! 🍿\n(Coming soon: BLOCKS 2: FREEDOM EDITION!)",
        "🚗 TRAFFIC UPDATE! 🛣️\n(Highway to freedom temporarily slowed!)",
        "🌮 FOOD TRUCK ALERT! 🚚\n(Taco Tuesday freedom fuel available!)",
        "📺 BREAKING NEWS! 📰\n(Local gamer achieves blocks and liberty!)"
      };

      // Select random message
      std::uniform_int_distribution<int> msgDist(0, 9);
      const char* selectedMessage = freedomInspections[msgDist(rng)];

      // Create popup message with American style
      GtkWidget *dialog = gtk_message_dialog_new(
          GTK_WINDOW(app->window),
          GTK_DIALOG_MODAL,
          GTK_MESSAGE_INFO, // Info instead of warning - more positive!
          GTK_BUTTONS_NONE,
          "%s", selectedMessage);

      // Auto-close after 2.5 seconds (slightly longer for American attention spans)
      g_timeout_add(2500,
        [](gpointer user_data) -> gboolean {
          GtkWidget *dialog = static_cast<GtkWidget*>(user_data);
          gtk_window_destroy(GTK_WINDOW(dialog));
          return G_SOURCE_REMOVE;
        },
        dialog);

      gtk_window_present(GTK_WINDOW(dialog));

      // Resume the game after 2.6 seconds
      g_timeout_add(2600,
        [](gpointer user_data) -> gboolean {
          TetrimoneApp *app = static_cast<TetrimoneApp *>(user_data);
          app->board->setPaused(false);
          return G_SOURCE_REMOVE;
        },
        app);
    }
  }
  return true; // Keep the timer running
}

void updateLabels(TetrimoneApp *app) {
  TetrimoneBoard *board = app->board;

  // Update score label with Soviet-style text in retro mode
  std::string score_text;
  if (board->retroModeActive) {
    score_text = "<b>Партийная Лояльность:</b> " + std::to_string(board->getScore()) + "%";
  } else {
    score_text = "<b>Score:</b> " + std::to_string(board->getScore());
  }
  gtk_label_set_markup(GTK_LABEL(app->scoreLabel), score_text.c_str());

  // Update level label with Soviet-style text in retro mode
  std::string level_text;
  if (board->retroModeActive) {
    level_text = "<b>Пятилетка:</b> " + std::to_string(board->getLevel());
  } else {
    level_text = "<b>Level:</b> " + std::to_string(board->getLevel());
  }
  gtk_label_set_markup(GTK_LABEL(app->levelLabel), level_text.c_str());

  // Update lines label with Soviet propaganda in retro mode
  std::string lines_text;
  if (board->retroModeActive) {
    lines_text = "<b>Уничтожено врагов народа:</b> " + std::to_string(board->getLinesCleared());
  } else {
    lines_text = "<b>Lines:</b> " + std::to_string(board->getLinesCleared());
  }
  gtk_label_set_markup(GTK_LABEL(app->linesLabel), lines_text.c_str());

  // Update sequence label
  std::string sequence_text;
  if (board->isSequenceActive() && board->getConsecutiveClears() > 1) {
    if (board->retroModeActive) {
      sequence_text = "<b>Коллективная эффективность:</b> " +
        std::to_string(board->getConsecutiveClears()) + " (Рекорд: " +
        std::to_string(board->getMaxConsecutiveClears()) + ")";
    } else {
      sequence_text = "<b>Sequence:</b> " + std::to_string(board->getConsecutiveClears()) +
        " (Max: " + std::to_string(board->getMaxConsecutiveClears()) + ")";
    }
    // Make it stand out when active
    sequence_text = "<span foreground='#00AA00'>" + sequence_text + "</span>";
  } else {
    if (board->retroModeActive) {
      sequence_text = "<b>Коллективная эффективность:</b> " +
        std::to_string(board->getConsecutiveClears()) + " (Рекорд: " +
        std::to_string(board->getMaxConsecutiveClears()) + ")";
    } else {
      sequence_text = "<b>Sequence:</b> " + std::to_string(board->getConsecutiveClears()) +
        " (Max: " + std::to_string(board->getMaxConsecutiveClears()) + ")";
    }
  }
  gtk_label_set_markup(GTK_LABEL(app->sequenceLabel), sequence_text.c_str());
}

void resetUI(TetrimoneApp *app) {
  // Reset all UI elements to their initial state
  gtk_widget_queue_draw(app->gameArea);
  gtk_widget_queue_draw(app->nextPieceArea);

  // Reset labels
  gtk_label_set_markup(GTK_LABEL(app->scoreLabel), "<b>Score:</b> 0");
  gtk_label_set_markup(GTK_LABEL(app->levelLabel), "<b>Level:</b> 1");
  gtk_label_set_markup(GTK_LABEL(app->linesLabel), "<b>Lines:</b> 0");

  // Update menu/action state (was gtk_widget_set_sensitive on GtkMenuItems)
  g_simple_action_set_enabled(app->startAction, FALSE);
  g_simple_action_set_enabled(app->pauseAction, TRUE);
  app->pauseActionLabel = "Pause";
}

void cleanupApp(gpointer data) {
  TetrimoneApp *app = static_cast<TetrimoneApp *>(data);

  // Stop timers first to prevent any race conditions
  if (app->timerId > 0) {
    g_source_remove(app->timerId);
    app->timerId = 0;
  }

  // Stop joystick timer before closing the joystick
  if (app->joystickTimerId > 0) {
    g_source_remove(app->joystickTimerId);
    app->joystickTimerId = 0;
  }

  // Close joystick properly
  if (app->joystick != NULL) {
    SDL_JoystickClose(app->joystick);
    app->joystick = NULL;
  }

  // Quit SDL
  if (app->joystickEnabled) {
    SDL_Quit();
    app->joystickEnabled = false;
  }

  // Delete board after all timers are stopped
  delete app->board;
  app->board = NULL;

  // Finally delete the app struct
  delete app;
}

void onScreenSizeChanged(TetrimoneApp *app) {
  // We intentionally avoid recalculating the block size here
  // so that manual resize doesn't affect the game area

  // Just redraw everything with the current block size
  gtk_widget_queue_draw(app->gameArea);
  gtk_widget_queue_draw(app->nextPieceArea);
}

// ----------------------------------------------------------------------------
// Application activation / main window construction.
//
// GTK4 changes exercised here:
//  - "delete-event" -> GtkWindow::close-request (gboolean handler; return
//    TRUE to prevent the close, FALSE to allow it - opposite convention from
//    GTK4's delete-event where TRUE meant "don't destroy").
//  - "focus-in-event"/"focus-out-event" on a plain widget are gone; window
//    focus is tracked with a GtkEventControllerFocus's "enter"/"leave".
//  - gtk_container_add() -> gtk_window_set_child() / gtk_frame_set_child().
//  - gtk_container_set_border_width() -> per-widget margins.
//  - gtk_box_pack_start(box, child, expand, fill, padding) ->
//    gtk_box_append(box, child); expand/fill are now hexpand/vexpand
//    properties on the child, padding becomes margins.
//  - "draw" signal -> gtk_drawing_area_set_draw_func().
//  - "key-press-event"/"key-release-event" -> GtkEventControllerKey.
//  - gtk_widget_show_all() doesn't exist; widgets are visible by default,
//    so we just gtk_window_present() at the end.
// ----------------------------------------------------------------------------

void onAppActivate(GtkApplication *app, gpointer userData) {
  (void)userData;
  TetrimoneApp *tetrimoneApp = new TetrimoneApp();
  tetrimoneApp->app = app;
  tetrimoneApp->board = new TetrimoneBoard();
  tetrimoneApp->board->setApp(tetrimoneApp);
  tetrimoneApp->timerId = 0;
  tetrimoneApp->dropSpeed = INITIAL_SPEED;
  tetrimoneApp->difficulty = 2; // Default to Medium

  // Calculate block size based on screen resolution
  calculateBlockSize(tetrimoneApp);

  // Create the main window
  tetrimoneApp->window = gtk_application_window_new(app);
  gtk_window_set_title(GTK_WINDOW(tetrimoneApp->window), "Tetrimone");

  setWindowIcon(GTK_WINDOW(tetrimoneApp->window));

  // Get command line arguments
  CommandLineArgs* args = static_cast<CommandLineArgs*>(
      g_object_get_data(G_OBJECT(app), "cmdline-args"));

  // Apply grid dimensions before calculating block size
  if (args && args->gridWidth != -1) {
      GRID_WIDTH = args->gridWidth;
  }
  if (args && args->gridHeight != -1) {
      GRID_HEIGHT = args->gridHeight;
  }

  // Calculate block size based on screen resolution (unless overridden)
  if (!args || args->blockSize == -1) {
      calculateBlockSize(tetrimoneApp);
  } else {
      BLOCK_SIZE = args->blockSize;
  }

  g_signal_connect(G_OBJECT(tetrimoneApp->window), "close-request",
                   G_CALLBACK(onCloseRequest), tetrimoneApp);

  // Window focus tracking via GtkEventControllerFocus (replaces
  // focus-in-event/focus-out-event, which no longer exist on GtkWindow).
  tetrimoneApp->focusController = gtk_event_controller_focus_new();
  g_signal_connect(tetrimoneApp->focusController, "enter",
                   G_CALLBACK(onWindowFocusEnter), tetrimoneApp);
  g_signal_connect(tetrimoneApp->focusController, "leave",
                   G_CALLBACK(onWindowFocusLeave), tetrimoneApp);
  gtk_widget_add_controller(tetrimoneApp->window, tetrimoneApp->focusController);

  // Use the calculated block size for window dimensions
  gtk_window_set_default_size(GTK_WINDOW(tetrimoneApp->window),
                              GRID_WIDTH * BLOCK_SIZE + 200,
                              GRID_HEIGHT * BLOCK_SIZE + 40);
  gtk_window_set_resizable(GTK_WINDOW(tetrimoneApp->window), FALSE);

  // Create main vertical box
  GtkWidget *mainVBox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
  gtk_window_set_child(GTK_WINDOW(tetrimoneApp->window), mainVBox);

  // Create menu (GMenu model + GtkPopoverMenuBar; see createMenu() below)
  createMenu(tetrimoneApp);
  gtk_box_append(GTK_BOX(mainVBox), tetrimoneApp->menuBar);

  // Create main horizontal box for game contents
  tetrimoneApp->mainBox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 10);
  gtk_widget_set_margin_start(tetrimoneApp->mainBox, 10);
  gtk_widget_set_margin_end(tetrimoneApp->mainBox, 10);
  gtk_widget_set_margin_top(tetrimoneApp->mainBox, 10);
  gtk_widget_set_margin_bottom(tetrimoneApp->mainBox, 10);
  gtk_widget_set_vexpand(tetrimoneApp->mainBox, TRUE);
  gtk_widget_set_hexpand(tetrimoneApp->mainBox, TRUE);
  gtk_box_append(GTK_BOX(mainVBox), tetrimoneApp->mainBox);

  // NOTE: GTK4 has no "size-allocate" signal on plain widgets any more.
  // Since the window is created non-resizable above, the old
  // onScreenSizeChanged() redraw-on-resize handler has nothing to react to;
  // it's kept as a plain function (see above) in case a future resizable
  // mode wants to call it from "notify::default-width".

  // Create the game area (drawing area)
  tetrimoneApp->gameArea = gtk_drawing_area_new();
  gtk_widget_set_size_request(tetrimoneApp->gameArea, GRID_WIDTH * BLOCK_SIZE,
                              GRID_HEIGHT * BLOCK_SIZE);
  gtk_drawing_area_set_draw_func(GTK_DRAWING_AREA(tetrimoneApp->gameArea),
                                 onDrawGameArea, tetrimoneApp, nullptr);
  gtk_box_append(GTK_BOX(tetrimoneApp->mainBox), tetrimoneApp->gameArea);

  // Create the side panel (vertical box)
  GtkWidget *sideBox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 10);
  gtk_box_append(GTK_BOX(tetrimoneApp->mainBox), sideBox);

  // Create the next piece preview frame
  GtkWidget *nextPieceFrame = gtk_frame_new("Next Piece");
  gtk_box_append(GTK_BOX(sideBox), nextPieceFrame);

  // Create the next piece drawing area
  tetrimoneApp->nextPieceArea = gtk_drawing_area_new();
  gtk_widget_set_size_request(tetrimoneApp->nextPieceArea, 3 * 2 * BLOCK_SIZE,
                              2.5 * BLOCK_SIZE);
  gtk_drawing_area_set_draw_func(GTK_DRAWING_AREA(tetrimoneApp->nextPieceArea),
                                 onDrawNextPiece, tetrimoneApp, nullptr);
  gtk_frame_set_child(GTK_FRAME(nextPieceFrame), tetrimoneApp->nextPieceArea);

  // Create score, level, and lines labels
  tetrimoneApp->scoreLabel = gtk_label_new(NULL);
  gtk_label_set_markup(GTK_LABEL(tetrimoneApp->scoreLabel), "<b>Score:</b> 0");
  gtk_widget_set_halign(tetrimoneApp->scoreLabel, GTK_ALIGN_START);
  gtk_box_append(GTK_BOX(sideBox), tetrimoneApp->scoreLabel);

  tetrimoneApp->levelLabel = gtk_label_new(NULL);
  gtk_label_set_markup(GTK_LABEL(tetrimoneApp->levelLabel), "<b>Level:</b> 1");
  gtk_widget_set_halign(tetrimoneApp->levelLabel, GTK_ALIGN_START);
  gtk_box_append(GTK_BOX(sideBox), tetrimoneApp->levelLabel);

  tetrimoneApp->linesLabel = gtk_label_new(NULL);
  gtk_label_set_markup(GTK_LABEL(tetrimoneApp->linesLabel), "<b>Lines:</b> 0");
  gtk_widget_set_halign(tetrimoneApp->linesLabel, GTK_ALIGN_START);
  gtk_box_append(GTK_BOX(sideBox), tetrimoneApp->linesLabel);

  tetrimoneApp->sequenceLabel = gtk_label_new(NULL);
  gtk_label_set_markup(GTK_LABEL(tetrimoneApp->sequenceLabel),
                       "<b>Sequence:</b> 0 (Max: 0)");
  gtk_widget_set_halign(tetrimoneApp->sequenceLabel, GTK_ALIGN_START);
  gtk_box_append(GTK_BOX(sideBox), tetrimoneApp->sequenceLabel);

  // Add difficulty label
  tetrimoneApp->difficultyLabel = gtk_label_new(NULL);
  gtk_label_set_markup(GTK_LABEL(tetrimoneApp->difficultyLabel),
                       tetrimoneApp->board->getDifficultyText(tetrimoneApp->difficulty).c_str());
  gtk_widget_set_halign(tetrimoneApp->difficultyLabel, GTK_ALIGN_START);
  gtk_box_append(GTK_BOX(sideBox), tetrimoneApp->difficultyLabel);

  // Add controls info
  GtkWidget *controlsHeaderLabel = gtk_label_new(NULL);
  if (tetrimoneApp->board->retroModeActive) {
    gtk_label_set_markup(GTK_LABEL(controlsHeaderLabel), "<b>ПАРТИЙНЫЕ ДИРЕКТИВЫ</b>");
  } else {
    gtk_label_set_markup(GTK_LABEL(controlsHeaderLabel), "<b>Controls</b>");
  }
  gtk_widget_set_halign(controlsHeaderLabel, GTK_ALIGN_START);
  gtk_widget_set_margin_top(controlsHeaderLabel, 10);
  gtk_box_append(GTK_BOX(sideBox), controlsHeaderLabel);
  tetrimoneApp->controlsHeaderLabel = controlsHeaderLabel;

  tetrimoneApp->controlsLabel = gtk_label_new(
    tetrimoneApp->board->retroModeActive ?
    "Управление клавиатурой:\n"
    "• Влево/Вправо/A/D: Перемещение блока\n"
    "• Вверх/W: Поворот по часовой стрелке\n"
    "• Z: Поворот против часовой стрелки\n"
    "• Вниз/S: Мягкое падение\n"
    "• Пробел: Быстрое падение\n"
    "• P: Пауза/Продолжение игры\n"
    "• R: Перезапуск игры\n"
    "• M: Переключение музыки\n\n"
    "Поддержка контроллера доступна.\n"
    "Настройка в меню Управление." :
    "Keyboard Controls:\n"
    "• Left/Right/A/D: Move block\n"
    "• Up/W: Rotate clockwise\n"
    "• Z: Rotate counter-clockwise\n"
    "• Down/S: Soft drop\n"
    "• Space: Hard drop\n"
    "• P: Pause/Resume game\n"
    "• R: Restart game\n"
    "• M: Toggle music\n\n"
    "Controller support is available.\n"
    "Configure in Controls menu.");

  gtk_widget_set_halign(tetrimoneApp->controlsLabel, GTK_ALIGN_START);
  gtk_box_append(GTK_BOX(sideBox), tetrimoneApp->controlsLabel);

  // Set up key events via GtkEventControllerKey (replaces
  // gtk_widget_add_events + key-press-event/key-release-event).
  tetrimoneApp->keyController = gtk_event_controller_key_new();
  g_signal_connect(tetrimoneApp->keyController, "key-pressed",
                   G_CALLBACK(onKeyPressed), tetrimoneApp);
  g_signal_connect(tetrimoneApp->keyController, "key-released",
                   G_CALLBACK(onKeyReleased), tetrimoneApp);
  gtk_widget_add_controller(tetrimoneApp->window, tetrimoneApp->keyController);

  // Connect cleanup function
  g_object_set_data_full(G_OBJECT(tetrimoneApp->window), "app-data",
                       tetrimoneApp, cleanupApp);

  // Present the window (gtk_widget_show_all no longer exists; GTK4 widgets
  // are visible by default, so presenting the window is all that's needed).
  gtk_window_present(GTK_WINDOW(tetrimoneApp->window));

  // Initialize the menu/action state
  g_simple_action_set_enabled(tetrimoneApp->startAction, FALSE);
  g_simple_action_set_enabled(tetrimoneApp->pauseAction, TRUE);

  if (tetrimoneApp->board->initializeAudio()) {
    // Only play music if initialization was successful
    tetrimoneApp->board->playBackgroundMusic();
    tetrimoneApp->backgroundMusicPlaying = true;
  } else {
    printf("Music failed to initialize");
    // Disable sound menu item
    g_simple_action_set_state(tetrimoneApp->soundToggleAction, g_variant_new_boolean(FALSE));
  }

  tetrimoneApp->joystick = NULL;
  tetrimoneApp->joystickEnabled = false;
  tetrimoneApp->joystickTimerId = 0;

  if (args) {
    applyCommandLineArgs(tetrimoneApp, *args);
  }

  // Try to initialize SDL
  initSDL(tetrimoneApp);

  // Start the game
  startGame(tetrimoneApp);
}

// GtkWindow::close-request replaces GtkWidget's "delete-event". Same return
// convention: TRUE prevents the close, FALSE allows it.
gboolean onCloseRequest(GtkWindow *window, gpointer userData) {
  (void)window;
#ifdef DEBUG
  std::cerr << "DEBUG: onCloseRequest called" << std::endl;
#endif
  TetrimoneApp *app = static_cast<TetrimoneApp *>(userData);
#ifdef DEBUG
  std::cerr << "DEBUG: Stopping timers" << std::endl;
#endif
  // Stop timers first to prevent any callbacks during cleanup
  if (app->timerId > 0) {
    g_source_remove(app->timerId);
    app->timerId = 0;
  }

  if (app->joystickTimerId > 0) {
    g_source_remove(app->joystickTimerId);
    app->joystickTimerId = 0;
  }

#ifdef DEBUG
  std::cerr << "DEBUG: Stopping audio" << std::endl;
#endif
  // Stop and cleanup audio
  if (app->board) {
    if (app->backgroundMusicPlaying) {
#ifdef DEBUG
      std::cerr << "DEBUG: Pausing background music" << std::endl;
#endif
      app->board->pauseBackgroundMusic();
      app->backgroundMusicPlaying = false;
    }
#ifdef DEBUG
    std::cerr << "DEBUG: Cleaning up audio" << std::endl;
#endif
    app->board->cleanupAudio();
  }
#ifdef DEBUG
  std::cerr << "DEBUG: Cleaning up joystick" << std::endl;
#endif
  // Close joystick
  if (app->joystick != NULL) {
#ifdef DEBUG
    std::cerr << "DEBUG: Closing joystick" << std::endl;
#endif
    SDL_JoystickClose(app->joystick);
    app->joystick = NULL;
  }

  // Quit SDL
  if (app->joystickEnabled) {
#ifdef DEBUG
    std::cerr << "DEBUG: Quitting SDL" << std::endl;
#endif
    SDL_Quit();
    app->joystickEnabled = false;
  }

#ifdef DEBUG
  std::cerr << "DEBUG: Returning FALSE to let GTK handle window destruction"
            << std::endl;
#endif
  // Allow the window to close
  return FALSE;
}

// Window focus tracking (replaces the combined onWindowFocusChanged, which
// handled both focus-in-event and focus-out-event via GdkEventFocus - a
// type that no longer exists in GTK4).
void onWindowFocusLeave(GtkEventControllerFocus *controller, gpointer userData) {
  (void)controller;
  TetrimoneApp *app = static_cast<TetrimoneApp *>(userData);

  if (!app->board->isPaused() && !app->board->isGameOver() &&
      !app->board->isSplashScreenActive()) {
    // Store the pause state before pausing
    app->pausedByFocusLoss = true;

    // Call the existing pause function to ensure proper behavior
    onPauseGame(app->pauseAction, nullptr, app);

    // Don't change the label from "Resume" - this is a special pause
    app->pauseActionLabel = "Resume";
  }
}

void onWindowFocusEnter(GtkEventControllerFocus *controller, gpointer userData) {
  (void)controller;
  TetrimoneApp *app = static_cast<TetrimoneApp *>(userData);

  if (!app->board->isGameOver() && !app->board->isSplashScreenActive() &&
      app->pausedByFocusLoss) {
    app->pausedByFocusLoss = false;

    // Only resume if still paused (user might have manually interacted with pause)
    if (app->board->isPaused()) {
      onPauseGame(app->pauseAction, nullptr, app);
    }
  }
}

// ----------------------------------------------------------------------------
// Menu bar construction.
//
// GTK4 removed GtkMenuBar / GtkMenu / GtkMenuItem / GtkRadioMenuItem /
// GtkCheckMenuItem completely - there is no compatibility shim, so this is a
// full redesign rather than a rename. The replacement:
//
//   - A GMenu is a plain data model describing the menu structure (labels,
//     submenus, separators). It has no "toggled"/"activate" signals itself.
//   - Each item's behavior is a GAction (we use GSimpleAction) that lives in
//     a GSimpleActionGroup, inserted on the window as "game" (so menu items
//     reference actions as "game.start", "game.difficulty", etc.)
//   - Checkboxes become stateful boolean actions (state g_variant_new_boolean).
//   - Radio groups become ONE stateful string/int action; each radio "item"
//     in the GMenu targets that action with a specific target value via
//     g_menu_item_set_action_and_target_value(). GTK renders these as radio
//     buttons automatically based on the target/state match.
//   - Enabled/disabled ("gtk_widget_set_sensitive") becomes
//     g_simple_action_set_enabled().
//   - The whole model is displayed by a GtkPopoverMenuBar
//     (gtk_popover_menu_bar_new_from_model), which replaces GtkMenuBar.
// ----------------------------------------------------------------------------

// Helper to add a simple (non-stateful) action to the app's action group.
static GSimpleAction* addSimpleAction(TetrimoneApp *app, const char *name,
                                       GCallback callback) {
  GSimpleAction *action = g_simple_action_new(name, nullptr);
  g_signal_connect(action, "activate", callback, app);
  g_action_map_add_action(G_ACTION_MAP(app->actionGroup), G_ACTION(action));
  return action;
}

// Helper to add a stateful boolean (checkbox) action.
static GSimpleAction* addBoolAction(TetrimoneApp *app, const char *name,
                                     bool initialState, GCallback callback) {
  GSimpleAction *action = g_simple_action_new_stateful(
      name, nullptr, g_variant_new_boolean(initialState));
  g_signal_connect(action, "change-state", callback, app);
  g_action_map_add_action(G_ACTION_MAP(app->actionGroup), G_ACTION(action));
  return action;
}

// Helper to add a stateful string (radio group) action.
static GSimpleAction* addRadioAction(TetrimoneApp *app, const char *name,
                                      const char *initialState, GCallback callback) {
  GSimpleAction *action = g_simple_action_new_stateful(
      name, G_VARIANT_TYPE_STRING, g_variant_new_string(initialState));
  g_signal_connect(action, "change-state", callback, app);
  g_action_map_add_action(G_ACTION_MAP(app->actionGroup), G_ACTION(action));
  return action;
}

void createMenu(TetrimoneApp *app) {
  app->actionGroup = g_simple_action_group_new();

  GMenu *menuModel = g_menu_new();

  // *** GAME MENU ***
  GMenu *gameMenu = g_menu_new();
  app->startAction = addSimpleAction(app, "start", G_CALLBACK(onStartGame));
  app->pauseAction = addSimpleAction(app, "pause", G_CALLBACK(onPauseGame));
  app->restartAction = addSimpleAction(app, "restart", G_CALLBACK(onRestartGame));
  addSimpleAction(app, "quit", G_CALLBACK(onQuitGame));
  addSimpleAction(app, "high-scores", G_CALLBACK(onViewHighScores));

  g_menu_append(gameMenu, "Start", "game.start");
  g_menu_append(gameMenu, "Pause", "game.pause");
  g_menu_append(gameMenu, "Restart", "game.restart");
  g_menu_append(gameMenu, "High Scores", "game.high-scores");

  GMenu *gameQuitSection = g_menu_new();
  g_menu_append(gameQuitSection, "Quit", "game.quit");
  g_menu_append_section(gameMenu, nullptr, G_MENU_MODEL(gameQuitSection));
  g_object_unref(gameQuitSection);

  g_menu_append_submenu(menuModel, "Game", G_MENU_MODEL(gameMenu));
  g_object_unref(gameMenu);

  // *** DIFFICULTY MENU *** (radio group backed by a single string action)
  GMenu *difficultyMenu = g_menu_new();
  app->difficultyAction = addRadioAction(app, "difficulty", "medium",
                                         G_CALLBACK(onDifficultyChanged));
  app->difficulty = 2; // Medium

  auto appendRadioItem = [](GMenu *menu, const char *label, const char *actionName,
                             const char *target) {
    GMenuItem *item = g_menu_item_new(label, nullptr);
    g_menu_item_set_action_and_target_value(item, actionName, g_variant_new_string(target));
    g_menu_append_item(menu, item);
    g_object_unref(item);
  };

  appendRadioItem(difficultyMenu, "Zen", "game.difficulty", "zen");
  appendRadioItem(difficultyMenu, "Easy", "game.difficulty", "easy");
  appendRadioItem(difficultyMenu, "Medium", "game.difficulty", "medium");
  appendRadioItem(difficultyMenu, "Hard", "game.difficulty", "hard");
  appendRadioItem(difficultyMenu, "Extreme", "game.difficulty", "extreme");
  appendRadioItem(difficultyMenu, "Insane", "game.difficulty", "insane");

  g_menu_append_submenu(menuModel, "Difficulty", G_MENU_MODEL(difficultyMenu));
  g_object_unref(difficultyMenu);

  // *** GRAPHICS MENU ***
  GMenu *graphicsMenu = g_menu_new();

  addSimpleAction(app, "block-size-dialog", G_CALLBACK(onBlockSizeDialog));
  g_menu_append(graphicsMenu, "Block Size...", "game.block-size-dialog");

  // Background submenu
  GMenu *backgroundMenu = g_menu_new();
  addSimpleAction(app, "background-image-dialog", G_CALLBACK(onBackgroundImageDialog));
  g_menu_append(backgroundMenu, "Set Background Image ...", "game.background-image-dialog");

  addSimpleAction(app, "background-zip-dialog", G_CALLBACK(onBackgroundZipDialog));
  g_menu_append(backgroundMenu, "Set Background Images from ZIP...", "game.background-zip-dialog");

  addSimpleAction(app, "background-opacity-dialog", G_CALLBACK(onBackgroundOpacityDialog));
  g_menu_append(backgroundMenu, "Background Opacity...", "game.background-opacity-dialog");

  app->backgroundToggleAction = addBoolAction(app, "background-toggle", true,
                                              G_CALLBACK(onBackgroundToggled));
  g_menu_append(backgroundMenu, "Enable Background Image", "game.background-toggle");

  g_menu_append_submenu(graphicsMenu, "Background", G_MENU_MODEL(backgroundMenu));
  g_object_unref(backgroundMenu);

  app->gridLinesToggleAction = addBoolAction(app, "grid-lines-toggle",
                                             app->board->isShowingGridLines(),
                                             G_CALLBACK(onGridLinesToggled));
  g_menu_append(graphicsMenu, "Show Grid Lines", "game.grid-lines-toggle");

  app->blockTrailsToggleAction = addBoolAction(app, "block-trails-toggle",
                                               app->board->isTrailsEnabled(),
                                               G_CALLBACK(onBlockTrailsToggled));
  g_menu_append(graphicsMenu, "Block Trails", "game.block-trails-toggle");

  addSimpleAction(app, "block-trails-config", G_CALLBACK(onBlockTrailsConfig));
  g_menu_append(graphicsMenu, "Block Trails Settings...", "game.block-trails-config");

  // Color themes submenu (radio group backed by a single int-encoded-as-string action)
  GMenu *themeMenu = g_menu_new();
  const char* themeNames[] = {
    "Watercolor", "Neon", "Pastel", "Earth Tones", "Monochrome Blue",
    "Monochrome Green", "Sunset", "Ocean", "Grayscale", "Candy",
    "Neon Dark", "Jewel Tones", "Retro Gaming", "Autumn", "Winter",
    "Spring", "Summer", "Monochrome Purple", "Desert", "Rainbow",
    "Art Deco", "Northern Lights", "Moroccan Tiles", "Bioluminescence",
    "Fossil", "Silk Road", "Digital Glitch", "Botanical", "Jazz Age",
    "Steampunk", "USA", "Soviet Retro"
  };

  char initialTheme[16];
  snprintf(initialTheme, sizeof(initialTheme), "%d", currentThemeIndex);
  app->themeAction = addRadioAction(app, "theme", initialTheme, G_CALLBACK(onThemeChanged));

  for (int i = 0; i < NUM_COLOR_THEMES; i++) {
    char target[16];
    snprintf(target, sizeof(target), "%d", i);
    appendRadioItem(themeMenu, themeNames[i], "game.theme", target);
  }
  g_menu_append_submenu(graphicsMenu, "Color Themes", G_MENU_MODEL(themeMenu));
  g_object_unref(themeMenu);

  app->ghostPieceToggleAction = addBoolAction(app, "ghost-piece-toggle",
                                              app->board->isGhostPieceEnabled(),
                                              G_CALLBACK(onGhostPieceToggled));
  g_menu_append(graphicsMenu, "Show Ghost Piece", "game.ghost-piece-toggle");

  app->simpleBlocksToggleAction = addBoolAction(app, "simple-blocks-toggle",
                                                app->board->simpleBlocksActive,
                                                G_CALLBACK(onSimpleBlocksToggled));
  g_menu_append(graphicsMenu, "Simple Blocks (No 3D Effect)", "game.simple-blocks-toggle");

  g_menu_append_submenu(menuModel, "Graphics", G_MENU_MODEL(graphicsMenu));
  g_object_unref(graphicsMenu);

  // *** SOUND MENU ***
  GMenu *soundMenu = g_menu_new();

  app->soundToggleAction = addBoolAction(app, "sound-toggle", true, G_CALLBACK(onSoundToggled));
  g_menu_append(soundMenu, "Enable Sound", "game.sound-toggle");

  addSimpleAction(app, "volume-dialog", G_CALLBACK(onVolumeDialog));
  g_menu_append(soundMenu, "Volume Settings...", "game.volume-dialog");

  app->retroMusicToggleAction = addBoolAction(app, "retro-music-toggle",
                                              app->board->retroMusicActive,
                                              G_CALLBACK(onRetroMusicToggled));
  g_menu_append(soundMenu, "Use Retro Music", "game.retro-music-toggle");

  // Music tracks submenu - 5 independent checkboxes
  GMenu *musicMenu = g_menu_new();
  for (int i = 0; i < 5; i++) {
    char label[20];
    snprintf(label, sizeof(label), "Track %d", i + 1);
    char actionName[24];
    snprintf(actionName, sizeof(actionName), "track-%d-toggle", i);

    app->trackActions[i] = addBoolAction(app, actionName, true, G_CALLBACK(onTrackToggled));
    g_object_set_data(G_OBJECT(app->trackActions[i]), "track-index", GINT_TO_POINTER(i));

    char detailedAction[40];
    snprintf(detailedAction, sizeof(detailedAction), "game.%s", actionName);
    g_menu_append(musicMenu, label, detailedAction);
  }
  g_menu_append_submenu(soundMenu, "Music Tracks", G_MENU_MODEL(musicMenu));
  g_object_unref(musicMenu);

  g_menu_append_submenu(menuModel, "Sound", G_MENU_MODEL(soundMenu));
  g_object_unref(soundMenu);

  // *** CONTROLS MENU ***
  GMenu *controlsMenu = g_menu_new();
  addSimpleAction(app, "joystick-config", G_CALLBACK(onJoystickConfig));
  g_menu_append(controlsMenu, "Configure Joystick...", "game.joystick-config");
  g_menu_append_submenu(menuModel, "Controls", G_MENU_MODEL(controlsMenu));
  g_object_unref(controlsMenu);

  // *** RULES MENU ***
  GMenu *rulesMenu = g_menu_new();

  // Minimum Block Size submenu (radio group)
  GMenu *blockSizeRulesMenu = g_menu_new();
  char initialMinBlockSize[16];
  snprintf(initialMinBlockSize, sizeof(initialMinBlockSize), "%d", app->board->getMinBlockSize());
  app->minBlockSizeAction = addRadioAction(app, "min-block-size", initialMinBlockSize,
                                           G_CALLBACK(onBlockSizeRulesChanged));

  appendRadioItem(blockSizeRulesMenu,
                  "1:  No Single, Double, or Triple Blocks; only Quadruple Blocks",
                  "game.min-block-size", "4");
  appendRadioItem(blockSizeRulesMenu, "2:   No Single or Double Blocks",
                  "game.min-block-size", "3");
  appendRadioItem(blockSizeRulesMenu, "3:  No Single Blocks",
                  "game.min-block-size", "2");
  appendRadioItem(blockSizeRulesMenu,
                  "4:  Single, Double, Triple, and Quadruple Blocks",
                  "game.min-block-size", "1");

  g_menu_append_submenu(rulesMenu, "Minimum Block Size", G_MENU_MODEL(blockSizeRulesMenu));
  g_object_unref(blockSizeRulesMenu);

  addSimpleAction(app, "game-size-dialog", G_CALLBACK(onGameSizeDialog));
  g_menu_append(rulesMenu, "Game Size", "game.game-size-dialog");

  addSimpleAction(app, "game-setup-dialog", G_CALLBACK(onGameSetupDialog));
  g_menu_append(rulesMenu, "Game Setup", "game.game-setup-dialog");

  g_menu_append_submenu(menuModel, "Rules", G_MENU_MODEL(rulesMenu));
  g_object_unref(rulesMenu);

  // *** HELP MENU ***
  GMenu *helpMenu = g_menu_new();
  addSimpleAction(app, "instructions-dialog", G_CALLBACK(onInstructionsDialog));
  g_menu_append(helpMenu, "Instructions", "game.instructions-dialog");
  addSimpleAction(app, "about-dialog", G_CALLBACK(onAboutDialog));
  g_menu_append(helpMenu, "About", "game.about-dialog");
  g_menu_append_submenu(menuModel, "Help", G_MENU_MODEL(helpMenu));
  g_object_unref(helpMenu);

  // Insert the action group on the window as "game" (so items reference
  // actions as "game.xxx" above) and build the popover menu bar widget.
  gtk_widget_insert_action_group(app->window, "game", G_ACTION_GROUP(app->actionGroup));

  app->menuBar = gtk_popover_menu_bar_new_from_model(G_MENU_MODEL(menuModel));
  g_object_unref(menuModel);
}

// ----------------------------------------------------------------------------
// Toggle / radio action callbacks. These are connected to "change-state" (see
// createMenu() above), so unlike a plain "activate" handler, each one is
// responsible for calling g_simple_action_set_state() itself if it wants the
// menu item's visual check/radio state to actually change. If a handler
// returns without calling it (e.g. the user declined a confirmation dialog),
// the item snaps back to its previous state automatically - which replaces
// the old GTK4 "block signal handler, reselect previous item" dance.
// ----------------------------------------------------------------------------

void onGridLinesToggled(GSimpleAction *action, GVariant *value, gpointer userData) {
  TetrimoneApp *app = static_cast<TetrimoneApp *>(userData);
  bool showLines = g_variant_get_boolean(value);

  app->board->setShowGridLines(showLines);
  g_simple_action_set_state(action, value);

  gtk_widget_queue_draw(app->gameArea);
}

void onThemeChanged(GSimpleAction *action, GVariant *value, gpointer userData) {
  TetrimoneApp *app = static_cast<TetrimoneApp *>(userData);

  int newThemeIndex = atoi(g_variant_get_string(value, nullptr));

  // Don't change if it's the same theme
  if (newThemeIndex == currentThemeIndex) {
    return;
  }

  // Start smooth theme transition
  app->board->startThemeTransition(newThemeIndex);

  // Also trigger background transition if using background zip
  if (app->board->isUsingBackgroundZip() && !app->board->backgroundImages.empty()) {
    app->board->startBackgroundTransition();
  }

  g_simple_action_set_state(action, value);

  gtk_widget_queue_draw(app->gameArea);
  gtk_widget_queue_draw(app->nextPieceArea);
}

void onSimpleBlocksToggled(GSimpleAction *action, GVariant *value, gpointer userData) {
  TetrimoneApp *app = static_cast<TetrimoneApp *>(userData);
  app->board->simpleBlocksActive = g_variant_get_boolean(value);
  g_simple_action_set_state(action, value);

  gtk_widget_queue_draw(app->gameArea);
  gtk_widget_queue_draw(app->nextPieceArea);
}

void onRetroMusicToggled(GSimpleAction *action, GVariant *value, gpointer userData) {
  TetrimoneApp *app = static_cast<TetrimoneApp *>(userData);
  app->board->retroMusicActive = g_variant_get_boolean(value);
  g_simple_action_set_state(action, value);

  // If music is playing, restart it to apply the change
  if (app->backgroundMusicPlaying && app->board->sound_enabled_) {
    app->board->pauseBackgroundMusic();
    app->board->playBackgroundMusic(); // This will use retroMusicActive
  }
}

void onBlockSizeRulesChanged(GSimpleAction *action, GVariant *value, gpointer userData) {
  TetrimoneApp *app = static_cast<TetrimoneApp *>(userData);

  int newMinBlockSize = atoi(g_variant_get_string(value, nullptr));
  int currentMinBlockSize = app->board->getMinBlockSize();

  // No-op if unchanged
  if (newMinBlockSize == currentMinBlockSize) {
    g_simple_action_set_state(action, value);
    return;
  }

  // Create confirmation dialog
  GtkWidget *dialog = gtk_message_dialog_new(
      GTK_WINDOW(app->window), GTK_DIALOG_MODAL, GTK_MESSAGE_QUESTION,
      GTK_BUTTONS_YES_NO,
      "Changing the minimum block size will restart the game. Continue?");

  gint response = gtk_dialog_run_gtk4(dialog);
  gtk_window_destroy(GTK_WINDOW(dialog));

  // If user clicked "No", don't set the new state - the radio item reverts
  // to its previous selection automatically since we never call
  // g_simple_action_set_state().
  if (response != GTK_RESPONSE_YES) {
    return;
  }

  // Apply the new minimum block size
  app->board->setMinBlockSize(newMinBlockSize);
  g_simple_action_set_state(action, value);

  // Restart the game with new settings
  app->board->restart();
  resetUI(app);

  // Start game with new settings
  if (app->board->isPaused()) {
    app->board->togglePause();
    app->pauseActionLabel = "Pause";
  }

  g_simple_action_set_enabled(app->startAction, FALSE);
  g_simple_action_set_enabled(app->pauseAction, TRUE);

  startGame(app);
  gtk_widget_queue_draw(app->gameArea);
  gtk_widget_queue_draw(app->nextPieceArea);
  updateLabels(app);
}

void onTrackToggled(GSimpleAction *action, GVariant *value, gpointer userData) {
  TetrimoneApp *app = static_cast<TetrimoneApp *>(userData);
  int trackIndex = GPOINTER_TO_INT(g_object_get_data(G_OBJECT(action), "track-index"));
  bool newState = g_variant_get_boolean(value);

  // Update the enabled state in the board
  app->board->enabledTracks[trackIndex] = newState;

  // Make sure at least one track is enabled
  bool anyEnabled = false;
  for (int i = 0; i < 5; i++) {
    if (app->board->enabledTracks[i]) {
      anyEnabled = true;
      break;
    }
  }

  // If no tracks are enabled, re-enable this one (refuse the toggle)
  if (!anyEnabled) {
    app->board->enabledTracks[trackIndex] = true;
    g_simple_action_set_state(action, g_variant_new_boolean(TRUE));
    return;
  }

  g_simple_action_set_state(action, value);
}

// ----------------------------------------------------------------------------
// Game flow: start/pause/restart/quit.
//
// NOTE: The GTK4 sources call a toggle-style "onPauseGame(GtkMenuItem*,...)"
// throughout, but that function's actual body was never defined in
// tetrimone_gtk3.cpp - it must live in one of the other translation units
// (gtk3_dialog_helpers.cpp / gtkstuff.cpp per the Makefile's SRCS_COMMON
// list) that wasn't provided for this conversion. The pauseGame() helper
// below IS defined in this file, so it has been faithfully ported. The
// onPauseGame()/unpauseGame() pair here is a best-effort reconstruction of
// the missing toggle wrapper based on how every call site in this file uses
// it (pause if running, resume if paused). If the real implementation in the
// other file does anything more elaborate, port that instead and keep this
// one as a fallback.
// ----------------------------------------------------------------------------

void onStartGame(GSimpleAction *action, GVariant *param, gpointer userData) {
  (void)action; (void)param;
  TetrimoneApp *app = static_cast<TetrimoneApp *>(userData);
  if (app->board->isGameOver()) {
    app->board->restart();
    resetUI(app);
  }
  app->board->setPaused(false);
  startGame(app);

  g_simple_action_set_enabled(app->startAction, FALSE);
  g_simple_action_set_enabled(app->pauseAction, TRUE);

  gtk_widget_queue_draw(app->gameArea);
  gtk_widget_queue_draw(app->nextPieceArea);
  updateLabels(app);
}

void pauseGame(TetrimoneApp *app) {
  // Remove the timer
  if (app->timerId > 0) {
    g_source_remove(app->timerId);
    app->timerId = 0;
  }

  // Pause background music if enabled
  if (app->backgroundMusicPlaying && app->board->sound_enabled_) {
    app->board->pauseBackgroundMusic();
    app->backgroundMusicPlaying = false;
  }

  // Update action state
  g_simple_action_set_enabled(app->startAction, TRUE);
  app->pauseActionLabel = "Resume";
}

void unpauseGame(TetrimoneApp *app) {
  // Resume background music if enabled
  if (!app->backgroundMusicPlaying && app->board->sound_enabled_) {
    app->board->resumeBackgroundMusic();
    app->backgroundMusicPlaying = true;
  }

  // Restart the drop timer
  if (app->timerId == 0) {
    app->timerId = g_timeout_add(app->dropSpeed, onTimerTick, app);
  }

  g_simple_action_set_enabled(app->startAction, FALSE);
  app->pauseActionLabel = "Pause";
}

void onPauseGame(GSimpleAction *action, GVariant *param, gpointer userData) {
  (void)action; (void)param;
  TetrimoneApp *app = static_cast<TetrimoneApp *>(userData);

  if (app->board->isPaused()) {
    app->board->togglePause();
    unpauseGame(app);
  } else {
    app->board->togglePause();
    pauseGame(app);
  }
}

void onRestartGame(GSimpleAction *action, GVariant *param, gpointer userData) {
  (void)action; (void)param;
  TetrimoneApp *app = static_cast<TetrimoneApp *>(userData);
  app->board->restart();
  resetUI(app);

  if (app->board->isPaused()) {
    app->board->togglePause();
    app->pauseActionLabel = "Pause";
  }

  g_simple_action_set_enabled(app->startAction, FALSE);
  g_simple_action_set_enabled(app->pauseAction, TRUE);

  startGame(app);
  gtk_widget_queue_draw(app->gameArea);
  gtk_widget_queue_draw(app->nextPieceArea);
  updateLabels(app);
}

void onQuitGame(GSimpleAction *action, GVariant *param, gpointer userData) {
  (void)action; (void)param;
  TetrimoneApp *app = static_cast<TetrimoneApp *>(userData);
  gtk_window_close(GTK_WINDOW(app->window));
}

void onSoundToggled(GSimpleAction *action, GVariant *value, gpointer userData) {
  TetrimoneApp *app = static_cast<TetrimoneApp *>(userData);
  bool isSoundEnabled = g_variant_get_boolean(value);

  app->board->sound_enabled_ = isSoundEnabled;

  if (isSoundEnabled) {
    // If sound is being turned on, we need to initialize the audio system
    if (!app->board->initializeAudio()) {
      // If initialization fails, don't set the new state (stays off)
      return;
    }

    if (!app->board->isPaused() && !app->board->isGameOver()) {
      app->board->resumeBackgroundMusic();
      app->backgroundMusicPlaying = true;
    }
  } else {
    // When disabling sound, pause the music and clean up audio resources
    app->board->pauseBackgroundMusic();
    app->backgroundMusicPlaying = false;
    app->board->cleanupAudio();
  }

  g_simple_action_set_state(action, value);
}

void onDifficultyChanged(GSimpleAction *action, GVariant *value, gpointer userData) {
  TetrimoneApp *app = static_cast<TetrimoneApp *>(userData);

  const char *target = g_variant_get_string(value, nullptr);
  int previousDifficulty = app->difficulty;
  int newDifficulty = previousDifficulty;

  if (strcmp(target, "zen") == 0) newDifficulty = 0;
  else if (strcmp(target, "easy") == 0) newDifficulty = 1;
  else if (strcmp(target, "medium") == 0) newDifficulty = 2;
  else if (strcmp(target, "hard") == 0) newDifficulty = 3;
  else if (strcmp(target, "extreme") == 0) newDifficulty = 4;
  else if (strcmp(target, "insane") == 0) newDifficulty = 5;

  if (newDifficulty != previousDifficulty) {
    // Don't prompt if we're at the splash screen or game over
    if (!app->board->isSplashScreenActive() && !app->board->isGameOver()) {
      GtkWidget *dialog = gtk_message_dialog_new(
          GTK_WINDOW(app->window), GTK_DIALOG_MODAL, GTK_MESSAGE_QUESTION,
          GTK_BUTTONS_YES_NO,
          "Changing difficulty will start a new game. Continue?");

      gint response = gtk_dialog_run_gtk4(dialog);
      gtk_window_destroy(GTK_WINDOW(dialog));

      // If user clicked "No", don't set the new state - the radio item
      // reverts automatically.
      if (response != GTK_RESPONSE_YES) {
        return;
      }
    }

    // Apply the new difficulty level
    app->difficulty = newDifficulty;
    g_simple_action_set_state(action, value);

    // Update difficulty label
    gtk_label_set_markup(GTK_LABEL(app->difficultyLabel),
                         app->board->getDifficultyText(app->difficulty).c_str());
    // Recalculate drop speed based on difficulty and level
    adjustDropSpeed(app);

    // Restart the game with new difficulty
    app->board->restart();
    resetUI(app);

    if (app->board->isPaused()) {
      app->board->togglePause();
      app->pauseActionLabel = "Pause";
    }

    g_simple_action_set_enabled(app->startAction, FALSE);
    g_simple_action_set_enabled(app->pauseAction, TRUE);

    startGame(app);
    gtk_widget_queue_draw(app->gameArea);
    gtk_widget_queue_draw(app->nextPieceArea);
    updateLabels(app);
  } else {
    g_simple_action_set_state(action, value);
    // If difficulty didn't change, just restart timer with new speed if game is running
    if (!app->board->isPaused() && !app->board->isGameOver() && app->timerId > 0) {
      g_source_remove(app->timerId);
      app->timerId = g_timeout_add(app->dropSpeed, onTimerTick, app);
    }
  }
}

void adjustDropSpeed(TetrimoneApp *app) {
  // Base speed based on level
  int baseSpeed = INITIAL_SPEED - (app->board->getLevel() - 1) * 50;

  // Apply difficulty modifier
  switch (app->difficulty) {
  case 0: // Zen
    app->dropSpeed = 1000;
    break;
  case 1: // Easy
    app->dropSpeed = baseSpeed * 1.5;
    break;
  case 2: // Medium
    app->dropSpeed = baseSpeed;
    break;
  case 3: // Hard
    app->dropSpeed = baseSpeed * 0.7;
    break;
  case 4: // Extreme
    app->dropSpeed = baseSpeed * 0.3;
    break;
  case 5: // Insane
    app->dropSpeed = baseSpeed * 0.1;
    break;
  default:
    app->dropSpeed = baseSpeed;
  }

  // Enforce minimum speed
  if (app->dropSpeed < 10) {
    app->dropSpeed = 10;
  }
}

void startGame(TetrimoneApp *app) {
  // Remove existing timer if any
  if (app->timerId > 0) {
    g_source_remove(app->timerId);
    app->timerId = 0;
  }

  // Resume background music if it was playing
  if (!app->backgroundMusicPlaying && app->board->sound_enabled_) {
    app->board->resumeBackgroundMusic();
    app->backgroundMusicPlaying = true;
  }

  // Calculate drop speed based on level and difficulty
  adjustDropSpeed(app);
  if (app->board->junkLinesPercentage > 0) {
    app->board->generateJunkLines(app->board->junkLinesPercentage);
  }

  if (app->board->junkLinesPerLevel > 0) {
    app->board->addJunkLinesFromBottom(app->board->junkLinesPerLevel);
  }

  // Start a new timer
  app->timerId = g_timeout_add(app->dropSpeed, onTimerTick, app);

  // Update action state
  g_simple_action_set_enabled(app->startAction, FALSE);
  g_simple_action_set_enabled(app->pauseAction, TRUE);
  app->pauseActionLabel = "Pause";
}

// ----------------------------------------------------------------------------
// calculateBlockSize: gdk_display_get_primary_monitor() was removed in
// GTK4 (multi-monitor "primary" concept is gone). The replacement is to
// enumerate gdk_display_get_monitors() (a GListModel) and take the first
// one, or better, the monitor the app's window is actually on via
// gtk_native_get_surface() + gdk_display_get_monitor_at_surface(). Since
// this runs before the window exists yet in onAppActivate(), we fall back
// to the first monitor in the list, which is the closest available
// approximation of the old "primary monitor" behavior.
// ----------------------------------------------------------------------------

void calculateBlockSize(TetrimoneApp *app) {
  (void)app;
  // Get the screen dimensions
  GdkRectangle workarea = {0, 0, 1920, 1080}; // sane fallback
  GdkDisplay *display = gdk_display_get_default();
  if (display) {
    GListModel *monitors = gdk_display_get_monitors(display);
    if (monitors && g_list_model_get_n_items(monitors) > 0) {
      GdkMonitor *monitor = GDK_MONITOR(g_list_model_get_item(monitors, 0));
      if (monitor) {
        gdk_monitor_get_geometry(monitor, &workarea);
        g_object_unref(monitor);
      }
    }
  }

  // Calculate available height and width (accounting for menu and side panel)
  int availableHeight = workarea.height - 100; // Allow for window decorations and menu
  int availableWidth = workarea.width - 300;   // Allow for side panel and margins

  // Calculate block size based on available space and grid dimensions
  int heightBasedSize = availableHeight / GRID_HEIGHT;
  int widthBasedSize = availableWidth / GRID_WIDTH;

  // Use the smaller of the two to ensure the game fits on screen
  BLOCK_SIZE = std::min(heightBasedSize, widthBasedSize);

  // Constrain to min/max values for usability
  BLOCK_SIZE = std::max(BLOCK_SIZE, MIN_BLOCK_SIZE);
  BLOCK_SIZE = std::min(BLOCK_SIZE, MAX_BLOCK_SIZE);
}

void updateSizeValueLabel(GtkRange *range, gpointer data) {
  GtkWidget *label = GTK_WIDGET(data);
  int value = (int)gtk_range_get_value(range);
  char buf[32];
  snprintf(buf, sizeof(buf), "Current size: %d", value);
  gtk_label_set_text(GTK_LABEL(label), buf);
}

void onBlockSizeValueChanged(GtkRange *range, gpointer data) {
  // Extract the app pointer and label from the data
  BlockSizeCallbackData *cbData = static_cast<BlockSizeCallbackData *>(data);
  TetrimoneApp *app = cbData->app;
  GtkWidget *label = cbData->label;

  // Get the new block size from the slider
  int newBlockSize = (int)gtk_range_get_value(range);

  // Update the displayed value
  char buf[32];
  snprintf(buf, sizeof(buf), "Current size: %d", newBlockSize);
  gtk_label_set_text(GTK_LABEL(label), buf);

  // Store the current game state before rebuilding UI
  bool gameWasPaused = app->board->isPaused();
  bool gameWasOver = app->board->isGameOver();

  // Update the global block size
  BLOCK_SIZE = newBlockSize;

  // Tear down and rebuild UI components
  rebuildGameUI(app);

  // Restore game state if needed
  if (gameWasPaused && !gameWasOver) {
    app->board->setPaused(true);
  }

  // Update action state
  if (app->board->isPaused()) {
    app->pauseActionLabel = "Resume";
    g_simple_action_set_enabled(app->startAction, TRUE);
  } else {
    app->pauseActionLabel = "Pause";
    g_simple_action_set_enabled(app->startAction, FALSE);
  }

  gtk_widget_queue_draw(app->gameArea);
  gtk_widget_queue_draw(app->nextPieceArea);
  updateLabels(app);
}

// ----------------------------------------------------------------------------
// rebuildGameUI: tears down and recreates the drawing areas + side panel
// after a block-size change.
//
// GTK4 notes:
//  - gtk_widget_destroy() doesn't exist; a widget is removed by unparenting
//    it (gtk_box_remove for box children) which also finalizes it if nothing
//    else holds a reference.
//  - gtk_container_get_children()/gtk_container_remove() are replaced by
//    walking gtk_widget_get_first_child()/gtk_widget_get_next_sibling() and
//    calling gtk_box_remove() (since app->mainBox is a GtkBox).
// ----------------------------------------------------------------------------

void rebuildGameUI(TetrimoneApp *app) {
  // Empty the mainBox entirely (game area + side panel); GTK4 has no
  // gtk_container_get_children, so we walk the child list directly.
  GtkWidget *child = gtk_widget_get_first_child(app->mainBox);
  while (child != NULL) {
    GtkWidget *next = gtk_widget_get_next_sibling(child);
    gtk_box_remove(GTK_BOX(app->mainBox), child);
    child = next;
  }
  app->gameArea = NULL;
  app->nextPieceArea = NULL;

  // Resize the window to match the new block size
  gtk_window_set_default_size(GTK_WINDOW(app->window), GRID_WIDTH * BLOCK_SIZE + 200,
                              GRID_HEIGHT * BLOCK_SIZE + 40);

  // Create new game area with correct size
  app->gameArea = gtk_drawing_area_new();
  gtk_widget_set_size_request(app->gameArea, GRID_WIDTH * BLOCK_SIZE,
                              GRID_HEIGHT * BLOCK_SIZE);
  gtk_drawing_area_set_draw_func(GTK_DRAWING_AREA(app->gameArea), onDrawGameArea, app, nullptr);
  gtk_box_append(GTK_BOX(app->mainBox), app->gameArea);

  // Create the side panel (vertical box)
  GtkWidget *sideBox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 10);
  gtk_box_append(GTK_BOX(app->mainBox), sideBox);

  // Create the next piece preview frame
  GtkWidget *nextPieceFrame = gtk_frame_new("Next Pieces");
  gtk_box_append(GTK_BOX(sideBox), nextPieceFrame);

  // Create the next piece drawing area - sized for 3 horizontal pieces with
  // half-size blocks
  int previewBlockSize = BLOCK_SIZE / 2;
  int previewWidth = 3 * 4 * previewBlockSize;   // 3 sections, each 4 blocks wide at half size
  int previewHeight = 4 * previewBlockSize + 30; // Height for pieces plus header

  app->nextPieceArea = gtk_drawing_area_new();
  gtk_widget_set_size_request(app->nextPieceArea, previewWidth, previewHeight);
  gtk_drawing_area_set_draw_func(GTK_DRAWING_AREA(app->nextPieceArea), onDrawNextPiece, app, nullptr);
  gtk_frame_set_child(GTK_FRAME(nextPieceFrame), app->nextPieceArea);

  // Create score, level, and lines labels
  app->scoreLabel = gtk_label_new(NULL);
  std::string score_text;
  if (app->board->retroModeActive) {
    score_text = "<b>Партийная Лояльность:</b> " + std::to_string(app->board->getScore()) + "%";
  } else {
    score_text = "<b>Score:</b> " + std::to_string(app->board->getScore());
  }
  gtk_label_set_markup(GTK_LABEL(app->scoreLabel), score_text.c_str());
  gtk_widget_set_halign(app->scoreLabel, GTK_ALIGN_START);
  gtk_box_append(GTK_BOX(sideBox), app->scoreLabel);

  app->levelLabel = gtk_label_new(NULL);
  std::string level_text;
  if (app->board->retroModeActive) {
    level_text = "<b>Пятилетка:</b> " + std::to_string(app->board->getLevel());
  } else {
    level_text = "<b>Level:</b> " + std::to_string(app->board->getLevel());
  }
  gtk_label_set_markup(GTK_LABEL(app->levelLabel), level_text.c_str());
  gtk_widget_set_halign(app->levelLabel, GTK_ALIGN_START);
  gtk_box_append(GTK_BOX(sideBox), app->levelLabel);

  app->linesLabel = gtk_label_new(NULL);
  std::string lines_text;
  if (app->board->retroModeActive) {
    lines_text = "<b>Уничтожено врагов народа:</b> " + std::to_string(app->board->getLinesCleared());
  } else {
    lines_text = "<b>Lines:</b> " + std::to_string(app->board->getLinesCleared());
  }
  gtk_label_set_markup(GTK_LABEL(app->linesLabel), lines_text.c_str());
  gtk_widget_set_halign(app->linesLabel, GTK_ALIGN_START);
  gtk_box_append(GTK_BOX(sideBox), app->linesLabel);

  app->sequenceLabel = gtk_label_new(NULL);
  std::string sequence_text;
  if (app->board->isSequenceActive() && app->board->getConsecutiveClears() > 1) {
    if (app->board->retroModeActive) {
      sequence_text = "<b>Коллективная эффективность:</b> " +
        std::to_string(app->board->getConsecutiveClears()) + " (Рекорд: " +
        std::to_string(app->board->getMaxConsecutiveClears()) + ")";
    } else {
      sequence_text = "<b>Sequence:</b> " + std::to_string(app->board->getConsecutiveClears()) +
        " (Max: " + std::to_string(app->board->getMaxConsecutiveClears()) + ")";
    }
    sequence_text = "<span foreground='#00AA00'>" + sequence_text + "</span>";
  } else {
    if (app->board->retroModeActive) {
      sequence_text = "<b>Коллективная эффективность:</b> " +
        std::to_string(app->board->getConsecutiveClears()) + " (Рекорд: " +
        std::to_string(app->board->getMaxConsecutiveClears()) + ")";
    } else {
      sequence_text = "<b>Sequence:</b> " + std::to_string(app->board->getConsecutiveClears()) +
        " (Max: " + std::to_string(app->board->getMaxConsecutiveClears()) + ")";
    }
  }
  gtk_label_set_markup(GTK_LABEL(app->sequenceLabel), sequence_text.c_str());
  gtk_widget_set_halign(app->sequenceLabel, GTK_ALIGN_START);
  gtk_box_append(GTK_BOX(sideBox), app->sequenceLabel);

  // Difficulty label
  app->difficultyLabel = gtk_label_new(NULL);
  gtk_label_set_markup(GTK_LABEL(app->difficultyLabel),
                       app->board->getDifficultyText(app->difficulty).c_str());
  gtk_widget_set_halign(app->difficultyLabel, GTK_ALIGN_START);
  gtk_box_append(GTK_BOX(sideBox), app->difficultyLabel);

  // Controls header + body
  app->controlsHeaderLabel = gtk_label_new(NULL);
  if (app->board->retroModeActive) {
    gtk_label_set_markup(GTK_LABEL(app->controlsHeaderLabel), "<b>ПАРТИЙНЫЕ ДИРЕКТИВЫ</b>");
  } else {
    gtk_label_set_markup(GTK_LABEL(app->controlsHeaderLabel), "<b>Controls</b>");
  }
  gtk_widget_set_halign(app->controlsHeaderLabel, GTK_ALIGN_START);
  gtk_widget_set_margin_top(app->controlsHeaderLabel, 10);
  gtk_box_append(GTK_BOX(sideBox), app->controlsHeaderLabel);

  app->controlsLabel = gtk_label_new(
    app->board->retroModeActive ?
    "Управление клавиатурой:\n"
    "• Влево/Вправо/A/D: Перемещение блока\n"
    "• Вверх/W: Поворот по часовой стрелке\n"
    "• Z: Поворот против часовой стрелки\n"
    "• Вниз/S: Мягкое падение\n"
    "• Пробел: Быстрое падение\n"
    "• P: Пауза/Продолжение игры\n"
    "• R: Перезапуск игры\n"
    "• M: Переключение музыки\n\n"
    "Поддержка контроллера доступна.\n"
    "Настройка в меню Управление." :
    "Keyboard Controls:\n"
    "• Left/Right/A/D: Move block\n"
    "• Up/W: Rotate clockwise\n"
    "• Z: Rotate counter-clockwise\n"
    "• Down/S: Soft drop\n"
    "• Space: Hard drop\n"
    "• P: Pause/Resume game\n"
    "• R: Restart game\n"
    "• M: Toggle music\n\n"
    "Controller support is available.\n"
    "Configure in Controls menu.");
  gtk_widget_set_halign(app->controlsLabel, GTK_ALIGN_START);
  gtk_box_append(GTK_BOX(sideBox), app->controlsLabel);

  // Widgets are visible by default in GTK4 (no gtk_widget_show_all needed).
}

void onBlockSizeDialog(GSimpleAction *action, GVariant *param, gpointer userData) {
  (void)action; (void)param;
  TetrimoneApp *app = static_cast<TetrimoneApp *>(userData);

  // Pause the game if it's running
  bool wasPaused = app->board->isPaused();
  if (!wasPaused && !app->board->isGameOver() && !app->board->isSplashScreenActive()) {
    onPauseGame(app->pauseAction, nullptr, app);
  }

  // Store original block size in case user cancels
  int originalBlockSize = BLOCK_SIZE;

  // Create dialog with Apply and Cancel buttons
  GtkWidget *dialog = gtk_dialog_new_with_buttons(
      "Block Size", GTK_WINDOW(app->window), GTK_DIALOG_MODAL, "_Cancel",
      GTK_RESPONSE_CANCEL, "_Apply", GTK_RESPONSE_APPLY, NULL);

  gtk_window_set_default_size(GTK_WINDOW(dialog), 300, 150);

  GtkWidget *contentArea = gtk_dialog_get_content_area(GTK_DIALOG(dialog));
  gtk_widget_set_margin_start(contentArea, 10);
  gtk_widget_set_margin_end(contentArea, 10);
  gtk_widget_set_margin_top(contentArea, 10);
  gtk_widget_set_margin_bottom(contentArea, 10);

  GtkWidget *vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 6);
  gtk_box_append(GTK_BOX(contentArea), vbox);

  GtkWidget *label = gtk_label_new("Adjust block size:");
  gtk_widget_set_halign(label, GTK_ALIGN_START);
  gtk_box_append(GTK_BOX(vbox), label);

  GtkWidget *scale = gtk_scale_new_with_range(
      GTK_ORIENTATION_HORIZONTAL, MIN_BLOCK_SIZE, MAX_BLOCK_SIZE, 1);
  gtk_range_set_value(GTK_RANGE(scale), BLOCK_SIZE);
  gtk_scale_set_digits(GTK_SCALE(scale), 0);
  gtk_scale_set_value_pos(GTK_SCALE(scale), GTK_POS_RIGHT);
  gtk_box_append(GTK_BOX(vbox), scale);

  GtkWidget *rangeBox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
  gtk_box_append(GTK_BOX(vbox), rangeBox);

  GtkWidget *minLabel = gtk_label_new("Small");
  gtk_widget_set_halign(minLabel, GTK_ALIGN_START);
  gtk_widget_set_hexpand(minLabel, TRUE);
  gtk_box_append(GTK_BOX(rangeBox), minLabel);

  GtkWidget *maxLabel = gtk_label_new("Large");
  gtk_widget_set_halign(maxLabel, GTK_ALIGN_END);
  gtk_widget_set_hexpand(maxLabel, TRUE);
  gtk_box_append(GTK_BOX(rangeBox), maxLabel);

  GtkWidget *currentValueLabel = gtk_label_new(NULL);
  char valueBuf[32];
  snprintf(valueBuf, sizeof(valueBuf), "Current size: %d", BLOCK_SIZE);
  gtk_label_set_text(GTK_LABEL(currentValueLabel), valueBuf);
  gtk_widget_set_halign(currentValueLabel, GTK_ALIGN_START);
  gtk_box_append(GTK_BOX(vbox), currentValueLabel);

  GtkWidget *noteLabel = gtk_label_new(
      "Click Apply to set the new block size.\nThis will reset the game UI.");
  gtk_widget_set_halign(noteLabel, GTK_ALIGN_START);
  gtk_widget_set_margin_top(noteLabel, 10);
  gtk_box_append(GTK_BOX(vbox), noteLabel);

  g_signal_connect(G_OBJECT(scale), "value-changed",
                   G_CALLBACK(updateSizeValueLabel), currentValueLabel);

  int response = gtk_dialog_run_gtk4(dialog);

  if (response == GTK_RESPONSE_APPLY) {
    BLOCK_SIZE = (int)gtk_range_get_value(GTK_RANGE(scale));
    rebuildGameUI(app);
  } else {
    BLOCK_SIZE = originalBlockSize;
  }

  gtk_window_destroy(GTK_WINDOW(dialog));

  // Resume the game if it wasn't paused before
  if (!wasPaused && !app->board->isGameOver() && !app->board->isSplashScreenActive()) {
    onPauseGame(app->pauseAction, nullptr, app);
  }
}

void updateWidthValueLabel(GtkAdjustment *adj, gpointer data) {
  GtkWidget *label = GTK_WIDGET(data);
  int value = (int)gtk_adjustment_get_value(adj);
  char text[50];
  snprintf(text, sizeof(text), "Width: %d", value);
  gtk_label_set_text(GTK_LABEL(label), text);
}

void updateHeightValueLabel(GtkAdjustment *adj, gpointer data) {
  GtkWidget *label = GTK_WIDGET(data);
  int value = (int)gtk_adjustment_get_value(adj);
  char text[50];
  snprintf(text, sizeof(text), "Height: %d", value);
  gtk_label_set_text(GTK_LABEL(label), text);
}

void onGameSizeDialog(GSimpleAction *action, GVariant *param, gpointer userData) {
  (void)action; (void)param;
  TetrimoneApp *app = static_cast<TetrimoneApp *>(userData);

  GtkWidget *dialog = gtk_dialog_new_with_buttons(
      "Game Size Settings", GTK_WINDOW(app->window), GTK_DIALOG_MODAL, "Apply",
      GTK_RESPONSE_APPLY, "Cancel", GTK_RESPONSE_CANCEL, NULL);

  GtkWidget *contentArea = gtk_dialog_get_content_area(GTK_DIALOG(dialog));
  gtk_widget_set_margin_start(contentArea, 10);
  gtk_widget_set_margin_end(contentArea, 10);
  gtk_widget_set_margin_top(contentArea, 10);
  gtk_widget_set_margin_bottom(contentArea, 10);

  GtkWidget *vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 8);
  gtk_box_append(GTK_BOX(contentArea), vbox);

  // Width settings
  GtkWidget *widthFrame = gtk_frame_new("Width");
  gtk_widget_set_vexpand(widthFrame, TRUE);
  gtk_box_append(GTK_BOX(vbox), widthFrame);

  GtkWidget *widthBox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 4);
  gtk_frame_set_child(GTK_FRAME(widthFrame), widthBox);

  GtkAdjustment *widthAdj = gtk_adjustment_new(GRID_WIDTH, MIN_GRID_WIDTH,
                                               MAX_GRID_WIDTH, 1, 5, 0);
  GtkWidget *widthScale = gtk_scale_new(GTK_ORIENTATION_HORIZONTAL, widthAdj);
  gtk_scale_set_digits(GTK_SCALE(widthScale), 0);
  gtk_box_append(GTK_BOX(widthBox), widthScale);

  GtkWidget *widthLabel = gtk_label_new("");
  gtk_box_append(GTK_BOX(widthBox), widthLabel);

  // Height settings
  GtkWidget *heightFrame = gtk_frame_new("Height");
  gtk_widget_set_vexpand(heightFrame, TRUE);
  gtk_box_append(GTK_BOX(vbox), heightFrame);

  GtkWidget *heightBox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 4);
  gtk_frame_set_child(GTK_FRAME(heightFrame), heightBox);

  GtkAdjustment *heightAdj = gtk_adjustment_new(GRID_HEIGHT, MIN_GRID_HEIGHT,
                                                MAX_GRID_HEIGHT, 1, 5, 0);
  GtkWidget *heightScale = gtk_scale_new(GTK_ORIENTATION_HORIZONTAL, heightAdj);
  gtk_scale_set_digits(GTK_SCALE(heightScale), 0);
  gtk_box_append(GTK_BOX(heightBox), heightScale);

  GtkWidget *heightLabel = gtk_label_new("");
  gtk_box_append(GTK_BOX(heightBox), heightLabel);

  // Update labels initially
  char widthText[50];
  snprintf(widthText, sizeof(widthText), "Width: %d", GRID_WIDTH);
  gtk_label_set_text(GTK_LABEL(widthLabel), widthText);

  char heightText[50];
  snprintf(heightText, sizeof(heightText), "Height: %d", GRID_HEIGHT);
  gtk_label_set_text(GTK_LABEL(heightLabel), heightText);

  g_signal_connect(widthAdj, "value-changed", G_CALLBACK(updateWidthValueLabel), widthLabel);
  g_signal_connect(heightAdj, "value-changed", G_CALLBACK(updateHeightValueLabel), heightLabel);

  // Warning message about restarting game
  GtkWidget *warningLabel =
      gtk_label_new("Note: Changing game size will restart the current game.");
  gtk_widget_set_margin_top(warningLabel, 10);
  gtk_box_append(GTK_BOX(vbox), warningLabel);

  gint response = gtk_dialog_run_gtk4(dialog);

  if (response == GTK_RESPONSE_APPLY) {
    int newWidth = (int)gtk_adjustment_get_value(widthAdj);
    int newHeight = (int)gtk_adjustment_get_value(heightAdj);

    if (newWidth != GRID_WIDTH || newHeight != GRID_HEIGHT) {
      GtkWidget *confirmDialog = gtk_message_dialog_new(
          GTK_WINDOW(app->window), GTK_DIALOG_MODAL, GTK_MESSAGE_QUESTION,
          GTK_BUTTONS_YES_NO,
          "Changing the game size will restart the current game. Continue?");

      gint confirmResponse = gtk_dialog_run_gtk4(confirmDialog);
      gtk_window_destroy(GTK_WINDOW(confirmDialog));

      if (confirmResponse == GTK_RESPONSE_YES) {
        GRID_WIDTH = newWidth;
        GRID_HEIGHT = newHeight;

        calculateBlockSize(app);
        rebuildGameUI(app);

        app->board->restart();
        resetUI(app);

        if (app->board->isPaused()) {
          app->board->togglePause();
          app->pauseActionLabel = "Pause";
        }

        g_simple_action_set_enabled(app->startAction, FALSE);
        g_simple_action_set_enabled(app->pauseAction, TRUE);

        startGame(app);
      }
    }
  }

  gtk_window_destroy(GTK_WINDOW(dialog));
}

void onBlockTrailsToggled(GSimpleAction *action, GVariant *value, gpointer userData) {
  TetrimoneApp *app = static_cast<TetrimoneApp *>(userData);
  app->board->setTrailsEnabled(g_variant_get_boolean(value));
  g_simple_action_set_state(action, value);

  gtk_widget_queue_draw(app->gameArea);
}

void onTrailOpacityChanged(GtkAdjustment *adj, gpointer data) {
  GtkWidget *label = GTK_WIDGET(data);
  double value = gtk_adjustment_get_value(adj);
  char text[50];
  snprintf(text, sizeof(text), "Opacity: %.2f", value);
  gtk_label_set_text(GTK_LABEL(label), text);
}

void onTrailDurationChanged(GtkAdjustment *adj, gpointer data) {
  GtkWidget *label = GTK_WIDGET(data);
  double value = gtk_adjustment_get_value(adj);
  char text[50];
  snprintf(text, sizeof(text), "Duration: %.2f seconds", value);
  gtk_label_set_text(GTK_LABEL(label), text);
}

void onBlockTrailsConfig(GSimpleAction *action, GVariant *param, gpointer userData) {
  (void)action; (void)param;
  TetrimoneApp *app = static_cast<TetrimoneApp *>(userData);

  GtkWidget *dialog = gtk_dialog_new_with_buttons(
      "Block Trails Settings", GTK_WINDOW(app->window), GTK_DIALOG_MODAL,
      "_Apply", GTK_RESPONSE_APPLY,
      "_Cancel", GTK_RESPONSE_CANCEL, NULL);

  GtkWidget *contentArea = gtk_dialog_get_content_area(GTK_DIALOG(dialog));
  gtk_widget_set_margin_start(contentArea, 10);
  gtk_widget_set_margin_end(contentArea, 10);
  gtk_widget_set_margin_top(contentArea, 10);
  gtk_widget_set_margin_bottom(contentArea, 10);

  GtkWidget *vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 8);
  gtk_box_append(GTK_BOX(contentArea), vbox);

  // Opacity settings
  GtkWidget *opacityFrame = gtk_frame_new("Trail Opacity");
  gtk_widget_set_vexpand(opacityFrame, TRUE);
  gtk_box_append(GTK_BOX(vbox), opacityFrame);

  GtkWidget *opacityBox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 4);
  gtk_frame_set_child(GTK_FRAME(opacityFrame), opacityBox);

  GtkAdjustment *opacityAdj = gtk_adjustment_new(
      app->board->getTrailOpacity(), 0.1, 1.0, 0.05, 0.1, 0);

  GtkWidget *opacityScale = gtk_scale_new(GTK_ORIENTATION_HORIZONTAL, opacityAdj);
  gtk_scale_set_digits(GTK_SCALE(opacityScale), 2);
  gtk_box_append(GTK_BOX(opacityBox), opacityScale);

  GtkWidget *opacityLabel = gtk_label_new("");
  gtk_box_append(GTK_BOX(opacityBox), opacityLabel);

  // Duration settings
  GtkWidget *durationFrame = gtk_frame_new("Trail Duration");
  gtk_widget_set_vexpand(durationFrame, TRUE);
  gtk_box_append(GTK_BOX(vbox), durationFrame);

  GtkWidget *durationBox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 4);
  gtk_frame_set_child(GTK_FRAME(durationFrame), durationBox);

  GtkAdjustment *durationAdj = gtk_adjustment_new(
      app->board->getTrailDuration(), 0.05, 1.0, 0.05, 0.1, 0);

  GtkWidget *durationScale = gtk_scale_new(GTK_ORIENTATION_HORIZONTAL, durationAdj);
  gtk_scale_set_digits(GTK_SCALE(durationScale), 2);
  gtk_box_append(GTK_BOX(durationBox), durationScale);

  GtkWidget *durationLabel = gtk_label_new("");
  gtk_box_append(GTK_BOX(durationBox), durationLabel);

  // Update labels initially
  char opacityText[50];
  snprintf(opacityText, sizeof(opacityText), "Opacity: %.2f", app->board->getTrailOpacity());
  gtk_label_set_text(GTK_LABEL(opacityLabel), opacityText);

  char durationText[50];
  snprintf(durationText, sizeof(durationText), "Duration: %.2f seconds", app->board->getTrailDuration());
  gtk_label_set_text(GTK_LABEL(durationLabel), durationText);

  g_signal_connect(opacityAdj, "value-changed", G_CALLBACK(onTrailOpacityChanged), opacityLabel);
  g_signal_connect(durationAdj, "value-changed", G_CALLBACK(onTrailDurationChanged), durationLabel);

  gint response = gtk_dialog_run_gtk4(dialog);

  if (response == GTK_RESPONSE_APPLY) {
    double newOpacity = gtk_adjustment_get_value(opacityAdj);
    double newDuration = gtk_adjustment_get_value(durationAdj);

    app->board->setTrailOpacity(newOpacity);
    app->board->setTrailDuration(newDuration);

    gtk_widget_queue_draw(app->gameArea);
  }

  gtk_window_destroy(GTK_WINDOW(dialog));
}
