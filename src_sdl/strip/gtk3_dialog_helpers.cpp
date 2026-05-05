#include "gtk3_dialog_helpers.h"

namespace GTK3Helpers {

// Build a labeled text widget
GtkWidget* createTextLabel(const TextConfig& config) {
    GtkWidget* label = gtk_label_new(NULL);
    
    if (config.isMarkup) {
        gtk_label_set_markup(GTK_LABEL(label), config.markup.c_str());
    } else {
        gtk_label_set_text(GTK_LABEL(label), config.content.c_str());
    }
    
    gtk_label_set_line_wrap(GTK_LABEL(label), TRUE);
    gtk_label_set_selectable(GTK_LABEL(label), TRUE);
    
    if (config.marginTop > 0 || config.marginBottom > 0) {
        gtk_widget_set_margin_top(label, config.marginTop);
        gtk_widget_set_margin_bottom(label, config.marginBottom);
    }
    
    return label;
}

// Build a radio button group and return the group list
// Returns GSList* for further manipulation, selected index is set automatically
GSList* createRadioGroup(GtkWidget* container, const RadioGroupConfig& config) {
    // Create frame
    GtkWidget* frame = gtk_frame_new(config.frameTitle.c_str());
    gtk_box_pack_start(GTK_BOX(container), frame, TRUE, TRUE, 0);
    
    GtkWidget* reasonBox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 5);
    gtk_container_add(GTK_CONTAINER(frame), reasonBox);
    gtk_container_set_border_width(GTK_CONTAINER(reasonBox), 10);
    
    GSList* group = NULL;
    
    for (size_t i = 0; i < config.options.size(); ++i) {
        GtkWidget* radioButton = gtk_radio_button_new_with_label(
            group, 
            config.options[i].c_str()
        );
        group = gtk_radio_button_get_group(GTK_RADIO_BUTTON(radioButton));
        gtk_box_pack_start(GTK_BOX(reasonBox), radioButton, FALSE, FALSE, 0);
        
        if (static_cast<int>(i) == config.defaultSelectedIndex) {
            gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(radioButton), TRUE);
        }
    }
    
    return group;
}

// Build a scrolled text view with content and font settings
GtkWidget* createScrolledTextView(const ScrolledTextConfig& config) {
    // Create scrolled window
    GtkWidget* scrolledWindow = gtk_scrolled_window_new(NULL, NULL);
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrolledWindow), 
                                   GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
    gtk_widget_set_size_request(scrolledWindow, config.width, config.height);
    
    // Create text view
    GtkWidget* textView = gtk_text_view_new();
    gtk_text_view_set_editable(GTK_TEXT_VIEW(textView), FALSE);
    gtk_text_view_set_wrap_mode(GTK_TEXT_VIEW(textView), GTK_WRAP_WORD);
    
    // Set font if specified
    if (!config.fontDescription.empty()) {
        PangoFontDescription* fontDesc = pango_font_description_from_string(config.fontDescription.c_str());
        gtk_widget_override_font(textView, fontDesc);
        pango_font_description_free(fontDesc);
    }
    
    // Set text content
    GtkTextBuffer* buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(textView));
    gtk_text_buffer_set_text(buffer, config.content.c_str(), -1);
    
    // Add text view to scrolled window
    gtk_container_add(GTK_CONTAINER(scrolledWindow), textView);
    
    return scrolledWindow;
}

// Build a complete dialog with title, content area, and buttons
// Returns the dialog widget (caller is responsible for showing and destroying)
GtkWidget* createDialog(
    GtkWindow* parent,
    const DialogConfig& dialogConfig,
    const std::vector<TextConfig>& textElements,
    const RadioGroupConfig* radioConfig,
    const std::vector<TextConfig>& footerElements
) {
    GtkWidget* dialog = gtk_dialog_new_with_buttons(
        dialogConfig.title.c_str(),
        parent,
        GTK_DIALOG_MODAL,
        dialogConfig.acceptButtonLabel.c_str(),
        GTK_RESPONSE_OK,
        NULL
    );
    
    gtk_window_set_default_size(GTK_WINDOW(dialog), dialogConfig.width, dialogConfig.height);
    
    GtkWidget* contentArea = gtk_dialog_get_content_area(GTK_DIALOG(dialog));
    gtk_container_set_border_width(GTK_CONTAINER(contentArea), 15);
    
    GtkWidget* vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 10);
    gtk_container_add(GTK_CONTAINER(contentArea), vbox);
    
    // Add text elements
    for (const auto& textConfig : textElements) {
        GtkWidget* label = createTextLabel(textConfig);
        gtk_box_pack_start(GTK_BOX(vbox), label, FALSE, FALSE, 0);
    }
    
    // Add radio group if provided
    if (radioConfig) {
        createRadioGroup(vbox, *radioConfig);
    }
    
    // Add footer elements
    for (const auto& textConfig : footerElements) {
        GtkWidget* label = createTextLabel(textConfig);
        gtk_box_pack_start(GTK_BOX(vbox), label, FALSE, FALSE, 0);
    }
    
    gtk_widget_show_all(dialog);
    
    return dialog;
}

// Run dialog and handle cleanup - encapsulates all GTK3 interaction
gint runDialog(GtkWidget* dialog) {
    gint result = gtk_dialog_run(GTK_DIALOG(dialog));
    gtk_widget_destroy(dialog);
    return result;
}

// High-level function: create and run dialog in one call
gint createAndRunDialog(
    GtkWindow* parent,
    const DialogConfig& dialogConfig,
    const std::vector<TextConfig>& textElements,
    const RadioGroupConfig* radioConfig,
    const std::vector<TextConfig>& footerElements
) {
    GtkWidget* dialog = createDialog(parent, dialogConfig, textElements, radioConfig, footerElements);
    return runDialog(dialog);
}

// Create a score entry dialog and return the player name
std::string createScoreEntryDialog(
    GtkWindow* parent,
    const ScoreEntryConfig& config
) {
    GtkWidget* dialog = gtk_dialog_new_with_buttons(
        config.title.c_str(),
        parent,
        GTK_DIALOG_MODAL,
        "_Submit", GTK_RESPONSE_OK,
        "_Cancel", GTK_RESPONSE_CANCEL,
        NULL
    );
    
    gtk_window_set_default_size(GTK_WINDOW(dialog), 400, 250);
    
    GtkWidget* contentArea = gtk_dialog_get_content_area(GTK_DIALOG(dialog));
    gtk_container_set_border_width(GTK_CONTAINER(contentArea), 15);
    
    GtkWidget* vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 10);
    gtk_container_add(GTK_CONTAINER(contentArea), vbox);
    
    // Score display
    gchar* scoreBuf = g_strdup_printf("Score: <b>%d</b>", config.score);
    GtkWidget* scoreLabel = gtk_label_new(NULL);
    gtk_label_set_markup(GTK_LABEL(scoreLabel), scoreBuf);
    g_free(scoreBuf);
    gtk_box_pack_start(GTK_BOX(vbox), scoreLabel, FALSE, FALSE, 5);
    
    // Difficulty display
    gchar* diffBuf = g_strdup_printf("Difficulty: %s", config.difficulty.c_str());
    GtkWidget* diffLabel = gtk_label_new(diffBuf);
    g_free(diffBuf);
    gtk_box_pack_start(GTK_BOX(vbox), diffLabel, FALSE, FALSE, 5);
    
    // Grid size display
    GtkWidget* sizeLabel = gtk_label_new(config.gridSize.c_str());
    gtk_box_pack_start(GTK_BOX(vbox), sizeLabel, FALSE, FALSE, 5);
    
    // Junk info display
    GtkWidget* junkLabel = gtk_label_new(config.junkInfo.c_str());
    gtk_box_pack_start(GTK_BOX(vbox), junkLabel, FALSE, FALSE, 5);
    
    // Separator
    GtkWidget* separator = gtk_separator_new(GTK_ORIENTATION_HORIZONTAL);
    gtk_box_pack_start(GTK_BOX(vbox), separator, FALSE, FALSE, 5);
    
    // Name entry
    GtkWidget* nameLabel = gtk_label_new("Enter your name:");
    gtk_box_pack_start(GTK_BOX(vbox), nameLabel, FALSE, FALSE, 0);
    
    GtkWidget* entry = gtk_entry_new();
    gtk_entry_set_placeholder_text(GTK_ENTRY(entry), "Anonymous");
    gtk_box_pack_start(GTK_BOX(vbox), entry, FALSE, FALSE, 5);
    
    gtk_widget_show_all(dialog);
    
    // Run dialog and capture response
    std::string playerName = "";
    gint response = gtk_dialog_run(GTK_DIALOG(dialog));
    
    if (response == GTK_RESPONSE_OK) {
        const char* name = gtk_entry_get_text(GTK_ENTRY(entry));
        if (name && strlen(name) > 0) {
            playerName = name;
        } else {
            playerName = "Anonymous";
        }
    }
    
    gtk_widget_destroy(dialog);
    return playerName;
}

}  // namespace GTK3Helpers
