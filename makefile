LIBS = `pkg-config --libs gtk+-3.0`

CFLAGS = `pkg-config --cflags gtk+-3.0`

all: tek4010

tek4010: main.c tek4010.c tek4010.h main.h
	gcc -o tek4010 main.c tek4010.c tek4010.h main.h $(LIBS) $(CFLAGS)

install:
	./install