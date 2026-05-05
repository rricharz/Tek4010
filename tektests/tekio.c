/*  tecio.c 2.11 BSD Tektronix graphics 	      */
/*  io functions for Tektronix 4010             */
/*  cc 2019 rricharz                            */
/*  If you want to use this, you need to	      */
/*  be in tek4010 terminal.                    	*/
/*  See https://github.com/rricharz/Tek4010     */

#include <stdio.h>
#include <math.h>
#include <string.h>
#include "tekio.h"

static int xs,ys;

void startDraw(int x1, int y1)
{
    if (x1 < 0) x1 = 0;
    if (x1 >= MAXX) x1 = MAXX -1;
    if (y1 < 0) y1 = 0;
    if (y1 >= MAXY) y1 = MAXY - 1;
    putchar(29);
    putchar((y1 >> 5) + 32);
    putchar((y1 & 31) + 96);
    putchar((x1 >> 5) + 32);
    putchar((x1 & 31) + 64);
    xs = x1;
    ys = y1;
}

void draw(int x2, int y2)
{
    int hxchange, lychange;

    if (x2 < 0) x2 = 0;
    if (x2 >= MAXX) x2 = MAXX -1;
    if (y2 < 0) y2 = 0;
    if (y2 >= MAXY) y2 = MAXY - 1;

    if ((y2 >> 5) != (ys >> 5))        /* if high y has changed */
	putchar((y2 >> 5) + 32);
    hxchange = (x2 >> 5) != (xs >> 5);
    lychange = (y2 & 31) != (ys & 31);
    if (hxchange || lychange) putchar((y2 & 31) + 96);
    if (hxchange)                                     /* if high order x has changed */
    	putchar((x2 >> 5) + 32);
    putchar((x2 & 31) + 64);
    xs = x2;
    ys = y2;
}

void endDraw()
{
    putchar(31);
    fflush(stdout);
}

void moveTo(int x1, int y1)
{
    startDraw(x1,y1);
    endDraw();
}

void moveAlpha(int line, int column)
{
    int vDotsPerChar;
    double hDotsPerChar;
    if (line < 1) line = 1;
    if (line > MAXLINES) line = MAXLINES;
    if (column < 1) column = 1;
    if (column > MAXCOLUMNS) column = MAXCOLUMNS;
    vDotsPerChar = MAXY / MAXLINES;
    hDotsPerChar = (double)(MAXX)/(double)(MAXCOLUMNS);
    moveTo((int)((double)(column - 1) * hDotsPerChar), MAXY - line * vDotsPerChar);
}

void drawVector(int x1, int y1, int x2, int y2)
{
    startDraw(x1,y1);
    if (x2 < 0) x2 = 0;
    if (x2 >= MAXX) x2 = MAXX -1;
    if (y2 < 0) y2 = 0;
    if (y2 >= MAXY) y2 = MAXY - 1;
    draw(x2,y2);
    endDraw();
}

void clearScreen()
{
    putchar(27);
    putchar(12);
}


void startWriteThrough()
{
    putchar(27);
    putchar('p');
    fflush(stdout);	
}

void endWriteThrough()
{
    putchar(27);
    putchar('`');
    fflush(stdout);
}

void drawRectangle(int x1, int y1, int x2, int y2)
{
    startDraw(x1,y1);
    draw(x2,y1);
    draw(x2,y2);
    draw(x1,y2);
    draw(x1,y1);
    endDraw();
}

void drawCircle(int x, int y, int r)
{
	int i;
	double arg;
    	startDraw(x + r, y);
    	for (i = 0; i <= r; i++) {
        	arg = (double)(i) * PI2 / (double) r;
		      draw(x + (int)((double)r * cos(arg)), y + (int)((double)r * sin(arg)));
    	}
    	endDraw();
}

void drawJustifiedText(char *s, int line, int justify)
/* justify 0=left, 1=center; 2=right */
{
	int length;
	length = strlen(s);
	switch (justify) {
		case 1: moveAlpha(line, 1); break;
                case 2: moveAlpha(line, (MAXCOLUMNS - length) / 2); break;
                case 3: moveAlpha(line, MAXCOLUMNS - length + 1); break;
	}
	printf("%s",s);
}

int fix(double r)
/* round double and convert to int */
{
 	if (r >= 0) return (int)(r + 0.5);
	else return (int)(r - 0.5);
}

void setCharacterSize(int size)
{
	if ((size >= 1) && (size <= 4)) {
		putchar(27);
		putchar('7' + size);
	}
	else
		printf("setCharacterSize: Illegal size %d\n", size);
}

void setLineMode(int type)
{
	if ((type >= SOLID) && (type <= LONGDASH)) {
		putchar(27);
		putchar(95 + type);
	}
	else
		printf("SetLineMode: illegal type %d\n", type);
}


