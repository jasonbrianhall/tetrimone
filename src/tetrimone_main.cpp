#include <cstring>
#include <iostream>
#include "tetrimone.h"
#include "commandline.h"

void printHelp(const char* programName) {
    std::cout << "Tetrimone - A block falling puzzle game\n\n";
    std::cout << "Usage: " << programName << " [OPTIONS]\n\n";
    std::cout << "Game Options:\n";
    std::cout << "  -d, --difficulty LEVEL     Set difficulty (0=Zen, 1=Easy, 2=Medium, 3=Hard, 4=Extreme, 5=Insane)\n";
    std::cout << "  -l, --level LEVEL          Set initial level (1-99)\n";
    std::cout << "  --min-block-size SIZE      Set minimum block size (1-4)\n";
    std::cout << "  --junk-lines PERCENT       Set junk lines percentage (0-50)\n";
    std::cout << "  --junk-per-level LINES     Set junk lines added per level (0-5)\n\n";
    
    std::cout << "Display Options:\n";
    std::cout << "  -b, --block-size SIZE      Set block size in pixels (20-80)\n";
    std::cout << "  -w, --width WIDTH          Set grid width (8-16)\n";
    std::cout << "  -h, --height HEIGHT        Set grid height (16-30)\n";
    std::cout << "  -t, --theme INDEX          Set color theme (0-30)\n";
    std::cout << "  -f, --fullscreen           Start in fullscreen mode\n";
    std::cout << "  --background IMAGE         Set background image file\n";
    std::cout << "  --background-zip ZIP       Set background images ZIP file\n";
    std::cout << "  --background-opacity VAL   Set background opacity (0.0-1.0)\n";
    std::cout << "  --grid-lines               Show grid lines\n";
    std::cout << "  --simple-blocks            Use simple blocks (no 3D effect)\n";
    std::cout << "  --no-ghost                 Disable ghost piece\n\n";
    
    std::cout << "Audio Options:\n";
    std::cout << "  --no-sound                 Disable all sound effects\n";
    std::cout << "  --no-music                 Disable background music\n";
    std::cout << "  --retro-music              Use retro music tracks\n";
    std::cout << "  --sound-zip ZIP            Set sound effects ZIP file\n\n";
    
    std::cout << "Special Modes:\n";
    std::cout << "  --retro                    Enable Soviet retro mode\n\n";
    
    std::cout << "Information:\n";
    std::cout << "  --help                     Show this help message\n";
    std::cout << "  --version                  Show version information\n\n";
    
    std::cout << "Examples:\n";
    std::cout << "  " << programName << " --difficulty 3 --level 5\n";
    std::cout << "  " << programName << " --retro --block-size 40\n";
    std::cout << "  " << programName << " --width 12 --height 20 --theme 15\n";
    std::cout << "  " << programName << " --background-zip backgrounds.zip --no-music\n";
}

void printVersion() {
    std::cout << "Tetrimone\n";
    std::cout << "A modern block falling puzzle game with extended features\n";
    std::cout << "Built with GTK+ and SDL2\n";
}

ArgType getArgType(const std::string& arg) {
    if (arg == "--help" || arg == "-help") return ArgType::HELP;
    if (arg == "--version") return ArgType::VERSION;
    if (arg == "-d") return ArgType::DIFFICULTY_SHORT;
    if (arg == "--difficulty") return ArgType::DIFFICULTY_LONG;
    if (arg == "-l") return ArgType::LEVEL_SHORT;
    if (arg == "--level") return ArgType::LEVEL_LONG;
    if (arg == "-b") return ArgType::BLOCK_SIZE_SHORT;
    if (arg == "--block-size") return ArgType::BLOCK_SIZE_LONG;
    if (arg == "-w") return ArgType::WIDTH_SHORT;
    if (arg == "--width") return ArgType::WIDTH_LONG;
    if (arg == "-h") return ArgType::HEIGHT_SHORT;
    if (arg == "--height") return ArgType::HEIGHT_LONG;
    if (arg == "-t") return ArgType::THEME_SHORT;
    if (arg == "--theme") return ArgType::THEME_LONG;
    if (arg == "--min-block-size") return ArgType::MIN_BLOCK_SIZE;
    if (arg == "--junk-lines") return ArgType::JUNK_LINES;
    if (arg == "--junk-per-level") return ArgType::JUNK_PER_LEVEL;
    if (arg == "--background") return ArgType::BACKGROUND;
    if (arg == "--background-zip") return ArgType::BACKGROUND_ZIP;
    if (arg == "--background-opacity") return ArgType::BACKGROUND_OPACITY;
    if (arg == "--sound-zip") return ArgType::SOUND_ZIP;
    if (arg == "-f") return ArgType::FULLSCREEN_SHORT;
    if (arg == "--fullscreen") return ArgType::FULLSCREEN_LONG;
    if (arg == "--no-sound") return ArgType::NO_SOUND;
    if (arg == "--no-music") return ArgType::NO_MUSIC;
    if (arg == "--no-ghost") return ArgType::NO_GHOST;
    if (arg == "--grid-lines") return ArgType::GRID_LINES;
    if (arg == "--retro") {printf("Retro\n"); return ArgType::RETRO;}
    if (arg == "--simple-blocks") return ArgType::SIMPLE_BLOCKS;
    if (arg == "--retro-music") return ArgType::RETRO_MUSIC;
    return ArgType::UNKNOWN;
}

CommandLineArgs parseCommandLine(int argc, char* argv[]) {
    CommandLineArgs args;
    
    for (int i = 1; i < argc; i++) {
        std::string arg = argv[i];
        ArgType argType = getArgType(arg);
        
        printf("DEBUG: arg='%s', argType=%d\n", arg.c_str(), (int)argType);  // Add this line
        
        switch (argType) {
            case ArgType::HELP:
                args.help = true;
                break;
                
            case ArgType::VERSION:
                args.version = true;
                break;
                
            case ArgType::DIFFICULTY_SHORT:
            case ArgType::DIFFICULTY_LONG:
                if (i + 1 < argc) {
                    args.difficulty = std::atoi(argv[++i]);
                    if (args.difficulty < 0 || args.difficulty > 5) {
                        std::cerr << "Error: Difficulty must be between 0-5\n";
                        args.difficulty = 2; // Default to medium
                    }
                } else {
                    std::cerr << "Error: --difficulty requires a value\n";
                }
                break;
                
            case ArgType::LEVEL_SHORT:
            case ArgType::LEVEL_LONG:
                if (i + 1 < argc) {
                    args.initialLevel = std::atoi(argv[++i]);
                    if (args.initialLevel < 1 || args.initialLevel > 99) {
                        std::cerr << "Error: Level must be between 1-99\n";
                        args.initialLevel = 1;
                    }
                } else {
                    std::cerr << "Error: --level requires a value\n";
                }
                break;
                
            case ArgType::BLOCK_SIZE_SHORT:
            case ArgType::BLOCK_SIZE_LONG:
                if (i + 1 < argc) {
                    args.blockSize = std::atoi(argv[++i]);
                    if (args.blockSize < MIN_BLOCK_SIZE || args.blockSize > MAX_BLOCK_SIZE) {
                        std::cerr << "Error: Block size must be between " << MIN_BLOCK_SIZE 
                                  << "-" << MAX_BLOCK_SIZE << "\n";
                        args.blockSize = -1;
                    }
                } else {
                    std::cerr << "Error: --block-size requires a value\n";
                }
                break;
                
            case ArgType::WIDTH_SHORT:
            case ArgType::WIDTH_LONG:
                if (i + 1 < argc) {
                    args.gridWidth = std::atoi(argv[++i]);
                    if (args.gridWidth < MIN_GRID_WIDTH || args.gridWidth > MAX_GRID_WIDTH) {
                        std::cerr << "Error: Grid width must be between " << MIN_GRID_WIDTH 
                                  << "-" << MAX_GRID_WIDTH << "\n";
                        args.gridWidth = -1;
                    }
                } else {
                    std::cerr << "Error: --width requires a value\n";
                }
                break;
                
            case ArgType::HEIGHT_SHORT:
            case ArgType::HEIGHT_LONG:
                if (i + 1 < argc) {
                    args.gridHeight = std::atoi(argv[++i]);
                    if (args.gridHeight < MIN_GRID_HEIGHT || args.gridHeight > MAX_GRID_HEIGHT) {
                        std::cerr << "Error: Grid height must be between " << MIN_GRID_HEIGHT 
                                  << "-" << MAX_GRID_HEIGHT << "\n";
                        args.gridHeight = -1;
                    }
                } else {
                    std::cerr << "Error: --height requires a value\n";
                }
                break;
                
            case ArgType::THEME_SHORT:
            case ArgType::THEME_LONG:
                if (i + 1 < argc) {
                    args.themeIndex = std::atoi(argv[++i]);
                    if (args.themeIndex < 0 || args.themeIndex >= NUM_COLOR_THEMES) {
                        std::cerr << "Error: Theme index must be between 0-" << (NUM_COLOR_THEMES-1) << "\n";
                        args.themeIndex = -1;
                    }
                } else {
                    std::cerr << "Error: --theme requires a value\n";
                }
                break;
                
            case ArgType::MIN_BLOCK_SIZE:
                if (i + 1 < argc) {
                    args.minBlockSize = std::atoi(argv[++i]);
                    if (args.minBlockSize < 1 || args.minBlockSize > 4) {
                        std::cerr << "Error: Minimum block size must be between 1-4\n";
                        args.minBlockSize = -1;
                    }
                } else {
                    std::cerr << "Error: --min-block-size requires a value\n";
                }
                break;
                
            case ArgType::JUNK_LINES:
                if (i + 1 < argc) {
                    args.junkLinesPercentage = std::atoi(argv[++i]);
                    if (args.junkLinesPercentage < 0 || args.junkLinesPercentage > 50) {
                        std::cerr << "Error: Junk lines percentage must be between 0-50\n";
                        args.junkLinesPercentage = -1;
                    }
                } else {
                    std::cerr << "Error: --junk-lines requires a value\n";
                }
                break;
                
            case ArgType::JUNK_PER_LEVEL:
                if (i + 1 < argc) {
                    args.junkLinesPerLevel = std::atoi(argv[++i]);
                    if (args.junkLinesPerLevel < 0 || args.junkLinesPerLevel > 5) {
                        std::cerr << "Error: Junk lines per level must be between 0-5\n";
                        args.junkLinesPerLevel = -1;
                    }
                } else {
                    std::cerr << "Error: --junk-per-level requires a value\n";
                }
                break;
                
            case ArgType::BACKGROUND:
                if (i + 1 < argc) {
                    args.backgroundImage = argv[++i];
                } else {
                    std::cerr << "Error: --background requires a file path\n";
                }
                break;
                
            case ArgType::BACKGROUND_ZIP:
                if (i + 1 < argc) {
                    args.backgroundZip = argv[++i];
                } else {
                    std::cerr << "Error: --background-zip requires a file path\n";
                }
                break;
                
            case ArgType::BACKGROUND_OPACITY:
                if (i + 1 < argc) {
                    args.backgroundOpacity = std::atof(argv[++i]);
                    if (args.backgroundOpacity < 0.0 || args.backgroundOpacity > 1.0) {
                        std::cerr << "Error: Background opacity must be between 0.0-1.0\n";
                        args.backgroundOpacity = -1.0;
                    }
                } else {
                    std::cerr << "Error: --background-opacity requires a value\n";
                }
                break;
                
            case ArgType::SOUND_ZIP:
                if (i + 1 < argc) {
                    args.soundZip = argv[++i];
                } else {
                    std::cerr << "Error: --sound-zip requires a file path\n";
                }
                break;
                
            case ArgType::FULLSCREEN_SHORT:
            case ArgType::FULLSCREEN_LONG:
                args.fullscreen = true;
                break;
                
            case ArgType::NO_SOUND:
                args.soundEnabled = false;
                break;
                
            case ArgType::NO_MUSIC:
                args.musicEnabled = false;
                break;
                
            case ArgType::NO_GHOST:
                args.ghostPiece = false;
                break;
                
            case ArgType::GRID_LINES:
                args.gridLines = true;
                break;
                
            case ArgType::RETRO:
                args.retroMode = true;
                break;
                
            case ArgType::SIMPLE_BLOCKS:
                args.simpleBlocks = true;
                break;
                
            case ArgType::RETRO_MUSIC:
                args.retroMusic = true;
                break;
                
    case ArgType::UNKNOWN:
    default:
        printf("DEBUG: Hit UNKNOWN/default case, argType=%d\n", (int)argType);
        std::cerr << "Warning: Unknown option '" << arg << "'\n";
        break;
        }
    }
    
    return args;
}

void applyCommandLineArgs(TetrimoneApp* app, const CommandLineArgs& args) {
    printf("DEBUG: Applying command line args...\n");
    printf("DEBUG: retroMode=%d, initialLevel=%d, minBlockSize=%d\n", 
           args.retroMode, args.initialLevel, args.minBlockSize);
    printf("DEBUG: difficulty=%d, blockSize=%d, gridWidth=%d, gridHeight=%d\n",
           args.difficulty, args.blockSize, args.gridWidth, args.gridHeight);
    printf("DEBUG: themeIndex=%d, soundEnabled=%d, ghostPiece=%d\n",
           args.themeIndex, args.soundEnabled, args.ghostPiece);
    
    // Apply grid dimensions first (before calculating block size)
    if (args.gridWidth != -1) {
        printf("DEBUG: Setting grid width to %d\n", args.gridWidth);
        GRID_WIDTH = args.gridWidth;
    }
    if (args.gridHeight != -1) {
        printf("DEBUG: Setting grid height to %d\n", args.gridHeight);
        GRID_HEIGHT = args.gridHeight;
    }
    
    // Apply block size (after grid dimensions are set)
    if (args.blockSize != -1) {
        printf("DEBUG: Setting block size to %d\n", args.blockSize);
        BLOCK_SIZE = args.blockSize;
    }
    
    // Apply difficulty
    if (args.difficulty != -1) {
        printf("DEBUG: Setting difficulty to %d\n", args.difficulty);
        app->difficulty = args.difficulty;
        // Update menu selection
        switch (args.difficulty) {
            case 0: 
                printf("DEBUG: Setting Zen difficulty\n");
                gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(app->zenMenuItem), TRUE); 
                break;
            case 1: 
                printf("DEBUG: Setting Easy difficulty\n");
                gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(app->easyMenuItem), TRUE); 
                break;
            case 2: 
                printf("DEBUG: Setting Medium difficulty\n");
                gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(app->mediumMenuItem), TRUE); 
                break;
            case 3: 
                printf("DEBUG: Setting Hard difficulty\n");
                gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(app->hardMenuItem), TRUE); 
                break;
            case 4: 
                printf("DEBUG: Setting Extreme difficulty\n");
                gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(app->extremeMenuItem), TRUE); 
                break;
            case 5: 
                printf("DEBUG: Setting Insane difficulty\n");
                gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(app->insaneMenuItem), TRUE); 
                break;
        }
    }
    
    // Apply theme
    if (args.themeIndex != -1) {
        printf("DEBUG: Setting theme index to %d\n", args.themeIndex);
        currentThemeIndex = args.themeIndex;
        gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(app->themeMenuItems[args.themeIndex]), TRUE);
    }
    
    // Apply board settings
    if (args.minBlockSize != -1) {
        printf("DEBUG: Setting min block size to %d\n", args.minBlockSize);
        app->board->setMinBlockSize(args.minBlockSize);
    }
    
if (args.initialLevel != -1) {
    printf("DEBUG: Setting initial level to %d\n", args.initialLevel);
    app->board->initialLevel = args.initialLevel;
    app->board->setLevel(args.initialLevel); 
}

    
    if (args.junkLinesPercentage != -1) {
        printf("DEBUG: Setting junk lines percentage to %d\n", args.junkLinesPercentage);
        app->board->junkLinesPercentage = args.junkLinesPercentage;
    }
    
    if (args.junkLinesPerLevel != -1) {
        printf("DEBUG: Setting junk lines per level to %d\n", args.junkLinesPerLevel);
        app->board->junkLinesPerLevel = args.junkLinesPerLevel;
    }
    
    // Apply display settings
    printf("DEBUG: Setting ghost piece to %d\n", args.ghostPiece);
    app->board->setGhostPieceEnabled(args.ghostPiece);
    
    printf("DEBUG: Setting grid lines to %d\n", args.gridLines);
    app->board->setShowGridLines(args.gridLines);
    
    printf("DEBUG: Setting simple blocks to %d\n", args.simpleBlocks);
    app->board->simpleBlocksActive = args.simpleBlocks;
    
    // Apply audio settings
    printf("DEBUG: Setting sound enabled to %d\n", args.soundEnabled);
    app->board->sound_enabled_ = args.soundEnabled;
    
    printf("DEBUG: Setting retro music to %d\n", args.retroMusic);
    app->board->retroMusicActive = args.retroMusic;
    
    // Update menu items to reflect settings
    gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(app->soundToggleMenuItem), args.soundEnabled);
    
    // Apply retro mode
    if (args.retroMode) {
        printf("DEBUG: Applying retro mode\n");
        app->board->retroModeActive = true;
        gtk_window_set_title(GTK_WINDOW(app->window), "БЛОЧНАЯ РЕВОЛЮЦИЯ");
        currentThemeIndex = NUM_COLOR_THEMES - 1; // Soviet Retro theme
        gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(app->themeMenuItems[currentThemeIndex]), TRUE);
        
        // In retro mode, disable backgrounds
        printf("DEBUG: Disabling backgrounds for retro mode\n");
        app->board->setUseBackgroundImage(false);
        app->board->setUseBackgroundZip(false);
        gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(app->backgroundToggleMenuItem), FALSE);
    }
    
    // Apply background settings
    if (!args.backgroundImage.empty()) {
        printf("DEBUG: Loading background image: %s\n", args.backgroundImage.c_str());
        if (app->board->loadBackgroundImage(args.backgroundImage)) {
            printf("DEBUG: Successfully loaded background image\n");
            app->board->setUseBackgroundImage(true);
            gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(app->backgroundToggleMenuItem), TRUE);
        } else {
            printf("DEBUG: Failed to load background image\n");
        }
    }
    
    if (!args.backgroundZip.empty()) {
        printf("DEBUG: Loading background ZIP: %s\n", args.backgroundZip.c_str());
        if (app->board->loadBackgroundImagesFromZip(args.backgroundZip)) {
            printf("DEBUG: Successfully loaded background ZIP\n");
            app->board->setUseBackgroundZip(true);
            gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(app->backgroundToggleMenuItem), TRUE);
        } else {
            printf("DEBUG: Failed to load background ZIP\n");
        }
    }
    
    if (args.backgroundOpacity != -1.0) {
        printf("DEBUG: Setting background opacity to %f\n", args.backgroundOpacity);
        app->board->setBackgroundOpacity(args.backgroundOpacity);
    }
    
    if (!args.soundZip.empty()) {
        printf("DEBUG: Setting sound ZIP path: %s\n", args.soundZip.c_str());
        app->board->setSoundsZipPath(args.soundZip);
    }
    
    // Apply fullscreen
    if (args.fullscreen) {
        printf("DEBUG: Setting fullscreen mode\n");
        gtk_window_fullscreen(GTK_WINDOW(app->window));
    }
    
    // Update labels to reflect new settings
    printf("DEBUG: Updating difficulty label\n");
    gtk_label_set_markup(GTK_LABEL(app->difficultyLabel),
                         app->board->getDifficultyText(app->difficulty).c_str());
                         
    printf("DEBUG: Command line args application complete\n");
}

int main(int argc, char *argv[]) {
    // Parse our arguments first and create filtered argc/argv for GTK
    CommandLineArgs args = parseCommandLine(argc, argv);
    
    // Handle help and version before GTK initialization
    if (args.help) {
        printHelp(argv[0]);
        return 0;
    }
    
    if (args.version) {
        printVersion();
        return 0;
    }
    
    // Create filtered argv with only GTK-compatible arguments
    std::vector<char*> gtkArgv;
    gtkArgv.push_back(argv[0]); // Always keep program name
    
    // Add any GTK-specific arguments you want to preserve
    // For now, just keep the program name
    int gtkArgc = gtkArgv.size();
    
#ifdef DEBUG
    freopen("debug_output.log", "w", stdout);
    freopen("debug_output.log", "a", stderr);
#endif

    GtkApplication *app = gtk_application_new("org.gtk.tetrimone", G_APPLICATION_DEFAULT_FLAGS);
    
    // Store command line args in the application data
    g_object_set_data(G_OBJECT(app), "cmdline-args", &args);
    
    g_signal_connect(app, "activate", G_CALLBACK(onAppActivate), NULL);
    
    // Pass filtered arguments to GTK
    int status = g_application_run(G_APPLICATION(app), gtkArgc, gtkArgv.data());
    g_object_unref(app);
    return status;
}
