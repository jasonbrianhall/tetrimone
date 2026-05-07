// ============================================================================
// joystick_core.h - Framework-agnostic joystick interface
// ============================================================================

#ifndef JOYSTICK_CORE_H
#define JOYSTICK_CORE_H

#include <SDL2/SDL.h>

// Forward declaration
struct TetrimoneApp;

// ============================================================================
// Core SDL Management
// ============================================================================

/**
 * Initialize SDL joystick subsystem and open first available joystick
 * Framework-specific timer setup must be done by the framework layer
 */
void initSDL(TetrimoneApp *app);

/**
 * Shutdown SDL joystick subsystem
 * Framework-specific timer cleanup should be done before calling this
 */
void shutdownSDL(TetrimoneApp *app);

// ============================================================================
// Core Input Processing
// ============================================================================

/**
 * Process joystick button inputs
 * Calls provided callbacks for framework-specific UI updates
 * 
 * @param app Application instance
 * @param onPauseCallback Called when pause button pressed: callback(app, shouldPause)
 * @param onRotateCallback Called when rotate button pressed: callback(app, isClockwise)
 * @param onHardDropCallback Called when hard drop button pressed: callback(app)
 * @return true if a button action was processed
 */
bool processJoystickButtons(
    TetrimoneApp* app,
    void (*onPauseCallback)(TetrimoneApp*, bool),
    void (*onRotateCallback)(TetrimoneApp*, bool),
    void (*onHardDropCallback)(TetrimoneApp*));

/**
 * Get normalized analog stick input
 * 
 * @param app Application instance
 * @param outX Output: -1 (left), 0 (center), or 1 (right)
 * @param outY Output: -1 (up), 0 (center), or 1 (down)
 */
void getJoystickAnalogMovement(TetrimoneApp* app, int* outX, int* outY);

// ============================================================================
// Configuration Persistence
// ============================================================================

/**
 * Save joystick mapping configuration to user config directory
 */
void saveJoystickMapping(TetrimoneApp* app);

/**
 * Load joystick mapping configuration from user config directory
 */
void loadJoystickMapping(TetrimoneApp* app);

#endif  // JOYSTICK_CORE_H
