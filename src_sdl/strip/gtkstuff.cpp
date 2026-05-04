#include <cstring>
#include <iostream>
#include "tetrimone.h"
#include "commandline.h"


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
