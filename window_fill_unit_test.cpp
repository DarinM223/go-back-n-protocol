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
        rdt_window w(2);
        rdt_packet req_packet;
        req_packet.copyToData("Hello.txt", 10);

        cout << "Request packet: Seq #: " << req_packet.getSeqNo() << " ACK #: " << req_packet.getACK()
                << " Content length: " << req_packet.getContentLength() << endl;

        w.handleRequest(req_packet);

        printf("Total packets: %d\n", w.getTotalPackets());

        vector<char*> result = w.fillWindow();

        printWindow(w);

        for (size_t i = 0; i < result.size(); i++) {
                rdt_packet p(result[i]);
                char data[DATA_SIZE+1];
                strcpy(data, p.getData());
                data[p.getContentLength()] = '\0';
                cout << p.getSeqNo() << endl << data << endl;
        }

        rdt_packet ackpacket(rdt_packet::TYPE_ACK, 0, 2048, 0, 0);

        //test handling ACK
        w.handleACK(ackpacket);

        printWindow(w);

        vector<char*> result2 = w.fillWindow();

        printWindow(w);

        for (size_t i = 0; i < result2.size(); i++) {
                rdt_packet p(result2[i]);
                char data[DATA_SIZE+1];
                strcpy(data, p.getData());
                data[p.getContentLength()] = '\0';
                cout << p.getSeqNo() << endl << data << endl;
        }

        printf("Total packets: %d\n", w.getTotalPackets());
}
