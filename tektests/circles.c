#define MAXX 1024
#define MAXY 680

#include <stdio.h>
#include <math.h>

int xs,ys;

void startDraw(int x1,int y1)
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

void draw(int x2,int y2)
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

int drawCircle(int x,int y,int r)
{
	int i;
	double arg;
    	startDraw(x + r, y);
    	for (i = 0; i <= r; i++) {
        	arg = (double)(i) * 6.283185307 / (double) r;
		draw(x + (int)((double)r * cos(arg)), y + (int)((double)r * sin(arg)));
    	}
    	endDraw();
}

int main (int argc, char *argv[])
{
	drawCircle(512, 340, 200);
        drawCircle(612, 340, 100);
        drawCircle(412, 340, 100);
        drawCircle(512, 240, 100);
	drawCircle(512, 440, 100);
	startDraw(212,340); draw(812,340); endDraw();
	startDraw(512,040); draw(512,640); endDraw();

	startDraw(1,1); endDraw(); // move the cursor out of the way
}
