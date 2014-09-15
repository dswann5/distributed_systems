#define PACKET_SIZE 1400

typedef struct packet {
    int index;
    int ack_num;
    int FIN;
    char *data[PACKET_SIZE];
} packet;
