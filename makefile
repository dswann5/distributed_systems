CC=gcc

CFLAGS = -ansi -c -Wall -pedantic

all: ncp rcv

ncp: ncp.o sendto_dbg.o
	    $(CC) -o ncp ncp.o sendto_dbg.o

rcv: rcv.o sendto_dbg.o
	    $(CC) -o rcv rcv.o sendto_dbg.o

clean:
	rm *.o
	rm ncp 
	rm rcv

%.o:    %.c
	$(CC) $(CFLAGS) $*.c


