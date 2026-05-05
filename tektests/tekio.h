/*  tecio.h 2.11 BSD Tektronix graphics 	      */
/*  io functions for Tektronix 4010             */
/*  cc 2019 rricharz                            */
/*  If you want to use this, you need to	      */
/*  be in tek4010                             	*/
/*  See https://github.com/rricharz/Tek4010     */

#define MAXX 1023	/* Tektronix graphics screen size */
#define MAXY  780

#define MAXCOLUMNS  74  /* Tektronix alpha screen size */
#define MAXLINES    35 

/*                                              */
/*  clear screen with                           */
/*     clearScreen()                            */                           
/*                                              */
/*  draw individual vectors with                */
/*     drawVector(x1,y1,x2,y2)                  */
/*                                              */
/*  draw a string using graphics coordinates    */
/*     moveTo(x1,y1)                            */
/*     printf(.....)                            */
/*                                              */
/*  draw a string using alpha coordinates       */
/*     moveAlpha(line,column)                   */
/*     printf(.....)                            */
/*                                              */ 
/*  fast draw multiple joint vectors            */
/*     startDraw(x1,y1)                         */
/*     draw(x2,y2)                              */
/*     draw(x3,y3)                              */
/*     .............                            */
/*     endDraw()                                */
/*                                              */
/*   drawRectange(x1,y1.x2.y2)                  */
/*                                              */
/*  drawCircle(x,y,r)                           */
/*                                              */
/* drawJustifiedText(s, line, justify)          */
/* justify 0=left, 1=center; 2=right            */
/*                                              */
/* writethrough mode for short animations       */
/*     startWriteThrough()                      */
/*     endWriteThrough()                        */
/*                                              */
/* setCharacterSize(size)                       */
/*     select character size 1..4               */
/*                                              */
/* void setLineMode(type)                       */
/*     set line type				*/
/*   SOLID,DOTTED,DOTDASH,SHORTDASH,LONGDASH    */

#define PI  3.14159265
#define PI2 6.28318531

#define SOLID		1
#define DOTTED  	2
#define DOTDASH 	3
#define SHORTDASH	4
#define LONGDASH	5 

extern void drawVector(int x1, int y1, int x2, int y2);
extern void moveTo(int lx, int ly);
extern void moveAlpha(int line, int column);
extern void clearScreen(void);
extern void startDraw(int lx, int ly);
extern void draw(int x2, int y2);
extern void endDraw(void);
extern void startWriteThrough();
extern void endWriteThrough();
extern void drawRectangle(int x1, int y1, int x2, int y2);
extern void drawCircle(int x, int y, int r);
extern void drawJustifiedText(char *s, int line, int justify);
extern int fix(double r);
extern void setCharacterSize(int size);
extern void setLineMode(int type);
