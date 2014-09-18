#include "net_include.h"
#include <assert.h>

#define NAME_LENGTH 80

int gethostname(char*,size_t);

void PromptForHostName( char *my_name, char *host_name, size_t max_len ); 

void split_string(char *destination, char **dest_file_name, char **dest_comp_name); 

int main(int argc, char **argv)
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
    struct packet         *send_buf;
    char                  ack[WINDOW_SIZE];
    struct timeval        timeout;

    /**added**/
    int loss_rate = atoi(argv[1]);
    char *filename = argv[2];
    char *destination = argv[3];
    char *dest_file_name; /* file name to be written at the receiver */
    char *dest_comp_name; /* receiver hostname */

    int i;
    int nread;
    int ret;

    FILE *fr;

    int *at_index;
    sendto_dbg_init(loss_rate);

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
    
    /*PromptForHostName(my_name,host_name,NAME_LENGTH);*/
    split_string(destination, &dest_file_name, &dest_comp_name);
   
    p_h_ent = gethostbyname(dest_comp_name);
    if ( p_h_ent == NULL ) {
        printf("Ucast: gethostbyname error.\n");
        exit(1);
    }

    memcpy( &h_ent, p_h_ent, sizeof(h_ent));
    memcpy( &host_num, h_ent.h_addr_list[0], sizeof(host_num) );

    send_addr.sin_family = AF_INET;
    send_addr.sin_addr.s_addr = host_num; 
    send_addr.sin_port = htons(PORT);

    FD_ZERO( &mask );
    FD_ZERO( &dummy_mask );
    FD_SET( sr, &mask );
    FD_SET( (long)0, &mask ); /* stdin */

    send_buf = malloc(WINDOW_SIZE * sizeof(struct packet));

    if ((fr = fopen(filename, "r")) == NULL) {
        perror("fopen");
        exit(0);
    }

    printf("Opened %s for reading...\n", filename);

    int z = 0;
    int packet_index;
    struct packet temp_packet;

    /* Continue sending first packet until it is acked */
    for(;;)
    {
        temp_mask = mask;
        timeout.tv_sec = 0;
        timeout.tv_usec = 1000;

        strcpy(temp_packet.payload, dest_file_name);
        sendto_dbg(ss, &temp_packet, PACKET_SIZE, 0,
            (struct sockaddr *)&send_addr, sizeof(send_addr));
        num = select( FD_SETSIZE, &temp_mask, &dummy_mask, &dummy_mask, &timeout);
        
        if (num > 0) {
            if ( FD_ISSET( sr, &temp_mask) ) {
                recv( sr, &temp_packet, PACKET_SIZE, 0 );
                printf("First packet is acked\nTemp Packet Payload: %s\n", temp_packet.payload);
                break;
            }
        }
    }

    /* Send subsequent data packets */
    for(;;)
    {
        temp_mask = mask;
        timeout.tv_sec = 0;
        timeout.tv_usec = 1000;

            /*****/
        packet_index = z % WINDOW_SIZE;
        printf("Packet index %d\n", z);

       
        /* Read in from the file one packet at a time */   
        nread = fread(send_buf[packet_index].payload, 1, PAYLOAD_SIZE, fr);
        
        /* Checks to see if the file length % PAYLOAD_SIZE == 0 */
        if (nread == 0)
        {
           /*TODO possibly put nested infinite for in here,
            * break when ack received and break after nested for*/

            /*send_buf[packet_index].FIN = 0;
            sendto_dbg(ss, &send_buf[packet_index], PACKET_SIZE, 0,
                (struct sockaddr *)&send_addr, sizeof(send_addr));
*/
           break;
        }
        else if (nread < PAYLOAD_SIZE ) /* checks that we are at EOF */
        {
            send_buf[packet_index].FIN = nread;
            printf("FIN is set to %d\n", send_buf[packet_index].FIN);
            printf("Last char is %c\n",send_buf[packet_index].payload[nread]);
        }
        else /* there is more of the file to read */
        {
            send_buf[packet_index].FIN = 0;
        }
        send_buf[packet_index].index = z;
        send_buf[packet_index].ack_num = 0;
        

        sendto_dbg(ss, &send_buf[packet_index], PACKET_SIZE, 0,
                (struct sockaddr *)&send_addr, sizeof(send_addr));
        z++;
        /******/
        num = select( FD_SETSIZE, &temp_mask, &dummy_mask, &dummy_mask, &timeout);
        if (num > 0) {
            if ( FD_ISSET( sr, &temp_mask) ) {
                /*printf("I'M HEEEEERRREEE\n");*/
                /*recv( sr, &temp_packet, PACKET_SIZE, 0 );
                ack[temp_packet.ack_num] = 1;
                printf("This packet was acked: %d\n", temp_packet.ack_num);*/
           }
	    } else { /* timeout occurs */
            /*int a, b, c;
            char full = 1;*/
            /*int new_window_start_index;
            new_window_start_index = 0;
            /*while (ack[new_window_start_index] != 0) {
                new_window_start_index++;
            }*/
            /*for (a = 0; a < strlen(ack); a++) {
                if (ack[a] == 0) {
                    sendto_dbg(ss, &send_buf[a], PACKET_SIZE, 0,
                            (struct sockaddr *)&send_addr, sizeof(send_addr));
                    full = 0;
                }
            }

            if (full == 1) {
                for (b = 0; b < strlen(ack); b++) {
                    ack[b] = 0;
                }
            }*/
            /*for (c = 0; c < strlen(ack); c++) {
                if (c+new_window_start_index < strlen(ack)) {
                    ack[c] = ack[c+new_window_start_index];
                    send_buf[c] = send_buf[c+new_window_start_index];
                } else {
                    ack[c] = 0;
                }

            }*/
		    fflush(0);
        }
    }

    fclose(fr);
    return 0;

}

/** Updates dest_file_name and dest_comp_name with tokenized values from destination **/
void split_string(char *destination, char **dest_file_name, char **dest_comp_name) {

    const char delimiter[2] = "@";
   
    /* Get the first token, aka the destination file name */
    *dest_file_name = strtok(destination, delimiter);
   
    /*printf( "Destination filename: %s\n", dest_file_name );*/
    
    /* Parse the second token, aka the destination hostname */
    *dest_comp_name = strtok(NULL, delimiter);
    
    /*printf("Destination: %s\n", dest_comp_name);*/
}

void PromptForHostName( char *my_name, char *host_name, size_t max_len ) {

    char *c;

    gethostname(my_name, max_len );
    printf("My host name is %s.\n", my_name);

    printf( "\nEnter host to send to:\n" );
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
