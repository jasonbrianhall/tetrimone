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

cairo_surface_t* cairo_image_surface_create_from_jpeg(const char* filename) {
    // Read the file into memory first
    GFile* file = g_file_new_for_path(filename);
    GError* error = NULL;
    
    // Get file content as bytes
    GBytes* bytes = NULL;
    GFileInputStream* stream = g_file_read(file, NULL, &error);
    
    if (!stream) {
        if (error) {
            std::cerr << "Failed to open JPEG file: " << error->message << std::endl;
            g_error_free(error);
        }
        g_object_unref(file);
        return NULL;
    }
    
    // Load file content into memory
    error = NULL;
    bytes = g_input_stream_read_bytes(G_INPUT_STREAM(stream), 10 * 1024 * 1024, NULL, &error); // 10MB max
    
    if (!bytes) {
        if (error) {
            std::cerr << "Failed to read JPEG file: " << error->message << std::endl;
            g_error_free(error);
        }
        g_object_unref(stream);
        g_object_unref(file);
        return NULL;
    }
    
    // Get the data and size
    gsize size;
    const void* data = g_bytes_get_data(bytes, &size);
    
    // Use the existing from_memory function that works with ZIP files
    cairo_surface_t* surface = cairo_image_surface_create_from_memory(data, size);
    
    // Clean up
    g_bytes_unref(bytes);
    g_object_unref(stream);
    g_object_unref(file);
    
    return surface;
}

cairo_surface_t* cairo_image_surface_create_from_memory(const void* data, size_t length) {
    GError* error = NULL;
    
    // Create a memory input stream from the data
    GInputStream* stream = g_memory_input_stream_new_from_data(data, length, NULL);
    if (!stream) {
        std::cerr << "Failed to create memory stream" << std::endl;
        return NULL;
    }
    
    // Load the image using GdkPixbuf
    GdkPixbuf* pixbuf = gdk_pixbuf_new_from_stream(stream, NULL, &error);
    g_object_unref(stream);
    
    if (!pixbuf) {
        if (error) {
            std::cerr << "Failed to load image from memory: " << error->message << std::endl;
            g_error_free(error);
        }
        return NULL;
    }
    
    // Create a cairo surface of the same size
    cairo_surface_t* surface = cairo_image_surface_create(
        CAIRO_FORMAT_ARGB32,
        gdk_pixbuf_get_width(pixbuf),
        gdk_pixbuf_get_height(pixbuf)
    );
    
    // Create a cairo context
    cairo_t* cr = cairo_create(surface);
    
    // Draw the pixbuf onto the surface
    gdk_cairo_set_source_pixbuf(cr, pixbuf, 0, 0);
    cairo_paint(cr);
    
    // Clean up
    cairo_destroy(cr);
    g_object_unref(pixbuf);
    
    return surface;
}

bool TetrimoneBoard::loadBackgroundImage(const std::string& imagePath) {
    // Clean up previous image if it exists
    if (backgroundImage != nullptr) {
        cairo_surface_destroy(backgroundImage);
        backgroundImage = nullptr;
    }
    
    // Get file extension
    std::string extension = imagePath.substr(imagePath.find_last_of(".") + 1);
    std::transform(extension.begin(), extension.end(), extension.begin(), ::tolower);
    
    // Log the attempt to load
    std::cout << "Attempting to load background image: " << imagePath 
              << " (type: " << extension << ")" << std::endl;
    
    if (extension == "jpg" || extension == "jpeg") {
        // Load JPEG file
        backgroundImage = cairo_image_surface_create_from_jpeg(imagePath.c_str());
    } else {
        // Load PNG file (or attempt to) using existing method
        backgroundImage = cairo_image_surface_create_from_png(imagePath.c_str());
    }
    
    // Check if image loaded successfully
    if (backgroundImage == nullptr) {
        std::cerr << "Failed to load background image (null pointer): " << imagePath << std::endl;
        return false;
    }
    
    cairo_status_t status = cairo_surface_status(backgroundImage);
    if (status != CAIRO_STATUS_SUCCESS) {
        std::cerr << "Failed to load background image (status error): " << imagePath 
                  << " - " << cairo_status_to_string(status) << std::endl;
        
        cairo_surface_destroy(backgroundImage);
        backgroundImage = nullptr;
        return false;
    }
    
    // Log successful loading
    std::cout << "Successfully loaded background image: " << imagePath 
              << " (" << cairo_image_surface_get_width(backgroundImage) << "x" 
              << cairo_image_surface_get_height(backgroundImage) << ")" << std::endl;
    
    // Store the path and set flag
    backgroundImagePath = imagePath;
    useBackgroundImage = true;
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
    ofn.lpstrFilter = "Image Files\0*.png;*.jpg;*.jpeg\0PNG Images\0*.png\0JPEG Images\0*.jpg;*.jpeg\0All Files\0*.*\0";
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
    
    // Add filter for image files
    GtkFileFilter* filterAll = gtk_file_filter_new();
    gtk_file_filter_set_name(filterAll, "All Image Files");
    gtk_file_filter_add_mime_type(filterAll, "image/png");
    gtk_file_filter_add_mime_type(filterAll, "image/jpeg");
    gtk_file_chooser_add_filter(GTK_FILE_CHOOSER(dialog), filterAll);
    
    // Add filter for PNG files
    GtkFileFilter* filterPng = gtk_file_filter_new();
    gtk_file_filter_set_name(filterPng, "PNG Images");
    gtk_file_filter_add_pattern(filterPng, "*.png");
    gtk_file_filter_add_mime_type(filterPng, "image/png");
    gtk_file_chooser_add_filter(GTK_FILE_CHOOSER(dialog), filterPng);
    
    // Add filter for JPEG files
    GtkFileFilter* filterJpeg = gtk_file_filter_new();
    gtk_file_filter_set_name(filterJpeg, "JPEG Images");
    gtk_file_filter_add_pattern(filterJpeg, "*.jpg");
    gtk_file_filter_add_pattern(filterJpeg, "*.jpeg");
    gtk_file_filter_add_mime_type(filterJpeg, "image/jpeg");
    gtk_file_chooser_add_filter(GTK_FILE_CHOOSER(dialog), filterJpeg);
    
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
        
        g_slist_free(filenames);
    }
    
    gtk_widget_destroy(dialog);
#endif
    
    // Process the selected file paths
    if (!filePaths.empty()) {
        // Clean up existing background images
        app->board->cleanupBackgroundImages();
        
        // Flag to track successful image loading
        bool imagesLoaded = false;
        
        // Process each selected file
        for (const auto& filepath : filePaths) {
            // Get file extension
            std::string extension = filepath.substr(filepath.find_last_of(".") + 1);
            std::transform(extension.begin(), extension.end(), extension.begin(), ::tolower);
            
            cairo_surface_t* surface = nullptr;
            
            if (extension == "jpg" || extension == "jpeg") {
                // Load JPEG
                surface = cairo_image_surface_create_from_jpeg(filepath.c_str());
            } else {
                // Default to PNG
                surface = cairo_image_surface_create_from_png(filepath.c_str());
            }
            
            // Check if the surface was created successfully
            if (surface && cairo_surface_status(surface) == CAIRO_STATUS_SUCCESS) {
                app->board->backgroundImages.push_back(surface);
                imagesLoaded = true;
            } else {
                std::cerr << "Failed to load image: " << filepath << std::endl;
                if (surface) cairo_surface_destroy(surface);
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
                "No valid image files could be loaded."
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
    
    // Count how many image files we found
    int imageCount = 0;
    
    // Process each file in the archive
    for (zip_int64_t i = 0; i < numEntries; i++) {
        // Get file info
        zip_stat_t stat;
        if (zip_stat_index(archive, i, 0, &stat) < 0) {
            std::cerr << "Failed to get file stats at index " << i << std::endl;
            continue;
        }
        
        // Check if it's an image file (PNG or JPEG)
        std::string filename = stat.name;
        std::string extension = "";
        size_t dotPos = filename.find_last_of('.');
        if (dotPos != std::string::npos) {
            extension = filename.substr(dotPos + 1);
            std::transform(extension.begin(), extension.end(), extension.begin(), 
                [](unsigned char c) { return std::tolower(c); });
        }
        
        if (extension != "png" && extension != "jpg" && extension != "jpeg") {
            continue;  // Skip non-image files
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
        
        cairo_surface_t* surface = nullptr;
        
        if (extension == "png") {
            // Use existing PNG loading from memory code
            struct PngReadData {
                const std::vector<uint8_t>* data;
                size_t offset;
            };
            
            PngReadData readData = { &fileData, 0 };
            
            surface = cairo_image_surface_create_from_png_stream(
                [](void* closure, unsigned char* data, unsigned int length) -> cairo_status_t {
                    PngReadData* readData = static_cast<PngReadData*>(closure);
                    
                    // Check if we've reached the end of our data
                    if (readData->offset >= readData->data->size()) {
                        return CAIRO_STATUS_READ_ERROR;
                    }
                    
                    // Calculate how much we can read
                    size_t remaining = readData->data->size() - readData->offset;
                    size_t toRead = (length < remaining) ? length : remaining;
                    
                    // Copy the data
                    memcpy(data, readData->data->data() + readData->offset, toRead);
                    readData->offset += toRead;
                    
                    return CAIRO_STATUS_SUCCESS;
                },
                &readData
            );
        } else if (extension == "jpg" || extension == "jpeg") {
            // Load JPEG from memory using GdkPixbuf
            surface = cairo_image_surface_create_from_memory(fileData.data(), fileData.size());
        }
        
        // Check if the surface was created successfully
        if (surface && cairo_surface_status(surface) == CAIRO_STATUS_SUCCESS) {
            // Add the surface to our collection
            backgroundImages.push_back(surface);
            imageCount++;
        } else {
            std::cerr << "Failed to load image from ZIP: " << filename << std::endl;
            if (surface) cairo_surface_destroy(surface);
        }
    }
    
    // Close the archive
    zip_close(archive);
    
    // Check if we loaded any images
    if (imageCount == 0) {
        std::cerr << "No valid image files found in ZIP archive" << std::endl;
        return false;
    }
    
    // Set the current background to a random one
    useBackgroundZip = true;
    useBackgroundImage = true;
    selectRandomBackground();
    
    std::cout << "Successfully loaded " << imageCount << " background images from ZIP" << std::endl;
    return true;
}

void onBackgroundOpacityDialog(GtkMenuItem *menuItem, gpointer userData) {
  TetrimoneApp *app = static_cast<TetrimoneApp *>(userData);

  // Create dialog
  GtkWidget *dialog = gtk_dialog_new_with_buttons(
      "Background Opacity", GTK_WINDOW(app->window), GTK_DIALOG_MODAL, "_OK",
      GTK_RESPONSE_OK, NULL);

  // Make it a reasonable size
  gtk_window_set_default_size(GTK_WINDOW(dialog), 300, 150);

  // Create content area
  GtkWidget *contentArea = gtk_dialog_get_content_area(GTK_DIALOG(dialog));
  gtk_container_set_border_width(GTK_CONTAINER(contentArea), 10);

  // Create a vertical box for content
  GtkWidget *vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 6);
  gtk_container_add(GTK_CONTAINER(contentArea), vbox);

  // Add a label
  GtkWidget *label = gtk_label_new("Adjust background opacity:");
  gtk_widget_set_halign(label, GTK_ALIGN_START);
  gtk_box_pack_start(GTK_BOX(vbox), label, FALSE, FALSE, 0);

  // Create a horizontal scale (slider)
  GtkWidget *scale =
      gtk_scale_new_with_range(GTK_ORIENTATION_HORIZONTAL, 0.0, 1.0, 0.05);
  gtk_range_set_value(GTK_RANGE(scale), app->board->getBackgroundOpacity());
  gtk_scale_set_digits(GTK_SCALE(scale), 2); // 2 decimal places
  gtk_scale_set_value_pos(GTK_SCALE(scale), GTK_POS_RIGHT);
  gtk_box_pack_start(GTK_BOX(vbox), scale, FALSE, FALSE, 0);

  // Add min/max labels
  GtkWidget *rangeBox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
  gtk_box_pack_start(GTK_BOX(vbox), rangeBox, FALSE, FALSE, 0);

  GtkWidget *minLabel = gtk_label_new("Transparent");
  gtk_widget_set_halign(minLabel, GTK_ALIGN_START);
  gtk_box_pack_start(GTK_BOX(rangeBox), minLabel, TRUE, TRUE, 0);

  GtkWidget *maxLabel = gtk_label_new("Opaque");
  gtk_widget_set_halign(maxLabel, GTK_ALIGN_END);
  gtk_box_pack_end(GTK_BOX(rangeBox), maxLabel, TRUE, TRUE, 0);

  // Connect value-changed signal to update the opacity in real-time
  g_signal_connect(G_OBJECT(scale), "value-changed",
                   G_CALLBACK(onOpacityValueChanged), app);

  // Show all dialog widgets
  gtk_widget_show_all(dialog);

  // Run the dialog
  gtk_dialog_run(GTK_DIALOG(dialog));

  // Destroy dialog
  gtk_widget_destroy(dialog);
}

void onOpacityValueChanged(GtkRange *range, gpointer userData) {
  TetrimoneApp *app = static_cast<TetrimoneApp *>(userData);

  // Update the opacity in the board
  double opacity = gtk_range_get_value(range);

  // Early return if the background image isn't valid
  if (!app->board->isUsingBackgroundImage() ||
      app->board->getBackgroundImage() == nullptr) {
    return;
  }

  // Check surface status before attempting to draw
  cairo_status_t status =
      cairo_surface_status(app->board->getBackgroundImage());
  if (status != CAIRO_STATUS_SUCCESS) {
    std::cerr << "Invalid background image surface during opacity change: "
              << cairo_status_to_string(status) << std::endl;
    return;
  }

  app->board->setBackgroundOpacity(opacity);

  // Queue a redraw rather than forcing immediate redraw
  gtk_widget_queue_draw(app->gameArea);
}
