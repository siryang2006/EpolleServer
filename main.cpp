#include <iostream>

#include "CEvent.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>


#define MYPORT 12345

using namespace std;


int main(int argc, char *argv[])
{
	EpollEventListener epollEventListener;
    epollEventListener.start(1111);
    while(1){
        sleep(1000);
    }
    return 0;
}
