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
#include "rdt_packet.h"
#include "rdt_window.h"

#define WINDOW_SIZE 5
#define TIMEOUT 5000

int main(int argc, char *argv[]) 
{
        int socketfd = 0, mode = 0;
        struct sockaddr_in serv_addr, cli_addr;
        int pid, clilen, portno, n, base, next_seq_num;
        struct stat st;
        //FILE *resource; 
        time_t timer;

        if (argc != 4) {
                fprintf(stderr, "TODO: Print out correct output format %s\n", argv[0]);
                exit(EXIT_FAILURE);
        }
        //port number is first argument
        portno = atoi(argv[1]);

        socketfd = socket(AF_INET, SOCK_DGRAM, 0);

        if (socketfd < 0) perror("Error opening socket!");

        memset(&serv_addr, '0', sizeof(serv_addr));
        serv_addr.sin_family = AF_INET;
        serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
        serv_addr.sin_port = htons(portno);


        if (bind(socketfd, (struct socketaddr*)&serv_addr, sizeof(serv_addr)) < 0)
                perror("Error binding!");

        clilen = sizeof(cli_addr);
        char req_string[DATA_SIZE];
        rdt_window w(WINDOW_SIZE);

        while (1) {
                if (recvfrom(socketfd, &req_string, DATA_SIZE, 0, (sockaddr*)&cli_addr, (socklen_t*)&clilen) < 0)
                        perror("ERROR receiving from client");
                rdt_packet req_packet(req_string);
                base = 1;
                next_seq_num = 1;
                w.clear();
                w.handleRequest(req_packet);

                printf("Total packets: %d\n", w.getTotalPackets());

                //right now ack # should be 0 so the sequence numbers should be like: 0, 1024, 2048, ....
                vector<char*> result = w.fillWindow();
                for (size_t i = 0; i < result.size(); i++) {
                        sendto(socketfd, result[i], PACKET_SIZE, 0, (struct sockaddr*) &cli_addr, sizeof(cli_addr)); 
                }

                while (w.getTotalPackets() > 0) {
                        //boilerplate code for timer
                        bool timer = false;
                        if (timer) {
                                resendAllPackets(w, socketfd, cli_addr);
                        } 
                        //receive stuff
                        if (recvfrom(socketfd, &req_string, DATA_SIZE, 0, (struct sockaddr*)&cli_addr, (socklen_t*)&clilen) > 0) {
                                printf("Received ack!\n");
                                rdt_packet new_req_packet(req_string); 
                                w.handleACK(new_req_packet);
                                
                                //if there is empty spaces
                                if (w.getCurrSize() != w.getWindowSize()) {
                                        //fillWindow(w, socketfd, cli_addr);
                                        vector<char*> new_result = w.fillWindow();

                                        for (size_t i = 0; i < new_result.size(); i++) {
                                                sendto(socketfd, new_result[i], PACKET_SIZE, 0, (struct sockaddr*) &cli_addr, sizeof(cli_addr)); 
                                        }
                                }
                        }
                }
        }
}
