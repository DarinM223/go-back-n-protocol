#include <iostream>
#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <malloc.h>
#include "packet.h"
using namespace std;

int main() 
{
        //TODO: Unit test constructors
        char *filename = "Hello.txt";
        packet p(TYPE_MESSAGE, 20, 20, sizeof(filename)+HEADER_SIZE, false);
        p.copyToData(filename, strlen(filename)+1);
        packet p2(p.packetStr());
        packet p3(p.packetStr());

        assert(p.getLength() == p2.getLength());
        assert(p.getSeqNo() == p2.getSeqNo());
        assert(strcmp(p.getData(), p2.getData()) == 0);

        FILE* resource = fopen(p2.getData(), "rb");
        p3.readFromFile(resource);
        char *str = (char*)malloc(p3.getLength()-HEADER_SIZE);
        strcpy(str, "");
        memcpy(str, p3.getData(), p3.getLength()-HEADER_SIZE);
        cout << str << " " << strlen(str) << endl;

        fclose(resource);
        free(str);
        cout << "All test passed!" << endl;
        return 0;
}

