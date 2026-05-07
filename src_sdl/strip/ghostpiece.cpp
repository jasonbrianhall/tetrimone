#ifdef GTK3
#include "tetrimone_gtk.h"
#endif

#ifdef QT5
#include "tetrimone_qt5.h"
#endif

int TetrimoneBoard::getGhostPieceY() const {
    if (!currentPiece || !ghostPieceEnabled) {
        return -1; // No current piece or ghost disabled
    }

    // Create a copy of the current piece to simulate dropping
    TetrimoneBlock ghostPiece(currentPiece->getType());
    ghostPiece.setPosition(currentPiece->getX(), currentPiece->getY());
    
    // Apply the same rotation as the current piece
    for (int i = 0; i < currentPiece->getRotation(); i++) {
        ghostPiece.rotate();
    }
    
    // Drop the ghost piece until it collides
    int yPos = currentPiece->getY();
    while (!checkCollision(ghostPiece)) {
        yPos++;
        ghostPiece.setPosition(currentPiece->getX(), yPos);
    }
    
    // Move back to the last valid position
    return yPos - 1;
}

// ============================================================================
// Framework-Specific Ghost Piece Toggle
// ============================================================================

#ifdef GTK3

void onGhostPieceToggled(GtkCheckMenuItem* menuItem, gpointer userData) {
    TetrimoneApp* app = static_cast<TetrimoneApp*>(userData);
    app->board->setGhostPieceEnabled(gtk_check_menu_item_get_active(menuItem));
    gtk_widget_queue_draw(app->gameArea);
}

#endif  // GTK3

#ifdef QT5

void onGhostPieceToggled(void* menuItem, void* userData) {
    TetrimoneApp* app = static_cast<TetrimoneApp*>(userData);
    // TODO: Implement Qt5 toggle handler
    // For now, just update the board setting
    // bool isChecked = ...; // Get from Qt5 widget
    // app->board->setGhostPieceEnabled(isChecked);
    // if (app->gameWidget) app->gameWidget->update();
}

#endif  // QT5
