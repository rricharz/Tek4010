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
#define BLACK_COLOR             0.08            // effect of flood gun

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
#include <locale.h>
#include <pwd.h>
// Cygwin defines FIONREAD in <sys/socket.h>. #26
#ifndef FIONREAD
#include <sys/socket.h>
#endif

#include "main.h"
#include "tube.h"

extern void gtk_main_quit();
extern int windowWidth;
extern int windowHeight;
extern char *windowName;


// variables for APL keycode translator
#define MAXKEYCODES 256
struct keyCode {
        int inputCode;
        int outputCode;
};

struct keyCode keyTable[MAXKEYCODES];

int argNoexit = 0;              // options
int argRaw = 0;
int argBaud = 19200;
int argTab1 = 0;
int argFull = 0;
int argFullV = 0;
int argARDS = 0;
int argAPL = 0;
int argAutoClear = 0;
int argKeepSize = 0;
int argHideCursor = 0;

int refresh_interval;           // after this time in msec next refresh is done

int showCursor;                 // set of cursor is shown (not set in graphics mode)
int isBrightSpot = 0;           // set if there is currently a bright spot on the screen

int xlast, ylast;

enum LineType ltype;
double dashset[] = {2,6,2,2,6,3,3,3,6,6};

int plotPointMode = 0;           // plot point mode
int writeThroughMode = 0;        // write through mode
int isGinMode = 0;               // set if GIN mode is active
int isGinSuppress = 0;           // suppressing characters after GIN.
int tube_doClearPersistent;

int specialPlotMode = 0;
int defocussed = 0;
int intensity = 100;
int aplMode = 0;

int tube_x0, tube_x2,tube_y0, tube_y2;

double pensize = 1.0;

static int debugCount = 0;

long refreshCount = 0;           // variables for baud rate and refresh rate measurements

static long charCount = 0;
static long charResetCount = 0;
static long characterInterval = 0;
static int currentFontSize = 18;
static int currentCharacterOffset = 0;

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
        if (isGinMode)
                // delay any input from host during GIN mode
                // this allows to test .plt files with GIN mode
                return 0;
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

void checkFont(char *fontName)
// check whether font has been installed
{
#define MAXL 255
        FILE *f;
        int i, ch;
        char line[MAXL];
        sprintf(line,"fc-list \'%s\'", fontName); // prepare system call
        f = popen(line, "r");
        if (f != NULL) {
                i = 0;
                line[0] = 0;
                while (((ch=fgetc(f))!=EOF) && (i<MAXL-1)) {
                        line[i++] = ch;
                }
                line[i-1]=0;
                // printf("%s\n",line);
                if (strstr(line, fontName) > 0) {
                        pclose(f);
                        return;
                }
       }
       pclose(f);
       printf("Error: APL font \'%s\' not installed. This font is required for the APL mode.\n", fontName);
       printf("See github.com/rricharz/tek4010 paragraph \'APL mode\'\n");
       exit(1);
}

void readKeyTranslationTable()
// read keyboard translation table from ~/.tekaplkeys
{
        FILE *confFile;
        int code1, code2, i;
        char *homedir = getpwuid(getuid())->pw_dir;	
	char s[255];
        
        memset(keyTable, 0, sizeof(keyTable));
        
        strcpy(s, homedir);
        strcat(s, "/.tek4010conf/aplkeys" );
        // printf("Looking for conf file %s\n",s);
        confFile = fopen(s,"r");
        if (confFile) {
                // printf("confFile open\n");
                i = 0;
                while (!feof(confFile)) {
                        if (fscanf(confFile, "%d %d\n", &code1, & code2) == 2) {
                                // printf("%d %d\n", code1, code2);
                                keyTable[i].inputCode = code1;
                                keyTable[i].outputCode = code2;
                                i++;
                                if (i >= MAXKEYCODES) {
                                        printf("Error: APL key code table too large, max %d entries\n",MAXKEYCODES);
                                        fclose(confFile);
                                        exit(1);
                                }
                        }
                        else 
                                fscanf(confFile,"%s\n",s); // skip comment line
                }
                fclose(confFile);
        }
}

int tube_translateKeyCode(int ch)
{
        int i = 0;
        // printf("TranslateKeyCode %d ", ch);
        while ((i < MAXKEYCODES) && (keyTable[i].inputCode != 0)) {
                if (keyTable[i].inputCode == ch) {
                        // printf("%d\n", keyTable[i].outputCode);
                        return keyTable[i].outputCode;
                }
                i++;
        }
        printf("\n");
        return ch;
}

void tube_init(int argc, char* argv[])
// put any code here to initialize the tek4010
{
        char *argv2[20];
        size_t bufsize = 127;
        int firstArg = 1;
        printf("tek4010 version 1.6\n");
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
                else if (strcmp(argv[firstArg],"-b100000") == 0)
                        argBaud = 100000;
                else if (strcmp(argv[firstArg],"-b38400") == 0)
                        argBaud = 38400;
                else if (strcmp(argv[firstArg],"-b19200") == 0)
                        argBaud = 19200;
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
                else if (strcmp(argv[firstArg],"-fullv") == 0)
                        argFullV = 1;
                else if (strcmp(argv[firstArg],"-autoClear") == 0)
                        argAutoClear = 1;
                else if (strcmp(argv[firstArg],"-keepsize") == 0)
                        argKeepSize = 1;
                else if (strcmp(argv[firstArg],"-hidecursor") == 0)
                        argHideCursor = 1;
                else if (strcmp(argv[firstArg],"-APL") == 0) {
                        argAPL = 1;
                        windowName = "Tektronix 4013/4015 emulator (APL)";
                        checkFont(APL_FONT);
                        readKeyTranslationTable();
                }
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
                // printf("Execution time: %0.3f sec\n", (double)t/1000.0);
                // if (t > 0) {
                //         printf("Average screen refresh rate: %0.1f Hz\n",(double)(1000.0*refreshCount)/t);
                //         printf("Average character rate: %0.0f baud\n",(double)(8000.0*charCount)/t);
                // }
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
        if (argARDS) return 0;
        if (button == 1) {
                tek4010_clicked(x, windowHeight - y);
                return 1;
        }
	return 0;
}

void tube_quit()
// is called if the main window is quit bevore the tek4010 exits
// put any code here which needs to be called on exit
{
        pclose(getData);
        system("pkill rs232-console");
}

void tube_doCursor(cairo_t *cr2)
{
        cairo_set_source_rgb(cr2, 0, CURSOR_INTENSITY, 0);
        cairo_set_line_width (cr2, 1);
        cairo_rectangle(cr2, tube_x0, windowHeight - tube_y0 - vDotsPerChar + 6 + currentCharacterOffset,
                                                hDotsPerChar - 3, vDotsPerChar - 3);
        cairo_fill(cr2);
        cairo_stroke (cr2);
}

void tube_clearPersistent(cairo_t *cr, cairo_t *cr2)
// clear the persistant surface
// flash using the second surface
{
        cairo_set_source_rgb(cr, 0.0, BLACK_COLOR, 0.0);
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
        char s[8];
        
        if (ch < 32) return; // non printable control character
        
        if (aplMode) {
                switch (ch) {
                        case 32: sprintf(s," ");
                                 break;
                        case 33: sprintf(s,"\u00A8");
                                 break;
                        case 34: sprintf(s,")");
                                 break;
                        case 35: sprintf(s,"<");
                                 break;
                        case 36: sprintf(s,"\u2264");
                                 break;
                        case 37: sprintf(s,"=");
                                 break;
                        case 38: sprintf(s,">");
                                 break;
                        case 39: sprintf(s,"]");
                                 break;
                        case 40: sprintf(s,"\u2228");           
                                 break;
                        case 41: sprintf(s,"\u2227");
                                 break;
                        case 42: sprintf(s,"\u2260");
                                 break;
                        case 43: sprintf(s,"\u00F7");
                                 break;
                        case 44: sprintf(s,",");
                                 break;
                        case 45: sprintf(s,"+");
                                 break;
                        case 46: sprintf(s,".");
                                 break;
                        case 47: sprintf(s,"/");
                                 break;
                                 
                        // 48 - 57: Digits
                                 
                        case 58: sprintf(s,"(");
                                 break;
                        case 59: sprintf(s,"[");
                                 break;
                        case 60: sprintf(s,";");
                                 break;
                        case 61: sprintf(s,"\u00D7");
                                 break;
                        case 62: sprintf(s,":");
                                 break;
                        case 63: sprintf(s,"\\");
                                 break;                                 
                        case 64: sprintf(s,"\u00AF");
                                 break;
                        case 65: sprintf(s,"\u237A");
                                 break;
                        case 66: sprintf(s,"\u22A5");
                                 break;
                        case 67: sprintf(s,"\u2229");
                                 break;
                        case 68: sprintf(s,"\u230A");
                                 break;
                        case 69: sprintf(s,"\u220A");
                                 break;
                        case 70: sprintf(s,"_");
                                 break;
                        case 71: sprintf(s,"\u2207");
                                 break;
                        case 72: sprintf(s,"\u2206");
                                 break;
                        case 73: sprintf(s,"\u2373");
                                 break;
                        case 74: sprintf(s,"\u2218");
                                 break;
                        case 75: sprintf(s,"'");
                                 break;
                        case 76: sprintf(s,"\u2395");
                                 break;
                        case 77: sprintf(s,"\u2223");
                                 break;
                        case 78: sprintf(s,"\u22A4");
                                 break;
                        case 79: sprintf(s,"\u25CB");
                                 break;
                        
                        case 80: sprintf(s,"\u22c6");
                                 break;
                        case 81: sprintf(s,"?");
                                 break;
                        case 82: sprintf(s,"\u2374");
                                 break;
                        case 83: sprintf(s,"\u2308");
                                 break;
                        case 84: sprintf(s,"\u223C");
                                 break;
                        case 85: sprintf(s,"\u2193");
                                 break;
                        case 86: sprintf(s,"\u222A");
                                 break;
                        case 87: sprintf(s,"\u03C9");
                                 break;
                        case 88: sprintf(s,"\u2283");
                                 break;
                        case 89: sprintf(s,"\u2191");
                                 break;
                        case 90: sprintf(s,"\u2282");
                                 break;
                        case 91: sprintf(s,"\u2190");
                                 break;
                        case 92: sprintf(s,"\u22A2");
                                 break;
                        case 93: sprintf(s,"\u2192");
                                 break;
                        case 94: sprintf(s,"\u2265");
                                 break;
                        case 95: sprintf(s,"-");
                                 break;
                        case 96: sprintf(s,"\u22C4");
                                 break;
                                 
                        // 97 - 122 capital letters
                                 
                        case 123: sprintf(s,"{");
                                 break;
                        case 124: sprintf(s,"\u22A3");
                                 break;
                        case 125: sprintf(s,"}");
                                 break;
                        case 126: sprintf(s,"$");
                                 break;
                                 
                        default: if ((ch>=48) && (ch<=57)) sprintf(s,"%c", ch); // digits
                                 else if ((ch>=97) && (ch<=122)) sprintf(s,"%c", ch - 32); // capital letters
                                 else sprintf(s," ");
                                 break;
                } 
        }
        else {
                s[0] = ch;
                s[1] = 0;
        }
        cairo_set_font_size(cr, currentFontSize);
        cairo_set_font_size(cr2,currentFontSize); 

        if (writeThroughMode) {  // draw the write-through character
                cairo_set_source_rgb(cr2, 0, WRITE_TROUGH_INTENSITY, 0);
                cairo_move_to(cr2, tube_x0, windowHeight - tube_y0 + currentCharacterOffset);
                cairo_show_text(cr2, s);
        }
                                                
        else {
                // draw the character
                cairo_set_source_rgb(cr, 0, BLACK_COLOR + ((NORMAL_INTENSITY - BLACK_COLOR) * intensity) / 100, 0);
                cairo_move_to(cr, tube_x0, windowHeight - tube_y0 + currentCharacterOffset);
                cairo_show_text(cr, s);
                                                
                // draw the bright spot
                cairo_set_source_rgb(cr2, BRIGHT_SPOT_COLOR, BRIGHT_SPOT_COLOR, BRIGHT_SPOT_COLOR);
                cairo_move_to(cr2, tube_x0, windowHeight - tube_y0 + currentCharacterOffset);
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
        int i1;
        cairo_set_line_width (cr, pensize + defocussed);
        cairo_set_source_rgb(cr, 0, BLACK_COLOR + ((1.0 - BLACK_COLOR) * intensity) / 100, 0);
        cairo_move_to(cr, tube_x2, windowHeight - tube_y2);
        cairo_line_to(cr, tube_x2 + 1, windowHeight - tube_y2 + 1);
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

void tube_crosshair(cairo_t *cr, cairo_t *cr2)
{
        // printf("crosshair at %d,%d\n", tube_x0, tube_y0);
        cairo_set_line_width (cr2, 1);
        cairo_set_source_rgb(cr2, 0.0, WRITE_TROUGH_INTENSITY, 0.0);
        cairo_move_to(cr2, tube_x0, 0);
        cairo_line_to(cr2, tube_x0, windowHeight);
        cairo_move_to(cr2, 0, windowHeight - tube_y0);
        cairo_line_to(cr2, windowWidth, windowHeight - tube_y0);
        cairo_stroke (cr2);        
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
                cairo_set_line_width (cr2, pensize + 1);
                cairo_set_source_rgb(cr2, 0.0, WRITE_TROUGH_INTENSITY, 0.0);
                cairo_move_to(cr2, tube_x0, windowHeight - tube_y0);
                cairo_line_to(cr2, tube_x2, windowHeight - tube_y2);
                cairo_stroke (cr2);
        }
        
        else {
                // draw the actual vector on permanent surface
                cairo_set_line_width (cr, pensize + defocussed);
                cairo_set_source_rgb(cr, 0, BLACK_COLOR + ((NORMAL_INTENSITY - BLACK_COLOR) * intensity) / 100, 0);
                tube_line_type(cr, cr2, ltype);
                cairo_move_to(cr, tube_x0, windowHeight - tube_y0);
                cairo_line_to(cr, tube_x2, windowHeight - tube_y2);
                cairo_stroke (cr);
        
                //draw the bright spot, half intensity
                cairo_set_line_width (cr2, 6 + pensize + 1 * defocussed);
                double bsc = (BRIGHT_SPOT_COLOR_HALF * intensity) / 100;
                cairo_set_source_rgb(cr2, bsc, bsc, bsc);                        
                cairo_move_to(cr2, tube_x0, windowHeight - tube_y0);
                cairo_line_to(cr2, tube_x2, windowHeight - tube_y2);
                cairo_stroke (cr2);
                                        
                // draw the bright spot, high intensity
                cairo_set_line_width (cr2, pensize + 2 + 2 * defocussed);
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
	cairo_set_line_width (cr, pensize);
        cairo_set_source_rgb(cr, 0, NORMAL_INTENSITY, 0);        
        cairo_select_font_face(cr, fontName, CAIRO_FONT_SLANT_NORMAL, CAIRO_FONT_WEIGHT_NORMAL);
        cairo_select_font_face(cr2, fontName, CAIRO_FONT_SLANT_NORMAL, CAIRO_FONT_WEIGHT_BOLD);        
}

void tube_changeCharacterSize(cairo_t *cr, cairo_t *cr2,int charsPerLine, int charsPerPage, double fontSize)
{
        cairo_font_extents_t et;
        hDotsPerChar = windowWidth / charsPerLine;
        vDotsPerChar = windowHeight / charsPerPage;
        leftmargin = 0;
        if (argARDS) {
                currentFontSize = (int) (fontSize * APL_FONT_SIZE);
        }
        else {
                currentFontSize = (int) (fontSize * STANDARD_FONT_SIZE);
        }
        cairo_set_font_size(cr, currentFontSize);
        cairo_set_font_size(cr2,currentFontSize);    
        if (argARDS) {
               cairo_font_extents(cr, &et);
               currentCharacterOffset =(int)et.ascent;
               if (DEBUG) printf("Set vertical character offset for ARDS mode to %d\n", currentCharacterOffset);
        }
        else
                currentCharacterOffset = 0;
}
