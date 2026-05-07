#include "highscores.h"
#ifdef GTK3
#include "gtk3_dialog_helpers.h"
#include "tetrimone_gtk.h"
#endif

#ifdef QT5
#include "qt5_dialog_helpers.h"
#include "tetrimone_qt5.h"
#endif

#include <fstream>
#include <algorithm>
#include <cstdlib>
#include <sstream>
#include <iostream>

using namespace GTK3Helpers;

#ifdef _WIN32
    #include <direct.h>
    #define MKDIR(dir) _mkdir(dir)
    #define PATH_SEP "\\"
#else
    #include <sys/stat.h>
    #include <sys/types.h>
    #define MKDIR(dir) mkdir(dir, 0700)
    #define PATH_SEP "/"
#endif

Highscores::Highscores() {
    #ifdef _WIN32
        const char* home = getenv("APPDATA");
    #else
        const char* home = getenv("HOME");
    #endif

    if (!home) {
        home = ".";
    }

    std::string dirPath = std::string(home) + PATH_SEP + ".tetrimone";
    MKDIR(dirPath.c_str());
    scorePath = dirPath + PATH_SEP + "scores.txt";
    loadScores();
}

// Helper function to create a unique key for a specific game configuration
std::string createScoreKey(const std::string& difficulty, int width, int height, 
                          int initialJunkPercent, int junkLinesPerLevel) {
    std::stringstream keyStream;
    keyStream << difficulty << "_w" << width << "_h" << height 
              << "_jp" << initialJunkPercent << "_jl" << junkLinesPerLevel;
    return keyStream.str();
}

void Highscores::addScore(const Score& score) {
    scores.push_back(score);
    
    // Create a key combining difficulty, dimensions, and junk settings
    std::string key = createScoreKey(score.difficulty, score.width, score.height, 
                                     score.initialJunkPercent, score.junkLinesPerLevel);
    scoresByDifficultyAndSize[key].push_back(score);
    
    // Sort scores for this configuration by score (higher is better)
    auto& configScores = scoresByDifficultyAndSize[key];
    std::sort(configScores.begin(), configScores.end(),
        [](const Score& a, const Score& b) {
            return a.score > b.score; // Higher scores are better
        });
    
    // Keep only top MAX_SCORES_PER_CONFIG scores for this configuration
    if (configScores.size() > MAX_SCORES_PER_CONFIG) {
        configScores.resize(MAX_SCORES_PER_CONFIG);
    }
    
    // Update main scores vector to reflect all config-specific scores
    scores.clear();
    for (const auto& pair : scoresByDifficultyAndSize) {
        scores.insert(scores.end(), pair.second.begin(), pair.second.end());
    }
    
    saveScores();
}

const std::vector<Score>& Highscores::getScores() const {
    return scores;
}

std::vector<Score> Highscores::getScoresByDifficulty(const std::string& difficulty) const {
    std::vector<Score> difficultyScores;
    
    // Find all scores that match the requested difficulty, regardless of size
    for (const auto& pair : scoresByDifficultyAndSize) {
        // Extract difficulty from the key
        std::string key = pair.first;
        size_t underscore = key.find('_');
        if (underscore != std::string::npos) {
            std::string keyDifficulty = key.substr(0, underscore);
            if (keyDifficulty == difficulty) {
                // Add all scores from this difficulty to the result vector
                const auto& scores = pair.second;
                difficultyScores.insert(difficultyScores.end(), scores.begin(), scores.end());
            }
        }
    }
    
    // Sort the combined scores by score (higher is better)
    std::sort(difficultyScores.begin(), difficultyScores.end(),
        [](const Score& a, const Score& b) {
            return a.score > b.score;
        });
    
    return difficultyScores;
}

bool Highscores::isHighScore(int score, int width, int height, const std::string& difficulty,
                            int initialJunkPercent, int junkLinesPerLevel) const {
    // Create a key combining difficulty, dimensions, and junk settings
    std::string key = createScoreKey(difficulty, width, height, initialJunkPercent, junkLinesPerLevel);
        
    auto it = scoresByDifficultyAndSize.find(key);
    if (it == scoresByDifficultyAndSize.end()) {
        return true;  // First score for this configuration
    }
    
    const auto& configScores = it->second;
    
    if (configScores.size() < MAX_SCORES_PER_CONFIG) {
        return true;  // Less than max scores for this configuration
    }
    
    // Check if the score is better than the worst score in the list
    // For scores, higher is better, so check against the last (lowest) score
    bool isHighScore = score > configScores.back().score;

    return isHighScore;
}

void Highscores::loadScores() {
    std::ifstream file(scorePath);
    if (!file) return;
    
    scores.clear();
    scoresByDifficultyAndSize.clear();
    
    std::string line;
    while (std::getline(file, line)) {
        std::vector<std::string> parts;
        std::string part;
        std::istringstream iss(line);
        
        while (std::getline(iss, part, '|')) {
            parts.push_back(part);
        }
        
        // Check if we have the expected number of parts
        // Old format: name|score|width|height|difficulty
        // New format: name|score|width|height|difficulty|initialJunkPercent|junkLinesPerLevel
        if (parts.size() >= 5) {
            Score score;
            score.name = parts[0];
            score.score = std::stoi(parts[1]);
            score.width = std::stoi(parts[2]);
            score.height = std::stoi(parts[3]);
            score.difficulty = parts[4];
            
            // Handle additional junk settings if they exist (for backwards compatibility)
            score.initialJunkPercent = (parts.size() > 5) ? std::stoi(parts[5]) : 0;
            score.junkLinesPerLevel = (parts.size() > 6) ? std::stoi(parts[6]) : 0;
            
            scores.push_back(score);
            
            // Create key and store in map
            std::string key = createScoreKey(score.difficulty, score.width, score.height, 
                                            score.initialJunkPercent, score.junkLinesPerLevel);
            scoresByDifficultyAndSize[key].push_back(score);
        }
    }
    
    // Sort scores for each configuration
    for (auto& pair : scoresByDifficultyAndSize) {
        auto& configScores = pair.second;
        std::sort(configScores.begin(), configScores.end(),
            [](const Score& a, const Score& b) {
                return a.score > b.score; // Higher scores are better
            });
        
        if (configScores.size() > MAX_SCORES_PER_CONFIG) {
            configScores.resize(MAX_SCORES_PER_CONFIG);
        }
    }
    
    // Rebuild main scores vector
    scores.clear();
    for (const auto& pair : scoresByDifficultyAndSize) {
        scores.insert(scores.end(), pair.second.begin(), pair.second.end());
    }
}

void Highscores::saveScores() {
    std::ofstream file(scorePath);
    if (!file) return;
    
    for (const auto& score : scores) {
        file << score.name << '|'
             << score.score << '|'
             << score.width << '|'
             << score.height << '|'
             << score.difficulty << '|'
             << score.initialJunkPercent << '|'
             << score.junkLinesPerLevel << '\n';
    }
}

// Version that takes app parameter and shows dialog
bool TetrimoneBoard::checkAndRecordHighScore(TetrimoneApp* app) {
    // Determine difficulty name based on app's difficulty setting
    std::string difficultyName;
    switch (app->difficulty) {
        case 0: difficultyName = "Zen"; break;
        case 1: difficultyName = "Easy"; break;
        case 2: difficultyName = "Medium"; break;
        case 3: difficultyName = "Hard"; break;
        case 4: difficultyName = "Extreme"; break;
        case 5: difficultyName = "Insane"; break;
        default: difficultyName = "Unknown"; break;
    }
    
    // Check if this is a high score
    if (highScores.isHighScore(score, GRID_WIDTH, GRID_HEIGHT, difficultyName, 
                               junkLinesPercentage, junkLinesPerLevel)) {
        
        // Configure score entry dialog
        ScoreEntryConfig entryConfig{
            .title = "New High Score!",
            .score = score,
            .difficulty = difficultyName,
            .gridSize = std::string("Grid: ") + std::to_string(GRID_WIDTH) + " x " + std::to_string(GRID_HEIGHT),
            .junkInfo = std::string("Junk: Initial ") + std::to_string(junkLinesPercentage) + 
                       "%, Per Level " + std::to_string(junkLinesPerLevel)
        };
        
        // Get player name using helper
        std::string playerName = createScoreEntryDialog(GTK_WINDOW(app->window), entryConfig);
        
        if (playerName.empty()) {
            playerName = "Anonymous";
        }
        
        // Add the high score
        Score newScore;
        newScore.name = playerName;
        newScore.score = score;
        newScore.width = GRID_WIDTH;
        newScore.height = GRID_HEIGHT;
        newScore.difficulty = difficultyName;
        newScore.initialJunkPercent = junkLinesPercentage;
        newScore.junkLinesPerLevel = junkLinesPerLevel;
        
        highScores.addScore(newScore);
        return true;
    }
    return false;
}

void onViewHighScores(GtkMenuItem* menuItem, gpointer userData) {
    TetrimoneApp* app = static_cast<TetrimoneApp*>(userData);
    
    // Get all scores
    const std::vector<Score>& allScores = app->board->getHighScores().getScores();
    
    // Difficulty levels to create tabs for
    const std::vector<std::string> difficulties = {
        "All", "Zen", "Easy", "Medium", "Hard", "Extreme", "Insane"
    };
    
    // Build tab data
    std::vector<ScoreTabData> tabs;
    for (const auto& difficulty : difficulties) {
        ScoreTabData tabData;
        tabData.tabName = difficulty;
        
        if (difficulty == "All") {
            tabData.scores = allScores;
        } else {
            tabData.scores = app->board->getHighScores().getScoresByDifficulty(difficulty);
        }
        
        tabs.push_back(tabData);
    }
    
    // Configure and display tabulator
    ScoreTabulatorConfig config{
        .title = "High Scores",
        .tabs = tabs,
        .width = 900,
        .height = 600
    };
    
    createScoreTabulatorDialog(GTK_WINDOW(app->window), config);
}
