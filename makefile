CC=gcc

CFLAGS = -ansi -c -Wall -pedantic

all: npc rcv

npc: npc.o
	    $(CC) -o npc npc.o

rcv: rcv.o
	    $(CC) -o rcv rcv.o

clean:
	rm *.o
	rm npc 
	rm rcv

%.o:    %.c
	$(CC) $(CFLAGS) $*.c


