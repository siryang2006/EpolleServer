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


class EventBuffer{
public:
    EventBuffer() {
        reset();
    }

    void reset(){
        pack_length = real_length = 0;
        buffer = (char*)malloc(head_length+1);
        buffer[head_length] = '\0';
    }

    ~EventBuffer() {
        pack_length = real_length = 0;
        if(buffer){
            free(buffer);
            buffer = NULL;
        }
    }

    int read_buffer(int fd, bool &finished){
        int read_length = 0;
        finished = false;
        if(pack_length == 0){
            read_length = read(fd, buffer+real_length, head_length-real_length);
            if(read_length<=0){
                return read_length;
            }

            real_length += read_length;
            if(real_length<head_length){
                return read_length;
            }

            pack_length = atoi(buffer)+head_length;
            buffer = (char*)realloc(buffer, pack_length+1);
            memset(buffer + head_length, pack_length-head_length, 0);
            buffer[pack_length+head_length] = '\0';
        }

        read_length = read(fd, buffer+real_length, pack_length-real_length);
        if(read_length<=0){
            return read_length;
        }

        real_length += read_length;
        if(real_length == pack_length){
            finished = true;
            static int index = 1;
            cout<<"got data "<<index<<":"<<buffer<<endl;
            index++;
        }

        return read_length;
    }

    int pack_length;
    int real_length;
    char* buffer;

    const int head_length = 4;
};

class EventData{
 public:
    EventData(){
        pEventBuffer = NULL;
    }

    ~EventData() {
        if (pEventBuffer != NULL) {
            delete pEventBuffer;
            pEventBuffer = NULL;
        }
    }

    int read_buffer(bool &finished){
        if(pEventBuffer == NULL){
            pEventBuffer = new EventBuffer();
        }
        return pEventBuffer->read_buffer(fd, finished);
    }

    void reset(){
        if(pEventBuffer != NULL){
            pEventBuffer->reset();
        }
    }

    int fd;
    EventBuffer *pEventBuffer;
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
    std::map<int, EventData*> m_agentMap;
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
