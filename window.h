#include "packet.h"
#include <map>
using std::map;

class window
{
        private:
                static map<packet*, int> dependencies;
                packet **packList; //list of pointers to packets
                int window_size;
                int currSize;
                bool deletePacket(packet *p);
        public:
                int getWindowSize() { return window_size; }
                bool getPacket(int index, packet &p);
                window(int window_size);
                window(const window& win);
                window operator=(const window& win);
                void slide_window();
                bool add_packet(packet p);
                ~window();
                bool handleACK(packet ackpacket);
};
