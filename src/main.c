#include <unistd.h>
#include <sys/syscall.h>
#include <linux/reboot.h>
#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <gtk/gtk.h>
#include <hawkwm/wm.h>
#include <hawkdm/scall.h>
#include <sys/syscall.h>

#define messagePipeline "/tmp/.hawk-session/system"

static void activate(GtkApplication *app, gpointer user_data) {
    (void)user_data;
    
    GdkDisplay *gdisplay = gdk_display_get_default();
    if (!gdisplay) {
        g_error("Failed to connect to display");
        return;
    }
    
    // Create application window
    GtkWidget *window = gtk_application_window_new(app);
    gtk_window_set_title(GTK_WINDOW(window), "Hawk Session");
    
    // Remove window decorations
    gtk_window_set_decorated(GTK_WINDOW(window), FALSE);
    
    // Make window fullscreen
    gtk_window_fullscreen(GTK_WINDOW(window));
    
    // Prevent closing via window manager
    gtk_window_set_deletable(GTK_WINDOW(window), FALSE);
    
    gtk_widget_set_visible(window, TRUE);
}

/* int broadcastMessage(const char *message, const char *content) {
    (void)content; // Currently unused
    
    if (strcmp(message, "shutdown") == 0) {
        int result = system("sudo shutdown now");
        if (result == -1) {
            result = syscall(SYS_reboot,
                LINUX_REBOOT_MAGIC1,
                LINUX_REBOOT_MAGIC2,
                LINUX_REBOOT_CMD_POWER_OFF,
                NULL);
            
            if (result == -1) {
                perror("Unable to shutdown system");
                return 1;
            }
        }
        return 0;
    }
    
    if (strcmp(message, "restart") == 0) {
        int result = system("sudo reboot");
        if (result == -1) {
            result = syscall(SYS_reboot,
                LINUX_REBOOT_MAGIC1,
                LINUX_REBOOT_MAGIC2,
                LINUX_REBOOT_CMD_RESTART,
                NULL);
            
            if (result == -1) {
                perror("Unable to restart system");
                return 1;
            }
        }
        return 0;
    }
    
    return 1;
} */

int main(int argc, char *argv[]) {
    // Initialize window manager
    wminit();

    GtkApplication *app;
    int status;

    app = gtk_application_new("com.swiftos.hawk-session", G_APPLICATION_DEFAULT_FLAGS);
    g_signal_connect(app, "activate", G_CALLBACK(activate), NULL);
    
    status = g_application_run(G_APPLICATION(app), argc, argv);
    g_object_unref(app);
    
    return status;
}