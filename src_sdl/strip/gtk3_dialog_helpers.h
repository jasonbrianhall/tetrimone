#ifndef GTK3_DIALOG_HELPERS_H
#define GTK3_DIALOG_HELPERS_H

#include <gtk/gtk.h>
#include <vector>
#include <string>

namespace GTK3Helpers {

// ============================================================================
// Generic Dialog Builder
// ============================================================================

struct DialogConfig {
    std::string title;
    std::string acceptButtonLabel;
    int width;
    int height;
};

struct TextConfig {
    std::string content;
    std::string markup;  // Use this for Pango markup instead of plain content
    bool isMarkup;
    double marginTop;
    double marginBottom;
};

struct RadioGroupConfig {
    std::string frameTitle;
    std::vector<std::string> options;
    int defaultSelectedIndex;
};

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

// Build a complete dialog with title, content area, and buttons
// Returns the dialog widget (caller is responsible for showing and destroying)
GtkWidget* createDialog(
    GtkWindow* parent,
    const DialogConfig& dialogConfig,
    const std::vector<TextConfig>& textElements,
    const RadioGroupConfig* radioConfig = nullptr,
    const std::vector<TextConfig>& footerElements = std::vector<TextConfig>()
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
    const RadioGroupConfig* radioConfig = nullptr,
    const std::vector<TextConfig>& footerElements = std::vector<TextConfig>()
) {
    GtkWidget* dialog = createDialog(parent, dialogConfig, textElements, radioConfig, footerElements);
    return runDialog(dialog);
}

}  // namespace GTK3Helpers

#endif  // GTK3_DIALOG_HELPERS_H
