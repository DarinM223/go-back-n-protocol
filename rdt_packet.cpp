#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "rdt_packet.h"

/*
 * Generates a random number
 */
double random_num() {
        return (double) rand() / (double) RAND_MAX;
}
/*
 * Constructs a packet with the specified fields
 * Have to set the content data manually with packet::readFromFile(FILE *resource)
 */
rdt_packet::rdt_packet(int type, int seq_no, int ack, int length, int fin) {
        this->type = type;
        this->seq_no = seq_no;
        this->contentLength = length;
        this->ack = ack;
        this->fin = fin;
}
/*
 * Constructs a packet from a packet string with header
 */
rdt_packet::rdt_packet(char* s) {
        memcpy(&this->type, s, 4);
        memcpy(&this->seq_no, s+4, 4);
        memcpy(&this->contentLength, s+8, 4);
        memcpy(&this->ack, s+12, 4);
        memcpy(&this->fin, s+16, 4);
        memcpy(&this->data, s+20, DATA_SIZE);
}

rdt_packet::rdt_packet(const rdt_packet &p) {
        char *s = p.packetStr();
        memcpy(&this->type, s, 4);
        memcpy(&this->seq_no, s+4, 4);
        memcpy(&this->contentLength, s+8, 4);
        memcpy(&this->ack, s+12, 4);
        memcpy(&this->fin, s+16, 4);
        memcpy(&this->data, s+20, DATA_SIZE);
}
rdt_packet rdt_packet::operator=(const rdt_packet &p) {
        char *s = p.packetStr();
        memcpy(&this->type, s, 4);
        memcpy(&this->seq_no, s+4, 4);
        memcpy(&this->contentLength, s+8, 4);
        memcpy(&this->ack, s+12, 4);
        memcpy(&this->fin, s+16, 4);
        memcpy(&this->data, s+20, DATA_SIZE);
        return *this;
}
/*
 * Returns a packet string constructed from the packet
 */
char* rdt_packet::packetStr() const {
        char *buf = (char*)malloc(PACKET_SIZE);
        memcpy(buf, &this->type, 4);
        memcpy(buf+4, &this->seq_no, 4);
        memcpy(buf+8, &this->contentLength, 4);
        memcpy(buf+12, &this->ack, 4);
        memcpy(buf+16, &this->fin, 4);
        memcpy(buf+20, &this->data, DATA_SIZE);
        return buf;
}
/*
 * Reads data from resource into the packet
 */
void rdt_packet::readFromFile(FILE *resource) {
        this->contentLength = fread(this->data, 1, DATA_SIZE, resource);
}
/*
 * Copies size amount from stuff into data
 */
void rdt_packet::copyToData(char *stuff, int size) {
        if (size > DATA_SIZE) return;
        memcpy(&this->data, stuff, size);
        this->contentLength = size;
}

bool rdt_packet::properACKForPacket(rdt_packet ackpacket) {
        if (ackpacket.getACK() == (this->getSeqNo() + this->getContentLength() + this->isFin()))
                return true;
        return false;
}
