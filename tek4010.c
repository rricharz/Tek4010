/*
 * tek4010.c
 * 
 * A tek4010/4014 graphics emulator
 * 
 * Copyright 2019  rricharz
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
 
#define DEBUG    0              // print debug info
 
#define TODO  (long)(8.0 * efactor * efactor)   // draw multiple objects until screen updates

#define _GNU_SOURCE
 
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

// mode handles the current state of the emulator:
//
// mode 0       alpha mode
//
// mode 1       expecting address byte of dark mode (move to) address
// mode 2       expecting second byte of dark mode (move to) address
// mode 3       expecting third byte of dark mode (move to) address
// mode 4       expecting fourth byte of dark mode (move to) address
// mode 5       expecting first byte of persistent vector end point address
// mode 6       expecting second byte of persistent vector end point address
// mode 7       expecting third byte of persistent vector end point address
// mode 8       expecting fourth byte of persistent vector end point address
//
// mode 30      expecting escape sequence, escape code received
// mode 31      received in ANSI escape sequence, escape sequence continues if next char is digit
//
// mode 40      incremental plot mode; is ignored until exit from incremental plot received
// mode 50      special point plot mode; not yet implemented
// mode 101     ignore until group separator

int mode, savemode;

extern int leftmargin;

static long startPaintTime;
static int xh,xl,yh,yl,xy4014;
static long todo;

void tek4010_checkLimits()
/* check whether char is in visibel space */
{
        /* don't check here for leftmargin, graphics needs to write characters to the whole screen */
        if (tube_x0 < 0) tube_x0 = 0;
        
        if (tube_x0 > windowWidth - hDotsPerChar) {
                tube_x0 = leftmargin; tube_y0 -= vDotsPerChar;
        }
        if (tube_y0 < 4) {
                tube_y0 = windowHeight - vDotsPerChar;
                if (leftmargin) leftmargin = 0;
                else leftmargin = windowWidth / 2;
                /* check here for leftmargin */
                if (tube_x0 < leftmargin) tube_x0 = leftmargin;
        }
        if (tube_y0 > (windowHeight - vDotsPerChar)) tube_y0 = windowHeight - vDotsPerChar;
}

void tek4010_escapeCodeHandler(cairo_t *cr, cairo_t *cr2, int ch)
// handle escape sequencies
{
        if (DEBUG) printf("Escape mode, ch=%02X\n",ch);
        switch (ch) {
                case 0: break;
                case 5: // ENQ: ask for status and position
                        // not yet implemented, needs to send 7 bytes
                        printf("ENQ not implemented\n");
                        break;
                case 12:
                        if (DEBUG) printf("Form feed, clear screen\n");
                        tube_changeCharacterSize(cr, cr2, 74, 35, (int) (18.0 * efactor));
                        tube_clearPersistent(cr,cr2);
                        mode = 0;
                        break;
                case '[':   
                        // a second escape code follows, do not reset mode
                        break;
                        
                case 14: // SO  activate alternative char set, not implemented
                case 15: // SI  deactivate alternative char set
                        mode = 0;
                        break;
                        
                case 23: system("scrot --focussed"); mode= 0; break;
                        
                case 28: // file separator  >> point plot mode
                        mode = 5;
                        plotPointMode= 1;
                        break;
                        
                case 29: // group separator >> graphics mode
                        mode = 1;               
                        plotPointMode = 0;
                        break;
                                         
                case '0':
                case '1':
                case '2':
                case '3':
                case '4':
                case '5':
                case '6':
                case '7': printf("esc %c\n", ch); mode = 31; break;
                case '8': tube_changeCharacterSize(cr, cr2, 74, 35, (int)(efactor * 18)); break;
                case '9': tube_changeCharacterSize(cr, cr2, 81, 38, (int)(efactor * 16)); break;
                case ':': tube_changeCharacterSize(cr, cr2, 121, 58, (int)(efactor * 11)); break;
                case ';': tube_changeCharacterSize(cr, cr2, 133, 64, (int)(efactor * 10)); break;
                case ']': printf("esc %c\n", ch);      break;
                case 'm': mode = 0; break;
                
                case '`': ltype = SOLID;    writeThroughMode = 0; mode = 0; break;
                case 'a': ltype = DOTTED;   writeThroughMode = 0; mode = 0; break;
                case 'b': ltype = DOTDASH;  writeThroughMode = 0; mode = 0; break;
                case 'c': ltype = SHORTDASH;writeThroughMode = 0; mode = 0; break;
                case 'd': ltype = LONGDASH; writeThroughMode = 0; mode = 0; break;
                case 'e': ltype = SOLID;    writeThroughMode = 0; mode = 0; break;
                case 'f': ltype = SOLID;    writeThroughMode = 0; mode = 0; break;
                case 'h': ltype = SOLID;    writeThroughMode = 0; mode = 0; break; 
                                    
                case 'p': ltype = SOLID;    writeThroughMode = 1; mode = 101; showCursor = 0; break;
                case 'q': ltype = DOTTED;   writeThroughMode = 1; mode = 101; showCursor = 0; break;
                case 'r': ltype = DOTDASH;  writeThroughMode = 1; mode = 101; showCursor = 0; break;
                case 's': ltype = SHORTDASH;writeThroughMode = 1; mode = 101; showCursor = 0; break;
                case 't': ltype = LONGDASH; writeThroughMode = 1; mode = 101; showCursor = 0; break;
                case 'u': ltype = SOLID;    writeThroughMode = 1; mode = 101; showCursor = 0; break;
                case 'v': ltype = SOLID;    writeThroughMode = 1; mode = 101; showCursor = 0; break;
                case 'w': ltype = SOLID;    writeThroughMode = 1; mode = 101; showCursor = 0; break; 
                        
                default: 
                        printf("Escape code %02X not implemented, mode = %d\n",ch, savemode);
                        mode = 0;
                        break;                                               
        }         
}


int tek4010_checkExitFromGraphics(int ch)
// test for exit from graphics character
{
        if ((ch==31) || (ch==13) || (ch==27) || (ch==12)) {
                mode = 0;
                if (DEBUG) printf("Leaving graphics mode\n");
                showCursor = 0;
                if (ch == 12) globaltube_clearPersistent = 1;
                plotPointMode = 0;
                return 1;
        }
        else return 0;
}

void tek4010_draw(cairo_t *cr, cairo_t *cr2, int first)
// draw onto the main window using cairo
// cr is used for persistent drawing, cr2 for temporary drawing 

{	
        int ch;
        
        refreshCount++;
        
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
                windowWidth = actualWidth;
                tube_changeCharacterSize(cr, cr2, 74, 35, (int) (18.0 * efactor));
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
                tube_changeCharacterSize(cr, cr2, 74, 35, (int) (18.0 * efactor));
        }
        
        tube_setupPainting(cr, cr2, "Monospace");
        
        if (plotPointMode)
                todo = 16 * TODO;
        else if (writeThroughMode)
                todo = 8 * TODO;
        else if (mode == 0)
                todo = 4 * TODO;      // for text speed
        else
                todo = TODO;
                
        do {
                ch = tube_getInputChar();
                
                if (tube_isInput() == 0) {
                        todo = 0;
                }

                if (ch == -1) {
                        if ((mode == 0) && showCursor) tube_doCursor(cr2);
                        return;         // no char available, need to allow for updates
                }
                
                if (DEBUG) {
                        printf("mode=%d, ch code %02X",mode,ch);
                        if ((ch>0x20)&&(ch<=0x7E)) printf(" (%c)",ch);
                        printf("\n");
                }
                
                if (mode == 31) {
                        printf("ANSI escape mode 31, ch=%02x\n",ch);
                        if ((ch>='0') && (ch<='9')) { savemode = mode; mode = 30; }
                }
                
                if (ch == 27) {  // escape code
                        savemode = mode;
                        mode = 30; return; 
                }
                
                if (ch == 30) {
                        printf("Incremental plot mode not implemented\n");
                        mode = 40; return;
                }
                
                int tag = (ch >> 5) & 3;
                                
                if ((mode >= 1) && (mode <= 8)) {
                        
                        if (tek4010_checkExitFromGraphics(ch)) {
                                todo = todo - 4;
                                goto endDo;
                        }
                        
                        // handle skipped coordinate bytes
                        // this cannot be done in switch(mode) below, because multiple bytes
                        // can be switched and the current byte must be executed after a mode change
                        
                        if ((mode == 5) && (ch == 29)) {
                                if (DEBUG) printf("group separator, go from mode 5 to mode 1\n");
                                mode = 1;
                                goto endDo; // goto end of do loop
                        }
                        
                        if (DEBUG) {
                                if (mode & 1)
                                        printf("mode=%d,tag=%d-H-val=%d,",
                                                mode,tag, 32 * (ch & 31));
                                else
                                        printf("mode=%d,tag=%d-L-val=%d,",
                                                mode,tag, ch & 31);
                                printf("xh=%d,xl=%d,yh=%d,yl=%d\n",xh,xl,yh,yl);                
                        }
                        
                        if (tag != 0) {
 
                                if ((mode == 1) && (tag != 1)) mode = 2;
                                
                                if ((mode == 3) && (tag == 3)) {
                                        // this overwrites the extra data byte of the 4014 for the
                                        // persistent mode ccordinates and stores it for further use
                                        mode = 2;
                                        xy4014 = yl;
                                        if (DEBUG)
                                                printf("4014 coordinates, overwrite last value\n");
                                }
                                
                                if ((mode == 2) && (tag != 3)) mode = 3;
                                if ((mode == 3) && (tag != 1)) mode = 4;
                                
                                if ((mode == 5) && (tag != 1)) mode = 6;                                

                                if ((mode == 7) && (tag == 3)) {
                                        // this overwrites the extra data byte of the 4014 for the
                                        // persistent mode ccordinates and stores it for further use
                                        mode = 6;
                                        xy4014 = yl;
                                        if (DEBUG)
                                                printf("4014 coordinates, overwrite last value\n");
                                }
                                                              
                                if ((mode == 6) && (tag != 3)) mode = 7;                                                        
                                if ((mode == 7) && (tag != 1)) mode = 8;
                        }
                        else {
                                if (ch ==  0) return;
                                if (ch == 29) mode = 1; // group separator
                                else if (ch == 28) { plotPointMode = 1; todo = 16 * todo; }
                                else printf("Plot mode, unknown char %d, plotPointMode = %d\n",ch,plotPointMode);
                                return;
                        }
                                
                }
                
                switch (mode) {                                
                        case 1: plotPointMode = 0; // normal graphics mode, starting coordinates
                                yh = 32 * (ch & 31); mode++;
                                if (DEBUG) printf("setting yh to %d\n", yh);
                                break;
                        case 2: yl = (ch & 31);
                                if (argFull) {
                                        int yb = (xy4014 >> 2) & 3;
                                        tube_y0 = (int)(efactor * (double)(((yh+yl) << 2) + yb) / 4.0);
                                }
                                else tube_y0 = yh + yl;
                                mode++;
                                if (DEBUG) printf("setting yl to %d\n", yl);
                                break;
                        case 3: if (tag == 1) xh = 32 * (ch & 31); mode++;
                                if (DEBUG) printf("setting xh to %d\n", xh);
                                break;
                        case 4: xl = (ch & 31);
                                if (argFull) {
                                        int xb = xy4014 & 3;
                                        tube_x0 = (int)(efactor * (double)(((xh+xl) << 2) + xb) / 4.0);
                                }
                                else tube_x0 = xh + xl;
                                mode++;
                                tube_emulateDeflectionTime();
                                if (DEBUG) printf("setting xl to %d\n", xl);
                                if (DEBUG) printf("******************************************** Moving to (%d,%d)\n",tube_x0,tube_y0);
                                break;
                        case 5: if (ch == 29) {
                                        if (DEBUG) printf("setting mode to 1\n");
                                        mode = 1;
                                }
                                else if (tag != 0) {
                                        yh = 32 * (ch & 31);  mode++;
                                }
                                else printf("case 5: tag is 0\n");
                                if (DEBUG) printf(">>>>>yh=%d\n",yh);
                                break;
                        case 6: yl = (ch & 31);               mode++;
                                break;
                                if (DEBUG) printf(">>>>>yl=%d\n",yl);
                        case 7: xh = 32 * (ch & 31);          mode++;
                                break;
                                if (DEBUG) printf(">>>>>xh=%d\n",xh);
                        case 8: xl = (ch & 31);
                                if (argFull) {
                                        int xb = xy4014 & 3;
                                        tube_x2 = (int)(efactor * (double)(((xh+xl) << 2) + xb) / 4.0);
                                        int yb = (xy4014 >> 2) & 3;
                                        tube_y2 = (int)(efactor * (double)(((yh+yl) << 2) + yb) / 4.0);
                                }
                                else {
                                        tube_x2 = xh + xl;
                                        tube_y2 = yh + yl;
                                }
                                if (DEBUG) printf(">>>>>xl=%d\n",xl);
                                
                                if (plotPointMode>0.0) {
                                        
                                        tube_drawPoint(cr, cr2);
                                        
                                        // draw the point
                                        
                                        mode = 50;
                                        todo--;
                                }
                                
                                else {
                                        tube_drawVector(cr,cr2);
                                        
                                        todo--;                                        
                                }
                                
                                showCursor = 0;
                                
                                tube_x0 = tube_x2;        // prepare for additional vectors
                                tube_y0 = tube_y2;                                
                                mode = 5;
                                
                                break;
                        case 30: 
                                tek4010_escapeCodeHandler(cr, cr2, ch);
                                break;               
                        case 40: // incremental plot mode, not implemented
                                if (ch == 31) mode = 0;  // leave this mode
                                break;
                        case 101: 
                                if (DEBUG) printf("Ignore until group separator, ch = %02x\n", ch);
                                if (ch == 29) mode = 1;
                                break;
                        default: 
                                switch (ch) {
                                case 0:     break;
                                case 7:     // bell function, delay 0.1 sec
                                            // cannot delay if bright spot is on, needs to be turned off first
                                            tube_u100ResetSeconds(1);
                                            usleep(50000);
                                            showCursor=0;
                                            todo = 0;
                                            break;
                                case 8:     // backspace
                                            tube_x0 -= hDotsPerChar;
                                            tek4010_checkLimits();
                                            break;
                                case 9:     // tab
                                            if (argTab1)
                                                tube_x0 += hDotsPerChar;
                                            else
                                                tube_x0 = tube_x0 - (tube_x0 % (8 * hDotsPerChar)) + 8 * hDotsPerChar;
                                            tek4010_checkLimits();
                                            break;
                                case 10:    // new line
                                            tube_y0 -= vDotsPerChar;
                                            if (!argRaw) tube_x0 = leftmargin;
                                            tek4010_checkLimits();
                                            break;
                                case 11:    // VT, move one line up
                                            tube_y0 += vDotsPerChar;
                                            tek4010_checkLimits();
                                            break;
                                case 13:    // return
                                            mode = 0; tube_x0 = leftmargin;
                                            break;
                                case 23:    // ctrl-w  screen dump
                                            system("scrot --focussed");
                                            break;
                                case 28:    // file separator  >> point plot mode
                                            mode = 5;
                                            plotPointMode= 1;
                                            break;
                                case 29:    // group separator
                                            mode = 1;
                                            break;
                                case 30:    // record separator
                                            mode = 40;
                                            break;
                                case 31:    // US, leave graphics mode
                                            mode = 0;
                                            break;
                                default:    if ((ch >= 32) && (ch <127)) { // printable character
                                                tek4010_checkLimits();
                                                tube_drawCharacter(cr,cr2, ch);
                                                
                                                todo-= 2;
                                            }
                                            break;
                                }
                                break;                                
                }
                endDo:;
        }
        while ((todo > 0) && ((tube_mSeconds() - startPaintTime) < refresh_interval));
        
        // display cursor
        
        if (showCursor && (tube_isInput() == 0)) tube_doCursor(cr2);
        
}
