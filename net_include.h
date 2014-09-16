#include <stdio.h>

#include <stdlib.h>

#include <string.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <netinet/in.h> 
#include <netdb.h>

#include <errno.h>

#include "packet.h"
#include "sendto_dbg.h"

#define PORT	     10100
#define WINDOW_SIZE 16
#define MAX_MESS_LEN 1400
#define MEGABYTE 52428800
