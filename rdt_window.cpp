#include "rdt_window.h"
#include <malloc.h>
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

void resendAllPackets(rdt_window &w, int sockfd, struct sockaddr_in addr) {
        for (int i = 0; i < w.getCurrSize(); i++) {
                rdt_packet p;
                w.getPacket(i, p);
                
                char *strp = p.packetStr();
                sendto(sockfd, strp, PACKET_SIZE, 0, (struct sockaddr*)&addr, sizeof(addr));
        } 
}

rdt_window::rdt_window(int window_size) {
        this->window_size = window_size;
        packList = new rdt_packet* [window_size];
        currSize = 0;
        this->totalPackets = 0;
        this->resource = NULL;
        this->curr_seq_no = 0;
}
rdt_window::rdt_window(const rdt_window& win) {
        this->packList = new rdt_packet* [win.window_size];
        for (int i = 0; i < win.window_size; i++) {
                this->packList[i] = win.packList[i];
                dependencies[packList[i]]++;
        }
        this->window_size = win.window_size;
        this->currSize = win.currSize;
        this->totalPackets = 0;
        this->resource = NULL;
        this->curr_seq_no = win.curr_seq_no;
}
rdt_window rdt_window::operator=(const rdt_window& win) {
        this->packList = new rdt_packet* [win.window_size];
        for (int i = 0; i < win.window_size; i++) {
                this->packList[i] = win.packList[i];
                dependencies[packList[i]]++;
        }
        this->window_size = win.window_size;
        this->currSize = win.currSize;
        this->totalPackets = 0;
        this->resource = NULL;
        this->curr_seq_no = win.curr_seq_no;
        return *this;
}

void rdt_window::slide_window() {
        if (window_size <= 1) return;
        for (int i = 1; i < window_size; i++) {
                if (packList[i-1]) {
                        deletePacket(packList[i-1]);
                }
                packList[i-1] = packList[i];
                packList[i] = NULL;
        }
        currSize--;
}

vector<char*> rdt_window::fillWindow() {
        vector<char*> packVec;
        //for all empty slots in window and while TOTAL_PACKETS is greater than 0
        for (int i = getCurrSize(); (i < getWindowSize() && this->totalPackets > 0); i++) {
                //create new packet and send them
                rdt_packet respond_packet(rdt_packet::TYPE_DATA, curr_seq_no, this->last_rdt_packet.getSeqNo() + this->last_rdt_packet.getContentLength(), 0, 0);
                respond_packet.readFromFile(this->resource);

                add_packet(respond_packet);
                packVec.push_back(respond_packet.packetStr());
                
                //decrease total number of packets
                this->totalPackets--;
                //increase the current sequence number by data length
                curr_seq_no += respond_packet.getContentLength();
        }
        return packVec;
}

void rdt_window::clear() {
        for (int i = 0; i < window_size; i++) {
                if (packList[i]) {
                        deletePacket(packList[i]);
                        packList[i] = NULL;
                }
        }
        currSize = 0;
}


bool rdt_window::deletePacket(rdt_packet *p) {
        //return false if you can't find the packet in the window
        if (dependencies.find(p) == dependencies.end()) {
                return false;
        }        
        //if the dependency is already 0, its been deleted already
        if (dependencies[p] <= 0) {
                return false;
        } 
        //decrease the count of that packet by 1
        dependencies[p]--;
        //if the count is now zero (there is no other class using that pointer)
        if (dependencies[p] == 0) {
                delete p;
        }
        return true;
}

bool rdt_window::add_packet(rdt_packet p) {
        if (currSize >= window_size) return false;
        rdt_packet *new_packet = new rdt_packet(p);
        packList[currSize] = new_packet;
        currSize++;

        //if dependency already exists, increment count, otherwise set count to 1
        if (dependencies.find(new_packet) != dependencies.end()) {
                dependencies[new_packet]++;
        } else {
                dependencies[new_packet] = 1;
        }
        return true;
}

bool rdt_window::getPacket(int index, rdt_packet &p) {
        if (!(index >= 0 && index < currSize)) return false;
        rdt_packet *ptr = packList[index];

        //if the dependency for the packet pointer doesn't exist or is 0 somebody deleted it already
        if (dependencies.find(ptr) == dependencies.end()) return false;
        if (dependencies[ptr] <= 0) return false;

        p = *ptr; //operator = hasn't been defined, but I think its ok because copying is ok
        return true;
}

bool rdt_window::handleACK(rdt_packet ackpacket) {
        for (int i = 0; i < currSize; i++) {
                rdt_packet* p = packList[i];
                //once you find a matching packet, slide window for every element before and including the matching packet
                if (p->properACKForPacket(ackpacket)) {
                        for (int j = 0; j <= i; j++) {
                                slide_window();
                        }
                        //this->last_rdt_packet = ackpacket;
                        return true;
                }
        }
        //there was no ack that matched the packets
        return false;
}

//initializes everything from a request packet
void rdt_window::handleRequest(rdt_packet &reqpacket) {
        this->last_rdt_packet = reqpacket;
        this->curr_seq_no = reqpacket.getACK();
        this->resource = fopen(reqpacket.getData(), "rb");

        if (this->resource == NULL) {
                perror("ERROR opening file!");
        }
        struct stat st; 
        stat(reqpacket.getData(), &st);

        this->totalPackets = st.st_size / DATA_SIZE;

        //if there is a remainder, add another packet
        if (st.st_size % DATA_SIZE) 
                this->totalPackets++;
}

rdt_window::~rdt_window() {
        for (int i = 0; i < window_size; i++) {
                if (packList[i]) {
                        deletePacket(packList[i]);
                }
        }
        if (packList) {
                delete [] packList;
                packList = NULL;
        }
}
map<rdt_packet*, int> rdt_window::dependencies;
