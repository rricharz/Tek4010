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
#include "tube.h"

static long startPaintTime;

void ards_draw(cairo_t *cr, cairo_t *cr2, int first)
// draw onto the main window using cairo
// cr is used for persistent drawing, cr2 for temporary drawing 

{	
        int ch;
        
        refreshCount++;  // to calculate the average refresh rate
               
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
        
        startPaintTime = tube_mSeconds(); // start to measure time for this draw operation

        showCursor = 1;
        isBrightSpot = 0;
        
        // clear the second surface
        tube_clearSecond(cr2);
        
        // clear persistent surface, if necessary
        if (globaltube_clearPersistent) {
                tube_clearPersistent(cr,cr2);
        }
        
        tube_setupPainting(cr, cr2, "Monospace", (int)(efactor * 18));
        
        do {
                ch = tube_getInputChar();
                
                
                
                
                // decode and act here
                printf("-ARDS mode not yet implemented\n");
                exit(1);
                
                
        }
        while (((tube_mSeconds() - startPaintTime) < refresh_interval));
        
        // display cursor
        
        if (showCursor && (tube_isInput() == 0)) tube_doCursor(cr2);
        
}
