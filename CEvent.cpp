#include "CEvent.h"
#include <pthread.h>
#include <iostream>

#include <sys/socket.h>
#include <poll.h>
#include <sys/epoll.h>
#include <sys/time.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <assert.h>

#define _LOG_FUNCION_()  log l(__PRETTY_FUNCTION__)

using namespace std;

EpollEventBase::EpollEventBase()
    :m_epfd(0)
{
    _LOG_FUNCION_();
}

int EpollEventBase::init()
{
    _LOG_FUNCION_();

    m_is_running = false;
    m_epfd = epoll_create(MAX_SIZE);
    if(m_epfd == -1)
    {
        printf("epoll_create failed.");
        return -1;
    }
    pthread_t tid = 0;
    pthread_create(&tid, NULL, eventHandle, (void*)this);
    m_tid = tid;
    return 0;
}

void EpollEventBase::stop()
{
    _LOG_FUNCION_();

    m_is_running = false;
    if (pthread_cancel(m_tid) == 0)
    {
        pthread_join(m_tid, (void **)NULL);
    }

    close(m_epfd);
}

EpollEventBase::~EpollEventBase()
{
    _LOG_FUNCION_();
    stop();
}

void EpollEventBase::setnoblocking(int v_sockfd)
{
    _LOG_FUNCION_();

    int opts = fcntl(v_sockfd,F_GETFL);
    if(opts < 0)
    {
        printf("fcntl(sockfd, F_GETFL) failed.");
        opts = opts|O_NONBLOCK;
    }
    fcntl(v_sockfd, F_SETFL, opts);

}

int EpollEventBase::register_event(int fd, EpollEventType type)
{
    _LOG_FUNCION_();
    setnoblocking(fd);
    struct epoll_event ev;
    ev.data.fd = fd;
    ev.events = type;
    if(epoll_ctl(m_epfd, EPOLL_CTL_ADD, fd, &ev) == -1)
    {
        printf("epoll_ctl: EPOLL_CTL_ADD failed, fd[%d].", &fd);
        return -1;
    }
    return 0;
}


int EpollEventBase::unregister_event(int fd)
{
    _LOG_FUNCION_();
    if(epoll_ctl(m_epfd, EPOLL_CTL_DEL, fd, NULL) == -1)
    {
        printf("epoll_ctl: EPOLL_CTL_DEL failed, fd[%d].",&fd);
        return -1;
    }
    return 0;
}



void* EpollEventBase::eventHandle(void* arg)
{
    _LOG_FUNCION_();

    EpollEventBase *pEvent = (EpollEventBase*)arg;
    if (pEvent != NULL)
    {
        pEvent->event_loop();
    }

    return NULL;
}

//=========================

EpollEventListener::EpollEventListener()
    :EpollEventBase(),m_listen_sock(0)
{
    _LOG_FUNCION_();
}

int EpollEventListener::init()
{
    _LOG_FUNCION_();
    struct sockaddr_in server_addr;
    if((m_listen_sock=socket(AF_INET, SOCK_STREAM, 0)) < 0)
    {
        printf("socket error\n");
        return -1;
    }

    bzero(&server_addr, sizeof(server_addr));
    server_addr.sin_family  =  AF_INET;
    server_addr.sin_port = htons(m_port);
    server_addr.sin_addr.s_addr  =  htonl(INADDR_ANY);

    int so_reuseaddr = 1;
    int z = setsockopt(m_listen_sock, SOL_SOCKET, SO_REUSEADDR,   &so_reuseaddr,  sizeof(so_reuseaddr));


    //bind
    if(bind(m_listen_sock, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0)
    {
        printf("bind error\n");
        return -1;
    }

    //listen
    if(listen(m_listen_sock, 5) < 0)
    {
        printf("listen error\n");
        return -1;
    }
    return 0;
}

void EpollEventListener::stop()
{
    _LOG_FUNCION_();

    m_epoll_event_agent_array[0].stop();
    m_epoll_event_agent_array[1].stop();

    EpollEventBase::stop();
    close(m_listen_sock);
    m_listen_sock = 0;
}

void EpollEventListener::event_loop()
{
    _LOG_FUNCION_();
    cout<<"EpollEventListener::event_loop"<<endl;
    m_is_running = true;
    while (m_is_running)
    {
        int ret = epoll_wait(m_epfd, m_events, MAX_SIZE, -1);
        if(ret < 0)
        {
            printf("epoll_wait failed, epfd[%d]",&m_epfd);
        }
        for(int i=0; i<ret; i++)
        {
            if (m_events[i].data.fd == m_listen_sock){
                accapt_event();
            }
            else
            {
                cout<<"fatal:event:"<<&m_epfd<<endl;
            }
        }
    }
}

EpollEventListener::~EpollEventListener()
{
    _LOG_FUNCION_();

    stop();
}

int EpollEventListener::start(int port)
{
    _LOG_FUNCION_();

    cout<<"EpollEventListener::start"<<endl;
    m_port = port;
    if (init()==0)
    {
        EpollEventBase::init();
        m_epoll_event_agent_array[0].init();
        //epoll_event_agent_array[1].init();
        set_socket_keepalive(m_listen_sock);
        return EpollEventBase::register_event(m_listen_sock, EDEFULT);
    }
    return -1;
}

int EpollEventListener::accapt_event()
{
    _LOG_FUNCION_();

    cout<<"EpollEventListener::accapt_event"<<endl;
    int fd = ::accept(m_listen_sock, NULL, NULL);
    if (fd <= 0)
    {
        cout<<"accapt_event error:"<<fd<<endl;
        return -1;
    }
    return m_epoll_event_agent_array[0].register_event(fd, EIN);
}

int EpollEventListener::set_socket_keepalive(int listenfd)
{
    _LOG_FUNCION_();

    int optval;
    socklen_t optlen = sizeof(optval);

    /* Check the status for the keepalive option */
    if(getsockopt(listenfd, SOL_SOCKET, SO_KEEPALIVE, &optval, &optlen) < 0) {
        perror("getsockopt()");
        close(listenfd);
        return -1;
        //exit(EXIT_FAILURE);
    }
    printf("SO_KEEPALIVE is %s\n", (optval ? "ON" : "OFF"));

    /* Set the option active */
    optval = 1;
    optlen = sizeof(optval);
    if(setsockopt(listenfd, SOL_SOCKET, SO_KEEPALIVE, &optval, optlen) < 0) {
        perror("setsockopt()");
        close(listenfd);
        return -1;
        //exit(EXIT_FAILURE);
    }

    int keepIdle = 2;     //30秒没有数据上来，则发送探测包
    int keepInterval = 2;  //每隔10发数一个探测包
    int keepCount = 3;      //发送3个探测包，未收到反馈则主动断开连接
    setsockopt(listenfd, SOL_SOCKET, TCP_KEEPIDLE, (void *)&keepIdle, sizeof(keepIdle));
    setsockopt(listenfd, SOL_SOCKET,TCP_KEEPINTVL, (void *)&keepInterval, sizeof(keepInterval));
    setsockopt(listenfd, SOL_SOCKET, TCP_KEEPCNT, (void *)&keepCount, sizeof(keepCount));

    printf("SO_KEEPALIVE set on socket\n");

    /* Check the status again */
    if(getsockopt(listenfd, SOL_SOCKET, SO_KEEPALIVE, &optval, &optlen) < 0) {
        perror("getsockopt()");
        close(listenfd);
        return -1;
        //exit(EXIT_FAILURE);
    }
    printf("SO_KEEPALIVE is %s\n", (optval ? "ON" : "OFF"));
    return 0;
}


//======================

EpollEventAgent::EpollEventAgent()
{
    _LOG_FUNCION_();
}

void EpollEventAgent::event_loop()
{
    _LOG_FUNCION_();
    m_is_running = true;
    cout<<"event_loop..."<<m_tid<<endl;
    while(m_is_running)
    {
        int ret = epoll_wait(m_epfd, m_events, MAX_SIZE, -1);
        if(ret < 0)
        {
            cout<<"epoll_wait failed, epfd:"<<&m_epfd<<endl;
        }
        for(int i=0; i<ret; i++)
        {
            EventData *pEventData = (EventData*)m_events[i].data.ptr;
            assert(pEventData!=NULL);
            cout<<"EPOLLIN..."<<endl;

            int connfd = pEventData->fd;

            if(m_events[i].events &EERR)
            {
                cout <<"===================="<<strerror(errno)<<endl;
                unregister_event(connfd);
                close(connfd);
            }
            else if(m_events[i].events & EPOLLIN)
            {
                do
                {
                    cout<<"before read"<<endl;
                    bool finished = false;
                    int nread = pEventData->read_buffer(finished);
                    cout <<nread<<":"<<errno<<endl;
                    if (nread > 0)
                    {
                        if(finished)
                        {
                            pEventData->reset();
                            continue;
                        }
                        else
                        {
                            break;
                        }
                    }
                    else if (0 == nread) //! eof 主动断开
                    {
                        unregister_event(connfd);
                        close(connfd);
                        cout<<"disconnect...."<<endl;
                        break;
                    }
                    else
                    {
                        if (errno == EINTR)
                        {
                            cout << "errno == EINTR...."<< strerror(errno) << endl;
                            continue;
                        }
                        else if (errno == EWOULDBLOCK || errno == EAGAIN)
                        {
                            cout << "errno == EWOULDBLOCK || errno == EAGAIN...."<< strerror(errno) << endl;
                            break;
                        }
                        else
                        {
                            //this->close();//=================
                            unregister_event(connfd);
                            close(connfd);
                            cout << "disconnect1111...."<< strerror(errno) << endl;
                            return;
                        }
                    }
                } while(1);
            }
        }
    }
}


int EpollEventAgent::register_event(int fd,  EpollEventType type)
{
    _LOG_FUNCION_();
    //int ret = EpollEventBase::register_event(fd, type);
    setnoblocking(fd);
    struct epoll_event ev;
    //ev.data.fd = fd;
    ev.events = type;
    EventData *pEventData = new EventData();
    pEventData->fd = fd;
    ev.data.ptr = (void*)pEventData;
    if(epoll_ctl(m_epfd, EPOLL_CTL_ADD, fd, &ev) == -1)
    {
        delete pEventData;
        pEventData = NULL;
        printf("epoll_ctl: EPOLL_CTL_ADD failed, fd[%d].", &fd);
        return -1;
    }

    if(pEventData == NULL) {
        cout<<"pEpollEventBuffer == NULL"<<endl;
        assert(false);
    }

    m_agentMap.insert(make_pair(fd, pEventData));

    return 0;
}

int EpollEventAgent::unregister_event(int fd)
{
    _LOG_FUNCION_();
    std::map<int, EventData*>::iterator it = m_agentMap.find(fd);
    if (it != m_agentMap.end()) {
        EventData *pEventData = it->second;
        if (pEventData != NULL) {
            delete pEventData;
            pEventData = NULL;
        }
        m_agentMap.erase(it);
    }
    return EpollEventBase::unregister_event(fd);
}

void EpollEventAgent::stop()
{
    _LOG_FUNCION_();
    disconnect_all();
    EpollEventBase::stop();
}

void EpollEventAgent::disconnect_all()
{
    _LOG_FUNCION_();
    std::map<int, EventData*>::iterator it = m_agentMap.begin();
    while (it != m_agentMap.end()) {
        EventData *pEventData = it->second;
        if (pEventData != NULL) {
            delete pEventData;
            pEventData = NULL;
        }
        EpollEventBase::unregister_event(it->first);
        close(it->first);
        m_agentMap.erase(it++);
    }
}

EpollEventAgent::~EpollEventAgent()
{
    _LOG_FUNCION_();
    stop();
}
