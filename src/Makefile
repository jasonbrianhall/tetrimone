# Compiler and flags
CXX = g++
CXXFLAGS = -Wall -std=c++17 -I. $(shell sdl2-config --cflags)

# Detect OS for platform-specific settings
UNAME_S := $(shell uname -s)
ifeq ($(UNAME_S),Linux)
    AUDIO_SOURCE = pulseaudioplayer.cpp
    CXXFLAGS += -DLINUX
    LIBS = -lgtk-3 -lgdk-3 -lpangocairo-1.0 -lpango-1.0 -lcairo -lgdk_pixbuf-2.0 \
           -latk-1.0 -lgio-2.0 -lgobject-2.0 -lglib-2.0 -lzip -lpulse -lpulse-simple -pthread \
           $(shell sdl2-config --libs)
    INCLUDES = $(shell pkg-config --cflags gtk+-3.0)
else
    AUDIO_SOURCE = windowsaudioplayer.cpp
    CXXFLAGS += -DWIN32
    LIBS = -lgtk-3 -lgdk-3 -lpangocairo-1.0 -lpango-1.0 -lcairo -lgdk_pixbuf-2.0 \
           -latk-1.0 -lgio-2.0 -lgobject-2.0 -lglib-2.0 -lzip -lwinmm \
           $(shell sdl2-config --libs)
    INCLUDES = $(shell pkg-config --cflags gtk+-3.0)
endif

# Source files
SOURCES = tetris.cpp audiomanager.cpp sound.cpp joystick.cpp $(AUDIO_SOURCE)
OBJECTS = $(SOURCES:.cpp=.o)
TARGET = tetris

# Main target
all: $(TARGET)

# Build the executable
$(TARGET): $(OBJECTS)
	$(CXX) $(CXXFLAGS) -o $@ $^ $(LIBS)

# Rule for object files
%.o: %.cpp
	$(CXX) $(CXXFLAGS) $(INCLUDES) -c $< -o $@

# Clean rule
clean:
	rm -f $(OBJECTS) $(TARGET)

# Phony targets
.PHONY: all clean
