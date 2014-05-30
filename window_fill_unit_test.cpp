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
#include <iostream>
using namespace std;

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

void printWindow(rdt_window &w) {
        for (int i = 0; i < w.getWindowSize(); i++) {
                rdt_packet p;
                if (w.getPacket(i, p)) {
                        cout << "Packet sequence no: " << p.getSeqNo() << " ACK no: " << p.getACK() << " FIN: " << p.isFin() << " Content Length: " << p.getContentLength() << " DATA: " << p.getData() << endl;
                }
        }
        cout << endl;
}

int main() 
{
        struct stat st;
        rdt_window w(5);
        rdt_packet req_packet;
        req_packet.copyToData("Hello.txt", 10);

        cout << "Request packet: Seq #: " << req_packet.getSeqNo() << " ACK #: " << req_packet.getACK()
                << " Content length: " << req_packet.getContentLength() << endl;

        LAST_RECV_PACKET = req_packet;
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

        fillWindow(w);

        printWindow(w);

        printf("Total packets: %d\n", TOTAL_PACKETS);
}
