// tube.h

enum LineType {SOLID,DOTTED,DOTDASH,SHORTDASH,LONGDASH};
extern enum LineType ltype;

extern int globaltube_clearPersistent;
extern int windowWidth;
extern int windowHeight;

extern int argFull;
extern int argTab1;
extern int argRaw;

extern int hDotsPerChar;
extern int vDotsPerChar;

extern int refresh_interval;           // after this time in msec next refresh is done
extern long refreshCount;

extern int showCursor;                 // set of cursor is shown (not set in graphics mode)
extern int isBrightSpot;               // set if there is currently a bright spot on the screen

extern int plotPointMode;
int writeThroughMode;

extern double efactor;
extern int eoffx;

extern int tube_x0, tube_x2, tube_y0, tube_y2;

extern long tube_mSeconds();
extern long tube_u100ResetSeconds(int reset);
extern void tube_doCursor(cairo_t *cr2);
extern void tube_clearPersistent(cairo_t *cr, cairo_t *cr2);
extern void tube_clearSecond(cairo_t *cr2);
extern void tube_clearPersistent(cairo_t *cr, cairo_t *cr2);
extern int  tube_isInput();
extern int  tube_getInputChar();
extern void tube_emulateDeflectionTime();
void        tube_drawVector(cairo_t *cr, cairo_t *cr2);
void        tube_drawCharacter(cairo_t *cr, cairo_t *cr2, char ch);
void        tube_drawPoint(cairo_t *cr, cairo_t *cr2);
void        tube_setupPainting(cairo_t *cr, cairo_t *cr2, char *fontName, int fontSize);
