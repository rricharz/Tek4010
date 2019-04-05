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
 
#define DEBUG    0              // print debug info
#define DEBUGMAX 0              // exit after DEBUGMAX chars, 0 means no exit

#define REFRESH_INTERVAL 30     // time in msec between refresh events

#define TODO  8                 // draw multiple objects until screen updates
                                // if this value is too large, drawing will become choppy
                                
#define PI2 6.283185307

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

int argNoexit = 0;              // options
int argRaw = 0;
int argBaud = 19200;
int argTab1 = 0;
int argFull = 0;

int showCursor;                         // set of cursor is shown (not set in graphics mode)
int isBrightSpot = 0;                   // set if there is currently a bright spot on the screen

int count = 0;
static int x0,y0,x2,y2,xh,xl,yh,yl,xy4014;

enum LineType {SOLID,DOTTED,DOTDASH,SHORTDASH,LONGDASH};
enum LineType ltype;
double dashset[] = {2,6,2,2,6,3,3,3,6,6};

static int plotPointMode = 0;           // plot point mode
static int writeThroughMode = 0;        // write through mode
static int debugCount = 0;
static double efactor = 0.0;

static long refreshCount = 0;           // variables for baud rate and refresh rate measurements
static long charCount = 0;
static long charResetCount = 0;
static long characterInterval = 0;

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
// mode 101     ignore until group separator, not yet implemented

static int mode, savemode;

int leftmargin;

int hDotsPerChar;
int vDotsPerChar;

FILE *getData;
int getDataPipe[2];

FILE *putKeys;
int putKeysPipe[2];

long mSeconds()
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

long u100ResetSeconds(int reset)
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

int isInput()
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
                u100ResetSeconds(1);
        }
        return bytesWaiting;
}

int getInputChar()
// get a char from getDataPipe, if available, otherwise return -1
{
        static long lastTime = 0;
        
        long t = u100ResetSeconds(0);
        if (isInput()) {
                // handle baud rate since last no input available
                if (t < charResetCount * characterInterval)
                        return -1; // there is time to refresh the screen
                int c = getc(getData) & 0x7F;
                if (DEBUG) printf(">>%02X<<",c);
                charCount++;
                charResetCount++;
                lastTime = t;
                return c;
        }
        else
                return -1;
}

void tek4010_init(int argc, char* argv[])
// put any code here to initialize the tek4010
{
        char *argv2[20];
        size_t bufsize = 127;
        int firstArg = 1;
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
                else {
                        printf("tek4010: unknown argument %s\n", argv[firstArg]);
                        exit(1);
                }
                firstArg++;
                
        }
                
        // A child process for rsh is forked ans communication
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
        
        characterInterval = 80000 / argBaud; // in 100 usecs
        
        if (DEBUG) printf("character_interval = %0.1f msec\n",(double)characterInterval/10.0);
                
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
        
        mSeconds();             // initialize the timer
        u100ResetSeconds(1);                        
}

int tek4010_on_timer_event()
// if TIMER_INTERVAL in tek4010.h is larger than zero, this function
// is called every TIMER-INTERVAL milliseconds
// if the function returns 1, the window is redrawn by calling applicatin_draw
{
        // if there is a char available on the imput stream
        // or there is still a bright spot, return 1 to ask for
        // one more redraw
       
        // is child process still running?
        
        int status;
        if ((!argNoexit) && (isInput() == 0) && (waitpid(-1, &status, WNOHANG))) {
                long t = mSeconds();
                printf("Execution time: %0.3f sec\n", (double)t/1000.0);
                if (t > 0) {
                        printf("Average screen refresh rate: %0.1f Hz\n",(double)(1000.0*refreshCount)/t);
                        printf("Average character rate: %0.0f baud\n",(double)(8000.0*charCount)/t);
                }
                tek4010_quit();
                gtk_main_quit();
                printf("Process has been terminated\n");
                exit(0);
        }
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
        cairo_rectangle(cr2, x0, windowHeight - y0 - vDotsPerChar + 8,
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
        y0 = windowHeight - vDotsPerChar;
        leftmargin = 0;
        cairo_set_source_rgb(cr, 0, 0.7, 0);
        cairo_set_source_rgb(cr2, 0.8, 1, 0.8);
        cairo_paint(cr2);
        isBrightSpot = 1;
        plotPointMode = 0;
        ltype = SOLID;  
}

void clearSecond(cairo_t *cr2)
// clear second surface
{ 
        cairo_set_source_rgba(cr2, 0, 0, 0, 0);
        cairo_set_operator(cr2, CAIRO_OPERATOR_SOURCE);
        cairo_paint(cr2);
        cairo_set_operator(cr2, CAIRO_OPERATOR_OVER);
}

void tek4010_line_type(cairo_t *cr, cairo_t *cr2, enum LineType ln)
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

void drawVector(cairo_t *cr, cairo_t *cr2)
{
        if (DEBUG) printf("******************************************** Drawing to (%d,%d)\n",x2,y2);
        if ((x2 == x0) && (y2 == y0)) x0++; // cairo cannot draw a dot
        if (writeThroughMode) {
                cairo_set_line_width (cr2, 1);
                cairo_set_source_rgb(cr2, 0.0, 1.0, 0.0);
                cairo_move_to(cr2, x0, windowHeight - y0);
                cairo_line_to(cr2, x2, windowHeight - y2);
                cairo_stroke (cr2);
        }
        
        else {
                tek4010_line_type(cr, cr2, ltype);
                cairo_move_to(cr, x0, windowHeight - y0);
                cairo_line_to(cr, x2, windowHeight - y2);
                cairo_stroke (cr);
        
                //draw the bright spot, lower intensity
                cairo_set_line_width (cr2, 9);
                cairo_set_source_rgb(cr2, 0.1, 0.3, 0.1);                        
                cairo_move_to(cr2, x0, windowHeight - y0);
                cairo_line_to(cr2, x2, windowHeight - y2);
                cairo_stroke (cr2);
                                        
                //draw the bright spot, medium intensity
                cairo_set_line_width (cr2, 6);
                cairo_set_source_rgb(cr2, 0.3, 0.6, 0.3);                        
                cairo_move_to(cr2, x0, windowHeight - y0);
                cairo_line_to(cr2, x2, windowHeight - y2);
                cairo_stroke (cr2);
                                        
                // draw the bright spot, higher intensity
                cairo_set_line_width (cr2, 4);
                cairo_set_source_rgb(cr2, 1, 1, 1);                        
                cairo_move_to(cr2, x0, windowHeight - y0);
                cairo_line_to(cr2, x2, windowHeight - y2);
                cairo_stroke(cr2);
        }

        isBrightSpot = 1; // also to be set if writeThroughMode
}

int escapeCodeHandler(cairo_t *cr, cairo_t *cr2, int todo, int ch)
// handle escape sequencies
// returns todo to allow changes to todo
{
        if (DEBUG) printf("Escape mode, ch=%02X\n",ch);
        switch (ch) {
                case 12:
                        if (DEBUG) printf("Form feed, clear screen\n");
                        clearPersistent(cr,cr2);
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
                case '9':       break;
                case ';':       mode = 31; break;
                case ']':       break;
                case 'm':       mode = 0; break;
                
                // end of ignoring ANSI escape sequencies

                case '`': ltype = SOLID;    writeThroughMode = 0; mode = 0; break;
                case 'a': ltype = DOTTED;   writeThroughMode = 0; mode = 0; break;
                case 'b': ltype = DOTDASH;  writeThroughMode = 0; mode = 0; break;
                case 'c': ltype = SHORTDASH;writeThroughMode = 0; mode = 0; break;
                case 'd': ltype = LONGDASH; writeThroughMode = 0; mode = 0; break;
                case 'e': ltype = SOLID;    writeThroughMode = 0; mode = 0; break;
                case 'f': ltype = SOLID;    writeThroughMode = 0; mode = 0; break;
                case 'h': ltype = SOLID;    writeThroughMode = 0; mode = 0; break; 
                                    
                // todo is set here to zero to achieve some synchronization with screen refresh cycle
                case 'p': ltype = SOLID;    writeThroughMode = 1; mode = 101; showCursor = 0; todo = 0; break;
                case 'q': ltype = DOTTED;   writeThroughMode = 1; mode = 101; showCursor = 0; todo = 0; break;
                case 'r': ltype = DOTDASH;  writeThroughMode = 1; mode = 101; showCursor = 0; todo = 0; break;
                case 's': ltype = SHORTDASH;writeThroughMode = 1; mode = 101; showCursor = 0; todo = 0; break;
                case 't': ltype = LONGDASH; writeThroughMode = 1; mode = 101; showCursor = 0; todo = 0; break;
                case 'u': ltype = SOLID;    writeThroughMode = 1; mode = 101; showCursor = 0; todo = 0; break;
                case 'v': ltype = SOLID;    writeThroughMode = 1; mode = 101; showCursor = 0; todo = 0; break;
                case 'w': ltype = SOLID;    writeThroughMode = 1; mode = 101; showCursor = 0; todo = 0; break; 
                        
                default: 
                        printf("Escape code %02X not implemented, mode = %d\n",ch, savemode);
                        mode = 0;
                        break;                                               
        } 
        return todo;           
}

void checkLimits()
/* check whether char is in visibel space */
{
        /* don't check here for leftmargin, graphics needs to write characters to the whole screen */
        if (x0 < 0) x0 = 0;
        
        if (x0 > windowWidth - hDotsPerChar) {
                x0 = leftmargin; y0 -= vDotsPerChar;
        }
        if (y0 < 4) {
                y0 = windowHeight - vDotsPerChar;
                if (leftmargin) leftmargin = 0;
                else leftmargin = windowWidth / 2;
                /* check here for leftmargin */
                if (x0 < leftmargin) x0 = leftmargin;
        }
        if (y0 > (windowHeight - vDotsPerChar)) y0 = windowHeight - vDotsPerChar;
}

void tek4010_draw(cairo_t *cr, cairo_t *cr2, int first)
// draw onto the main window using cairo
// width is the actual width of the main window
// height is the actual height of the main window
// cr is used for persistent drawing, cr2 for temporary drawing 

{	
        int ch;
        int todo;
        char s[2];
        
        refreshCount++;
        
        int xlast = 0;
        int ylast = 0;
        
        if (first) {
        
                hDotsPerChar  = windowWidth / 74;
                vDotsPerChar  = windowHeight / 35;
                first = 0;
                
                if (windowWidth == 1024) efactor = 0.0;
                else efactor = windowWidth / 1024.0;              
        }
        
        long startPaintTime = mSeconds(); // start to measure time for this draw operation

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
        cairo_set_source_rgb(cr, 0, 0.7, 0);
        
        cairo_select_font_face(cr, "Monospace", CAIRO_FONT_SLANT_NORMAL, CAIRO_FONT_WEIGHT_NORMAL);
        cairo_select_font_face(cr2, "Monospace", CAIRO_FONT_SLANT_NORMAL, CAIRO_FONT_WEIGHT_BOLD);
        
        if (efactor > 0.0) {
                cairo_set_font_size(cr, (int)(efactor * 18));
                cairo_set_font_size(cr2,(int)(efactor * 18));
        }
        else {
                cairo_set_font_size(cr, 18);
                cairo_set_font_size(cr2, 18);
        }
        
        if (plotPointMode) todo = 4 * TODO;
        else if (writeThroughMode) todo = 8 * todo;
        else todo = TODO;
        
        do {
                ch = getInputChar();
                
                if (isInput() == 0) todo = 0;

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
                        
                        if (checkExitFromGraphics(ch)) {
                                todo = todo - 4;
                                if (todo < 0) todo = 0;
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
                                if (ch == 29) mode = 1; // group separator
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
                                if (efactor > 0.0) {
                                        int yb = (xy4014 >> 2) & 3;
                                        y0 = (int)(efactor * (double)(((yh+yl) << 2) + yb) / 4.0);
                                }
                                else y0 = yh + yl;
                                mode++;
                                if (DEBUG) printf("setting yl to %d\n", yl);
                                break;
                        case 3: if (tag == 1) xh = 32 * (ch & 31); mode++;
                                if (DEBUG) printf("setting xh to %d\n", xh);
                                break;
                        case 4: xl = (ch & 31);
                                if (efactor > 0.0) {
                                        int xb = xy4014 & 3;
                                        x0 = (int)(efactor * (double)(((xh+xl) << 2) + xb) / 4.0);
                                }
                                else x0 = xh + xl;
                                mode++;
                                if (DEBUG) printf("setting xl to %d\n", xl);
                                        if (DEBUG) printf("******************************************** Moving to (%d,%d)\n",x0,y0);
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
                                if (efactor > 0.0) {
                                        int xb = xy4014 & 3;
                                        x2 = (int)(efactor * (double)(((xh+xl) << 2) + xb) / 4.0);
                                        int yb = (xy4014 >> 2) & 3;
                                        y2 = (int)(efactor * (double)(((yh+yl) << 2) + yb) / 4.0);
                                }
                                else {
                                        x2 = xh + xl;
                                        y2 = yh + yl;
                                }
                                if (DEBUG) printf(">>>>>xl=%d\n",xl);
                                
                                if (plotPointMode>0.0) {
                                        
                                        // draw the point
                                        cairo_set_source_rgb(cr, 0, 7, 0);
                                        cairo_move_to(cr, x2, windowHeight - y2);
                                        cairo_line_to(cr, x2+1, windowHeight - y2);
                                        cairo_stroke (cr);
                                        
                                        // speed is a problem here
                                        // do not draw adjacent bright spots
                                        
                                        if (((x2 - xlast) > 2) || ((xlast - x2) > 2) ||
                                                ((y2 - ylast) > 2) || ((ylast - y2) > 2))  {
                                        
                                                //draw the bright spot, lower intensity
                                                cairo_set_line_width (cr2, 2);
                                                cairo_set_source_rgb(cr2, 0.1, 0.3, 0.1);                        
                                                cairo_arc(cr2, x2, windowHeight - y2, 6, 0, PI2);
                                                cairo_stroke(cr2);
                                        
                                                //draw the bright spot, medium intensity
                                                cairo_set_source_rgb(cr2, 0.3, 0.6, 0.3);                        
                                                cairo_arc(cr2, x2, windowHeight - y2, 4, 0, PI2);
                                                cairo_stroke(cr2);
                                        
                                                // draw the bright spot, higher intensity
                                                cairo_set_line_width (cr2, 0.1);
                                                cairo_set_source_rgb(cr2, 1, 1, 1);                        
                                                cairo_arc(cr2, x2, windowHeight - y2, 3, 0, PI2);
                                                cairo_fill(cr2);
                                                
                                                xlast = x2;
                                                ylast = y2;
                                        }
                                        
                                        isBrightSpot = 1;
                                        mode = 50;
                                        todo--;
                                }
                                
                                else {
                                        cairo_set_source_rgb(cr, 0, 0.7, 0);
                                        drawVector(cr,cr2);
                                        
                                        todo--;                                        
                                }
                                
                                showCursor = 0;
                                
                                x0 = x2;        // prepare for additional vectors
                                y0 = y2;                                
                                mode = 5;
                                
                                break;
                        case 30: 
                                todo = escapeCodeHandler(cr, cr2, todo, ch);
                                break;               
                        case 40: // incremental plot mode, wait for end mark
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
                                            u100ResetSeconds(1);
                                            usleep(50000);
                                            showCursor=0;
                                            todo = 0;
                                            break;
                                case 8:     // backspace
                                            x0 -= hDotsPerChar;
                                            checkLimits();
                                            break;
                                case 9:     // tab
                                            if (argTab1)
                                                x0 += hDotsPerChar;
                                            else
                                                x0 = x0 - (x0 % (8 * hDotsPerChar)) + 8 * hDotsPerChar;
                                            checkLimits();
                                            break;
                                case 10:    // new line
                                            y0 -= vDotsPerChar;
                                            if (!argRaw) x0 = leftmargin;
                                            checkLimits();
                                            break;
                                case 11:    // VT, move one line up
                                            y0 += vDotsPerChar;
                                            checkLimits();
                                            break;
                                case 13:    // return
                                            mode = 0; x0 = leftmargin;
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
                                                checkLimits();
                                                s[0] = ch;
                                                s[1] = 0;
                                                
                                                if (writeThroughMode) {  // draw the write-through character
                                                        cairo_set_source_rgb(cr2, 0, 0.7, 0);
                                                        cairo_move_to(cr2, x0, windowHeight - y0 + 4);
                                                        cairo_show_text(cr2, s);
                                                }
                                                
                                                else {
                                                        // draw the character
                                                        cairo_set_source_rgb(cr, 0, 0.7, 0);
                                                        cairo_move_to(cr, x0, windowHeight - y0 + 4);
                                                        cairo_show_text(cr, s);
                                                
                                                        // draw the bright spot
                                                        cairo_set_source_rgb(cr2, 1, 1, 1);
                                                        cairo_move_to(cr2, x0, windowHeight - y0 + 4);
                                                        cairo_show_text(cr2, s);                                                
                                                }
                                                
                                                x0 += hDotsPerChar;
                                                isBrightSpot = 1;
                                                todo--;
                                            }
                                            break;
                                }
                                break;                                
                }
                endDo:;
        }
        while (todo && ((mSeconds() - startPaintTime) < REFRESH_INTERVAL));
        
        // display cursor
        
        if (showCursor) doCursor(cr2);
}
