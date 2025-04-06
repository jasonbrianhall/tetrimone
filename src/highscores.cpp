#include "highscores.h"
#include <fstream>
#include <algorithm>
#include <cstdlib>
#include <sstream>
#include "tetrimone.h"
#include <iostream>

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
    
    // Create main dialog
    GtkWidget* dialog = gtk_dialog_new_with_buttons(
        "High Scores",
        GTK_WINDOW(app->window),
        GTK_DIALOG_MODAL,
        "_Close", GTK_RESPONSE_CLOSE,
        NULL
    );
    gtk_window_set_default_size(GTK_WINDOW(dialog), 800, 600);
    
    // Get content area
    GtkWidget* contentArea = gtk_dialog_get_content_area(GTK_DIALOG(dialog));
    gtk_container_set_border_width(GTK_CONTAINER(contentArea), 10);
    
    // Create notebook (tabbed interface)
    GtkWidget* notebook = gtk_notebook_new();
    // Ensure notebook expands to fill the entire content area
    gtk_box_pack_start(GTK_BOX(contentArea), notebook, TRUE, TRUE, 0);
    
    // Get all scores
    const std::vector<Score>& allScores = app->board->getHighScores().getScores();
    
    // Difficulty levels to create tabs for
    const std::vector<std::string> difficulties = {
        "All", "Zen", "Easy", "Medium", "Hard", "Extreme", "Insane"
    };
    
    // Create a tab for each difficulty level
    for (const auto& difficulty : difficulties) {
        // Prepare scores for this difficulty
        std::vector<Score> tabScores;
        if (difficulty == "All") {
            tabScores = allScores;
        } else {
            tabScores = app->board->getHighScores().getScoresByDifficulty(difficulty);
        }
        
        // Create scrolled window for this tab
        GtkWidget* scrollWindow = gtk_scrolled_window_new(NULL, NULL);
        gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrollWindow), 
                                        GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
        
        // Ensure scrolled window expands
        gtk_scrolled_window_set_shadow_type(GTK_SCROLLED_WINDOW(scrollWindow), GTK_SHADOW_ETCHED_IN);
        
        // Create list store and tree view
        GtkListStore* listStore = gtk_list_store_new(5, 
            G_TYPE_STRING,  // Name
            G_TYPE_INT,     // Score
            G_TYPE_STRING,  // Difficulty
            G_TYPE_STRING,  // Grid Size
            G_TYPE_INT      // Unused column for potential sorting
        );
        
        // Populate list store
        for (const auto& score : tabScores) {
            GtkTreeIter iter;
            gtk_list_store_append(listStore, &iter);
            // Create grid size string explicitly
            char gridSizeBuffer[50];
            snprintf(gridSizeBuffer, sizeof(gridSizeBuffer), "%d x %d", score.width, score.height);
            
            gtk_list_store_set(listStore, &iter, 
                0, score.name.c_str(),
                1, score.score,
                2, score.difficulty.c_str(), 
                3, gridSizeBuffer,
                4, score.score,  // Duplicate score for potential sorting
                -1
            );
        }
        
        // Create tree view
        GtkWidget* treeView = gtk_tree_view_new_with_model(GTK_TREE_MODEL(listStore));
        g_object_unref(listStore);
        
        // Ensure tree view can expand
        gtk_tree_view_set_grid_lines(GTK_TREE_VIEW(treeView), GTK_TREE_VIEW_GRID_LINES_BOTH);
        
        // Create columns with sorting
        GtkCellRenderer* renderer = gtk_cell_renderer_text_new();
        
        // Name column
        GtkTreeViewColumn* nameColumn = gtk_tree_view_column_new_with_attributes(
            "Name", renderer, "text", 0, NULL
        );
        gtk_tree_view_column_set_expand(nameColumn, TRUE);
        gtk_tree_view_column_set_sort_column_id(nameColumn, 0);
        gtk_tree_view_column_set_resizable(nameColumn, TRUE);
        gtk_tree_view_append_column(GTK_TREE_VIEW(treeView), nameColumn);
        
        // Score column
        GtkTreeViewColumn* scoreColumn = gtk_tree_view_column_new_with_attributes(
            "Score", renderer, "text", 1, NULL
        );
        gtk_tree_view_column_set_expand(scoreColumn, TRUE);
        gtk_tree_view_column_set_sort_column_id(scoreColumn, 1);
        gtk_tree_view_column_set_resizable(scoreColumn, TRUE);
        gtk_tree_view_append_column(GTK_TREE_VIEW(treeView), scoreColumn);
        
        // Difficulty column
        GtkTreeViewColumn* diffColumn = gtk_tree_view_column_new_with_attributes(
            "Difficulty", renderer, "text", 2, NULL
        );
        gtk_tree_view_column_set_expand(diffColumn, TRUE);
        gtk_tree_view_column_set_sort_column_id(diffColumn, 2);
        gtk_tree_view_column_set_resizable(diffColumn, TRUE);
        gtk_tree_view_append_column(GTK_TREE_VIEW(treeView), diffColumn);
        
        // Grid Size column
        GtkTreeViewColumn* sizeColumn = gtk_tree_view_column_new_with_attributes(
            "Grid Size", renderer, "text", 3, NULL
        );
        gtk_tree_view_column_set_expand(sizeColumn, TRUE);
        gtk_tree_view_column_set_sort_column_id(sizeColumn, 3);
        gtk_tree_view_column_set_resizable(sizeColumn, TRUE);
        gtk_tree_view_append_column(GTK_TREE_VIEW(treeView), sizeColumn);
        
        // Add some form of interaction
        gtk_tree_view_set_headers_clickable(GTK_TREE_VIEW(treeView), TRUE);
        
        // Add tree view to scrolled window
        gtk_container_add(GTK_CONTAINER(scrollWindow), treeView);
        
        // Add tab to notebook
        char tabLabel[50];
        snprintf(tabLabel, sizeof(tabLabel), "%s (%zu)", difficulty.c_str(), tabScores.size());
        gtk_notebook_append_page(GTK_NOTEBOOK(notebook), scrollWindow, gtk_label_new(tabLabel));
    }
    
    // Show all widgets
    gtk_widget_show_all(dialog);
    
    // Run dialog
    gtk_dialog_run(GTK_DIALOG(dialog));
    
    // Destroy dialog
    gtk_widget_destroy(dialog);
}
