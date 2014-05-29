#include "rdt_window.h"
#include <malloc.h>

rdt_window::rdt_window(int window_size) {
        this->window_size = window_size;
        packList = new rdt_packet* [window_size];
        currSize = 0;
}
rdt_window::rdt_window(const rdt_window& win) {
        this->packList = new rdt_packet* [win.window_size];
        for (int i = 0; i < win.window_size; i++) {
                this->packList[i] = win.packList[i];
                dependencies[packList[i]]++;
        }
        this->window_size = win.window_size;
        this->currSize = win.currSize;
}
rdt_window rdt_window::operator=(const rdt_window& win) {
        this->packList = new rdt_packet* [win.window_size];
        for (int i = 0; i < win.window_size; i++) {
                this->packList[i] = win.packList[i];
                dependencies[packList[i]]++;
        }
        this->window_size = win.window_size;
        this->currSize = win.currSize;
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
                        return true;
                }
        }
        //there was no ack that matched the packets
        return false;
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
