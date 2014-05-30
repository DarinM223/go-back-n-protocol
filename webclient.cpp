#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>

#include "rdt_packet.h"
#define SERVER_PORT 8080

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
FILE *outputFile = NULL;

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

        char pkt[PACKET_SIZE];
        socklen_t slen = sizeof(si_send);
        int last_seqno = -1;
        int last_contentlength = -1;
        rdt_packet last_seqpacket;

        //send initial request packet
        //content length sizeof filename, ACK#, SEQ# are 0 because it is just starting
        rdt_packet request_packet;
        request_packet.copyToData(filename, strlen(filename)+1); 

        
        //create a string from the packet and send it
        char *strp = request_packet.packetStr(); 
        sendto(sockfd, strp, PACKET_SIZE, 0, (struct sockaddr*)&si_send, sizeof(si_send));
        free(strp);
        strp = NULL;

        //TODO: make proper output messages
        //receive data packets, if data packet is in order, send ack for the packet, otherwise send ack for last sequence number
        while (1) {
                recvfrom(sockfd, pkt, PACKET_SIZE, 0, (struct sockaddr*) &si_send, &slen);
                //get packet from response
                rdt_packet p(pkt);
                double r = random_num();
                if (r < corrupt_prob) {
                        printf("Packet was corrupted!\n");
                } else if (r < loss_prob) {
                        printf("Packet was lost!\n");
                } else if (p.getType() == rdt_packet::TYPE_DATA && p.getSeqNo() == last_seqno+1) { //received in order packet 
                        printf("Received packet: %d\n", p.getSeqNo());
                        //last_seqno++;
                        last_seqpacket = p;
                        last_contentlength = p.getContentLength();
                        printf("Packet valid, sending ACK\n");
                        //send ACK of the received packet
                        rdt_packet ack_packet(rdt_packet::TYPE_ACK, p.getACK(), p.getSeqNo()+p.getContentLength()+p.isFin(), 0, p.isFin());
                        strp = ack_packet.packetStr();
                        sendto(sockfd, strp, PACKET_SIZE, 0, (struct sockaddr*)&si_send, sizeof(si_send));
                        free(strp);
                        strp = NULL;

                        for (int i = 0; i < p.getContentLength(); i++) {
                                char byte = *(p.getData()+i);
                                fprintf(outputFile, "%c", byte);
                                fflush(stdout);
                        }
                        if (p.getType() == rdt_packet::TYPE_END) break; //if the packet is the end of the packet, stop receiving
                } else if (p.getType() == rdt_packet::TYPE_DATA) { //if packet is out of order resend ack for last received
                        printf("Packet was out of order. Sending ACK for packet %d\n", last_seqno);
                        //send ACK of the last properly received sequence number
                        rdt_packet ack_packet(rdt_packet::TYPE_ACK, last_seqpacket.getACK(), last_seqpacket.getSeqNo()+last_seqpacket.getContentLength()+last_seqpacket.isFin(), 0, last_seqpacket.isFin());
                        strp = ack_packet.packetStr();
                        sendto(sockfd, strp, PACKET_SIZE, 0, (struct sockaddr*)&si_send, sizeof(si_send));
                        free(strp);
                        strp = NULL;
                } 
        }
        printf("Goodbye");
        close(sockfd);
        fclose(outputFile);
}
