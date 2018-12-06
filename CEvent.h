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
#include <map>
#include <iostream>

using namespace std;

#define MAX_SIZE 1024
#define EPOLL_SIZE 1024
#define MAX_BUFFER_SIZE 1024

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

class log{
public:
    log(const char *strLog){
        memset(m_log, sizeof(m_log), 0);
        strcpy(m_log, strLog);
    }
    ~log(){
        cout<<m_log<<endl;
    }
   private:
    char m_log[100];
};

class EpollEventBase//基类
{
public:
    EpollEventBase();
    virtual ~EpollEventBase();
    
    virtual int init();
    virtual void stop();

    virtual int register_event(int fd,  EpollEventType type);
    virtual int unregister_event(int fd);

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

class EpollEventBuffer{
public:
    EpollEventBuffer();
    ~EpollEventBuffer();
    int push(char *data, int len);
    void pop(char **pData, int *pLen);
    void rest();
    void setfd(int fd);
    int getfd();
private:
    int m_length;
    char* m_buffer;
    int m_fd;
};

class EpollEventAgent : public EpollEventBase//监听端数据接收处理
{
public:
    EpollEventAgent();
    ~EpollEventAgent();

    virtual int register_event(int fd,  EpollEventType type);
    virtual int unregister_event(int fd);
    virtual void stop();

private:
    void disconnect_all();

private:
    virtual void event_loop();
    std::map<int, EpollEventBuffer*> m_agentMap;
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
