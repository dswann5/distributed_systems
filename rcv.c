#include "net_include.h"

#define NAME_LENGTH 80

int main()
{
    struct sockaddr_in    name;
    struct sockaddr_in    from_addr;
    socklen_t             from_len;
    int                   from_ip;
    int                   sr;
    fd_set                mask;
    fd_set                dummy_mask,temp_mask;
    int                   bytes;
    int                   num;
    struct timeval        timeout;

    /**added**/
    int i = 0;
    struct packet *rcv_buffer = malloc(WINDOW_SIZE * sizeof(struct packet));
    
    /*********/
    sr = socket(AF_INET, SOCK_DGRAM, 0);  /* socket for receiving (udp) */
    if (sr<0) {
        perror("Ucast: socket");
        exit(1);
    }

    name.sin_family = AF_INET; 
    name.sin_addr.s_addr = INADDR_ANY; 
    name.sin_port = htons(PORT);

    if ( bind( sr, (struct sockaddr *)&name, sizeof(name) ) < 0 ) {
        perror("Ucast: bind");
        exit(1);
    }

    FD_ZERO( &mask );
    FD_ZERO( &dummy_mask );
    FD_SET( sr, &mask );
    FD_SET( (long)0, &mask ); /* stdin */
    for(;;)
    {
        temp_mask = mask;
        timeout.tv_sec = 10;
	timeout.tv_usec = 0;
        num = select( FD_SETSIZE, &temp_mask, &dummy_mask, &dummy_mask, &timeout);
        if (num > 0) {
            if ( FD_ISSET( sr, &temp_mask) ) {
                from_len = sizeof(from_addr);
                
                /*bytes = recvfrom( sr, mess_buf, sizeof(mess_buf), 0,  
                          (struct sockaddr *)&from_addr, 
                          &from_len );
                mess_buf[bytes] = 0;
                */
                
                from_ip = from_addr.sin_addr.s_addr;


                printf( "Received from (%d.%d.%d.%d): \n", 
								(htonl(from_ip) & 0xff000000)>>24,
								(htonl(from_ip) & 0x00ff0000)>>16,
								(htonl(from_ip) & 0x0000ff00)>>8,
								(htonl(from_ip) & 0x000000ff));
                
                printf("This is the index: %d", i);

                recv( sr, rcv_buffer[i].payload, PAYLOAD_SIZE, 0 );

                printf("%s\n", rcv_buffer[i].payload);
                i++;
            }
	    } else {
		    printf(".");
		    fflush(0);
        }
    }

    return 0;

}
