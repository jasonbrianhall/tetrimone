// ============================================================================
// wx_glib_compat.h - Minimal GLib replacement for the wxWidgets frontend
//
// The shared game code (animations, heat decay, propaganda pulses, joystick
// config paths) was written against a handful of GLib calls. Rather than
// dragging GLib/GTK into the Windows build, the wxWidgets frontend provides
// these few functions on top of wxTimer / wxStandardPaths, with the same
// semantics the shared code relies on:
//
//   * g_timeout_add() returns a non-zero id; the callback keeps firing while
//     it returns TRUE and is removed when it returns FALSE.
//   * g_source_remove() may be called from inside the source's own callback.
//   * Removing an id that has already finished is a harmless no-op.
// ============================================================================

#ifndef WX_GLIB_COMPAT_H
#define WX_GLIB_COMPAT_H

#ifdef WXWIDGETS

typedef int           gboolean;
typedef int           gint;
typedef unsigned int  guint;
typedef char          gchar;
typedef void*         gpointer;
typedef const void*   gconstpointer;

typedef gboolean (*GSourceFunc)(gpointer userData);

#ifndef TRUE
#define TRUE 1
#endif
#ifndef FALSE
#define FALSE 0
#endif

#define G_SOURCE_REMOVE   FALSE
#define G_SOURCE_CONTINUE TRUE

#define GINT_TO_POINTER(i) ((gpointer)(intptr_t)(i))
#define GPOINTER_TO_INT(p) ((gint)(intptr_t)(p))

#include <cstdint>

// Timers (implemented with wxTimer, run on the GUI thread)
guint    g_timeout_add(guint intervalMs, GSourceFunc function, gpointer data);
guint    g_idle_add(GSourceFunc function, gpointer data);
gboolean g_source_remove(guint id);

// Stops and frees every pending source (called on shutdown)
void     wx_glib_compat_remove_all_sources();

// Paths / misc
const gchar* g_get_user_config_dir();
gint         g_mkdir_with_parents(const gchar* path, gint mode);
void         g_print(const gchar* format, ...);

#endif // WXWIDGETS

#endif // WX_GLIB_COMPAT_H
