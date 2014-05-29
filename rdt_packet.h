#pragma once
#define HEADER_SIZE 20
#define DATA_SIZE 1024 
#define PACKET_SIZE (DATA_SIZE  + HEADER_SIZE)
#include <stdio.h>

class rdt_packet 
{
        private:
                int type;
                int seq_no;
                int contentLength;
                int ack;
                int fin;
                char data[DATA_SIZE];
        public:
                typedef enum {
                        TYPE_REQUEST,
                        TYPE_DATA,
                        TYPE_ACK,
                        TYPE_FINACK,
                        TYPE_END
                } TYPES;
                int getType() const { return type; }
                void setType(int type) { this->type = type; }
                int getSeqNo() const { return seq_no; }
                void setSeqNo(int seq_no) { this->seq_no = seq_no; }
                int getACK() const { return ack; }
                void setACK(int ack) { this->ack = ack; }
                bool isFin() const {return fin;}
                void setFin(bool fin) { this->fin = fin; }

                int getLength() const { return contentLength+HEADER_SIZE; }
                int getContentLength() const {return contentLength;}

                char *getData() { return data; }

                rdt_packet(const rdt_packet& p);
                rdt_packet operator=(const rdt_packet& p);

                bool properACKForPacket(rdt_packet ackpacket); //TODO: Implement this

                //default constructor creates a request packet
                rdt_packet() : type(TYPE_REQUEST), seq_no(0), ack(0), fin(0) { }

                //create a packet from a packet string
                rdt_packet(char *s);
                rdt_packet(int type, int seq_no, int ack, int length, int fin); 
                //rdt_packet(int type, int seq_no, int length, int fin);
                void readFromFile(FILE *resource);
                void copyToData(char *stuff, int size);
                char* packetStr() const;
};
double random_num();
