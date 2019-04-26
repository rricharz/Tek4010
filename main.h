
#include <cairo.h>

void tube_init(int argc, char* argv[]);
void tek4010_draw(cairo_t *cr, cairo_t *cr2, int first);
void tek4010_clicked(int x, int y);
int  tube_on_timer_event();
int  tube_clicked(int button, int x, int y);
void tube_quit();

void ards_draw(cairo_t *cr, cairo_t *cr2, int first);


