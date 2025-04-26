# Last modification 04/04/2025

# Makefile for Tetrimone with Windows and Linux Support
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
CXXFLAGS_COMMON = -std=c++17 -Wall -Wextra -s -fpermissive

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
SRCS_COMMON = src/tetrimone.cpp src/audiomanager.cpp src/sound.cpp src/joystick.cpp src/background.cpp src/audioconverter.cpp src/volume.cpp src/ghostpiece.cpp src/highscores.cpp src/icon.cpp src/dbopl.cpp src/dbopl_wrapper.cpp src/instruments.cpp src/midiplayer.cpp src/virtual_mixer.cpp src/wav_converter.cpp src/convertmidi.cpp
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
TARGET_LINUX = tetrimone
TARGET_WIN = tetrimone.exe
TARGET_WIN_SDL = tetrimone.exe
TARGET_LINUX_DEBUG = tetrimone_debug
TARGET_WIN_DEBUG = tetrimone_debug.exe

# Build directories
BUILD_DIR = build
BUILD_DIR_LINUX = $(BUILD_DIR)/linux
BUILD_DIR_WIN = $(BUILD_DIR)/windows
BUILD_DIR_LINUX_DEBUG = $(BUILD_DIR)/linux_debug
BUILD_DIR_WIN_DEBUG = $(BUILD_DIR)/windows_debug

# Windows DLL settings
DLL_SOURCE_DIR = /usr/x86_64-w64-mingw32/sys-root/mingw/bin

# Background image settings
BACKGROUNDS_DIR = images/Tetrimone_backgrounds
BACKGROUND_ZIP = background.zip

# Sound file settings
SOUND_DIR = sound
SOUND_ZIP = sound.zip

# Audio conversion settings
WAV_FILES := $(wildcard $(SOUND_DIR)/*.wav)
MIDI_FILES := $(wildcard $(SOUND_DIR)/*.mid)
WAV_FROM_MIDI := $(MIDI_FILES:.mid=.wav)
MP3_FROM_WAV := $(WAV_FILES:.wav=.mp3) $(WAV_FROM_MIDI:.wav=.mp3)

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
windows: tetrimone-windows

.PHONY: linux
linux: tetrimone-linux

# Audio-specific builds
.PHONY: sdl
sdl:
	$(MAKE) linux AUDIO_BACKEND=sdl

.PHONY: pulse
pulse:
	$(MAKE) linux AUDIO_BACKEND=pulse

# Debug targets
.PHONY: debug
debug: tetrimone-linux-debug tetrimone-windows-debug

# Debug with specific audio backend
.PHONY: sdl-debug
sdl-debug:
	$(MAKE) tetrimone-linux-debug AUDIO_BACKEND=sdl

.PHONY: pulse-debug
pulse-debug:
	$(MAKE) tetrimone-linux-debug AUDIO_BACKEND=pulse

#
# Linux build targets
#
.PHONY: tetrimone-linux
tetrimone-linux: $(BUILD_DIR_LINUX)/$(TARGET_LINUX) pack-backgrounds-linux convert-midi convert-wav-to-mp3 pack-sounds

$(BUILD_DIR_LINUX)/$(TARGET_LINUX): $(addprefix $(BUILD_DIR_LINUX)/,$(OBJS_LINUX))
	$(CXX_LINUX) $^ -o $@ $(LDFLAGS_LINUX)

# Generic compilation rules for Linux
$(BUILD_DIR_LINUX)/%.o: %.cpp
	$(CXX_LINUX) $(CXXFLAGS_LINUX) -c $< -o $@

#
# Linux debug targets
#
.PHONY: tetrimone-linux-debug
tetrimone-linux-debug: $(BUILD_DIR_LINUX_DEBUG)/$(TARGET_LINUX_DEBUG) pack-backgrounds-linux-debug convert-midi convert-wav-to-mp3 pack-sounds link-sound-linux-debug

$(BUILD_DIR_LINUX_DEBUG)/$(TARGET_LINUX_DEBUG): $(addprefix $(BUILD_DIR_LINUX_DEBUG)/,$(OBJS_LINUX_DEBUG))
	$(CXX_LINUX) $^ -o $@ $(LDFLAGS_LINUX)

# Generic compilation rules for Linux debug
$(BUILD_DIR_LINUX_DEBUG)/%.debug.o: %.cpp
	$(CXX_LINUX) $(CXXFLAGS_LINUX_DEBUG) -c $< -o $@

#
# Windows build targets
#
.PHONY: tetrimone-windows
tetrimone-windows: $(BUILD_DIR_WIN)/$(TARGET_WIN) tetrimone-collect-dlls pack-backgrounds-windows convert-midi convert-wav-to-mp3 pack-sounds link-sound-windows

$(BUILD_DIR_WIN)/$(TARGET_WIN): $(addprefix $(BUILD_DIR_WIN)/,$(OBJS_WIN))
	$(CXX_WIN) $^ -o $@ $(LDFLAGS_WIN)

# Generic compilation rules for Windows
$(BUILD_DIR_WIN)/%.win.o: %.cpp
	$(CXX_WIN) $(CXXFLAGS_WIN) -c $< -o $@

#
# Windows debug targets
#
.PHONY: tetrimone-windows-debug
tetrimone-windows-debug: $(BUILD_DIR_WIN_DEBUG)/$(TARGET_WIN_DEBUG) tetrimone-collect-debug-dlls pack-backgrounds-windows-debug convert-midi convert-wav-to-mp3 pack-sounds link-sound-windows-debug

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
convert-wav-to-mp3: convert-midi $(MP3_FROM_WAV)

# Rule to convert .wav to .mp3 files
%.mp3: %.wav
	@echo "Converting $< to $@..."
	@$(FFMPEG) $(FFMPEG_OPTS) $< $(FFMPEG_MP3_OPTS) $@

#
# DLL collection for Windows builds
#
.PHONY: tetrimone-collect-dlls
tetrimone-collect-dlls: $(BUILD_DIR_WIN)/$(TARGET_WIN)
	@echo "Collecting DLLs for Tetrimone..."
	@build/windows/collect_dlls.sh $(BUILD_DIR_WIN)/$(TARGET_WIN) $(DLL_SOURCE_DIR) $(BUILD_DIR_WIN)

.PHONY: tetrimone-collect-debug-dlls
tetrimone-collect-debug-dlls: $(BUILD_DIR_WIN_DEBUG)/$(TARGET_WIN_DEBUG)
	@echo "Collecting Debug DLLs for Tetrimone..."
	@build/windows/collect_dlls.sh $(BUILD_DIR_WIN_DEBUG)/$(TARGET_WIN_DEBUG) $(DLL_SOURCE_DIR) $(BUILD_DIR_WIN_DEBUG)

#
# Background image packing
#
.PHONY: pack-backgrounds-linux
pack-backgrounds-linux:
	@echo "Packing background images for Windows build..."
	cd $(BACKGROUNDS_DIR) && zip -r ../../$(BUILD_DIR_LINUX)/$(BACKGROUND_ZIP) *.jpg;
	@echo "Background images packed to $(BUILD_DIR_LINUX)/$(BACKGROUND_ZIP)"

.PHONY: pack-backgrounds-linux-debug
pack-backgrounds-linux-debug:
	@echo "Packing background images for Windows build..."
	cd $(BACKGROUNDS_DIR) && zip -r ../../$(BUILD_DIR_LINUX_DEBUG)/$(BACKGROUND_ZIP) *.jpg;
	@echo "Background images packed to $(BUILD_DIR_LINUX_DEBUG)/$(BACKGROUND_ZIP)"

.PHONY: pack-backgrounds-windows
pack-backgrounds-windows:
	@echo "Packing background images for Windows build..."
	cd $(BACKGROUNDS_DIR) && zip -r ../../$(BUILD_DIR_WIN)/$(BACKGROUND_ZIP) *.jpg;
	@echo "Background images packed to $(BUILD_DIR_WIN)/$(BACKGROUND_ZIP)"
	
.PHONY: pack-backgrounds-windows-debug
pack-backgrounds-windows-debug:
	@echo "Packing background images for Windows build..."
	cd $(BACKGROUNDS_DIR) && zip -r ../../$(BUILD_DIR_WIN_DEBUG)/$(BACKGROUND_ZIP) *.jpg;
	@echo "Background images packed to $(BUILD_DIR_WIN_DEBUG)/$(BACKGROUND_ZIP)"
	
.PHONY: pack-backgrounds-all
pack-backgrounds-all: pack-backgrounds-linux pack-backgrounds-linux-debug pack-backgrounds-windows pack-backgrounds-windows-debug

#
# Sound file packing and linking
#
.PHONY: pack-sounds
pack-sounds: convert-wav-to-mp3
	@echo "Creating sound.zip with MP3 files in sound directory..."
	cd $(SOUND_DIR) && zip -r $(SOUND_ZIP) *.mp3 *.mid
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

# Full clean including converted audio files
.PHONY: clean-all
clean-all: clean clean-audio

# Help target
.PHONY: help
help:
	@echo "Available targets:"
	@echo "  make               - Build Tetrimone for Linux with SDL audio (default)"
	@echo "  make linux         - Build Tetrimone for Linux with SDL audio"
	@echo "  make windows       - Build Tetrimone for Windows"
	@echo ""
	@echo "  make sdl           - Build Tetrimone for Linux with SDL audio explicitly"
	@echo "  make pulse         - Build Tetrimone for Linux with PulseAudio (still uses SDL for joystick)"
	@echo ""
	@echo "  make tetrimone-windows-sdl - Build Tetrimone for Windows with SDL audio"
	@echo ""
	@echo "  make debug         - Build Tetrimone with debug symbols (using SDL for Linux)"
	@echo "  make sdl-debug     - Build Tetrimone with debug symbols using SDL audio"
	@echo "  make pulse-debug   - Build Tetrimone with debug symbols using PulseAudio"
	@echo ""
	@echo "  make tetrimone-linux  - Build Tetrimone for Linux (with current audio backend)"
	@echo "  make tetrimone-windows - Build Tetrimone for Windows (requires MinGW)"
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
	@echo "  make clean-audio   - Remove all converted audio files"
	@echo "  make clean-all     - Remove all build files and converted audio files"
	@echo "  make help          - Show this help message"
