#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <unistd.h>
#include <time.h>
#include <fcntl.h>
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
//./client hostname portnumber filename prob_lost prob_corr 
//need timestamp

#define  TIMEOUT 2

int main(int argc, char *argv[]) 
{
        int sockfd;
        struct sockaddr_in si_send;
        srand(time(NULL));

        double corrupt_prob;
        double loss_prob; 
        FILE *outputFile = NULL;

        if (argc < 6) {
                fprintf(stderr, "Usage: %s server_hostname server_port_number filename prob_loss prob_corruption\n", argv[0]);
                exit(0);
        }

        char *sender_hostname = argv[1];
        int sender_portnumber = atoi(argv[2]);
        char *filename = argv[3];
        loss_prob = atof(argv[4]);
        corrupt_prob = atof(argv[5]);

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
        printf("File name: %s\n", out);
        outputFile = fopen(out, "w");
        free(out);

        char pkt[PACKET_SIZE];
        socklen_t slen = sizeof(si_send);
        int last_seq_no = -1;
        rdt_packet last_seqpacket;

        //send initial request packet
        //content length sizeof filename, ACK#, SEQ# are 0 because it is just starting
        rdt_packet request_packet;
        request_packet.copyToData(filename, strlen(filename)+1); 
        
        printf("Request string: %s\n", request_packet.packetStr());
        //create a string from the packet and send it
        char *strp = request_packet.packetStr(); 
        sendto(sockfd, strp, PACKET_SIZE, 0, (struct sockaddr*)&si_send, sizeof(si_send));
        free(strp);
        strp = NULL;
        time_t timer = -1;

        fcntl(sockfd, F_SETFL, O_NONBLOCK);
        rdt_packet last_fin;
        int state = 0;
        while (1) {
                //if timeout resend last finack
                if (timer != -1 && time(NULL) >= timer + TIMEOUT && state == 1) {
                        printf("Sender: (FINACK lost or corrupted) Timeout\n");
                        rdt_packet ack_packet(rdt_packet::TYPE_ACK, last_fin.getACK(), last_fin.getSeqNo() + last_fin.getContentLength() + last_fin.isFin(), 0, last_fin.isFin());
                        printf("Receiver: FINACK sent seq#%d, ACK#%d, FIN %d, content-length: %d\n", ack_packet.getSeqNo(), ack_packet.getACK(), ack_packet.isFin(), ack_packet.getContentLength());
                        sendto(sockfd, ack_packet.packetStr(), PACKET_SIZE, 0, (struct sockaddr*)&si_send, sizeof(si_send));
                        timer = time(NULL); //reset timer
                }
                if (recvfrom(sockfd, pkt, PACKET_SIZE, 0, (struct sockaddr*) &si_send, &slen) > 0) {
                        //get packet from response
                        rdt_packet p(pkt);
                        double r = random_num();
                        double r2 = random_num();
                        if (r < corrupt_prob) {
                                if (last_seq_no != -1) {
                                        //if packet is corrupted resend ACK for last received sequence number
                                        printf("Receiver: Packet was corrupted!\n");
                                        rdt_packet ack_packet(rdt_packet::TYPE_ACK, last_seqpacket.getACK(), last_seqpacket.getSeqNo()+last_seqpacket.getContentLength()+last_seqpacket.isFin(), 0, last_seqpacket.isFin());
                                        strp = ack_packet.packetStr();
                                        printf("Receiver: ACK sent seq#%d, ACK#%d, FIN %d, content-length: %d\n", ack_packet.getSeqNo(), ack_packet.getACK(), ack_packet.isFin(), ack_packet.getContentLength());
                                        sendto(sockfd, strp, PACKET_SIZE, 0, (struct sockaddr*)&si_send, sizeof(si_send));
                                        free(strp);
                                        strp = NULL;
                                }
                                continue;
                        }
                        if (r2 < loss_prob) { //if packet is lost, drop it
                                printf("Receiver: Packet was lost!\n");
                                continue;
                        }

                        if (p.isFin()) { //if FIN
                                if (state == 0) { //if just received fin, send finack in response
                                        printf("Receiver: DATA received seq#%d, ACK#%d, FIN %d, content-length: %d\n", p.getSeqNo(), p.getACK(), p.isFin(), p.getContentLength());
                                        rdt_packet ack_packet(rdt_packet::TYPE_ACK, p.getACK(), p.getSeqNo()+p.getContentLength()+p.isFin(), 0, p.isFin());
                                        strp = ack_packet.packetStr();
                                        last_fin = p;
                                        printf("Receiver: FINACK sent seq#%d, ACK#%d, FIN %d, content-length: %d\n", ack_packet.getSeqNo(), ack_packet.getACK(), ack_packet.isFin(), ack_packet.getContentLength());
                                        sendto(sockfd, strp, PACKET_SIZE, 0, (struct sockaddr*)&si_send, sizeof(si_send));
                                        free(strp);
                                        strp = NULL;

                                        timer = time(NULL); //set timer
                                        state = 1; //go to next state
                                } else if (state == 1) { //if received finack to the finack, close the connection
                                        printf("Receiver: FINACK received seq#%d, ACK#%d, FIN %d, content-length: %d\n", p.getSeqNo(), p.getACK(), p.isFin(), p.getContentLength());
                                        printf("Receiver: close connection\n");
                                        break;
                                }
                        } else if (p.getType() == rdt_packet::TYPE_DATA && p.getSeqNo() == last_seqpacket.getSeqNo() + last_seqpacket.getContentLength()) { //received in order packet 
                                printf("Receiver: DATA received seq#%d, ACK#%d, FIN %d, content-length: %d\n", p.getSeqNo(), p.getACK(), p.isFin(), p.getContentLength());
                                last_seqpacket = p;
                                last_seq_no = p.getSeqNo();
                                //send ACK of the received packet
                                rdt_packet ack_packet(rdt_packet::TYPE_ACK, p.getACK(), p.getSeqNo()+p.getContentLength()+p.isFin(), 0, p.isFin());
                                strp = ack_packet.packetStr();
                                printf("Receiver: ACK sent seq#%d, ACK#%d, FIN %d, content-length: %d\n", ack_packet.getSeqNo(), ack_packet.getACK(), ack_packet.isFin(), ack_packet.getContentLength());
                                sendto(sockfd, strp, PACKET_SIZE, 0, (struct sockaddr*)&si_send, sizeof(si_send));
                                free(strp);
                                strp = NULL;

                                for (int i = 0; i < p.getContentLength(); i++) {
                                        char byte = *(p.getData()+i);
                                        fprintf(outputFile, "%c", byte);
                                        fflush(stdout);
                                }
                        } else if (p.getType() == rdt_packet::TYPE_DATA) { //if packet is out of order resend ack for last received
                                if (last_seq_no != -1) {
                                        printf("Receiver: Packet %d was out of order\n", p.getSeqNo());
                                        //send ACK of the last properly received sequence number
                                        rdt_packet ack_packet(rdt_packet::TYPE_ACK, last_seqpacket.getACK(), last_seqpacket.getSeqNo()+last_seqpacket.getContentLength()+last_seqpacket.isFin(), 0, last_seqpacket.isFin());
                                        strp = ack_packet.packetStr();
                                        printf("Receiver: ACK sent seq#%d, ACK#%d, FIN %d, content-length: %d\n", ack_packet.getSeqNo(), ack_packet.getACK(), ack_packet.isFin(), ack_packet.getContentLength());
                                        sendto(sockfd, strp, PACKET_SIZE, 0, (struct sockaddr*)&si_send, sizeof(si_send));
                                        free(strp);
                                        strp = NULL;
                                }
                        } 
                }
        }
        close(sockfd);
        fclose(outputFile);
}
