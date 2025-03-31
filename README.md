# GTK Tetrimone: Pixel-Dropping Pandemonium! 🧩🚀

## Overview

Prepare for a block-stacking adventure that'll make your neurons dance! GTK Tetrimone is a pulse-pounding reimagining of the classic puzzle game, powered by GTK+, SDL, and flexible audio backends. Warning: May cause sudden outbursts of strategic genius and controller-gripping intensity!

## Features

### Gameplay Madness
- **Classic Tetrimone Mechanics**: Because some legends never die
- **Keyboard Ninja Controls**: Move, rotate, and drop with lightning reflexes
- **Gamepad Support**: For those who prefer their tetrominos with analog swagger
- **20 Color Themes**: Watch your world transform as you level up
- **Difficulty Levels**: From "Casual Puzzler" to "Tetrimone Terminator"

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

### Linux Tetrimone Taming

#### Debian/Ubuntu
```bash
# Summon the dependencies
sudo apt-get update
sudo apt-get install libgtk-3-dev libsdl2-dev libsdl2-mixer-dev libpulse-dev libzip-dev gcc g++ make

# Clone the block-dropping battlefield
git clone https://github.com/jasonbrianhall/tetrimone.git
cd tetrimone

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
git clone https://github.com/jasonbrianhall/tetrimone.git
cd tetrimone

# Build with default SDL audio
make

# Alternative build options:
# make sdl           # Explicitly use SDL audio
# make pulse         # Use PulseAudio (still uses SDL for joystick)
# make debug         # Build with debug symbols
# make sdl-debug     # Debug build with SDL audio
# make pulse-debug   # Debug build with PulseAudio
```

## Scoring: The Tetrimone Triumph Scale

- **1 line**: 40 × level (Appetizer)
- **2 lines**: 100 × level (Light Meal)
- **3 lines**: 300 × level (Feast)
- **4 lines**: 1200 × level (TETRIS GODMODE!)

## Difficulty Levels: Choose Your Destiny

- **Zen**: For those who are about to rock but aren't ready yet
- **Easy**: For the block-dropping novice
- **Medium**: The classic Tetrimone experience
- **Hard**: For those who laugh in the face of falling blocks
- **Extreme**: Brace yourself
- **Insane**: A good way to lose in less then a minute

## Sound Configuration
Sounds loaded from `sound.zip`. Dare to customize? Replace the audio files and make the game your own sonic playground!

### Windows Build
```bash
# Requires MinGW
make windows

# The Windows binary will be located at:
# build/windows/tetrimone.exe
```

#### Linux Build Locations
```bash
# Standard build
build/linux/tetrimone

# Debug build
build/linux_debug/tetrimone
```

## Troubleshooting
- Encountering issues? Run `make debug` for detailed diagnostic information
- Verify all dependencies are correctly installed
- Check the generated debug logs for specific error details

## Contributing
1. Fork the repository
2. Create a feature branch
3. Code like a Tetrimone god
4. Push your block-dropping brilliance
5. Submit a Pull Request

## License
MIT License - Spread the Tetrimone love freely!

## Credits
- **Developer**: Jason Brian Hall - Tetrimone Mastermind
- **Libraries**: GTK+, SDL2, PulseAudio/SDL2 Audio, libzip

**May your blocks always fall true, and your lines always clear!** 🕹️✨

This game is based on Tetrimone®, created by Alexey Pajitnov in 1984.
Tetrimone® is a registered trademark of The Tetrimone Company, LLC.

Original game published by ELORG (also, go watch the Tetrimone movie on AppleTV)

## TODO

* Thought about multiplayer support but didn't really architect it for MP support.  Would require a great deal of reengineering.

