#include "net_include.h"

#define NAME_LENGTH 80

int gethostname(char*,size_t);

void PromptForHostName( char *my_name, char *host_name, size_t max_len ); 

void split_string(char *destination, char *dest_file_name, char *dest_comp_name); 

void find_char(char *destination, int *index);
/*
void parse_filename(char *destination, char *dest_file_name);

void parse_compname(char *destination, char *dest_comp_name);
*/
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

    /****************************
     * **************
     ****************************/
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
    
    PromptForHostName(my_name,host_name,NAME_LENGTH);
    
    p_h_ent = gethostbyname(host_name);
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
    /****************************
     * **************
     ****************************/
/*    find_char(destination, at_index);
    printf("%i", at_index);
    dest_file_name = malloc((at_index+1) * sizeof(char));
    dest_comp_name = malloc((strlen(destination)-at_index-1) * sizeof(char));
*/
    send_buf = malloc(WINDOW_SIZE * sizeof(struct packet));
    split_string(destination, dest_file_name, dest_comp_name);

    if ((fr = fopen(filename, "r")) == NULL) {
        perror("fopen");
        exit(0);
    }

    printf("Opened %s for reading...\n", filename);


    /** our code **/

    int z;

    for (z = 0; z < 16; z++) {
        printf("Packet index %d\n", z);
        nread = fread(send_buf[z].payload, 1, PAYLOAD_SIZE, fr);
        sendto_dbg(ss, send_buf[z].payload, PAYLOAD_SIZE, 0,
            (struct sockaddr *)&send_addr, sizeof(send_addr));

    }

   /* 
    for(;;)
    {
        temp_mask = mask;
        timeout.tv_sec = 10;
	timeout.tv_usec = 0;
        num = select( FD_SETSIZE, &temp_mask, &dummy_mask, &dummy_mask, &timeout);
        if (num > 0) {
            if ( FD_ISSET( sr, &temp_mask) ) {
                from_len = sizeof(from_addr);
                bytes = recvfrom( sr, mess_buf, sizeof(mess_buf), 0,  
                          (struct sockaddr *)&from_addr, 
                          &from_len );
                mess_buf[bytes] = 0;
                from_ip = from_addr.sin_addr.s_addr;

                printf( "Received from (%d.%d.%d.%d): %s\n", 
								(htonl(from_ip) & 0xff000000)>>24,
								(htonl(from_ip) & 0x00ff0000)>>16,
								(htonl(from_ip) & 0x0000ff00)>>8,
								(htonl(from_ip) & 0x000000ff),
								mess_buf );

            }else if( FD_ISSET(0, &temp_mask) ) {
                bytes = read( 0, input_buf, sizeof(input_buf) );
                input_buf[bytes] = 0;
                printf( "There is an input: %s\n", input_buf );
                sendto( ss, input_buf, strlen(input_buf), 0, 
                    (struct sockaddr *)&send_addr, sizeof(send_addr) );
            }
	} else {
		printf(".");
		fflush(0);
        }
    }
*/
    fclose(fr);
    return 0;

}

/**added**/
void split_string(char *destination, char *dest_file_name, char *dest_comp_name) {

    const char delimiter[2] = "@";
   
    /* Get the first token, aka the destination file name */
    dest_file_name = strtok(destination, delimiter);
   
    printf( "Destinaion filename: %s\n", dest_file_name );
    
    /* Parse the second token, aka the destination hostname */
    dest_comp_name = strtok(NULL, delimiter);
    
    printf("Destination: %s\n", dest_comp_name);
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
