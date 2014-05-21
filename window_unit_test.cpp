#include "window.h"
#include <iostream>
#include <assert.h>
using namespace std;

void printWindow(window w) {
        for (int i = 0; i < w.getWindowSize(); i++) {
                packet p;
                if (w.getPacket(i, p)) {
                        cout << "Packet sequence no: " << p.getSeqNo() << " ACK no: " << p.getACK() << " FIN: " << p.isFin() << " Length: " << p.getLength() << " DATA: " << p.getData() << endl;
                }
        }
        cout << endl;
}

void printWindow2(window w) {
        printWindow(w);
}
void printWindow3(window w) {
        printWindow2(w);
        printWindow(w);
}

int main() 
{
        window w(4);
        packet p1(TYPE_MESSAGE, 0, 0, 20, 0);
        packet p2(TYPE_ACK, 10, 10, 50, 0);
        packet p3(TYPE_END, 101, 100, 4, 1);
        packet p4(TYPE_REQUEST, 0, 0, 4, 0);
        packet p5(TYPE_MESSAGE, 5, 10, 5, 0);

        assert(w.add_packet(p1));
        assert(w.add_packet(p2));
        assert(w.add_packet(p3));
        assert(w.add_packet(p4));
        assert(!w.add_packet(p5));

        window w2(5);
        assert(w2.add_packet(p1));
        assert(w2.add_packet(p2));
        assert(w2.add_packet(p3));
        assert(w2.add_packet(p4));
        assert(w2.add_packet(p5));

        printWindow3(w);
        w.slide_window();
        w.slide_window();
        printWindow2(w);
        w2.slide_window();
        w2.slide_window();
        printWindow2(w2);

        cout << "All test passed!" << endl;

        return 0;
}

