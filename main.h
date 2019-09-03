
#include <cairo.h>

#define BORDER 80       // we need to make an educated guess to make sure that the full vertical
                        // space is used if required
                        // if BORDER is too small, we might end up with a too large window
                        // if BORDER is too large, the decorated window will be smaller than possible
                        // with a reasonable size BORDER, both are acceptable
                        // ideally, one could force the window manager to use a certain aspect ratio

void tube_init(int argc, char* argv[]);
void tek4010_draw(cairo_t *cr, cairo_t *cr2, int first);
void tek4010_clicked(int x, int y);
int  tube_on_timer_event();
int  tube_clicked(int button, int x, int y);
void tube_quit();

void ards_draw(cairo_t *cr, cairo_t *cr2, int first);


