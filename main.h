
#include <cairo.h>

void tek4010_init(int argc, char* argv[]);
void tek4010_draw(cairo_t *cr, cairo_t *cr2, int width, int height, int first);
int tek4010_on_timer_event();
int tek4010_clicked(int button, int x, int y);
void tek4010_quit();

