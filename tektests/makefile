CC = gcc
LDFLAGS = -lm

all: incremental circles dodecagon

incremental: incremental.c

circles: circles.c
	
dodecagon: tekio.h tekio.c dodecagon.c
	cc -o dodecagon dodecagon.c tekio.c -lm
