#include "tetrimone.h"

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

void onGhostPieceToggled(GtkCheckMenuItem* menuItem, gpointer userData) {
    TetrimoneApp* app = static_cast<TetrimoneApp*>(userData);
    app->board->setGhostPieceEnabled(gtk_check_menu_item_get_active(menuItem));
    gtk_widget_queue_draw(app->gameArea);
}

