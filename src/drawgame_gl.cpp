#include <GL/glew.h>
#include <GL/gl.h>
#include <math.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <gtk/gtk.h>
#include "tetrimone.h"
#include "audiomanager.h"
#include <iostream>
#include <string>
#include <algorithm>
#include <chrono>
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

// ============================================================================
// OpenGL 3.3+ RENDERING CONTEXT
// ============================================================================

typedef struct {
    float x, y;
    float r, g, b, a;
} Vertex;

typedef struct {
    float m[16];
} Mat4;

static Mat4 mat4_identity(void) {
    Mat4 m = {0};
    m.m[0] = m.m[5] = m.m[10] = m.m[15] = 1.0f;
    return m;
}

static Mat4 mat4_ortho(float left, float right, float bottom, float top, float near, float far) {
    Mat4 m = mat4_identity();
    m.m[0] = 2.0f / (right - left);
    m.m[5] = 2.0f / (top - bottom);
    m.m[10] = -2.0f / (far - near);
    m.m[12] = -(right + left) / (right - left);
    m.m[13] = -(top + bottom) / (top - bottom);
    m.m[14] = -(far + near) / (far - near);
    return m;
}

typedef struct {
    GLuint program;
    GLuint vao;
    GLuint vbo;
    Mat4 projection;
    float color[4];
} GLRenderState;

static GLRenderState gl_state = {0};

// Shader sources for OpenGL 3.3+
static const char *vertex_shader = 
    "#version 330 core\n"
    "layout(location = 0) in vec2 position;\n"
    "layout(location = 1) in vec4 color;\n"
    "uniform mat4 projection;\n"
    "out vec4 vertexColor;\n"
    "void main() {\n"
    "    gl_Position = projection * vec4(position, 0.0, 1.0);\n"
    "    vertexColor = color;\n"
    "}\n";

static const char *fragment_shader =
    "#version 330 core\n"
    "in vec4 vertexColor;\n"
    "out vec4 FragColor;\n"
    "void main() {\n"
    "    FragColor = vertexColor;\n"
    "}\n";

// ============================================================================
// SHADER COMPILATION AND PROGRAM CREATION
// ============================================================================

static GLuint compile_shader(const char *src, GLenum type) {
    GLuint shader = glCreateShader(type);
    glShaderSource(shader, 1, &src, NULL);
    glCompileShader(shader);
    
    int success;
    char log[512];
    glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
    if (!success) {
        glGetShaderInfoLog(shader, 512, NULL, log);
        fprintf(stderr, "[GL] Shader compile error: %s\n", log);
    }
    return shader;
}

static GLuint create_program(const char *vs_src, const char *fs_src) {
    GLuint vs = compile_shader(vs_src, GL_VERTEX_SHADER);
    GLuint fs = compile_shader(fs_src, GL_FRAGMENT_SHADER);
    GLuint prog = glCreateProgram();
    glAttachShader(prog, vs);
    glAttachShader(prog, fs);
    glLinkProgram(prog);
    
    int success;
    char log[512];
    glGetProgramiv(prog, GL_LINK_STATUS, &success);
    if (!success) {
        glGetProgramInfoLog(prog, 512, NULL, log);
        fprintf(stderr, "[GL] Program link error: %s\n", log);
    }
    
    glDeleteShader(vs);
    glDeleteShader(fs);
    return prog;
}

static void gl_init(void) {
    if (gl_state.program) return;
    
    fprintf(stderr, "[GL] Initializing modern GL 3.3+ renderer for Tetrimone\n");
    
    gl_state.program = create_program(vertex_shader, fragment_shader);
    
    glGenVertexArrays(1, &gl_state.vao);
    glGenBuffers(1, &gl_state.vbo);
    
    glBindVertexArray(gl_state.vao);
    glBindBuffer(GL_ARRAY_BUFFER, gl_state.vbo);
    glBufferData(GL_ARRAY_BUFFER, 1000000 * sizeof(Vertex), NULL, GL_DYNAMIC_DRAW);
    
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, x));
    glEnableVertexAttribArray(0);
    
    glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, r));
    glEnableVertexAttribArray(1);
    
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
    
    gl_state.color[0] = 1.0f;
    gl_state.color[1] = 1.0f;
    gl_state.color[2] = 1.0f;
    gl_state.color[3] = 1.0f;
    
    fprintf(stderr, "[GL] GL 3.3+ renderer initialized for Tetrimone\n");
}

static void draw_vertices(Vertex *verts, int count, GLenum mode) {
    glUseProgram(gl_state.program);
    
    GLint proj_loc = glGetUniformLocation(gl_state.program, "projection");
    glUniformMatrix4fv(proj_loc, 1, GL_FALSE, gl_state.projection.m);
    
    glBindBuffer(GL_ARRAY_BUFFER, gl_state.vbo);
    glBufferSubData(GL_ARRAY_BUFFER, 0, count * sizeof(Vertex), verts);
    
    glBindVertexArray(gl_state.vao);
    glDrawArrays(mode, 0, count);
}

// ============================================================================
// HIGH-LEVEL DRAWING API (Replacing Cairo functions)
// ============================================================================

void gl_setup_2d_projection(int width, int height) {
    if (width <= 0) width = 1920;
    if (height <= 0) height = 1080;
    gl_state.projection = mat4_ortho(0, width, height, 0, -1, 1);
}

void gl_set_color(float r, float g, float b) {
    gl_state.color[0] = r;
    gl_state.color[1] = g;
    gl_state.color[2] = b;
    gl_state.color[3] = 1.0f;
}

void gl_set_color_alpha(float r, float g, float b, float a) {
    gl_state.color[0] = r;
    gl_state.color[1] = g;
    gl_state.color[2] = b;
    gl_state.color[3] = a;
}

void gl_draw_rect_filled(float x, float y, float width, float height) {
    Vertex verts[6] = {
        {x, y, gl_state.color[0], gl_state.color[1], gl_state.color[2], gl_state.color[3]},
        {x + width, y, gl_state.color[0], gl_state.color[1], gl_state.color[2], gl_state.color[3]},
        {x + width, y + height, gl_state.color[0], gl_state.color[1], gl_state.color[2], gl_state.color[3]},
        {x, y, gl_state.color[0], gl_state.color[1], gl_state.color[2], gl_state.color[3]},
        {x + width, y + height, gl_state.color[0], gl_state.color[1], gl_state.color[2], gl_state.color[3]},
        {x, y + height, gl_state.color[0], gl_state.color[1], gl_state.color[2], gl_state.color[3]}
    };
    draw_vertices(verts, 6, GL_TRIANGLES);
}

void gl_draw_rect_outline(float x, float y, float width, float height, float line_width) {
    glLineWidth(line_width);
    Vertex verts[5] = {
        {x, y, gl_state.color[0], gl_state.color[1], gl_state.color[2], gl_state.color[3]},
        {x + width, y, gl_state.color[0], gl_state.color[1], gl_state.color[2], gl_state.color[3]},
        {x + width, y + height, gl_state.color[0], gl_state.color[1], gl_state.color[2], gl_state.color[3]},
        {x, y + height, gl_state.color[0], gl_state.color[1], gl_state.color[2], gl_state.color[3]},
        {x, y, gl_state.color[0], gl_state.color[1], gl_state.color[2], gl_state.color[3]}
    };
    draw_vertices(verts, 5, GL_LINE_STRIP);
}

void gl_draw_line(float x1, float y1, float x2, float y2, float width) {
    glLineWidth(width);
    Vertex verts[2] = {
        {x1, y1, gl_state.color[0], gl_state.color[1], gl_state.color[2], gl_state.color[3]},
        {x2, y2, gl_state.color[0], gl_state.color[1], gl_state.color[2], gl_state.color[3]}
    };
    draw_vertices(verts, 2, GL_LINES);
}

void gl_draw_circle(float cx, float cy, float radius, int segments) {
    Vertex *verts = (Vertex*)malloc((segments + 2) * sizeof(Vertex));
    verts[0] = (Vertex){cx, cy, gl_state.color[0], gl_state.color[1], gl_state.color[2], gl_state.color[3]};
    
    for (int i = 0; i <= segments; i++) {
        float angle = 2.0f * M_PI * i / segments;
        float x = cx + radius * cosf(angle);
        float y = cy + radius * sinf(angle);
        verts[i+1] = (Vertex){x, y, gl_state.color[0], gl_state.color[1], gl_state.color[2], gl_state.color[3]};
    }
    
    draw_vertices(verts, segments + 2, GL_TRIANGLE_FAN);
    free(verts);
}

void gl_draw_circle_outline(float cx, float cy, float radius, float line_width, int segments) {
    glLineWidth(line_width);
    Vertex *verts = (Vertex*)malloc((segments + 1) * sizeof(Vertex));
    
    for (int i = 0; i <= segments; i++) {
        float angle = 2.0f * M_PI * i / segments;
        float x = cx + radius * cosf(angle);
        float y = cy + radius * sinf(angle);
        verts[i] = (Vertex){x, y, gl_state.color[0], gl_state.color[1], gl_state.color[2], gl_state.color[3]};
    }
    
    draw_vertices(verts, segments + 1, GL_LINE_STRIP);
    free(verts);
}

// ============================================================================
// DRAWING FUNCTIONS - CONVERTED FROM CAIRO
// ============================================================================

void drawBackground_gl(TetrimoneBoard *board, int width, int height) {
    // Draw solid background color
    gl_set_color(0.1f, 0.1f, 0.1f);
    gl_draw_rect_filled(0, 0, width, height);
    
    // TODO: Add background image texture support if needed
    // For now, only solid background is rendered
}

void drawGridLines_gl(TetrimoneBoard *board) {
    if (board->isShowingGridLines()) {
        gl_set_color(0.3f, 0.3f, 0.3f);
        
        // Vertical lines
        for (int x = 1; x < GRID_WIDTH; ++x) {
            gl_draw_line(x * BLOCK_SIZE, 0, x * BLOCK_SIZE, GRID_HEIGHT * BLOCK_SIZE, 1.0f);
        }
        
        // Horizontal lines
        for (int y = 1; y < GRID_HEIGHT; ++y) {
            gl_draw_line(0, y * BLOCK_SIZE, GRID_WIDTH * BLOCK_SIZE, y * BLOCK_SIZE, 1.0f);
        }
    }
}

void drawFailureLine_gl() {
    // Draw a red line at the top to indicate failure threshold
    gl_set_color(1.0f, 0.0f, 0.0f);
    gl_draw_line(0, 0, GRID_WIDTH * BLOCK_SIZE, 0, 2.0f);
}

void drawSplashScreen_gl(TetrimoneBoard *board, TetrimoneApp *app) {
    // Semi-transparent overlay
    gl_set_color_alpha(0.0f, 0.0f, 0.0f, 0.7f);
    gl_draw_rect_filled(0, 0, GRID_WIDTH * BLOCK_SIZE, GRID_HEIGHT * BLOCK_SIZE);
    
    // Draw title text area (centered white rectangle for now)
    gl_set_color_alpha(1.0f, 1.0f, 1.0f, 0.3f);
    int title_y = (GRID_HEIGHT * BLOCK_SIZE) / 3;
    gl_draw_rect_filled(100, title_y - 40, GRID_WIDTH * BLOCK_SIZE - 200, 100);
    
    // Draw colored blocks for decoration
    int startX = (GRID_WIDTH * BLOCK_SIZE - 4 * BLOCK_SIZE) / 2;
    int startY = title_y + 20;
    
    // Draw I piece (cyan)
    gl_set_color(0.0f, 0.7f, 0.9f);
    for (int i = 0; i < 4; i++) {
        gl_draw_rect_filled(startX + i * BLOCK_SIZE, startY, BLOCK_SIZE - 2, BLOCK_SIZE - 2);
    }
    
    // Draw T piece (purple)
    gl_set_color(0.8f, 0.0f, 0.8f);
    startY += BLOCK_SIZE * 1.5;
    gl_draw_rect_filled(startX + BLOCK_SIZE, startY, BLOCK_SIZE - 2, BLOCK_SIZE - 2);
    gl_draw_rect_filled(startX, startY + BLOCK_SIZE, BLOCK_SIZE - 2, BLOCK_SIZE - 2);
    gl_draw_rect_filled(startX + BLOCK_SIZE, startY + BLOCK_SIZE, BLOCK_SIZE - 2, BLOCK_SIZE - 2);
    gl_draw_rect_filled(startX + BLOCK_SIZE * 2, startY + BLOCK_SIZE, BLOCK_SIZE - 2, BLOCK_SIZE - 2);
}

// Animation value struct
struct LineClearAnimValues {
    double alpha;
    double scale;
    double offsetX;
    double offsetY;
};

// Get line clear animation values based on type
static LineClearAnimValues getLineClearAnimationValues(int type, double progress, int x, int y) {
    // Placeholder: implement based on drawgame_cairo.cpp animations
    LineClearAnimValues result = {1.0, 1.0, 0.0, 0.0};
    
    // Simple fade-out animation for now
    if (progress < 0.6) {
        result.alpha = 1.0 - progress;
        result.scale = 1.0 - progress * 0.3;
    } else {
        result.alpha = 0.0;
        result.scale = 0.7;
    }
    
    return result;
}

void drawPlacedBlocks_gl(TetrimoneBoard *board, TetrimoneApp *app) {
    for (int y = 0; y < GRID_HEIGHT; ++y) {
        for (int x = 0; x < GRID_WIDTH; ++x) {
            int value = board->getGridValue(x, y);
            if (value > 0) {
                double alpha = 1.0;
                double scale = 1.0;
                double offsetX = 0.0;
                double offsetY = 0.0;
                
                // Handle line clear animations
                if (board->isLineClearActive() && board->isLineBeingCleared(y)) {
                    double progress = board->getLineClearProgress();
                    
                    if (board->retroModeActive) {
                        // Soviet-era effect
                        if (progress < 0.3) {
                            double scanProgress = progress / 0.3;
                            int scanX = (int)(scanProgress * GRID_WIDTH);
                            if (x <= scanX) {
                                alpha = 0.3 + 0.4 * sin(progress * 20.0);
                            }
                        } else if (progress < 0.7) {
                            double flashProgress = (progress - 0.3) / 0.4;
                            alpha = 1.0 - flashProgress * 0.7;
                            offsetY = flashProgress * BLOCK_SIZE * 0.3;
                        }
                    } else {
                        // Modern animations
                        int animationType = board->getCurrentAnimationType();
                        LineClearAnimValues animValues = getLineClearAnimationValues(animationType, progress, x, y);
                        alpha = animValues.alpha;
                        scale = animValues.scale;
                        offsetX = animValues.offsetX;
                        offsetY = animValues.offsetY;
                    }
                }
                
                // Get color from theme
                auto baseColor = board->isInThemeTransition() ? 
                    board->getInterpolatedColor(value - 1, board->getThemeTransitionProgress()) :
                    TETRIMONEBLOCK_COLOR_THEMES[currentThemeIndex][value - 1];
                
                // Apply heat modifications if available
                auto color = baseColor; // TODO: getHeatModifiedColor(baseColor, board->getHeatLevel());
                
                gl_set_color_alpha(color[0], color[1], color[2], alpha);
                
                // Calculate position with animation
                double drawX = x * BLOCK_SIZE + offsetX + (BLOCK_SIZE * (1.0 - scale)) / 2;
                double drawY = y * BLOCK_SIZE + offsetY + (BLOCK_SIZE * (1.0 - scale)) / 2;
                double drawSize = BLOCK_SIZE * scale;
                
                // Draw main block
                gl_draw_rect_filled(drawX + 1, drawY + 1, drawSize - 2, drawSize - 2);
                
                // Draw 3D highlight if not in retro mode
                if (!board->retroModeActive && !board->simpleBlocksActive) {
                    gl_set_color_alpha(1.0f, 1.0f, 1.0f, 0.3f * alpha);
                    // Draw highlight triangle
                    Vertex highlight[3] = {
                        {(float)(drawX + 1), (float)(drawY + 1), 1.0f, 1.0f, 1.0f, 0.3f * (float)alpha},
                        {(float)(drawX + drawSize - 1), (float)(drawY + 1), 1.0f, 1.0f, 1.0f, 0.3f * (float)alpha},
                        {(float)(drawX + 1), (float)(drawY + drawSize - 1), 1.0f, 1.0f, 1.0f, 0.3f * (float)alpha}
                    };
                    draw_vertices(highlight, 3, GL_TRIANGLES);
                    
                    // Draw shadow triangle
                    gl_set_color_alpha(0.0f, 0.0f, 0.0f, 0.3f * alpha);
                    Vertex shadow[3] = {
                        {(float)(drawX + drawSize - 1), (float)(drawY + 1), 0.0f, 0.0f, 0.0f, 0.3f * (float)alpha},
                        {(float)(drawX + drawSize - 1), (float)(drawY + drawSize - 1), 0.0f, 0.0f, 0.0f, 0.3f * (float)alpha},
                        {(float)(drawX + 1), (float)(drawY + drawSize - 1), 0.0f, 0.0f, 0.0f, 0.3f * (float)alpha}
                    };
                    draw_vertices(shadow, 3, GL_TRIANGLES);
                }
            }
        }
    }
}

void drawCurrentPiece_gl(TetrimoneBoard *board) {
    auto piece = board->getCurrentPiece();
    auto shape = piece.getShape();
    auto color = piece.getColor();
    
    int pieceX = piece.getX();
    int pieceY = piece.getY();
    
    // Draw each block in the piece
    for (size_t row = 0; row < shape.size(); ++row) {
        for (size_t col = 0; col < shape[row].size(); ++col) {
            if (shape[row][col]) {
                int gridX = pieceX + col;
                int gridY = pieceY + row;
                
                if (gridX >= 0 && gridX < GRID_WIDTH && gridY >= 0 && gridY < GRID_HEIGHT) {
                    gl_set_color((float)color[0], (float)color[1], (float)color[2]);
                    gl_draw_rect_filled(gridX * BLOCK_SIZE + 1, gridY * BLOCK_SIZE + 1, 
                                       BLOCK_SIZE - 2, BLOCK_SIZE - 2);
                    
                    // Draw 3D effect
                    gl_set_color_alpha(1.0f, 1.0f, 1.0f, 0.3f);
                    Vertex highlight[3] = {
                        {(float)(gridX * BLOCK_SIZE + 1), (float)(gridY * BLOCK_SIZE + 1), 1.0f, 1.0f, 1.0f, 0.3f},
                        {(float)(gridX * BLOCK_SIZE + BLOCK_SIZE - 1), (float)(gridY * BLOCK_SIZE + 1), 1.0f, 1.0f, 1.0f, 0.3f},
                        {(float)(gridX * BLOCK_SIZE + 1), (float)(gridY * BLOCK_SIZE + BLOCK_SIZE - 1), 1.0f, 1.0f, 1.0f, 0.3f}
                    };
                    draw_vertices(highlight, 3, GL_TRIANGLES);
                }
            }
        }
    }
}

void drawGhostPiece_gl(TetrimoneBoard *board) {
    auto piece = board->getCurrentPiece();
    auto shape = piece.getShape();
    auto color = piece.getColor();
    
    // Get current piece position
    int pieceX = piece.getX();
    int ghostY = board->getGhostPieceY();
    
    gl_set_color_alpha((float)color[0], (float)color[1], (float)color[2], 0.3f);
    
    // Draw ghost piece with transparency
    for (size_t row = 0; row < shape.size(); ++row) {
        for (size_t col = 0; col < shape[row].size(); ++col) {
            if (shape[row][col]) {
                int gridX = pieceX + col;
                int gridY = ghostY + row;
                
                if (gridX >= 0 && gridX < GRID_WIDTH && gridY >= 0 && gridY < GRID_HEIGHT) {
                    gl_draw_rect_outline(gridX * BLOCK_SIZE + 1, gridY * BLOCK_SIZE + 1,
                                        BLOCK_SIZE - 2, BLOCK_SIZE - 2, 1.0f);
                }
            }
        }
    }
}

void drawGameOver_gl(TetrimoneBoard *board) {
    if (board->isGameOver()) {
        // Semi-transparent overlay
        gl_set_color_alpha(0.0f, 0.0f, 0.0f, 0.6f);
        gl_draw_rect_filled(0, 0, GRID_WIDTH * BLOCK_SIZE, GRID_HEIGHT * BLOCK_SIZE);
        
        // White rectangle for GAME OVER text
        gl_set_color_alpha(1.0f, 0.0f, 0.0f, 0.8f);
        int textY = (GRID_HEIGHT * BLOCK_SIZE) / 2;
        gl_draw_rect_filled(100, textY - 50, GRID_WIDTH * BLOCK_SIZE - 200, 100);
    }
}

void drawPauseMenu_gl(TetrimoneBoard *board) {
    if (board->isPaused()) {
        // Semi-transparent overlay
        gl_set_color_alpha(0.0f, 0.0f, 0.0f, 0.5f);
        gl_draw_rect_filled(0, 0, GRID_WIDTH * BLOCK_SIZE, GRID_HEIGHT * BLOCK_SIZE);
        
        // Yellow rectangle for PAUSED text
        gl_set_color_alpha(1.0f, 1.0f, 0.0f, 0.8f);
        int textY = (GRID_HEIGHT * BLOCK_SIZE) / 2;
        gl_draw_rect_filled(100, textY - 40, GRID_WIDTH * BLOCK_SIZE - 200, 80);
    }
}

void drawPropagandaMessage_gl(TetrimoneBoard *board) {
    if (board->retroModeActive) {
        // Draw retro-style message box
        gl_set_color(0.8f, 0.2f, 0.2f);
        gl_draw_rect_filled(0, 0, GRID_WIDTH * BLOCK_SIZE, 40);
    }
}

void drawFireworks_gl(TetrimoneBoard *board, TetrimoneApp *app) {
    // TODO: Implement particle effects for line clears
    // Draw simple circles for now
    gl_set_color(1.0f, 1.0f, 0.0f);
    gl_draw_circle(GRID_WIDTH * BLOCK_SIZE / 2, GRID_HEIGHT * BLOCK_SIZE / 2, 50, 32);
}

void drawBlockTrails_gl(TetrimoneBoard *board) {
    // TODO: Implement block trail effects
}

void drawFireyGlow_gl(double x, double y, double size, float heatLevel, double time) {
    // Draw orange glow effect for hot blocks
    gl_set_color_alpha(1.0f, 0.6f, 0.0f, 0.3f * heatLevel);
    gl_draw_circle(x + size/2, y + size/2, size/2 + 5, 16);
}

void drawFreezyEffect_gl(double x, double y, double size, float heatLevel, double time) {
    // Draw light blue glow for cold blocks
    float freezeIntensity = (0.3f - heatLevel) / 0.3f;
    gl_set_color_alpha(0.7f, 0.9f, 1.0f, 0.2f * freezeIntensity);
    gl_draw_circle(x + size/2, y + size/2, size/2 + 3, 16);
}

// ============================================================================
// GTK CALLBACKS
// ============================================================================

gboolean on_realize_gl(GtkGLArea *area, gpointer data) {
    (void)data;
    gtk_gl_area_make_current(area);
    
    static gboolean glew_initialized = FALSE;
    if (!glew_initialized) {
        glewExperimental = GL_TRUE;
        GLenum err = glewInit();
        fprintf(stderr, "[GL] glewInit: %s\n", glewGetErrorString(err));
        glew_initialized = TRUE;
    }
    
    glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glEnable(GL_LINE_SMOOTH);
    glHint(GL_LINE_SMOOTH_HINT, GL_NICEST);
    glEnable(GL_MULTISAMPLE);
    
    gl_init();
    
    fprintf(stderr, "[GL] GL Context initialized\n");
    return TRUE;
}

gboolean on_render_gl(GtkGLArea *area, GdkGLContext *context, gpointer data) {
    (void)context;
    TetrimoneApp *app = (TetrimoneApp *)data;
    if (!app) return FALSE;
    
    TetrimoneBoard *board = app->board;
    
    gtk_gl_area_make_current(area);
    
    int window_width = gtk_widget_get_allocated_width(GTK_WIDGET(area));
    int window_height = gtk_widget_get_allocated_height(GTK_WIDGET(area));
    
    if (window_width < 10 || window_height < 10) {
        gtk_widget_queue_draw(GTK_WIDGET(area));
        return TRUE;
    }
    
    // Set up projection for Tetrimone board size
    int board_width = GRID_WIDTH * BLOCK_SIZE;
    int board_height = GRID_HEIGHT * BLOCK_SIZE;
    
    glViewport(0, 0, window_width, window_height);
    gl_setup_2d_projection(board_width, board_height);
    
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    
    // Draw all game elements
    drawBackground_gl(board, board_width, board_height);
    drawGridLines_gl(board);
    drawFailureLine_gl();
    drawPlacedBlocks_gl(board, app);
    
    if (board->isSplashScreenActive()) {
        drawSplashScreen_gl(board, app);
        glFlush();
        gtk_widget_queue_draw(GTK_WIDGET(area));
        return TRUE;
    }
    
    drawPropagandaMessage_gl(board);
    drawCurrentPiece_gl(board);
    drawGhostPiece_gl(board);
    drawPauseMenu_gl(board);
    drawGameOver_gl(board);
    
    if (board->isFireworksActive()) {
        drawFireworks_gl(board, app);
    }
    
    if (board->isTrailsEnabled() && board->isBlockTrailsActive()) {
        drawBlockTrails_gl(board);
    }
    
    glFlush();
    gtk_widget_queue_draw(GTK_WIDGET(area));
    
    return TRUE;
}

// ============================================================================
// INITIALIZATION
// ============================================================================

void tetrimone_gl_init(GtkGLArea *gl_area) {
    g_signal_connect(gl_area, "realize", G_CALLBACK(on_realize_gl), NULL);
    g_signal_connect(gl_area, "render", G_CALLBACK(on_render_gl), NULL);
}
