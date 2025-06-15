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
#include "smb.h"
#include <thread>
#include <memory>

// Add this global variable to track the thread
static std::unique_ptr<std::thread> smbThread = nullptr;

// Wrapper function to handle thread lifecycle
void startMainLoopThread() {
    // Check if thread is already running
    if (smbThread && smbThread->joinable()) {
        // Thread is already running, don't start another
        return;
    }
    
    // Start mainLoop in a new thread
    smbThread = std::make_unique<std::thread>([]() {
        // Initialize the SMB system in the new thread
        if (!initialize()) {
            std::cout << "Failed to initialize SMB system in thread\n";
            return;
        }
        
        // Run the main loop
        mainLoop();
        
        // Clean up when done
        shutdown();
    });
    
    // Detach the thread so it runs independently
    smbThread->detach();
}

static gboolean on_dialog_button_press(GtkWidget *widget, GdkEventButton *event, gpointer user_data) {
    // Check if it's a left click with Ctrl held down
    if (event->button == 1 && (event->state & GDK_CONTROL_MASK)) {
        // Store the Ctrl+click state
        gboolean *ctrl_clicked = (gboolean*)user_data;
        *ctrl_clicked = TRUE;
    }
    return FALSE; // Let the event propagate
}

void onAboutDialog(GtkMenuItem *menuItem, gpointer userData) {
    TetrimoneApp *app = static_cast<TetrimoneApp *>(userData);
    gboolean ctrl_clicked = FALSE; // Track if Ctrl was held during click

    // Check if in retro mode
    if (app->board->retroModeActive) {
        // Create a custom Soviet-style dialog
        GtkWidget *dialog = gtk_dialog_new_with_buttons(
            "Государственное Сообщение: Система Управления Блоками", 
            GTK_WINDOW(app->window), 
            GTK_DIALOG_MODAL, 
            "_Понял!", 
            GTK_RESPONSE_OK, 
            NULL);

        // Connect button press event to detect Ctrl+click
        g_signal_connect(dialog, "button-press-event", 
                        G_CALLBACK(on_dialog_button_press), &ctrl_clicked);

        // Get the content area
        GtkWidget *contentArea = gtk_dialog_get_content_area(GTK_DIALOG(dialog));
        gtk_container_set_border_width(GTK_CONTAINER(contentArea), 15);

        // Create a vertical box for layout
        GtkWidget *vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 10);
        gtk_container_add(GTK_CONTAINER(contentArea), vbox);

        // Add program name
        GtkWidget *nameLabel = gtk_label_new(NULL);
        gtk_label_set_markup(
            GTK_LABEL(nameLabel),
            "<span size='x-large' weight='bold'>БЛОЧНАЯ РЕВОЛЮЦИЯ</span>\n"
            "<span size='small'>(Block Revolution)</span>");
        gtk_box_pack_start(GTK_BOX(vbox), nameLabel, FALSE, FALSE, 5);

        // Add version (in classic Soviet style)
        GtkWidget *versionLabel = gtk_label_new("Официальный Выпуск: Производственный Цикл 1.0");
        gtk_box_pack_start(GTK_BOX(vbox), versionLabel, FALSE, FALSE, 0);

        // Add description with humorous Russian flair
        GtkWidget *descLabel = gtk_label_new(
            "Передовая Система Геометрической Оптимизации\n"
            "(Advanced Geometric Optimization System)\n\n"
            "★ Одобрено Центральным Комитетом Блочного Позиционирования ★\n"
            "(Approved by the Central Block Positioning Committee)\n\n"
            "Где нет блоков - там нет прогресса!\n"
            "(Where there are no blocks, there is no progress!)");
        gtk_box_pack_start(GTK_BOX(vbox), descLabel, FALSE, FALSE, 10);

        // Humorous Soviet-style achievements
        GtkWidget *achievementsLabel = gtk_label_new(NULL);
        gtk_label_set_markup(
            GTK_LABEL(achievementsLabel),
            "<span weight='bold'>ДОСТИЖЕНИЯ ГОСУДАРСТВЕННОЙ ВАЖНОСТИ:</span>\n"
            "• Максимальная эффективность падения блоков\n"
            "• Абсолютная точность геометрической трансформации\n"
            "• Непрерывность производственного процесса\n\n"
            "<i>Каждый падающий блок - удар по капиталистическому хаосу!</i>");
        gtk_box_pack_start(GTK_BOX(vbox), achievementsLabel, FALSE, FALSE, 5);

        // Add license info with Soviet humor
        GtkWidget *licenseLabel = gtk_label_new(NULL);
        gtk_label_set_markup(
            GTK_LABEL(licenseLabel),
            "Распространяется по протоколам Коллективного Программного Обеспечения\n"
            "<i>(Distributed under Collective Software Protocols)</i>");
        gtk_box_pack_start(GTK_BOX(vbox), licenseLabel, FALSE, FALSE, 5);

        // Add website button (in Soviet style)
        GtkWidget *websiteButton = gtk_link_button_new_with_label(
            "https://block-revolution.moscow.soviet", 
            "Государственный Информационный Портал\n(State Information Portal)");
        gtk_box_pack_start(GTK_BOX(vbox), websiteButton, FALSE, FALSE, 10);

        // Add copyright with Russian twist
        GtkWidget *copyrightLabel = gtk_label_new(
            "© Государственное Бюро Управления Блоками, Московское Отделение\n"
            "(State Block Management Bureau, Moscow Division)");
        gtk_box_pack_start(GTK_BOX(vbox), copyrightLabel, FALSE, FALSE, 5);

        // Add no warranty disclaimer (Soviet style)
        GtkWidget *disclaimerLabel = gtk_label_new(NULL);
        gtk_label_set_markup(
            GTK_LABEL(disclaimerLabel),
            "<span color='red'>ВНИМАНИЕ:</span> Производительность гарантирована\n"
            "высшим государственным авторитетом!\n"
            "<i>(Performance Guaranteed by Highest State Authority!)</i>\n\n"
            "Неудача - это всего лишь временное искажение\n"
            "коллективного потенциала!\n"
            "<i>(Failure is merely a temporary misalignment\n"
            "of collective potential!)</i>");
        gtk_box_pack_start(GTK_BOX(vbox), disclaimerLabel, FALSE, FALSE, 0);

        // Show all content
        gtk_widget_show_all(dialog);

        // Run the dialog
        gint response = gtk_dialog_run(GTK_DIALOG(dialog));

        // Check if OK was clicked with Ctrl held
        if (response == GTK_RESPONSE_OK) {
            printf("Main Loop was pressed\n");
            startMainLoopThread();
        }

        // Destroy the dialog when closed
        gtk_widget_destroy(dialog);
    } else {
        // Original about dialog code with same modification
        GtkWidget *dialog = gtk_dialog_new_with_buttons(
            "About GTK Tetrimone", GTK_WINDOW(app->window), GTK_DIALOG_MODAL, "_OK",
            GTK_RESPONSE_OK, NULL);

        // Connect button press event to detect Ctrl+click
        g_signal_connect(dialog, "button-press-event", 
                        G_CALLBACK(on_dialog_button_press), &ctrl_clicked);

        // Get the content area
        GtkWidget *contentArea = gtk_dialog_get_content_area(GTK_DIALOG(dialog));
        gtk_container_set_border_width(GTK_CONTAINER(contentArea), 15);

        // Create a vertical box for layout
        GtkWidget *vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 10);
        gtk_container_add(GTK_CONTAINER(contentArea), vbox);

        // Add an image (optional)
        GtkWidget *image =
            gtk_image_new_from_icon_name("applications-games", GTK_ICON_SIZE_DIALOG);
        gtk_box_pack_start(GTK_BOX(vbox), image, FALSE, FALSE, 0);

        // Add program name
        GtkWidget *nameLabel = gtk_label_new(NULL);
        gtk_label_set_markup(
            GTK_LABEL(nameLabel),
            "<span size='x-large' weight='bold'>GTK Tetrimone</span>");
        gtk_box_pack_start(GTK_BOX(vbox), nameLabel, FALSE, FALSE, 5);

        // Add version
        GtkWidget *versionLabel = gtk_label_new("Version 1.0");
        gtk_box_pack_start(GTK_BOX(vbox), versionLabel, FALSE, FALSE, 0);

        // Add description
        GtkWidget *descLabel = gtk_label_new(
            "A feature-rich falling block puzzle game with advanced graphics,\n"
            "multiple difficulty levels, theme progression, and comprehensive\n"
            "control options including joystick support.");
        gtk_box_pack_start(GTK_BOX(vbox), descLabel, FALSE, FALSE, 10);

        // Add license info
        GtkWidget *licenseLabel =
            gtk_label_new("This software is released under the MIT License.");
        gtk_box_pack_start(GTK_BOX(vbox), licenseLabel, FALSE, FALSE, 5);

        // Add acknowledgment heading
        GtkWidget *ackHeadingLabel = gtk_label_new(NULL);
        gtk_label_set_markup(GTK_LABEL(ackHeadingLabel),
                           "<span weight='bold'>Acknowledgments:</span>");
        gtk_widget_set_halign(ackHeadingLabel, GTK_ALIGN_START);
        gtk_box_pack_start(GTK_BOX(vbox), ackHeadingLabel, FALSE, FALSE, 5);

        // Add acknowledgment text
        GtkWidget *ackLabel =
            gtk_label_new("This game is inspired by classic falling block puzzle "
                        "games that originated in the 1980s.\n"
                        "Tetrimone is an independent creation and is not "
                        "affiliated with, endorsed by,\n"
                        "or connected to any commercial puzzle game publishers.");
        gtk_box_pack_start(GTK_BOX(vbox), ackLabel, FALSE, FALSE, 0);

        // Add website button
        GtkWidget *websiteButton = gtk_link_button_new_with_label(
            "https://github.com/jasonbrianhall/tetrimone", "Website");
        gtk_box_pack_start(GTK_BOX(vbox), websiteButton, FALSE, FALSE, 10);

        // Add copyright
        GtkWidget *copyrightLabel = gtk_label_new("© 2025 Jason Brian Hall");
        gtk_box_pack_start(GTK_BOX(vbox), copyrightLabel, FALSE, FALSE, 5);

        // Add no warranty disclaimer
        GtkWidget *disclaimerLabel =
            gtk_label_new("This program comes with absolutely no warranty.");
        gtk_box_pack_start(GTK_BOX(vbox), disclaimerLabel, FALSE, FALSE, 0);

        GtkWidget *licenseInfoLabel = gtk_label_new(NULL);
        gtk_label_set_markup(
            GTK_LABEL(licenseInfoLabel),
            "See the <a href='https://opensource.org/licenses/MIT'>MIT License</a> "
            "for details.");
        gtk_box_pack_start(GTK_BOX(vbox), licenseInfoLabel, FALSE, FALSE, 0);

        // Show all content
        gtk_widget_show_all(dialog);

        // Run the dialog
        gint response = gtk_dialog_run(GTK_DIALOG(dialog));

        // Check if OK was clicked with Ctrl held
        if (response == GTK_RESPONSE_OK) {
            printf("Main Loop was pressed\n");
            startMainLoopThread();
        }

        // Destroy the dialog when closed
        gtk_widget_destroy(dialog);
    }
}

void onInstructionsDialog(GtkMenuItem *menuItem, gpointer userData) {
  TetrimoneApp *app = static_cast<TetrimoneApp *>(userData);

  // Check if in retro mode
  if (app->board->retroModeActive) {
    // Create dialog with Soviet-style propaganda messaging
    GtkWidget *dialog = gtk_dialog_new_with_buttons(
        "ИНСТРУКЦИЯ ПО ПАДАЮЩИМ БЛОКАМ", 
        GTK_WINDOW(app->window), 
        GTK_DIALOG_MODAL, 
        "_Понял! (Understood!)", 
        GTK_RESPONSE_OK, 
        NULL);

    // Get the content area
    GtkWidget *contentArea = gtk_dialog_get_content_area(GTK_DIALOG(dialog));
    gtk_container_set_border_width(GTK_CONTAINER(contentArea), 15);

    // Create a scrolled window with specific size
    GtkWidget *scrolledWindow = gtk_scrolled_window_new(NULL, NULL);
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrolledWindow), 
                                   GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
    gtk_widget_set_size_request(scrolledWindow, 800, 600); // Large, explicit size

    // Create a text view
    GtkWidget *textView = gtk_text_view_new();
    gtk_text_view_set_editable(GTK_TEXT_VIEW(textView), FALSE);
    gtk_text_view_set_wrap_mode(GTK_TEXT_VIEW(textView), GTK_WRAP_WORD);
    
    // Set a monospace font to ensure better readability of Russian text
    PangoFontDescription *font_desc = pango_font_description_from_string("Monospace 10");
    gtk_widget_override_font(textView, font_desc);
    pango_font_description_free(font_desc);

    // Get the buffer and set the text
    GtkTextBuffer *buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(textView));
    gtk_text_buffer_set_text(buffer, 
        "СОВЕРШЕННО СЕКРЕТНО\n"
        "(TOP SECRET)\n\n"
        "ИНСТРУКЦИЯ № 1984/Б-БЛОК\n"
        "О ПРАВИЛАХ ГОСУДАРСТВЕННОГО ПЕРЕМЕЩЕНИЯ ГЕОМЕТРИЧЕСКИХ ЕДИНИЦ\n\n"
        "ВНИМАНИЕ, ТОВАРИЩ!\n"
        "Настоящая инструкция является ОБЯЗАТЕЛЬНОЙ к неукоснительному исполнению.\n"
        "Несоблюдение может привести к немедленной переквалификации.\n\n"
        "ОСНОВНЫЕ ДИРЕКТИВЫ УПРАВЛЕНИЯ БЛОКАМИ:\n\n"
        "1. ПЕРЕМЕЩЕНИЕ ВЛЕВО/ВПРАВО:\n"
        "   • Точное боковое перемещение СТРОГО по указанию партии\n"
        "   • Самовольное отклонение карается немедленным переселением в Сибирь\n\n"
        "2. ВРАЩЕНИЕ:\n"
        "   • Разрешено ТОЛЬКО по часовой стрелке\n"
        "   • Против часовой стрелки - признак идеологической диверсии!\n\n"
        "3. ВЕРТИКАЛЬНОЕ УСКОРЕНИЕ:\n"
        "   • Мягкое опускание: контролируемое падение\n"
        "   • Моментальное размещение: высшая форма блочной дисциплины\n\n"
        "СИСТЕМА ОЦЕНКИ ПРОИЗВОДИТЕЛЬНОСТИ:\n"
        "• Каждая заполненная линия - удар по капиталистическому хаосу!\n"
        "• Бонусные очки начисляются за НЕПРЕРЫВНОСТЬ и ДИСЦИПЛИНУ\n"
        "• Неэффективность приравнивается к САБОТАЖУ\n\n"
        "ИДЕОЛОГИЧЕСКИЕ ПРЕДУПРЕЖДЕНИЯ:\n"
        "★ ПОМНИ! Каждый ПАДАЮЩИЙ БЛОК СЛУЖИТ ВЕЛИКОМУ ДЕЛУ ПАРТИИ! ★\n\n"
        "ОСОБЫЕ УКАЗАНИЯ:\n"
        "• Красная линия: ЗОНА ГОСУДАРСТВЕННОЙ ОПАСНОСТИ\n"
        "• Выход за пределы линии равносилен ГОСУДАРСТВЕННОЙ ИЗМЕНЕ\n\n"
        "НАКАЗАНИЯ ЗА НЕЭФФЕКТИВНОСТЬ:\n"
        "1. Первое нарушение: Публичное порицание\n"
        "2. Второе нарушение: Принудительное перевоспитание\n"
        "3. Третье нарушение: IMMEDIATE VACATION TO SIBERIAN REDESIGN CAMP\n\n"
        "ПОМНИ, ТОВАРИЩ: \n"
        "В ИГРЕ, КАК И В ЖИЗНИ - ПАРТИЯ ВСЕГДА ПРАВА!\n\n"
        "Подпись: Начальник Управления Блочной Дисциплины\n"
        "Печать: СТРОГО СЕКРЕТНО\n\n"
        "P.S. Big Brother is watching your blocks!\n", -1);

    // Add text view to scrolled window
    gtk_container_add(GTK_CONTAINER(scrolledWindow), textView);

    // Add scrolled window to content area
    gtk_container_add(GTK_CONTAINER(contentArea), scrolledWindow);

    // Set window size
    gtk_window_set_default_size(GTK_WINDOW(dialog), 800, 600);

    // Show all widgets
    gtk_widget_show_all(dialog);

    // Run dialog
    gtk_dialog_run(GTK_DIALOG(dialog));

    // Destroy dialog when closed
    gtk_widget_destroy(dialog);
  } else {
    // Original instructions dialog code (similar modifications)
    GtkWidget *dialog = gtk_dialog_new_with_buttons(
        "Instructions", GTK_WINDOW(app->window), GTK_DIALOG_MODAL, "_OK",
        GTK_RESPONSE_OK, NULL);

    // Get the content area
    GtkWidget *contentArea = gtk_dialog_get_content_area(GTK_DIALOG(dialog));
    gtk_container_set_border_width(GTK_CONTAINER(contentArea), 15);

    // Create a scrolled window with specific size
    GtkWidget *scrolledWindow = gtk_scrolled_window_new(NULL, NULL);
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrolledWindow), 
                                   GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
    gtk_widget_set_size_request(scrolledWindow, 800, 600); // Large, explicit size

    // Create a text view
    GtkWidget *textView = gtk_text_view_new();
    gtk_text_view_set_editable(GTK_TEXT_VIEW(textView), FALSE);
    gtk_text_view_set_wrap_mode(GTK_TEXT_VIEW(textView), GTK_WRAP_WORD);

    // Set a monospace font for better readability
    PangoFontDescription *font_desc = pango_font_description_from_string("Monospace 10");
    gtk_widget_override_font(textView, font_desc);
    pango_font_description_free(font_desc);

    // Get the buffer and set the text
    GtkTextBuffer *buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(textView));
    gtk_text_buffer_set_text(buffer, 
        "Tetrimone Instructions:\n\n"
        "Goal: Arrange falling blocks to complete lines.\n\n"
        "Controls:\n"
        "• Left/Right Arrow or A/D: Move block left/right\n"
        "• Up Arrow or W: Rotate block clockwise\n"
        "• Z: Rotate block counter-clockwise\n"
        "• Down Arrow or S: Move block down (soft drop)\n"
        "• Space: Hard drop (instantly places block at bottom)\n"
        "• P: Pause/Resume game\n"
        "• R: Restart game when game over\n"
        "• N: New game (when paused)\n"
        "• Q: Quit game (when paused)\n\n"
        "Controller Support:\n"
        "• D-pad/Analog: Move piece\n"
        "• A/B buttons: Rotate piece\n"
        "• X button: Hard drop\n"
        "• Start: Pause/Resume\n"
        "• Custom mapping available in Options menu\n\n"
        "Scoring:\n"
        "• 1 line: 40 × level\n"
        "• 2 lines: 100 × level\n"
        "• 3 lines: 300 × level\n"
        "• 4 lines: 1200 × level\n"
        "• Sequence bonus: 10% extra per consecutive clear\n"
        "• Consistency bonus: 20% extra for repeating same line count\n"
        "• Hard drops: 2 points per cell\n\n"
        "Levels:\n"
        "• Every 10 lines cleared increases the level\n"
        "• Higher levels increase speed and points\n"
        "• Color themes change with level progression\n"
        "• Difficulty can be adjusted in Options menu\n\n"
        "Tips:\n"
        "• Keep the stack low and even\n"
        "• Save I-pieces for Tetrimone clears (4 lines)\n"
        "• Watch the preview for the next piece\n"
        "• Red line indicates the game over zone\n"
        "• Try to build sequences by clearing lines consecutively", -1);

    // Add text view to scrolled window
    gtk_container_add(GTK_CONTAINER(scrolledWindow), textView);

    // Add scrolled window to content area
    gtk_container_add(GTK_CONTAINER(contentArea), scrolledWindow);

    // Set window size
    gtk_window_set_default_size(GTK_WINDOW(dialog), 800, 600);

    // Show all widgets
    gtk_widget_show_all(dialog);

    // Run dialog
    gtk_dialog_run(GTK_DIALOG(dialog));

    // Destroy dialog when closed
    gtk_widget_destroy(dialog);
  }
}
