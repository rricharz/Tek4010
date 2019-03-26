/*
 * tek4010.c
 * 
 * A tek 4010 graphics emulator
 * 
 * Copyright 2016,2019  rricharz
 * 
 */

#define MEM 128

#define TODO 6          // for speed reasons, draw multiple objects until screen updates

#include <stdio.h>
#include <stdlib.h>
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

/* not yet used, for dsrk mode
int memx1[MEM], memy1[MEM], memx2[MEM], memy2[MEM];
float memv[MEM];
*/

int count = 0;
static int mode;
static int x0,y0,x2,y2;

int leftmargin;

int hDotsPerChar;
int vDotsPerChar;

FILE *getData;
int getDataPipe[2];

FILE *putKeys;
int putKeysPipe[2];

int getk()
// get a char, if available, otherwise return -1
// When called for the first time, a child process for rsh is forked
// and communication between parent and child are established
{
        int bytesWaiting;
        ioctl(getDataPipe[0], FIONREAD, &bytesWaiting);
        if (bytesWaiting > 0)
                return getc(getData);
        else
                return -1;
}

void tek4010_init(int argc, char* argv[])
// put any code here to initialize the tek4010
{
        char *argv2[10];
        if ((argc<2) || (argc>9)) {
                printf("Error:number of arguments\n");
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
                exit(1);
        }
                
        // parent process
                
        close(getDataPipe[1]); // not used
        close(putKeysPipe[0]); // not used
                
        // use termios to turn off line buffering for both pipes
        struct termios term;
        tcgetattr(getDataPipe[0], &term);
        term.c_lflag &= ~ICANON ;
        tcsetattr(getDataPipe[0], TCSANOW,&term);
        tcgetattr(putKeysPipe[1], &term);
        tcsetattr(putKeysPipe[0], TCSANOW,&term);
                
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
}

int tek4010_on_timer_event()
// if TIMER_INTERVAL in tek4010.h is larger than zero, this function
// is called every TIMER-INTERVAL milliseconds
// if the function returns 1, the window is redrawn by calling applicatin_draw
{
	return 1;
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



void tek4010_draw(cairo_t *cr, cairo_t *cr2, int width, int height, int first)
// draw onto the main window using cairo
// width is the actual width of the main window
// height is the actual height of the main window
// surface1 is used for persistent drawing, surface2 for faiding drawing 

{		
        int ch;
        int todo;
        char s[2];
        int showCursor = 1;
        
/*        if (first) {
                for (int i=0; i<MEM; i++) {
                        memx1[i]=2;
                        memy1[i]=i * (WINDOW_HEIGHT/MEM); 
                        memx2[i]=100;
                        memy2[i]=i * (WINDOW_HEIGHT/MEM);
                        memv[i]=0;
                }
        } */
        if (globalClearPersistent) {
                cairo_set_source_rgb(cr, 0, 0, 0);
                cairo_paint(cr);
                globalClearPersistent = 0;
                x0 = 0;
                y0 = WINDOW_HEIGHT - vDotsPerChar;
                leftmargin = 0;
        }
        cairo_set_source_rgba(cr2, 0, 0, 0, 0); // second surface is cleared each time
        cairo_set_operator(cr2, CAIRO_OPERATOR_SOURCE);
        cairo_paint(cr2);
        cairo_set_operator(cr2, CAIRO_OPERATOR_OVER);
        cairo_set_line_width (cr2, 3);
        cairo_set_source_rgb(cr2, 1, 1, 1);
        
        cairo_set_antialias(cr, CAIRO_ANTIALIAS_BEST);      
	cairo_set_line_width (cr, 1);
        cairo_set_source_rgb(cr, 0, 0.8, 0);
        
        cairo_select_font_face(cr, "Monospace", CAIRO_FONT_SLANT_NORMAL, CAIRO_FONT_WEIGHT_NORMAL);
        cairo_set_font_size(cr, 18);
        cairo_select_font_face(cr2, "Monospace", CAIRO_FONT_SLANT_NORMAL, CAIRO_FONT_WEIGHT_BOLD);
        cairo_set_font_size(cr2, 18);
        
        todo = TODO;
        
        do {
                ch = getk();
                if (ch == 31) {               // exit from graphics mode
                        mode = 0;
                        ch = getk();
                }
                if (ch == -1) todo--;         // no char available, need to allow for updates
                
                if ((mode>=1) && (mode <=9) &&
                                        ((ch==31) || (ch==13))) {
                        mode = 0;  // leave graphics mode
                        showCursor = 0;
                        ch = -1;
                }
                
                switch (mode) {
                        case 1: y0 = 32 * (ch - 32); mode++; break;
                        case 2: y0 = y0 + ch - 96; mode++; break;
                        case 3: x0 =  32 * (ch - 32); mode++; break;
                        case 4: x0 = x0 + ch - 64; mode++; break;
                        case 5: y2 = 32 * (ch - 32); mode++; break;
                        case 6: y2 = y2 + ch - 96; mode++; break;
                        case 7: x2 =  32 * (ch - 32); mode++; break;
                        case 8: x2 = x2 + ch - 64;
                                cairo_move_to(cr, x0, WINDOW_HEIGHT - y0);
                                cairo_line_to(cr, x2, WINDOW_HEIGHT - y2);
                                cairo_stroke (cr);                        
                                cairo_move_to(cr2, x0, WINDOW_HEIGHT - y0);
                                cairo_line_to(cr2, x2, WINDOW_HEIGHT - y2);
                                cairo_stroke (cr2);
                                showCursor = 0;
                                
                                // for speed reasons, do not update screen right away
                                // if many very small verctors are drawn
                                todo--;
                                if ((x2-x0) > TODO) todo = 0;
                                if ((x0-x2) > TODO) todo = 0;
                                if ((y2-y0) > TODO) todo = 0;
                                if ((y0-y2) > TODO) todo = 0;
                                
                                x0 = x2;        // prepare to additional vectors
                                y0 = y2;                                
                                mode = 5;
                                break;
                        case 10:                        // handle escape modes
                                switch (ch) {
                                    case 12: cairo_set_source_rgb(cr, 0, 0.0, 0); // clear screen
                                         cairo_paint(cr);
                                         cairo_set_source_rgb(cr, 0, 0.8, 0);
                                         x0 = 0;
                                         y0 = WINDOW_HEIGHT - vDotsPerChar;
                                         mode = 0;
                                         break;
                                    case '[': // a second escape code follows, do not reset mode
                                         break;
                                    default: // printf("Escape code %d\n",ch);
                                         mode = 0;
                                         break;                                               
                                }
                                break;
                        default:// if (ch != -1) printf("ch code %d\n",ch);
                                switch (ch) {
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
                                            mode = 0; todo = 0; x0 = leftmargin;
                                            break;
                                case 27:    // escape
                                            mode = 10;
                                            break;
                                case 29:    // group separator
                                            mode = 1;
                                            break;
                                case 31:    // US, leave graphics mode
                                            mode = 0;
                                            break;
                                default:
                                            if ((ch >= 32) && (ch <=127)) { // printable character
                                                if (y0 < 8) y0 = 8;
                                                s[0] = ch;
                                                s[1] = 0;
                                                cairo_move_to(cr, x0, WINDOW_HEIGHT - y0 + 4);
                                                cairo_show_text(cr, s);
                                                cairo_move_to(cr2, x0, WINDOW_HEIGHT - y0 + 4);
                                                cairo_show_text(cr2, s);
                                                x0 += hDotsPerChar;
                                                todo--;
                                            }
                                            break;
                                }
                                break;                                
                }
        }
        while (todo);
        
        // display cursor
        
        if (showCursor) {
        
                cairo_set_source_rgb(cr2, 0, 0.8, 0);
                cairo_set_line_width (cr, 1);
                cairo_rectangle(cr2, x0, WINDOW_HEIGHT - y0 - vDotsPerChar + 8,
                                                hDotsPerChar - 3, vDotsPerChar - 3);
                cairo_fill(cr2);
                cairo_stroke (cr2);
        }
        
        // is child process still running?
        
        int status;
        if (waitpid(-1, &status, WNOHANG)) {    // Is child process terminated?
                tek4010_quit();
                gtk_main_quit();
                exit(0);
        }
}