#include "tetrimone.h"
#include <iostream>
#include <string>
#include "zip.h"
#include <fstream>
#include <algorithm>
#ifdef _WIN32
#include <windows.h>
#include <commdlg.h>
#include <direct.h>
#endif

void TetrimoneBoard::cleanupBackgroundImages() {
    for (auto surface : backgroundImages) {
        if (surface != nullptr) {
            cairo_surface_destroy(surface);
        }
    }
    backgroundImages.clear();
}

void onBackgroundZipDialog(GtkMenuItem* menuItem, gpointer userData) {
    TetrimoneApp* app = static_cast<TetrimoneApp*>(userData);
    
    // Pause the game if it's running
    bool wasPaused = app->board->isPaused();
    if (!wasPaused && !app->board->isGameOver() && !app->board->isSplashScreenActive()) {
        onPauseGame(GTK_MENU_ITEM(app->pauseMenuItem), app);
    }
    
    std::string filePath;
    
#ifdef _WIN32
    // Use Windows native dialog
    OPENFILENAME ofn;
    char szFile[260] = {0};
    
    ZeroMemory(&ofn, sizeof(ofn));
    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner = NULL;  // Ideally get the HWND from GTK window
    ofn.lpstrFile = szFile;
    ofn.nMaxFile = sizeof(szFile);
    ofn.lpstrFilter = "ZIP Files\0*.zip\0All Files\0*.*\0";
    ofn.nFilterIndex = 1;
    ofn.lpstrFileTitle = NULL;
    ofn.nMaxFileTitle = 0;
    ofn.lpstrInitialDir = NULL;
    ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST;
    
    if (GetOpenFileName(&ofn)) {
        filePath = ofn.lpstrFile;
    }
#else
    // Use GTK dialog on other platforms
    GtkWidget* dialog = gtk_file_chooser_dialog_new(
        "Select Background Images ZIP File",
        GTK_WINDOW(app->window),
        GTK_FILE_CHOOSER_ACTION_OPEN,
        "_Cancel", GTK_RESPONSE_CANCEL,
        "_Open", GTK_RESPONSE_ACCEPT,
        NULL
    );
    
    // Add filter for ZIP files only
    GtkFileFilter* filter = gtk_file_filter_new();
    gtk_file_filter_set_name(filter, "ZIP Files");
    gtk_file_filter_add_pattern(filter, "*.zip");
    gtk_file_filter_add_mime_type(filter, "application/zip");
    gtk_file_chooser_add_filter(GTK_FILE_CHOOSER(dialog), filter);
    
    // Run the dialog
    if (gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_ACCEPT) {
        char* filename = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(dialog));
        filePath = filename;
        g_free(filename);
    }
    
    gtk_widget_destroy(dialog);
#endif
    
    // Process the selected file path
    if (!filePath.empty()) {
        if (app->board->loadBackgroundImagesFromZip(filePath)) {
            gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(app->backgroundToggleMenuItem), TRUE);
            
            // Process any pending events before showing opacity dialog
            while (gtk_events_pending())
                gtk_main_iteration();
                
            // Now show the opacity dialog
            onBackgroundOpacityDialog(NULL, app);
        } else {
            GtkWidget* errorDialog = gtk_message_dialog_new(
                GTK_WINDOW(app->window),
                GTK_DIALOG_MODAL,
                GTK_MESSAGE_ERROR,
                GTK_BUTTONS_OK,
                "Failed to load background images from ZIP: %s", filePath.c_str()
            );
            gtk_dialog_run(GTK_DIALOG(errorDialog));
            gtk_widget_destroy(errorDialog);
        }
    }
    
    // Redraw the game area
    gtk_widget_queue_draw(app->gameArea);
    
    // Resume the game if it wasn't paused before
    if (!wasPaused && !app->board->isGameOver() && !app->board->isSplashScreenActive()) {
        onPauseGame(GTK_MENU_ITEM(app->pauseMenuItem), app);
    }
}

bool TetrimoneBoard::loadBackgroundImagesFromZip(const std::string& zipPath) {
    // Clean up existing background images first
    cleanupBackgroundImages();
    
    // Store the ZIP path
    backgroundZipPath = zipPath;
    
    int errCode = 0;
    zip_t *archive = zip_open(zipPath.c_str(), 0, &errCode);
    
    if (!archive) {
        zip_error_t zipError;
        zip_error_init_with_code(&zipError, errCode);
        std::cerr << "Failed to open ZIP archive: " << zip_error_strerror(&zipError) << std::endl;
        zip_error_fini(&zipError);
        return false;
    }
    
    // Get the number of entries in the archive
    zip_int64_t numEntries = zip_get_num_entries(archive, 0);
    if (numEntries <= 0) {
        std::cerr << "No files found in ZIP archive" << std::endl;
        zip_close(archive);
        return false;
    }
    
    // Count how many PNG files we found
    int pngCount = 0;
    
#ifdef _WIN32
    // Windows-specific approach - extract to a user writable location
    char tempPath[MAX_PATH];
    DWORD pathLen = GetTempPath(MAX_PATH, tempPath);
    if (pathLen == 0 || pathLen > MAX_PATH) {
        std::cerr << "Failed to get temp path" << std::endl;
        zip_close(archive);
        return false;
    }
    
    // Create a temporary directory for our extracted files
    std::string extractDir = std::string(tempPath) + "tetris_backgrounds_" + std::to_string(GetTickCount());
    if (!CreateDirectory(extractDir.c_str(), NULL)) {
        DWORD error = GetLastError();
        if (error != ERROR_ALREADY_EXISTS) {
            std::cerr << "Failed to create temp directory (error " << error << ")" << std::endl;
            zip_close(archive);
            return false;
        }
    }
    
    // Add trailing slash
    if (extractDir.back() != '\\') {
        extractDir += '\\';
    }
    
    // Process each file in the archive
    for (zip_int64_t i = 0; i < numEntries; i++) {
        // Get file info
        zip_stat_t stat;
        if (zip_stat_index(archive, i, 0, &stat) < 0) {
            std::cerr << "Failed to get file stats at index " << i << std::endl;
            continue;
        }
        
        // Check if it's a PNG file
        std::string filename = stat.name;
        std::string extension = "";
        size_t dotPos = filename.find_last_of('.');
        if (dotPos != std::string::npos) {
            extension = filename.substr(dotPos + 1);
            std::transform(extension.begin(), extension.end(), extension.begin(), 
                [](unsigned char c) { return std::tolower(c); });
        }
        
        if (extension != "png") {
            continue;  // Skip non-PNG files
        }
        
        // Generate a unique filename
        std::string baseName = "bg_" + std::to_string(i) + ".png";
        std::string extractPath = extractDir + baseName;
        
        // Open the file in the archive
        zip_file_t *file = zip_fopen_index(archive, i, 0);
        if (!file) {
            std::cerr << "Failed to open file in ZIP archive: " << zip_strerror(archive) << std::endl;
            continue;
        }
        
        // Allocate memory for the file content
        std::vector<uint8_t> fileData(stat.size);
        
        // Read the file content
        zip_int64_t bytesRead = zip_fread(file, fileData.data(), stat.size);
        if (bytesRead < 0 || static_cast<zip_uint64_t>(bytesRead) != stat.size) {
            std::cerr << "Failed to read file: " << zip_file_strerror(file) << std::endl;
            zip_fclose(file);
            continue;
        }
        
        // Close the file
        zip_fclose(file);
        
        // Extract the PNG to the temporary directory
        std::ofstream outFile(extractPath, std::ios::binary);
        if (!outFile) {
            std::cerr << "Failed to create output file: " << extractPath << std::endl;
            continue;
        }
        
        outFile.write(reinterpret_cast<const char*>(fileData.data()), fileData.size());
        outFile.close();
        
        if (!outFile) {
            std::cerr << "Error writing to output file: " << extractPath << std::endl;
            continue;
        }
        
        // Convert path to UTF-8 for Cairo
        std::string utf8Path = extractPath;
        
        // Load the PNG with Cairo
        cairo_surface_t* surface = cairo_image_surface_create_from_png(utf8Path.c_str());
        
        // Check if the surface was created successfully
        if (cairo_surface_status(surface) != CAIRO_STATUS_SUCCESS) {
            std::cerr << "Failed to load PNG: " << cairo_status_to_string(cairo_surface_status(surface)) << std::endl;
            cairo_surface_destroy(surface);
            DeleteFile(utf8Path.c_str());
            continue;
        }
        
        // Add the surface to our collection
        backgroundImages.push_back(surface);
        pngCount++;
        
        // Delete the temporary file (Cairo has loaded it into memory now)
        DeleteFile(utf8Path.c_str());
    }
    
    // Try to remove the temporary directory
    RemoveDirectory(extractDir.c_str());
    
#else
    // Original Linux approach
    for (zip_int64_t i = 0; i < numEntries; i++) {
        // Get file info
        zip_stat_t stat;
        if (zip_stat_index(archive, i, 0, &stat) < 0) {
            std::cerr << "Failed to get file stats at index " << i << std::endl;
            continue;
        }
        
        // Check if it's a PNG file
        std::string filename = stat.name;
        std::string extension = "";
        size_t dotPos = filename.find_last_of('.');
        if (dotPos != std::string::npos) {
            extension = filename.substr(dotPos + 1);
            std::transform(extension.begin(), extension.end(), extension.begin(), 
                [](unsigned char c) { return std::tolower(c); });
        }
        
        if (extension != "png") {
            continue;  // Skip non-PNG files
        }
        
        // Open the file in the archive
        zip_file_t *file = zip_fopen_index(archive, i, 0);
        if (!file) {
            std::cerr << "Failed to open file in ZIP archive: " << zip_strerror(archive) << std::endl;
            continue;
        }
        
        // Allocate memory for the file content
        std::vector<uint8_t> fileData(stat.size);
        
        // Read the file content
        zip_int64_t bytesRead = zip_fread(file, fileData.data(), stat.size);
        if (bytesRead < 0 || static_cast<zip_uint64_t>(bytesRead) != stat.size) {
            std::cerr << "Failed to read file: " << zip_file_strerror(file) << std::endl;
            zip_fclose(file);
            continue;
        }
        
        // Close the file
        zip_fclose(file);
        
        // Create a temporary file for Cairo to read
        char tempFilename[L_tmpnam];
        tmpnam(tempFilename);
        std::string tempPath = std::string(tempFilename) + ".png";
        
        // Write the data to the temporary file
        std::ofstream tempFile(tempPath, std::ios::binary);
        if (!tempFile) {
            std::cerr << "Failed to create temporary file for PNG" << std::endl;
            continue;
        }
        
        tempFile.write(reinterpret_cast<const char*>(fileData.data()), fileData.size());
        tempFile.close();
        
        // Load the PNG with Cairo
        cairo_surface_t* surface = cairo_image_surface_create_from_png(tempPath.c_str());
        
        // Check if the surface was created successfully
        if (cairo_surface_status(surface) != CAIRO_STATUS_SUCCESS) {
            std::cerr << "Failed to load PNG: " << cairo_status_to_string(cairo_surface_status(surface)) << std::endl;
            cairo_surface_destroy(surface);
            remove(tempPath.c_str());  // Delete the temporary file
            continue;
        }
        
        // Add the surface to our collection
        backgroundImages.push_back(surface);
        pngCount++;
        
        // Delete the temporary file
        remove(tempPath.c_str());
    }
#endif
    
    // Close the archive
    zip_close(archive);
    
    // Check if we loaded any PNGs
    if (pngCount == 0) {
        std::cerr << "No valid PNG files found in ZIP archive" << std::endl;
        return false;
    }
    
    // Set the current background to a random one
    useBackgroundZip = true;
    useBackgroundImage = true;
    selectRandomBackground();
    
    std::cout << "Successfully loaded " << pngCount << " background images from ZIP" << std::endl;
    return true;
}

void TetrimoneBoard::selectRandomBackground() {
    if (backgroundImages.empty()) {
        return;
    }
    
    // Generate a random index
    std::uniform_int_distribution<int> dist(0, backgroundImages.size() - 1);
    currentBackgroundIndex = dist(rng);
    
    // Update the current background image
    if (backgroundImage != nullptr) {
        cairo_surface_destroy(backgroundImage);
    }
    
    // Clone the selected surface to avoid double-free issues
    cairo_surface_t* selectedSurface = backgroundImages[currentBackgroundIndex];
    int width = cairo_image_surface_get_width(selectedSurface);
    int height = cairo_image_surface_get_height(selectedSurface);
    
    // Create a new surface and copy the data
    backgroundImage = cairo_image_surface_create(
        cairo_image_surface_get_format(selectedSurface),
        width, height);
    
    // Copy the surface data
    cairo_t* cr = cairo_create(backgroundImage);
    cairo_set_source_surface(cr, selectedSurface, 0, 0);
    cairo_paint(cr);
    cairo_destroy(cr);
    
    // Set the single-image mode to use our new image
    useBackgroundImage = true;
}

// Update the background toggle handler to handle ZIP mode
void onBackgroundToggled(GtkCheckMenuItem* menuItem, gpointer userData) {
    TetrimoneApp* app = static_cast<TetrimoneApp*>(userData);
    bool useBackground = gtk_check_menu_item_get_active(menuItem);
    
    // The toggle should control visibility, regardless of background mode
    app->board->setUseBackgroundImage(useBackground);
    
    // Also update ZIP mode flag to match if using backgrounds from ZIP
    if (app->board->isUsingBackgroundZip()) {
        app->board->setUseBackgroundZip(useBackground);
    }
    
    // Redraw the game area
    gtk_widget_queue_draw(app->gameArea);
}

void onBackgroundImageDialog(GtkMenuItem* menuItem, gpointer userData) {
    TetrimoneApp* app = static_cast<TetrimoneApp*>(userData);
    
    // Pause the game if it's running
    bool wasPaused = app->board->isPaused();
    if (!wasPaused && !app->board->isGameOver() && !app->board->isSplashScreenActive()) {
        onPauseGame(GTK_MENU_ITEM(app->pauseMenuItem), app);
    }
    
    std::vector<std::string> filePaths;
    
#ifdef _WIN32
    // Use Windows native dialog with multi-select support
    OPENFILENAME ofn;
    char szFile[4096] = {0};  // Increased buffer size to support multiple files
    
    ZeroMemory(&ofn, sizeof(ofn));
    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner = NULL;  // Ideally get the HWND from GTK window
    ofn.lpstrFile = szFile;
    ofn.nMaxFile = sizeof(szFile);
    ofn.lpstrFilter = "PNG Images\0*.png\0All Files\0*.*\0";
    ofn.nFilterIndex = 1;
    ofn.lpstrFileTitle = NULL;
    ofn.nMaxFileTitle = 0;
    ofn.lpstrInitialDir = NULL;
    ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST | OFN_ALLOWMULTISELECT | OFN_EXPLORER;
    
    if (GetOpenFileName(&ofn)) {
        // Parse multiple file selection
        char* filePtr = ofn.lpstrFile;
        
        // First string is the directory
        std::string directory(filePtr);
        filePtr += directory.length() + 1;
        
        // If no files selected after directory, it means only one file was chosen
        if (*filePtr == '\0') {
            filePaths.push_back(directory);
        } else {
            // Multiple files selected
            while (*filePtr != '\0') {
                std::string filename(filePtr);
                std::string fullPath = directory + "\\" + filename;
                filePaths.push_back(fullPath);
                
                // Move to next filename
                filePtr += filename.length() + 1;
            }
        }
    }
#else
    // Use GTK dialog on other platforms
    GtkWidget* dialog = gtk_file_chooser_dialog_new(
        "Select Background Images",
        GTK_WINDOW(app->window),
        GTK_FILE_CHOOSER_ACTION_OPEN,
        "_Cancel", GTK_RESPONSE_CANCEL,
        "_Open", GTK_RESPONSE_ACCEPT,
        NULL
    );
    
    // Allow multiple file selection
    gtk_file_chooser_set_select_multiple(GTK_FILE_CHOOSER(dialog), TRUE);
    
    // Add filter for PNG files only
    GtkFileFilter* filter = gtk_file_filter_new();
    gtk_file_filter_set_name(filter, "PNG Images");
    gtk_file_filter_add_pattern(filter, "*.png");
    gtk_file_filter_add_mime_type(filter, "image/png");
    gtk_file_chooser_add_filter(GTK_FILE_CHOOSER(dialog), filter);
    
    // Run the dialog
    if (gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_ACCEPT) {
        // Get the selected filenames
        GSList* filenames = gtk_file_chooser_get_filenames(GTK_FILE_CHOOSER(dialog));
        
        // Convert GSList to vector of strings
        for (GSList* list = filenames; list != NULL; list = list->next) {
            char* filename = static_cast<char*>(list->data);
            filePaths.push_back(filename);
            g_free(filename);
        }
        
        // Free the list of filenames
        g_slist_free(filenames);
        
        // Destroy the dialog
        gtk_widget_destroy(dialog);
    } else {
        // User cancelled
        if (dialog) gtk_widget_destroy(dialog);
        return;
    }
#endif
    
    // Process the selected file paths
    if (!filePaths.empty()) {
        // Clean up existing background images
        app->board->cleanupBackgroundImages();
        
        // Flag to track successful image loading
        bool imagesLoaded = false;
        
        // Process each selected file
        for (const auto& filepath : filePaths) {
            // Load the PNG with Cairo
            cairo_surface_t* surface = cairo_image_surface_create_from_png(filepath.c_str());
            
            // Check if the surface was created successfully
            if (cairo_surface_status(surface) == CAIRO_STATUS_SUCCESS) {
                app->board->backgroundImages.push_back(surface);
                imagesLoaded = true;
            } else {
                std::cerr << "Failed to load PNG: " << filepath << std::endl;
                cairo_surface_destroy(surface);
            }
        }
        
        if (imagesLoaded) {
            // Set background modes
            app->board->useBackgroundZip = true;
            app->board->useBackgroundImage = true;
            
            // Select initial random background
            app->board->selectRandomBackground();
            
            // Activate background toggle
            gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(app->backgroundToggleMenuItem), TRUE);
            
            // Process any pending events before showing opacity dialog
            while (gtk_events_pending())
                gtk_main_iteration();
                
            // Show opacity dialog
            onBackgroundOpacityDialog(NULL, app);
        } else {
            // No images loaded
            GtkWidget* errorDialog = gtk_message_dialog_new(
                GTK_WINDOW(app->window),
                GTK_DIALOG_MODAL,
                GTK_MESSAGE_ERROR,
                GTK_BUTTONS_OK,
                "No valid PNG images could be loaded."
            );
            gtk_dialog_run(GTK_DIALOG(errorDialog));
            gtk_widget_destroy(errorDialog);
        }
    }
    
    // Redraw the game area
    gtk_widget_queue_draw(app->gameArea);
    
    // Resume the game if it wasn't paused before
    if (!wasPaused && !app->board->isGameOver() && !app->board->isSplashScreenActive()) {
        onPauseGame(GTK_MENU_ITEM(app->pauseMenuItem), app);
    }
}

void TetrimoneBoard::startBackgroundTransition() {
    if (!useBackgroundZip || backgroundImages.empty() || !useBackgroundImage) {
        return; // Only perform transitions when using background images from ZIP
    }
    
    // If already transitioning, cancel the current transition
    if (isTransitioning && transitionTimerId > 0) {
        g_source_remove(transitionTimerId);
        transitionTimerId = 0;
    }
    
    // Store the current background for the fade out effect
    if (oldBackground != nullptr) {
        cairo_surface_destroy(oldBackground);
    }
    
    // Clone the current background
    if (backgroundImage != nullptr) {
        int width = cairo_image_surface_get_width(backgroundImage);
        int height = cairo_image_surface_get_height(backgroundImage);
        
        oldBackground = cairo_image_surface_create(
            cairo_image_surface_get_format(backgroundImage),
            width, height);
        
        cairo_t* cr = cairo_create(oldBackground);
        cairo_set_source_surface(cr, backgroundImage, 0, 0);
        cairo_paint(cr);
        cairo_destroy(cr);
    }
    
    // Start with the current opacity
    transitionOpacity = backgroundOpacity;
    
    // Set up transition state
    isTransitioning = true;
    transitionDirection = -1; // Start by fading out
    
    // Select the next background but don't display it yet
    // We'll wait until fully faded out
    int oldIndex = currentBackgroundIndex;
    
    // Select a new random background that's different from the current one
    if (backgroundImages.size() > 1) {
        std::uniform_int_distribution<int> dist(0, backgroundImages.size() - 1);
        do {
            currentBackgroundIndex = dist(rng);
        } while (currentBackgroundIndex == oldIndex);
    }
    
    // Start the transition timer - update 20 times per second
    transitionTimerId = g_timeout_add(50, 
        [](gpointer data) -> gboolean {
            TetrimoneBoard* board = static_cast<TetrimoneBoard*>(data);
            board->updateBackgroundTransition();
            return TRUE; // Keep the timer running
        }, 
        this);
}

void TetrimoneBoard::updateBackgroundTransition() {
    if (!isTransitioning) {
        return;
    }
    
    // Update opacity based on direction
    const double TRANSITION_SPEED = 0.02; // Change this to adjust fade speed
    transitionOpacity += transitionDirection * TRANSITION_SPEED;
    
    // Check for direction change (from fade-out to fade-in)
    if (transitionDirection == -1 && transitionOpacity <= 0.0) {
        transitionOpacity = 0.0;
        transitionDirection = 1; // Change to fade in
        
        // Update the actual background image with the new selection
        if (backgroundImage != nullptr) {
            cairo_surface_destroy(backgroundImage);
        }
        
        // Create a new background from the selected image
        cairo_surface_t* selectedSurface = backgroundImages[currentBackgroundIndex];
        int width = cairo_image_surface_get_width(selectedSurface);
        int height = cairo_image_surface_get_height(selectedSurface);
        
        backgroundImage = cairo_image_surface_create(
            cairo_image_surface_get_format(selectedSurface),
            width, height);
        
        cairo_t* cr = cairo_create(backgroundImage);
        cairo_set_source_surface(cr, selectedSurface, 0, 0);
        cairo_paint(cr);
        cairo_destroy(cr);
    }
    
    // Check if transition is complete
    if (transitionDirection == 1 && transitionOpacity >= backgroundOpacity) {
        transitionOpacity = backgroundOpacity;
        isTransitioning = false;
        
        // Clean up the old background
        if (oldBackground != nullptr) {
            cairo_surface_destroy(oldBackground);
            oldBackground = nullptr;
        }
        
        // Clean up the timer
        if (transitionTimerId > 0) {
            g_source_remove(transitionTimerId);
            transitionTimerId = 0;
        }
    }
}

void TetrimoneBoard::cancelBackgroundTransition() {
    if (isTransitioning && transitionTimerId > 0) {
        g_source_remove(transitionTimerId);
        transitionTimerId = 0;
    }
    
    isTransitioning = false;
    
    // Clean up the old background
    if (oldBackground != nullptr) {
        cairo_surface_destroy(oldBackground);
        oldBackground = nullptr;
    }
}
