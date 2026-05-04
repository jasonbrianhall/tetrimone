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
#include "freedom_messages.h"

void showPatrioticPerformanceDialog(TetrimoneApp* app) {
    // Only show in patriotic mode
    if (!app->board->patrioticModeActive) {
        return;
    }
    
    // Create dialog
    GtkWidget* dialog = gtk_dialog_new_with_buttons(
        "FREEDOM PERFORMANCE EVALUATION", 
        GTK_WINDOW(app->window), 
        GTK_DIALOG_MODAL,
        "_Accept Responsibility", GTK_RESPONSE_OK,
        NULL);
    
    // Set size
    gtk_window_set_default_size(GTK_WINDOW(dialog), 550, 450);
    
    // Get content area
    GtkWidget* contentArea = gtk_dialog_get_content_area(GTK_DIALOG(dialog));
    gtk_container_set_border_width(GTK_CONTAINER(contentArea), 15);
    
    // Create vbox for content
    GtkWidget* vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 10);
    gtk_container_add(GTK_CONTAINER(contentArea), vbox);
    
    // Add explanatory text
    GtkWidget* headerLabel = gtk_label_new(NULL);
    gtk_label_set_markup(GTK_LABEL(headerLabel), 
        "<span size='large' weight='bold' foreground='blue'>"
        "ATTENTION, CITIZEN!\n"
        "YOUR FREEDOM PERFORMANCE REQUIRES EVALUATION</span>");
    gtk_box_pack_start(GTK_BOX(vbox), headerLabel, FALSE, FALSE, 0);
    
    // Add sub-heading
    GtkWidget* subheadLabel = gtk_label_new(
        "Please indicate the primary reason for your un-American block performance:\n"
        "(Your response helps us serve you better!)");
    gtk_box_pack_start(GTK_BOX(vbox), subheadLabel, FALSE, FALSE, 10);
    
    // Create radio buttons for American-themed reasons
    GtkWidget* frame = gtk_frame_new("Personal Accountability Assessment");
    gtk_box_pack_start(GTK_BOX(vbox), frame, TRUE, TRUE, 0);
    
    GtkWidget* reasonBox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 5);
    gtk_container_add(GTK_CONTAINER(frame), reasonBox);
    gtk_container_set_border_width(GTK_CONTAINER(reasonBox), 10);
    
    // Create radio button group
    GSList* group = NULL;
    
    // List of "American performance issues" options
    const char* performanceOptions[] = {
        "🇺🇸 Insufficient consumption of freedom during gameplay",
        "🍔 Too much fast food affecting hand-eye coordination",
        "📺 Distracted by the latest Netflix series during play",
        "🏈 Still thinking about last night's football game",
        "💼 Work-life balance priorities shifted toward actual work",
        "🎬 Hollywood movies set unrealistic block-stacking expectations",
        "☕ Not enough coffee to maintain peak performance",
        "🚗 Traffic on the commute affected my gaming mindset",
        "📱 Social media notifications broke my concentration",
        "🛒 Worried about credit card bills instead of focusing on blocks",
        "🏠 HOA meeting stressed me out before playing",
        "🌮 Taco Tuesday made me hungry instead of focused"
    };
    
    // Create a radio button for each option
    for (int i = 0; i < 12; i++) {
        GtkWidget* radioButton = gtk_radio_button_new_with_label(group, performanceOptions[i]);
        group = gtk_radio_button_get_group(GTK_RADIO_BUTTON(radioButton));
        gtk_box_pack_start(GTK_BOX(reasonBox), radioButton, FALSE, FALSE, 0);
        
        // Select first option by default
        if (i == 0) {
            gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(radioButton), TRUE);
        }
    }
    
    // Add motivational text at bottom
    GtkWidget* motivationLabel = gtk_label_new(NULL);
    gtk_label_set_markup(GTK_LABEL(motivationLabel), 
        "<span style='italic' foreground='blue'>"
        "Remember: In America, failure is just another opportunity to succeed!\n"
        "Your data helps us optimize your gaming experience. Privacy policy available online.</span>");
    gtk_box_pack_start(GTK_BOX(vbox), motivationLabel, FALSE, FALSE, 10);
    
    // Add patriotic footer
    GtkWidget* footerLabel = gtk_label_new(NULL);
    gtk_label_set_markup(GTK_LABEL(footerLabel), 
        "<span weight='bold' foreground='red'>"
        "🦅 GOD BLESS AMERICA AND GOD BLESS YOUR BLOCKS! 🦅</span>");
    gtk_box_pack_start(GTK_BOX(vbox), footerLabel, FALSE, FALSE, 5);
    
    // Show all widgets
    gtk_widget_show_all(dialog);
    
    // Run the dialog
    gtk_dialog_run(GTK_DIALOG(dialog));
    
    // Clean up
    gtk_widget_destroy(dialog);
    
    // The game will restart after this dialog with renewed American spirit!
    onRestartGame(NULL, app);
}
