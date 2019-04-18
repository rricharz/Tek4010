/*
 * tube.c
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
#define DEBUGMAX 0              // exit after DEBUGMAX chars, 0 means no exit

#define WRITE_TROUGH_INTENSITY  0.5             // green only
#define NORMAL_INTENSITY        0.8
#define CURSOR_INTENSITY        0.8
#define BRIGHT_SPOT_COLOR       1.0
#define BRIGHT_SPOT_COLOR_HALF  0.6
#define BLACK_COLOR             0.0,0.08,0.0    // effect of flood gun

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

extern void gtk_main_quit();
extern int windowWidth;
extern int windowHeight;
extern char *windowName;

int argNoexit = 0;              // options
int argRaw = 0;
int argBaud = 19200;
int argTab1 = 0;
int argFull = 0;
int argARDS = 0;

int refresh_interval;           // after this time in msec next refresh is done

int showCursor;                 // set of cursor is shown (not set in graphics mode)
int isBrightSpot = 0;           // set if there is currently a bright spot on the screen

int xlast, ylast;

enum LineType ltype;
double dashset[] = {2,6,2,2,6,3,3,3,6,6};

int plotPointMode = 0;           // plot point mode
int writeThroughMode = 0;        // write through mode
int tube_doClearPersistent;

int specialPlotMode = 0;
int defocussed = 0;
int intensity = 100;

int tube_x0, tube_x2,tube_y0, tube_y2;

static int debugCount = 0;

long refreshCount = 0;           // variables for baud rate and refresh rate measurements

static long charCount = 0;
static long charResetCount = 0;
static long characterInterval = 0;
static int currentFontSize = 18;

long startPaintTime;

int leftmargin;

int hDotsPerChar;
int vDotsPerChar;

FILE *getData;
int getDataPipe[2];

FILE *putKeys;
int putKeysPipe[2];

long tube_mSeconds()
// return time in msec since start of program
{
        static int initialized = 0;
        static long startTime;
        struct timeval tv;
        gettimeofday(&tv,NULL);
        long t = (1000 * tv.tv_sec)  + (tv.tv_usec/1000);
        if (!initialized) startTime = t;
        initialized = 1;
        t = t - startTime;
        if (t < 0) t += 86400000;        
        return t;
}

long tube_u100ResetSeconds(int reset)
// returns time in 100 usec since last reset
{
        static long startTime;
        struct timeval tv;
        gettimeofday(&tv,NULL);
        long t = (10000 * tv.tv_sec)  + (tv.tv_usec/100);
        if (reset) {
                startTime = t;
                charResetCount = 0;
        }
        t = t - startTime;
        if (t < 0) t += 864000000;       
        return t;
}

int tube_isInput()
// is char available on getDataPipe?
{
        int bytesWaiting;
        ioctl(getDataPipe[0], FIONREAD, &bytesWaiting);
        if (DEBUG) {
                debugCount++;
                if (DEBUGMAX && (debugCount > DEBUGMAX)) return 0;
        }
        if (bytesWaiting == 0) {
                // reset the baud rate counter
                // without this, baud rate goal would not work after waiting for chars
                tube_u100ResetSeconds(1);
        }
        return bytesWaiting;
}

int tube_getInputChar()
// get a char from getDataPipe, if available, otherwise return -1
{
        static long lastTime = 0;
        
        long t = tube_u100ResetSeconds(0);
        if (tube_isInput()) {
                // handle baud rate since last no input available
                if (t < charResetCount * characterInterval)
                        return -1; // there is time to refresh the screen
                int c = getc(getData) & 0x7F;
                charCount++;
                charResetCount++;
                lastTime = t;
                return c;
        }
        else
                return -1;
}

void tube_init(int argc, char* argv[])
// put any code here to initialize the tek4010
{
        char *argv2[20];
        size_t bufsize = 127;
        int firstArg = 1;
        printf("tek4010 version 1.2.4\n");
        windowName = "Tektronix 4010/4014 emulator";
        if ((argc<2) || (argc>19)) {
                printf("Error:number of arguments\n");
                exit(1);
        }
        
        // this stays here for compatibility with early versions of tek4010
        if (strcmp(argv[argc-1],"-noexit") == 0) {
                argNoexit = 1;
                argc--;
        }
        
        while ((argv[firstArg][0] == '-') && firstArg < argc-1) {
                if (strcmp(argv[firstArg],"-raw") == 0)
                        argRaw = 1;
                else if (strcmp(argv[firstArg],"-noexit") == 0)
                        argNoexit = 1;
                else if (strcmp(argv[firstArg],"-b9600") == 0)
                        argBaud = 9600;
                else if (strcmp(argv[firstArg],"-b4800") == 0)
                        argBaud = 4800;
                else if (strcmp(argv[firstArg],"-b2400") == 0)
                        argBaud = 2400;
                else if (strcmp(argv[firstArg],"-b1200") == 0)
                        argBaud = 1200;
                else if (strcmp(argv[firstArg],"-b600") == 0)
                        argBaud = 600;
                else if (strcmp(argv[firstArg],"-b300") == 0)
                        argBaud = 300;
                else if (strcmp(argv[firstArg],"-tab1") == 0)
                        argTab1 = 1;
                else if (strcmp(argv[firstArg],"-full") == 0)
                        argFull = 1;
                else if (strcmp(argv[firstArg],"-ARDS") == 0) {
                        argARDS = 1;
                        windowName = "ARDS emulator";
                }
                else {
                        printf("tek4010: unknown argument %s\n", argv[firstArg]);
                        exit(1);
                }
                firstArg++;
                
        }
                
        // A child process for rsh is forked and communication
        // between parent and child are established
        
        // expand argv[firstArg] to full path and check, whether it exists
        char *str = (char *) malloc(bufsize * sizeof(char));
        if (str == NULL) {
               printf("Cannot allocate memory for absolute path\n");
               exit(1);
        }
        strcpy(str,"which ");
        strcat(str, argv[firstArg]);
        FILE *fullPath = popen(str,"r");
        if (fullPath) {
                getline(&str, &bufsize,fullPath);
                
                // remove the endline character
                str[strlen(str)-1] = 0;
                
                if (strncmp(str,"which",5) == 0) {
                        printf("Unknown command %s\n", argv[firstArg]);
                        exit(1);
                }
                
                argv[firstArg] = str;
                pclose(fullPath);
        }
        else {
                printf("Unknown command %s\n", argv[firstArg]);
                exit(1);
        }
        
        characterInterval = 100000 / argBaud; // in 100 usecs, assuming 1 start and 1 stop bit.
        
        if (DEBUG) printf("character_interval = %0.1f msec\n",(double)characterInterval/10.0);
                
        tube_doClearPersistent = 1;
                
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
                argv2[0] = argv[firstArg];
                for (int i = 1; i < argc; i++)
                        argv2[i] = argv[firstArg+i-1];
                argv2[argc-firstArg+1] = (char*) NULL;
                
                // int i = 0;
                // do {
                //        printf("argv2[%d] = %s\n",i,argv2[i]);
                //        i++;
                // }
                // while (argv2[i] != (char*) NULL);
                        
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
        
        tube_mSeconds();             // initialize the timer
        tube_u100ResetSeconds(1);                        
}

int tube_on_timer_event()
// if TIMER_INTERVAL in tek4010.h is larger than zero, this function
// is called every TIMER-INTERVAL milliseconds
// if the function returns 1, the window is redrawn by calling applicatin_draw
{
        // if there is a char available on the imput stream
        // or there is still a bright spot, return 1 to ask for
        // one more redraw
       
        // is child process still running?
        
        int status;
        if ((!argNoexit) && (tube_isInput() == 0) && (waitpid(-1, &status, WNOHANG))) {
                long t = tube_mSeconds();
                printf("Execution time: %0.3f sec\n", (double)t/1000.0);
                if (t > 0) {
                        printf("Average screen refresh rate: %0.1f Hz\n",(double)(1000.0*refreshCount)/t);
                        printf("Average character rate: %0.0f baud\n",(double)(8000.0*charCount)/t);
                }
                tube_quit();
                gtk_main_quit();
                printf("Process has been terminated\n");
                exit(0);
        }
        return (isBrightSpot || tube_isInput());
}
 
int tube_clicked(int button, int x, int y)
// is called if a mouse button is clicked in the window
// button = 1: means left mouse button; button = 3 means right mouse button
// x and y are the coordinates
// if the function returns 1, the window is redrawn by calling applicatin_draw
{
	return 1;
}

void tube_quit()
// is called if the main window is called bevore the tek4010 exits
// put any code here which needs to be called on exit
{
        pclose(getData);
}

void tube_doCursor(cairo_t *cr2)
{
        cairo_set_source_rgb(cr2, 0, CURSOR_INTENSITY, 0);
        cairo_set_line_width (cr2, 1);
        cairo_rectangle(cr2, tube_x0, windowHeight - tube_y0 - vDotsPerChar + 8,
                                                hDotsPerChar - 3, vDotsPerChar - 3);
        cairo_fill(cr2);
        cairo_stroke (cr2);
}

void tube_clearPersistent(cairo_t *cr, cairo_t *cr2)
// clear the persistant surface
// flash using the second surface
{
        cairo_set_source_rgb(cr, BLACK_COLOR);
        cairo_paint(cr);
        tube_doClearPersistent = 0;
        tube_x0 = 0;
        tube_y0 = windowHeight - vDotsPerChar;
        tube_x2 = tube_x0;
        tube_y2 = tube_y0;
        leftmargin = 0;
        cairo_set_source_rgb(cr, 0, NORMAL_INTENSITY, 0);
        cairo_set_source_rgb(cr2, BRIGHT_SPOT_COLOR, BRIGHT_SPOT_COLOR, BRIGHT_SPOT_COLOR);
        cairo_paint(cr2);
        isBrightSpot = 1;
        plotPointMode = 0;
        ltype = SOLID;
        xlast = 0;
        ylast = 0;
        specialPlotMode = 0;
        defocussed = 0;
        intensity = 100; 
}

void tube_clearSecond(cairo_t *cr2)
// clear second surface
{ 
        cairo_set_source_rgba(cr2, 0, 0, 0, 0);
        cairo_set_operator(cr2, CAIRO_OPERATOR_SOURCE);
        cairo_paint(cr2);
        cairo_set_operator(cr2, CAIRO_OPERATOR_OVER);
}

void tube_line_type(cairo_t *cr, cairo_t *cr2, enum LineType ln)
{
    int ndash,ndx;
    double ofs = 0.5;

    switch (ln) {
    case SOLID:
        ndx = 0;
        ndash = 0;
        break;
    case DOTTED:
        ndx = 0;
        ndash = 2;
        break;
    case DOTDASH:
        ndx = 2;
        ndash = 4;
        break;
    case LONGDASH:
        ndx = 8;
        ndash = 2;
        break;
    case SHORTDASH:
        ndx = 6;
        ndash = 2;
        break;
    }
    cairo_set_dash (cr,&dashset[ndx],ndash,ofs);
    cairo_set_dash (cr2,&dashset[ndx],ndash,ofs);
}

void tube_drawCharacter(cairo_t *cr, cairo_t *cr2, char ch)
{
        char s[2];
        s[0] = ch;
        s[1] = 0;
        
        cairo_set_font_size(cr, currentFontSize);
        cairo_set_font_size(cr2,currentFontSize);                                              

        if (writeThroughMode) {  // draw the write-through character
                cairo_set_source_rgb(cr2, 0, WRITE_TROUGH_INTENSITY, 0);
                cairo_move_to(cr2, tube_x0, windowHeight - tube_y0);
                cairo_show_text(cr2, s);
        }
                                                
        else {
                // draw the character
                cairo_set_source_rgb(cr, 0, (NORMAL_INTENSITY * intensity) / 100, 0);
                cairo_move_to(cr, tube_x0, windowHeight - tube_y0);
                cairo_show_text(cr, s);
                                                
                // draw the bright spot
                cairo_set_source_rgb(cr2, BRIGHT_SPOT_COLOR, BRIGHT_SPOT_COLOR, BRIGHT_SPOT_COLOR);
                cairo_move_to(cr2, tube_x0, windowHeight - tube_y0);
                cairo_show_text(cr2, s);                        
        }
                                                
        tube_x0 += hDotsPerChar;
        isBrightSpot = 1;
}

void tube_emulateDeflectionTime()
{
        // find length of longer component
        int l = tube_x2 - tube_x0;
        if ((tube_x0-tube_x2) > l) l = tube_x0 - tube_x2;
        if ((tube_y2-tube_y0) > l) l = tube_y2 - tube_y0;
        if ((tube_y0-tube_y2) > l) l = tube_y0 - tube_y2;
        if (l > 300) {  // the 300 accounts for other overheads
                usleep((l - 300) * 2);  // roughly 2 usec per dot
        }
}

void tube_drawPoint(cairo_t *cr, cairo_t *cr2)
{
#define PI2 6.283185307
        cairo_set_line_width (cr, 1 + defocussed);
        cairo_set_source_rgb(cr, 0, (NORMAL_INTENSITY * intensity) / 100, 0);
        cairo_move_to(cr, tube_x2, windowHeight - tube_y2);
        cairo_line_to(cr, tube_x2 + 1, windowHeight - tube_y2);
        cairo_stroke (cr);
                                        
        // speed is a problem here
        // do not draw adjacent bright spots
                                        
        if (((tube_x2 - xlast) > 2) || ((xlast - tube_x2) > 2) ||
                ((tube_y2 - ylast) > 2) || ((ylast - tube_y2) > 2))  {
                                        
                // draw the bright spot
                cairo_set_line_width (cr2, 0.1);
                double bsc = (BRIGHT_SPOT_COLOR * intensity) / 100;
                cairo_set_source_rgb(cr2, bsc, bsc, bsc);                        
                cairo_arc(cr2, tube_x2, windowHeight - tube_y2, 2 + defocussed, 0, PI2);
                cairo_fill(cr2);
                                                
                xlast = tube_x2;
                ylast = tube_y2;
        }
                                        
        isBrightSpot = 1;      
}

void tube_drawVector(cairo_t *cr, cairo_t *cr2)
{
        if (DEBUG) {
                printf("********************************************");
                printf("Drawing from (%d,%d) to (%d,%d), writethrough = %d\n",
                                tube_x0, tube_y0, tube_x2, tube_y2, writeThroughMode);
        }
        tube_emulateDeflectionTime();

        if ((tube_x2 == tube_x0) && (tube_y2 == tube_y0)) tube_x0++; // cairo cannot draw a dot

        if (writeThroughMode) {
                cairo_set_line_width (cr2, 2);
                cairo_set_source_rgb(cr2, 0.0, WRITE_TROUGH_INTENSITY, 0.0);
                cairo_move_to(cr2, tube_x0, windowHeight - tube_y0);
                cairo_line_to(cr2, tube_x2, windowHeight - tube_y2);
                cairo_stroke (cr2);
        }
        
        else {
                // draw the actual vector on permanent surface
                cairo_set_line_width (cr, 1 + defocussed);
                cairo_set_source_rgb(cr, 0, (NORMAL_INTENSITY * intensity) / 100, 0);
                tube_line_type(cr, cr2, ltype);
                cairo_move_to(cr, tube_x0, windowHeight - tube_y0);
                cairo_line_to(cr, tube_x2, windowHeight - tube_y2);
                cairo_stroke (cr);
        
                //draw the bright spot, half intensity
                cairo_set_line_width (cr2, 6 + 2 * defocussed);
                double bsc = (BRIGHT_SPOT_COLOR_HALF * intensity) / 100;
                cairo_set_source_rgb(cr2, bsc, bsc, bsc);                        
                cairo_move_to(cr2, tube_x0, windowHeight - tube_y0);
                cairo_line_to(cr2, tube_x2, windowHeight - tube_y2);
                cairo_stroke (cr2);
                                        
                // draw the bright spot, high intensity
                cairo_set_line_width (cr2, 3 + 2 * defocussed);
                bsc = (BRIGHT_SPOT_COLOR * intensity) / 100;
                cairo_set_source_rgb(cr2, bsc, bsc, bsc);                        
                cairo_move_to(cr2, tube_x0, windowHeight - tube_y0);
                cairo_line_to(cr2, tube_x2, windowHeight - tube_y2);
                cairo_stroke(cr2);
        }

        isBrightSpot = 1; // also to be set if writeThroughMode
}

void tube_setupPainting(cairo_t *cr, cairo_t *cr2, char *fontName)
{
        cairo_set_antialias(cr, CAIRO_ANTIALIAS_BEST);      
	cairo_set_line_width (cr, 1);
        cairo_set_source_rgb(cr, 0, NORMAL_INTENSITY, 0);        
        cairo_select_font_face(cr, fontName, CAIRO_FONT_SLANT_NORMAL, CAIRO_FONT_WEIGHT_NORMAL);
        cairo_select_font_face(cr2, fontName, CAIRO_FONT_SLANT_NORMAL, CAIRO_FONT_WEIGHT_BOLD);        
}

void tube_changeCharacterSize(cairo_t *cr, cairo_t *cr2,int charsPerLine, int charsPerPage, int fontSize)
{
        int fontsize;
        hDotsPerChar = windowWidth / charsPerLine;
        vDotsPerChar = windowHeight / charsPerPage;
        leftmargin = 0;
        currentFontSize = fontSize;
}
