#include "net_include.h"

#define NAME_LENGTH 80

int gethostname(char*,size_t);

void PromptForHostName( char *my_name, char *host_name, size_t max_len ); 

int main()
{
    struct sockaddr_in    name;
    struct sockaddr_in    send_addr;
    struct sockaddr_in    from_addr;
    socklen_t             from_len;
    struct hostent        h_ent;
    struct hostent        *p_h_ent;
    char                  host_name[NAME_LENGTH] = {'\0'};
    char                  my_name[NAME_LENGTH] = {'\0'};
    int                   host_num;
    int                   from_ip;
    int                   ss,sr;
    fd_set                mask;
    fd_set                dummy_mask,temp_mask;
    int                   bytes;
    int                   num;
    struct packet         *rcv_buf;
    struct timeval        timeout;

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
 
    ss = socket(AF_INET, SOCK_DGRAM, 0); /* socket for sending (udp) */
    if (ss<0) {
        perror("Ucast: socket");
        exit(1);
    } 
/*
    PromptForHostName(my_name,host_name,NAME_LENGTH);
    
    p_h_ent = gethostbyname(host_name);
    if ( p_h_ent == NULL ) {
        printf("Ucast: gethostbyname error.\n");
        exit(1);
    }

    memcpy( &h_ent, p_h_ent, sizeof(h_ent));
    memcpy( &host_num, h_ent.h_addr_list[0], sizeof(host_num) );
*/
    send_addr.sin_family = AF_INET;
    send_addr.sin_addr.s_addr = host_num; 
    send_addr.sin_port = htons(PORT);

    FD_ZERO( &mask );
    FD_ZERO( &dummy_mask );
    FD_SET( sr, &mask );
    FD_SET( (long)0, &mask ); /* stdin */

    int packet_index;
    rcv_buf = malloc(WINDOW_SIZE * sizeof(struct packet));

    FILE *fw;
    int size;
    char file_end = 0;
    struct packet dummy_ack;
    struct packet dummy_packet;
    char *filename;
    /* Wait for first packet */
    for (;;)
    {
        temp_mask = mask;
        timeout.tv_sec = 0;
        timeout.tv_usec = 1000;

        num = select( FD_SETSIZE, &temp_mask, &dummy_mask, &dummy_mask, &timeout);
        if (num > 0) {
            if ( FD_ISSET( sr, &temp_mask) ) {
                from_len = sizeof(from_addr);
                recvfrom( sr, &dummy_ack, PACKET_SIZE, 0,
                        (struct sockaddr *)&from_addr,
                        &from_len );
                from_addr.sin_port = htons(PORT);
                from_ip = from_addr.sin_addr.s_addr;
                printf( "Received from (%d.%d.%d.%d)\n", 
								(htonl(from_ip) & 0xff000000)>>24,
								(htonl(from_ip) & 0x00ff0000)>>16,
								(htonl(from_ip) & 0x0000ff00)>>8,
								(htonl(from_ip) & 0x000000ff));

                printf("%d\n", from_addr);
                filename = dummy_ack.payload;

                printf("First packet received, sending ack: %s", dummy_ack.payload);
                dummy_ack.ack_num = -1;
                sendto_dbg( ss, &dummy_ack, PACKET_SIZE, 0,
                        (struct sockaddr *)&from_addr, sizeof(from_addr));
                break;
            }
        }
    }
    /* Open file to write to after receiving name in first paylaod */
    if ((fw = fopen(filename, "wb")) == NULL) {
        perror("fopen");
        exit(0);
    }

    /* Continue receiving data packets */
    for(;;)
    {
        temp_mask = mask;
        timeout.tv_sec = 0;
        timeout.tv_usec = 1000;

        num = select( FD_SETSIZE, &temp_mask, &dummy_mask, &dummy_mask, &timeout);
        if (num > 0) {
            if ( FD_ISSET( sr, &temp_mask) ) {
                printf( "Received from (%d.%d.%d.%d)\n", 
								(htonl(from_ip) & 0xff000000)>>24,
								(htonl(from_ip) & 0x00ff0000)>>16,
								(htonl(from_ip) & 0x0000ff00)>>8,
								(htonl(from_ip) & 0x000000ff));

                recvfrom( sr, &dummy_packet, PACKET_SIZE, 0,
                         (struct sockaddr *)&from_addr,
                         &from_len );
                packet_index = dummy_packet.index % 16;
                rcv_buf[packet_index] = dummy_packet;

                from_addr.sin_port = htons(PORT);
                dummy_ack.ack_num = dummy_packet.index;
                sendto_dbg( ss, &dummy_ack, PACKET_SIZE, 0,
                        (struct sockaddr *)&from_addr, sizeof(from_addr));
                printf("This is the dummy ack number: %d\n", dummy_ack.ack_num);
                /* write method for this */
                /* hackily catches when 1st packet fails and 2nd succeeds */
                /*if (&rcv_buf[0]) {
                    /* checks whether to move window up */
                  /*  if (dummy_packet.index > rcv_buf[0].index) {
                        int window_index;
                        for (window_index = 0; window_index < WINDOW_SIZE; window_index++) {
                            rcv_buf[window_index] = rcv_buf[window_index+1];
                        }
                    }
                }
                /* last packet case */
                if (rcv_buf[packet_index].FIN > 0) {
                    size = rcv_buf[packet_index].FIN;
                    printf("\n*******************\nHIIIIIIIIIIIIIIIIIIII %d\n************\n", rcv_buf[packet_index].FIN);

                    /*printf("swiggity swooty %s\n", rcv_buf[packet_index].payload);*/
                    fwrite(&rcv_buf[packet_index].payload, 1, size, fw );
                    fclose(fw);
                    file_end = 1;
                    /*break;*/
                } else {
                    size = PAYLOAD_SIZE;
                }

                /*printf("swag %s\n", rcv_buf[packet_index].payload);*/
                if (file_end == 0)
                    fwrite(&rcv_buf[packet_index].payload, 1, size, fw );
/*                if (i == 0) {
                    int q;
                    for (q = 0; q < WINDOW_SIZE; q++) {
                        if (q+1 < WINDOW_SIZE)
                            rcv_buf[q] = rcv_buf[q+1];
                    }
                }*/
            }
	    } else {
		    fflush(0);
        }
    }
    /*fclose(fw);*/
    return 0;

}

void PromptForHostName( char *my_name, char *host_name, size_t max_len ) {

    char *c;

    gethostname(my_name, max_len );
    printf("My host name is %s.\n", my_name);

    printf( "\nEnter host to receive from:\n" );
    if ( fgets(host_name,max_len,stdin) == NULL ) {
        perror("Ucast: read_name");
        exit(1);
    }
    
    c = strchr(host_name,'\n');
    if ( c ) *c = '\0';
    c = strchr(host_name,'\r');
    if ( c ) *c = '\0';

    printf( "Sending from %s to %s.\n", my_name, host_name );

}
