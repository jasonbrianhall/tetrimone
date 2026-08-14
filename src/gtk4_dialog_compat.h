#ifndef GTK4_DIALOG_COMPAT_H
#define GTK4_DIALOG_COMPAT_H

#include <gtk/gtk.h>

// ============================================================================
// gtk_dialog_run() was removed in GTK4 - dialogs are async-only now (they
// emit "response" and you're expected to continue in a callback). Rewriting
// every dialog in this codebase into a callback chain is a much bigger job
// than the dialogs themselves, so this shim reproduces the old blocking
// behavior with a nested GMainLoop, the same technique GTK's own porting
// guide suggests for a first-pass migration. Existing call sites only need
// to change:
//
//     gint response = gtk_dialog_run(GTK_DIALOG(dialog));
// to:
//     gint response = gtk_dialog_run_gtk4(dialog);
//
// and:
//     gtk_widget_destroy(dialog);
// to:
//     gtk_window_destroy(GTK_WINDOW(dialog));
//
// Everything else in a dialog body (packing widgets, reading slider values
// after the call returns, etc.) is unchanged.
// ============================================================================

inline gint gtk_dialog_run_gtk4(GtkWidget* dialog) {
    struct RunData {
        GMainLoop* loop;
        gint response;
    } data;

    data.loop = g_main_loop_new(nullptr, FALSE);
    data.response = GTK_RESPONSE_NONE;

    gulong handlerId = g_signal_connect(
        dialog, "response",
        G_CALLBACK(+[](GtkDialog*, gint response, gpointer userData) {
            RunData* d = static_cast<RunData*>(userData);
            d->response = response;
            if (g_main_loop_is_running(d->loop)) {
                g_main_loop_quit(d->loop);
            }
        }),
        &data);

    // Also quit the loop if the dialog is closed/destroyed without a
    // "response" (e.g. window manager close button).
    gulong destroyId = g_signal_connect(
        dialog, "destroy",
        G_CALLBACK(+[](GtkWidget*, gpointer userData) {
            RunData* d = static_cast<RunData*>(userData);
            if (g_main_loop_is_running(d->loop)) {
                g_main_loop_quit(d->loop);
            }
        }),
        &data);

    gtk_window_present(GTK_WINDOW(dialog));
    g_main_loop_run(data.loop);

    if (g_signal_handler_is_connected(dialog, handlerId)) {
        g_signal_handler_disconnect(dialog, handlerId);
    }
    if (g_signal_handler_is_connected(dialog, destroyId)) {
        g_signal_handler_disconnect(dialog, destroyId);
    }

    g_main_loop_unref(data.loop);
    return data.response;
}

#endif // GTK4_DIALOG_COMPAT_H
