#include "rdt_packet.h"
#include <map>
#include <vector>
using std::vector;
using std::map;

class rdt_window
{
        private:
                static map<rdt_packet*, int> dependencies;
                rdt_packet **packList; //list of pointers to packets
                int window_size;
                int currSize;
                int curr_seq_no;
                int totalPackets;
                FILE* resource;
                rdt_packet last_rdt_packet;

                bool deletePacket(rdt_packet *p);
        public:
                vector<char*> fillWindow();
                int getTotalPackets() {return totalPackets; }
                int getWindowSize() { return window_size; }
                int getCurrSize() { return currSize; }
                bool getPacket(int index, rdt_packet &p);

                rdt_window(int window_size);
                rdt_window(const rdt_window& win);
                rdt_window operator=(const rdt_window& win);
                ~rdt_window();

                void slide_window();
                void clear();
                bool add_packet(rdt_packet p);

                bool handleACK(rdt_packet ackpacket);
                void handleRequest(rdt_packet &reqpacket);
};

void resendAllPackets(rdt_window &w, int sockfd, struct sockaddr_in addr);
