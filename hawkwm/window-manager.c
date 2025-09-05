#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <pthread.h>
#include <string.h>
#include <X11/Xlib.h>
#include <X11/Xatom.h>
#include <gtk/gtk.h>
#include <gtk/gtkx.h>
#include <gdk/gdkx.h>
#include "wm.h"

// Global variables
Display *display;
Window root, parent;
Window *children;
Window targetWin;
unsigned int nchildren;
char errorMessage[256] = "";
int WX, WY;
bool wm_running = false;
pthread_t wm_thread;  // Added thread handle
pthread_mutex_t wm_mutex = PTHREAD_MUTEX_INITIALIZER;

// Function declarations
void manageWindow(Window window, const char *title);
void* wm_init(void *arg);

void manageWindow(Window window, const char *title) {
    if (!display || window == None) {
        return;
    }
    
    XWindowAttributes attrs;
    if (XGetWindowAttributes(display, window, &attrs) == 0) {
        return;
    }
    
    unsigned long winidlong =  window;
    pthread_mutex_lock(&wm_mutex);
    targetWin = window;
    pthread_mutex_unlock(&wm_mutex);
    Window xwinid = (Window)winidlong;
    
    // Create main container
    GtkWidget *plug = gtk_plug_new((Window)window); // Bug Detected Here
    if (!plug) return;
    
    GtkWidget *panel = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
    
    // Title label (centered)
    GtkWidget *title_label = gtk_label_new(title ? title : "");
    gtk_label_set_ellipsize(GTK_LABEL(title_label), PANGO_ELLIPSIZE_END);
    
    // Controls container (right-aligned)
    GtkWidget *controls = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 2);
    
    // Window controls
    GtkWidget *minimize_btn = gtk_button_new_with_label("_");
    GtkWidget *maximize_btn = gtk_button_new_with_label("□");
    GtkWidget *close_btn = gtk_button_new_with_label("X");
    
    // Pack controls
    gtk_box_pack_end(GTK_BOX(controls), close_btn, FALSE, FALSE, 0);
    gtk_box_pack_end(GTK_BOX(controls), maximize_btn, FALSE, FALSE, 0);
    gtk_box_pack_end(GTK_BOX(controls), minimize_btn, FALSE, FALSE, 0);
    
    // Pack main panel
    gtk_box_pack_start(GTK_BOX(panel), title_label, TRUE, TRUE, 0);
    gtk_box_pack_end(GTK_BOX(panel), controls, FALSE, FALSE, 0);

    // Add panel to plug
    gtk_container_add(GTK_CONTAINER(plug), panel);

    // Check window state with proper error handling
    int title_bar_state = is_title_bar_showing(display, window);
    if (title_bar_state == 1) {
        gtk_widget_set_visible(panel, true);
    } else if (title_bar_state == 0) {
        gtk_widget_set_visible(panel, false);
    }
    
    gtk_widget_show_all(plug);
    
    int wX, wY;
    unsigned int wWidth, wHeight;
    unsigned int border_width;
    unsigned int depth;

    pthread_mutex_lock(&wm_mutex);
    if (XGetGeometry(display, targetWin, &root, &wX, &wY,
        &wWidth, &wHeight, &border_width, &depth) == true)
    {   
        pthread_mutex_unlock(&wm_mutex);
        if (is_window_maximized(display, targetWin) == 1) {
            int sCount = ScreenCount(display);
            int sNum;
            int dWidth = DisplayWidth(display, sNum);
            int dHeight = DisplayHeight(display, sNum);
            
            for (int i = 0; i < sCount; i++) {
                if (attrs.root == RootWindow(display, i)) {
                    sNum = i;
                    break;
                }
            }

            pthread_mutex_lock(&wm_mutex);
            XMoveResizeWindow(display, targetWin, 0, 0, dWidth, dHeight);
            pthread_mutex_unlock(&wm_mutex);
        }
        else {
            pthread_mutex_lock(&wm_mutex);
            WX = wX;
            WY = wY;
            pthread_mutex_unlock(&wm_mutex);

            pthread_mutex_lock(&wm_mutex);
            XMoveResizeWindow(display, targetWin, WX, WY, wWidth, wHeight);
            pthread_mutex_unlock(&wm_mutex);
        }
    } else {
        pthread_mutex_unlock(&wm_mutex);
    }

    pthread_mutex_lock(&wm_mutex);
    if(is_window_minimized(display, targetWin) == 1) {
        XUnmapWindow(display, targetWin);
    }
    else if(is_window_minimized(display, targetWin) == -1) {
        perror("Unable To Minimize Window");
    }
    else {
        XMapWindow(display, targetWin);
    }
    pthread_mutex_unlock(&wm_mutex);

    pthread_mutex_lock(&wm_mutex);
    if(is_window_closeable(display, targetWin) == 1) {
        gtk_widget_set_visible(close_btn, true);
        gtk_widget_set_sensitive(close_btn, true);
    }
    else if(is_window_closeable(display, targetWin) == 0) {
        gtk_widget_set_visible(close_btn, false);
        gtk_widget_set_sensitive(close_btn, false);
    }
    else {
        perror("Unable To Get Window Is Closeable");
    }
    pthread_mutex_unlock(&wm_mutex);

    pthread_mutex_lock(&wm_mutex);
    if(is_window_maximizable(display, targetWin) == 1) {
        gtk_widget_set_visible(maximize_btn, true);
        gtk_widget_set_sensitive(maximize_btn, true);
    }
    else if(is_window_maximizable(display, targetWin) == 0) {
        gtk_widget_set_visible(maximize_btn, false);
        gtk_widget_set_sensitive(maximize_btn, false);
    }
    else {
        perror("Unable To Get Window Is Maximizable");
    }
    pthread_mutex_unlock(&wm_mutex);

    pthread_mutex_lock(&wm_mutex);
    if(is_window_minimizable(display, targetWin) == 1) {
        gtk_widget_set_visible(minimize_btn, true);
        gtk_widget_set_sensitive(minimize_btn, true);
    }
    else if(is_window_minimizable(display, targetWin) == 0) {
        gtk_widget_set_visible(minimize_btn, false);
        gtk_widget_set_sensitive(minimize_btn, false);
    }
    else {
        perror("Unable To Get Window Is Minimizable");
    }
    pthread_mutex_unlock(&wm_mutex);

    g_signal_connect(close_btn, "clicked", G_CALLBACK(close_window), NULL);
    g_signal_connect(maximize_btn, "clicked", G_CALLBACK(maximize_window), NULL);
    g_signal_connect(minimize_btn, "clicked", G_CALLBACK(minimize_window), NULL);
}

int is_window_maximized(Display *display, Window window) {
    Atom wm_state_atom;
    Atom maximized_horz_atom;
    Atom maximized_vert_atom;

    // Get the atoms for the properties we are interested in.
    wm_state_atom = XInternAtom(display, "_NET_WM_STATE", False);
    maximized_horz_atom = XInternAtom(display, "_NET_WM_STATE_MAXIMIZED_HORZ", False);
    maximized_vert_atom = XInternAtom(display, "_NET_WM_STATE_MAXIMIZED_VERT", False);

    Atom actual_type;
    int actual_format;
    unsigned long nitems, bytes_after;
    unsigned char *prop_return = NULL;

    // Get the _NET_WM_STATE property of the window.
    pthread_mutex_lock(&wm_mutex);
    if (XGetWindowProperty(display, window, wm_state_atom, 0, 1024, False,
                           XA_ATOM, &actual_type, &actual_format,
                           &nitems, &bytes_after, &prop_return) != Success) {
        pthread_mutex_unlock(&wm_mutex);
        return 0;
    }
    pthread_mutex_unlock(&wm_mutex);

    if (actual_type != XA_ATOM || prop_return == NULL) {
        if (prop_return) XFree(prop_return);
        return 0;
    }

    int is_max_horz = 0;
    int is_max_vert = 0;

    // Iterate through the list of atoms to find the maximized states.
    Atom *atoms = (Atom *)prop_return;
    for (unsigned long i = 0; i < nitems; ++i) {
        if (atoms[i] == maximized_horz_atom) {
            is_max_horz = 1;
        }
        if (atoms[i] == maximized_vert_atom) {
            is_max_vert = 1;
        }
    }

    XFree(prop_return);

    // A window is considered maximized if both horizontal and vertical states are set.
    return is_max_horz && is_max_vert;
}

int is_window_minimized(Display *display, Window window) {
    Atom wm_state_atom, wm_hidden_atom, actual_type;
    int actual_format;
    unsigned long nitems, bytes_after;
    unsigned char *states = NULL;
    int is_minimized = 0;
    
    // Get the atom for _NET_WM_STATE
    wm_state_atom = XInternAtom(display, "_NET_WM_STATE", False);
    if (wm_state_atom == None) {
        fprintf(stderr, "Could not intern _NET_WM_STATE atom.\n");
        return -1;
    }

    // Get the atom for _NET_WM_STATE_HIDDEN
    wm_hidden_atom = XInternAtom(display, "_NET_WM_STATE_HIDDEN", False);
    if (wm_hidden_atom == None) {
        fprintf(stderr, "Could not intern _NET_WM_STATE_HIDDEN atom.\n");
        return -1;
    }

    // Get the _NET_WM_STATE property from the window
    pthread_mutex_lock(&wm_mutex);
    if (XGetWindowProperty(
        display, window, wm_state_atom, 0L, 1024L, False,
        XA_ATOM, &actual_type, &actual_format, &nitems, &bytes_after, &states
    ) == Success) {
        pthread_mutex_unlock(&wm_mutex);
        // Iterate through the list of states to find _NET_WM_STATE_HIDDEN
        if (actual_type == XA_ATOM && states != NULL) {
            Atom *state_atoms = (Atom *)states;
            for (unsigned long i = 0; i < nitems; ++i) {
                if (state_atoms[i] == wm_hidden_atom) {
                    is_minimized = 1;
                    break;
                }
            }
        }
        
        if (states) {
            XFree(states); // Free the memory allocated by XGetWindowProperty
        }
    } else {
        pthread_mutex_unlock(&wm_mutex);
        return -1; // XGetWindowProperty failed
    }
    
    return is_minimized;
}

int is_window_closeable(Display *display, Window window) {
    Atom wm_protocols, wm_delete_window;
    Atom net_wm_allowed, net_wm_action_close;
    Atom *protocols = NULL;
    Atom *allowed_actions = NULL;
    int protocols_count = 0;
    int allowed_count = 0;
    int status;
    int is_closeable = 0;

    // --- Check for WM_PROTOCOLS with WM_DELETE_WINDOW (ICCCM) ---
    wm_protocols = XInternAtom(display, "WM_PROTOCOLS", False);
    wm_delete_window = XInternAtom(display, "WM_DELETE_WINDOW", False);

    if (wm_protocols != None && wm_delete_window != None) {
        pthread_mutex_lock(&wm_mutex);
        status = XGetWMProtocols(display, window, &protocols, &protocols_count);
        pthread_mutex_unlock(&wm_mutex);
        if (status) {
            for (int i = 0; i < protocols_count; i++) {
                if (protocols[i] == wm_delete_window) {
                    is_closeable = 1;
                    break;
                }
            }
            if (protocols) XFree(protocols);
        }
    }

    // If already found to be closeable, the function stops here.
    if (is_closeable) return 1;

    // --- Check for _NET_WM_ALLOWED_ACTIONS with _NET_WM_ACTION_CLOSE (EWMH) ---
    net_wm_allowed = XInternAtom(display, "_NET_WM_ALLOWED_ACTIONS", False);
    net_wm_action_close = XInternAtom(display, "_NET_WM_ACTION_CLOSE", False);

    if (net_wm_allowed != None && net_wm_action_close != None) {
        Atom actual_type;
        int actual_format;
        unsigned long nitems, bytes_after;
        unsigned char *prop_ret = NULL;

        pthread_mutex_lock(&wm_mutex);
        if (XGetWindowProperty(
            display, window, net_wm_allowed, 0L, 1024L, False,
            XA_ATOM, &actual_type, &actual_format, &nitems, &bytes_after, &prop_ret
        ) == Success) {
            pthread_mutex_unlock(&wm_mutex);
            if (actual_type == XA_ATOM && prop_ret) {
                allowed_actions = (Atom *)prop_ret;
                allowed_count = (int)nitems;
                for (int i = 0; i < allowed_count; i++) {
                    if (allowed_actions[i] == net_wm_action_close) {
                        is_closeable = 1;
                        break;
                    }
                }
            }
            if (prop_ret) XFree(prop_ret);
        } else {
            pthread_mutex_unlock(&wm_mutex);
        }
    }

    return is_closeable;
}

int is_window_maximizable(Display *display, Window window) {
    Atom net_wm_allowed_atom, max_vert_atom, max_horz_atom, actual_type;
    int actual_format;
    unsigned long nitems, bytes_after;
    unsigned char *prop_ret = NULL;
    int is_maximizable = 0;
    
    // Get the atoms for the required properties
    net_wm_allowed_atom = XInternAtom(display, "_NET_WM_ALLOWED_ACTIONS", False);
    max_vert_atom = XInternAtom(display, "_NET_WM_ACTION_MAXIMIZE_VERT", False);
    max_horz_atom = XInternAtom(display, "_NET_WM_ACTION_MAXIMIZE_HORZ", False);

    if (net_wm_allowed_atom == None || max_vert_atom == None || max_horz_atom == None) {
        fprintf(stderr, "Error: Failed to intern required atoms.\n");
        return -1;
    }

    // Get the _NET_WM_ALLOWED_ACTIONS property from the window
    pthread_mutex_lock(&wm_mutex);
    if (XGetWindowProperty(
        display, window, net_wm_allowed_atom, 0L, 1024L, False,
        XA_ATOM, &actual_type, &actual_format, &nitems, &bytes_after, &prop_ret
    ) == Success) {
        pthread_mutex_unlock(&wm_mutex);
        // Iterate through the list of allowed actions to find maximization atoms
        if (actual_type == XA_ATOM && prop_ret != NULL) {
            Atom *actions = (Atom *)prop_ret;
            int has_vert = 0;
            int has_horz = 0;

            for (unsigned long i = 0; i < nitems; ++i) {
                if (actions[i] == max_vert_atom) {
                    has_vert = 1;
                }
                if (actions[i] == max_horz_atom) {
                    has_horz = 1;
                }
            }

            if (has_vert && has_horz) {
                is_maximizable = 1;
            }
        }
        
        if (prop_ret) {
            XFree(prop_ret); // Free the memory allocated by XGetWindowProperty
        }
    } else {
        pthread_mutex_unlock(&wm_mutex);
        return -1; // XGetWindowProperty failed
    }
    
    return is_maximizable;
}

int is_window_minimizable(Display *display, Window window) {
    Atom net_wm_allowed_atom, min_atom, actual_type;
    int actual_format;
    unsigned long nitems, bytes_after;
    unsigned char *prop_ret = NULL;
    int is_minimizable = 0;
    
    // Get the atoms for the required properties
    net_wm_allowed_atom = XInternAtom(display, "_NET_WM_ALLOWED_ACTIONS", False);
    min_atom = XInternAtom(display, "_NET_WM_ACTION_MINIMIZE", False);

    if (net_wm_allowed_atom == None || min_atom == None) {
        fprintf(stderr, "Error: Failed to intern required atoms.\n");
        return -1;
    }

    // Get the _NET_WM_ALLOWED_ACTIONS property from the window
    pthread_mutex_lock(&wm_mutex);
    if (XGetWindowProperty(
        display, window, net_wm_allowed_atom, 0L, 1024L, False,
        XA_ATOM, &actual_type, &actual_format, &nitems, &bytes_after, &prop_ret
    ) == Success) {
        pthread_mutex_unlock(&wm_mutex);
        // Iterate through the list of allowed actions to find the minimize atom
        if (actual_type == XA_ATOM && prop_ret != NULL) {
            Atom *actions = (Atom *)prop_ret;
            for (unsigned long i = 0; i < nitems; ++i) {
                if (actions[i] == min_atom) {
                    is_minimizable = 1;
                    break;
                }
            }
        }
        
        if (prop_ret) {
            XFree(prop_ret); // Free the memory allocated by XGetWindowProperty
        }
    } else {
        pthread_mutex_unlock(&wm_mutex);
        return -1; // XGetWindowProperty failed
    }
    
    return is_minimizable;
}

int is_title_bar_showing(Display *display, Window window) {
    if (!display || window == None) {
        return -1;
    }
    
    Atom frame_extents_atom = XInternAtom(display, "_NET_FRAME_EXTENTS", False);
    if (frame_extents_atom == None) {
        return -1;
    }

    Atom actual_type;
    int actual_format;
    unsigned long nitems, bytes_after;
    unsigned char *prop_ret = NULL;
    int is_title_bar = 0;
    
    int status = XGetWindowProperty(
        display, window, frame_extents_atom, 0L, 4L, False,
        XA_CARDINAL, &actual_type, &actual_format, &nitems, &bytes_after, &prop_ret
    );
    
    if (status == Success && prop_ret && actual_type == XA_CARDINAL && nitems == 4) {
        unsigned long *extents = (unsigned long *)prop_ret;
        is_title_bar = (extents[2] > 0) ? 1 : 0;
    }
    
    if (prop_ret) {
        XFree(prop_ret);
    }
    
    return is_title_bar;
}

void close_window(GtkWidget *widget, gpointer data) {
    (void)widget; (void)data;
    pthread_mutex_lock(&wm_mutex);
    if(is_window_closeable(display, targetWin) == 1)
        XDestroyWindow(display, targetWin);

    else if (is_window_closeable(display, targetWin) == 0) {
        pthread_mutex_unlock(&wm_mutex);
        return;
    }
    else {
        XDestroyWindow(display, targetWin);
        pthread_mutex_unlock(&wm_mutex);
    }
    pthread_mutex_unlock(&wm_mutex);
}

void maximize_window(GtkWidget *widget, gpointer data) {
    (void)widget; (void)data;
    XWindowAttributes attrs;
    pthread_mutex_lock(&wm_mutex);
    XGetWindowAttributes(display, targetWin, &attrs);
    pthread_mutex_unlock(&wm_mutex);
    
    pthread_mutex_lock(&wm_mutex);
    if(is_window_maximizable(display, targetWin) == 1) {
        if (is_window_maximized(display, targetWin) == 1) {
            int sCount = ScreenCount(display);
            int sNum;
            int dWidth = DisplayWidth(display, sNum);
            int dHeight = DisplayHeight(display, sNum);
            
            for (int i = 0; i < sCount; i++) {
                if (attrs.root == RootWindow(display, i)) {
                    sNum = i;
                    break;
                }
            }
            
            pthread_mutex_lock(&wm_mutex);
            XMoveResizeWindow(display, targetWin, 0, 0, dWidth, dHeight);
            pthread_mutex_unlock(&wm_mutex);
        }
        else {
            int wX, wY;
            unsigned int wWidth, wHeight;
            unsigned int border_width;
            unsigned int depth;
            
            pthread_mutex_lock(&wm_mutex);
            if (XGetGeometry(display, targetWin, &root, &wX, &wY,
                &wWidth, &wHeight, &border_width, &depth) == true)
            {
                pthread_mutex_unlock(&wm_mutex);
                XMoveResizeWindow(display, targetWin, WX, WY, wWidth, wHeight);
            } else {
                pthread_mutex_unlock(&wm_mutex);
            }
        }
    }
    else if(is_window_maximizable(display, targetWin) == 0) {
        pthread_mutex_unlock(&wm_mutex);
        return;
    }
    else {
        perror("Unable to determine if window is maximizable");
        pthread_mutex_unlock(&wm_mutex);
    }
    pthread_mutex_unlock(&wm_mutex);
}

void minimize_window(GtkWidget *widget, gpointer data) {
    (void)widget; (void)data;
    pthread_mutex_lock(&wm_mutex);
    if(is_window_minimizable(display, targetWin) == 1) {
        if (is_window_minimized(display, targetWin) == 1) {
            XMapWindow(display, targetWin);
        }
        else {
            XUnmapWindow(display, targetWin);
        }
    }
    else if(is_window_minimizable(display, targetWin) == 0) {
        pthread_mutex_unlock(&wm_mutex);
        return;
    }
    else {
        perror("Unable to determine if window is minimizable");
        pthread_mutex_unlock(&wm_mutex);
    }
    pthread_mutex_unlock(&wm_mutex);
}

void* wm_prog(void *arg) {
    (void)arg;
    
    pthread_mutex_lock(&wm_mutex);
    system("glxgears &");
    pthread_mutex_unlock(&wm_mutex);

    while (1) {
        pthread_mutex_lock(&wm_mutex);
        if (!wm_running) break;
        
        struct timespec ts = {0, 100000000}; // 0.1 second
        nanosleep(&ts, NULL);

        pthread_mutex_lock(&wm_mutex);
        root = DefaultRootWindow(display);
        pthread_mutex_unlock(&wm_mutex);

        pthread_mutex_lock(&wm_mutex);
        if (XQueryTree(display, root, &root, &parent, &children, &nchildren) == 0) {
            pthread_mutex_unlock(&wm_mutex);
            pthread_exit((void*)2);
        }
        pthread_mutex_unlock(&wm_mutex);

        for (unsigned int i = 0; i < nchildren; i++) {
            char *window_name = NULL;
            pthread_mutex_lock(&wm_mutex);
            if (XFetchName(display, children[i], &window_name) == True) {
                pthread_mutex_unlock(&wm_mutex);
                manageWindow(children[i], window_name);
                pthread_mutex_lock(&wm_mutex);
                XFree(window_name);
                pthread_mutex_unlock(&wm_mutex);
            } else {
                pthread_mutex_unlock(&wm_mutex);
                manageWindow(children[i], NULL);
            }
        }

        pthread_mutex_lock(&wm_mutex);
        XFree(children);
        pthread_mutex_unlock(&wm_mutex);
        XFlush(display);
    }
    return NULL;
}

int wminit() {
    gtk_init(NULL, NULL);

    pthread_mutex_lock(&wm_mutex);
    wm_running = true;
    pthread_mutex_unlock(&wm_mutex);

    pthread_mutex_lock(&wm_mutex);
    display = XOpenDisplay(NULL);
    pthread_mutex_unlock(&wm_mutex);
    if (!display) {
        fprintf(stderr, "Failed to connect to display\n");
        return 1;
    }

    pthread_t thread;
    int result = pthread_create(&thread, NULL, wm_prog, NULL);
    if (result != 0) {
        if(result == 1)
            fprintf(stderr, "Failed to create window manager thread\n");
        else if(result == 2) {
            fprintf(stderr, "Failed to query window tree\n");
        }
        pthread_mutex_lock(&wm_mutex);
        XCloseDisplay(display);
        pthread_mutex_unlock(&wm_mutex);
        return 1;
    }
    pthread_mutex_lock(&wm_mutex);
    wm_thread = thread;  // Store thread handle
    pthread_mutex_unlock(&wm_mutex);

    return 0;
}

void wmshutdown() {
    pthread_mutex_lock(&wm_mutex);
    wm_running = false;
    pthread_mutex_unlock(&wm_mutex);
    
    pthread_join(wm_thread, NULL);
    
    pthread_mutex_lock(&wm_mutex);
    XCloseDisplay(display);
    pthread_mutex_unlock(&wm_mutex);
}