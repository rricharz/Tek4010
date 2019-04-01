/*
 * tek4010.c
 * 
 * A tek 4010 graphics emulator
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
 
#define DEBUG 0         // print debug info

#define TODO  99999         // for speed reasons, draw multiple objects until screen updates

#include <stdio.h>
#include <stdlib.h>
#include <string.h> 
#include <termios.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <errno.h>

#include "main.h"
#include "tek4010.h"

extern void gtk_main_quit();
extern int globalClearPersistent;

int noexit = 0;
int showCursor;
int isBrightSpot = 0;

struct tableEntry {
        int x0;
        int y0;
        int x2;
        int y2;
        int linewidth;
        double intensity;
} vectorTable;

int count = 0;
static int x0,y0,x2,y2,xh,xl,yh,yl,xy4014;
static int plotPointMode = 0;

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
// mode 40      incremental plot (4104); is ignored until exit from incremental plot received
// mode 50      special point plot mode; not yet implemented

static int mode;

int leftmargin;

int hDotsPerChar;
int vDotsPerChar;

FILE *getData;
int getDataPipe[2];

FILE *putKeys;
int putKeysPipe[2];

int isInput()
// is char available on getDataPipe?
{
        int bytesWaiting;
        ioctl(getDataPipe[0], FIONREAD, &bytesWaiting);
        return bytesWaiting;
}

int getInputChar()
// get a char from getDataPipe, if available, otherwise return -1
{
        if (isInput()) {
                int c = getc(getData) & 0x7F;
                if (DEBUG) printf(">>%02X<<",c);
                return c;
        }
        else
                return -1;
}

void tek4010_init(int argc, char* argv[])
// put any code here to initialize the tek4010
{
        char *argv2[10];
        size_t bufsize = 127;
        if ((argc<2) || (argc>9)) {
                printf("Error:number of arguments\n");
                exit(1);
        }
        
        if (strcmp(argv[argc-1],"-noexit") == 0) {
                noexit = 1;
                argc--;
        }
                
        // A child process for rsh is forked ans communication
        // between parent and child are established
        
        // expand argv[1] to full path and check, whether it exists
        char *str = (char *) malloc(bufsize * sizeof(char));
        if (str == NULL) {
               printf("Cannot allocate memory for absolute path\n");
               exit(1);
        }
        strcpy(str,"which ");
        strcat(str, argv[1]);
        FILE *fullPath = popen(str,"r");
        if (fullPath) {
                getline(&str, &bufsize,fullPath);
                // remove the endline character
                str[strlen(str)-1] = 0;
                argv[1] = str;
                pclose(fullPath);
        }
        else {
                printf("Cannot find command %s\n", argv[1]);
                exit(1);
        }
                
        hDotsPerChar  = WINDOW_WIDTH / 74;
        vDotsPerChar  = WINDOW_HEIGHT / 35;
        globalClearPersistent = 1;
                
        // create pipes for communication between parent and child
        if (pipe(getDataPipe) == -1) {
                printf("Cannot initialize data pipe\n");
                exit(1);
        }
                
        if (pipe(putKeysPipe) == -1) {
                printf("Cannot initialize key pipe\n");
                exit(1);
        }
                
        // now fork a child process
        pid_t pid = fork();
        if (pid == -1) {
                printf("Cannot fork child process\n");
                exit(1);
        }
        else if (pid == 0) {  // child process
                
                // we need a second string array with an empty string as last item!
                argv2[0] = argv[1];
                argv2[argc] = (char*) NULL;
                for (int i = 1; i < argc; i++)
                        argv2[i] = argv[i];
                        
                // set stdout of child process to getDataPipe
                while ((dup2(getDataPipe[1], STDOUT_FILENO) == -1) && (errno == EINTR)) {}
                close(getDataPipe[1]); // not used anymore
                close(getDataPipe[0]); // not used
                        
                // set stdin of child process to putKeysPipe
                while ((dup2(putKeysPipe[0], STDIN_FILENO) == -1) && (errno == EINTR)) {}
                close(putKeysPipe[1]); // not used
                close(putKeysPipe[0]); // not used anymore
                
                // run rsh in the child process
                execv(argv2[0],argv2+1);
                free(str);
                exit(0);
        }
                
        // parent process
        
        free(str);
                
        close(getDataPipe[1]); // not used
        close(putKeysPipe[0]); // not used
                
        // use termios to turn off line buffering for both pipes
        // struct termios term;
        // tcgetattr(getDataPipe[0], &term);
        // term.c_lflag &= ~ICANON ;
        // tcsetattr(getDataPipe[0], TCSANOW,&term);
        // tcgetattr(putKeysPipe[1], &term);
        // tcsetattr(putKeysPipe[0], TCSANOW,&term);
                
        // open now a stream from the getDataPipe descriptor
        getData = fdopen(getDataPipe[0],"r");
        if (getData == 0) {
                printf("Parent: Cannot open input stream\n");
                exit(1);
        }
        setbuf(getData,0);
                
        // open now a stream from the putKeysPipe descriptor
        putKeys = fdopen(putKeysPipe[1],"w");
        if (putKeys == 0) {
                printf("Parent: Cannot open output stream\n");
                exit(1);
        }
        setbuf(putKeys,0);
        
        vectorTable.intensity = 0.0;                
}

int tek4010_on_timer_event()
// if TIMER_INTERVAL in tek4010.h is larger than zero, this function
// is called every TIMER-INTERVAL milliseconds
// if the function returns 1, the window is redrawn by calling applicatin_draw
{
        // if there is a char available on the imput stream
        // or there is still a bright spot, return 1 to ask for
        // one more redraw
       
        return (isBrightSpot || isInput());
}
 
int tek4010_clicked(int button, int x, int y)
// is called if a mouse button is clicked in the window
// button = 1: means left mouse button; button = 3 means right mouse button
// x and y are the coordinates
// if the function returns 1, the window is redrawn by calling applicatin_draw
{
	return 1;
}

void tek4010_quit()
// is called if the main window is called bevore the tek4010 exits
// put any code here which needs to be called on exit
{
        pclose(getData);
}

int checkExitFromGraphics(int ch)
// test for exit from graphics character
{
        if ((ch==31) || (ch==13) || (ch==27) || (ch==12)) {
                mode = 0;
                if (DEBUG) printf("Leaving graphics mode\n");
                showCursor = 0;
                if (ch == 12) globalClearPersistent = 1;
                plotPointMode = 0;
                return 1;
        }
        else return 0;
}

void doCursor(cairo_t *cr2)
{
        cairo_set_source_rgb(cr2, 0, 0.8, 0);
        cairo_set_line_width (cr2, 1);
        cairo_rectangle(cr2, x0, WINDOW_HEIGHT - y0 - vDotsPerChar + 8,
                                                hDotsPerChar - 3, vDotsPerChar - 3);
        cairo_fill(cr2);
        cairo_stroke (cr2);
}

void clearPersistent(cairo_t *cr, cairo_t *cr2)
// clear the persistant surface
// flash using the second surface
{
        cairo_set_source_rgb(cr, 0, 0, 0);
        cairo_paint(cr);
        globalClearPersistent = 0;
        x0 = 0;
        y0 = WINDOW_HEIGHT - vDotsPerChar;
        leftmargin = 0;
        cairo_set_source_rgb(cr, 0, 0.7, 0);
        cairo_set_source_rgb(cr2, 0.5, 1, 0.5);
        cairo_paint(cr2);
        isBrightSpot = 1;
        plotPointMode = 0;
}


void tek4010_draw(cairo_t *cr, cairo_t *cr2, int width, int height, int first)
// draw onto the main window using cairo
// width is the actual width of the main window
// height is the actual height of the main window
// cr is used for persistent drawing, cr2 for temporary drawing 

{		
        int ch;
        int todo;
        char s[2];

        showCursor = 1;
        isBrightSpot = 0;
        
        cairo_set_source_rgba(cr2, 0, 0, 0, 0); // second surface is cleared each time
        cairo_set_operator(cr2, CAIRO_OPERATOR_SOURCE);
        cairo_paint(cr2);
        cairo_set_operator(cr2, CAIRO_OPERATOR_OVER);
        
        // clear persistent surface, if necessary
        if (globalClearPersistent) {
                clearPersistent(cr,cr2);
        }
        
        cairo_set_antialias(cr, CAIRO_ANTIALIAS_BEST);      
	cairo_set_line_width (cr, 1);
        cairo_set_source_rgb(cr, 0, 0.7, 0);
        
        cairo_select_font_face(cr, "Monospace", CAIRO_FONT_SLANT_NORMAL, CAIRO_FONT_WEIGHT_NORMAL);
        cairo_set_font_size(cr, 18);
        cairo_select_font_face(cr2, "Monospace", CAIRO_FONT_SLANT_NORMAL, CAIRO_FONT_WEIGHT_BOLD);
        cairo_set_font_size(cr2, 18);
        
        if (plotPointMode) todo = 5 * TODO;  // more dots before refresh in plot point mode
        else todo = TODO;
        
        do {
                ch = getInputChar();
                
                // fade away last bright spot vector
                if (vectorTable.intensity > 0.1) {
                        cairo_set_line_width (cr2, vectorTable.linewidth);
                        cairo_set_source_rgb(cr2, 0.25, 0.7, 0.25);
                        cairo_move_to(cr2, vectorTable.x0, WINDOW_HEIGHT - vectorTable.y0);
                        cairo_line_to(cr2, vectorTable.x2, WINDOW_HEIGHT - vectorTable.y2);
                        cairo_stroke (cr2);
                        vectorTable.intensity = 0.0;
                        isBrightSpot = 1;
                }

                if (ch == -1) {
                        if ((mode == 0) && showCursor) doCursor(cr2);
                        return;         // no char available, need to allow for updates
                }
                
                if (DEBUG) {
                        printf("mode=%d, ch code %02X",mode,ch);
                        if ((ch>0x20)&&(ch<=0x7E)) printf(" (%c)",ch);
                        printf("\n");
                }
                
                if (mode == 31) {
                        // printf("ANSI escape mode 31, ch=%02x\n",ch);
                        if ((ch>='0') && (ch<='9')) mode = 30;
                }
                
                int tag = (ch >> 5) & 3;
                                
                if ((mode >= 1) && (mode <= 8)) {
                        
                        checkExitFromGraphics(ch);
                        
                        // handle skipped coordinate bytes
                        // this cannot be done in switch(mode) below, because multiple bytes
                        // can be switched and the current byte must be executed after a mode change
                        
                        if ((mode == 3) && (tag == 3)) {
                                // this overwrites the extra data byte of the 4014 for the
                                // first dark mode coordinates and stores it for further use
                                mode = 2;
                                xy4014 = yl;
                                if (DEBUG) printf("4014 coordinates, overwrite last value\n");
                        }
                        
                        if ((mode == 5) && (ch == 29)) {
                                mode = 1; return;
                        }
                        if (ch == 30) {
                                if (DEBUG) printf("Starting incremental plot mode (4014)\n");
                                mode = 40; return;
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
                                
                }
                
                switch (mode) {                                
                        case 1: plotPointMode= 0; // normal graphics mode, starting coordinates
                                yh = 32 * (ch & 31); mode++;
                                break;
                        case 2: yl = (ch & 31); y0 = yh + yl; mode++;
                                break;
                        case 3: xh = 32 * (ch & 31);          mode++;
                                break;
                        case 4: xl = (ch & 31); x0= xh + xl;  mode++; 
                                        if (DEBUG) printf("***** Moving to (%d,%d)\n",x0,y0);
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
                                x2 = xh + xl;
                                y2 = yh + yl;
                                if (DEBUG) printf(">>>>>xl=%d\n",xl);
                                
                                if (plotPointMode>0.0) {
                                        if (DEBUG) printf("plotting point at %d,%d,todo =%d\n",
                                                        x2,y2,todo);
                                        cairo_set_source_rgb(cr, 0, 1, 0);
                                        cairo_move_to(cr, x2, WINDOW_HEIGHT - y2);
                                        cairo_line_to(cr, x2+1, WINDOW_HEIGHT - y2);
                                        cairo_stroke (cr);
                                        cairo_set_source_rgb(cr, 0, 0.7, 0);
                                        mode = 50;
                                        todo--;
                                }
                                
                                else {
                                
                                        if (DEBUG) printf("tag=%d,***** Drawing vector to (%d,%d)\n",tag,x2,y2);
                                        cairo_move_to(cr, x0, WINDOW_HEIGHT - y0);
                                        cairo_line_to(cr, x2, WINDOW_HEIGHT - y2);
                                        cairo_stroke (cr);
                                
                                        cairo_set_line_width (cr2, 3);
                                        cairo_set_source_rgb(cr2, 0.5, 1, 0.5);                        
                                        cairo_move_to(cr2, x0, WINDOW_HEIGHT - y0);
                                        cairo_line_to(cr2, x2, WINDOW_HEIGHT - y2);
                                        cairo_stroke (cr2);
                                        isBrightSpot = 1;
                                                // for speed reasons, do not update screen right away
                                        // if many very small vectors are drawn
                                        todo--;
                                        if ((x2-x0) > 25) todo = 0;
                                        if ((x0-x2) > 25) todo = 0;
                                        if ((y2-y0) > 25) todo = 0;
                                        if ((y0-y2) > 25) todo = 0;
                                        
                                        // save bright spot vector for fading
                                
                                        vectorTable.x0 = x0;
                                        vectorTable.x2 = x2;
                                        vectorTable.y0 = y0;
                                        vectorTable.y2 = y2;
                                        vectorTable.linewidth = 2;
                                        vectorTable.intensity = 0.8;
                                }
                                
                                showCursor = 0;
                                
                                x0 = x2;        // prepare for additional vectors
                                y0 = y2;                                
                                mode = 5;
                                
                                break;
                        case 30:                // handle escape sequencies
                                if (DEBUG) printf("Escape mode, ch=%02X\n",ch);
                                switch (ch) {
                                    case 12:
                                         if (DEBUG) printf("Form feed, clear screen\n");
                                         clearPersistent(cr,cr2);
                                         mode = 0;
                                         break;
                                    case '[': // a second escape code follows, do not reset mode
                                         break;
// start of ignoring ANSI escape sequencies, could be improved (but the Tek4010 couldn't do this either!)
                                    case '0':
                                    case '1':
                                    case '2':
                                    case '3':
                                    case '4':
                                    case '5':
                                    case '6':
                                    case '7':
                                    case '8':
                                    case '9': break;
                                    case ';': mode = 31; break;
                                    case ']': break;
                                    case 'm': mode = 0; break;
// end of ignoring ANSI escape sequencies
                                    default: 
                                         if (DEBUG) printf("Ignore\n");
                                         mode = 0;
                                         break;                                               
                                }
                                break;
                        case 40: // used to ignore certain 4014 sequencies
                                if (ch == 31) mode = 0;  // leave this mode
                                break;
                        default: switch (ch) {
                                case 0:     break;
                                case EOF:   break;
                                case 8:     // backspace
                                            x0 -= hDotsPerChar;
                                            if (x0<leftmargin) x0 = leftmargin;
                                            break;
                                case 9:     // tab
                                            x0 = x0 - (x0 % (8 * hDotsPerChar)) + 8 * hDotsPerChar;
                                            break;
                                case 10:    // new line
                                            y0 -= vDotsPerChar;
                                            if (y0 < 4) {
                                                y0 = WINDOW_HEIGHT - vDotsPerChar;
                                                if (leftmargin) leftmargin = 0;
                                                else leftmargin = WINDOW_WIDTH / 2;
                                            }
                                            x0 = leftmargin;
                                            break;
                                case 11:    // VT, move one line up
                                            y0 += vDotsPerChar;
                                            break;
                                case 13:    // return
                                            mode = 0; x0 = leftmargin;
                                            break;
                                case 27:    // escape
                                            mode = 30;
                                            // printf("Starting escape mode\n");
                                            break;
                                case 28:    // file separator
                                            if (DEBUG) printf("Point plot mode\n");
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
                                default:    if ((ch >= 32) && (ch <=127)) { // printable character
                                                if (y0 < 8) y0 = 8;
                                                s[0] = ch;
                                                s[1] = 0;
                                                cairo_move_to(cr, x0, WINDOW_HEIGHT - y0 + 4);
                                                cairo_show_text(cr, s);
                                                cairo_set_source_rgb(cr2, 0.5, 1, 0.5);
                                                cairo_move_to(cr2, x0, WINDOW_HEIGHT - y0 + 4);
                                                cairo_show_text(cr2, s);
                                                x0 += hDotsPerChar;
                                                isBrightSpot = 1;
                                                todo--;
                                            }
                                            break;
                                }
                                break;                                
                }
        }
        while (todo);
        
        // display cursor
        
        if (showCursor) doCursor(cr2);
        
        // is child process still running?
        
        int status;
        if ((! noexit) && (waitpid(-1, &status, WNOHANG))) {    // Is child process terminated?
                tek4010_quit();
                gtk_main_quit();
                printf("Child process has been terminated\n");
                exit(0);
        }
}
