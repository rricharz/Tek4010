/*
 *
 *
 * main.c
 * provides a simple framework for basic drawing with cairo and gtk+ 3.0
 * define main window in tek4010.h
 * and cairo drawing in tek4010.c
 * 
 * The framework provides functions to
 *   a function to draw using cairo
 *   a function to initialize your code
 *   a function to exit your code
 *   a peridicaly called function
 *   a function called if the mouse is clicked
 * 
 * Copyright 2016  rricharz
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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <cairo.h>
#include <gtk/gtk.h>

#include "main.h"
#include "tek4010.h"

extern FILE *putKeys;

static cairo_surface_t *global_surface, *global_surface2;
static int global_firstcall;

int global_noexit;
int globalClearPersistent;

static void do_drawing(cairo_t *, GtkWidget *);

static gboolean on_draw_event(GtkWidget *widget, cairo_t *cr, 
    gpointer user_data)
{  
	do_drawing(cr, widget);  
	return FALSE;
}

static gboolean on_timer_event(GtkWidget *widget)
{
	if (tek4010_on_timer_event())
	  gtk_widget_queue_draw(widget);
	return TRUE;
}

static gboolean clicked(GtkWidget *widget, GdkEventButton *event, gpointer user_data)
{
    if (tek4010_clicked(event->button, event->x, event->y))
        gtk_widget_queue_draw(widget);
    return TRUE;
}

static void on_quit_event()
{
	tek4010_quit();
	gtk_main_quit();
        exit(0);
}

static void do_drawing(cairo_t *cr, GtkWidget *widget)
{
	int width = WINDOW_WIDTH;		// we do not use actual window size here
	int height = WINDOW_HEIGHT;             // because global_surface does not change size
	
	if (global_firstcall) {
		global_surface = cairo_surface_create_similar(cairo_get_target(cr),
			CAIRO_CONTENT_COLOR_ALPHA, width, height);
                global_surface2 = cairo_surface_create_similar(cairo_get_target(cr),
			CAIRO_CONTENT_COLOR_ALPHA, width, height);
	}
	
	cairo_t *surface_cr = cairo_create(global_surface);
        cairo_t *surface2_cr = cairo_create(global_surface2);
	tek4010_draw(surface_cr, surface2_cr, width, height, global_firstcall);
	global_firstcall = FALSE;

	cairo_set_source_surface(cr, global_surface, 0, 0);
	cairo_paint(cr);
        cairo_set_source_surface(cr, global_surface2, 0, 0);
	cairo_paint(cr);
  
	cairo_destroy(surface_cr);
        cairo_destroy(surface2_cr);  
}

static void on_key_press(GtkWidget *widget, GdkEventKey *event, gpointer user_data)
{
        int ch;
        // printf("key pressed, state =%04X, keyval=%04X\r\n", event->state, event->keyval);
        
        if ((event->keyval == 0xFF50) ||        // "home" key
                (event->keyval == 0xFF51) ||    // "<ctrl>left arrow" key
                (event->keyval == 0xFF52) ||    // "<ctrl>up arrow" key
                (event->keyval == 0xFF55) ||    // "page up" key
                (event->keyval == 0xFF56))      // "page down" key        
        {
                globalClearPersistent = 1;
                return;
        }
        
        // control keys
        else if ((event->keyval >= 0xFF00) && (event->keyval <= 0xFF1F))
                ch = event->keyval & 0x1F;
        else if (event->state & GDK_CONTROL_MASK)
                ch = event->keyval & 0x1F;
        
        // normal keys
        else if ((event->keyval >= 0x0020) && (event->keyval <= 0x007F))
                ch = event->keyval & 0x7F;
                
        else return;
                
        if (putKeys)
                putc(ch,putKeys);      // pipe key to child process, if stream open
}

int main (int argc, char *argv[])
{
	GtkWidget *darea;
	GtkWidget *window;
        
        if (strcmp(argv[argc-1],"-noexit") == 0) {
                global_noexit = 1;
                argc--;
        }
        else
                global_noexit = 0;
  
	gtk_init(&argc, &argv);
	
	global_firstcall = TRUE;

	window = gtk_window_new(GTK_WINDOW_TOPLEVEL);

	darea = gtk_drawing_area_new();
	gtk_container_add(GTK_CONTAINER(window), darea);
	gtk_widget_add_events(window, GDK_BUTTON_PRESS_MASK);
        gtk_widget_add_events(window, GDK_KEY_PRESS_MASK);

	g_signal_connect(G_OBJECT(darea), "draw",  G_CALLBACK(on_draw_event), NULL);
	g_signal_connect(G_OBJECT(window), "destroy", G_CALLBACK(on_quit_event), NULL);
	g_signal_connect(G_OBJECT(window), "button-press-event", G_CALLBACK(clicked), NULL);
        g_signal_connect(G_OBJECT(window), "key_press_event", G_CALLBACK(on_key_press), NULL);
        
  
        if (TIME_INTERVAL > 0) {
		// Add timer event
		// Register the timer and set time in mS.
		// The timer_event() function is called repeatedly until it returns FALSE. 
		g_timeout_add(TIME_INTERVAL, (GSourceFunc) on_timer_event, (gpointer) window);
	}

	gtk_window_set_default_size(GTK_WINDOW(window), WINDOW_WIDTH, WINDOW_HEIGHT); 
	gtk_window_set_title(GTK_WINDOW(window), WINDOW_NAME);
	
	if (strlen(ICON_NAME) > 0) {
		gtk_window_set_icon_from_file(GTK_WINDOW(window), ICON_NAME, NULL);	
	}
	
	tek4010_init(argc, argv);

	gtk_widget_show_all(window);

	gtk_main();

	return 0;
}
