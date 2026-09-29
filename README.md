# GTK Tetrimone: Pixel-Dropping Pandemonium! 🧩🚀

## Tetrimone: The Perfect Fusion of Gaming Elements

### Tetrimone Meaning

"Tetrimone" brilliantly combines "tetromino" (the four-block geometric pieces that are the heart of the gameplay) with a hint of "metronome" (suggesting the rhythmic, pulse-pounding timing that drives the game forward).

This ingenious portmanteau captures the essence of what makes the game so addictive—geometric precision meeting musical rhythm in a perfect harmony of gaming elements. The name evokes both the mathematical elegance of the falling shapes and the escalating tempo that challenges players as they progress through levels.

The "-one" suffix also subtly suggests "the one"—positioning this as the definitive, ultimate version of the classic block-dropping experience. It's not just another clone; it's a reimagining that honors the legacy while carving its own unique identity in the puzzle game pantheon.

Tetrimone stands as a testament to gaming history while boldly facing the future—each carefully placed block building not just toward line clears, but toward gaming greatness!

## Overview

Prepare for a block-stacking adventure that'll make your neurons dance! GTK Tetrimone is a pulse-pounding reimagining of the classic puzzle game, powered by GTK+, SDL, and flexible audio backends. Warning: May cause sudden outbursts of strategic genius and controller-gripping intensity!

## Features

### Gameplay Madness
- **Classic Tetrimone Mechanics**: Because some legends never die
- **Keyboard Ninja Controls**: Move, rotate, and drop with lightning reflexes
- **Gamepad Support**: For those who prefer their tetrominos with analog swagger
- **30 Color Themes**: Watch your world transform as you level up
- **Difficulty Levels**: From "Casual Puzzler" to "Tetrimone Terminator"
- **Initial Block Level**: From 0-50%; Punish yourself with higher initial junk blocks.
- **Blocks Per Level**: Add anywhere from zero to five junk lines per level; the higher the number, the faster you lose.
- **Control Block Shapes**: Initial settings include mononomes, dominoes, triominoes, and tetrominoes.  Don't like mononomes, domnioes, or triominoes, remove them.
- **Retro Blocks**: You like retro blocks, configure it.
- **Heat Sensor**: It gets warmer or colder depending how fast blocks are laid and conquered.
- **Fireworks**: Four lines cleared will generate a fireworks animation.


### Audio Awesomeness
- **Pumping Background Music**: Powered by PulseAudio or SDL2 audio
- **Sound Effects**: Every block drop, rotation, and line clear has its own sonic personality
- **Mute Button**: For when you need to sneak in some tetromino action
- **Retro Music Support**: Wrote a classic MIDI player that plays 8 bit sounds (based on DOSBOX's excellent OPL3SA emulation and some QBASIC code I tactically acquired and converted to C).  Feel like the MS-DOS days.  **NOTE**:  Current music must play through one time for it to change until I fix that.
- **Custom Music support**: If you don't like the themes, their is a sound.zip file, simply replace with your own music and and sound (mp3, midi, and wav support).  Must use midi for retromode.

### 🎬 Animation Spectacular: Where Blocks Come Alive! ✨

#### Smooth Movement System
Your tetrominos don't just teleport—they **glide** with cinematic grace! Every piece movement is interpolated with silky-smooth 60 FPS animations using advanced easing functions. Watch as your blocks flow across the grid like digital poetry in motion.

#### Epic Line Clear Animations 🌟

When you clear lines, prepare for a visual feast! Tetrimone features **11 spectacular line clearing animations** that transform mundane line clears into jaw-dropping spectacles:

#### 🔥 Modern Animation Arsenal (10 Types)
Each line clear randomly selects from these visual masterpieces:

1. **🌀 Classic Shrink & Scatter**: Blocks flash, shrink, then scatter like digital confetti
2. **✨ Dissolve**: Blocks fade away randomly with ethereal beauty  
3. **🌊 Ripple Wave**: A wave effect emanates from the center, blocks pulse and vanish
4. **💥 Explosion**: Blocks shoot outward from the center in a spectacular burst
5. **🌪️ Spin & Vanish**: Blocks rotate frantically before disappearing into the void
6. **➡️ Left-to-Right Sweep**: Blocks slide gracefully off-screen in sequence
7. **🎈 Bounce & Pop**: Blocks bounce up, inflate, then pop like balloons
8. **🫠 Melt Down**: Blocks appear to melt and sink downward
9. **⚡ Zigzag Wipe**: Blocks disappear in a mesmerizing zigzag pattern
10. **🎆 Fireworks Burst**: Blocks compress, explode, then sparkle like fireworks

#### 🚩 Soviet Retro Mode Animation
When in retro mode (press the `.` key), line clears transform into authentic **Soviet-era computer animations**:
- **Scan Line Sweep**: Horizontal scan lines sweep across like old CRT monitors
- **Flash & Collapse**: Blocks flash in unison with vintage monitor effects  
- **Segment Wipe**: Blocks disappear in chunks, mimicking 1980s computer graphics

### Visual Polish Features

#### 🎨 3D Block Rendering
- **Highlight Effects**: Each block features realistic 3D lighting
- **Shadow System**: Dynamic shadows create depth and dimensionality
- **Retro Mode Override**: Simple flat blocks for authentic vintage feel

#### 👻 Ghost Piece Technology
- **Predictive Positioning**: See exactly where your piece will land
- **Semi-Transparent Rendering**: Subtle visual guidance without distraction
- **Smart Visibility**: Only appears when different from current piece position

#### 🎭 Background Animation Support
- **Image Transitions**: Smooth fading between background images
- **Opacity Control**: Customizable background transparency
- **Scaling Technology**: Perfect aspect ratio maintenance

#### 🌈 Theme Transition Support
- **Transition Technology** 3-second smooth color morphing between themes with cubic ease-in-out

#### 

### Performance Optimization

#### ⚡ 60 FPS Rendering
- **16ms Update Cycles**: Buttery-smooth 60 frames per second
- **Efficient Animation Timers**: Minimal CPU overhead
- **Smart Animation Culling**: Only animates visible elements

#### 🧠 Memory Management
- **Timer Cleanup**: Automatic cleanup prevents memory leaks
- **State Tracking**: Efficient animation state management
- **Resource Optimization**: Minimal memory footprint

### Animation Control

#### 🎛️ Customization Options
- **Animation Duration**: Configurable timing (default: 800ms)
- **Retro Mode Toggle**: Instant switching between modern and vintage effects
- **Performance Scaling**: Adapts to system capabilities

#### 🎯 Technical Details
- **Interpolation**: Smoothstep easing functions for natural motion
- **Multi-line Support**: Handles singles, doubles, triples, and TETRIMONES simultaneously
- **Race Condition Prevention**: Bulletproof timing synchronization

---

**The Result?** Every moment in Tetrimone feels alive with motion, from the graceful fall of each piece to the spectacular celebration of every line clear. This isn't just a puzzle game—it's a **visual symphony of falling blocks**! 🎵🧩

*Warning: Side effects may include uncontrollable urges to clear more lines just to see the animations again!* 😄

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
sudo apt-get install libgtk-3-dev libsdl2-dev libsdl2-mixer-dev libpulse-dev libzip-dev gcc g++ make fluidsynth ffmpeg

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
sudo dnf install gtk3-devel SDL2-devel SDL2_mixer-devel pulseaudio-libs-devel libzip-devel gcc-c++ fluidsynth ffmpeg

# For Windows Cross-Compile (only tested on Fedora)
dnf install ffmpeg mingw64-gcc mingw64-gcc-c++ mingw64-gtk3 mingw64-gtkmm30 wine wine-devel wixl binutils make zip unzip libzip-devel mingw64-libzip mingw64-SDL2 mingw64-SDL2_mixer fluidsynth

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

### wxWidgets Build (recommended for Windows)

`Makefile.wx` builds the same game with a wxWidgets frontend: native Win32 menus and dialogs on Windows, GTK on Linux, with the shared Cairo renderer so the board looks identical. It has the full GTK3 feature set and needs no GTK runtime on Windows.

```bash
# Linux (Debian/Ubuntu)
sudo apt-get install libwxgtk3.2-dev libcairo2-dev libsdl2-dev libsdl2-mixer-dev libzip-dev zip fluidsynth fluid-soundfont-gm ffmpeg
make -f Makefile.wx            # -> build/linux_wx/tetrimone

# Windows, inside an MSYS2 MINGW64 (or UCRT64) shell
pacman -S --needed make zip $MINGW_PACKAGE_PREFIX-{gcc,pkgconf,wxwidgets3.2-msw,cairo,SDL2,SDL2_mixer,libzip,fluidsynth,ffmpeg}
make -f Makefile.wx            # -> build/windows_wx/tetrimone.exe (+ DLLs copied next to it)

# Windows, cross-compiled from Linux (needs a MinGW build of wxWidgets 3.2)
make -f Makefile.wx windows WX_CONFIG_WIN=/path/to/mingw/bin/wx-config
```

`make -f Makefile.wx debug` builds with symbols (and a console window on Windows for the debug output). The build also renders the music with FluidSynth and FFmpeg into `sound.zip` next to the executable (pass `SOUNDFONT=/path/to/file.sf2` if no General MIDI SoundFont is found). The window can be resized or maximized and the board scales with it; F11 toggles full screen.

## Scoring: The Tetrimone Triumph Scale

- **1 line**: 40 × level (Appetizer)
- **2 lines**: 100 × level (Light Meal)
- **3 lines**: 300 × level (Feast)
- **4 lines**: 1200 × level (TETRIMONE GODMODE!)

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
- **Support**: Like Tetrimone or any of my projects? [Buy me a Coffee! ☕](https://buymeacoffee.com/jasonbrianhall)

**May your blocks always fall true, and your lines always clear!** 🕹️✨

## TODO

* Thought about multiplayer support but didn't really architect it for MP support.  Would require a great deal of reengineering.

## 🥚 Секретный Пасхальный Режим (Secret Soviet Easter Egg) 🚩

### 🕹️ Инструкция По Активации Специального Режима 🕹️

Внимание, товарищ! Вы обнаружили ВЫСОКОСЕКРЕТНУЮ кнопку невероятной силы! 🕵️‍♀️

**Инструкция**: Нажмите клавишу "." (точка) и WITNESS THE TRANSFORMATION! 

Что происходит? 
- 🎨 Цветовая тема превращается в СОВЕТСКИЙ РЕТРО-СТИЛЬ
- 🎵 Музыка замолкает (потому что в Советском Союзе музыка - это буржуазная роскошь!)
- 📝 Интерфейс становится ИДЕОЛОГИЧЕСКИ ЧИСТЫМ
- 🌍 Игра превращается в инструмент ГОСУДАРСТВЕННОЙ ВАЖНОСТИ

**Предупреждение**: Злоупотребление пасхальным яйцом может привести к немедленной переквалификации в лагерь переподготовки разработчиков! 😂

### Translation for Capitalist Spies 🕵️

In English: Press the period key to activate an EPIC retro Soviet mode that transforms the entire game aesthetic! Complete with Russian text, Soviet color scheme, and a dash of communist humor. Your block-dropping experience will never be the same! 

**Pro Tip**: Press the period key again to return to the decadent Western interface. 🇺🇸➡️🇷🇺➡️🇺🇸

Remember: In Tetrimone, as in life, the Party is always right! 🚩✊

## Freedom Mode

Press comma (,) to activate patriotic American mode with rock music, freedom messaging, and red-white-blue aesthetics!

## 🎮 Other Projects by Jason Brian Hall

Bored? Let me rescue you from the depths of monotony with these digital delights! 🚀

🎶 **Zenamp**: [Zenamp - The Ultimate Audio Player](https://github.com/jasonbrianhall/zenamp) - A feature-rich, cross-platform audio player combining modern format support with nostalgic MIDI synthesis, comprehensive karaoke capabilities, and 30+ mesmerizing visualizations. Experience authentic OPL3 FM synthesis from the golden age of PC gaming!

💣 **Minesweeper Madness**: [Minesweeper](https://github.com/jasonbrianhall/minesweeper) - Not just a game, it's a digital minefield of excitement! (It's actually a really good version, pinky promise! 🤞)

🧩 **Sudoku Solver Spectacular**: [Sudoku Solver](https://github.com/jasonbrianhall/sudoku_solver) - A Sudoku Swiss Army Knife! 🚀 This project is way more than just solving puzzles. Dive into a world where:
- 🧠 Puzzle Generation: Create brain-twisting Sudoku challenges
- 📄 MS-Word Magic: Generate professional puzzle documents
- 🚀 Extreme Solver: Crack instantaneously the most mind-bending Sudoku puzzles
- 🎮 Bonus Game Mode: Check out the playable version hidden in python_generated_puzzles

Numbers have never been this exciting! Prepare for a Sudoku adventure that'll make your brain cells do a happy dance! 🕺

🧊 **Rubik's Cube Chaos**: [Rubik's Cube Solver](https://github.com/jasonbrianhall/rubikscube/) - Crack the code of the most mind-bending 3x3 puzzle known to humanity! Solving optional, frustration guaranteed! 😅

🐛 **Willy the Worm's Wild Ride**: [Willy the worm](https://github.com/jasonbrianhall/willytheworm) - A 2D side-scroller starring the most adventurous invertebrate in gaming history! Who said worms can't be heroes? 🦸‍♂️

🧙‍♂️ **The Wizard's Castle: Choose Your Own Adventure**: [The Wizard's Castle](https://github.com/jasonbrianhall/wizardscastle) - A Text-Based RPG that works on QT5, CLI, and even Android! Magic knows no boundaries! ✨

🔤 **Hangman**: [Hangman](https://github.com/jasonbrianhall/hangman) - Word-guessing game in your terminal! Prepare for linguistic warfare! 💬

🃏 **Card Games Collection**: [Solitaire, FreeCell & Spider](https://github.com/jasonbrianhall/cardgames) - The most meticulously crafted card games with custom decks, animations, and more features than you can shuffle!

📺 **Super Mario Brothers**: [Super Mario Brothers](https://github.com/jasonbrianhall/super_mario_brothers/) - A 6502 to C++ conversion of Super Mario Brothers.  ROM not included.

🚀 **WarpNES**: [WarpNES](https://github.com/jasonbrianhall/warpnes/) - A 6502 emulator that plays some NES games with increasing compatibility over time.
