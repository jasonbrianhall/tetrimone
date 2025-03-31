# Makefile for Tetris with Windows and Linux Support
# Configurable audio backend (SDL or PulseAudio)
# Extended with FFMPEG support for MIDI to WAV conversion
# Modified to convert WAV to MP3 and only pack MP3 files

# MIDI Conversion
FLUIDSYNTH = fluidsynth
SOUNDFONT = /usr/share/sounds/sf2/default.sf2  # Default soundfont path, adjust as needed
FLUIDSYNTH_OPTS = -ni -g 1 -F

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
  AUDIO_FLAGS_LINUX = -DUSE_PULSEAUDIO $(shell pkg-config --cflags libpulse libpulse-simple) -DUSE_SDL
  AUDIO_LIBS_LINUX = $(shell pkg-config --libs libpulse libpulse-simple) -lSDL2_mixer
else
  AUDIO_SRCS_LINUX = src/sdlaudioplayer.cpp
  AUDIO_FLAGS_LINUX = -DUSE_SDL
  AUDIO_LIBS_LINUX = -lSDL2_mixer
endif

# Source files
SRCS_COMMON = src/tetris.cpp src/audiomanager.cpp src/sound.cpp src/joystick.cpp src/background.cpp src/audioconverter.cpp src/volume.cpp
SRCS_LINUX = $(AUDIO_SRCS_LINUX)
SRCS_WIN = src/sdlaudioplayer.cpp
SRCS_WIN_SDL = src/sdlaudioplayer.cpp

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
CXXFLAGS_WIN = $(CXXFLAGS_COMMON) $(GTK_CFLAGS_WIN) $(SDL_CFLAGS_WIN) $(ZIP_CFLAGS_WIN) -DWIN32 -DUSE_SDL

# Debug-specific flags
CXXFLAGS_LINUX_DEBUG = $(CXXFLAGS_LINUX) $(DEBUG_FLAGS)
CXXFLAGS_WIN_DEBUG = $(CXXFLAGS_WIN) $(DEBUG_FLAGS)

# Linker flags
LDFLAGS_LINUX = $(GTK_LIBS_LINUX) $(SDL_LIBS_LINUX) $(AUDIO_LIBS_LINUX) $(ZIP_LIBS_LINUX) -pthread
LDFLAGS_WIN = $(GTK_LIBS_WIN) $(SDL_LIBS_WIN) $(ZIP_LIBS_WIN) -lwinmm -lstdc++ -lSDL2_mixer

# Object files
OBJS_LINUX = $(SRCS_COMMON:.cpp=.o) $(SRCS_LINUX:.cpp=.o)
OBJS_WIN = $(SRCS_COMMON:.cpp=.win.o) $(SRCS_WIN:.cpp=.win.o)
OBJS_WIN_SDL = $(SRCS_COMMON:.cpp=.win.o) $(SRCS_WIN_SDL:.cpp=.win.o)
OBJS_LINUX_DEBUG = $(SRCS_COMMON:.cpp=.debug.o) $(SRCS_LINUX:.cpp=.debug.o)
OBJS_WIN_DEBUG = $(SRCS_COMMON:.cpp=.win.debug.o) $(SRCS_WIN:.cpp=.win.debug.o)

# Target executables
TARGET_LINUX = tetris
TARGET_WIN = tetris.exe
TARGET_WIN_SDL = tetris.exe
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

# Sound file settings
SOUND_DIR = sound
SOUND_ZIP = sound.zip

# Audio conversion settings
WAV_FILES := $(wildcard $(SOUND_DIR)/*.wav)
MIDI_FILES := $(wildcard $(SOUND_DIR)/*.mid)
WAV_FROM_MIDI := $(MIDI_FILES:.mid=.wav)
MP3_FROM_WAV := $(WAV_FILES:.wav=.mp3) $(WAV_FROM_MIDI:.wav=.mp3)

# Combined theme file for Windows
THEME_ALL_MP3 = $(SOUND_DIR)/themeall.mp3

# FFmpeg command for audio conversion
FFMPEG = ffmpeg
FFMPEG_OPTS = -y -loglevel error -i
FFMPEG_MP3_OPTS = -af "silenceremove=stop_periods=-1:stop_duration=1:stop_threshold=-50dB" -codec:a libmp3lame -qscale:a 2

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
tetris-linux: $(BUILD_DIR_LINUX)/$(TARGET_LINUX) pack-backgrounds-linux convert-midi convert-wav-to-mp3 pack-sounds

$(BUILD_DIR_LINUX)/$(TARGET_LINUX): $(addprefix $(BUILD_DIR_LINUX)/,$(OBJS_LINUX))
	$(CXX_LINUX) $^ -o $@ $(LDFLAGS_LINUX)

# Generic compilation rules for Linux
$(BUILD_DIR_LINUX)/%.o: %.cpp
	$(CXX_LINUX) $(CXXFLAGS_LINUX) -c $< -o $@

#
# Linux debug targets
#
.PHONY: tetris-linux-debug
tetris-linux-debug: $(BUILD_DIR_LINUX_DEBUG)/$(TARGET_LINUX_DEBUG) pack-backgrounds-linux-debug convert-midi convert-wav-to-mp3 pack-sounds link-sound-linux-debug

$(BUILD_DIR_LINUX_DEBUG)/$(TARGET_LINUX_DEBUG): $(addprefix $(BUILD_DIR_LINUX_DEBUG)/,$(OBJS_LINUX_DEBUG))
	$(CXX_LINUX) $^ -o $@ $(LDFLAGS_LINUX)

# Generic compilation rules for Linux debug
$(BUILD_DIR_LINUX_DEBUG)/%.debug.o: %.cpp
	$(CXX_LINUX) $(CXXFLAGS_LINUX_DEBUG) -c $< -o $@

#
# Windows build targets
#
.PHONY: tetris-windows
tetris-windows: $(BUILD_DIR_WIN)/$(TARGET_WIN) tetris-collect-dlls pack-backgrounds-windows convert-midi convert-wav-to-mp3 pack-sounds link-sound-windows

$(BUILD_DIR_WIN)/$(TARGET_WIN): $(addprefix $(BUILD_DIR_WIN)/,$(OBJS_WIN))
	$(CXX_WIN) $^ -o $@ $(LDFLAGS_WIN)

# Generic compilation rules for Windows
$(BUILD_DIR_WIN)/%.win.o: %.cpp
	$(CXX_WIN) $(CXXFLAGS_WIN) -c $< -o $@

# Windows SDL build target
.PHONY: tetris-windows-sdl
tetris-windows-sdl: $(BUILD_DIR_WIN)/$(TARGET_WIN_SDL) tetris-collect-sdl-dlls pack-backgrounds-windows convert-midi convert-wav-to-mp3 pack-sounds link-sound-windows

$(BUILD_DIR_WIN)/$(TARGET_WIN_SDL): $(addprefix $(BUILD_DIR_WIN)/,$(OBJS_WIN_SDL))
	$(CXX_WIN) $^ -o $@ $(LDFLAGS_WIN)

# DLL collection for Windows SDL build
.PHONY: tetris-collect-sdl-dlls
tetris-collect-sdl-dlls: $(BUILD_DIR_WIN)/$(TARGET_WIN_SDL)
	@echo "Collecting DLLs for Tetris SDL..."
	@build/windows/collect_dlls.sh $(BUILD_DIR_WIN)/$(TARGET_WIN_SDL) $(DLL_SOURCE_DIR) $(BUILD_DIR_WIN)

#
# Windows debug targets
#
.PHONY: tetris-windows-debug
tetris-windows-debug: $(BUILD_DIR_WIN_DEBUG)/$(TARGET_WIN_DEBUG) tetris-collect-debug-dlls pack-backgrounds-windows-debug convert-midi convert-wav-to-mp3 pack-sounds link-sound-windows-debug

$(BUILD_DIR_WIN_DEBUG)/$(TARGET_WIN_DEBUG): $(addprefix $(BUILD_DIR_WIN_DEBUG)/,$(OBJS_WIN_DEBUG))
	$(CXX_WIN) $(CXXFLAGS_WIN_DEBUG) $^ -o $@ $(LDFLAGS_WIN)

# Generic compilation rules for Windows debug
$(BUILD_DIR_WIN_DEBUG)/%.win.debug.o: %.cpp
	$(CXX_WIN) $(CXXFLAGS_WIN_DEBUG) -c $< -o $@

#
# MIDI to WAV conversion
#
.PHONY: convert-midi
convert-midi: $(WAV_FROM_MIDI)

# Rule to convert .mid to .wav files
%.wav: %.mid
	@echo "Converting $< to $@ using FluidSynth..."
	@$(FLUIDSYNTH) $(FLUIDSYNTH_OPTS) $@ $(SOUNDFONT) $<

#
# WAV to MP3 conversion
#
.PHONY: convert-wav-to-mp3
convert-wav-to-mp3: convert-midi $(MP3_FROM_WAV) $(THEME_ALL_MP3)

# Rule to convert .wav to .mp3 files
%.mp3: %.wav
	@echo "Converting $< to $@..."
	@$(FFMPEG) $(FFMPEG_OPTS) $< $(FFMPEG_MP3_OPTS) $@

# Rule to create the combined theme MP3 file from multiple WAV files
$(THEME_ALL_MP3): $(SOUND_DIR)/theme.wav $(SOUND_DIR)/TetrisA.wav $(SOUND_DIR)/TetrisB.wav $(SOUND_DIR)/TetrisC.wav $(SOUND_DIR)/futuristic.wav
	@echo "Creating combined theme file $(THEME_ALL_MP3)..."
	@$(FFMPEG) -y \
		-i "$(SOUND_DIR)/theme.wav" \
		-i "$(SOUND_DIR)/TetrisA.wav" \
		-i "$(SOUND_DIR)/TetrisB.wav" \
		-i "$(SOUND_DIR)/TetrisC.wav" \
		-i "$(SOUND_DIR)/futuristic.wav" \
		-filter_complex "\
			[0:a]silenceremove=stop_periods=-1:stop_duration=1:stop_threshold=-50dB[a0];\
			[1:a]silenceremove=stop_periods=-1:stop_duration=1:stop_threshold=-50dB[a1];\
			[2:a]silenceremove=stop_periods=-1:stop_duration=1:stop_threshold=-50dB[a2];\
			[3:a]silenceremove=stop_periods=-1:stop_duration=1:stop_threshold=-50dB[a3];\
			[4:a]silenceremove=stop_periods=-1:stop_duration=1:stop_threshold=-50dB[a4];\
			[a0][a1][a2][a3][a4]concat=n=5:v=0:a=1[outaudio]" \
		-map "[outaudio]" \
		-c:a libmp3lame -qscale:a 2 \
		$(THEME_ALL_MP3)

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

#
# Sound file packing and linking
#
.PHONY: pack-sounds
pack-sounds: convert-wav-to-mp3
	@echo "Creating sound.zip with MP3 files in sound directory..."
	cd $(SOUND_DIR) && zip -r $(SOUND_ZIP) *.mp3
	@echo "MP3 files packed to $(SOUND_DIR)/$(SOUND_ZIP)"
	@$(MAKE) link-sound-linux

.PHONY: link-sound-linux
link-sound-linux:
	@echo "Linking sound.zip to Linux build directory..."
	ln -sf ../../$(SOUND_DIR)/$(SOUND_ZIP) $(BUILD_DIR_LINUX)/$(SOUND_ZIP)

.PHONY: link-sound-linux-debug
link-sound-linux-debug:
	@echo "Linking sound.zip to Linux debug build directory..."
	ln -sf ../../$(SOUND_DIR)/$(SOUND_ZIP) $(BUILD_DIR_LINUX_DEBUG)/$(SOUND_ZIP)

.PHONY: link-sound-windows
link-sound-windows:
	@echo "Linking sound.zip to Windows build directory..."
	ln -sf ../../$(SOUND_DIR)/$(SOUND_ZIP) $(BUILD_DIR_WIN)/$(SOUND_ZIP)

.PHONY: link-sound-windows-debug
link-sound-windows-debug:
	@echo "Linking sound.zip to Windows debug build directory..."
	ln -sf ../../$(SOUND_DIR)/$(SOUND_ZIP) $(BUILD_DIR_WIN_DEBUG)/$(SOUND_ZIP)

.PHONY: link-sound-all
link-sound-all: link-sound-linux link-sound-linux-debug link-sound-windows link-sound-windows-debug

# Clean target
.PHONY: clean
clean:
	find $(BUILD_DIR) -type f -name "*.o" -delete
	find $(BUILD_DIR) -type f -name "*.dll" -delete
	find $(BUILD_DIR) -type f -name "*.exe" -delete
	find $(BUILD_DIR) -type f -name "$(BACKGROUND_ZIP)" -delete
	rm -f $(BUILD_DIR_LINUX)/$(TARGET_LINUX)
	rm -f $(BUILD_DIR_LINUX_DEBUG)/$(TARGET_LINUX_DEBUG)
	rm -f $(BUILD_DIR_WIN)/$(TARGET_WIN_SDL)
	rm -f $(SOUND_DIR)/$(SOUND_ZIP)

# Clean converted audio files
.PHONY: clean-audio
clean-audio:
	@echo "Cleaning converted audio files..."
	@for midi in $(MIDI_FILES); do \
		wav_file="$${midi%.mid}.wav"; \
		mp3_file="$${midi%.mid}.mp3"; \
		if [ -f "$$wav_file" ]; then \
			echo "Removing $$wav_file"; \
			rm -f "$$wav_file"; \
		fi; \
		if [ -f "$$mp3_file" ]; then \
			echo "Removing $$mp3_file"; \
			rm -f "$$mp3_file"; \
		fi; \
	done
	@for wav in $(WAV_FILES); do \
		mp3_file="$${wav%.wav}.mp3"; \
		if [ -f "$$mp3_file" ]; then \
			echo "Removing $$mp3_file"; \
			rm -f "$$mp3_file"; \
		fi; \
	done
	@if [ -f "$(THEME_ALL_MP3)" ]; then \
		echo "Removing $(THEME_ALL_MP3)"; \
		rm -f "$(THEME_ALL_MP3)"; \
	fi

# Full clean including converted audio files
.PHONY: clean-all
clean-all: clean clean-audio

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
	@echo "  make tetris-windows-sdl - Build Tetris for Windows with SDL audio"
	@echo ""
	@echo "  make debug         - Build Tetris with debug symbols (using SDL for Linux)"
	@echo "  make sdl-debug     - Build Tetris with debug symbols using SDL audio"
	@echo "  make pulse-debug   - Build Tetris with debug symbols using PulseAudio"
	@echo ""
	@echo "  make tetris-linux  - Build Tetris for Linux (with current audio backend)"
	@echo "  make tetris-windows - Build Tetris for Windows (requires MinGW)"
	@echo ""
	@echo "  make convert-midi  - Convert MIDI files to WAV format using ffmpeg"
	@echo "  make convert-wav-to-mp3 - Convert WAV files to MP3 format using ffmpeg"
	@echo ""
	@echo "  make pack-backgrounds-all - Pack background images for all build targets"
	@echo "  make pack-backgrounds-linux - Pack background images for Linux build"
	@echo "  make pack-backgrounds-windows - Pack background images for Windows build"
	@echo ""
	@echo "  make pack-sounds   - Pack MP3 sound files and create symbolic links"
	@echo "  make link-sound-all - Create symbolic links to sound.zip in all build directories"
	@echo ""
	@echo "  make clean         - Remove all build files"
	@echo "  make clean-audio   - Remove all converted MIDI and MP3 files"
	@echo "  make clean-all     - Remove all build files and converted audio files"
	@echo "  make help          - Show this help message"
