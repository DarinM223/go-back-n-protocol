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
#include <time.h>
#include <fcntl.h>
#include "packet.h"
#include <string>
using std::string;

#define WINDOW_SIZE 5
#define TIMEOUT 5000

int main(int argc, char *argv[]) 
{
        int socketfd = 0, mode = 0;
        struct sockaddr_in serv_addr, cli_addr;
        int pid, clilen, portno, n, base, next_seq_num;

        if (argc != 4) {
                fprintf(stderr, "TODO: Print out correct output format %s\n", argv[0]);
                exit(EXIT_FAILURE);
        }
        //port number is first argument
        portno = atoi(argv[1]);

}
