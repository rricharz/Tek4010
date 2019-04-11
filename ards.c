/*
 * ards.c
 * 
 * ARDS option for tek4010 graphics emulator
 * 
 * Copyright 2019  Lars Brinkhoff
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
 */
 
#define WRITE_TROUGH_INTENSITY  0.5             // green only
#define NORMAL_INTENSITY        0.8
#define CURSOR_INTENSITY        0.8
#define BRIGHT_SPOT_COLOR       1.0,1.0,1.0
#define BRIGHT_SPOT_COLOR_HALF  0.3,0.6,0.3
#define BLACK_COLOR             0.0,0.08,0.0    // effect of flood gun

#include <stdio.h>
#include <stdlib.h>
#include <string.h> 
#include <termios.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <errno.h>
#include <sys/time.h>

#include "main.h"
#include "tek4010.h"

extern void gtk_main_quit();
extern int globalClearPersistent;
extern int windowWidth;
extern int windowHeight;

extern int argFull;

extern int hDotsPerChar;
extern int vDotsPerChar;

extern int refresh_interval;           // after this time in msec next refresh is done
extern long refreshCount;

extern int showCursor;                 // set of cursor is shown (not set in graphics mode)
extern int isBrightSpot;               // set if there is currently a bright spot on the screen

extern double efactor;
extern int eoffx;

static long startPaintTime;

extern long mSeconds();
extern void doCursor(cairo_t *cr2);
extern void clearPersistent(cairo_t *cr, cairo_t *cr2);
extern void clearSecond(cairo_t *cr2);
extern void clearPersistent(cairo_t *cr, cairo_t *cr2);
extern int isInput();
extern int getInputChar();

void ards_draw(cairo_t *cr, cairo_t *cr2, int first)
// draw onto the main window using cairo
// cr is used for persistent drawing, cr2 for temporary drawing 

{	
        int ch;
        char s[2];
        
        refreshCount++;  // to calculate the average refresh rate
        
        int xlast = 0;
        int ylast = 0;
        
        if (first) {
                first = 0;
                int actualWidth;                
                if (argFull) {
                        efactor = windowHeight / 780.0;
                        actualWidth = (int)(efactor * 1024.0);
                        eoffx = (windowWidth - actualWidth) / 2;
                        refresh_interval = (int)(30.0 * efactor * efactor);
                }
                else {
                        efactor = 1.0;
                        actualWidth = windowWidth;
                        eoffx = 0;
                        refresh_interval = 30;   
                }
                hDotsPerChar  = actualWidth / 74;
                vDotsPerChar  = windowHeight / 35;
                windowWidth = actualWidth;
                // printf("Scaling: %0.2f\n", efactor);
                // printf("Offset: %d\n",eoffx);
                // printf("Refresh interval: %d\n",refresh_interval);
        }
        
        startPaintTime = mSeconds(); // start to measure time for this draw operation

        showCursor = 1;
        isBrightSpot = 0;
        
        // clear the second surface
        clearSecond(cr2);
        
        // clear persistent surface, if necessary
        if (globalClearPersistent) {
                clearPersistent(cr,cr2);
        }
        
        cairo_set_antialias(cr, CAIRO_ANTIALIAS_BEST);      
	cairo_set_line_width (cr, 1);
        cairo_set_source_rgb(cr, 0, NORMAL_INTENSITY, 0);
        
        cairo_select_font_face(cr, "Monospace", CAIRO_FONT_SLANT_NORMAL, CAIRO_FONT_WEIGHT_NORMAL);
        cairo_select_font_face(cr2, "Monospace", CAIRO_FONT_SLANT_NORMAL, CAIRO_FONT_WEIGHT_BOLD);
        
        if (argFull > 0.0) {
                cairo_set_font_size(cr, (int)(efactor * 18));
                cairo_set_font_size(cr2,(int)(efactor * 18));
        }
        else {
                cairo_set_font_size(cr, 18);
                cairo_set_font_size(cr2, 18);
        }
        
        do {
                ch = getInputChar();
                
                
                
                
                // decode and act here
                printf("-ARDS mode not yet implemented\n");
                exit(1);
                
                
        }
        while (((mSeconds() - startPaintTime) < refresh_interval));
        
        // display cursor
        
        if (showCursor && (isInput() == 0)) doCursor(cr2);
        
}
