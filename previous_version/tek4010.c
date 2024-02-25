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

 
#define TODO  (long)(16)   // draw multiple objects until screen updates

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
// mode 50      special point plot mode
// mode 60      crosshair mode
// mode 101     ignore until group separator

int mode, savemode;
int penDown = 1;

extern int leftmargin;
extern FILE *putKeys;

static long startPaintTime;
static int xh,xl,yh,yl,xy4014;
static long todo;
static double efactor;

int ginCharacter[6] = {0, 0, 0, 0, 0, 0};

// table for special plot point mode
// 4014 manual page F-9
int intensityTable[] = {14,16,17,19,  20,22,23,25,  28,31,34,38,  41,33,47,50,
                        56,62,69,75,  81,88,94,100, 56,62,69,75,  81,88,96,100,
                         0, 1, 1, 1,   1, 1, 1, 2,   2, 2, 2, 2,   3, 3, 3, 3,
                         4, 4, 4, 5,   5, 5, 6, 6,   7, 8, 9,10,  11,12,12,13,
                        14,16,17,19,  20,22,23,25,  28,31,34,38,  41,44,47,50,
                        56,62,69,75,  81,88,94,100, 56,63,69,75,  81,88};
                     

void tek4010_checkLimits(cairo_t *cr, cairo_t *cr2)
/* check whether char is in visibel space */
{
        /* don't check here for leftmargin, graphics needs to write characters to the whole screen */
        if (tube_x0 < 0) tube_x0 = 0;
        
        if (tube_x0 > windowWidth - hDotsPerChar) {
                tube_x0 = leftmargin; tube_y0 -= vDotsPerChar;
        }
        if (tube_y0 < 4) { // new line at bottom of page
                tube_y0 = windowHeight - vDotsPerChar;
                if (leftmargin) leftmargin = 0;
                else leftmargin = windowWidth / 2;
                if (argAutoClear) {
                        leftmargin = 0;
                        tube_clearPersistent(cr,cr2);
                }
                /* check here for leftmargin */
                if (tube_x0 < leftmargin) tube_x0 = leftmargin;
        }
        if (tube_y0 > (windowHeight - vDotsPerChar)) tube_y0 = windowHeight - vDotsPerChar;
}

void tek4010_bell()
{
        // bell function, delay 0.05 sec
        tube_u100ResetSeconds(1);
        usleep(50000);
        showCursor=0;
        todo = 0;
}

void sendCoordinates()
{
        // send 4 coordinate bytes
        int x,y,ch;
        x = (int)((double)tube_x0 / efactor);
        y = (int)((double)tube_y0 / efactor);
        
        if (DEBUG) printf("sendCoordinates, x=%d, y=%d\n", x, y);
        
        ch = (x >> 5) + 0x20;
        putc(ch, putKeys);
        ginCharacter[4] = ch;       // save to check for echo
        ch = (x & 31) + 0x20;
        putc(ch,putKeys);
        ginCharacter[3] = ch;       // save to check for echo
        ch = (y >> 5) + 0x20;
        putc(ch, putKeys);
        ginCharacter[2] = ch;       // save to check for echo
        ch = (y & 31) + 0x20;
        putc(ch,putKeys);
        ginCharacter[1] = ch;       // save to check for echo
}

void enqMode()
{
        // activated by sending ESC ENQ
        int status;
        status = 0x20;
        if (leftmargin == 0) status += 2;
        if (mode == 0) status += 4;
        putc(status,putKeys);           // send status byte
        sendCoordinates();
        putc(13, putKeys);              // cannot send a EOT here 
}

void ginMode(cairo_t *cr, cairo_t *cr2)
{
        // activated by sending ESC SUB
        if (DEBUG) printf("GIN, mode = %d, isGinMode = %d\n", mode, isGinMode);
        tube_crosshair(cr, cr2);
        mode = 60;
        todo = 0;
        showCursor = 0;
        isGinMode = 1;
}

void ginSend(int ch)
{
        // user has stoken a key during GIN mode
        if (DEBUG) printf("ginSend, ch = %d\n", ch);
        putc(ch,putKeys);           // user key stroke character
        ginCharacter[5] = ch;       // save to check for echo
        sendCoordinates();          // 4 characters of packed coordinates
        // cannot send a EOT here
        // wait 5 ms, then send CR.
        usleep(5000);
        putc(13,putKeys);           // cr
        ginCharacter[0] = 13;
        // prepare to suppress unwanted echoed characters.
        isGinSuppress = 6;      
}

void tek4010_clicked(int x, int y)
{
        if (DEBUG) printf("Clicked, mode = %d\n", mode);
        if (mode == 60) {
                tube_x0 = x;
                tube_y0 = y;
        }
}

void tek4010_escapeCodeHandler(cairo_t *cr, cairo_t *cr2, int ch)
// handle escape sequencies, see 4014 user manual, table page G-1
// codes identical for all modes are handled elsewhere
{
        if (DEBUG) printf("Escape %02X, savemode=%d\n",ch,savemode);
        switch (ch) {
                case 0: break; // ignore filler 0

                case 5: // ENQ: ask for status and position
                        enqMode();
                        mode = 0; break;
                case 6: break;
                
                case 8: // backspace during ESC
                        tube_x0 -= hDotsPerChar;
                        tek4010_checkLimits(cr, cr2);
                        mode = 0; break;
                case 9: // tab during ESC
                        if (argTab1)
                                tube_x0 += hDotsPerChar;
                        else
                                tube_x0 = tube_x0 - (tube_x0 % (8 * hDotsPerChar)) + 8 * hDotsPerChar;
                        tek4010_checkLimits(cr, cr2);
                        mode = 0; break;

                case 11:// VT during ESC, move one line up
                        tube_y0 += vDotsPerChar;
                        tek4010_checkLimits(cr, cr2);
                        mode = 0; break;
                case 12:// FF during ESC
                        if (!argKeepSize)
                                tube_changeCharacterSize(cr, cr2, 74, 35, efactor);
                        tube_clearPersistent(cr,cr2);
                        mode = 0; break;
                case 13:mode = 0; break;
                case 14: // SO  activate alternative char set
                        if (argAPL) {            // switch only of argAPL is set
                                aplMode = 1;
                                // printf("Setting APL mode to 1 from computer\n");
                        }
                        mode = 0;
                        todo = 0;
                        break;
                case 15: // SI  deactivate alternative char set
                        aplMode = 0;
                        mode = 0;
                        todo = 0;
                        // printf("Setting APL mode to 0 from computer\n");
                        break;
                
                case 23: system("scrot --focussed"); mode= 0; break;
                
                case 26: // sub
                        ginMode(cr, cr2);
                        break;
                        
                // modes 27 and 29 - 31 are identical in all modes
                case 28: // record separator
                        if (DEBUG) printf("Special point plot mode, mode=%d\n",savemode);
                        mode = 50; // for the intensity/focus character
                        plotPointMode = 1;
                        specialPlotMode = 1;
                        double intensity = 1.0;
                        int defocussed = 0;
                        break;

                case '8': tube_changeCharacterSize(cr, cr2, 74, 35, efactor); mode = 0; break;
                case '9': tube_changeCharacterSize(cr, cr2, 81, 38, efactor * 0.9); mode = 0; break;
                case ':': tube_changeCharacterSize(cr, cr2, 121, 58, efactor * 0.65); mode = 0; break;
                case ';': tube_changeCharacterSize(cr, cr2, 133, 64, efactor * 0.55); mode = 0; break;
                
                case '[': printf("Ignoring ANSI escape sequence: [");
                          mode=31;
                          break;
                
                // normal mode
                case '`': ltype = SOLID;    writeThroughMode = 0; mode = savemode; break;
                case 'a': ltype = DOTTED;   writeThroughMode = 0; mode = savemode; break;
                case 'b': ltype = DOTDASH;  writeThroughMode = 0; mode = savemode; break;
                case 'c': ltype = SHORTDASH;writeThroughMode = 0; mode = savemode; break;
                case 'd': ltype = LONGDASH; writeThroughMode = 0; mode = savemode; break;
                case 'e': ltype = SOLID;    writeThroughMode = 0; mode = savemode; break;
                case 'f': ltype = SOLID;    writeThroughMode = 0; mode = savemode; break;                
                case 'g': ltype = SOLID;    writeThroughMode = 0; mode = savemode; break;
                        
                // defocussed mode
                case 'h':
                case 'i': 
                case 'j':
                case 'k':
                case 'l':
                case 'm':
                case 'n':
                case 'o': if (DEBUG) printf("Defocussed mode ESC %c not supported, ignored\n", ch);
                          mode = 101;  break;
                                
                // write-trough mode
                case 'p': ltype = SOLID;    writeThroughMode = 1; mode = 101; showCursor = 0; break;
                case 'q': ltype = DOTTED;   writeThroughMode = 1; mode = 101; showCursor = 0; break;
                case 'r': ltype = DOTDASH;  writeThroughMode = 1; mode = 101; showCursor = 0; break;
                case 's': ltype = SHORTDASH;writeThroughMode = 1; mode = 101; showCursor = 0; break;
                case 't': ltype = LONGDASH; writeThroughMode = 1; mode = 101; showCursor = 0; break;
                case 'u': ltype = SOLID;    writeThroughMode = 1; mode = 101; showCursor = 0; break;
                case 'v': ltype = SOLID;    writeThroughMode = 1; mode = 101; showCursor = 0; break;
                case 'w': ltype = SOLID;    writeThroughMode = 1; mode = 101; showCursor = 0; break; 
                        
                default: 
                        printf("Ignoring escape code: 0x%02x\n",ch);
                        mode = 0;
                        break;                                               
        }         
}

int tek4010_checkReturnToAlpha(int ch)
// test for return to alpha character set
// see 4014 manual, page F-10, note 1
{
        if (ch == 27)
                savemode = mode;
        if ((ch==31) || (ch==13) || (ch==27) /*|| (ch==12)*/) {
                if (DEBUG && mode) printf("Going to alpha mode\n");
                mode = 0;
                showCursor = 0;
                //if (ch == 12) {
                //        tube_doClearPersistent = 1;
                //        todo = 0;
                //}
                if (ch == 27) {
                        mode = 30;
                        todo = 0;
                }
                plotPointMode = 0;
                specialPlotMode = 0;
                if (isGinMode && DEBUG)
                        printf("clearing isGinMode, char = %d\n", ch);
                isGinMode = 0;
                return 1;
        }
        else return 0;
}

int digit(char ch)
{
        return ((ch>='0') && (ch<='9'));
}

void tek4010_draw(cairo_t *cr, cairo_t *cr2, int first)
// draw onto the main window using cairo
// cr is used for persistent drawing, cr2 for temporary drawing 

{	
        int ch, tag;
        
        refreshCount++;
        
        if (first) {
                first = 0; 
                efactor = windowWidth / 1024.0;
                refresh_interval = 30;
                tube_changeCharacterSize(cr, cr2, 74, 35, efactor);
                if (efactor > 0.8) pensize = efactor * 1.25;
                if (windowWidth != 1024) printf("Scaling: %0.3f\n", efactor / 4.0);
        }
        
        startPaintTime = tube_mSeconds(); // start to measure time for this draw operation

        showCursor = 1;
        isBrightSpot = 0;
        
        // clear the second surface
        tube_clearSecond(cr2);
        
        // clear persistent surface, if necessary
        if (tube_doClearPersistent) {
                tube_clearPersistent(cr,cr2);
                if (!argKeepSize)
                        tube_changeCharacterSize(cr, cr2, 74, 35, efactor);
        }
        
        if (aplMode)
                tube_setupPainting(cr, cr2, APL_FONT);
        else
                tube_setupPainting(cr, cr2, STANDARD_FONT);
        
        if (plotPointMode)
                todo = 100 * TODO;
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
                        if (mode != 60) return;         // no char available, need to allow for updates
                }

		// Try suppressing GIN echoed characters here.
		if (isGinSuppress){
                        if (ch == 10) // additional line feed may be echoed if cr is typed
                                return;
                        if ((ch & 0x7F) == ginCharacter[isGinSuppress - 1]) {
                                if (DEBUG) printf( "isGinSuppress (%d): suppressing: %d\n",
                                        isGinSuppress, ch);
                                isGinSuppress --;
                                return;
                        }
                        else {
                                if (DEBUG) printf("isGinSuppress, characters are different (%d,%d)\n",
                                        ch & 0x7F, ginCharacter[isGinSuppress - 1]);
                                isGinSuppress = 0;
                        }
		}
                
                // if (aplMode) printf("Receiving character %d from host\n", ch);
                
                if (DEBUG) {
                        printf("mode=%d, ch code %02X",mode,ch);
                        if ((ch>0x20)&&(ch<=0x7E)) printf(" (%c)",ch);
                        printf("\n");
                }

                if (tek4010_checkReturnToAlpha(ch)) {
                        todo = todo - 4;
                        goto endDo;
                }

                // the following chars are identical in all modes (with exception: 13,28)
                // see 4014 user manual, table on page G1 ff
                
                switch (ch) {
                        case 7:         tek4010_bell();        
                                        goto endDo;
                        case 10:        // new line
                                        tube_y0 -= vDotsPerChar;
                                        if (!argRaw) tube_x0 = leftmargin;
                                        tek4010_checkLimits(cr, cr2);
                                        goto endDo;
                        case 13:        // return
                                        if (mode != 30) { // special handling in ESC mode
                                                mode = 0; tube_x0 = leftmargin;
                                                goto endDo;
                                        }
                                        break;
                        case 27:        // escape code, all modes
                                        savemode = mode;
                                        mode = 30; 
                                        goto endDo; 
                        case 28:        // file separator  >> point plot mode
                                        if (mode != 30) { // special handling in ESC mode
                                                mode = 5;
                                                plotPointMode= 1;
                                                goto endDo;
                                        }
                                        break;
                        case 29:        // group separator >> graphics mode
                                        mode = 1;
                                        plotPointMode = 0;
                                        goto endDo;
                        case 30:        // record separator >> incremental plot mode
                                        if (DEBUG) printf("Incremental point plot mode\n");
                                        penDown = 1;
                                        mode = 40;
                                        goto endDo;
                        case 31:        // US, normal mode
                                        mode = 0;
                                        goto endDo;
                }
                
                
                // handle skipping coordinate bytes
                // this cannot be done in switch(mode) below, because multiple bytes
                // can be skipped and the current byte must be executed after a mode change
                
                tag = (ch >> 5) & 3;
                                
                if ((mode >= 1) && (mode <= 8)) {
                        
                        if ((mode == 5) && (ch == 29)) {
                                if (DEBUG) printf("group separator, go from mode 5 to mode 1\n");
                                mode = 1;
                                goto endDo; // goto end of do loop
                        }
                        
                        if (DEBUG) {
                                if (mode & 1)
                                        printf("    mode=%d,tag=%d-H-val=%d,",
                                                mode,tag, 32 * (ch & 31));
                                else
                                        printf("    mode=%d,tag=%d-L-val=%d,",
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
                                else if (DEBUG) printf("Plot mode, unknown char %d, plotPointMode = %d\n",ch,plotPointMode);
                                return;
                        }
                                
                }

                
                // handling anything specific to a mode
                
                switch (mode) {                                
                        case 1: plotPointMode = 0; // normal graphics mode, starting coordinates
                                yh = 32 * (ch & 31); mode++;
                                if (DEBUG) printf("setting yh to %d\n", yh);
                                break;
                        case 2: yl = (ch & 31);
                                mode++;
                                if (DEBUG) printf("setting yl to %d\n", yl);
                                break;
                        case 3: if (tag == 1) xh = 32 * (ch & 31); mode++;
                                if (DEBUG) printf("setting xh to %d\n", xh);
                                break;
                        case 4: xl = (ch & 31);
                                if (windowWidth != 1024) {
                                        int xb = xy4014 & 3;
                                        tube_x0 = (int)(efactor * (double)(((xh+xl) << 2) + xb) / 4.0);
                                        int yb = (xy4014 >> 2) & 3;
                                        tube_y0 = (int)(efactor * (double)(((yh+yl) << 2) + yb) / 4.0);
                                }
                                else {
                                        tube_x0 = xh + xl;
                                        tube_y0 = yh + yl;
                                }
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
                                else if (DEBUG) printf("case 5: tag is 0\n");
                                if (DEBUG) printf(">>>>>yh=%d\n",yh);
                                break;
                        case 6: yl = (ch & 31);               mode++;
                                break;
                                if (DEBUG) printf(">>>>>yl=%d\n",yl);
                        case 7: xh = 32 * (ch & 31);          mode++;
                                break;
                                if (DEBUG) printf(">>>>>xh=%d\n",xh);
                        case 8: xl = (ch & 31);
                                if (windowWidth != 1024) {
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
                                        
                                        // draw the point
                                        tube_drawPoint(cr, cr2);
                                        
                                        todo--;
                                }
                                
                                else {
                                        tube_drawVector(cr,cr2);
                                        
                                        todo--;                                        
                                }
                                
                                showCursor = 0;
                                
                                tube_x0 = tube_x2;        // prepare for additional vectors
                                tube_y0 = tube_y2;
                                if (specialPlotMode) mode = 50;  // another intensity/focus char follows                             
                                else mode = 5;                                
                                break;
                        case 30: // escape code handler
                                tek4010_escapeCodeHandler(cr, cr2, ch);
                                break;
                        case 31: // ANSI CSI sequence
                                printf("%c",ch);
                                if ((ch<0x20) || (ch>0x3F)) {
                                  mode=0;
                                  printf("\n");
                                }
                        case 40: // incremental plot mode
                                tek4010_checkReturnToAlpha(ch);  // check for exit
                                if (DEBUG) printf("Incremental plot mode, ch = %d, penDown = %d\n",ch, penDown);
                                if (ch == 32) penDown = 0;
                                else if (ch == 80) penDown = 1;
                                else if ((ch & 0x70) == 0x40){
                                        if (ch & 4) tube_y0++;
                                        if (ch & 1) tube_x0++;
                                        if (ch & 8) tube_y0--;
                                        if (ch & 2) tube_x0--;
                                        if (DEBUG) printf("point (%d,%d)\n", tube_x0, tube_y0);
                                        tube_x2 = tube_x0;
                                        tube_y2 = tube_y0;
                                        if (penDown) tube_drawPoint(cr, cr2);
                                }
                                else if (DEBUG) printf("Illegal byte 0x%02X in incremental plot\n", ch);  
                                break;
                        case 50:// special plot mode
                                tag = ch >> 5;
                                if ((ch < 32) || (ch >= 126)) return;
                                if (DEBUG) printf("intensity/focus control = %c: %d: ", ch, tag);
                                defocussed = (tag == 1);
                                intensity = intensityTable[ch - 32];
                                if (DEBUG) printf("defocussed = %d, intensity = %d%%\n", defocussed, intensity);
                                mode = 5; // coordinates follow                                
                                break;
                        case 60:// crosshair mode
                                if (isGinMode > 1) { // key stroken by user
                                        ginSend(isGinMode);
                                        mode = 0;
                                        todo = 0;
                                        if (DEBUG) printf("GIN: key stroken by user, exiting GIN mode\n");
                                        isGinMode = 0;
                                }
                                else
                                        ginMode(cr, cr2);
                                break;
                        case 101: 
                                if (DEBUG) printf("Ignore until group separator, ch = %02x\n", ch);
                                if (ch == 29) mode = 1;
                                break;
                        case 0: // handle ALPHA mode; 4014 user manual, table page G-1
                                // some characters are indentical for all modes and handled elsewhere
                                switch (ch) {
                                case 0:     break;
                                case 8:     // backspace
                                            tube_x0 -= hDotsPerChar;
                                            tek4010_checkLimits(cr, cr2);
                                            break;
                                case 9:     // tab
                                            if (argTab1)
                                                tube_x0 += hDotsPerChar;
                                            else
                                                tube_x0 = tube_x0 - (tube_x0 % (8 * hDotsPerChar)) + 8 * hDotsPerChar;
                                            tek4010_checkLimits(cr, cr2);
                                            break;
                                case 11:    // VT, move one line up
                                            tube_y0 += vDotsPerChar;
                                            tek4010_checkLimits(cr, cr2);
                                            break;
                                case 23:    // ctrl-w  screen dump
                                            system("scrot --focussed");
                                            break;

                                default:    if ((ch >= 32) && (ch <127)) { // printable character
                                                tek4010_checkLimits(cr, cr2);
                                                tube_drawCharacter(cr,cr2, ch);                                                
                                                todo-= 2;
                                            }
                                            break;
                                }
                                break;
                        default: if (DEBUG) printf("Illegal mode - this is a tek4010decoder error and should not happen\n");
                                break;
                }
                endDo:;
        }
        while ((todo > 0) && ((tube_mSeconds() - startPaintTime) < refresh_interval));
        
        // display cursor
        
        if (showCursor && (tube_isInput() == 0)) tube_doCursor(cr2);
        
}
