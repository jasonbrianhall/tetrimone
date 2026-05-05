#include "tetrimone.h"
#include "audiomanager.h"
#include "gtk3_dialog_helpers.h"
#include <iostream>
#include <string>
#include <algorithm>
#ifdef _WIN32
#include <windows.h>
#include <commdlg.h>
#endif
#include "highscores.h"
#include "propaganda_messages.h"

using namespace GTK3Helpers;

void onAboutDialog(GtkMenuItem *menuItem, gpointer userData) {
  TetrimoneApp *app = static_cast<TetrimoneApp *>(userData);

  // Check if in retro mode
  if (app->board->retroModeActive) {
    DialogConfig dialogConfig{
        .title = "Государственное Сообщение: Система Управления Блоками",
        .acceptButtonLabel = "_Понял!",
        .width = 600,
        .height = 600
    };
    
    ScrolledTextConfig textConfig{
        .content = "БЛОЧНАЯ РЕВОЛЮЦИЯ\n"
                  "(Block Revolution)\n\n"
                  "Официальный Выпуск: Производственный Цикл 1.0\n\n"
                  "Передовая Система Геометрической Оптимизации\n"
                  "(Advanced Geometric Optimization System)\n\n"
                  "★ Одобрено Центральным Комитетом Блочного Позиционирования ★\n"
                  "(Approved by the Central Block Positioning Committee)\n\n"
                  "Где нет блоков - там нет прогресса!\n"
                  "(Where there are no blocks, there is no progress!)\n\n"
                  "ДОСТИЖЕНИЯ ГОСУДАРСТВЕННОЙ ВАЖНОСТИ:\n"
                  "• Максимальная эффективность падения блоков\n"
                  "• Абсолютная точность геометрической трансформации\n"
                  "• Непрерывность производственного процесса\n\n"
                  "Каждый падающий блок - удар по капиталистическому хаосу!\n\n"
                  "Распространяется по протоколам Коллективного Программного Обеспечения\n"
                  "(Distributed under Collective Software Protocols)\n\n"
                  "© Государственное Бюро Управления Блоками, Московское Отделение\n"
                  "(State Block Management Bureau, Moscow Division)\n\n"
                  "ВНИМАНИЕ: Производительность гарантирована высшим государственным авторитетом!\n"
                  "(Performance Guaranteed by Highest State Authority!)\n\n"
                  "Неудача - это всего лишь временное искажение коллективного потенциала!\n"
                  "(Failure is merely a temporary misalignment of collective potential!)",
        .fontDescription = "Sans 10",
        .width = 600,
        .height = 600
    };
    
    GtkWidget* dialog = createDialog(
        GTK_WINDOW(app->window),
        dialogConfig,
        std::vector<TextConfig>()
    );
    gtk_window_set_default_size(GTK_WINDOW(dialog), 600, 600);
    
    GtkWidget* contentArea = gtk_dialog_get_content_area(GTK_DIALOG(dialog));
    gtk_container_set_border_width(GTK_CONTAINER(contentArea), 15);
    
    GtkWidget* scrolledText = createScrolledTextView(textConfig);
    gtk_container_add(GTK_CONTAINER(contentArea), scrolledText);
    
    gtk_widget_show_all(dialog);
    runDialog(dialog);
    
  } else if (app->board->patrioticModeActive) {
    DialogConfig dialogConfig{
        .title = "🇺🇸 FREEDOM BLOCKS - AMERICAN EXCELLENCE EDITION 🦅",
        .acceptButtonLabel = "_God Bless America!",
        .width = 650,
        .height = 650
    };
    
    ScrolledTextConfig textConfig{
        .content = "FREEDOM BLOCKS\n"
                  "🇺🇸 AMERICAN EXCELLENCE EDITION 🦅\n"
                  "Making Block Stacking Great Again!\n\n"
                  "Premium Release: Freedom Edition v1.0 🎯\n\n"
                  "🚀 Revolutionary Block Management Solution\n"
                  "Powered by American Innovation & Entrepreneurial Spirit\n\n"
                  "⭐ CERTIFIED BY THE DEPARTMENT OF FREEDOM & LIBERTY ⭐\n"
                  "🏆 Winner: 'Best Block Game' - American Gaming Awards 🏆\n\n"
                  "🦅 Where Eagles Soar, Blocks Fall With Purpose! 🦅\n"
                  "Life, Liberty, and the Pursuit of Perfect Line Clears!\n\n"
                  "🇺🇸 FEATURES OF FREEDOM:\n"
                  "• 💪 Maximum Block Drop Efficiency (Made in USA!)\n"
                  "• 🎯 Precision Rotation Technology (Patent Pending)\n"
                  "• ⚡ Lightning-Fast Performance (Faster than a Mustang!)\n"
                  "• 🏈 All-American Gameplay Experience\n"
                  "• 🍔 Optimized for Coffee & Gaming Sessions\n\n"
                  "Every Block Drop is a Victory for Democracy!\n\n"
                  "📜 Licensed under the American Dream Public License\n"
                  "(Freedom for All, Blocks for Everyone!)\n\n"
                  "© FREEDOM BLOCKS™ - PROUDLY AMERICAN\n\n"
                  "⭐ SATISFACTION GUARANTEE:\n"
                  "Not completely amazed? Your freedom is FULLY REFUNDABLE! 💯",
        .fontDescription = "Sans 11",
        .width = 650,
        .height = 650
    };
    
    GtkWidget* dialog = createDialog(
        GTK_WINDOW(app->window),
        dialogConfig,
        std::vector<TextConfig>()
    );
    gtk_window_set_default_size(GTK_WINDOW(dialog), 650, 650);
    
    GtkWidget* contentArea = gtk_dialog_get_content_area(GTK_DIALOG(dialog));
    gtk_container_set_border_width(GTK_CONTAINER(contentArea), 15);
    
    GtkWidget* scrolledText = createScrolledTextView(textConfig);
    gtk_container_add(GTK_CONTAINER(contentArea), scrolledText);
    
    gtk_widget_show_all(dialog);
    runDialog(dialog);
    
  } else {
    // Default about dialog
    DialogConfig dialogConfig{
        .title = "About",
        .acceptButtonLabel = "_OK",
        .width = 600,
        .height = 500
    };
    
    ScrolledTextConfig textConfig{
        .content = "Tetrimone\n\n"
                  "A classic block-stacking game with modern features.\n\n"
                  "Goal: Arrange falling blocks to complete lines and score points.\n\n"
                  "Features:\n"
                  "• Multiple game modes\n"
                  "• Controller support\n"
                  "• Progressive difficulty levels\n"
                  "• High score tracking\n"
                  "• Customizable settings\n\n"
                  "Controls:\n"
                  "• Arrow keys or WASD to move and rotate\n"
                  "• Space to hard drop\n"
                  "• P to pause\n"
                  "• R to restart\n\n"
                  "For complete instructions, select Help from the menu.",
        .fontDescription = "Sans 11",
        .width = 600,
        .height = 500
    };
    
    GtkWidget* dialog = createDialog(
        GTK_WINDOW(app->window),
        dialogConfig,
        std::vector<TextConfig>()
    );
    gtk_window_set_default_size(GTK_WINDOW(dialog), 600, 500);
    
    GtkWidget* contentArea = gtk_dialog_get_content_area(GTK_DIALOG(dialog));
    gtk_container_set_border_width(GTK_CONTAINER(contentArea), 15);
    
    GtkWidget* scrolledText = createScrolledTextView(textConfig);
    gtk_container_add(GTK_CONTAINER(contentArea), scrolledText);
    
    gtk_widget_show_all(dialog);
    runDialog(dialog);
  }
}

void onInstructionsDialog(GtkMenuItem *menuItem, gpointer userData) {
  TetrimoneApp *app = static_cast<TetrimoneApp *>(userData);

  // Check if in retro mode
  if (app->board->retroModeActive) {
    DialogConfig dialogConfig{
        .title = "ИНСТРУКЦИЯ ПО ПАДАЮЩИМ БЛОКАМ",
        .acceptButtonLabel = "_Понял! (Understood!)",
        .width = 800,
        .height = 600
    };
    
    ScrolledTextConfig textConfig{
        .content = "СОВЕРШЕННО СЕКРЕТНО\n"
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
                  "P.S. Big Brother is watching your blocks!",
        .fontDescription = "Monospace 10",
        .width = 800,
        .height = 600
    };
    
    GtkWidget* dialog = createDialog(
        GTK_WINDOW(app->window),
        dialogConfig,
        std::vector<TextConfig>()
    );
    gtk_window_set_default_size(GTK_WINDOW(dialog), 800, 600);
    
    GtkWidget* contentArea = gtk_dialog_get_content_area(GTK_DIALOG(dialog));
    gtk_container_set_border_width(GTK_CONTAINER(contentArea), 15);
    
    GtkWidget* scrolledText = createScrolledTextView(textConfig);
    gtk_container_add(GTK_CONTAINER(contentArea), scrolledText);
    
    gtk_widget_show_all(dialog);
    runDialog(dialog);
    
  } else if (app->board->patrioticModeActive) {
    DialogConfig dialogConfig{
        .title = "🇺🇸 FREEDOM BLOCKS - OFFICIAL PATRIOT MANUAL 🦅",
        .acceptButtonLabel = "_USA! USA! USA!",
        .width = 850,
        .height = 650
    };
    
    ScrolledTextConfig textConfig{
        .content = "🇺🇸 FREEDOM BLOCKS™ - OFFICIAL PATRIOT TRAINING MANUAL 🦅\n"
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
                  "⭐⭐⭐⭐⭐ \"Best block game ever! So American!\" - PatriotGamer2025",
        .fontDescription = "Sans 11",
        .width = 850,
        .height = 650
    };
    
    GtkWidget* dialog = createDialog(
        GTK_WINDOW(app->window),
        dialogConfig,
        std::vector<TextConfig>()
    );
    gtk_window_set_default_size(GTK_WINDOW(dialog), 850, 650);
    
    GtkWidget* contentArea = gtk_dialog_get_content_area(GTK_DIALOG(dialog));
    gtk_container_set_border_width(GTK_CONTAINER(contentArea), 15);
    
    GtkWidget* scrolledText = createScrolledTextView(textConfig);
    gtk_container_add(GTK_CONTAINER(contentArea), scrolledText);
    
    gtk_widget_show_all(dialog);
    runDialog(dialog);
    
  } else {
    // Default instructions
    DialogConfig dialogConfig{
        .title = "Instructions",
        .acceptButtonLabel = "_OK",
        .width = 800,
        .height = 600
    };
    
    ScrolledTextConfig textConfig{
        .content = "Tetrimone Instructions:\n\n"
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
                  "• Try to build sequences by clearing lines consecutively",
        .fontDescription = "Monospace 10",
        .width = 800,
        .height = 600
    };
    
    GtkWidget* dialog = createDialog(
        GTK_WINDOW(app->window),
        dialogConfig,
        std::vector<TextConfig>()
    );
    gtk_window_set_default_size(GTK_WINDOW(dialog), 800, 600);
    
    GtkWidget* contentArea = gtk_dialog_get_content_area(GTK_DIALOG(dialog));
    gtk_container_set_border_width(GTK_CONTAINER(contentArea), 15);
    
    GtkWidget* scrolledText = createScrolledTextView(textConfig);
    gtk_container_add(GTK_CONTAINER(contentArea), scrolledText);
    
    gtk_widget_show_all(dialog);
    runDialog(dialog);
  }
}
