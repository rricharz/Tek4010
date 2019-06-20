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

// mode handles the current state of the emulator:
//
// mode 0       symbol mode
// mode 1       set point mode
// mode 2       extended vector mode
// mode 3       short vector mode

static int mode;

static double efactor;

static int args;
static unsigned char data[4];

static int x0 = -485, y0 = 450, x2, y2;

static void draw_char (cairo_t *cr, cairo_t *cr2, char ch)
{
  tube_x0 = (int)(efactor * (x0 + 540));
  tube_y0 = (int)(efactor * (y0 + 707));
  //fprintf (stderr, "[tube: %d,%d]", tube_x0, tube_y0);
  tube_drawCharacter(cr, cr2, ch);
  x0 += (int)(hDotsPerChar / efactor);
}

static void draw_vector (cairo_t *cr, cairo_t *cr2)
{
  tube_x0 = (int)(efactor * (x0 + 540));
  tube_y0 = (int)(efactor * (y0 + 707));
  tube_x2 = (int)(efactor * (x2 + 540));
  tube_y2 = (int)(efactor * (y2 + 707));
  //fprintf (stderr, "[tube: %d,%d - %d,%d]", tube_x0, tube_y0, tube_x2, tube_y2);
  tube_drawVector(cr, cr2);
}

void ards_draw(cairo_t *cr, cairo_t *cr2, int first)
// draw onto the main window using cairo
// cr is used for persistent drawing, cr2 for temporary drawing 

{	
        int ch;
        
        refreshCount++;  // to calculate the average refresh rate
               
        if (first) {
                first = 0;
                efactor = windowWidth / 1080.0;
                // fprintf (stderr, "efactor: %0.2f\n", efactor);
                refresh_interval = 30;        
                tube_changeCharacterSize(cr, cr2, 80, 50, efactor * 1.1);
                
        }
        
        startPaintTime = tube_mSeconds(); // start to measure time for this draw operation

        showCursor = 1;
        isBrightSpot = 0;
        
        // clear the second surface
        tube_clearSecond(cr2);
        
        // clear persistent surface, if necessary
        if (tube_doClearPersistent) {
                tube_clearPersistent(cr,cr2);
        }
        
        tube_setupPainting(cr, cr2, STANDARD_FONT);
        
        do {
                ch = tube_getInputChar();
                
                if (tube_isInput() == 0) {
                }

                if (ch == -1) {
                        return;         // no char available, need to allow for updates
                }

		//fprintf (stderr, "\n[INPUT %03o [%d/%d]]", ch, mode, args);

		if (args > 0) {
		  data[--args] = ch;
		}

		switch (ch) {
		case 007:
		  mode = 0;
		  args = 0;
		  break;
		case 010:
		  mode = 0;
		  args = 0;
		  x0 -= hDotsPerChar;
		  break;
		case 012:
		  mode = 0;
		  args = 0;
		  y0 -= vDotsPerChar;
		  break;
		case 014:
		  mode = 0;
		  args = 0;
		  x0 = -485;
		  y0 = 450;
		  tube_clearPersistent(cr, cr2);
		  break;
		case 015:
		  mode = 0;
		  args = 0;
		  x0 = -479;
		  break;
		case 034:
		  mode = 0;
		  args = 0;
		  break;
		case 035:
		  //fprintf (stderr, "[POINT]");
		  mode = 1;
		  args = 4;
		  break;
		case 036:
		  //fprintf (stderr, "[E VEC]");
		  mode = 2;
		  args = 4;
		  break;
		case 037:
		  //fprintf (stderr, "[S VEC]");
		  mode = 3;
		  args = 2;
		  break;
		default:
		  if (args == 0) {
		    switch (mode) {
		    case 0:
		      //fprintf (stderr, "[SYMBOL %c @ %d, %d]", ch, x0, y0);
		      draw_char (cr, cr2, ch);
		      break;
		    case 1:
		      args = 4;
		      x0 = (data[3] & 076) >> 1;
		      x0 |= (data[2] & 037) << 5;
		      if (data[3] & 1)
			x0 = -x0;
		      y0 = (data[1] & 076) >> 1;
		      y0 |= (data[0] & 037) << 5;
		      if (data[1] & 1)
			y0 = -y0;
		      //fprintf (stderr, "[POINT @ %d, %d]", x0, y0);
		      break;
		    case 2:
		      args = 4;
		      x2 = (data[3] & 076) >> 1;
		      x2 |= (data[2] & 037) << 5;
		      if (data[3] & 1)
			x2 = -x2;
		      x2 += x0;
		      y2 = (data[1] & 076) >> 1;
		      y2 |= (data[0] & 037) << 5;
		      if (data[1] & 1)
			y2 = -y2;
		      y2 += y0;
		      //fprintf (stderr, "[E VEC @ %d,%d - %d,%d]", x0, y0, x2, y2);
		      if (data[2] & 040)
			; //fprintf (stderr, "[INVISIBLE]");
		      else
			draw_vector(cr, cr2);
		      if (data[0] & 040)
			; //fprintf (stderr, "[DOTTED]");
		      x0 = x2;
		      y0 = y2;
		      break;
		    case 3:
		      args = 2;
		      x2 = (data[1] & 076) >> 1;
		      if (data[1] & 1)
			x2 = -x2;
		      x2 += x0;
		      y2 = (data[0] & 076) >> 1;
		      if (data[0] & 1)
			y2 = -y2;
		      y2 += y0;
		      draw_vector(cr, cr2);
		      //fprintf (stderr, "[S VEC @ %d,%d - %d,%d]", x0, y0, x2, y2);
		      x0 = x2;
		      y0 = y2;
		      break;
		    }
		  }
		  break;
		}
        }
        while (((tube_mSeconds() - startPaintTime) < refresh_interval));
        
        // display cursor
        
        if (showCursor && (tube_isInput() == 0)) tube_doCursor(cr2);
        
}
