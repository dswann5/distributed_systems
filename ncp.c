#include "net_include.h"
#include <assert.h>

#define NAME_LENGTH 80

void split_string(char *destination, char **dest_file_name, char **dest_comp_name); 

int main(int argc, char **argv)
{
    struct sockaddr_in    name;
    struct sockaddr_in    send_addr;
    struct hostent        h_ent;
    struct hostent        *p_h_ent;
    int                   host_num;
    int                   ss,sr;
    fd_set                mask;
    fd_set                dummy_mask,temp_mask;
    int                   num;
    struct timeval        timeout;

    int                   i, j;
    int                   new_sn, last_acked_sn, last_sent_sn;
    int                   nread;
    struct packet         first_packet; /** maybe think about making a pointer **/
    struct packet         ack;

    int                   loss_rate;
    char                  *filename;
    char                  *destination;
    char                  *dest_file_name; /* file name to be written at the receiver */
    char                  *dest_comp_name; /* receiver hostname */

    struct packet         window[WINDOW_SIZE];
    char                  ack_array[WINDOW_SIZE];

    FILE *fr;

    loss_rate = atoi(argv[1]);
    filename = argv[2];
    destination = argv[3];

    /** set loss rate **/
    sendto_dbg_init(loss_rate);

    /** begin socket logic **/
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

    /** properly parse argv[3] **/
    destination = argv[3];
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

    /** end socket logic **/


    /** open file for reading **/
    if ((fr = fopen(filename, "r")) == NULL) {
        perror("fopen");
        exit(0);
    }

    printf("Opened %s for reading...\n", filename);


    /* Continue sending first packet until it is acked */
    for(;;)
    {
        temp_mask = mask;
        timeout.tv_sec = 0;
        timeout.tv_usec = 1000000;

        /*first_packet.index = -1;*/
        strcpy(first_packet.payload, dest_file_name);
        first_packet.FIN = 0;
        first_packet.index = -1;

        sendto_dbg(ss, (const char *)&first_packet, PACKET_SIZE, 0,
            (struct sockaddr *)&send_addr, sizeof(send_addr));

        num = select( FD_SETSIZE, &temp_mask, &dummy_mask, &dummy_mask, &timeout);
        if (num > 0) {
            if ( FD_ISSET( sr, &temp_mask) ) {
                recv( sr, &ack, PACKET_SIZE, 0 );
                if (ack.ack_num == -1) {
                    printf("First packet is acked\nTemp Packet Payload: %s, Temp Packet Ack Num: %i\n", ack.payload, ack.ack_num);
                    break;
                }
            }
        }
    }

    /** initialize window **/
    for (i = 0; i < WINDOW_SIZE; i++) {
        window[i].FIN = -1;
        window[i].index = -1;
        window[i].ack_num = -1;
    }
    /** initialize sequence numbers**/
    last_acked_sn = 0;
    last_sent_sn = 0;
    /* Send subsequent data packets */
    int x;
    for(x = 0; x < 26; x++)
    {
        temp_mask = mask;
        timeout.tv_sec = 0;
        timeout.tv_usec = 10;

        if(window[0].index < 0) {
            /** send whole first window, think about catching FIN in first window **/
            while (last_sent_sn < WINDOW_SIZE) {
                nread = fread(window[last_sent_sn].payload, 1, PAYLOAD_SIZE, fr);
                window[last_sent_sn].index = last_sent_sn;
                sendto_dbg(ss, (const char *)&window[last_sent_sn], PACKET_SIZE, 0,
                    (struct sockaddr *)&send_addr, sizeof(send_addr));
                last_sent_sn++;
            }
        }
        num = select( FD_SETSIZE, &temp_mask, &dummy_mask, &dummy_mask, &timeout);
        if (num > 0) {
            if ( FD_ISSET( sr, &temp_mask) ) {
                recv( sr, &ack, PACKET_SIZE, 0 );
                
                /** need to catch rogue acks for first header HACKY CONDITIONAL**/
                if (ack.ack_num > last_acked_sn) {
                    printf("Ack Number: %i, Payload: %s\n", ack.ack_num, ack.payload);
                    new_sn = ack.ack_num - last_acked_sn;
                     i = last_acked_sn % WINDOW_SIZE; /**ugly, but mod is expensive**/
                    while(last_acked_sn < new_sn) {
                        nread = fread(window[i].payload, 1, PAYLOAD_SIZE, fr);
                        ack_array[i] = '0';
                        window[i].index = last_sent_sn;
                        if (nread < PAYLOAD_SIZE) {
                            window[i].FIN = PAYLOAD_SIZE;
                            break;
                        }
                        else
                            window[i].FIN = 0;
                        i++; /**ugly, but mod is expensive**/
                        last_acked_sn++;
                        last_sent_sn++;
                    }
                    j = last_acked_sn % WINDOW_SIZE;
                    for(i = 0; i < WINDOW_SIZE; i++) {
                        if (j > WINDOW_SIZE) {
                            j = j % WINDOW_SIZE;
                        }
                        if (ack.payload[j] == '0') {
                            sendto_dbg(ss, (const char *)&window[j], PACKET_SIZE, 0,
                                (struct sockaddr *)&send_addr, sizeof(send_addr));

                        } else {
                            ack_array[j] = '1';
                        }
                        j++;
                    }
                }
           }
	    } else {
            for(i = 0; i < WINDOW_SIZE; i++) {
                if (ack_array[i] == '0') {
                    sendto_dbg(ss, (const char *)&window[i], PACKET_SIZE, 0,
                        (struct sockaddr *)&send_addr, sizeof(send_addr));
                }
            }
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
   
    /*printf( "Destination filename: %s\n", *dest_file_name );*/
    
    /* Parse the second token, aka the destination hostname */
    *dest_comp_name = strtok(NULL, delimiter);
    
    /*printf("Destination: %s\n", *dest_comp_name);*/
}
