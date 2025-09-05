#ifndef _HAWKWM_H
#define _HAWKWM_H

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

extern int wminit();
extern void wmshutdown();

// Window state functions
int is_window_maximized(Display *display, Window window);
int is_window_minimized(Display *display, Window window);
int is_window_closeable(Display *display, Window window);
int is_window_maximizable(Display *display, Window window);
int is_window_minimizable(Display *display, Window window);
int is_title_bar_showing(Display *display, Window window);

// Window control functions
void close_window(GtkWidget *widget, gpointer data);
void maximize_window(GtkWidget *widget, gpointer data);
void minimize_window(GtkWidget *widget, gpointer data);

#endif