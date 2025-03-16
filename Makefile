# Makefile for Tetris with Windows and Linux Support
# Similar structure to the Solitaire Makefile

# Compiler settings
CXX_LINUX = g++
CXX_WIN = x86_64-w64-mingw32-gcc
CXXFLAGS_COMMON = -std=c++17 -Wall -Wextra

# Debug flags
DEBUG_FLAGS = -g -DDEBUG

# Source files
SRCS_COMMON = src/tetris.cpp src/audiomanager.cpp src/sound.cpp src/joystick.cpp
SRCS_LINUX = src/pulseaudioplayer.cpp
SRCS_WIN = src/windowsaudioplayer.cpp

# Use pkg-config for dependencies
SDL_CFLAGS_LINUX := $(shell sdl2-config --cflags)
SDL_LIBS_LINUX := $(shell sdl2-config --libs)
SDL_CFLAGS_WIN := $(shell mingw64-pkg-config --cflags sdl2)
SDL_LIBS_WIN := $(shell mingw64-pkg-config --libs sdl2)

GTK_CFLAGS_LINUX := $(shell pkg-config --cflags gtk+-3.0)
GTK_LIBS_LINUX := $(shell pkg-config --libs gtk+-3.0)
GTK_CFLAGS_WIN := $(shell mingw64-pkg-config --cflags gtk+-3.0)
GTK_LIBS_WIN := $(shell mingw64-pkg-config --libs gtk+-3.0)

# PulseAudio flags for Linux
PULSE_CFLAGS := $(shell pkg-config --cflags libpulse libpulse-simple)
PULSE_LIBS := $(shell pkg-config --libs libpulse libpulse-simple)

# ZIP library flags
ZIP_CFLAGS_LINUX := $(shell pkg-config --cflags libzip)
ZIP_LIBS_LINUX := $(shell pkg-config --libs libzip)
ZIP_CFLAGS_WIN := $(shell mingw64-pkg-config --cflags libzip)
ZIP_LIBS_WIN := $(shell mingw64-pkg-config --libs libzip)

# Platform-specific settings
CXXFLAGS_LINUX = $(CXXFLAGS_COMMON) $(GTK_CFLAGS_LINUX) $(SDL_CFLAGS_LINUX) $(PULSE_CFLAGS) $(ZIP_CFLAGS_LINUX) -DLINUX
CXXFLAGS_WIN = $(CXXFLAGS_COMMON) $(GTK_CFLAGS_WIN) $(SDL_CFLAGS_WIN) $(ZIP_CFLAGS_WIN) -DWIN32

# Debug-specific flags
CXXFLAGS_LINUX_DEBUG = $(CXXFLAGS_LINUX) $(DEBUG_FLAGS)
CXXFLAGS_WIN_DEBUG = $(CXXFLAGS_WIN) $(DEBUG_FLAGS)

# Linker flags
LDFLAGS_LINUX = $(GTK_LIBS_LINUX) $(SDL_LIBS_LINUX) $(PULSE_LIBS) $(ZIP_LIBS_LINUX) -pthread
LDFLAGS_WIN = $(GTK_LIBS_WIN) $(SDL_LIBS_WIN) $(ZIP_LIBS_WIN) -lwinmm -lstdc++ -mwindows

# Object files
OBJS_LINUX = $(SRCS_COMMON:.cpp=.o) $(SRCS_LINUX:.cpp=.o)
OBJS_WIN = $(SRCS_COMMON:.cpp=.win.o) $(SRCS_WIN:.cpp=.win.o)
OBJS_LINUX_DEBUG = $(SRCS_COMMON:.cpp=.debug.o) $(SRCS_LINUX:.cpp=.debug.o)
OBJS_WIN_DEBUG = $(SRCS_COMMON:.cpp=.win.debug.o) $(SRCS_WIN:.cpp=.win.debug.o)

# Target executables
TARGET_LINUX = tetris
TARGET_WIN = tetris.exe
TARGET_LINUX_DEBUG = tetris_debug
TARGET_WIN_DEBUG = tetris_debug.exe

# Build directories
BUILD_DIR = build
BUILD_DIR_LINUX = $(BUILD_DIR)/linux
BUILD_DIR_WIN = $(BUILD_DIR)/windows
BUILD_DIR_LINUX_DEBUG = $(BUILD_DIR)/linux_debug
BUILD_DIR_WIN_DEBUG = $(BUILD_DIR)/windows_debug

# Windows DLL settings
DLL_SOURCE_DIR = /usr/x86_64-w64-mingw32/sys-root/mingw/bin

# Create necessary directories
$(shell mkdir -p $(BUILD_DIR_LINUX)/src $(BUILD_DIR_WIN)/src \
	$(BUILD_DIR_LINUX_DEBUG)/src $(BUILD_DIR_WIN_DEBUG)/src)

# Default target - build for Linux
.PHONY: all
all: linux

# OS-specific builds
.PHONY: windows
windows: tetris-windows

.PHONY: linux
linux: tetris-linux

# Debug targets
.PHONY: debug
debug: tetris-linux-debug tetris-windows-debug

#
# Linux build targets
#
.PHONY: tetris-linux
tetris-linux: $(BUILD_DIR_LINUX)/$(TARGET_LINUX)

$(BUILD_DIR_LINUX)/$(TARGET_LINUX): $(addprefix $(BUILD_DIR_LINUX)/,$(OBJS_LINUX))
	$(CXX_LINUX) $^ -o $@ $(LDFLAGS_LINUX)

# Generic compilation rules for Linux
$(BUILD_DIR_LINUX)/%.o: %.cpp
	$(CXX_LINUX) $(CXXFLAGS_LINUX) -c $< -o $@

#
# Linux debug targets
#
.PHONY: tetris-linux-debug
tetris-linux-debug: $(BUILD_DIR_LINUX_DEBUG)/$(TARGET_LINUX_DEBUG)

$(BUILD_DIR_LINUX_DEBUG)/$(TARGET_LINUX_DEBUG): $(addprefix $(BUILD_DIR_LINUX_DEBUG)/,$(OBJS_LINUX_DEBUG))
	$(CXX_LINUX) $^ -o $@ $(LDFLAGS_LINUX)

# Generic compilation rules for Linux debug
$(BUILD_DIR_LINUX_DEBUG)/%.debug.o: %.cpp
	$(CXX_LINUX) $(CXXFLAGS_LINUX_DEBUG) -c $< -o $@

#
# Windows build targets
#
.PHONY: tetris-windows
tetris-windows: $(BUILD_DIR_WIN)/$(TARGET_WIN) tetris-collect-dlls

$(BUILD_DIR_WIN)/$(TARGET_WIN): $(addprefix $(BUILD_DIR_WIN)/,$(OBJS_WIN))
	$(CXX_WIN) $^ -o $@ $(LDFLAGS_WIN)

# Generic compilation rules for Windows
$(BUILD_DIR_WIN)/%.win.o: %.cpp
	$(CXX_WIN) $(CXXFLAGS_WIN) -c $< -o $@

#
# Windows debug targets
#
.PHONY: tetris-windows-debug
tetris-windows-debug: $(BUILD_DIR_WIN_DEBUG)/$(TARGET_WIN_DEBUG) tetris-collect-debug-dlls

$(BUILD_DIR_WIN_DEBUG)/$(TARGET_WIN_DEBUG): $(addprefix $(BUILD_DIR_WIN_DEBUG)/,$(OBJS_WIN_DEBUG))
	$(CXX_WIN) $(CXXFLAGS_WIN_DEBUG) $^ -o $@ $(LDFLAGS_WIN)

# Generic compilation rules for Windows debug
$(BUILD_DIR_WIN_DEBUG)/%.win.debug.o: %.cpp
	$(CXX_WIN) $(CXXFLAGS_WIN_DEBUG) -c $< -o $@

#
# DLL collection for Windows builds
#
.PHONY: tetris-collect-dlls
tetris-collect-dlls: $(BUILD_DIR_WIN)/$(TARGET_WIN)
	@echo "Collecting DLLs for Tetris..."
	@build/windows/collect_dlls.sh $(BUILD_DIR_WIN)/$(TARGET_WIN) $(DLL_SOURCE_DIR) $(BUILD_DIR_WIN)

.PHONY: tetris-collect-debug-dlls
tetris-collect-debug-dlls: $(BUILD_DIR_WIN_DEBUG)/$(TARGET_WIN_DEBUG)
	@echo "Collecting Debug DLLs for Tetris..."
	@build/windows/collect_dlls.sh $(BUILD_DIR_WIN_DEBUG)/$(TARGET_WIN_DEBUG) $(DLL_SOURCE_DIR) $(BUILD_DIR_WIN_DEBUG)

# Clean target
.PHONY: clean
clean:
	find $(BUILD_DIR) -type f -name "*.o" -delete
	find $(BUILD_DIR) -type f -name "*.dll" -delete
	find $(BUILD_DIR) -type f -name "*.exe" -delete
	rm -f $(BUILD_DIR_LINUX)/$(TARGET_LINUX)
	rm -f $(BUILD_DIR_LINUX_DEBUG)/$(TARGET_LINUX_DEBUG)

# Help target
.PHONY: help
help:
	@echo "Available targets:"
	@echo "  make               - Build Tetris for Linux (default)"
	@echo "  make linux         - Build Tetris for Linux"
	@echo "  make windows       - Build Tetris for Windows"
	@echo "  make debug         - Build Tetris with debug symbols for Linux and Windows"
	@echo ""
	@echo "  make tetris-linux  - Build Tetris for Linux"
	@echo "  make tetris-linux-debug  - Build Tetris for Linux with debug symbols"
	@echo ""
	@echo "  make tetris-windows      - Build Tetris for Windows (requires MinGW)"
	@echo "  make tetris-windows-debug - Build Tetris for Windows with debug symbols"
	@echo ""
	@echo "  make clean         - Remove all build files"
	@echo "  make help          - Show this help message"
