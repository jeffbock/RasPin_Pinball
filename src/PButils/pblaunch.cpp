/*
 * PBLaunch - GUI based launcher for Pinball on Raspberry Pi
 * Uses GTK4 for the user interface
 */

#include <gtk/gtk.h>
#include <string>
#include <cstring>
#include <vector>
#include <algorithm>
#include <filesystem>
#include <thread>
#include <chrono>
#include <sys/wait.h>
#include <unistd.h>

namespace fs = std::filesystem;

// Global main loop reference
static GMainLoop *main_loop = nullptr;

// Helper function to show simple message dialogs (GTK4 compatible)
void show_message_dialog(GtkWindow *parent, const char *title, const char *message) {
    GtkWidget *dialog = gtk_window_new();
    gtk_window_set_title(GTK_WINDOW(dialog), title);
    gtk_window_set_transient_for(GTK_WINDOW(dialog), parent);
    gtk_window_set_modal(GTK_WINDOW(dialog), TRUE);
    gtk_window_set_default_size(GTK_WINDOW(dialog), 400, 150);
    
    GtkWidget *box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 10);
    gtk_widget_set_margin_start(box, 20);
    gtk_widget_set_margin_end(box, 20);
    gtk_widget_set_margin_top(box, 20);
    gtk_widget_set_margin_bottom(box, 20);
    
    GtkWidget *label = gtk_label_new(message);
    gtk_label_set_wrap(GTK_LABEL(label), TRUE);
    gtk_label_set_max_width_chars(GTK_LABEL(label), 50);
    gtk_box_append(GTK_BOX(box), label);
    
    GtkWidget *button = gtk_button_new_with_label("OK");
    gtk_widget_set_halign(button, GTK_ALIGN_CENTER);
    g_signal_connect_swapped(button, "clicked", G_CALLBACK(gtk_window_destroy), dialog);
    gtk_box_append(GTK_BOX(box), button);
    
    gtk_window_set_child(GTK_WINDOW(dialog), box);
    gtk_window_present(GTK_WINDOW(dialog));
}

class RasPinLauncher {
private:
    GtkWidget *window;
    GtkWidget *status_label;
    GtkWidget *play_button;
    GtkWidget *update_button;
    GtkWidget *path_button;
    GtkWidget *stop_button;
    
    std::string pinball_path;
    std::string project_root;
    
public:
    RasPinLauncher() {
        window = nullptr;
        status_label = nullptr;
        play_button = nullptr;
        update_button = nullptr;
        path_button = nullptr;
        stop_button = nullptr;
    }
    
    void find_pinball() {
        // Search common locations for a project directory containing the Pinball executable.
        // Supports both Raspberry Pi (build/raspi/{debug,release}/Pinball) and
        // Debian (build/debian/{debug,release}/Pinball_DebianOS) directory structures.
        std::vector<std::string> search_paths = {
            std::string(getenv("HOME") ? getenv("HOME") : "") + "/Dev",
            std::string(getenv("HOME") ? getenv("HOME") : ""),
            std::string(getenv("HOME") ? getenv("HOME") : "") + "/Documents",
            std::string(getenv("HOME") ? getenv("HOME") : "") + "/Games",
            "/opt"
        };

        // Candidate sub-paths within a project root, in priority order
        struct Candidate { const char* sub_path; const char* exe_name; };
        static const Candidate candidates[] = {
            { "build/raspi/release",  "Pinball"          },
            { "build/raspi/debug",    "Pinball"          },
            { "build/debian/release", "Pinball_DebianOS" },
            { "build/debian/debug",   "Pinball_DebianOS" },
        };

        for (const auto& base : search_paths) {
            if (base.empty() || !fs::exists(base)) continue;
            try {
                for (const auto& entry : fs::recursive_directory_iterator(
                        base, fs::directory_options::skip_permission_denied)) {
                    if (!entry.is_directory()) continue;

                    // Limit recursion depth to avoid very deep traversal
                    std::string ep = entry.path().string();
                    size_t depth = std::count(ep.begin() + base.length(), ep.end(), '/');
                    if (depth > 5) continue;

                    // Check if this directory could be a project root by probing
                    // each candidate executable path beneath it
                    for (const auto& cand : candidates) {
                        std::string exe_path = ep + "/" + cand.sub_path + "/" + cand.exe_name;
                        if (fs::exists(exe_path) && fs::is_regular_file(exe_path) &&
                                access(exe_path.c_str(), X_OK) == 0) {
                            pinball_path = exe_path;
                            project_root = ep;
                            return;
                        }
                    }
                }
            } catch (const std::exception&) {
                // Continue to the next search path
            }
        }

        // Not found - user can set the path manually via Update Path
    }
    
    std::string get_status_text() const {
        if (!pinball_path.empty()) {
            return "Path: " + pinball_path;
        } else {
            return "Path: Not set";
        }
    }
    
    void update_status() {
        gtk_label_set_text(GTK_LABEL(status_label), get_status_text().c_str());
    }
    
    static void on_play_clicked(GtkWidget *widget __attribute__((unused)), gpointer data) {
        RasPinLauncher *app = static_cast<RasPinLauncher*>(data);
        app->play_pinball();
    }
    
    void play_pinball() {
        if (pinball_path.empty() || !fs::exists(pinball_path)) {
            show_message_dialog(GTK_WINDOW(window), "Error", 
                "Pinball executable not found!\nPlease set the path using 'Update Path'.");
            return;
        }
        
        // Use the project root as working directory (where assets are relative to),
        // falling back to the executable's parent directory
        std::string working_dir;
        if (!project_root.empty()) {
            working_dir = project_root;
        } else {
            working_dir = fs::path(pinball_path).parent_path().string();
        }
        
        // Launch Pinball as a completely separate process (double fork to detach)
        pid_t pid = fork();
        if (pid == 0) {
            // First child process - fork again to detach from parent
            pid_t pid2 = fork();
            if (pid2 == 0) {
                // Second child process - this will become the Pinball process
                // Close standard file descriptors to fully detach
                close(STDIN_FILENO);
                close(STDOUT_FILENO);
                close(STDERR_FILENO);
                
                // Create new session to detach from terminal
                setsid();
                
                // Change to working directory and launch Pinball
                chdir(working_dir.c_str());
                execl(pinball_path.c_str(), pinball_path.c_str(), nullptr);
                exit(1); // If exec fails
            }
            // First child exits immediately
            exit(0);
        } else if (pid > 0) {
            // Parent process - wait for first child to exit, then close launcher
            waitpid(pid, nullptr, 0);
            
            // Close the launcher
            if (main_loop && g_main_loop_is_running(main_loop)) {
                g_main_loop_quit(main_loop);
            }
        } else {
            // Fork failed
            show_message_dialog(GTK_WINDOW(window), "Error", "Failed to launch Pinball!");
        }
    }
    
    static gboolean update_progress_bar(gpointer data) {
        GtkProgressBar *progress = GTK_PROGRESS_BAR(data);
        
        // Check if widget still exists and is valid
        if (!GTK_IS_PROGRESS_BAR(progress)) {
            return FALSE;
        }
        
        double fraction = gtk_progress_bar_get_fraction(progress);
        
        if (fraction >= 1.0) {
            return FALSE; // Stop the timer
        }
        
        fraction += 0.01; // Increment by 1% (100 steps for 5 seconds)
        gtk_progress_bar_set_fraction(progress, fraction);
        
        char text[32];
        snprintf(text, sizeof(text), "%d%%", (int)(fraction * 100));
        gtk_progress_bar_set_text(progress, text);
        
        return TRUE; // Continue the timer
    }
    
    struct UpdateDialogData {
        GtkWidget *dialog;
        GtkWidget *label;
        GtkWidget *button;
        GtkWidget *progress;
        guint progress_timeout_id;
        guint complete_timeout_id;
    };
    
    static void on_update_dialog_destroy(GtkWidget *widget __attribute__((unused)), gpointer data) {
        UpdateDialogData *udata = static_cast<UpdateDialogData*>(data);
        
        // Cancel any running timeouts
        if (udata->progress_timeout_id > 0) {
            g_source_remove(udata->progress_timeout_id);
            udata->progress_timeout_id = 0;
        }
        if (udata->complete_timeout_id > 0) {
            g_source_remove(udata->complete_timeout_id);
            udata->complete_timeout_id = 0;
        }
        
        delete udata;
    }
    
    static gboolean enable_close_button(gpointer data) {
        UpdateDialogData *udata = static_cast<UpdateDialogData*>(data);
        
        // Check if widgets still exist
        if (!GTK_IS_WIDGET(udata->button) || !GTK_IS_LABEL(udata->label)) {
            return FALSE;
        }
        
        gtk_widget_set_sensitive(udata->button, TRUE);
        gtk_label_set_text(GTK_LABEL(udata->label), "Update complete!");
        udata->complete_timeout_id = 0; // Mark as completed
        return FALSE;
    }
    
    static void on_update_clicked(GtkWidget *widget __attribute__((unused)), gpointer data) {
        RasPinLauncher *app = static_cast<RasPinLauncher*>(data);
        app->update_pinball();
    }
    
    void update_pinball() {
        // Create progress dialog
        GtkWidget *dialog = gtk_window_new();
        gtk_window_set_title(GTK_WINDOW(dialog), "Updating Pinball");
        gtk_window_set_transient_for(GTK_WINDOW(dialog), GTK_WINDOW(window));
        gtk_window_set_modal(GTK_WINDOW(dialog), TRUE);
        gtk_window_set_default_size(GTK_WINDOW(dialog), 400, 150);
        gtk_window_set_resizable(GTK_WINDOW(dialog), FALSE);
        
        GtkWidget *main_box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 10);
        gtk_widget_set_margin_start(main_box, 20);
        gtk_widget_set_margin_end(main_box, 20);
        gtk_widget_set_margin_top(main_box, 20);
        gtk_widget_set_margin_bottom(main_box, 20);
        
        // Status label
        GtkWidget *label = gtk_label_new("Downloading update...");
        gtk_box_append(GTK_BOX(main_box), label);
        
        // Progress bar
        GtkWidget *progress = gtk_progress_bar_new();
        gtk_progress_bar_set_show_text(GTK_PROGRESS_BAR(progress), TRUE);
        gtk_progress_bar_set_text(GTK_PROGRESS_BAR(progress), "0%");
        gtk_box_append(GTK_BOX(main_box), progress);
        
        // Close button
        GtkWidget *close_button = gtk_button_new_with_label("Close");
        gtk_widget_set_sensitive(close_button, FALSE);
        g_signal_connect_swapped(close_button, "clicked", G_CALLBACK(gtk_window_destroy), dialog);
        gtk_box_append(GTK_BOX(main_box), close_button);
        
        gtk_window_set_child(GTK_WINDOW(dialog), main_box);
        
        // Create data structure to track timeouts and widgets
        UpdateDialogData *udata = new UpdateDialogData{dialog, label, close_button, progress, 0, 0};
        
        // Connect cleanup handler when dialog is destroyed
        g_signal_connect(dialog, "destroy", G_CALLBACK(on_update_dialog_destroy), udata);
        
        // Start progress updates (50ms * 100 = 5000ms = 5 seconds)
        udata->progress_timeout_id = g_timeout_add(50, update_progress_bar, progress);
        
        // Enable close button after 5 seconds
        udata->complete_timeout_id = g_timeout_add(5000, enable_close_button, udata);
        
        gtk_window_present(GTK_WINDOW(dialog));
    }
    
    static void on_path_clicked(GtkWidget *widget __attribute__((unused)), gpointer data) {
        RasPinLauncher *app = static_cast<RasPinLauncher*>(data);
        app->update_path();
    }
    
    static void on_file_selected(GtkWidget *button, gpointer data) {
        RasPinLauncher *app = static_cast<RasPinLauncher*>(data);
        GtkWidget *dialog = gtk_widget_get_ancestor(button, GTK_TYPE_WINDOW);
        GtkWidget *entry = GTK_WIDGET(g_object_get_data(G_OBJECT(button), "entry"));
        
        const char *filename = gtk_editable_get_text(GTK_EDITABLE(entry));
        if (!filename || strlen(filename) == 0) {
            show_message_dialog(GTK_WINDOW(app->window), "Error", "No file selected!");
            return;
        }
        
        // Check if file exists
        if (!fs::exists(filename)) {
            show_message_dialog(GTK_WINDOW(app->window), "Error", "Selected file does not exist!");
            return;
        }
        
        // Check if executable
        if (access(filename, X_OK) != 0) {
            show_message_dialog(GTK_WINDOW(app->window), "Warning", "Selected file is not executable!");
            gtk_window_destroy(GTK_WINDOW(dialog));
            return;
        }
        
        // Update the path
        app->pinball_path = filename;
        
        // Detect project root from the standard build directory structure:
        // <project_root>/build/{raspi,debian}/{debug,release}/Pinball[_DebianOS]
        fs::path exe_fs(filename);
        fs::path platform_dir = exe_fs.parent_path().parent_path(); // .../build/raspi|debian
        if (platform_dir.parent_path().filename() == "build" &&
                (platform_dir.filename() == "raspi" || platform_dir.filename() == "debian")) {
            app->project_root = platform_dir.parent_path().parent_path().string();
        } else {
            app->project_root.clear();
        }
        
        app->update_status();
        
        char msg[512];
        snprintf(msg, sizeof(msg), "Pinball path updated to:\n%s", app->pinball_path.c_str());
        show_message_dialog(GTK_WINDOW(app->window), "Path Updated", msg);
        
        gtk_window_destroy(GTK_WINDOW(dialog));
    }
    
    static void on_native_response(GtkNativeDialog *native, int response, gpointer data) {
        GtkWidget *entry = GTK_WIDGET(data);
        
        if (response == GTK_RESPONSE_ACCEPT) {
            GtkFileChooser *chooser = GTK_FILE_CHOOSER(native);
            GFile *file = gtk_file_chooser_get_file(chooser);
            char *filename = g_file_get_path(file);
            gtk_editable_set_text(GTK_EDITABLE(entry), filename);
            g_free(filename);
            g_object_unref(file);
        }
        
        g_object_unref(native);
    }
    
    static void on_file_browse(GtkWidget *button, gpointer data) {
        GtkWidget *entry = GTK_WIDGET(data);
        
        // Create file chooser native dialog
        GtkFileChooserNative *filechooser = gtk_file_chooser_native_new(
            "Select Pinball Executable",
            GTK_WINDOW(gtk_widget_get_ancestor(button, GTK_TYPE_WINDOW)),
            GTK_FILE_CHOOSER_ACTION_OPEN,
            "_Open",
            "_Cancel"
        );
        
        // Set initial folder if entry has content
        const char *current = gtk_editable_get_text(GTK_EDITABLE(entry));
        if (current && strlen(current) > 0 && fs::exists(current)) {
            std::string parent = fs::path(current).parent_path().string();
            GFile *folder = g_file_new_for_path(parent.c_str());
            gtk_file_chooser_set_current_folder(GTK_FILE_CHOOSER(filechooser), folder, nullptr);
            g_object_unref(folder);
        }
        
        g_signal_connect(filechooser, "response", G_CALLBACK(on_native_response), entry);
        gtk_native_dialog_show(GTK_NATIVE_DIALOG(filechooser));
    }
    
    void update_path() {
        // Create custom file selection dialog
        GtkWidget *dialog = gtk_window_new();
        gtk_window_set_title(GTK_WINDOW(dialog), "Select Pinball Executable");
        gtk_window_set_transient_for(GTK_WINDOW(dialog), GTK_WINDOW(window));
        gtk_window_set_modal(GTK_WINDOW(dialog), TRUE);
        gtk_window_set_default_size(GTK_WINDOW(dialog), 500, 150);
        
        GtkWidget *box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 10);
        gtk_widget_set_margin_start(box, 20);
        gtk_widget_set_margin_end(box, 20);
        gtk_widget_set_margin_top(box, 20);
        gtk_widget_set_margin_bottom(box, 20);
        
        GtkWidget *label = gtk_label_new("Enter path to pinball executable:");
        gtk_box_append(GTK_BOX(box), label);
        
        GtkWidget *hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 5);
        GtkWidget *entry = gtk_entry_new();
        gtk_widget_set_hexpand(entry, TRUE);
        if (!pinball_path.empty()) {
            gtk_editable_set_text(GTK_EDITABLE(entry), pinball_path.c_str());
        }
        gtk_box_append(GTK_BOX(hbox), entry);
        
        GtkWidget *browse = gtk_button_new_with_label("Browse...");
        g_signal_connect(browse, "clicked", G_CALLBACK(on_file_browse), entry);
        gtk_box_append(GTK_BOX(hbox), browse);
        gtk_box_append(GTK_BOX(box), hbox);
        
        GtkWidget *button_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 10);
        gtk_widget_set_halign(button_box, GTK_ALIGN_CENTER);
        
        GtkWidget *ok_button = gtk_button_new_with_label("OK");
        g_object_set_data(G_OBJECT(ok_button), "entry", entry);
        g_signal_connect(ok_button, "clicked", G_CALLBACK(on_file_selected), this);
        gtk_box_append(GTK_BOX(button_box), ok_button);
        
        GtkWidget *cancel_button = gtk_button_new_with_label("Cancel");
        g_signal_connect_swapped(cancel_button, "clicked", G_CALLBACK(gtk_window_destroy), dialog);
        gtk_box_append(GTK_BOX(button_box), cancel_button);
        
        gtk_box_append(GTK_BOX(box), button_box);
        
        gtk_window_set_child(GTK_WINDOW(dialog), box);
        gtk_window_present(GTK_WINDOW(dialog));
    }
    
    static void on_stop_clicked(GtkWidget *widget __attribute__((unused)), gpointer data) {
        RasPinLauncher *app = static_cast<RasPinLauncher*>(data);
        app->stop_pinball();
    }

    void stop_pinball() {
        // Kill any running Pinball process by exact name (covers both Raspi and Debian builds)
        static const char* exe_names[] = { "Pinball", "Pinball_DebianOS", nullptr };
        bool killed = false;
        for (const char** name = exe_names; *name; ++name) {
            pid_t pid = fork();
            if (pid == 0) {
                execlp("pkill", "pkill", "-x", *name, nullptr);
                exit(1);
            } else if (pid > 0) {
                int status;
                waitpid(pid, &status, 0);
                if (WIFEXITED(status) && WEXITSTATUS(status) == 0) {
                    killed = true;
                }
            }
        }
        if (killed) {
            show_message_dialog(GTK_WINDOW(window), "Stop Pinball", "Pinball has been stopped.");
        } else {
            show_message_dialog(GTK_WINDOW(window), "Stop Pinball", "Pinball is not currently running.");
        }
    }

    static gboolean on_window_close(GtkWindow *window __attribute__((unused)), gpointer data __attribute__((unused))) {
        if (main_loop && g_main_loop_is_running(main_loop)) {
            g_main_loop_quit(main_loop);
        }
        return FALSE;
    }
    
    void create_ui() {
        // Create main window
        window = gtk_window_new();
        gtk_window_set_title(GTK_WINDOW(window), "RasPin Launcher");
        gtk_window_set_default_size(GTK_WINDOW(window), 550, 200);
        gtk_window_set_resizable(GTK_WINDOW(window), FALSE);
        
        // Set window icon
        gtk_window_set_icon_name(GTK_WINDOW(window), "launch");
        
        g_signal_connect(window, "close-request", G_CALLBACK(on_window_close), nullptr);
        
        // Main container
        GtkWidget *vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 10);
        gtk_widget_set_margin_start(vbox, 20);
        gtk_widget_set_margin_end(vbox, 20);
        gtk_widget_set_margin_top(vbox, 15);
        gtk_widget_set_margin_bottom(vbox, 15);
        
        // Title label
        GtkWidget *title = gtk_label_new(nullptr);
        gtk_label_set_markup(GTK_LABEL(title), "<big><b>Dragons of Destiny</b></big>");
        gtk_box_append(GTK_BOX(vbox), title);
        
        // Status label
        status_label = gtk_label_new(get_status_text().c_str());
        gtk_label_set_wrap(GTK_LABEL(status_label), TRUE);
        gtk_label_set_max_width_chars(GTK_LABEL(status_label), 60);
        gtk_box_append(GTK_BOX(vbox), status_label);
        
        // Add spacer to push buttons down
        GtkWidget *spacer = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
        gtk_widget_set_vexpand(spacer, TRUE);
        gtk_box_append(GTK_BOX(vbox), spacer);
        
        // Button box
        GtkWidget *button_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 10);
        gtk_widget_set_hexpand(button_box, TRUE);
        gtk_box_set_homogeneous(GTK_BOX(button_box), TRUE);
        gtk_box_append(GTK_BOX(vbox), button_box);
        
        // Play button
        play_button = gtk_button_new_with_label("Play Pinball");
        g_signal_connect(play_button, "clicked", G_CALLBACK(on_play_clicked), this);
        gtk_box_append(GTK_BOX(button_box), play_button);
        
        // Update button
        update_button = gtk_button_new_with_label("Update Pinball");
        g_signal_connect(update_button, "clicked", G_CALLBACK(on_update_clicked), this);
        gtk_box_append(GTK_BOX(button_box), update_button);
        
        // Path button
        path_button = gtk_button_new_with_label("Update Path");
        g_signal_connect(path_button, "clicked", G_CALLBACK(on_path_clicked), this);
        gtk_box_append(GTK_BOX(button_box), path_button);

        // Stop button
        stop_button = gtk_button_new_with_label("Stop Pinball");
        g_signal_connect(stop_button, "clicked", G_CALLBACK(on_stop_clicked), this);
        gtk_box_append(GTK_BOX(button_box), stop_button);

        gtk_window_set_child(GTK_WINDOW(window), vbox);
        gtk_window_present(GTK_WINDOW(window));
    }
    
    void run() {
        // GTK4 uses GtkApplication, but for simplicity we'll use a GLib main loop
        main_loop = g_main_loop_new(nullptr, FALSE);
        g_main_loop_run(main_loop);
        g_main_loop_unref(main_loop);
        main_loop = nullptr;
    }
};

int main(int argc __attribute__((unused)), char *argv[] __attribute__((unused))) {
    // Disable accessibility bus to avoid warnings when running without a11y support
    g_setenv("GTK_A11Y", "none", TRUE);
    
    // Set application name for proper icon association
    g_set_prgname("pblaunch");
    g_set_application_name("RasPin Launcher");
    
    gtk_init();
    
    RasPinLauncher app;
    app.create_ui();
    app.find_pinball();
    app.update_status(); // Update the UI after searching for pinball
    app.run();
    
    return 0;
}
