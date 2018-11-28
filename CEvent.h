#ifndef _CEVENT_H_
#define _CEVENT_H_


#include <sys/socket.h>
#include <sys/types.h>
#include <stdio.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/wait.h>
#include <sys/epoll.h>

#define MAX_SIZE 1024
#define EPOLL_SIZE 1024
#define BUFFER_SIZE 1024

enum EventType
{
    EIN = EPOLLIN,		  // 读事件
    EOUT = EPOLLOUT,	  // 写事件
    ECLOSE = EPOLLRDHUP,  // 对端关闭连接或者写半部
    EPRI = EPOLLPRI,	  // 紧急数据到达
    EERR = EPOLLERR,	  // 错误事件
    EET = EPOLLET, 		  // 边缘触发
    EDEFULT = EIN | ECLOSE | EERR | EET
};


class CEvent
{
public:
    CEvent();
    ~CEvent();
    int Register_event(int fd,  EventType type = EDEFULT);
    int unRegister_event(int fd);
    static void* EventHandle(void* arg);
    void SetNoblocking(int v_sockfd);

private:
    int epfd;
    bool is_running;
    pthread_t  m_tid;
    struct epoll_event events[EPOLL_SIZE];

    //CThreadPoolProxy *pool;
};

#endif
