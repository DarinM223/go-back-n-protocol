#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "packet.h"

/*
 * Constructs a packet with the specified fields
 * Have to set the content data manually with packet::readFromFile(FILE *resource)
 */
packet::packet(int type, int ack, int seq_no, int length, int fin) {
        this->type = type;
        this->seq_no = seq_no;
        this->length = length;
        this->ack = ack;
        this->fin = fin;
}
/*
 * Constructs a packet from a packet string with header
 */
packet::packet(char* s) {
        memcpy(&this->type, s, 4);
        memcpy(&this->seq_no, s+4, 4);
        memcpy(&this->length, s+8, 4);
        memcpy(&this->ack, s+12, 4);
        memcpy(&this->fin, s+16, 4);
        memcpy(&this->data, s+20, DATA_SIZE);
}

packet::packet(const packet &p) {
        this->type = p.type;
        this->seq_no = p.seq_no;
        this->ack = p.ack;
        this->fin = p.fin;
        this->length = p.length;
}
packet packet::operator=(const packet &p) {
        this->type = p.type;
        this->seq_no = p.seq_no;
        this->ack = p.ack;
        this->fin = p.fin;
        this->length = p.length;
        return *this;
}
/*
 * Returns a packet string constructed from the packet
 */
char* packet::packetStr() const {
        char *buf = (char*)malloc(PACKET_SIZE);
        memcpy(buf, &this->type, 4);
        memcpy(buf+4, &this->seq_no, 4);
        memcpy(buf+8, &this->length, 4);
        memcpy(buf+12, &this->ack, 4);
        memcpy(buf+16, &this->fin, 4);
        memcpy(buf+20, &this->data, DATA_SIZE);
        return buf;
}
/*
 * Reads data from resource into the packet
 */
void packet::readFromFile(FILE *resource) {
        this->length = fread(this->data, 1, DATA_SIZE, resource) + HEADER_SIZE;
}
/*
 * Copies size amount from stuff into data
 */
void packet::copyToData(char *stuff, int size) {
        memcpy(&this->data, stuff, size);
}

bool packet::properACKForPacket(packet ackpacket) {
        return true;
}
