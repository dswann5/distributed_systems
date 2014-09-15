#include "net_include.h"

void split_string(char *destination, char *dest_file_name, char *dest_comp_name);

int main(int argc, char **argv)
{
    struct sockaddr_in host;
    struct hostent     h_ent, *p_h_ent;

    char               host_name[80];
    char               *c;

    int                s;
    int                ret;
    int                mess_len;
    char               mess_buf[MAX_MESS_LEN];
    char               *neto_mess_ptr = &mess_buf[sizeof(mess_len)]; 

    /******** code from us */
    struct packet *packet_buffer = malloc(WINDOW_SIZE * sizeof(struct packet));
    char *acks = malloc(WINDOW_SIZE * sizeof(char));
    struct timeval timeout;
    static long total_data;
    /*local*/
    int data_counter;
    /************/
    s = socket(AF_INET, SOCK_STREAM, 0); /* Create a socket (TCP) */
    if (s<0) {
        perror("Net_client: socket error");
        exit(1);
    }

    host.sin_family = AF_INET;
    host.sin_port   = htons(PORT);

    printf("Enter the server name:\n");
    if ( fgets(host_name,80,stdin) == NULL ) {
        perror("net_client: Error reading server name.\n");
        exit(1);
    }
    c = strchr(host_name,'\n'); /* remove new line */
    if ( c ) *c = '\0';
    c = strchr(host_name,'\r'); /* remove carriage return */
    if ( c ) *c = '\0';
    printf("Your server is %s\n",host_name);

    p_h_ent = gethostbyname(host_name);
    if ( p_h_ent == NULL ) {
        printf("net_client: gethostbyname error.\n");
        exit(1);
    }

    memcpy( &h_ent, p_h_ent, sizeof(h_ent) );
    memcpy( &host.sin_addr, h_ent.h_addr_list[0],  sizeof(host.sin_addr) );

    ret = connect(s, (struct sockaddr *)&host, sizeof(host) ); /* Connect! */
    if( ret < 0)
    {
        perror( "Net_client: could not connect to server"); 
        exit(1);
    }
/**
 * we added
 */
/*    int loss_rate = atoi(argv[1]);
    char filename[strlen(argv[2])]; strcpy(filename, argv[2]);
    char destination[strlen(argv[3])]; strcpy(filename, argv[3]);*/
    char *filename = argv[1];
    char *destination = "test@ugrad5.cs.jhu.edu";
    char *dest_file_name;
    char *dest_comp_name;

    /*split_string(destination, dest_file_name, dest_comp_name);*/

    FILE *fr;
    if ((fr = fopen(filename, "r")) == NULL) {
        perror("fopen");
        exit(0);
    }
    printf("Opened %s for reading...\n", argv[1]);


    for(;;)
    {
/*        printf("enter message: ");
        scanf("%s",neto_mess_ptr);
        mess_len = strlen(neto_mess_ptr) + sizeof(mess_len);
        memcpy( mess_buf, &mess_len, sizeof(mess_len) );

        ret = send( s, mess_buf, mess_len, 0);
        if(ret != mess_len) 
        {
            perror( "Net_client: error in writing");
            exit(1);
        }*/
        if (data_counter == MEGABYTE) {
            /*TODO PRINT/CALCULATE AVERAGE RATE OF TRANSFER*/
            printf("\n************\n%d\n*************\n", total_data);
            data_counter = 0;
        }
       
    }
    return 0;
}

void split_string(char *destination, char *dest_file_name, char *dest_comp_name)
{
    int at_index;
    int i;
    for (i = 0; i<strlen(destination); i++)
    {       
        if (destination[i] == '@')
        {
            at_index = i;
        }
    }
    memcpy(dest_file_name, &destination[0], at_index);
    dest_file_name[at_index] = '\0';
    
    memcpy(dest_comp_name, &destination[at_index+1], strlen(destination)-at_index-1); 
    dest_comp_name[strlen(destination)-at_index-1] = '\0';
}

