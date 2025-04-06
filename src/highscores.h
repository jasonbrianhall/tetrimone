#pragma once
#include <string>
#include <vector>
#include <map>

// Modified Score structure to include width and height instead of time
struct Score {
    std::string name;
    int score;
    int width;         // Grid width
    int height;        // Grid height
    std::string difficulty;
};

class Highscores {
public:
    Highscores();
    void addScore(const Score& score);
    const std::vector<Score>& getScores() const;
    std::vector<Score> getScoresByDifficulty(const std::string& difficulty) const;
    // Modified to check score against width and height
    bool isHighScore(int score, int width, int height, const std::string& difficulty) const;

private:
    void loadScores();
    void saveScores();
    
    std::string scorePath;
    std::vector<Score> scores;  // All scores
    // Map key is now a combination of difficulty, width, and height
    std::map<std::string, std::vector<Score>> scoresByDifficultyAndSize;
    const size_t MAX_SCORES_PER_CONFIG = 10;
};
