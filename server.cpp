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

//#define WINDOW_SIZE 5
#define TIMEOUT 5

int main(int argc, char *argv[]) 
{
        int socketfd = 0;
        struct sockaddr_in serv_addr, cli_addr;
        int clilen, portno, base, next_seq_num;
        int window_size;
        double corrupt_prob, loss_prob;

        time_t timer;

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
        }

        fcntl(socketfd, F_SETFL, O_NONBLOCK);

        while (w.getTotalPackets() > 0 || w.getCurrSize() > 0) {
                if (time(NULL) >= timer + TIMEOUT) {
                        printf("Sender: (ACK lost or corrupted) Timeout\n");
                        resendAllPackets(w, socketfd, cli_addr);
                        timer = time(NULL);
                } 
                //receive stuff
                if (recvfrom(socketfd, &req_string, DATA_SIZE, 0, (struct sockaddr*)&cli_addr, (socklen_t*)&clilen) > 0) {
                        rdt_packet new_req_packet(req_string); 
                        printf("Sender: ACK received seq#%d, ACK#%d, FIN %d, content-length: %d\n", new_req_packet.getSeqNo(), new_req_packet.getACK(), new_req_packet.isFin(), new_req_packet.getContentLength());
                        //if the window slid, reset timer
                        if (w.handleACK(new_req_packet)) timer = time(NULL);
                        
                        //if there is empty spaces
                        if (w.getCurrSize() != w.getWindowSize()) {
                                //fillWindow(w, socketfd, cli_addr);
                                vector<char*> new_result = w.fillWindow();

                                for (size_t i = 0; i < new_result.size(); i++) {
                                        rdt_packet p(new_result[i]);
                                        printf("Sender: DATA sent seq#%d, ACK#%d, FIN %d, content-length: %d\n", p.getSeqNo(), p.getACK(), p.isFin(), p.getContentLength());
                                        sendto(socketfd, new_result[i], PACKET_SIZE, 0, (struct sockaddr*) &cli_addr, sizeof(cli_addr)); 
                                }
                        }
                }
        }
        printf("Sender: file transfer complete\n");
}
