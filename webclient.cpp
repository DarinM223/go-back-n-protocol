#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>

#include "packet.h"
#define SERVER_PORT 8080

FILE *outputFile = NULL;

//packets.h
//client.cpp
//server.cpp
//report.pdf
//makefile
//largest tested file: 1MB
//smallest tested file: 5KB
//dd if=/dev/urandom of=3k bs=1k count=3
//./server portnumber cong_window prob_loss prob_corr
//./client hostname portnumber prob_lost prob_corr
//need timestamp


//1) send request
//2) receive receive receive
//3) if out of order packet, discard and resend last received sequence number

//TODO: change request and ACKs so it properly sends ACKS
double corrupt_prob = 0;
double loss_prob = 0;

void storeInFile(int length, char* packetStr) 
{
        int i;
        for (i = HEADER_SIZE; i < length; i++) {
                char byte = *(packetStr+i);
                fprintf(outputFile, "%c", byte);
                fflush(stdout);
        }
}

void sendACK(int _packet, int sockfd, struct sockaddr_in si_send, bool fin = false) 
{
        //construct ACK packet
        packet ack_packet(TYPE_ACK, _packet, HEADER_SIZE, fin);
        char *strp = ack_packet.packetStr();
        
        sendto(sockfd, strp, PACKET_SIZE, 0, (struct sockaddr*)&si_send, sizeof( si_send));
        free(strp);
}

void sendRequestPacket(char *filename, int sockfd, struct sockaddr_in si_send) {
        //packet request_packet;
        if (strlen(filename) > DATA_SIZE) 
                perror("filename is too long");
        packet request_packet(TYPE_REQUEST, 0,  sizeof(filename) + HEADER_SIZE, false);
        request_packet.copyToData(filename, strlen(filename)+1);

        char *strp = createPacketStr(request_packet);
        
        sendto(sockfd, strp, PACKET_SIZE, 0, (struct sockaddr*)&si_send, sizeof(si_send));
        free(strp);
}

int main(int argc, char *argv[]) 
{
        int sockfd, portno, n;
        struct sockaddr_in si_recv, si_send;

        if (argc < 4) {
                fprintf(stderr, "Usage: %s sender_hostname sender_portnumber  filename\n", argv[0]);
                exit(0);
        }

        char *sender_hostname = argv[1];
        int sender_portnumber = atoi(argv[2]);
        char *filename = argv[3];

        sockfd = socket(AF_INET, SOCK_DGRAM, 0);
        if (sockfd < 0) perror("Error opening socket!");

        bzero((char*)&si_send, sizeof(si_send));

        si_send.sin_family = AF_INET;
        inet_aton(sender_hostname, &si_send.sin_addr);
        si_send.sin_port = htons(sender_portnumber);

        //check if server exists
        struct hostent *server = gethostbyname(sender_hostname);
        if (!server) perror("Can't find hostname!");

        //open the file to be written to (filename.out)
        char *out;
        asprintf(&out, "%s%s", filename, ".out");
        outputFile = fopen(out, "w");
        free(out);

        //send initial request packet
        char pkt[1000];
        socklen_t slen = sizeof(si_send);
        int last_seqno = -1;

        sendRequestPacket(out, sockfd, si_send);

        while (1) {
                recvfrom(sockfd, pkt, PACKET_SIZE, 0, (struct sockaddr*) &si_send, &slen);
                //get packet from response
                //packet p = getPacketFromStr(pkt);
                packet p(pkt);
                //printf("Received packet: %d\n", p.seq_no);
                printf("Received packet: %d\n", p.getSeqNo());
                if (p.getSeqNo() == last_seqno+1) { //received in order packet 
                        last_seqno++;
                        printf("Packet valid, sending ACK\n");
                        //send ACK of the received packet
                        sendACK(p.getSeqNo(), sockfd, si_send); 
                        storeInFile(p.getLength(), pkt);                        
                        if (p.getType() == TYPE_END) break; //if the packet is the end of the packet, stop receiving
                } else { //if packet is out of order don't do nuthin
                        printf("Packet was out of order. Sending ACK for packet %d\n", last_seqno);
                        //send ACK of the last properly received sequence number
                        sendACK(last_seqno, sockfd, si_send);
                }
        }
        printf("Goodbye");
        close(sockfd);
        fclose(outputFile);
}
