CC=gcc

CFLAGS = -ansi -c -Wall -pedantic

all: ncp rcv

npc: ncp.o
	    $(CC) -o ncp ncp.o

rcv: rcv.o
	    $(CC) -o rcv rcv.o

clean:
	rm *.o
	rm ncp 
	rm rcv

%.o:    %.c
	$(CC) $(CFLAGS) $*.c


