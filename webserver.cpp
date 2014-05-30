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

int TOTAL_PACKETS = 0;
FILE *RESOURCE = NULL;
rdt_packet LAST_RECV_PACKET;

void fillWindow(rdt_window &w) {
        //get the sequence number to send from the ACK number of the last sent packet
        int curr_seq_no = LAST_RECV_PACKET.getACK(); 
        //for all empty slots in window and while TOTAL_PACKETS is greater than 0
        for (int i = w.getCurrSize(); (i < w.getWindowSize() && TOTAL_PACKETS > 0); i++) {
                //create new packet and send them
                rdt_packet respond_packet(rdt_packet::TYPE_DATA, curr_seq_no, LAST_RECV_PACKET.getSeqNo() + LAST_RECV_PACKET.getContentLength(), 0, 0);
                respond_packet.readFromFile(RESOURCE);

                w.add_packet(respond_packet);
                //decrease total number of packets
                TOTAL_PACKETS--;
                //increase the current sequence number by data length
                curr_seq_no += respond_packet.getContentLength();
        }
}

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
                if (recvfrom(socketfd, &req_string, sizeof(req_string), 0, (sockaddr*)&cli_addr, (socklen_t*)&clilen) < 0)
                        perror("ERROR receiving from client");
                rdt_packet req_packet(req_string);
                LAST_RECV_PACKET = req_packet;
                base = 1;
                next_seq_num = 1;
                w.clear();

                RESOURCE = fopen(req_packet.getData(), "rb");
                if (RESOURCE == NULL) {
                        perror("ERROR opening file!");
                }

                stat(req_packet.getData(), &st);

                TOTAL_PACKETS = st.st_size / DATA_SIZE;

                //if there is a remainder, add another packet
                if (st.st_size % DATA_SIZE) 
                        TOTAL_PACKETS++;
                printf("Total packets: %d\n", TOTAL_PACKETS);

                //right now ack # should be 0 so the sequence numbers should be like: 0, 1024, 2048, ....
                fillWindow(w);

                while (TOTAL_PACKETS > 0) {

                }
        }
}
