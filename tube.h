// tube.h

// fonts
#define STANDARD_FONT "Monospace"
#define STANDARD_FONT_SIZE 18.0
#define APL_FONT "APL385 Unicode"
#define APL_FONT_SIZE 20.0

enum LineType {SOLID,DOTTED,DOTDASH,SHORTDASH,LONGDASH};
extern enum LineType ltype;

extern int tube_doClearPersistent;
extern int windowWidth;
extern int windowHeight;

extern int argFull;
extern int argTab1;
extern int argRaw;
extern int argAPL;
extern int argAutoClear;
extern int argKeepSize;
extern int argHideCursor;

extern int hDotsPerChar;
extern int vDotsPerChar;

extern int refresh_interval;           // after this time in msec next refresh is done
extern long refreshCount;

extern int showCursor;                 // set of cursor is shown (not set in graphics mode)
extern int isBrightSpot;               // set if there is currently a bright spot on the screen
extern int isGinMode;                  // set if GIN mode is active
extern int isGinSuppress;              // set if suppressing echoed chars in/after GIN.

extern int specialPlotMode;
extern int defocussed;
extern int intensity;
extern int aplMode;

extern int plotPointMode;
extern int writeThroughMode;

extern int tube_x0, tube_x2, tube_y0, tube_y2;

extern double pensize;

extern long tube_mSeconds();
extern long tube_u100ResetSeconds(int reset);
extern int tube_translateKeyCode(int ch);
extern void tube_doCursor(cairo_t *cr2);
extern void tube_clearPersistent(cairo_t *cr, cairo_t *cr2);
extern void tube_clearSecond(cairo_t *cr2);
extern int  tube_isInput();
extern int  tube_getInputChar();
extern void tube_emulateDeflectionTime();
extern void tube_crosshair(cairo_t *cr, cairo_t *cr2);
extern void tube_drawVector(cairo_t *cr, cairo_t *cr2);
extern void tube_drawCharacter(cairo_t *cr, cairo_t *cr2, char ch);
extern void tube_drawPoint(cairo_t *cr, cairo_t *cr2);
extern void tube_setupPainting(cairo_t *cr, cairo_t *cr2, char *fontName);
extern void tube_changeCharacterSize(cairo_t *cr, cairo_t *cr2, int charsPerLine, int charsPerPage, double fontSize);
