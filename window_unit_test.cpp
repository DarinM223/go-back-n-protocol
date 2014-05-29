#include "rdt_window.h"
#include <iostream>
#include <assert.h>
using namespace std;

void printWindow(rdt_window w) {
        for (int i = 0; i < w.getWindowSize(); i++) {
                rdt_packet p;
                if (w.getPacket(i, p)) {
                        cout << "Packet sequence no: " << p.getSeqNo() << " ACK no: " << p.getACK() << " FIN: " << p.isFin() << " Length: " << p.getLength() << " DATA: " << p.getData() << endl;
                }
        }
        cout << endl;
}

void printWindow2(rdt_window w) {
        printWindow(w);
}
void printWindow3(rdt_window w) {
        printWindow2(w);
        printWindow(w);
}

int main() 
{
        rdt_window w(4);
        rdt_packet p1(rdt_packet::TYPE_DATA, 0, 0, 20, 0);
        rdt_packet p2(rdt_packet::TYPE_ACK, 10, 10, 50, 0);
        rdt_packet p3(rdt_packet::TYPE_END, 101, 100, 4, 1);
        rdt_packet p4(rdt_packet::TYPE_REQUEST, 0, 0, 4, 0);
        rdt_packet p5(rdt_packet::TYPE_DATA, 5, 10, 5, 0);

        assert(w.add_packet(p1));
        assert(w.add_packet(p2));
        assert(w.add_packet(p3));
        assert(w.add_packet(p4));
        assert(!w.add_packet(p5));

        rdt_window w2(5);
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

        w2.clear();
        assert(w2.getCurrSize() == 0);
        printWindow2(w2);

        cout << "All test passed!" << endl;

        return 0;
}

