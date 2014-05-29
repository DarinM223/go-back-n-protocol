#include "rdt_packet.h"
#include <map>
using std::map;

class rdt_window
{
        private:
                static map<rdt_packet*, int> dependencies;
                rdt_packet **packList; //list of pointers to packets
                int window_size;
                int currSize;
                bool deletePacket(rdt_packet *p);
        public:
                int getWindowSize() { return window_size; }
                int getCurrSize() { return currSize; }
                bool getPacket(int index, rdt_packet &p);
                rdt_window(int window_size);
                rdt_window(const rdt_window& win);
                rdt_window operator=(const rdt_window& win);
                void slide_window();
                void clear();
                bool add_packet(rdt_packet p);
                ~rdt_window();
                bool handleACK(rdt_packet ackpacket);
};
