// Colors for each tetromino type (RGB)
const std::vector<std::vector<std::array<double, 3>>> TETROMINO_COLOR_THEMES = {
    // Theme 1: Classic Tetrimone (Levels 1-3)
    {
        {0.0, 0.7, 0.9},  // I - Cyan
        {0.9, 0.9, 0.0},  // O - Yellow
        {0.8, 0.0, 0.8},  // T - Purple
        {0.0, 0.8, 0.0},  // S - Green
        {0.9, 0.0, 0.0},  // Z - Red
        {0.0, 0.0, 0.8},  // J - Blue
        {0.9, 0.5, 0.0}   // L - Orange
    },
    
    // Theme 2: Neon (Levels 4-6)
    {
        {0.0, 1.0, 1.0},  // I - Neon Cyan
        {1.0, 1.0, 0.0},  // O - Neon Yellow
        {1.0, 0.0, 1.0},  // T - Neon Magenta
        {0.0, 1.0, 0.0},  // S - Neon Green
        {1.0, 0.0, 0.0},  // Z - Neon Red
        {0.3, 0.3, 1.0},  // J - Neon Blue
        {1.0, 0.5, 0.0}   // L - Neon Orange
    },
    
    // Theme 3: Pastel (Levels 7-9)
    {
        {0.5, 0.8, 1.0},  // I - Pastel Blue
        {1.0, 1.0, 0.6},  // O - Pastel Yellow
        {0.9, 0.6, 0.9},  // T - Pastel Purple
        {0.6, 0.9, 0.6},  // S - Pastel Green
        {1.0, 0.6, 0.6},  // Z - Pastel Red
        {0.6, 0.6, 1.0},  // J - Pastel Blue
        {1.0, 0.8, 0.6}   // L - Pastel Orange
    },
    
    // Theme 4: Earth Tones (Levels 10-12)
    {
        {0.5, 0.8, 0.8},  // I - Soft Teal
        {0.9, 0.8, 0.4},  // O - Sand
        {0.7, 0.5, 0.3},  // T - Brown
        {0.5, 0.7, 0.3},  // S - Olive Green
        {0.8, 0.5, 0.3},  // Z - Terra Cotta
        {0.3, 0.5, 0.7},  // J - Slate Blue
        {0.6, 0.4, 0.2}   // L - Sienna
    },
    
    // Theme 5: Monochrome Blue (Levels 13-15)
    {
        {0.0, 0.2, 0.4},  // I - Navy
        {0.3, 0.5, 0.7},  // O - Steel Blue
        {0.1, 0.3, 0.5},  // T - Denim
        {0.2, 0.4, 0.6},  // S - Blue Gray
        {0.0, 0.1, 0.3},  // Z - Dark Blue
        {0.4, 0.6, 0.8},  // J - Sky Blue
        {0.5, 0.7, 0.9}   // L - Light Blue
    },
    
    // Theme 6: Monochrome Green (Levels 16-18)
    {
        {0.0, 0.4, 0.2},  // I - Forest Green
        {0.3, 0.7, 0.5},  // O - Mint
        {0.1, 0.5, 0.3},  // T - Emerald
        {0.2, 0.6, 0.4},  // S - Jade
        {0.0, 0.3, 0.1},  // Z - Dark Green
        {0.4, 0.8, 0.6},  // J - Sea Green
        {0.5, 0.9, 0.7}   // L - Light Green
    },
    
    // Theme 7: Sunset (Levels 19-21)
    {
        {0.9, 0.6, 0.1},  // I - Golden
        {0.9, 0.3, 0.1},  // O - Sunset Orange
        {0.8, 0.1, 0.1},  // T - Crimson
        {0.7, 0.2, 0.3},  // S - Ruby
        {0.6, 0.0, 0.1},  // Z - Dark Red
        {0.9, 0.5, 0.3},  // J - Salmon
        {1.0, 0.7, 0.4}   // L - Peach
    },
    
    // Theme 8: Ocean (Levels 22-24)
    {
        {0.0, 0.2, 0.6},  // I - Deep Blue
        {0.1, 0.5, 0.8},  // O - Ocean Blue
        {0.0, 0.3, 0.5},  // T - Navy
        {0.2, 0.6, 0.7},  // S - Turquoise
        {0.1, 0.2, 0.3},  // Z - Dark Blue-Gray
        {0.4, 0.7, 0.8},  // J - Sky Blue
        {0.6, 0.8, 0.9}   // L - Light Blue
    },
    
    // Theme 9: Grayscale (Levels 25-27)
    {
        {0.1, 0.1, 0.1},  // I - Near Black
        {0.9, 0.9, 0.9},  // O - Near White
        {0.3, 0.3, 0.3},  // T - Dark Gray
        {0.7, 0.7, 0.7},  // S - Light Gray
        {0.2, 0.2, 0.2},  // Z - Darker Gray
        {0.5, 0.5, 0.5},  // J - Medium Gray
        {0.8, 0.8, 0.8}   // L - Lighter Gray
    },
    
    // Theme 10: Candy (Levels 28-30)
    {
        {0.9, 0.5, 0.8},  // I - Pink
        {0.9, 0.8, 0.4},  // O - Light Yellow
        {0.7, 0.4, 0.9},  // T - Lavender
        {0.5, 0.9, 0.6},  // S - Mint Green
        {1.0, 0.5, 0.5},  // Z - Salmon
        {0.4, 0.7, 1.0},  // J - Baby Blue
        {1.0, 0.7, 0.3}   // L - Peach
    },
    
    // Theme 11: Neon Dark (Levels 31-33)
    {
        {0.0, 0.9, 0.9},  // I - Neon Cyan
        {0.9, 0.9, 0.0},  // O - Neon Yellow
        {0.9, 0.0, 0.9},  // T - Neon Magenta
        {0.0, 0.9, 0.0},  // S - Neon Green
        {0.9, 0.0, 0.0},  // Z - Neon Red
        {0.0, 0.0, 0.9},  // J - Neon Blue
        {0.9, 0.5, 0.0}   // L - Neon Orange
    },
    
    // Theme 12: Jewel Tones (Levels 34-36)
    {
        {0.0, 0.6, 0.8},  // I - Sapphire
        {0.9, 0.8, 0.0},  // O - Amber
        {0.6, 0.0, 0.6},  // T - Amethyst
        {0.0, 0.6, 0.3},  // S - Emerald
        {0.8, 0.0, 0.2},  // Z - Ruby
        {0.2, 0.2, 0.7},  // J - Lapis
        {0.8, 0.4, 0.0}   // L - Topaz
    },
    
    // Theme 13: Retro Gaming (Levels 37-39)
    {
        {0.0, 0.8, 0.0},  // I - Green Phosphor
        {0.8, 0.8, 0.0},  // O - Amber Phosphor
        {0.0, 0.7, 0.0},  // T - Dark Green Phosphor
        {0.0, 0.9, 0.0},  // S - Light Green Phosphor
        {0.0, 0.6, 0.0},  // Z - Medium Green Phosphor
        {0.0, 0.7, 0.3},  // J - Teal Phosphor
        {0.3, 0.8, 0.0}   // L - Yellow-Green Phosphor
    },
    
    // Theme 14: Autumn (Levels 40-42)
    {
        {0.7, 0.3, 0.1},  // I - Rust
        {0.9, 0.7, 0.2},  // O - Gold
        {0.6, 0.2, 0.0},  // T - Mahogany
        {0.5, 0.6, 0.1},  // S - Olive
        {0.8, 0.3, 0.0},  // Z - Copper
        {0.4, 0.2, 0.1},  // J - Brown
        {0.9, 0.5, 0.0}   // L - Orange
    },
    
    // Theme 15: Winter (Levels 43-45)
    {
        {0.7, 0.9, 1.0},  // I - Ice Blue
        {1.0, 1.0, 1.0},  // O - Snow White
        {0.8, 0.9, 0.9},  // T - Frost
        {0.5, 0.7, 0.8},  // S - Winter Sky
        {0.7, 0.8, 1.0},  // Z - Pale Blue
        {0.3, 0.5, 0.7},  // J - Slate Blue
        {0.9, 0.9, 0.9}   // L - Silver
    },
    
    // Theme 16: Spring (Levels 46-48)
    {
        {0.7, 0.9, 0.7},  // I - Mint Green
        {1.0, 0.9, 0.7},  // O - Peach
        {0.9, 0.7, 0.9},  // T - Lilac
        {0.7, 0.9, 0.5},  // S - Lime Green
        {1.0, 0.7, 0.7},  // Z - Light Pink
        {0.5, 0.7, 0.9},  // J - Sky Blue
        {1.0, 1.0, 0.7}   // L - Cream
    },
    
    // Theme 17: Summer (Levels 49-51)
    {
        {0.0, 0.8, 0.8},  // I - Turquoise
        {1.0, 0.8, 0.0},  // O - Sunshine Yellow
        {0.9, 0.5, 0.7},  // T - Coral
        {0.3, 0.9, 0.3},  // S - Bright Green
        {1.0, 0.3, 0.3},  // Z - Watermelon
        {0.1, 0.6, 0.9},  // J - Azure
        {1.0, 0.6, 0.0}   // L - Orange
    },
    
    // Theme 18: Monochrome Purple (Levels 52-54)
    {
        {0.2, 0.0, 0.4},  // I - Deep Purple
        {0.8, 0.6, 1.0},  // O - Light Lavender
        {0.4, 0.0, 0.6},  // T - Royal Purple
        {0.6, 0.4, 0.8},  // S - Medium Lavender
        {0.3, 0.0, 0.5},  // Z - Dark Purple
        {0.5, 0.2, 0.7},  // J - Violet
        {0.7, 0.5, 0.9}   // L - Light Purple
    },
    
    // Theme 19: Desert (Levels 55-57)
    {
        {0.9, 0.8, 0.6},  // I - Sand
        {0.7, 0.6, 0.4},  // O - Khaki
        {0.8, 0.5, 0.3},  // T - Terra Cotta
        {0.6, 0.5, 0.3},  // S - Olive Brown
        {0.9, 0.6, 0.4},  // Z - Peach
        {0.5, 0.4, 0.3},  // J - Dark Taupe
        {0.8, 0.7, 0.5}   // L - Beige
    },
    
    // Theme 20: Rainbow (Levels 58+)
    {
        {1.0, 0.0, 0.0},  // I - Red
        {1.0, 0.5, 0.0},  // O - Orange
        {1.0, 1.0, 0.0},  // T - Yellow
        {0.0, 1.0, 0.0},  // S - Green
        {0.0, 0.0, 1.0},  // Z - Blue
        {0.3, 0.0, 0.5},  // J - Indigo
        {0.5, 0.0, 1.0}   // L - Violet
    }
};
