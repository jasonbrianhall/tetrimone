# GTK Tetris: Pixel-Dropping Pandemonium! 🧩🚀

## Overview

Prepare for a block-stacking adventure that'll make your neurons dance! GTK Tetris is a pulse-pounding reimagining of the classic puzzle game, powered by GTK+, SDL, and flexible audio backends. Warning: May cause sudden outbursts of strategic genius and controller-gripping intensity!

## Features

### Gameplay Madness
- **Classic Tetris Mechanics**: Because some legends never die
- **Keyboard Ninja Controls**: Move, rotate, and drop with lightning reflexes
- **Gamepad Support**: For those who prefer their tetrominos with analog swagger
- **20 Color Themes**: Watch your world transform as you level up
- **Difficulty Levels**: From "Casual Puzzler" to "Tetris Terminator"

### Audio Awesomeness
- **Pumping Background Music**: Powered by PulseAudio or SDL2 audio
- **Sound Effects**: Every block drop, rotation, and line clear has its own sonic personality
- **Mute Button**: For when you need to sneak in some tetromino action

## Controls

### Keyboard Warriors
- **Left/Right Arrow or A/D**: Slide those blocks
- **Up Arrow or W**: Rotate clockwise
- **Z**: Rotate counter-clockwise
- **Down Arrow or S**: Soft drop for the impatient
- **Space**: HARD DROP - For when subtlety is overrated
- **P**: Pause/Resume - Because life happens
- **R**: Restart - Defeat is just a temporary state

### Gamepad Gladiators
- **Directional Pad/Stick**: Navigate the block battlefield
- **A Button**: Rotate clockwise
- **B Button**: Rotate counter-clockwise
- **Start Button**: Game control central

## Installation Dependencies

### Required Libraries
- GTK+ 3.0
- SDL2 (for joystick support)
- SDL2_mixer (optional for Linux; Windows uses winmm; must use SDL Mixer or pulse)
- PulseAudio (optional; see above)
- libzip

### Compilation Arsenal
- CMake or Make
- C++17 or later compiler

## Build Instructions

### Linux Tetris Taming

#### Debian/Ubuntu
```bash
# Summon the dependencies
sudo apt-get update
sudo apt-get install libgtk-3-dev libsdl2-dev libsdl2-mixer-dev libpulse-dev libzip-dev gcc g++ make

# Clone the block-dropping battlefield
git clone https://github.com/jasonbrianhall/tetris.git
cd tetris

# Build with default SDL audio
make

# Alternative build options:
# make sdl           # Explicitly use SDL audio
# make pulse         # Use PulseAudio (still uses SDL for joystick)
# make debug         # Build with debug symbols
# make sdl-debug     # Debug build with SDL audio
# make pulse-debug   # Debug build with PulseAudio
```

#### Fedora/RHEL/CentOS
```bash
# Summon the dependencies
sudo dnf groupinstall "Development Tools"
sudo dnf install gtk3-devel SDL2-devel SDL2_mixer-devel pulseaudio-libs-devel libzip-devel gcc-c++

# Clone the block-dropping battlefield
git clone https://github.com/jasonbrianhall/tetris.git
cd tetris

# Build with default SDL audio
make

# Alternative build options:
# make sdl           # Explicitly use SDL audio
# make pulse         # Use PulseAudio (still uses SDL for joystick)
# make debug         # Build with debug symbols
# make sdl-debug     # Debug build with SDL audio
# make pulse-debug   # Debug build with PulseAudio
```

## Scoring: The Tetris Triumph Scale

- **1 line**: 40 × level (Appetizer)
- **2 lines**: 100 × level (Light Meal)
- **3 lines**: 300 × level (Feast)
- **4 lines**: 1200 × level (TETRIS GODMODE!)

## Difficulty Levels: Choose Your Destiny

- **Zen**: For those who are about to rock but aren't ready yet
- **Easy**: For the block-dropping novice
- **Medium**: The classic Tetris experience
- **Hard**: For those who laugh in the face of falling blocks
- **Extreme**: Brace yourself
- **Instance**: A good way to lose in less then a minute

## Sound Configuration
Sounds loaded from `sound.zip`. Dare to customize? Replace the audio files and make the game your own sonic playground!

### Windows Build
```bash
# Requires MinGW
make windows

# The Windows binary will be located at:
# build/windows/tetris.exe
```

#### Linux Build Locations
```bash
# Standard build
build/linux/tetris

# Debug build
build/linux_debug/tetris
```

## Troubleshooting
- Encountering issues? Run `make debug` for detailed diagnostic information
- Verify all dependencies are correctly installed
- Check the generated debug logs for specific error details

## Contributing
1. Fork the repository
2. Create a feature branch
3. Code like a Tetris god
4. Push your block-dropping brilliance
5. Submit a Pull Request

## License
MIT License - Spread the Tetris love freely!

## Credits
- **Developer**: Jason Brian Hall - Tetris Mastermind
- **Libraries**: GTK+, SDL2, PulseAudio/SDL2 Audio, libzip, winmm

**May your blocks always fall true, and your lines always clear!** 🕹️✨

This game is based on Tetris®, created by Alexey Pajitnov in 1984.
Tetris® is a registered trademark of The Tetris Company, LLC.

Original game published by ELORG (also, go watch the Tetris movie on AppleTV)

## TODO

* Thought about multiplayer support but didn't really architect it for MP support.  Would require a great deal of reengineering.

