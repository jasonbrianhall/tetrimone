#ifndef TETRIMONE_SETTINGS_H
#define TETRIMONE_SETTINGS_H

#include "tetrimone.h"
#include <fstream>
#include <iostream>
#include <string>
#include <map>
#include <variant>
#include <vector>
#include <sstream>
#include <regex>  // For regular expression support
#include <algorithm>  // For std::max, std::min
#include <atomic>  // For std::atomic in TetrimoneBoard

#ifdef _WIN32
    #include <direct.h>
    #define MKDIR(dir) _mkdir(dir)
#else
    #include <sys/stat.h>
    #include <sys/types.h>
    #define MKDIR(dir) mkdir(dir, 0755)
#endif

// Simple JSON library implementation for minimal dependencies
class SimpleJson {
private:
    std::string indent(int level) const {
        return std::string(level * 4, ' ');
    }
    
public:
    std::map<std::string, std::variant<bool, int, double, std::string, 
        std::map<std::string, std::variant<bool, int, double, std::string>>,
        std::vector<bool>>> data;
    
    template<typename T>
    void set(const std::string& key, const T& value) {
        data[key] = value;
    }
    
    template<typename T>
    T get(const std::string& key, const T& defaultValue) const {
        if (data.find(key) != data.end()) {
            try {
                return std::get<T>(data.at(key));
            } catch (const std::bad_variant_access&) {
                return defaultValue;
            }
        }
        return defaultValue;
    }
    
    bool contains(const std::string& key) const {
        return data.find(key) != data.end();
    }
    
    std::string toString(int level = 0) const {
        std::stringstream ss;
        ss << "{\n";
        
        bool first = true;
        for (const auto& [key, value] : data) {
            if (!first) ss << ",\n";
            first = false;
            
            ss << indent(level + 1) << "\"" << key << "\": ";
            
            if (auto* pval = std::get_if<bool>(&value)) {
                ss << (*pval ? "true" : "false");
            } else if (auto* pval = std::get_if<int>(&value)) {
                ss << *pval;
            } else if (auto* pval = std::get_if<double>(&value)) {
                ss << *pval;
            } else if (auto* pval = std::get_if<std::string>(&value)) {
                ss << "\"" << *pval << "\"";
            } else if (auto* pval = std::get_if<std::map<std::string, 
                      std::variant<bool, int, double, std::string>>>(&value)) {
                ss << "{\n";
                bool inner_first = true;
                for (const auto& [inner_key, inner_value] : *pval) {
                    if (!inner_first) ss << ",\n";
                    inner_first = false;
                    
                    ss << indent(level + 2) << "\"" << inner_key << "\": ";
                    
                    if (auto* inner_pval = std::get_if<bool>(&inner_value)) {
                        ss << (*inner_pval ? "true" : "false");
                    } else if (auto* inner_pval = std::get_if<int>(&inner_value)) {
                        ss << *inner_pval;
                    } else if (auto* inner_pval = std::get_if<double>(&inner_value)) {
                        ss << *inner_pval;
                    } else if (auto* inner_pval = std::get_if<std::string>(&inner_value)) {
                        ss << "\"" << *inner_pval << "\"";
                    }
                }
                ss << "\n" << indent(level + 1) << "}";
            } else if (auto* pval = std::get_if<std::vector<bool>>(&value)) {
                ss << "[";
                bool array_first = true;
                for (bool item : *pval) {
                    if (!array_first) ss << ", ";
                    array_first = false;
                    ss << (item ? "true" : "false");
                }
                ss << "]";
            }
        }
        
        ss << "\n" << indent(level) << "}";
        return ss.str();
    }
    
    static SimpleJson parse(const std::string& jsonStr) {
        SimpleJson json;
        // Basic JSON parsing implementation
        // This would be expanded in a real implementation
        
        return json;
    }
};

/**
 * Get platform-specific config directory
 * Creates the directory if it doesn't exist
 * @return Path to the config directory
 */
std::string getConfigDirectory() {
    std::string configDir;
    
    #ifdef _WIN32
        // Windows: Use %APPDATA%
        const char* appdata = std::getenv("APPDATA");
        if (appdata) {
            configDir = std::string(appdata) + "\\Tetrimone";
        } else {
            configDir = "."; // Fallback to current directory
        }
        
        // Create directory if it doesn't exist
        MKDIR(configDir.c_str());
    #else
        // Linux/Unix: Use XDG_CONFIG_HOME or fallback to ~/.config
        const char* xdg_config = std::getenv("XDG_CONFIG_HOME");
        if (xdg_config && xdg_config[0] != '\0') {
            configDir = std::string(xdg_config) + "/tetrimone";
        } else {
            const char* home = std::getenv("HOME");
            if (home) {
                configDir = std::string(home) + "/.config/tetrimone";
            } else {
                configDir = "."; // Fallback to current directory
            }
        }
        
        // Create directory if it doesn't exist
        MKDIR(configDir.c_str());
    #endif
    
    return configDir;
}

/**
 * Get the full path to the settings file
 * @return Path to the settings file
 */
std::string getSettingsFilePath() {
    return getConfigDirectory() + 
           #ifdef _WIN32
               "\\"
           #else
               "/"
           #endif
           "tetrimone_settings.json";
}

/**
 * Saves all current game settings to a JSON file
 * @param app Pointer to the TetrimoneApp struct
 * @return bool True if settings were saved successfully
 */
bool saveGameSettings(TetrimoneApp* app) {
    try {
        SimpleJson settings;
        
        // Basic game settings
        settings.set("retroMode", false);
        settings.set("simpleBlocks", app->board->simpleBlocksActive);
        settings.set("retroMusic", app->board->retroMusicActive);
        settings.set("showGridLines", app->board->showGridLines);
        settings.set("ghostPieceEnabled", app->board->isGhostPieceEnabled());
        //settings.set("soundEnabled", app->board->sound_enabled_);
        settings.set("soundEnabled", true);
        settings.set("difficulty", app->difficulty);
        
        // Background image settings
        settings.set("useBackgroundImage", app->board->isUsingBackgroundImage());
        settings.set("backgroundImagePath", app->board->getBackgroundImagePath());
        settings.set("backgroundOpacity", app->board->getBackgroundOpacity());
        settings.set("useBackgroundZip", app->board->isUsingBackgroundZip());
        settings.set("backgroundZipPath", app->board->backgroundZipPath);
        
        // Game board settings
        settings.set("gridWidth", GRID_WIDTH);
        settings.set("gridHeight", GRID_HEIGHT);
        settings.set("blockSize", BLOCK_SIZE);
        settings.set("minBlockSize", app->board->getMinBlockSize());
        
        // Music track settings
        settings.set("backgroundMusicPlaying", app->backgroundMusicPlaying);
        
        std::vector<bool> enabledTracksVec;
        for (int i = 0; i < 5; i++) {
            enabledTracksVec.push_back(app->board->enabledTracks[i]);
        }
        settings.set("enabledTracks", enabledTracksVec);
        
        // Advanced settings
        settings.set("junkLinesPercentage", app->board->junkLinesPercentage);
        settings.set("junkLinesPerLevel", app->board->junkLinesPerLevel);
        settings.set("initialLevel", app->board->initialLevel);
        
        // Joystick settings
        settings.set("joystickEnabled", app->joystickEnabled);
        
        std::map<std::string, std::variant<bool, int, double, std::string>> joyMap;
        joyMap["rotate_cw_button"] = app->joystickMapping.rotate_cw_button;
        joyMap["rotate_ccw_button"] = app->joystickMapping.rotate_ccw_button;
        joyMap["hard_drop_button"] = app->joystickMapping.hard_drop_button;
        joyMap["pause_button"] = app->joystickMapping.pause_button;
        joyMap["x_axis"] = app->joystickMapping.x_axis;
        joyMap["y_axis"] = app->joystickMapping.y_axis;
        joyMap["invert_x"] = app->joystickMapping.invert_x;
        joyMap["invert_y"] = app->joystickMapping.invert_y;
        settings.set("joystickMapping", joyMap);
        
        // Visual theme settings
        settings.set("currentThemeIndex", currentThemeIndex);
        
        // Write to file
        std::string settingsJson = settings.toString();
        std::ofstream file(getSettingsFilePath());
        if (!file.is_open()) {
            std::cerr << "Failed to open settings file for writing: " 
                     << getSettingsFilePath() << std::endl;
            return false;
        }
        
        file << settingsJson;
        file.close();
        
        return true;
    } catch (const std::exception& e) {
        std::cerr << "Error saving settings: " << e.what() << std::endl;
        return false;
    }
}

/**
 * Loads game settings from JSON file and applies them to the app
 * @param app Pointer to the TetrimoneApp struct
 * @return bool True if settings were loaded successfully
 */
/**
 * Loads game settings from JSON file and applies them to the app
 * @param app Pointer to the TetrimoneApp struct
 * @return bool True if settings were loaded successfully
 */
bool loadGameSettings(TetrimoneApp* app) {
    try {
        std::string settingsFilePath = getSettingsFilePath();
        std::ifstream file(settingsFilePath);
        if (!file.is_open()) {
            std::cerr << "Settings file not found: " << settingsFilePath << std::endl;
            // No settings file exists, use defaults
            return false;
        }
        
        // Read the entire file into a string
        std::string jsonContent;
        std::string line;
        while (std::getline(file, line)) {
            jsonContent += line + "\n";
        }
        file.close();
        
        // Basic parsing for key settings (especially minBlockSize)
        // This doesn't rely on the incomplete SimpleJson::parse method
        
        // Function to extract an integer value from a JSON key
        auto extractInt = [&jsonContent](const std::string& key, int defaultVal) -> int {
            std::string pattern = "\"" + key + "\"\\s*:\\s*(\\d+)";
            std::regex keyRegex(pattern);
            std::smatch match;
            if (std::regex_search(jsonContent, match, keyRegex) && match.size() > 1) {
                return std::stoi(match[1].str());
            }
            return defaultVal;
        };
        
        // Function to extract a boolean value from a JSON key
        auto extractBool = [&jsonContent](const std::string& key, bool defaultVal) -> bool {
            std::string pattern = "\"" + key + "\"\\s*:\\s*(true|false)";
            std::regex keyRegex(pattern);
            std::smatch match;
            if (std::regex_search(jsonContent, match, keyRegex) && match.size() > 1) {
                return match[1].str() == "true";
            }
            return defaultVal;
        };
        
        // Function to extract a double value from a JSON key
        auto extractDouble = [&jsonContent](const std::string& key, double defaultVal) -> double {
            std::string pattern = "\"" + key + "\"\\s*:\\s*(\\d+\\.?\\d*)";
            std::regex keyRegex(pattern);
            std::smatch match;
            if (std::regex_search(jsonContent, match, keyRegex) && match.size() > 1) {
                return std::stod(match[1].str());
            }
            return defaultVal;
        };
        
        // Function to extract a string value from a JSON key
        auto extractString = [&jsonContent](const std::string& key, const std::string& defaultVal) -> std::string {
            std::string pattern = "\"" + key + "\"\\s*:\\s*\"([^\"]*)\"";
            std::regex keyRegex(pattern);
            std::smatch match;
            if (std::regex_search(jsonContent, match, keyRegex) && match.size() > 1) {
                return match[1].str();
            }
            return defaultVal;
        };
        
        // Function to extract an array of booleans from a JSON key
        auto extractBoolArray = [&jsonContent](const std::string& key, const std::vector<bool>& defaultVal) -> std::vector<bool> {
            std::string pattern = "\"" + key + "\"\\s*:\\s*\\[(true|false)(?:\\s*,\\s*(true|false))*\\]";
            std::regex keyRegex(pattern);
            std::smatch match;
            if (std::regex_search(jsonContent, match, keyRegex)) {
                std::string arrayStr = match[0].str();
                std::regex valueRegex("(true|false)");
                std::vector<bool> result;
                
                auto begin = std::sregex_iterator(arrayStr.begin(), arrayStr.end(), valueRegex);
                auto end = std::sregex_iterator();
                
                for (std::sregex_iterator i = begin; i != end; ++i) {
                    std::smatch valueMatch = *i;
                    result.push_back(valueMatch[1].str() == "true");
                }
                
                return result;
            }
            return defaultVal;
        };
        
        // Apply basic game settings
        app->board->retroModeActive = extractBool("retroMode", false);
        app->board->simpleBlocksActive = extractBool("simpleBlocks", false);
        app->board->retroMusicActive = extractBool("retroMusic", false);
        app->board->setShowGridLines(extractBool("showGridLines", false));
        app->board->setGhostPieceEnabled(extractBool("ghostPieceEnabled", true));
        app->board->sound_enabled_ = extractBool("soundEnabled", true);
        app->difficulty = extractInt("difficulty", 2);
        
bool useBackgroundImage = extractBool("useBackgroundImage", false);
app->board->setUseBackgroundImage(useBackgroundImage);

std::string bgImagePath = extractString("backgroundImagePath", "");
if (useBackgroundImage && !bgImagePath.empty()) {
    app->board->loadBackgroundImage(bgImagePath);
}

app->board->setBackgroundOpacity(extractDouble("backgroundOpacity", 0.3));

// Extract background zip settings
bool useBackgroundZip = extractBool("useBackgroundZip", false);
app->board->setUseBackgroundZip(useBackgroundZip);

std::string bgZipPath = extractString("backgroundZipPath", "");
if (useBackgroundZip && !bgZipPath.empty()) {
    app->board->loadBackgroundImagesFromZip(bgZipPath);
}
        
        // Apply game board settings
        int gridWidth = extractInt("gridWidth", 10);
        GRID_WIDTH = std::max(MIN_GRID_WIDTH, std::min(MAX_GRID_WIDTH, gridWidth));
        
        int gridHeight = extractInt("gridHeight", 20);
        GRID_HEIGHT = std::max(MIN_GRID_HEIGHT, std::min(MAX_GRID_HEIGHT, gridHeight));
        
        int blockSize = extractInt("blockSize", 30);
        BLOCK_SIZE = std::max(MIN_BLOCK_SIZE, std::min(MAX_BLOCK_SIZE, blockSize));
        
        // This is the key setting we're fixing - load minimum block size
        int minBlockSize = extractInt("minBlockSize", 1);
        app->board->setMinBlockSize(minBlockSize);
        
        // Apply music track settings
        app->backgroundMusicPlaying = extractBool("backgroundMusicPlaying", true);
        
        auto enabledTracks = extractBoolArray("enabledTracks", {true, true, true, true, true});
        for (int i = 0; i < std::min(5, static_cast<int>(enabledTracks.size())); i++) {
            app->board->enabledTracks[i] = enabledTracks[i];
        }
        
        // Apply advanced settings
        app->board->junkLinesPercentage = extractInt("junkLinesPercentage", 0);
        app->board->junkLinesPerLevel = extractInt("junkLinesPerLevel", 0);
        app->board->initialLevel = extractInt("initialLevel", 1);
        
        // Apply joystick settings
        app->joystickEnabled = extractBool("joystickEnabled", false);
        
        // Extract joystick mapping - this is a bit more complex for JSON objects
        // So we'll read individual settings directly
        app->joystickMapping.rotate_cw_button = extractInt("joystickMapping_rotate_cw_button", 0);
        app->joystickMapping.rotate_ccw_button = extractInt("joystickMapping_rotate_ccw_button", 1);
        app->joystickMapping.hard_drop_button = extractInt("joystickMapping_hard_drop_button", 2);
        app->joystickMapping.pause_button = extractInt("joystickMapping_pause_button", 3);
        app->joystickMapping.x_axis = extractInt("joystickMapping_x_axis", 0);
        app->joystickMapping.y_axis = extractInt("joystickMapping_y_axis", 1);
        app->joystickMapping.invert_x = extractBool("joystickMapping_invert_x", false);
        app->joystickMapping.invert_y = extractBool("joystickMapping_invert_y", false);
        
        // Apply visual theme settings
        currentThemeIndex = extractInt("currentThemeIndex", 0);
        
        // Update UI to reflect loaded settings
        updateLabels(app);
        
        // Update menu checkboxes to match loaded settings
        gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(app->soundToggleMenuItem), 
                                      app->board->sound_enabled_);
        
        gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(app->backgroundToggleMenuItem), 
                                      app->board->isUsingBackgroundImage());
        
        // Set the correct difficulty radio button
        switch(app->difficulty) {
            case 0:
                gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(app->zenMenuItem), TRUE);
                break;
            case 1:
                gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(app->easyMenuItem), TRUE);
                break;
            case 2:
                gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(app->mediumMenuItem), TRUE);
                break;
            case 3:
                gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(app->hardMenuItem), TRUE);
                break;
            case 4:
                gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(app->extremeMenuItem), TRUE);
                break;
            case 5:
                gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(app->insaneMenuItem), TRUE);
                break;
        }
        
        // Set track menu items
        for (int i = 0; i < 5; i++) {
            gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(app->trackMenuItems[i]), 
                                          app->board->enabledTracks[i]);
        }
        
        return true;
    } catch (const std::exception& e) {
        std::cerr << "Error loading settings: " << e.what() << std::endl;
        return false;
    }
}

/**
 * Resets all settings to their default values
 * @param app Pointer to the TetrimoneApp struct
 */
void resetGameSettings(TetrimoneApp* app) {
    // Basic game settings
    app->board->retroModeActive = false;
    app->board->simpleBlocksActive = false;
    app->board->retroMusicActive = false;
    app->board->setShowGridLines(false);
    app->board->setGhostPieceEnabled(true);
    app->board->sound_enabled_ = true;
    app->difficulty = 2; // Medium difficulty
    
    // Background image settings
    app->board->setUseBackgroundImage(true);
    app->board->setBackgroundOpacity(0.3);
    app->board->setUseBackgroundZip(true);
    
    // Game board settings
    GRID_WIDTH = 10; // Default width
    GRID_HEIGHT = 22; // Default height
    
    // Music track settings
    app->backgroundMusicPlaying = true;
    for (int i = 0; i < 5; i++) {
        app->board->enabledTracks[i] = true;
    }
    
    // Advanced settings
    app->board->junkLinesPercentage = 0;
    app->board->junkLinesPerLevel = 0;
    app->board->initialLevel = 1;
    
    // Joystick settings
    app->joystickEnabled = true;
    app->joystickMapping.rotate_cw_button = 0;
    app->joystickMapping.rotate_ccw_button = 1;
    app->joystickMapping.hard_drop_button = 2;
    app->joystickMapping.pause_button = 3;
    app->joystickMapping.x_axis = 0;
    app->joystickMapping.y_axis = 1;
    app->joystickMapping.invert_x = false;
    app->joystickMapping.invert_y = false;
    
    // Visual theme settings
    currentThemeIndex = 0;
    
    // Update UI to reflect reset settings
    updateLabels(app);
    
    // Update menu checkboxes to match reset settings
    gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(app->soundToggleMenuItem), true);
    gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(app->backgroundToggleMenuItem), false);
    gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(app->mediumMenuItem), TRUE);
    
    // Set track menu items all to enabled
    for (int i = 0; i < 5; i++) {
        gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(app->trackMenuItems[i]), true);
    }
    
    // Save the reset settings
    saveGameSettings(app);
    
    // Rebuild the UI with the new settings
    rebuildGameUI(app);
}

void onResetSettings(GtkMenuItem* menuItem, gpointer userData) {
    TetrimoneApp* app = static_cast<TetrimoneApp*>(userData);
    
    // Create confirmation dialog
    GtkWidget* dialog = gtk_message_dialog_new(
        GTK_WINDOW(app->window),
        GTK_DIALOG_MODAL,
        GTK_MESSAGE_QUESTION,
        GTK_BUTTONS_YES_NO,
        "Are you sure you want to reset all settings to defaults?"
    );
    
    gtk_window_set_title(GTK_WINDOW(dialog), "Reset Settings");
    
    gint result = gtk_dialog_run(GTK_DIALOG(dialog));
    gtk_widget_destroy(dialog);
    
    if (result == GTK_RESPONSE_YES) {
        // First, delete the existing settings file to avoid any parsing issues
        std::string settingsPath = getSettingsFilePath();
        if (std::remove(settingsPath.c_str()) != 0) {
            g_print("Warning: Could not delete old settings file\n");
        }
        
        // Reset all settings to defaults in memory
        
        // Basic game settings
        app->board->retroModeActive = false;
        app->board->simpleBlocksActive = false;
        app->board->retroMusicActive = false;
        app->board->setShowGridLines(false);
        app->board->setGhostPieceEnabled(true);
        app->board->sound_enabled_ = true;
        app->difficulty = 2; // Medium difficulty
        
        // Background image settings
        app->board->setUseBackgroundImage(false);
        app->board->setBackgroundOpacity(0.3);
        app->board->setUseBackgroundZip(true);
        app->board->backgroundZipPath = "background.zip";
        app->board->startBackgroundTransition();
        // Game board settings
        GRID_WIDTH = 10; // Default width
        GRID_HEIGHT = 22; // Default height
        calculateBlockSize(app);
        
        // Music track settings
        app->backgroundMusicPlaying = true;
        for (int i = 0; i < 5; i++) {
            app->board->enabledTracks[i] = true;
        }
        
        // Advanced settings
        app->board->junkLinesPercentage = 0;
        app->board->junkLinesPerLevel = 0;
        app->board->initialLevel = 1;
        
        // Joystick settings
        app->joystickEnabled = true;
        app->joystickMapping.rotate_cw_button = 0;
        app->joystickMapping.rotate_ccw_button = 1;
        app->joystickMapping.hard_drop_button = 2;
        app->joystickMapping.pause_button = 3;
        app->joystickMapping.x_axis = 0;
        app->joystickMapping.y_axis = 1;
        app->joystickMapping.invert_x = false;
        app->joystickMapping.invert_y = false;
        
        // Visual theme settings
        currentThemeIndex = 0;
        
        // Update UI to reflect reset settings
        
        // Update menu checkboxes to match reset settings
        gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(app->soundToggleMenuItem), true);
        gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(app->backgroundToggleMenuItem), false);
        
        // Set the correct difficulty radio button
        gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(app->zenMenuItem), FALSE);
        gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(app->easyMenuItem), FALSE);
        gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(app->mediumMenuItem), TRUE);
        gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(app->hardMenuItem), FALSE);
        gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(app->extremeMenuItem), FALSE);
        gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(app->insaneMenuItem), FALSE);
        
        // Set track menu items all to enabled
        for (int i = 0; i < 5; i++) {
            gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(app->trackMenuItems[i]), true);
        }
        
        // Update UI labels
        updateLabels(app);
        
        // Rebuild the game UI to reflect changes
        rebuildGameUI(app);
        
        // Display confirmation message
        GtkWidget* confirmDialog = gtk_message_dialog_new(
            GTK_WINDOW(app->window),
            GTK_DIALOG_MODAL,
            GTK_MESSAGE_INFO,
            GTK_BUTTONS_OK,
            "All settings have been reset to defaults."
        );
        
        gtk_window_set_title(GTK_WINDOW(confirmDialog), "Settings Reset");
        gtk_dialog_run(GTK_DIALOG(confirmDialog));
        gtk_widget_destroy(confirmDialog);
        
        // Debug output to verify changes
        g_print("Settings reset to defaults. MinBlockSize: %d\n", app->board->getMinBlockSize());
        
        // Force a redraw of the game area
        if (app->gameArea) {
            gtk_widget_queue_draw(app->gameArea);
        }
        if (app->nextPieceArea) {
            gtk_widget_queue_draw(app->nextPieceArea);
        }
    }
}

#endif // TETRIMONE_SETTINGS_H
