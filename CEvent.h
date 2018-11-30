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

enum EpollEventType
{
    EIN = EPOLLIN,		  // 读事件
    EOUT = EPOLLOUT,	  // 写事件
    ECLOSE = EPOLLRDHUP,  // 对端关闭连接或者写半部
    EPRI = EPOLLPRI,	  // 紧急数据到达
    EERR = EPOLLERR,	  // 错误事件
    EET = EPOLLET, 		  // 边缘触发
    EDEFULT = EIN | ECLOSE | EERR | EET
};


class EpollEventBase//基类
{
public:
    EpollEventBase();
    virtual ~EpollEventBase();
    
    virtual int init();
    virtual void stop();

    int register_event(int fd,  EpollEventType type = EDEFULT);
    int unregister_event(int fd);

protected:
    void setnoblocking(int v_sockfd);
    virtual void event_loop() = 0;

private:
    static void* eventHandle(void *arg);

protected:
    int m_epfd;
    bool m_is_running;
    struct epoll_event m_events[EPOLL_SIZE];

    pthread_t  m_tid;

};

class EpollEventAgent : public EpollEventBase//监听端数据接收处理
{
public:
    EpollEventAgent();
    ~EpollEventAgent();

private:
    virtual void event_loop();
};

class EpollEventListener : public EpollEventBase//监听端
{
public:
    EpollEventListener();
    ~EpollEventListener();

    int start(int port);

    int init();

    virtual void event_loop();

    virtual void stop();

private:
    int accapt_event();
    int set_socket_keepalive(int listenfd);

private:
    int m_listen_sock;
    int m_port;
    EpollEventAgent m_epoll_event_agent_array[2];
};

#endif
