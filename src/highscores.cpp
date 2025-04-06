#include "highscores.h"
#include <fstream>
#include <algorithm>
#include <cstdlib>
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

    std::string dirPath = std::string(home) + PATH_SEP + ".minesweeper";
    MKDIR(dirPath.c_str());
    scorePath = dirPath + PATH_SEP + "scores.txt";
    loadScores();
}

void Highscores::addScore(const Score& score) {
    scores.push_back(score);
    scoresByDifficulty[score.difficulty].push_back(score);
    
    // Sort scores for this difficulty by time
    auto& difficultyScores = scoresByDifficulty[score.difficulty];
    std::sort(difficultyScores.begin(), difficultyScores.end(),
        [](const Score& a, const Score& b) {
            return a.time < b.time;
        });
    
    // Keep only top MAX_SCORES_PER_DIFFICULTY scores for this difficulty
    if (difficultyScores.size() > MAX_SCORES_PER_DIFFICULTY) {
        difficultyScores.resize(MAX_SCORES_PER_DIFFICULTY);
    }
    
    // Update main scores vector to reflect all difficulty-specific scores
    scores.clear();
    for (const auto& pair : scoresByDifficulty) {
        scores.insert(scores.end(), pair.second.begin(), pair.second.end());
    }
    
    saveScores();
}

const std::vector<Score>& Highscores::getScores() const {
    return scores;
}

std::vector<Score> Highscores::getScoresByDifficulty(const std::string& difficulty) const {
    auto it = scoresByDifficulty.find(difficulty);
    if (it != scoresByDifficulty.end()) {
        return it->second;
    }
    return std::vector<Score>();
}

bool Highscores::isHighScore(int time, const std::string& difficulty) const {
    auto it = scoresByDifficulty.find(difficulty);
    if (it == scoresByDifficulty.end()) {
        return true;  // First score for this difficulty
    }
    
    const auto& difficultyScores = it->second;
    if (difficultyScores.size() < MAX_SCORES_PER_DIFFICULTY) {
        return true;  // Less than max scores for this difficulty
    }
    
    return time < difficultyScores.back().time;  // Compare with worst time in top 10
}

void Highscores::loadScores() {
    std::ifstream file(scorePath);
    if (!file) return;
    
    scores.clear();
    scoresByDifficulty.clear();
    
    std::string line;
    while (std::getline(file, line)) {
        size_t pos1 = line.find('|');
        size_t pos2 = line.find('|', pos1 + 1);
        if (pos1 != std::string::npos && pos2 != std::string::npos) {
            Score score;
            score.name = line.substr(0, pos1);
            score.time = std::stoi(line.substr(pos1 + 1, pos2 - pos1 - 1));
            score.difficulty = line.substr(pos2 + 1);
            scores.push_back(score);
            scoresByDifficulty[score.difficulty].push_back(score);
        }
    }
    
    // Sort scores for each difficulty
    for (auto& pair : scoresByDifficulty) {
        auto& difficultyScores = pair.second;
        std::sort(difficultyScores.begin(), difficultyScores.end(),
            [](const Score& a, const Score& b) {
                return a.time < b.time;
            });
        if (difficultyScores.size() > MAX_SCORES_PER_DIFFICULTY) {
            difficultyScores.resize(MAX_SCORES_PER_DIFFICULTY);
        }
    }
    
    // Rebuild main scores vector
    scores.clear();
    for (const auto& pair : scoresByDifficulty) {
        scores.insert(scores.end(), pair.second.begin(), pair.second.end());
    }
}

void Highscores::saveScores() {
    std::ofstream file(scorePath);
    if (!file) return;
    
    for (const auto& score : scores) {
        file << score.name << '|' << score.time << '|' << score.difficulty << '\n';
    }
}

bool TetrimoneBoard::checkAndRecordHighScore() {
    // Determine difficulty name using current difficulty level
    std::string difficultyName;
    switch (level) {
        case 0: difficultyName = "Zen"; break;
        case 1: difficultyName = "Easy"; break;
        case 2: difficultyName = "Medium"; break;
        case 3: difficultyName = "Hard"; break;
        case 4: difficultyName = "Extreme"; break;
        case 5: difficultyName = "Insane"; break;
        default: difficultyName = "Medium";
    }

    // Check if this is a high score
    if (highScores.isHighScore(score, difficultyName)) {
        // This would be implemented to show a GTK dialog for name input
        // For now, we'll use a default name
        std::string playerName = "Player";
        
        // Add the high score
        Score newScore;
        newScore.name = playerName;
        newScore.time = score;
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
    gtk_window_set_default_size(GTK_WINDOW(dialog), 400, 400);
    
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
        
        gtk_widget_set_halign(rankHeader, GTK_ALIGN_START);
        gtk_widget_set_halign(nameHeader, GTK_ALIGN_START);
        gtk_widget_set_halign(scoreHeader, GTK_ALIGN_END);
        
        gtk_grid_attach(GTK_GRID(grid), rankHeader, 0, 0, 1, 1);
        gtk_grid_attach(GTK_GRID(grid), nameHeader, 1, 0, 1, 1);
        gtk_grid_attach(GTK_GRID(grid), scoreHeader, 2, 0, 1, 1);
        
        // Add separator
        GtkWidget* separator = gtk_separator_new(GTK_ORIENTATION_HORIZONTAL);
        gtk_grid_attach(GTK_GRID(grid), separator, 0, 1, 3, 1);
        
        // Add scores
        if (difficultyScores.empty()) {
            GtkWidget* noScores = gtk_label_new("No scores recorded for this difficulty yet.");
            gtk_grid_attach(GTK_GRID(grid), noScores, 0, 2, 3, 1);
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
                GtkWidget* scoreLabel = gtk_label_new(std::to_string(score.time).c_str());
                gtk_widget_set_halign(scoreLabel, GTK_ALIGN_END);
                
                // Add to grid
                gtk_grid_attach(GTK_GRID(grid), rankLabel, 0, row, 1, 1);
                gtk_grid_attach(GTK_GRID(grid), nameLabel, 1, row, 1, 1);
                gtk_grid_attach(GTK_GRID(grid), scoreLabel, 2, row, 1, 1);
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
