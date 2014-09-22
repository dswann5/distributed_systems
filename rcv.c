#include "net_include.h"

#define NAME_LENGTH 80

void print_stats(int is_done);

    struct sockaddr_in    name;
    struct sockaddr_in    from_addr;
    socklen_t             from_len;
    int                   from_ip;
    int                   ss,sr;
    fd_set                mask;
    fd_set                dummy_mask,temp_mask;
    int                   num;
    struct timeval        timeout;

    int                   loss_rate;
    int                   i;
    int                   new_index, shift_index;
    int                   curr_seq_num;
    char         	      nack_array[WINDOW_SIZE];

    FILE                  *fw;
    struct packet         ack;
    struct packet         first_packet;
    struct packet         rcv_buf;
    char                  *filename;
    struct packet         window[WINDOW_SIZE];

    /* Stats globals */
    int 		  total_data_transferred;
    struct timeval	  start_time;
    struct timeval	  local_time;


int main(int argc, char **argv)
{ 
    loss_rate = atoi(argv[1]);

    total_data_transferred = 0;
    gettimeofday(&start_time, NULL);
    local_time = (struct timeval){0};

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

    FD_ZERO( &mask );
    FD_ZERO( &dummy_mask );
    FD_SET( sr, &mask );
    FD_SET( (long)0, &mask ); /* stdin */
    /** end socket logic **/



    /* Wait for first packet */
    for (;;)
    {
        temp_mask = mask;
        timeout.tv_sec = 0;
        timeout.tv_usec = TIMEOUT_USEC;

        num = select( FD_SETSIZE, &temp_mask, &dummy_mask, &dummy_mask, &timeout);
        if (num > 0) {
            if ( FD_ISSET( sr, &temp_mask) ) {
                from_len = sizeof(from_addr);

                recvfrom( sr, &first_packet, PACKET_SIZE, 0,
                        (struct sockaddr *)&from_addr,
                        &from_len );
                from_addr.sin_port = htons(PORT);
                from_ip = from_addr.sin_addr.s_addr;
                printf( "Received from (%d.%d.%d.%d)\n", 
								(htonl(from_ip) & 0xff000000)>>24,
								(htonl(from_ip) & 0x00ff0000)>>16,
								(htonl(from_ip) & 0x0000ff00)>>8,
								(htonl(from_ip) & 0x000000ff));
                
                filename = first_packet.payload;

                printf("First packet index: %i received, sending ack: %s", first_packet.index, first_packet.payload);
                first_packet.ack_num = -1;
                sendto_dbg( ss, (const char *)&first_packet, PACKET_SIZE, 0,
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

    /* initialize window */
    for (i = 0; i < WINDOW_SIZE; i++) {
        window[i].FIN = 0;
        window[i].index = -1;
        window[i].ack_num = -1;
        nack_array[i] = '0';
    }
    printf("%s\n", nack_array);

    /* initialize curr_seq_num 
     * last_acked_sn */
    curr_seq_num = 0;
    int is_done = 0;
    
    /* Continue receiving data packets */
    /*int x;
    for (x = 0; x < 26; x++)*/
    for (;;)
    {
        temp_mask = mask;
        timeout.tv_sec = 0;
        timeout.tv_usec = TIMEOUT_USEC;

	    if (is_done == 1)
	    {
	        break;
	    }

        for (i = 0; i < WINDOW_SIZE / 2; i++) {
            num = select( FD_SETSIZE, &temp_mask, &dummy_mask, &dummy_mask, &timeout);
            if (num > 0) {
                if ( FD_ISSET( sr, &temp_mask) ) {
                    printf( "Received from (%d.%d.%d.%d)\n", 
                                    (htonl(from_ip) & 0xff000000)>>24,
                                    (htonl(from_ip) & 0x00ff0000)>>16,
                                    (htonl(from_ip) & 0x0000ff00)>>8,
                                    (htonl(from_ip) & 0x000000ff));

                    recvfrom( sr, &rcv_buf, PACKET_SIZE, 0,
                             (struct sockaddr *)&from_addr,
                             &from_len );
                    
                    /** checks if received packet is first packet **/
                    if (rcv_buf.index < 0)
                        break;
                    printf("Sequence Number: %i, Payload: %s, Curr: %i\n", rcv_buf.index, rcv_buf.payload, curr_seq_num);

                    /* Check for final packet */
                    if (rcv_buf.FIN >= 0) {
                        printf("RECEIVED FINAL PACKET\n");
                        fwrite(&window[new_index].payload, 1, rcv_buf.FIN, fw );

                        total_data_transferred += PAYLOAD_SIZE;
                        print_stats(1);

                        fclose(fw);
                        is_done = 1;
                        break;
                    }

                    /* Check for a subsequent packet and update window/nack array accordingly */
                    if (curr_seq_num <= rcv_buf.index) {
                        new_index = rcv_buf.index % WINDOW_SIZE;
                        window[new_index] = rcv_buf;
                        nack_array[new_index] = '1';
                        
                        /**shift the window**/
                        shift_index = curr_seq_num % WINDOW_SIZE;
                        while(window[shift_index].index > -1) {
                            shift_index = curr_seq_num % WINDOW_SIZE;
                            fwrite(window[shift_index].payload, 1, PAYLOAD_SIZE, fw);

                            total_data_transferred += PAYLOAD_SIZE;
                            print_stats(0);
	
                            window[shift_index].index = -1;
                            nack_array[shift_index] = '0';
                            curr_seq_num++;
                        }
                    }
                } else {
                    fflush(0);
                }
            }
        }
        if (rcv_buf.index < 0) /** resend the first packet ack, it was lost **/
        {
            ack.ack_num = rcv_buf.index;
        } 
        else /* Otherwise update the cumulative ack with seq num and nacks */
        {
            ack.ack_num = curr_seq_num;
            strcpy( ack.payload, nack_array);
        }
        
        from_addr.sin_port = htons(PORT);
        printf("sent ack, ack_num: %i, payload: %s\n", ack.ack_num, ack.payload);
        sendto_dbg( ss, (const char *)&ack, PACKET_SIZE, 0,
                (struct sockaddr *)&from_addr, sizeof(from_addr));

        /*printf("This is the dummy ack number: %d\n", ack.ack_num);*/

    }
    /*fclose(fw);*/
    return 0;
}

void print_stats(int is_done)
{
    /* Print stats for every 50 MB */
    if (total_data_transferred % PAYLOAD_SIZE == 0)/*(50*1048576) == 0)*/
    {
	printf("**********************************************************************\n");
	printf("Current Mbytes transferred: %d\n", total_data_transferred); /* 1048576);*/

	/* Find the current time for later comparison */
	struct timeval temp_time;
	gettimeofday(&temp_time);

	/* Calculate time in microseconds since last 50 MB transfer*/
 	float local_elapsed_time = (temp_time.tv_sec-local_time.tv_sec)*1000000.0 + temp_time.tv_usec-local_time.tv_usec;

	printf("Local elapsed time: %f microseconds\n", local_elapsed_time);	
	printf("Average transfer rate: %f Mbits/s\n", /*(50*131072)*/(PAYLOAD_SIZE/8) / (local_elapsed_time/1000000.0));

	/* Reset elapsed time */
	gettimeofday(&local_time);
    }
    if (is_done == 1) /* print final stats */
    {
	struct timeval end_time;
	gettimeofday(&end_time);
	unsigned long total_elapsed = (end_time.tv_sec-start_time.tv_sec)*1000000.0 + end_time.tv_usec-start_time.tv_usec;
	float average_transfer_rate = (total_data_transferred/8.0)/(total_elapsed/1000000.0); 
	
	printf("***********************************************************************\n");
	printf("Total Mbytes transferred: %d\n", total_data_transferred); /* 1048576);*/	
   	printf("Total time elapsed: %lu us \n", total_elapsed);
	printf("Total average transfer rate: %f Mbit/s \n", average_transfer_rate);
    }
}
