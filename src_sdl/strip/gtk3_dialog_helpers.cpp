#include "gtk3_dialog_helpers.h"
#include "highscores.h"
#include <cstring>

namespace GTK3Helpers {

// ============================================================================
// GTK3FileDialog Implementation
// ============================================================================

std::string GTK3FileDialog::openFile(
    const std::string& title,
    const std::string& filter,
    const std::string& filterDescription
) {
    GtkWidget* dialog = gtk_file_chooser_dialog_new(
        title.c_str(),
        parentWindow,
        GTK_FILE_CHOOSER_ACTION_OPEN,
        "_Cancel", GTK_RESPONSE_CANCEL,
        "_Open", GTK_RESPONSE_ACCEPT,
        NULL
    );
    
    // Add file filter
    GtkFileFilter* fileFilter = gtk_file_filter_new();
    gtk_file_filter_set_name(fileFilter, filterDescription.c_str());
    gtk_file_filter_add_pattern(fileFilter, filter.c_str());
    
    // Add MIME type if it's a ZIP file
    if (filter == "*.zip") {
        gtk_file_filter_add_mime_type(fileFilter, "application/zip");
    }
    
    gtk_file_chooser_add_filter(GTK_FILE_CHOOSER(dialog), fileFilter);
    
    std::string filePath;
    if (gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_ACCEPT) {
        char* filename = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(dialog));
        filePath = filename;
        g_free(filename);
    }
    
    gtk_widget_destroy(dialog);
    return filePath;
}

std::vector<std::string> GTK3FileDialog::openFiles(
    const std::string& title,
    const std::string& filter,
    const std::string& filterDescription
) {
    std::vector<std::string> filePaths;
    
    GtkWidget* dialog = gtk_file_chooser_dialog_new(
        title.c_str(),
        parentWindow,
        GTK_FILE_CHOOSER_ACTION_OPEN,
        "_Cancel", GTK_RESPONSE_CANCEL,
        "_Open", GTK_RESPONSE_ACCEPT,
        NULL
    );
    
    gtk_file_chooser_set_select_multiple(GTK_FILE_CHOOSER(dialog), TRUE);
    
    // Add file filter
    GtkFileFilter* fileFilter = gtk_file_filter_new();
    gtk_file_filter_set_name(fileFilter, filterDescription.c_str());
    gtk_file_filter_add_pattern(fileFilter, filter.c_str());
    gtk_file_chooser_add_filter(GTK_FILE_CHOOSER(dialog), fileFilter);
    
    if (gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_ACCEPT) {
        GSList* filenames = gtk_file_chooser_get_filenames(GTK_FILE_CHOOSER(dialog));
        
        for (GSList* list = filenames; list != NULL; list = list->next) {
            char* filename = static_cast<char*>(list->data);
            filePaths.push_back(filename);
            g_free(filename);
        }
        
        g_slist_free(filenames);
    }
    
    gtk_widget_destroy(dialog);
    return filePaths;
}

void GTK3FileDialog::showError(
    const std::string& title,
    const std::string& message
) {
    GtkWidget* errorDialog = gtk_message_dialog_new(
        parentWindow,
        GTK_DIALOG_MODAL,
        GTK_MESSAGE_ERROR,
        GTK_BUTTONS_OK,
        "%s", message.c_str()
    );
    gtk_window_set_title(GTK_WINDOW(errorDialog), title.c_str());
    gtk_dialog_run(GTK_DIALOG(errorDialog));
    gtk_widget_destroy(errorDialog);
}

// ============================================================================
// Existing Dialog Helpers (unchanged)
// ============================================================================

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

// Create a tabbed score viewer dialog
void createScoreTabulatorDialog(
    GtkWindow* parent,
    const ScoreTabulatorConfig& config
) {
    GtkWidget* dialog = gtk_dialog_new_with_buttons(
        config.title.c_str(),
        parent,
        GTK_DIALOG_MODAL,
        "_Close", GTK_RESPONSE_CLOSE,
        NULL
    );
    gtk_window_set_default_size(GTK_WINDOW(dialog), config.width, config.height);
    
    GtkWidget* contentArea = gtk_dialog_get_content_area(GTK_DIALOG(dialog));
    gtk_container_set_border_width(GTK_CONTAINER(contentArea), 10);
    
    // Create notebook (tabbed interface)
    GtkWidget* notebook = gtk_notebook_new();
    gtk_box_pack_start(GTK_BOX(contentArea), notebook, TRUE, TRUE, 0);
    
    // Create a tab for each score set
    for (const auto& tabData : config.tabs) {
        // Create scrolled window for this tab
        GtkWidget* scrollWindow = gtk_scrolled_window_new(NULL, NULL);
        gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrollWindow), 
                                        GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
        gtk_scrolled_window_set_shadow_type(GTK_SCROLLED_WINDOW(scrollWindow), GTK_SHADOW_ETCHED_IN);
        
        // Create list store and tree view
        GtkListStore* listStore = gtk_list_store_new(7, 
            G_TYPE_STRING,  // Name
            G_TYPE_INT,     // Score
            G_TYPE_STRING,  // Difficulty
            G_TYPE_STRING,  // Grid Size
            G_TYPE_STRING,  // Junk Settings
            G_TYPE_INT,     // Initial Junk % (hidden)
            G_TYPE_INT      // Junk per Level (hidden)
        );
        
        // Populate list store
        for (const auto& score : tabData.scores) {
            GtkTreeIter iter;
            gtk_list_store_append(listStore, &iter);
            
            char gridSizeBuffer[50];
            snprintf(gridSizeBuffer, sizeof(gridSizeBuffer), "%d x %d", score.width, score.height);
            
            char junkBuffer[50];
            snprintf(junkBuffer, sizeof(junkBuffer), "Init: %d%%, Level: %d", 
                    score.initialJunkPercent, score.junkLinesPerLevel);
            
            gtk_list_store_set(listStore, &iter, 
                0, score.name.c_str(),
                1, score.score,
                2, score.difficulty.c_str(), 
                3, gridSizeBuffer,
                4, junkBuffer,
                5, score.initialJunkPercent,
                6, score.junkLinesPerLevel,
                -1
            );
        }
        
        // Create tree view
        GtkWidget* treeView = gtk_tree_view_new_with_model(GTK_TREE_MODEL(listStore));
        g_object_unref(listStore);
        
        gtk_tree_view_set_grid_lines(GTK_TREE_VIEW(treeView), GTK_TREE_VIEW_GRID_LINES_BOTH);
        
        // Create columns
        GtkCellRenderer* renderer = gtk_cell_renderer_text_new();
        
        // Name column
        GtkTreeViewColumn* nameColumn = gtk_tree_view_column_new_with_attributes(
            "Name", renderer, "text", 0, NULL
        );
        gtk_tree_view_column_set_expand(nameColumn, TRUE);
        gtk_tree_view_column_set_sort_column_id(nameColumn, 0);
        gtk_tree_view_column_set_resizable(nameColumn, TRUE);
        gtk_tree_view_append_column(GTK_TREE_VIEW(treeView), nameColumn);
        
        // Score column
        GtkTreeViewColumn* scoreColumn = gtk_tree_view_column_new_with_attributes(
            "Score", renderer, "text", 1, NULL
        );
        gtk_tree_view_column_set_expand(scoreColumn, TRUE);
        gtk_tree_view_column_set_sort_column_id(scoreColumn, 1);
        gtk_tree_view_column_set_resizable(scoreColumn, TRUE);
        gtk_tree_view_append_column(GTK_TREE_VIEW(treeView), scoreColumn);
        
        // Difficulty column
        GtkTreeViewColumn* diffColumn = gtk_tree_view_column_new_with_attributes(
            "Difficulty", renderer, "text", 2, NULL
        );
        gtk_tree_view_column_set_expand(diffColumn, TRUE);
        gtk_tree_view_column_set_sort_column_id(diffColumn, 2);
        gtk_tree_view_column_set_resizable(diffColumn, TRUE);
        gtk_tree_view_append_column(GTK_TREE_VIEW(treeView), diffColumn);
        
        // Grid Size column
        GtkTreeViewColumn* sizeColumn = gtk_tree_view_column_new_with_attributes(
            "Grid Size", renderer, "text", 3, NULL
        );
        gtk_tree_view_column_set_expand(sizeColumn, TRUE);
        gtk_tree_view_column_set_sort_column_id(sizeColumn, 3);
        gtk_tree_view_column_set_resizable(sizeColumn, TRUE);
        gtk_tree_view_append_column(GTK_TREE_VIEW(treeView), sizeColumn);
        
        // Junk Settings column
        GtkTreeViewColumn* junkColumn = gtk_tree_view_column_new_with_attributes(
            "Junk Lines", renderer, "text", 4, NULL
        );
        gtk_tree_view_column_set_expand(junkColumn, TRUE);
        gtk_tree_view_column_set_sort_column_id(junkColumn, 5);
        gtk_tree_view_column_set_resizable(junkColumn, TRUE);
        gtk_tree_view_append_column(GTK_TREE_VIEW(treeView), junkColumn);
        
        gtk_tree_view_set_headers_clickable(GTK_TREE_VIEW(treeView), TRUE);
        
        // Add tree view to scrolled window
        gtk_container_add(GTK_CONTAINER(scrollWindow), treeView);
        
        // Add tab to notebook
        char tabLabel[50];
        snprintf(tabLabel, sizeof(tabLabel), "%s (%zu)", tabData.tabName.c_str(), tabData.scores.size());
        gtk_notebook_append_page(GTK_NOTEBOOK(notebook), scrollWindow, gtk_label_new(tabLabel));
    }
    
    gtk_widget_show_all(dialog);
    gtk_dialog_run(GTK_DIALOG(dialog));
    gtk_widget_destroy(dialog);
}

// Create an opacity slider dialog
void createOpacitySliderDialog(
    GtkWindow* parent,
    const OpacitySliderConfig& config,
    GCallback onValueChanged,
    gpointer userData
) {
    GtkWidget* dialog = gtk_dialog_new_with_buttons(
        config.title.c_str(),
        parent,
        GTK_DIALOG_MODAL,
        "_OK", GTK_RESPONSE_OK,
        NULL
    );
    
    gtk_window_set_default_size(GTK_WINDOW(dialog), config.width, config.height);
    
    GtkWidget* contentArea = gtk_dialog_get_content_area(GTK_DIALOG(dialog));
    gtk_container_set_border_width(GTK_CONTAINER(contentArea), 10);
    
    GtkWidget* vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 6);
    gtk_container_add(GTK_CONTAINER(contentArea), vbox);
    
    // Add instruction label
    TextConfig instructionConfig{
        .content = "Adjust background opacity:",
        .markup = "",
        .isMarkup = false,
        .marginTop = 0,
        .marginBottom = 10
    };
    GtkWidget* label = createTextLabel(instructionConfig);
    gtk_box_pack_start(GTK_BOX(vbox), label, FALSE, FALSE, 0);
    
    // Create slider
    GtkWidget* scale = gtk_scale_new_with_range(
        GTK_ORIENTATION_HORIZONTAL,
        config.minValue,
        config.maxValue,
        config.stepValue
    );
    gtk_range_set_value(GTK_RANGE(scale), config.currentValue);
    gtk_scale_set_digits(GTK_SCALE(scale), 2);
    gtk_scale_set_value_pos(GTK_SCALE(scale), GTK_POS_RIGHT);
    gtk_box_pack_start(GTK_BOX(vbox), scale, FALSE, FALSE, 5);
    
    // Connect callback
    if (onValueChanged) {
        g_signal_connect(G_OBJECT(scale), "value-changed",
                        onValueChanged, userData);
    }
    
    // Add range labels
    GtkWidget* rangeBox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
    gtk_box_pack_start(GTK_BOX(vbox), rangeBox, FALSE, FALSE, 0);
    
    TextConfig minConfig{
        .content = "Transparent",
        .markup = "",
        .isMarkup = false,
        .marginTop = 0,
        .marginBottom = 0
    };
    GtkWidget* minLabel = createTextLabel(minConfig);
    gtk_widget_set_halign(minLabel, GTK_ALIGN_START);
    gtk_box_pack_start(GTK_BOX(rangeBox), minLabel, TRUE, TRUE, 0);
    
    TextConfig maxConfig{
        .content = "Opaque",
        .markup = "",
        .isMarkup = false,
        .marginTop = 0,
        .marginBottom = 0
    };
    GtkWidget* maxLabel = createTextLabel(maxConfig);
    gtk_widget_set_halign(maxLabel, GTK_ALIGN_END);
    gtk_box_pack_end(GTK_BOX(rangeBox), maxLabel, TRUE, TRUE, 0);
    
    gtk_widget_show_all(dialog);
    gtk_dialog_run(GTK_DIALOG(dialog));
    gtk_widget_destroy(dialog);
}

}  // namespace GTK3Helpers
