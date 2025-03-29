# Makefile for Tetris with Windows and Linux Support
# Configurable audio backend (SDL or PulseAudio)

# Compiler settings
CXX_LINUX = g++
CXX_WIN = x86_64-w64-mingw32-gcc
CXXFLAGS_COMMON = -std=c++17 -Wall -Wextra

# Debug flags
DEBUG_FLAGS = -g -DDEBUG

# Audio backend selection - default to SDL
AUDIO_BACKEND ?= sdl

# Common SDL flags for joystick support (needed for both audio backends)
SDL_CFLAGS_LINUX := $(shell sdl2-config --cflags)
SDL_LIBS_LINUX := $(shell sdl2-config --libs)

ifeq ($(AUDIO_BACKEND),pulse)
  AUDIO_SRCS_LINUX = src/pulseaudioplayer.cpp
  AUDIO_FLAGS_LINUX = -DUSE_PULSEAUDIO $(shell pkg-config --cflags libpulse libpulse-simple)
  AUDIO_LIBS_LINUX = $(shell pkg-config --libs libpulse libpulse-simple)
else
  AUDIO_SRCS_LINUX = src/sdlaudioplayer.cpp
  AUDIO_FLAGS_LINUX = -DUSE_SDL
  AUDIO_LIBS_LINUX = -lSDL2_mixer
endif

# Source files
SRCS_COMMON = src/tetris.cpp src/audiomanager.cpp src/sound.cpp src/joystick.cpp src/background.cpp
SRCS_LINUX = $(AUDIO_SRCS_LINUX)
SRCS_WIN = src/windowsaudioplayer.cpp

# Windows SDL flags
SDL_CFLAGS_WIN := $(shell mingw64-pkg-config --cflags sdl2)
SDL_LIBS_WIN := $(shell mingw64-pkg-config --libs sdl2)

# GTK flags
GTK_CFLAGS_LINUX := $(shell pkg-config --cflags gtk+-3.0)
GTK_LIBS_LINUX := $(shell pkg-config --libs gtk+-3.0)
GTK_CFLAGS_WIN := $(shell mingw64-pkg-config --cflags gtk+-3.0)
GTK_LIBS_WIN := $(shell mingw64-pkg-config --libs gtk+-3.0)

# ZIP library flags
ZIP_CFLAGS_LINUX := $(shell pkg-config --cflags libzip)
ZIP_LIBS_LINUX := $(shell pkg-config --libs libzip)
ZIP_CFLAGS_WIN := $(shell mingw64-pkg-config --cflags libzip)
ZIP_LIBS_WIN := $(shell mingw64-pkg-config --libs libzip)

# Platform-specific settings
CXXFLAGS_LINUX = $(CXXFLAGS_COMMON) $(GTK_CFLAGS_LINUX) $(SDL_CFLAGS_LINUX) $(AUDIO_FLAGS_LINUX) $(ZIP_CFLAGS_LINUX) -DLINUX
CXXFLAGS_WIN = $(CXXFLAGS_COMMON) $(GTK_CFLAGS_WIN) $(SDL_CFLAGS_WIN) $(ZIP_CFLAGS_WIN) -DWIN32

# Debug-specific flags
CXXFLAGS_LINUX_DEBUG = $(CXXFLAGS_LINUX) $(DEBUG_FLAGS)
CXXFLAGS_WIN_DEBUG = $(CXXFLAGS_WIN) $(DEBUG_FLAGS)

# Linker flags
LDFLAGS_LINUX = $(GTK_LIBS_LINUX) $(SDL_LIBS_LINUX) $(AUDIO_LIBS_LINUX) $(ZIP_LIBS_LINUX) -pthread
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

# Background image settings
BACKGROUNDS_DIR = images/Tetris_backgrounds
BACKGROUND_ZIP = background.zip

# Create necessary directories
$(shell mkdir -p $(BUILD_DIR_LINUX)/src $(BUILD_DIR_WIN)/src \
	$(BUILD_DIR_LINUX_DEBUG)/src $(BUILD_DIR_WIN_DEBUG)/src)

# Default target - build for Linux with SDL audio
.PHONY: all
all: linux

# OS-specific builds
.PHONY: windows
windows: tetris-windows

.PHONY: linux
linux: tetris-linux

# Audio-specific builds
.PHONY: sdl
sdl:
	$(MAKE) linux AUDIO_BACKEND=sdl

.PHONY: pulse
pulse:
	$(MAKE) linux AUDIO_BACKEND=pulse

# Debug targets
.PHONY: debug
debug: tetris-linux-debug tetris-windows-debug

# Debug with specific audio backend
.PHONY: sdl-debug
sdl-debug:
	$(MAKE) tetris-linux-debug AUDIO_BACKEND=sdl

.PHONY: pulse-debug
pulse-debug:
	$(MAKE) tetris-linux-debug AUDIO_BACKEND=pulse

#
# Linux build targets
#
.PHONY: tetris-linux
tetris-linux: $(BUILD_DIR_LINUX)/$(TARGET_LINUX) pack-backgrounds-linux

$(BUILD_DIR_LINUX)/$(TARGET_LINUX): $(addprefix $(BUILD_DIR_LINUX)/,$(OBJS_LINUX))
	$(CXX_LINUX) $^ -o $@ $(LDFLAGS_LINUX)

# Generic compilation rules for Linux
$(BUILD_DIR_LINUX)/%.o: %.cpp
	$(CXX_LINUX) $(CXXFLAGS_LINUX) -c $< -o $@

#
# Linux debug targets
#
.PHONY: tetris-linux-debug
tetris-linux-debug: $(BUILD_DIR_LINUX_DEBUG)/$(TARGET_LINUX_DEBUG) pack-backgrounds-linux-debug

$(BUILD_DIR_LINUX_DEBUG)/$(TARGET_LINUX_DEBUG): $(addprefix $(BUILD_DIR_LINUX_DEBUG)/,$(OBJS_LINUX_DEBUG))
	$(CXX_LINUX) $^ -o $@ $(LDFLAGS_LINUX)

# Generic compilation rules for Linux debug
$(BUILD_DIR_LINUX_DEBUG)/%.debug.o: %.cpp
	$(CXX_LINUX) $(CXXFLAGS_LINUX_DEBUG) -c $< -o $@

#
# Windows build targets
#
.PHONY: tetris-windows
tetris-windows: $(BUILD_DIR_WIN)/$(TARGET_WIN) tetris-collect-dlls pack-backgrounds-windows

$(BUILD_DIR_WIN)/$(TARGET_WIN): $(addprefix $(BUILD_DIR_WIN)/,$(OBJS_WIN))
	$(CXX_WIN) $^ -o $@ $(LDFLAGS_WIN)

# Generic compilation rules for Windows
$(BUILD_DIR_WIN)/%.win.o: %.cpp
	$(CXX_WIN) $(CXXFLAGS_WIN) -c $< -o $@

#
# Windows debug targets
#
.PHONY: tetris-windows-debug
tetris-windows-debug: $(BUILD_DIR_WIN_DEBUG)/$(TARGET_WIN_DEBUG) tetris-collect-debug-dlls pack-backgrounds-windows-debug

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

#
# Background image packing
#
.PHONY: pack-backgrounds-linux
pack-backgrounds-linux:
	@echo "Packing background images for Linux build..."
	cd $(BACKGROUNDS_DIR) && zip -r ../../$(BUILD_DIR_LINUX)/$(BACKGROUND_ZIP) *.png
	@echo "Background images packed to $(BUILD_DIR_LINUX)/$(BACKGROUND_ZIP)"

.PHONY: pack-backgrounds-linux-debug
pack-backgrounds-linux-debug:
	@echo "Packing background images for Linux debug build..."
	cd $(BACKGROUNDS_DIR) && zip -r ../../$(BUILD_DIR_LINUX_DEBUG)/$(BACKGROUND_ZIP) *.png
	@echo "Background images packed to $(BUILD_DIR_LINUX_DEBUG)/$(BACKGROUND_ZIP)"

.PHONY: pack-backgrounds-windows
pack-backgrounds-windows:
	@echo "Packing background images for Windows build..."
	cd $(BACKGROUNDS_DIR) && zip -r ../../$(BUILD_DIR_WIN)/$(BACKGROUND_ZIP) *.png
	@echo "Background images packed to $(BUILD_DIR_WIN)/$(BACKGROUND_ZIP)"

.PHONY: pack-backgrounds-windows-debug
pack-backgrounds-windows-debug:
	@echo "Packing background images for Windows debug build..."
	cd $(BACKGROUNDS_DIR) && zip -r ../../$(BUILD_DIR_WIN_DEBUG)/$(BACKGROUND_ZIP) *.png
	@echo "Background images packed to $(BUILD_DIR_WIN_DEBUG)/$(BACKGROUND_ZIP)"

.PHONY: pack-backgrounds-all
pack-backgrounds-all: pack-backgrounds-linux pack-backgrounds-linux-debug pack-backgrounds-windows pack-backgrounds-windows-debug

# Clean target
.PHONY: clean
clean:
	find $(BUILD_DIR) -type f -name "*.o" -delete
	find $(BUILD_DIR) -type f -name "*.dll" -delete
	find $(BUILD_DIR) -type f -name "*.exe" -delete
	find $(BUILD_DIR) -type f -name "$(BACKGROUND_ZIP)" -delete
	rm -f $(BUILD_DIR_LINUX)/$(TARGET_LINUX)
	rm -f $(BUILD_DIR_LINUX_DEBUG)/$(TARGET_LINUX_DEBUG)

# Help target
.PHONY: help
help:
	@echo "Available targets:"
	@echo "  make               - Build Tetris for Linux with SDL audio (default)"
	@echo "  make linux         - Build Tetris for Linux with SDL audio"
	@echo "  make windows       - Build Tetris for Windows"
	@echo ""
	@echo "  make sdl           - Build Tetris for Linux with SDL audio explicitly"
	@echo "  make pulse         - Build Tetris for Linux with PulseAudio (still uses SDL for joystick)"
	@echo ""
	@echo "  make debug         - Build Tetris with debug symbols (using SDL for Linux)"
	@echo "  make sdl-debug     - Build Tetris with debug symbols using SDL audio"
	@echo "  make pulse-debug   - Build Tetris with debug symbols using PulseAudio"
	@echo ""
	@echo "  make tetris-linux  - Build Tetris for Linux (with current audio backend)"
	@echo "  make tetris-windows - Build Tetris for Windows (requires MinGW)"
	@echo ""
	@echo "  make pack-backgrounds-all - Pack background images for all build targets"
	@echo "  make pack-backgrounds-linux - Pack background images for Linux build"
	@echo "  make pack-backgrounds-windows - Pack background images for Windows build"
	@echo ""
	@echo "  make clean         - Remove all build files"
	@echo "  make help          - Show this help message"
