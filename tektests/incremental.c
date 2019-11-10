#define MAXX 1024
#define MAXY 780

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

void endDraw()
{
    putchar(31);
    fflush(stdout);
}

int main (int argc, char *argv[])
{
    int i;
    startDraw(512,140);
    putchar(30);
    putchar(80);
    for (i =0; i<200; i++) {
        putchar(69);
    }
    for (i =0; i<200; i++) {
        putchar(70);
    }
    for (i =0; i<200; i++) {
        putchar(74);
    }
        for (i =0; i<200; i++) {
        putchar(73);
    }
    endDraw();
}
