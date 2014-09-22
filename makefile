CC=gcc

CFLAGS = -ansi -c -Wall -pedantic

all: ncp rcv

ncp: ncp.o sendto_dbg.o
	    $(CC) -o ncp ncp.o sendto_dbg.o

rcv: rcv.o sendto_dbg.o
	    $(CC) -o rcv rcv.o sendto_dbg.o

#t_rcv: t_rcv.o sendto_dbg.o
#	    $(CC) -o t_rcv t_rcv.o sendto_dbg.o

#t_ncp: t_ncp.o sendto_dbg.o
#	    $(CC) -o t_ncp t_ncp.o sendto_dbg.o

clean:
	rm *.o
	rm ncp 
	rm rcv
	rm t_rcv
	rm t_ncp

%.o:    %.c
	$(CC) $(CFLAGS) $*.c


