#include "window.h"
#include <malloc.h>

window::window(int window_size) {
        this->window_size = window_size;
        packList = new packet* [window_size];
        currSize = 0;
}
window::window(const window& win) {
        this->packList = new packet* [win.window_size];
        for (int i = 0; i < win.window_size; i++) {
                this->packList[i] = win.packList[i];
                dependencies[packList[i]]++;
        }
        this->window_size = win.window_size;
        this->currSize = win.currSize;
}
window window::operator=(const window& win) {
        this->packList = new packet* [win.window_size];
        for (int i = 0; i < win.window_size; i++) {
                this->packList[i] = win.packList[i];
                dependencies[packList[i]]++;
        }
        this->window_size = win.window_size;
        this->currSize = win.currSize;
        return *this;
}

void window::slide_window() {
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

bool window::deletePacket(packet *p) {
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

bool window::add_packet(packet p) {
        if (currSize >= window_size) return false;
        packet *new_packet = new packet(p);
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

bool window::getPacket(int index, packet &p) {
        if (!(index >= 0 && index < currSize)) return false;
        packet *ptr = packList[index];

        //if the dependency for the packet pointer doesn't exist or is 0 somebody deleted it already
        if (dependencies.find(ptr) == dependencies.end()) return false;
        if (dependencies[ptr] <= 0) return false;

        p = *ptr; //operator = hasn't been defined, but I think its ok because copying is ok
        return true;
}

bool window::handleACK(packet ackpacket) {
        for (int i = 0; i < currSize; i++) {
                packet* p = packList[i];
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

window::~window() {
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
map<packet*, int> window::dependencies;
