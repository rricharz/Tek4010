/*
 *
 * main.c
 * provides a simple framework for basic drawing with cairo and gtk+ 3.0
 * 
 * Version adapted for tek4010
 * 
 * Copyright 2016,2019  rricharz
 *
 * https://github.com/rricharz/Tek4010
 * 
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
 * MA 02110-1301, USA.
 * 
 * 
 */
 
#define TIME_INTERVAL       35              // time interval for timer function in msec (after last refresh)
 
#define GDK_DISABLE_DEPRECATION_WARNINGS

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <cairo.h>
#include <gtk/gtk.h>

#include "main.h"
#include "tube.h"

extern FILE *putKeys;

char *windowName;

static int global_firstcall;

extern int argFull;
extern int argFullV;
extern int argARDS;

static GtkWidget *window;
int windowWidth;
int windowHeight;
static double aspectRatio;
static int windowHeightOffset = 0;
static int windowWidthOffset = 0;

guint global_timeout_ref;

extern int tube_doClearPersistent;

static void do_drawing(cairo_t *, GtkWidget *);

static gboolean on_draw_event(GtkWidget *widget, cairo_t *cr, 
    gpointer user_data)
{  
	do_drawing(cr, widget);  
	return FALSE;
}

static gboolean on_timer_event(GtkWidget *widget)
{
	if (tube_on_timer_event())
                gtk_widget_queue_draw(widget);
	return TRUE;
}

static gboolean clicked(GtkWidget *widget, GdkEventButton *event, gpointer user_data)
{
    if (tube_clicked(event->button, event->x - windowWidthOffset, event->y - windowHeightOffset))
                gtk_widget_queue_draw(widget);
    return TRUE;
}

static void on_quit_event()
{
	tube_quit();
	gtk_main_quit();
        exit(0);
}

static void do_drawing(cairo_t *cr, GtkWidget *widget)
{
        static cairo_surface_t *permanent_surface, *temporary_surface;
        
        g_source_remove(global_timeout_ref);    // stop timer, in case do_drawing takes too long
	
	if (global_firstcall) {
                // force aspect ratio by making black stripes at left and right, or top and bottom
                gtk_window_get_size(GTK_WINDOW(window), &windowWidth, &windowHeight);
                // gtk_window_set_resizable(GTK_WINDOW(window), 0); // do not allow further resizing
                
                if ((windowWidth != 1024) || (windowHeight != 768)) {
                    if (windowWidth > (int)((double)windowHeight * aspectRatio + 0.5)) {
                        windowWidthOffset = (windowWidth - (int)((double)windowHeight * aspectRatio)) / 2;
                        windowWidth = (int)((double)windowHeight * aspectRatio + 0.5);
                    }
                    if (windowHeight > (int)((double)windowWidth / aspectRatio + 0.5) ) {
                        windowHeightOffset = (windowHeight - (int)((double)windowWidth / aspectRatio)) / 2;
                        windowHeight = (int)((double)windowWidth / aspectRatio + 0.5);
                    }
                }
                printf("Window dimensions: %d x %d\n", windowWidth, windowHeight);

		permanent_surface = cairo_surface_create_similar(cairo_get_target(cr),
			CAIRO_CONTENT_COLOR, windowWidth, windowHeight);
                temporary_surface = cairo_surface_create_similar(cairo_get_target(cr),
			CAIRO_CONTENT_COLOR_ALPHA, windowWidth, windowHeight);
                        
                if (argHideCursor) { // hide cursor (does not allow GIN mode)
                        GdkCursor* Cursor = gdk_cursor_new(GDK_BLANK_CURSOR);
                        GdkWindow* win = gtk_widget_get_window(window);
                        gdk_window_set_cursor((win), Cursor);
                }
	}
	
	cairo_t *permanent_cr = cairo_create(permanent_surface);
        cairo_t *temporary_cr = cairo_create(temporary_surface);
        if ((permanent_cr == NULL) || (temporary_cr == NULL)) {
                printf("Cannot create drawing surfaces\n");
                exit(1);
        }
        if (argARDS)
                ards_draw(permanent_cr, temporary_cr, global_firstcall);
        else
                tek4010_draw(permanent_cr, temporary_cr, global_firstcall);
	global_firstcall = FALSE;

	cairo_set_source_surface(cr, permanent_surface, windowWidthOffset, windowHeightOffset);
	cairo_paint(cr);
        cairo_set_source_surface(cr, temporary_surface, windowWidthOffset, windowHeightOffset);
	cairo_paint(cr);
         
	cairo_destroy(permanent_cr);
        cairo_destroy(temporary_cr);
        global_timeout_ref = g_timeout_add(TIME_INTERVAL, (GSourceFunc) on_timer_event,
                                                (gpointer) window);
}

static void on_key_press(GtkWidget *widget, GdkEventKey *event, gpointer user_data)
{
        int ch;
        // printf("key pressed, state =%04X, keyval=%04X, isGinMode = %d\r\n", event->state, event->keyval, isGinMode);
        
        if ((event->keyval == 0xFF50) ||        // "home" key
                (event->keyval == 0xFF55) ||    // "page up" key
                (event->keyval == 0xFF56))      // "page down" key        
        {
                tube_doClearPersistent = 1;
                gtk_widget_queue_draw(widget);
                return;
        }
        
        // control keys
        else if ((event->keyval >= 0xFF00) && (event->keyval <= 0xFF1F))
                ch = event->keyval & 0x1F;
        else if (event->state & GDK_CONTROL_MASK) {
                if ((event->keyval == 0xFF51) ||    // "<ctrl>left arrow" key
                    (event->keyval == 0xFF52)) {    // "<ctrl>up arrow" key
                        tube_doClearPersistent = 1;
                        gtk_widget_queue_draw(widget);
                        return;
                }
                else if (event->keyval == 0x0077) { // "<ctrl>w" makes screendump
                        system("scrot --focussed");
                        return;
                }
                else if (event->keyval == 0x0071) { // "<ctrl>q" quits tek4010
                        on_quit_event();
                        return;
                }
                else if (argAPL && (event->keyval == 0x006E)) { // "<ctrl>n" switch to alternative character set
                        aplMode = 1;
                        // printf("Setting APL mode to 1 from keyboard\n");
                        return;
                }
                else if (argAPL && (event->keyval == 0x006F)) { // "<ctrl>o" switch to normalcharacter set
                        aplMode = 0;
                        // printf("Setting APL mode to 0 from keyboard\n");
                        return;
                }                
                else
                        ch = event->keyval & 0x1F;
        }
        else if (event->keyval == 0xFF52) ch = 16;  // arrow up for history up
        else if (event->keyval == 0xFF54) ch = 14;  // arrow down for history down
        
        else if ((event->state & GDK_MOD1_MASK) && (aplMode)) {   // alt key
                printf("alt key, ch = %4X\n", event->keyval & 0x7F);
                ch = (event->keyval & 0x7F) + 128;
        }
        
        // normal keys
        else if ((event->keyval >= 0x0020) && (event->keyval <= 0x007F))
                ch = event->keyval & 0x7F;
                
        else return;
        
        if (isGinMode) { // need to pass key to GIN mode handling, not to child process
                isGinMode = ch;
                gtk_widget_queue_draw(widget);
        }
                
        else if (putKeys) {
                // pipe key to child process, if stream open
                if (aplMode) {
                        ch = tube_translateKeyCode(ch);
                        putc(ch & 0xFF, putKeys);
                        if (ch >> 8) { // overstrike
                                putc(8, putKeys);
                                putc(ch >> 8, putKeys);
                        }
                }
                else
                        putc(ch,putKeys);
        }
}

int main (int argc, char *argv[])
{
	GtkWidget *darea;
        
        int askWindowWidth;
        int askWindowHeight;
        
        tube_init(argc, argv);
        
        if (argARDS) {
                askWindowWidth = 1080;
                askWindowHeight = 1414;                
        }
        
        else if (argFull) {
                askWindowWidth = 4096;
                askWindowHeight = 3120;
        }
        else {
                askWindowWidth = 1024;
                askWindowHeight = 780;
        }
        
        aspectRatio = (double)askWindowWidth / (double)askWindowHeight;
          
	gtk_init(&argc, &argv);
	
	global_firstcall = TRUE;

	window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
        
        // set the background color
        GdkColor color;
        color.red   = 0;
        color.green = 0;
        color.blue  = 0;
        gtk_widget_modify_bg(window, GTK_STATE_NORMAL, &color);

	darea = gtk_drawing_area_new();
	gtk_container_add(GTK_CONTAINER(window), darea);
	gtk_widget_add_events(window, GDK_BUTTON_PRESS_MASK);
        gtk_widget_add_events(window, GDK_KEY_PRESS_MASK);

	g_signal_connect(G_OBJECT(darea),  "draw",  G_CALLBACK(on_draw_event), NULL);
	g_signal_connect(G_OBJECT(window), "destroy", G_CALLBACK(on_quit_event), NULL);
	g_signal_connect(G_OBJECT(window), "button-press-event", G_CALLBACK(clicked), NULL);
        g_signal_connect(G_OBJECT(window), "key_press_event", G_CALLBACK(on_key_press), NULL);
        
        GdkScreen *screen = gtk_window_get_screen(GTK_WINDOW(window));
	int screenWidth = gdk_screen_get_width(screen);
	int screenHeight = gdk_screen_get_height(screen);
	printf("Screen dimensions: %d x %d\n", screenWidth, screenHeight);
        
        if (argFull) {        
                // DISPLAY UNDECORATED FULL SCREEN WINDOW
		gtk_window_set_decorated(GTK_WINDOW(window), FALSE);
		gtk_window_fullscreen(GTK_WINDOW(window));
		gtk_window_set_keep_above(GTK_WINDOW(window), FALSE);
		windowWidth  = screenWidth;
		windowHeight = screenHeight;
        }
 
        else {
                // DISPLAY DECORATED WINDOW
                if (argFullV || (askWindowHeight > (screenHeight - BORDER))) {
                        askWindowWidth = (int)((double)(screenHeight - BORDER) * aspectRatio);
                        askWindowHeight = screenHeight - BORDER;
                }
                gtk_window_set_decorated(GTK_WINDOW(window), TRUE);
                gtk_window_set_default_size(GTK_WINDOW(window), askWindowWidth, askWindowHeight);
                windowWidth  = askWindowWidth;
                windowHeight = askWindowHeight;                
        }
        // printf("Requested window dimensions: %d x %d\n", windowWidth, windowHeight);
 
        if (TIME_INTERVAL > 0) {
		// Add timer event
		// Register the timer and set time in mS.
		// The timer_event() function is called repeatedly until it returns FALSE. 
		global_timeout_ref = g_timeout_add(TIME_INTERVAL, (GSourceFunc) on_timer_event,
                                                (gpointer) window);
	}

	gtk_window_set_title(GTK_WINDOW(window), windowName);
	
	gtk_widget_show_all(window);

	gtk_main();

	return 0;
}
