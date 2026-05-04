#include "tetrimone.h"
#include "audiomanager.h"
#include <iostream>
#include <string>
#include <algorithm>
#ifdef _WIN32
#include <windows.h>
#include <commdlg.h>
#endif
#include "highscores.h"
#include "propaganda_messages.h"

void showIdeologicalFailureDialog(TetrimoneApp* app) {
    // Only show in retro mode
    if (!app->board->retroModeActive) {
        return;
    }
    
    // Create dialog
    GtkWidget* dialog = gtk_dialog_new_with_buttons(
        "ОБЪЯСНЕНИЕ ИДЕОЛОГИЧЕСКОГО ПРОВАЛА", 
        GTK_WINDOW(app->window), 
        GTK_DIALOG_MODAL,
        "_Принять наказание", GTK_RESPONSE_OK,
        NULL);
    
    // Set size
    gtk_window_set_default_size(GTK_WINDOW(dialog), 500, 400);
    
    // Get content area
    GtkWidget* contentArea = gtk_dialog_get_content_area(GTK_DIALOG(dialog));
    gtk_container_set_border_width(GTK_CONTAINER(contentArea), 15);
    
    // Create vbox for content
    GtkWidget* vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 10);
    gtk_container_add(GTK_CONTAINER(contentArea), vbox);
    
    // Add explanatory text
    GtkWidget* headerLabel = gtk_label_new(NULL);
    gtk_label_set_markup(GTK_LABEL(headerLabel), 
        "<span size='large' weight='bold' foreground='red'>"
        "ВНИМАНИЕ, ГРАЖДАНИН!\n"
        "ВАШ ИДЕОЛОГИЧЕСКИЙ ПРОВАЛ ТРЕБУЕТ ОБЪЯСНЕНИЯ</span>");
    gtk_box_pack_start(GTK_BOX(vbox), headerLabel, FALSE, FALSE, 0);
    
    // Add sub-heading
    GtkWidget* subheadLabel = gtk_label_new(
        "Укажите основную причину вашего антиреволюционного поведения:\n"
        "(Indicate the main reason for your counter-revolutionary behavior:)");
    gtk_box_pack_start(GTK_BOX(vbox), subheadLabel, FALSE, FALSE, 10);
    
    // Create radio buttons for ideological failings
    GtkWidget* frame = gtk_frame_new("Самокритика (Self-Criticism)");
    gtk_box_pack_start(GTK_BOX(vbox), frame, TRUE, TRUE, 0);
    
    GtkWidget* failureBox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 5);
    gtk_container_add(GTK_CONTAINER(frame), failureBox);
    gtk_container_set_border_width(GTK_CONTAINER(failureBox), 10);
    
    // Create radio button group
    GSList* group = NULL;
    
    // List of "ideological failings" options
    const char* failureOptions[] = {
        "Недостаточная преданность Партии (Insufficient Party loyalty)",
        "Буржуазные наклонности к неэффективности (Bourgeois tendencies toward inefficiency)",
        "Западный шпионаж повлиял на мои движения (Western espionage influenced my movements)",
        "Слишком много времени тратил на чтение непартийной литературы (Too much time spent reading non-Party literature)",
        "Идеологическая диверсия со стороны капиталистических блоков (Ideological subversion from capitalist blocks)",
        "Недостаточное потребление пропаганды в свободное время (Insufficient consumption of propaganda during free time)",
        "Контрреволюционное мышление (Counter-revolutionary thinking)"
    };
    
    // Create a radio button for each option
    for (int i = 0; i < 7; i++) {
        GtkWidget* radioButton = gtk_radio_button_new_with_label(group, failureOptions[i]);
        group = gtk_radio_button_get_group(GTK_RADIO_BUTTON(radioButton));
        gtk_box_pack_start(GTK_BOX(failureBox), radioButton, FALSE, FALSE, 0);
        
        // Select first option by default
        if (i == 0) {
            gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(radioButton), TRUE);
        }
    }
    
    // Add warning text at bottom
    GtkWidget* warningLabel = gtk_label_new(NULL);
    gtk_label_set_markup(GTK_LABEL(warningLabel), 
        "<span style='italic' foreground='red'>"
        "Внимание: Ваш ответ будет записан в ваше личное дело\n"
        "(Warning: Your answer will be recorded in your personal file)</span>");
    gtk_box_pack_start(GTK_BOX(vbox), warningLabel, FALSE, FALSE, 10);
    
    // Show all widgets
    gtk_widget_show_all(dialog);
    
    // Run the dialog
    gtk_dialog_run(GTK_DIALOG(dialog));
    
    // Clean up
    gtk_widget_destroy(dialog);
    
    // The game will restart after this dialog
    onRestartGame(NULL, app);
}

