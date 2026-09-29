// ============================================================================
// wx_glib_compat.cpp - wxTimer-backed stand-ins for the GLib calls used by
// the shared game code. See wx_glib_compat.h for the contract.
// ============================================================================

#ifdef WXWIDGETS

#include "wx_glib_compat.h"

#include <wx/wx.h>
#include <wx/timer.h>
#include <wx/filename.h>

#include <cstdarg>
#include <cstdio>
#include <cstdlib>
#include <map>
#include <string>

namespace {

class GSourceTimer;

std::map<guint, GSourceTimer*>& sources() {
    static std::map<guint, GSourceTimer*> s;
    return s;
}

guint nextSourceId = 1;

class GSourceTimer : public wxTimer {
public:
    GSourceTimer(guint id, GSourceFunc fn, gpointer data)
        : id_(id), fn_(fn), data_(data) {}

    // Called by g_source_remove(). If we're inside our own callback we must
    // not delete ourselves yet; Notify() finishes the job.
    void cancel() {
        cancelled_ = true;
        Stop();
        if (!dispatching_) {
            scheduleDelete();
        }
    }

    void Notify() override {
        // Like GLib, never re-enter a source whose callback is still running
        // (e.g. the game tick showing the modal high score dialog).
        if (cancelled_ || dispatching_) return;

        dispatching_ = true;
        gboolean keep = fn_ ? fn_(data_) : FALSE;
        dispatching_ = false;

        if (cancelled_) {
            // Removed from inside the callback: map entry is already gone.
            scheduleDelete();
            return;
        }

        if (!keep) {
            Stop();
            cancelled_ = true;
            sources().erase(id_);
            scheduleDelete();
        }
    }

private:
    void scheduleDelete() {
        if (deleteScheduled_) return;
        deleteScheduled_ = true;
        // Never delete a wxTimer from inside its own Notify(); defer it.
        if (wxTheApp) {
            wxTheApp->CallAfter([this]() { delete this; });
        } else {
            delete this;
        }
    }

    guint       id_;
    GSourceFunc fn_;
    gpointer    data_;
    bool        dispatching_     = false;
    bool        cancelled_       = false;
    bool        deleteScheduled_ = false;
};

guint addSource(guint intervalMs, GSourceFunc function, gpointer data) {
    guint id = nextSourceId++;
    if (nextSourceId == 0) nextSourceId = 1;  // never hand out 0

    GSourceTimer* timer = new GSourceTimer(id, function, data);
    sources()[id] = timer;
    // wxTimer requires a positive interval; 1ms behaves like an idle source.
    timer->Start(intervalMs > 0 ? static_cast<int>(intervalMs) : 1, wxTIMER_CONTINUOUS);
    return id;
}

} // namespace

guint g_timeout_add(guint intervalMs, GSourceFunc function, gpointer data) {
    return addSource(intervalMs, function, data);
}

guint g_idle_add(GSourceFunc function, gpointer data) {
    return addSource(0, function, data);
}

gboolean g_source_remove(guint id) {
    auto& map = sources();
    auto it = map.find(id);
    if (it == map.end()) {
        return FALSE;  // already finished; GLib would warn, we stay quiet
    }
    GSourceTimer* timer = it->second;
    map.erase(it);
    timer->cancel();
    return TRUE;
}

void wx_glib_compat_remove_all_sources() {
    auto& map = sources();
    while (!map.empty()) {
        g_source_remove(map.begin()->first);
    }
}

const gchar* g_get_user_config_dir() {
    static std::string dir;
    if (!dir.empty()) return dir.c_str();

#ifdef _WIN32
    // Match GLib on Windows: CSIDL_LOCAL_APPDATA
    if (const char* local = std::getenv("LOCALAPPDATA")) {
        dir = local;
    } else if (const char* roaming = std::getenv("APPDATA")) {
        dir = roaming;
    } else {
        dir = ".";
    }
#else
    if (const char* xdg = std::getenv("XDG_CONFIG_HOME"); xdg && *xdg) {
        dir = xdg;
    } else if (const char* home = std::getenv("HOME")) {
        dir = std::string(home) + "/.config";
    } else {
        dir = ".";
    }
#endif
    return dir.c_str();
}

gint g_mkdir_with_parents(const gchar* path, gint /*mode*/) {
    if (!path) return -1;
    wxString p = wxString::FromUTF8(path);
    if (wxFileName::DirExists(p)) return 0;
    return wxFileName::Mkdir(p, wxS_DIR_DEFAULT, wxPATH_MKDIR_FULL) ? 0 : -1;
}

void g_print(const gchar* format, ...) {
    va_list args;
    va_start(args, format);
    std::vfprintf(stdout, format, args);
    va_end(args);
    std::fflush(stdout);
}

#endif // WXWIDGETS
