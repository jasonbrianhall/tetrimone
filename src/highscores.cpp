#include "highscores.h"
#include <fstream>
#include <algorithm>
#include <cstdlib>
#include <sstream>
#include "tetrimone.h"

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
std::string createScoreKey(const std::string& difficulty, int width, int height) {
    std::stringstream keyStream;
    keyStream << difficulty << "_w" << width << "_h" << height;
    return keyStream.str();
}

void Highscores::addScore(const Score& score) {
    scores.push_back(score);
    
    // Create a key combining difficulty, width, and height
    std::string key = createScoreKey(score.difficulty, score.width, score.height);
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

bool Highscores::isHighScore(int score, int width, int height, const std::string& difficulty) const {
    // Create a key combining difficulty, width, and height
    std::string key = createScoreKey(difficulty, width, height);
    
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
    return score > configScores.back().score;
}

void Highscores::loadScores() {
    std::ifstream file(scorePath);
    if (!file) return;
    
    scores.clear();
    scoresByDifficultyAndSize.clear();
    
    std::string line;
    while (std::getline(file, line)) {
        size_t pos1 = line.find('|');
        size_t pos2 = line.find('|', pos1 + 1);
        size_t pos3 = line.find('|', pos2 + 1);
        size_t pos4 = line.find('|', pos3 + 1);
        
        if (pos1 != std::string::npos && pos2 != std::string::npos && 
            pos3 != std::string::npos && pos4 != std::string::npos) {
            
            Score score;
            score.name = line.substr(0, pos1);
            score.score = std::stoi(line.substr(pos1 + 1, pos2 - pos1 - 1));
            score.width = std::stoi(line.substr(pos2 + 1, pos3 - pos2 - 1));
            score.height = std::stoi(line.substr(pos3 + 1, pos4 - pos3 - 1));
            score.difficulty = line.substr(pos4 + 1);
            
            scores.push_back(score);
            
            // Create key and store in map
            std::string key = createScoreKey(score.difficulty, score.width, score.height);
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
             << score.difficulty << '\n';
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
        default: difficultyName = "Unknown";
    }

    // Check if this is a high score for the current grid size and difficulty
    if (highScores.isHighScore(score, GRID_WIDTH, GRID_HEIGHT, difficultyName)) {
        // Show dialog for name entry
        GtkWidget* dialog = gtk_dialog_new_with_buttons(
            "High Score!",
            GTK_WINDOW(app->window),
            GTK_DIALOG_MODAL,
            "_OK", GTK_RESPONSE_OK,
            NULL
        );
    
        GtkWidget* contentArea = gtk_dialog_get_content_area(GTK_DIALOG(dialog));
        gtk_container_set_border_width(GTK_CONTAINER(contentArea), 10);
    
        // Add congratulations message
        GtkWidget* label = gtk_label_new("Congratulations! You got a high score.");
        gtk_box_pack_start(GTK_BOX(contentArea), label, FALSE, FALSE, 10);
        
        // Add score display
        char scoreBuf[100];
        snprintf(scoreBuf, sizeof(scoreBuf), "Score: %d", score);
        GtkWidget* scoreLabel = gtk_label_new(scoreBuf);
        gtk_box_pack_start(GTK_BOX(contentArea), scoreLabel, FALSE, FALSE, 5);
        
        // Add grid size info
        char sizeBuf[100];
        snprintf(sizeBuf, sizeof(sizeBuf), "Grid: %d x %d", GRID_WIDTH, GRID_HEIGHT);
        GtkWidget* sizeLabel = gtk_label_new(sizeBuf);
        gtk_box_pack_start(GTK_BOX(contentArea), sizeLabel, FALSE, FALSE, 5);
        
        // Add difficulty display
        char diffBuf[100];
        snprintf(diffBuf, sizeof(diffBuf), "Difficulty: %s", difficultyName.c_str());
        GtkWidget* diffLabel = gtk_label_new(diffBuf);
        gtk_box_pack_start(GTK_BOX(contentArea), diffLabel, FALSE, FALSE, 5);
    
        // Add name entry field
        GtkWidget* entry = gtk_entry_new();
        gtk_entry_set_placeholder_text(GTK_ENTRY(entry), "Enter your name");
        gtk_box_pack_start(GTK_BOX(contentArea), entry, FALSE, FALSE, 10);
    
        gtk_widget_show_all(dialog);
    
        // Run the dialog
        int response = gtk_dialog_run(GTK_DIALOG(dialog));
    
        std::string playerName = "Anonymous";
        if (response == GTK_RESPONSE_OK) {
            const char* name = gtk_entry_get_text(GTK_ENTRY(entry));
            if (name && strlen(name) > 0) {
                playerName = name;
            }
        }
    
        // Destroy the dialog
        gtk_widget_destroy(dialog);
        
        // Add the high score
        Score newScore;
        newScore.name = playerName;
        newScore.score = score;
        newScore.width = GRID_WIDTH;
        newScore.height = GRID_HEIGHT;
        newScore.difficulty = difficultyName;
        
        highScores.addScore(newScore);
        return true;
    }
    return false;
}

void onViewHighScores(GtkMenuItem* menuItem, gpointer userData) {
    TetrimoneApp* app = static_cast<TetrimoneApp*>(userData);
    
    // Create a dialog to display high scores
    GtkWidget* dialog = gtk_dialog_new_with_buttons(
        "High Scores",
        GTK_WINDOW(app->window),
        GTK_DIALOG_MODAL,
        "_OK", GTK_RESPONSE_OK,
        NULL
    );
    
    // Make it a reasonable size
    gtk_window_set_default_size(GTK_WINDOW(dialog), 500, 400);
    
    // Get content area
    GtkWidget* contentArea = gtk_dialog_get_content_area(GTK_DIALOG(dialog));
    gtk_container_set_border_width(GTK_CONTAINER(contentArea), 10);
    
    // Create a notebook for different difficulty tabs
    GtkWidget* notebook = gtk_notebook_new();
    gtk_container_add(GTK_CONTAINER(contentArea), notebook);
    
    // Define difficulties
    std::vector<std::string> difficulties = {"Zen", "Easy", "Medium", "Hard", "Extreme", "Insane"};
    
    for (const auto& difficulty : difficulties) {
        // Get scores for this difficulty
        std::vector<Score> difficultyScores = 
            app->board->getHighScores().getScoresByDifficulty(difficulty);
        
        // Create a scrolled window for this tab
        GtkWidget* scrolledWindow = gtk_scrolled_window_new(NULL, NULL);
        gtk_scrolled_window_set_policy(
            GTK_SCROLLED_WINDOW(scrolledWindow),
            GTK_POLICY_AUTOMATIC,
            GTK_POLICY_AUTOMATIC
        );
        
        // Create a vertical box for the scores
        GtkWidget* vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 5);
        gtk_container_add(GTK_CONTAINER(scrolledWindow), vbox);
        
        // Add header
        GtkWidget* header = gtk_label_new(NULL);
        std::string headerText = "<b>" + difficulty + " Difficulty</b>";
        gtk_label_set_markup(GTK_LABEL(header), headerText.c_str());
        gtk_box_pack_start(GTK_BOX(vbox), header, FALSE, FALSE, 5);
        
        // Create a grid for the scores
        GtkWidget* grid = gtk_grid_new();
        gtk_grid_set_column_spacing(GTK_GRID(grid), 20);
        gtk_grid_set_row_spacing(GTK_GRID(grid), 5);
        gtk_box_pack_start(GTK_BOX(vbox), grid, FALSE, FALSE, 10);
        
        // Add header row
        GtkWidget* rankHeader = gtk_label_new("Rank");
        GtkWidget* nameHeader = gtk_label_new("Name");
        GtkWidget* scoreHeader = gtk_label_new("Score");
        GtkWidget* sizeHeader = gtk_label_new("Grid Size");
        
        gtk_widget_set_halign(rankHeader, GTK_ALIGN_START);
        gtk_widget_set_halign(nameHeader, GTK_ALIGN_START);
        gtk_widget_set_halign(scoreHeader, GTK_ALIGN_END);
        gtk_widget_set_halign(sizeHeader, GTK_ALIGN_END);
        
        gtk_grid_attach(GTK_GRID(grid), rankHeader, 0, 0, 1, 1);
        gtk_grid_attach(GTK_GRID(grid), nameHeader, 1, 0, 1, 1);
        gtk_grid_attach(GTK_GRID(grid), scoreHeader, 2, 0, 1, 1);
        gtk_grid_attach(GTK_GRID(grid), sizeHeader, 3, 0, 1, 1);
        
        // Add separator
        GtkWidget* separator = gtk_separator_new(GTK_ORIENTATION_HORIZONTAL);
        gtk_grid_attach(GTK_GRID(grid), separator, 0, 1, 4, 1);
        
        // Add scores
        if (difficultyScores.empty()) {
            GtkWidget* noScores = gtk_label_new("No scores recorded for this difficulty yet.");
            gtk_grid_attach(GTK_GRID(grid), noScores, 0, 2, 4, 1);
        } else {
            for (size_t i = 0; i < difficultyScores.size(); i++) {
                const Score& score = difficultyScores[i];
                int row = i + 2; // +2 because header and separator take rows 0 and 1
                
                // Rank
                std::string rankStr = std::to_string(i + 1) + ".";
                GtkWidget* rankLabel = gtk_label_new(rankStr.c_str());
                gtk_widget_set_halign(rankLabel, GTK_ALIGN_START);
                
                // Name
                GtkWidget* nameLabel = gtk_label_new(score.name.c_str());
                gtk_widget_set_halign(nameLabel, GTK_ALIGN_START);
                
                // Score
                GtkWidget* scoreLabel = gtk_label_new(std::to_string(score.score).c_str());
                gtk_widget_set_halign(scoreLabel, GTK_ALIGN_END);
                
                // Grid Size
                std::string sizeStr = std::to_string(score.width) + " x " + std::to_string(score.height);
                GtkWidget* sizeLabel = gtk_label_new(sizeStr.c_str());
                gtk_widget_set_halign(sizeLabel, GTK_ALIGN_END);
                
                // Add to grid
                gtk_grid_attach(GTK_GRID(grid), rankLabel, 0, row, 1, 1);
                gtk_grid_attach(GTK_GRID(grid), nameLabel, 1, row, 1, 1);
                gtk_grid_attach(GTK_GRID(grid), scoreLabel, 2, row, 1, 1);
                gtk_grid_attach(GTK_GRID(grid), sizeLabel, 3, row, 1, 1);
            }
        }
        
        // Add tab to notebook
        GtkWidget* tabLabel = gtk_label_new(difficulty.c_str());
        gtk_notebook_append_page(GTK_NOTEBOOK(notebook), scrolledWindow, tabLabel);
    }
    
    // Show all widgets
    gtk_widget_show_all(dialog);
    
    // Run dialog
    gtk_dialog_run(GTK_DIALOG(dialog));
    
    // Destroy dialog
    gtk_widget_destroy(dialog);
}
