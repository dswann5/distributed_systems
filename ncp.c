#include "net_include.h"

#define NAME_LENGTH 80

int gethostname(char*,size_t);

void PromptForHostName( char *my_name, char *host_name, size_t max_len ); 

/**added**/
void split_string(char *destination, char *dest_file_name, char *dest_comp_name);

int main(int argc, char **argv)
{
    struct sockaddr_in    send_addr;
    struct hostent        h_ent;
    struct hostent        *p_h_ent;
    char                  host_name[NAME_LENGTH] = {'\0'};
    char                  my_name[NAME_LENGTH] = {'\0'};
    int                   host_num;
    int                   ss;
    fd_set                mask;
    fd_set                dummy_mask,temp_mask;
    int                   bytes;
    int                   num;
    struct timeval        timeout;

    /*** NON COPIED CODE ***/
    struct packet *send_buffer = malloc(WINDOW_SIZE * sizeof(struct packet));
    /*char *acks = malloc(WINDOW_SIZE * sizeof(char));*/
    
    int loss_rate = atoi(argv[1]);
    char *filename = argv[2];
    char *destination = argv[3];
    char *dest_file_name;
    char *dest_comp_name;

    int i;
    /*int nread;*/
    int ret;

    FILE *fr;

    sendto_dbg_init(loss_rate);
    /********************/

/*** Socket Initialization (I think) ***/

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
    /*FD_SET( sr, &mask );*/
    FD_SET( (long)0, &mask ); /* stdin */

/**************/

    /**ADDED CODE**********/

    split_string(destination, &dest_file_name, &dest_comp_name);

    printf("-------------------\nDestination:%s\nDest-File-Name:%s\nDest-Comp-Name:%s\n", destination, &dest_file_name, &dest_comp_name);
    printf("-------------------\nDest-File-Name:%s\n", &dest_file_name);
    if ((fr = fopen(filename, "r")) == NULL) {
        perror("fopen");
        exit(0);
    }

    printf("Opened %s for reading...\n", filename);


    /** our code **/

    int nread;
    for (i = 0; i < WINDOW_SIZE; i++) {
        temp_mask = mask;
        timeout.tv_sec = 10;
	    timeout.tv_usec = 0;
        num = select( FD_SETSIZE, &temp_mask, &dummy_mask, &dummy_mask, &timeout);
        if (num > 0) {
            if (FD_ISSET(0, &temp_mask) ) {
                nread = fread(send_buffer[i].payload, 1, PAYLOAD_SIZE, fr);
                ret = sendto_dbg(ss, send_buffer[i].payload, PAYLOAD_SIZE, 0,
                (struct sockaddr *)&send_addr, sizeof(send_addr)); 
            }
        } 
        else {
		    printf(".");
		    fflush(0);
        }
    }

    /*
    for(;;)
    {
        temp_mask = mask;
        timeout.tv_sec = 10;
	timeout.tv_usec = 0;
        num = select( FD_SETSIZE, &temp_mask, &dummy_mask, &dummy_mask, &timeout);
        if (num > 0) {
            if (FD_ISSET(0, &temp_mask) ) {
                bytes = read( 0, input_buf, sizeof(input_buf) );
                input_buf[bytes] = 0;
                printf( "There is an input: %s\n", input_buf );
                sendto_dbg( ss, input_buf, strlen(input_buf), 0, 
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
/**TODO fix dest_file_name **/
void split_string(char *destination, char *dest_file_name, char *dest_comp_name) {
    int at_index;
    int i, j, k;

    for (i = 0; i<strlen(destination); i++)
    {       
        
        if (destination[i] == '@')
        {
            at_index = i;
        }

    }

    for (j = 0; j < at_index; j++)
    {
        dest_file_name[j] = malloc(sizeof(char));
    }

    for (k = 0; k < strlen(destination)-at_index-1; k++)
    {
        dest_comp_name[k] = malloc(sizeof(char));
    }

    memcpy(dest_file_name, &destination[0], at_index+1);
    dest_file_name[at_index] = '\0';
    
    memcpy(dest_comp_name, &destination[at_index+1], strlen(destination)-at_index-1); 
    dest_comp_name[strlen(destination)-at_index-1] = '\0';
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
