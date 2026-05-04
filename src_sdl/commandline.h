struct CommandLineArgs {
    int difficulty = -1;           // -1 means use default
    int blockSize = -1;            // -1 means use default
    int minBlockSize = -1;         // -1 means use default
    int gridWidth = -1;            // -1 means use default
    int gridHeight = -1;           // -1 means use default
    int initialLevel = -1;         // -1 means use default
    int junkLinesPercentage = -1;  // -1 means use default
    int junkLinesPerLevel = -1;    // -1 means use default
    int themeIndex = -1;           // -1 means use default
    bool soundEnabled = true;      // Default enabled
    bool musicEnabled = true;      // Default enabled
    bool ghostPiece = true;        // Default enabled
    bool gridLines = false;        // Default disabled
    bool retroMode = false;        // Default disabled
    bool simpleBlocks = false;     // Default disabled
    bool retroMusic = false;       // Default disabled
    bool fullscreen = false;       // Default windowed
    bool help = false;             // Show help
    bool version = false;          // Show version
    std::string backgroundImage;   // Path to background image
    std::string backgroundZip;     // Path to background ZIP
    std::string soundZip;          // Path to sound ZIP
    double backgroundOpacity = -1.0; // -1 means use default
};

enum class ArgType {
    HELP,
    VERSION,
    DIFFICULTY_SHORT,
    DIFFICULTY_LONG,
    LEVEL_SHORT,
    LEVEL_LONG,
    BLOCK_SIZE_SHORT,
    BLOCK_SIZE_LONG,
    WIDTH_SHORT,
    WIDTH_LONG,
    HEIGHT_SHORT,
    HEIGHT_LONG,
    THEME_SHORT,
    THEME_LONG,
    MIN_BLOCK_SIZE,
    JUNK_LINES,
    JUNK_PER_LEVEL,
    BACKGROUND,
    BACKGROUND_ZIP,
    BACKGROUND_OPACITY,
    SOUND_ZIP,
    FULLSCREEN_SHORT,
    FULLSCREEN_LONG,
    NO_SOUND,
    NO_MUSIC,
    NO_GHOST,
    GRID_LINES,
    RETRO,
    SIMPLE_BLOCKS,
    RETRO_MUSIC,
    UNKNOWN
};
ArgType getArgType(const std::string& arg);
void printHelp(const char* programName);
void printVersion();
void applyCommandLineArgs(TetrimoneApp* app, const CommandLineArgs& args) ;
CommandLineArgs parseCommandLine(int argc, char* argv[]);
