LIBS = `pkg-config --libs gtk+-3.0`

CFLAGS = -std=c99 `pkg-config --cflags gtk+-3.0`

all: tek4010

tek4010: main.c main.h tube.c tube.h tek4010.c ards.c
	$(CC) -o tek4010 main.c tube.c tek4010.c ards.c $(LIBS) $(CFLAGS)

install: tek4010
	./install

.PHONY : clean
clean :
	-rm -f tek4010
