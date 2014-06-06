#include <sys/socket.h>
#include <sys/stat.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <fcntl.h>
#include <time.h>
#include "rdt_packet.h"
#include "rdt_window.h"
#include <stdio.h>

//#define WINDOW_SIZE 5
#define TIMEOUT 2
#define TIME_WAIT 6

int main(int argc, char *argv[]) 
{
        int socketfd = 0;
        struct sockaddr_in serv_addr, cli_addr;
        int clilen, portno, base, next_seq_num;
        int window_size;
        double corrupt_prob, loss_prob;
        srand(time(NULL));

        time_t timer = -1;

        if (argc != 5) {
                fprintf(stderr, "Usage: %s port_number congestion_window_size prob_loss prob_corruption\n", argv[0]);
                exit(EXIT_FAILURE);
        }
        portno = atoi(argv[1]); //port number 
        window_size = atoi(argv[2]); 
        loss_prob = atof(argv[3]); //probability of loss
        corrupt_prob = atof(argv[4]); //probability of corruption

        socketfd = socket(AF_INET, SOCK_DGRAM, 0);

        if (socketfd < 0) perror("Error opening socket!");

        memset(&serv_addr, '0', sizeof(serv_addr));
        serv_addr.sin_family = AF_INET;
        serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
        serv_addr.sin_port = htons(portno);

        if (bind(socketfd, (struct sockaddr*)&serv_addr, sizeof(serv_addr)) < 0)
                perror("Error binding!");

        clilen = sizeof(cli_addr);
        char req_string[DATA_SIZE];
        rdt_window w(window_size);

        printf("Sender: waiting for file request\n");
        if (recvfrom(socketfd, &req_string, DATA_SIZE, 0, (sockaddr*)&cli_addr, (socklen_t*)&clilen) < 0)
                perror("ERROR receiving from client");
        rdt_packet req_packet(req_string);

        printf("Sender: DATA received seq#%d, ACK#%d, FIN %d, content-length: %d\n", req_packet.getSeqNo(), req_packet.getACK(), req_packet.isFin(), req_packet.getContentLength());
        printf("Sender: File requested is \"%s\"\n", req_packet.getData());
        printf("..................................\n");

        base = 1;
        next_seq_num = 1;
        w.clear();
        w.handleRequest(req_packet);

        vector<char*> result = w.fillWindow();
        printf("----\n");
        for (size_t i = 0; i < result.size(); i++) {
                rdt_packet p(result[i]);
                printf("Sender: DATA sent seq#%d, ACK#%d, FIN %d, content-length: %d\n", p.getSeqNo(), p.getACK(), p.isFin(), p.getContentLength());
                sendto(socketfd, result[i], PACKET_SIZE, 0, (struct sockaddr*) &cli_addr, sizeof(cli_addr)); 
                if (i == 0) {
                        timer = time(NULL);
                }
        }

        fcntl(socketfd, F_SETFL, O_NONBLOCK);
        int state = 0;
        rdt_packet last_recv_finack;
        while (1) {
                if (timer != -1 && time(NULL) >= timer + TIMEOUT && state != 2) {
                        printf("Sender: (ACK lost or corrupted) Timeout\n");
                        resendAllPackets(w, socketfd, cli_addr);
                        timer = time(NULL);
                } else if (state == 2 && time(NULL) >= timer + TIME_WAIT) { //if the wait time is up and client didn't resend then we close
                        printf("Sender: connection close\n");
                        break;
                }
                //receive stuff
                if (recvfrom(socketfd, &req_string, DATA_SIZE, 0, (struct sockaddr*)&cli_addr, (socklen_t*)&clilen) > 0) {
                        rdt_packet recv_ack_packet(req_string); 
                        double r = random_num();
                        double r2 = random_num();
                        if (r < corrupt_prob) {
                                printf("Sender: Packet was corrupted!\n");
                                continue;
                        } 
                        if (r2 < loss_prob) {
                                printf("Sender: Packet was lost!\n");
                                continue;
                        }
                        printf("Sender: ACK received seq#%d, ACK#%d, FIN %d, content-length: %d\n", recv_ack_packet.getSeqNo(), recv_ack_packet.getACK(), recv_ack_packet.isFin(), recv_ack_packet.getContentLength());
                        //if the window slid, reset timer
                        bool window_slid = false;
                        if ((window_slid = w.handleACK(recv_ack_packet))) timer = time(NULL);
                        
                        if (w.getCurrSize() <= 0) { //if the new current size is now zero, save the last ack
                                if (state == 0) { //if state is 0 that means you just received the ack for the last real data packet
                                        printf("Sender: file transfer complete\n");
                                        //time to send the fin packet
                                        rdt_packet fin_packet(rdt_packet::TYPE_DATA, recv_ack_packet.getACK(), recv_ack_packet.getSeqNo(), 0, 1);
                                        w.add_packet(fin_packet);
                                        printf("Sender: DATA sent seq#%d, ACK#%d, FIN %d, content-length: %d\n", fin_packet.getSeqNo(), fin_packet.getACK(), fin_packet.isFin(), fin_packet.getContentLength());
                                        sendto(socketfd, fin_packet.packetStr(), PACKET_SIZE, 0, (struct sockaddr*) &cli_addr, sizeof(cli_addr));
                                        state = 1;
                                } else if (state == 1 && window_slid) { //if your are waiting for the ack of the fin packet and the window slid
                                        last_recv_finack = recv_ack_packet;
                                        rdt_packet fin_packet(rdt_packet::TYPE_ACK, recv_ack_packet.getACK(), recv_ack_packet.getSeqNo() + recv_ack_packet.isFin(), 0, 1);
                                        printf("Sender: FINACK sent seq#%d, ACK#%d, FIN %d, content-length: %d\n", fin_packet.getSeqNo(), fin_packet.getACK(), fin_packet.isFin(), fin_packet.getContentLength());
                                        sendto(socketfd, fin_packet.packetStr(), PACKET_SIZE, 0, (struct sockaddr*) &cli_addr, sizeof(cli_addr));
                                        state = 2;
                                } else if (state == 2 && last_recv_finack.getACK() == recv_ack_packet.getACK()) { //if the client is resending the old finack
                                        //then it didn't get the finack we sent
                                        //so resend finack
                                        rdt_packet fin_packet(rdt_packet::TYPE_ACK, recv_ack_packet.getACK(), recv_ack_packet.getSeqNo() + recv_ack_packet.isFin(), 0, 1);
                                        printf("Sender: FINACK sent seq#%d, ACK#%d, FIN %d, content-length: %d\n", fin_packet.getSeqNo(), fin_packet.getACK(), fin_packet.isFin(), fin_packet.getContentLength());
                                        sendto(socketfd, fin_packet.packetStr(), PACKET_SIZE, 0, (struct sockaddr*) &cli_addr, sizeof(cli_addr));
                                }
                        } else if (w.getCurrSize() != w.getWindowSize()) {
                                vector<char*> new_result = w.fillWindow();

                                for (size_t i = 0; i < new_result.size(); i++) {
                                        rdt_packet p(new_result[i]);
                                        printf("Sender: DATA sent seq#%d, ACK#%d, FIN %d, content-length: %d\n", p.getSeqNo(), p.getACK(), p.isFin(), p.getContentLength());
                                        sendto(socketfd, new_result[i], PACKET_SIZE, 0, (struct sockaddr*) &cli_addr, sizeof(cli_addr)); 
                                }
                        }
                }
        }
}
