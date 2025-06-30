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

void onAboutDialog(GtkMenuItem *menuItem, gpointer userData) {
  TetrimoneApp *app = static_cast<TetrimoneApp *>(userData);

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
    gtk_dialog_run(GTK_DIALOG(dialog));

    // Destroy the dialog when closed
    gtk_widget_destroy(dialog);
  } else if (app->board->patrioticModeActive) {
    // Create a custom American patriotic-style dialog
    GtkWidget *dialog = gtk_dialog_new_with_buttons(
        "🇺🇸 FREEDOM BLOCKS - AMERICAN EXCELLENCE EDITION 🦅", 
        GTK_WINDOW(app->window), 
        GTK_DIALOG_MODAL, 
        "_God Bless America!", 
        GTK_RESPONSE_OK, 
        NULL);

    // Get the content area
    GtkWidget *contentArea = gtk_dialog_get_content_area(GTK_DIALOG(dialog));
    gtk_container_set_border_width(GTK_CONTAINER(contentArea), 15);

    // Create a vertical box for layout
    GtkWidget *vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 10);
    gtk_container_add(GTK_CONTAINER(contentArea), vbox);

    // Add program name with patriotic flair
    GtkWidget *nameLabel = gtk_label_new(NULL);
    gtk_label_set_markup(
        GTK_LABEL(nameLabel),
        "<span size='x-large' weight='bold' color='red'>FREEDOM BLOCKS</span>\n"
        "<span size='large' color='blue'>🇺🇸 AMERICAN EXCELLENCE EDITION 🦅</span>\n"
        "<span size='small'>Making Block Stacking Great Again!</span>");
    gtk_box_pack_start(GTK_BOX(vbox), nameLabel, FALSE, FALSE, 5);

    // Add version (American corporate style)
    GtkWidget *versionLabel = gtk_label_new("Premium Release: Freedom Edition v1.0 🎯");
    gtk_box_pack_start(GTK_BOX(vbox), versionLabel, FALSE, FALSE, 0);

    // Add description with American corporate marketing flair
    GtkWidget *descLabel = gtk_label_new(
        "🚀 Revolutionary Block Management Solution\n"
        "Powered by American Innovation & Entrepreneurial Spirit\n\n"
        "⭐ CERTIFIED BY THE DEPARTMENT OF FREEDOM & LIBERTY ⭐\n"
        "🏆 Winner: 'Best Block Game' - American Gaming Awards 🏆\n\n"
        "🦅 Where Eagles Soar, Blocks Fall With Purpose! 🦅\n"
        "Life, Liberty, and the Pursuit of Perfect Line Clears!");
    gtk_box_pack_start(GTK_BOX(vbox), descLabel, FALSE, FALSE, 10);

    // American-style features and achievements
    GtkWidget *achievementsLabel = gtk_label_new(NULL);
    gtk_label_set_markup(
        GTK_LABEL(achievementsLabel),
        "<span weight='bold' color='blue'>🇺🇸 FEATURES OF FREEDOM:</span>\n"
        "• 💪 Maximum Block Drop Efficiency (Made in USA!)\n"
        "• 🎯 Precision Rotation Technology (Patent Pending)\n"
        "• ⚡ Lightning-Fast Performance (Faster than a Mustang!)\n"
        "• 🏈 All-American Gameplay Experience\n"
        "• 🍔 Optimized for Coffee & Gaming Sessions\n\n"
        "<i>🦅 Every Block Drop is a Victory for Democracy! 🦅</i>");
    gtk_box_pack_start(GTK_BOX(vbox), achievementsLabel, FALSE, FALSE, 5);

    // Add license info with American corporate style
    GtkWidget *licenseLabel = gtk_label_new(NULL);
    gtk_label_set_markup(
        GTK_LABEL(licenseLabel),
        "📜 Licensed under the American Dream Public License\n"
        "<i>🗽 (Freedom for All, Blocks for Everyone!) 🗽</i>");
    gtk_box_pack_start(GTK_BOX(vbox), licenseLabel, FALSE, FALSE, 5);

    // Add website button (American style)
    GtkWidget *websiteButton = gtk_link_button_new_with_label(
        "https://freedomblocks.usa.gov", 
        "🌐 Official Freedom Portal\n🦅 Visit FreedomBlocks.USA.gov 🇺🇸");
    gtk_box_pack_start(GTK_BOX(vbox), websiteButton, FALSE, FALSE, 10);

    // Add social media section
    GtkWidget *socialLabel = gtk_label_new(NULL);
    gtk_label_set_markup(
        GTK_LABEL(socialLabel),
        "<span weight='bold'>📱 FOLLOW US ON SOCIAL MEDIA:</span>\n"
        "🐦 @FreedomBlocks 📘 /FreedomBlocksUSA 📸 @freedom_blocks\n"
        "💬 Join our Discord Community! 🎮");
    gtk_box_pack_start(GTK_BOX(vbox), socialLabel, FALSE, FALSE, 5);

    // Add copyright with American corporate twist
    GtkWidget *copyrightLabel = gtk_label_new(
        "© 2025 Freedom Blocks Corporation, LLC\n"
        "🏢 Proudly Headquartered in Silicon Valley, USA 🇺🇸\n"
        "A Subsidiary of American Dream Enterprises");
    gtk_box_pack_start(GTK_BOX(vbox), copyrightLabel, FALSE, FALSE, 5);

    // Add warranty with American optimism
    GtkWidget *disclaimerLabel = gtk_label_new(NULL);
    gtk_label_set_markup(
        GTK_LABEL(disclaimerLabel),
        "<span color='blue'>🛡️ SATISFACTION GUARANTEED!</span>\n"
        "💯 100% American-Made Quality Assurance!\n"
        "<i>🦅 (If not completely satisfied, your freedom is refunded!) 🦅</i>\n\n"
        "🎯 Success is just one block away!\n"
        "Failure is merely an opportunity for a comeback!\n"
        "<i>🇺🇸 The American Way: Never Give Up! 🇺🇸</i>\n\n"
        "📞 Customer Support: 1-800-FREEDOM\n"
        "💬 Live Chat Available 24/7");
    gtk_box_pack_start(GTK_BOX(vbox), disclaimerLabel, FALSE, FALSE, 0);

    // Show all content
    gtk_widget_show_all(dialog);

    // Run the dialog
    gtk_dialog_run(GTK_DIALOG(dialog));

    // Destroy the dialog when closed
    gtk_widget_destroy(dialog);
  } else {
    // Original about dialog code
    GtkWidget *dialog = gtk_dialog_new_with_buttons(
        "About GTK Tetrimone", GTK_WINDOW(app->window), GTK_DIALOG_MODAL, "_OK",
        GTK_RESPONSE_OK, NULL);

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
    gtk_dialog_run(GTK_DIALOG(dialog));

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
  } else if (app->board->patrioticModeActive) {
    // Create dialog with American patriotic-style instructions
    GtkWidget *dialog = gtk_dialog_new_with_buttons(
        "🇺🇸 FREEDOM BLOCKS - OFFICIAL PATRIOT MANUAL 🦅", 
        GTK_WINDOW(app->window), 
        GTK_DIALOG_MODAL, 
        "_USA! USA! USA!", 
        GTK_RESPONSE_OK, 
        NULL);

    // Get the content area
    GtkWidget *contentArea = gtk_dialog_get_content_area(GTK_DIALOG(dialog));
    gtk_container_set_border_width(GTK_CONTAINER(contentArea), 15);

    // Create a scrolled window with specific size
    GtkWidget *scrolledWindow = gtk_scrolled_window_new(NULL, NULL);
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrolledWindow), 
                                   GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
    gtk_widget_set_size_request(scrolledWindow, 850, 650); // Slightly larger for American style

    // Create a text view
    GtkWidget *textView = gtk_text_view_new();
    gtk_text_view_set_editable(GTK_TEXT_VIEW(textView), FALSE);
    gtk_text_view_set_wrap_mode(GTK_TEXT_VIEW(textView), GTK_WRAP_WORD);
    
    // Set a modern font for American corporate style
    PangoFontDescription *font_desc = pango_font_description_from_string("Sans 11");
    gtk_widget_override_font(textView, font_desc);
    pango_font_description_free(font_desc);

    // Get the buffer and set the text
    GtkTextBuffer *buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(textView));
    gtk_text_buffer_set_text(buffer, 
        "🇺🇸 FREEDOM BLOCKS™ - OFFICIAL PATRIOT TRAINING MANUAL 🦅\n"
        "===============================================\n\n"
        "📜 DOCUMENT #1776-FB\n"
        "OFFICIAL GUIDELINES FOR AMERICAN BLOCK EXCELLENCE\n\n"
        "🎯 ATTENTION, FREEDOM FIGHTER!\n"
        "This manual contains CLASSIFIED FREEDOM TECHNIQUES for maximum block performance.\n"
        "Sharing with enemies of liberty is STRONGLY DISCOURAGED! 🚫\n\n"
        "🦅 CORE FREEDOM BLOCK COMMANDS: 🦅\n\n"
        "1. 🏃‍♂️ LEFT/RIGHT MOVEMENT:\n"
        "   • Precise lateral freedom navigation (Your constitutional right!)\n"
        "   • Smooth as a Harley-Davidson on Route 66! 🏍️\n\n"
        "2. 🔄 ROTATION POWERS:\n"
        "   • Clockwise: The American way (like NASCAR!) 🏁\n"
        "   • Counter-clockwise: Also available (because FREEDOM!) 🗽\n\n"
        "3. ⚡ VERTICAL ACCELERATION:\n"
        "   • Soft Drop: Gentle like a Southern breeze 🌾\n"
        "   • Hard Drop: DECISIVE like American military action! 💥\n\n"
        "🏆 FREEDOM SCORING SYSTEM: 🏆\n"
        "• Every completed line = VICTORY FOR DEMOCRACY! 🎉\n"
        "• Bonus points awarded for ENTREPRENEURIAL SPIRIT 💼\n"
        "• High scores earn you AMERICAN DREAM STATUS! 🌟\n\n"
        "⭐ PATRIOTIC PERFORMANCE GUIDELINES: ⭐\n"
        "🦅 REMEMBER! Every falling block represents AMERICAN INGENUITY! 🦅\n\n"
        "🚨 SPECIAL FREEDOM ZONES: 🚨\n"
        "• Red Line: DANGER ZONE (Like crossing into enemy territory!) ⚠️\n"
        "• Exceeding limits triggers EMERGENCY FREEDOM PROTOCOLS 🚁\n\n"
        "📈 PERFORMANCE IMPROVEMENT PROGRAM: 📈\n"
        "1. First Challenge: MOTIVATIONAL COACHING SESSION 📣\n"
        "2. Second Challenge: ADVANCED FREEDOM TRAINING 🎓\n"
        "3. Third Challenge: ALL-EXPENSES-PAID VACATION TO SUCCESS CAMP! 🏕️\n\n"
        "🎮 PREMIUM FEATURES (Available with Freedom Pass™): 🎮\n"
        "• Custom eagle sound effects 🦅\n"
        "• Patriotic victory animations 🎆\n"
        "• Real-time freedom level monitoring 📊\n"
        "• Direct hotline to customer success team! ☎️\n\n"
        "💡 PRO TIPS FROM TEAM AMERICA: 💡\n"
        "• Coffee break every 15 minutes for optimal performance ☕\n"
        "• Play while listening to country music for +10% accuracy 🎵\n"
        "• Customize your experience in the Settings menu 🔧\n"
        "• Join our Discord community for exclusive tips! 💬\n\n"
        "🛡️ SATISFACTION GUARANTEE: 🛡️\n"
        "Not completely amazed? Your freedom is FULLY REFUNDABLE! 💯\n\n"
        "📞 24/7 FREEDOM SUPPORT: 📞\n"
        "• Hotline: 1-800-FREEDOM (1-800-373-3366)\n"
        "• Email: support@freedomblocks.usa\n"
        "• Live Chat: Available on our website\n"
        "• Social Media: @FreedomBlocks on all platforms\n\n"
        "🇺🇸 REMEMBER, PATRIOT: 🇺🇸\n"
        "IN BLOCKS, AS IN LIFE - AMERICA ALWAYS WINS!\n"
        "SUCCESS IS YOUR BIRTHRIGHT! 🌟\n\n"
        "Approved by: Chief Freedom Officer, Jason 'Eagle' Rodriguez\n"
        "Certified by: Department of Digital Liberty\n"
        "Stamped: FREEDOM APPROVED ✅\n\n"
        "🦅 P.S. Customer satisfaction ratings available on Yelp! 🦅\n"
        "⭐⭐⭐⭐⭐ \"Best block game ever! So American!\" - PatriotGamer2025\n", -1);

    // Add text view to scrolled window
    gtk_container_add(GTK_CONTAINER(scrolledWindow), textView);

    // Add scrolled window to content area
    gtk_container_add(GTK_CONTAINER(contentArea), scrolledWindow);

    // Set window size
    gtk_window_set_default_size(GTK_WINDOW(dialog), 850, 650);

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
