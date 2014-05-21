#define HEADER_SIZE 20
#define DATA_SIZE 1024 
#define PACKET_SIZE (DATA_SIZE  + HEADER_SIZE)
#include <stdio.h>

typedef enum {
        TYPE_REQUEST,
        TYPE_MESSAGE,
        TYPE_ACK,
        TYPE_END
} TYPES;

class packet 
{
        private:
                int type;
                int seq_no;
                int length;
                int ack;
                int fin;
                char data[DATA_SIZE];
        public:
                int getType() const { return type; }
                int getSeqNo() const { return seq_no; }
                int getACK() const { return ack; }
                bool isFin() const {return fin;}
                int getLength() const { return length; }
                char *getData() { return data; }

                packet(const packet& p);
                packet operator=(const packet& p);

                bool properACKForPacket(packet ackpacket); //TODO: Implement this

                packet() { }
                packet(char *s);
                packet(int type, int ack, int seq_no, int length, int fin); 
                void readFromFile(FILE *resource);
                void copyToData(char *stuff, int size);
                char* packetStr() const;
};

char *createPacketStr(packet p);
packet getPacketFromStr(char *s);
