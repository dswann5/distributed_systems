#define PAYLOAD_SIZE 1400

typedef struct packet {
    int index;
    int ack_num;
    int FIN;
    char payload[PAYLOAD_SIZE];
} packet;
