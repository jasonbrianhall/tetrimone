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
    updateDisplay(app);
}
