CC=gcc

CFLAGS = -ansi -c -Wall -pedantic

all: npc

npc: npc.o
	    $(CC) -o npc npc.o

clean:
	rm *.o
	rm npc 

%.o:    %.c
	$(CC) $(CFLAGS) $*.c


