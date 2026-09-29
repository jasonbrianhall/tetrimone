# CMake build for the wxWidgets frontend of Tetrimone.
#
# Primarily for MSVC on Windows (dependencies from vcpkg, see vcpkg.json):
#
#   cmake -S . -B build-msvc -A x64 ^
#         -DCMAKE_TOOLCHAIN_FILE=%VCPKG_ROOT%\scripts\buildsystems\vcpkg.cmake
#   cmake --build build-msvc --config Release
#
# Also works on Linux with system packages (wxGTK, cairo, SDL2, SDL2_mixer, libzip).
# The GTK3 / Qt5 frontends and the MinGW wx build keep using the Makefiles.

cmake_minimum_required(VERSION 3.21)
project(Tetrimone VERSION 1.0 LANGUAGES CXX)
if(WIN32)
    enable_language(RC)
endif()

# C++20 for designated initializers (GCC/Clang accept them in C++17 as an extension, MSVC doesn't)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

option(TETRIMONE_CONSOLE "Build a console-subsystem exe on Windows (shows debug output)" OFF)

# ---------------------------------------------------------------------------
# Dependencies
# ---------------------------------------------------------------------------

# vcpkg ships a wxWidgets CMake config (wx::core / wx::base); distro packages
# usually don't, so fall back to CMake's FindwxWidgets module.
find_package(wxWidgets CONFIG QUIET)
if(TARGET wx::core)
    set(TETRIMONE_WX_LIBS wx::core wx::base)
else()
    find_package(wxWidgets REQUIRED COMPONENTS core base)
    include(${wxWidgets_USE_FILE})
    set(TETRIMONE_WX_LIBS ${wxWidgets_LIBRARIES})
endif()

find_package(SDL2 CONFIG REQUIRED)
find_package(SDL2_mixer CONFIG REQUIRED)
find_package(libzip CONFIG REQUIRED)

# cairo has no CMake config: pkg-config on Linux, plain header/library search on Windows
if(NOT WIN32)
    find_package(PkgConfig REQUIRED)
    pkg_check_modules(CAIRO REQUIRED IMPORTED_TARGET cairo)
    set(TETRIMONE_CAIRO_LIB PkgConfig::CAIRO)
else()
    find_path(CAIRO_INCLUDE_DIR cairo/cairo.h REQUIRED)
    find_library(CAIRO_LIBRARY NAMES cairo cairo-2 REQUIRED)
    add_library(Tetrimone::cairo UNKNOWN IMPORTED)
    set_target_properties(Tetrimone::cairo PROPERTIES
        IMPORTED_LOCATION "${CAIRO_LIBRARY}"
        INTERFACE_INCLUDE_DIRECTORIES "${CAIRO_INCLUDE_DIR}")
    set(TETRIMONE_CAIRO_LIB Tetrimone::cairo)
endif()

# ---------------------------------------------------------------------------
# Sources (same list as Makefile.wx)
# ---------------------------------------------------------------------------

set(TETRIMONE_SOURCES
    src/tetrimone_wx.cpp src/wx_dialog_helpers.cpp src/wx_glib_compat.cpp
    src/tetrimone.cpp src/tetrimone_main.cpp src/drawgame.cpp src/drawgame_cairo.cpp
    src/drawgame_board.cpp src/background.cpp src/heat.cpp src/ghostpiece.cpp
    src/junklines.cpp src/propaganda.cpp src/freedom.cpp src/help.cpp
    src/highscores.cpp src/saveloadsettings.cpp src/icon.cpp src/joystick_core.cpp
    src/audiomanager.cpp src/sound.cpp src/audioconverter.cpp src/dbopl.cpp
    src/dbopl_wrapper.cpp src/instruments.cpp src/midiplayer.cpp src/virtual_mixer.cpp
    src/wav_converter.cpp src/convertmidi.cpp src/sdlaudioplayer.cpp
)
if(WIN32)
    list(APPEND TETRIMONE_SOURCES src/tetrimone_wx.rc)
endif()

add_executable(tetrimone ${TETRIMONE_SOURCES})

target_include_directories(tetrimone PRIVATE src ${CMAKE_CURRENT_SOURCE_DIR})
target_compile_definitions(tetrimone PRIVATE WXWIDGETS USE_SDL SDL_MAIN_HANDLED)

target_link_libraries(tetrimone PRIVATE
    ${TETRIMONE_WX_LIBS}
    ${TETRIMONE_CAIRO_LIB}
    $<IF:$<TARGET_EXISTS:SDL2_mixer::SDL2_mixer>,SDL2_mixer::SDL2_mixer,SDL2_mixer::SDL2_mixer-static>
    $<IF:$<TARGET_EXISTS:SDL2::SDL2>,SDL2::SDL2,SDL2::SDL2-static>
    libzip::zip
)

if(WIN32)
    target_compile_definitions(tetrimone PRIVATE
        WIN32 NOMINMAX _USE_MATH_DEFINES
        _CRT_SECURE_NO_WARNINGS _CRT_NONSTDC_NO_DEPRECATE)
    target_link_libraries(tetrimone PRIVATE winmm)
    # Release: GUI subsystem (no console window). Debug or TETRIMONE_CONSOLE: console,
    # like `make -f Makefile.wx debug`, so the game's stdout/stderr is visible.
    if(NOT TETRIMONE_CONSOLE)
        set_target_properties(tetrimone PROPERTIES WIN32_EXECUTABLE $<NOT:$<CONFIG:Debug>>)
    endif()
else()
    target_compile_definitions(tetrimone PRIVATE LINUX)
    find_package(Threads REQUIRED)
    target_link_libraries(tetrimone PRIVATE Threads::Threads)
endif()

if(MSVC)
    # /utf-8: sources contain UTF-8 emoji in string literals
    target_compile_options(tetrimone PRIVATE /utf-8 /W3 /MP /bigobj)
    # The game uses main() (wxEntry) rather than WinMain, even as a GUI app.
    # wx/msw/wx.rc embeds the common-controls/DPI manifest, so the linker must not add its own.
    target_link_options(tetrimone PRIVATE /ENTRY:mainCRTStartup /MANIFEST:NO)
    set_property(DIRECTORY PROPERTY VS_STARTUP_PROJECT tetrimone)
else()
    target_compile_options(tetrimone PRIVATE -Wall -Wextra)
endif()
