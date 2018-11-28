#include <iostream>

#include "CEvent.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <poll.h>
#include <sys/epoll.h>
#include <sys/time.h>
#include <netinet/in.h>

#define MYPORT 12345

using namespace std;

int initSock(int *sock){
    struct sockaddr_in server_addr;

    //socket
    if((*sock=socket(AF_INET, SOCK_STREAM, 0)) < 0)
    {
        printf("socket error\n");
        return -1;
    }

    bzero(&server_addr, sizeof(server_addr));
    server_addr.sin_family  =  AF_INET;
    server_addr.sin_port = htons(MYPORT);
    server_addr.sin_addr.s_addr  =  htonl(INADDR_ANY);

    //bind
    if(bind(*sock, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0)
    {
        printf("bind error\n");
        return -1;
    }

    //listen
    if(listen(*sock, 5) < 0)
    {
        printf("listen error\n");
        return -1;
    }
    return 0;
}


int main(int argc, char *argv[])
{
    int sock = 0;
    initSock(&sock);

    cout << "Hello World!" << endl;

    CEvent event(sock);
    event.Register_event(sock);
    while(1){
        sleep(1000);
    }
    return 0;
}
