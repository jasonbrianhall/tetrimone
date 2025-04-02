// Colors for each tetrimoneblock type (RGB)
const std::vector<std::vector<std::array<double, 3>>> TETRIMONEBLOCK_COLOR_THEMES = {
    // Theme 1: Watercolor
    {
        {0.6, 0.8, 0.9},  // I - Soft Blue Wash
        {0.9, 0.7, 0.5},  // O - Peach Blush
        {0.7, 0.5, 0.8},  // T - Lavender Bloom
        {0.5, 0.8, 0.6},  // S - Sage Watercolor
        {0.9, 0.6, 0.6},  // Z - Soft Coral
        {0.5, 0.6, 0.9},  // J - Sky Blue Wash
        {0.8, 0.7, 0.5},  // L - Sand Tone
        // Additional colors for smaller pieces
        {0.6, 0.9, 0.7},  // Straight Triomino - Mint Wash
        {0.8, 0.6, 0.7},  // L Triomino - Rose Petal
        {0.5, 0.7, 0.8},  // Reverse L Triomino - Ocean Mist
        {0.7, 0.8, 0.6},  // V Triomino - Soft Sage
        {0.7, 0.6, 0.8},  // H/V Domino - Lilac Wash
        {0.6, 0.7, 0.9},  // Diagonal Domino - Cloud Blue
        {0.9, 0.8, 0.7}   // Monomino - Cream Wash
    },    
    // Theme 2: Neon 
    {
        {0.0, 1.0, 1.0},  // I - Neon Cyan
        {1.0, 1.0, 0.0},  // O - Neon Yellow
        {1.0, 0.0, 1.0},  // T - Neon Magenta
        {0.0, 1.0, 0.0},  // S - Neon Green
        {1.0, 0.0, 0.0},  // Z - Neon Red
        {0.3, 0.3, 1.0},  // J - Neon Blue
        {1.0, 0.5, 0.0},  // L - Neon Orange
        // Additional colors for smaller pieces
        {0.0, 0.7, 0.7},  // Straight Triomino - Medium Cyan
        {0.7, 0.0, 0.7},  // L Triomino - Medium Magenta
        {0.0, 0.7, 0.0},  // Reverse L Triomino - Medium Green
        {0.7, 0.7, 0.0},  // V Triomino - Medium Yellow
        {0.5, 0.0, 0.7},  // H/V Domino - Purple Blue
        {0.7, 0.3, 0.0},  // Diagonal Domino - Brown
        {0.7, 0.7, 0.7}   // Monomino - Light Gray
    },
    
    // Theme 3: Pastel
    {
        {0.5, 0.8, 1.0},  // I - Pastel Blue
        {1.0, 1.0, 0.6},  // O - Pastel Yellow
        {0.9, 0.6, 0.9},  // T - Pastel Purple
        {0.6, 0.9, 0.6},  // S - Pastel Green
        {1.0, 0.6, 0.6},  // Z - Pastel Red
        {0.6, 0.6, 1.0},  // J - Pastel Blue
        {1.0, 0.8, 0.6},  // L - Pastel Orange
        // Additional colors for smaller pieces
        {0.6, 0.9, 0.9},  // Straight Triomino - Pastel Teal
        {0.9, 0.6, 0.7},  // L Triomino - Pastel Pink
        {0.7, 0.9, 0.7},  // Reverse L Triomino - Pastel Mint
        {0.9, 0.8, 0.9},  // V Triomino - Pastel Lavender
        {0.8, 0.8, 0.8},  // H/V Domino - Pastel Gray
        {0.7, 0.7, 0.9},  // Diagonal Domino - Pastel Periwinkle
        {1.0, 0.9, 0.9}   // Monomino - Pastel Rose
    },
    
    // Theme 4: Earth Tones
    {
        {0.5, 0.8, 0.8},  // I - Soft Teal
        {0.9, 0.8, 0.4},  // O - Sand
        {0.7, 0.5, 0.3},  // T - Brown
        {0.5, 0.7, 0.3},  // S - Olive Green
        {0.8, 0.5, 0.3},  // Z - Terra Cotta
        {0.3, 0.5, 0.7},  // J - Slate Blue
        {0.6, 0.4, 0.2},  // L - Sienna
        // Additional colors for smaller pieces
        {0.4, 0.6, 0.6},  // Straight Triomino - Dusty Teal
        {0.6, 0.5, 0.4},  // L Triomino - Taupe
        {0.4, 0.5, 0.4},  // Reverse L Triomino - Sage
        {0.7, 0.6, 0.5},  // V Triomino - Tan
        {0.5, 0.4, 0.3},  // H/V Domino - Coffee
        {0.4, 0.3, 0.2},  // Diagonal Domino - Dark Brown
        {0.8, 0.7, 0.6}   // Monomino - Light Tan
    },
    
    // Theme 5: Monochrome Blue
    {
        {0.0, 0.2, 0.4},  // I - Navy
        {0.3, 0.5, 0.7},  // O - Steel Blue
        {0.1, 0.3, 0.5},  // T - Denim
        {0.2, 0.4, 0.6},  // S - Blue Gray
        {0.0, 0.1, 0.3},  // Z - Dark Blue
        {0.4, 0.6, 0.8},  // J - Sky Blue
        {0.5, 0.7, 0.9},  // L - Light Blue
        // Additional colors for smaller pieces
        {0.2, 0.3, 0.7},  // Straight Triomino - Royal Blue
        {0.1, 0.2, 0.6},  // L Triomino - Cobalt
        {0.3, 0.4, 0.8},  // Reverse L Triomino - Periwinkle
        {0.1, 0.4, 0.7},  // V Triomino - Azure
        {0.0, 0.3, 0.6},  // H/V Domino - Medium Blue
        {0.2, 0.5, 0.8},  // Diagonal Domino - Cornflower Blue
        {0.6, 0.8, 1.0}   // Monomino - Baby Blue
    },
    
    // Theme 6: Monochrome Green
    {
        {0.0, 0.4, 0.2},  // I - Forest Green
        {0.3, 0.7, 0.5},  // O - Mint
        {0.1, 0.5, 0.3},  // T - Emerald
        {0.2, 0.6, 0.4},  // S - Jade
        {0.0, 0.3, 0.1},  // Z - Dark Green
        {0.4, 0.8, 0.6},  // J - Sea Green
        {0.5, 0.9, 0.7},  // L - Light Green
        // Additional colors for smaller pieces
        {0.2, 0.5, 0.2},  // Straight Triomino - Medium Green
        {0.1, 0.4, 0.1},  // L Triomino - Moss Green
        {0.3, 0.6, 0.3},  // Reverse L Triomino - Grass Green
        {0.4, 0.7, 0.4},  // V Triomino - Sage Green
        {0.1, 0.3, 0.2},  // H/V Domino - Pine Green
        {0.3, 0.5, 0.4},  // Diagonal Domino - Muted Green
        {0.7, 1.0, 0.7}   // Monomino - Pale Green
    },
    
    // Theme 7: Sunset
    {
        {0.9, 0.6, 0.1},  // I - Golden
        {0.9, 0.3, 0.1},  // O - Sunset Orange
        {0.8, 0.1, 0.1},  // T - Crimson
        {0.7, 0.2, 0.3},  // S - Ruby
        {0.6, 0.0, 0.1},  // Z - Dark Red
        {0.9, 0.5, 0.3},  // J - Salmon
        {1.0, 0.7, 0.4},  // L - Peach
        // Additional colors for smaller pieces
        {0.8, 0.4, 0.2},  // Straight Triomino - Amber
        {0.7, 0.3, 0.2},  // L Triomino - Rust
        {0.9, 0.4, 0.3},  // Reverse L Triomino - Coral
        {0.6, 0.2, 0.2},  // V Triomino - Burgundy
        {0.8, 0.3, 0.3},  // H/V Domino - Red-Orange
        {0.7, 0.5, 0.2},  // Diagonal Domino - Bronze
        {1.0, 0.8, 0.6}   // Monomino - Light Peach
    },
    
    // Theme 8: Ocean
    {
        {0.0, 0.2, 0.6},  // I - Deep Blue
        {0.1, 0.5, 0.8},  // O - Ocean Blue
        {0.0, 0.3, 0.5},  // T - Navy
        {0.2, 0.6, 0.7},  // S - Turquoise
        {0.1, 0.2, 0.3},  // Z - Dark Blue-Gray
        {0.4, 0.7, 0.8},  // J - Sky Blue
        {0.6, 0.8, 0.9},  // L - Light Blue
        // Additional colors for smaller pieces
        {0.3, 0.5, 0.6},  // Straight Triomino - Slate Blue
        {0.0, 0.4, 0.6},  // L Triomino - Teal
        {0.2, 0.4, 0.5},  // Reverse L Triomino - Steel Blue
        {0.1, 0.6, 0.7},  // V Triomino - Aqua
        {0.3, 0.6, 0.6},  // H/V Domino - Seafoam
        {0.5, 0.7, 0.7},  // Diagonal Domino - Light Teal
        {0.8, 0.9, 1.0}   // Monomino - Ice Blue
    },
    
    // Theme 9: Grayscale
    {
        {0.1, 0.1, 0.1},  // I - Near Black
        {0.9, 0.9, 0.9},  // O - Near White
        {0.3, 0.3, 0.3},  // T - Dark Gray
        {0.7, 0.7, 0.7},  // S - Light Gray
        {0.2, 0.2, 0.2},  // Z - Darker Gray
        {0.5, 0.5, 0.5},  // J - Medium Gray
        {0.8, 0.8, 0.8},  // L - Lighter Gray
        // Additional colors for smaller pieces
        {0.25, 0.25, 0.25}, // Straight Triomino - Dark-Mid Gray
        {0.35, 0.35, 0.35}, // L Triomino - Mid-Dark Gray
        {0.45, 0.45, 0.45}, // Reverse L Triomino - Medium-Dark Gray
        {0.55, 0.55, 0.55}, // V Triomino - Medium-Light Gray
        {0.65, 0.65, 0.65}, // H/V Domino - Light-Mid Gray
        {0.75, 0.75, 0.75}, // Diagonal Domino - Light Gray
        {0.15, 0.15, 0.15}  // Monomino - Very Dark Gray
    },
    
    // Theme 10: Candy
    {
        {0.9, 0.5, 0.8},  // I - Pink
        {0.9, 0.8, 0.4},  // O - Light Yellow
        {0.7, 0.4, 0.9},  // T - Lavender
        {0.5, 0.9, 0.6},  // S - Mint Green
        {1.0, 0.5, 0.5},  // Z - Salmon
        {0.4, 0.7, 1.0},  // J - Baby Blue
        {1.0, 0.7, 0.3},  // L - Peach
        // Additional colors for smaller pieces
        {0.8, 0.3, 0.6},  // Straight Triomino - Bubblegum
        {0.6, 0.8, 0.3},  // L Triomino - Lime Green
        {0.8, 0.6, 0.9},  // Reverse L Triomino - Light Purple
        {0.9, 0.6, 0.6},  // V Triomino - Light Coral
        {0.6, 0.9, 0.9},  // H/V Domino - Powder Blue
        {1.0, 0.8, 0.9},  // Diagonal Domino - Light Pink
        {0.9, 0.9, 0.6}   // Monomino - Pastel Yellow
    },
    
    // Theme 11: Neon Dark
    {
        {0.0, 0.9, 0.9},  // I - Neon Cyan
        {0.9, 0.9, 0.0},  // O - Neon Yellow
        {0.9, 0.0, 0.9},  // T - Neon Magenta
        {0.0, 0.9, 0.0},  // S - Neon Green
        {0.9, 0.0, 0.0},  // Z - Neon Red
        {0.0, 0.0, 0.9},  // J - Neon Blue
        {0.9, 0.5, 0.0},  // L - Neon Orange
        // Additional colors for smaller pieces
        {0.6, 0.0, 0.9},  // Straight Triomino - Neon Purple
        {0.9, 0.4, 0.6},  // L Triomino - Neon Pink
        {0.5, 0.9, 0.9},  // Reverse L Triomino - Light Cyan
        {0.8, 0.8, 0.4},  // V Triomino - Pastel Yellow
        {0.4, 0.9, 0.4},  // H/V Domino - Medium Green
        {0.9, 0.4, 0.0},  // Diagonal Domino - Burnt Orange
        {1.0, 1.0, 1.0}   // Monomino - Pure White
    },
    
    // Theme 12: Jewel Tones
    {
        {0.0, 0.6, 0.8},  // I - Sapphire
        {0.9, 0.8, 0.0},  // O - Amber
        {0.6, 0.0, 0.6},  // T - Amethyst
        {0.0, 0.6, 0.3},  // S - Emerald
        {0.8, 0.0, 0.2},  // Z - Ruby
        {0.2, 0.2, 0.7},  // J - Lapis
        {0.8, 0.4, 0.0},  // L - Topaz
        // Additional colors for smaller pieces
        {0.0, 0.4, 0.4},  // Straight Triomino - Turquoise
        {0.5, 0.0, 0.3},  // L Triomino - Garnet
        {0.3, 0.5, 0.0},  // Reverse L Triomino - Peridot
        {0.7, 0.0, 0.5},  // V Triomino - Pink Sapphire
        {0.4, 0.2, 0.6},  // H/V Domino - Iolite
        {0.6, 0.3, 0.1},  // Diagonal Domino - Citrine
        {0.9, 0.9, 0.9}   // Monomino - Diamond
    },
    
    // Theme 13: Retro Gaming
    {
        {0.0, 0.8, 0.0},  // I - Green Phosphor
        {0.8, 0.8, 0.0},  // O - Amber Phosphor
        {0.0, 0.7, 0.0},  // T - Dark Green Phosphor
        {0.0, 0.9, 0.0},  // S - Light Green Phosphor
        {0.0, 0.6, 0.0},  // Z - Medium Green Phosphor
        {0.0, 0.7, 0.3},  // J - Teal Phosphor
        {0.3, 0.8, 0.0},  // L - Yellow-Green Phosphor
        // Additional colors for smaller pieces
        {0.0, 0.5, 0.0},  // Straight Triomino - Medium-Dark Green
        {0.0, 0.4, 0.0},  // L Triomino - Darker Green
        {0.0, 0.3, 0.0},  // Reverse L Triomino - Dark Green
        {0.2, 0.6, 0.0},  // V Triomino - Yellow-Green
        {0.4, 0.7, 0.0},  // H/V Domino - Lime-Green
        {0.0, 0.7, 0.2},  // Diagonal Domino - Sea Green
        {0.5, 0.9, 0.5}   // Monomino - Pale Green
    },
    
    // Theme 14: Autumn
    {
        {0.7, 0.3, 0.1},  // I - Rust
        {0.9, 0.7, 0.2},  // O - Gold
        {0.6, 0.2, 0.0},  // T - Mahogany
        {0.5, 0.6, 0.1},  // S - Olive
        {0.8, 0.3, 0.0},  // Z - Copper
        {0.4, 0.2, 0.1},  // J - Brown
        {0.9, 0.5, 0.0},  // L - Orange
        // Additional colors for smaller pieces
        {0.7, 0.4, 0.2},  // Straight Triomino - Amber
        {0.6, 0.3, 0.2},  // L Triomino - Chestnut
        {0.5, 0.4, 0.2},  // Reverse L Triomino - Bronze
        {0.8, 0.6, 0.1},  // V Triomino - Honey
        {0.7, 0.5, 0.3},  // H/V Domino - Tan
        {0.5, 0.3, 0.1},  // Diagonal Domino - Cinnamon
        {0.9, 0.8, 0.5}   // Monomino - Wheat
    },
    
    // Theme 15: Winter
    {
        {0.7, 0.9, 1.0},  // I - Ice Blue
        {1.0, 1.0, 1.0},  // O - Snow White
        {0.8, 0.9, 0.9},  // T - Frost
        {0.5, 0.7, 0.8},  // S - Winter Sky
        {0.7, 0.8, 1.0},  // Z - Pale Blue
        {0.3, 0.5, 0.7},  // J - Slate Blue
        {0.9, 0.9, 0.9},  // L - Silver
        // Additional colors for smaller pieces
        {0.6, 0.8, 0.9},  // Straight Triomino - Powder Blue
        {0.4, 0.6, 0.8},  // L Triomino - Steel Blue
        {0.8, 0.8, 0.8},  // Reverse L Triomino - Light Gray
        {0.6, 0.7, 0.9},  // V Triomino - Periwinkle
        {0.9, 0.9, 1.0},  // H/V Domino - Almost White
        {0.5, 0.5, 0.6},  // Diagonal Domino - Slate Gray
        {0.9, 0.95, 1.0}  // Monomino - Snow
    },
    
    // Theme 16: Spring
    {
        {0.7, 0.9, 0.7},  // I - Mint Green
        {1.0, 0.9, 0.7},  // O - Peach
        {0.9, 0.7, 0.9},  // T - Lilac
        {0.7, 0.9, 0.5},  // S - Lime Green
        {1.0, 0.7, 0.7},  // Z - Light Pink
        {0.5, 0.7, 0.9},  // J - Sky Blue
        {1.0, 1.0, 0.7},  // L - Cream
        // Additional colors for smaller pieces
        {0.8, 0.9, 0.6},  // Straight Triomino - Pale Green
        {0.9, 0.8, 0.8},  // L Triomino - Pale Rose
        {0.7, 0.8, 0.9},  // Reverse L Triomino - Light Blue
        {0.9, 0.9, 0.8},  // V Triomino - Pale Yellow
        {0.8, 0.7, 0.8},  // H/V Domino - Lavender
        {0.6, 0.8, 0.8},  // Diagonal Domino - Pastel Teal
        {0.9, 0.8, 0.9}   // Monomino - Pale Lavender
    },
    
    // Theme 17: Summer
    {
        {0.0, 0.8, 0.8},  // I - Turquoise
        {1.0, 0.8, 0.0},  // O - Sunshine Yellow
        {0.9, 0.5, 0.7},  // T - Coral
        {0.3, 0.9, 0.3},  // S - Bright Green
        {1.0, 0.3, 0.3},  // Z - Watermelon
        {0.1, 0.6, 0.9},  // J - Azure
        {1.0, 0.6, 0.0},  // L - Orange
        // Additional colors for smaller pieces
        {0.0, 0.7, 0.3},  // Straight Triomino - Emerald
        {0.1, 0.8, 0.7},  // L Triomino - Aqua
        {0.7, 0.2, 0.5},  // Reverse L Triomino - Magenta
        {0.7, 0.7, 0.0},  // V Triomino - Olive
        {0.9, 0.3, 0.6},  // H/V Domino - Hot Pink
        {0.6, 0.9, 0.5},  // Diagonal Domino - Sea Green
        {0.0, 0.5, 1.0}   // Monomino - Clear Blue
    },
    
    // Theme 18: Monochrome Purple
    {
        {0.2, 0.0, 0.4},  // I - Deep Purple
        {0.8, 0.6, 1.0},  // O - Light Lavender
        {0.4, 0.0, 0.6},  // T - Royal Purple
        {0.6, 0.4, 0.8},  // S - Medium Lavender
        {0.3, 0.0, 0.5},  // Z - Dark Purple
        {0.5, 0.2, 0.7},  // J - Violet
        {0.7, 0.5, 0.9},  // L - Light Purple
        // Additional colors for smaller pieces
        {0.35, 0.1, 0.55}, // Straight Triomino - Plum
        {0.45, 0.2, 0.65}, // L Triomino - Amethyst
        {0.55, 0.3, 0.75}, // Reverse L Triomino - Lilac
        {0.25, 0.05, 0.45}, // V Triomino - Dark Violet
        {0.65, 0.4, 0.85}, // H/V Domino - Wisteria
        {0.4, 0.15, 0.6}, // Diagonal Domino - Indigo
        {0.9, 0.7, 1.0}   // Monomino - Pale Lilac
    },
    
    // Theme 19: Desert
    {
        {0.9, 0.8, 0.6},  // I - Sand
        {0.7, 0.6, 0.4},  // O - Khaki
        {0.8, 0.5, 0.3},  // T - Terra Cotta
        {0.6, 0.5, 0.3},  // S - Olive Brown
        {0.9, 0.6, 0.4},  // Z - Peach
        {0.5, 0.4, 0.3},  // J - Dark Taupe
        {0.8, 0.7, 0.5},  // L - Beige
        // Additional colors for smaller pieces
        {0.7, 0.5, 0.2},  // Straight Triomino - Camel
        {0.6, 0.4, 0.2},  // L Triomino - Brown
        {0.8, 0.6, 0.2},  // Reverse L Triomino - Gold
        {0.7, 0.4, 0.3},  // V Triomino - Rust
        {0.5, 0.3, 0.2},  // H/V Domino - Chocolate
        {0.9, 0.7, 0.3},  // Diagonal Domino - Mustard
        {0.95, 0.9, 0.7}  // Monomino - Ivory
    },
    
    // Theme 20: Rainbow
    {
        {1.0, 0.0, 0.0},  // I - Red
        {1.0, 0.5, 0.0},  // O - Orange
        {1.0, 1.0, 0.0},  // T - Yellow
        {0.0, 1.0, 0.0},  // S - Green
        {0.0, 0.0, 1.0},  // Z - Blue
        {0.3, 0.0, 0.5},  // J - Indigo
        {0.5, 0.0, 1.0},  // L - Violet
        // Additional colors for smaller pieces
        {0.7, 0.0, 0.0},  // Straight Triomino - Dark Red
        {0.0, 0.7, 0.0},  // L Triomino - Dark Green
        {0.0, 0.0, 0.7},  // Reverse L Triomino - Dark Blue
        {0.7, 0.7, 0.0},  // V Triomino - Olive
        {0.0, 0.7, 0.7},  // H/V Domino - Teal
        {0.7, 0.0, 0.7},  // Diagonal Domino - Purple
        {1.0, 1.0, 1.0}   // Monomino - White
    },
    
  // Theme 21: Art Deco
    {
        {0.2, 0.4, 0.6},  // I - Deep Sapphire
        {0.9, 0.7, 0.2},  // O - Gold Leaf
        {0.5, 0.3, 0.7},  // T - Aubergine
        {0.3, 0.6, 0.5},  // S - Emerald Green
        {0.8, 0.2, 0.3},  // Z - Ruby Red
        {0.4, 0.5, 0.8},  // J - Cornflower Blue
        {0.9, 0.5, 0.2},  // L - Burnt Orange
        {0.3, 0.5, 0.5},  // Straight Triomino - Teal
        {0.7, 0.4, 0.6},  // L Triomino - Plum
        {0.4, 0.6, 0.4},  // Reverse L Triomino - Sage
        {0.6, 0.5, 0.7},  // V Triomino - Lavender
        {0.5, 0.4, 0.6},  // H/V Domino - Dusty Purple
        {0.8, 0.5, 0.3},  // Diagonal Domino - Bronze
        {0.9, 0.8, 0.7}   // Monomino - Ivory
    },
    
    // Theme 22: Northern Lights
    {
        {0.1, 0.3, 0.5},  // I - Deep Arctic Blue
        {0.2, 0.8, 0.8},  // O - Teal Aurora
        {0.7, 0.3, 0.8},  // T - Violet Aurora
        {0.3, 0.7, 0.5},  // S - Emerald Green
        {0.8, 0.2, 0.6},  // Z - Magenta Glow
        {0.0, 0.5, 0.7},  // J - Glacier Blue
        {0.9, 0.6, 0.2},  // L - Solar Flare Orange
        {0.4, 0.6, 0.7},  // Straight Triomino - Azure
        {0.6, 0.3, 0.7},  // L Triomino - Amethyst
        {0.3, 0.6, 0.4},  // Reverse L Triomino - Sea Green
        {0.5, 0.4, 0.8},  // V Triomino - Indigo
        {0.2, 0.7, 0.6},  // H/V Domino - Aquamarine
        {0.7, 0.5, 0.3},  // Diagonal Domino - Amber
        {0.9, 0.9, 0.9}   // Monomino - Snow White
    },
    
    // Theme 23: Moroccan Tiles
    {
        {0.2, 0.6, 0.7},  // I - Turquoise
        {0.9, 0.7, 0.4},  // O - Saffron
        {0.7, 0.3, 0.5},  // T - Burgundy
        {0.5, 0.8, 0.5},  // S - Mint Green
        {0.8, 0.4, 0.3},  // Z - Terracotta
        {0.4, 0.5, 0.7},  // J - Indigo
        {0.9, 0.5, 0.2},  // L - Burnt Sienna
        {0.3, 0.6, 0.6},  // Straight Triomino - Deep Teal
        {0.7, 0.4, 0.6},  // L Triomino - Magenta
        {0.4, 0.7, 0.4},  // Reverse L Triomino - Jade
        {0.6, 0.5, 0.7},  // V Triomino - Lilac
        {0.5, 0.6, 0.5},  // H/V Domino - Sage
        {0.8, 0.6, 0.3},  // Diagonal Domino - Gold
        {0.9, 0.8, 0.7}   // Monomino - Sand
    },
    
    // Theme 24: Bioluminescence
    {
        {0.0, 0.7, 0.7},  // I - Aqua Glow
        {0.3, 0.9, 0.6},  // O - Lime Phosphor
        {0.7, 0.3, 0.8},  // T - Violet Luminescence
        {0.2, 0.8, 0.5},  // S - Emerald Glow
        {0.9, 0.3, 0.5},  // Z - Coral Luminance
        {0.1, 0.5, 0.9},  // J - Electric Blue
        {0.9, 0.6, 0.2},  // L - Amber Radiance
        {0.2, 0.7, 0.6},  // Straight Triomino - Sea Green Glow
        {0.6, 0.3, 0.7},  // L Triomino - Purple Luminescence
        {0.3, 0.8, 0.4},  // Reverse L Triomino - Emerald Phosphor
        {0.5, 0.4, 0.8},  // V Triomino - Indigo Glow
        {0.4, 0.7, 0.5},  // H/V Domino - Mint Phosphor
        {0.8, 0.5, 0.3},  // Diagonal Domino - Amber Luminance
        {0.9, 0.9, 0.7}   // Monomino - Pale Glow
    },
    
    // Theme 25: Fossil
    {
        {0.6, 0.5, 0.4},  // I - Sandstone
        {0.7, 0.6, 0.5},  // O - Limestone
        {0.5, 0.4, 0.3},  // T - Dark Sediment
        {0.6, 0.7, 0.5},  // S - Moss Green
        {0.8, 0.5, 0.3},  // Z - Terra Cotta
        {0.4, 0.5, 0.6},  // J - Slate Gray
        {0.9, 0.7, 0.4},  // L - Amber
        {0.5, 0.6, 0.4},  // Straight Triomino - Olive
        {0.7, 0.5, 0.3},  // L Triomino - Rust
        {0.4, 0.5, 0.5},  // Reverse L Triomino - Stone
        {0.6, 0.4, 0.4},  // V Triomino - Clay
        {0.5, 0.4, 0.3},  // H/V Domino - Sepia
        {0.8, 0.6, 0.2},  // Diagonal Domino - Gold
        {0.9, 0.8, 0.6}   // Monomino - Ivory
    },
    
    // Theme 26: Silk Road
    {
        {0.3, 0.5, 0.7},  // I - Lapis Lazuli
        {0.9, 0.7, 0.3},  // O - Gold Thread
        {0.7, 0.3, 0.6},  // T - Byzantium Purple
        {0.4, 0.7, 0.5},  // S - Jade Green
        {0.8, 0.4, 0.3},  // Z - Rust Red
        {0.5, 0.6, 0.8},  // J - Soft Blue
        {0.9, 0.5, 0.2},  // L - Saffron
        {0.3, 0.6, 0.6},  // Straight Triomino - Turquoise
        {0.7, 0.4, 0.5},  // L Triomino - Magenta
        {0.4, 0.7, 0.4},  // Reverse L Triomino - Emerald
        {0.6, 0.5, 0.7},  // V Triomino - Lavender
        {0.5, 0.6, 0.5},  // H/V Domino - Sage
        {0.8, 0.6, 0.3},  // Diagonal Domino - Bronze
        {0.9, 0.8, 0.7}   // Monomino - Ivory
    },
    
    // Theme 27: Digital Glitch
    {
        {0.1, 0.8, 0.8},  // I - Glitch Cyan
        {0.9, 0.3, 0.6},  // O - Digital Pink
        {0.6, 0.2, 0.8},  // T - Pixel Purple
        {0.3, 0.8, 0.4},  // S - Circuit Green
        {0.8, 0.3, 0.3},  // Z - Error Red
        {0.2, 0.5, 0.9},  // J - Data Blue
        {0.9, 0.6, 0.2},  // L - Signal Orange
        {0.2, 0.7, 0.7},  // Straight Triomino - Teal Glitch
        {0.7, 0.3, 0.5},  // L Triomino - Magenta Pixel
        {0.4, 0.8, 0.5},  // Reverse L Triomino - Digital Green
        {0.5, 0.4, 0.7},  // V Triomino - Glitch Purple
        {0.3, 0.6, 0.6},  // H/V Domino - Aqua Circuit
        {0.8, 0.4, 0.4},  // Diagonal Domino - Error Crimson
        {0.9, 0.9, 0.5}   // Monomino - Signal Yellow
    },
    
    // Theme 28: Botanical
    {
        {0.3, 0.6, 0.5},  // I - Sage
        {0.8, 0.7, 0.4},  // O - Wheat
        {0.6, 0.4, 0.7},  // T - Lavender
        {0.4, 0.7, 0.4},  // S - Moss
        {0.8, 0.5, 0.3},  // Z - Rust
        {0.5, 0.6, 0.7},  // J - Stone Blue
        {0.9, 0.7, 0.5},  // L - Sand
        {0.4, 0.7, 0.5},  // Straight Triomino - Mint
        {0.7, 0.5, 0.4},  // L Triomino - Terra Cotta
        {0.5, 0.6, 0.4},  // Reverse L Triomino - Olive
        {0.6, 0.5, 0.7},  // V Triomino - Lilac
        {0.5, 0.6, 0.5},  // H/V Domino - Sea Green
        {0.8, 0.6, 0.3},  // Diagonal Domino - Amber
        {0.9, 0.8, 0.7}   // Monomino - Cream
    },
    
    // Theme 29: Jazz Age
    {
        {0.3, 0.4, 0.6},  // I - Midnight Blue
        {0.9, 0.7, 0.3},  // O - Gold Champagne
        {0.7, 0.3, 0.5},  // T - Burgundy
        {0.4, 0.6, 0.5},  // S - Sage Green
        {0.8, 0.4, 0.3},  // Z - Copper
        {0.5, 0.5, 0.7},  // J - Soft Indigo
        {0.9, 0.5, 0.2},  // L - Burnt Sienna
        {0.3, 0.5, 0.6},  // Straight Triomino - Steel Blue
        {0.7, 0.4, 0.5},  // L Triomino - Dusty Rose
        {0.4, 0.6, 0.4},  // Reverse L Triomino - Muted Green
        {0.6, 0.5, 0.7},  // V Triomino - Lavender
        {0.5, 0.4, 0.6},  // H/V Domino - Mauve
        {0.8, 0.5, 0.3},  // Diagonal Domino - Bronze
        {0.9, 0.8, 0.7}   // Monomino - Ivory
    },
    
    // Theme 30: Steampunk
    {
        {0.4, 0.5, 0.6},  // I - Gunmetal
        {0.8, 0.6, 0.3},  // O - Brass
        {0.6, 0.4, 0.7},  // T - Bronze Purple
        {0.4, 0.7, 0.5},  // S - Verdigris
        {0.8, 0.4, 0.3},  // Z - Copper
        {0.5, 0.6, 0.7},  // J - Steel Blue
        {0.9, 0.5, 0.2},  // L - Rust
        {0.4, 0.5, 0.5},  // Straight Triomino - Iron
        {0.7, 0.5, 0.4},  // L Triomino - Bronze
        {0.5, 0.6, 0.4},  // Reverse L Triomino - Patina
        {0.6, 0.4, 0.7},  // V Triomino - Violet Gear
        {0.5, 0.5, 0.6},  // H/V Domino - Silver
        {0.8, 0.6, 0.3},  // Diagonal Domino - Gold Leaf
        {0.9, 0.8, 0.6}   // Monomino - Parchment
    }
};
