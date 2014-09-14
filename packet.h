#define PACKET_SIZE 1400

typedef struct packet {
    int index; //sequence number
    int ack_num;
    int FIN;
    char *data[PACKET_SIZE];
} packet;
