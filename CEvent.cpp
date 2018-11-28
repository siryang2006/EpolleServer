#include "CEvent.h"
#include <pthread.h>
#include <iostream>

using namespace std;

CEvent::CEvent()
{
    is_running = false;
    epfd = epoll_create(MAX_SIZE);
    if(epfd == -1)
    {
        printf("epoll_create failed.");
        return;
    }
    pthread_t tid = 0;
    pthread_create(&tid, NULL, EventHandle, (void*)this);
    m_tid = tid;
    //线程池初始化
    //pool = CThreadPoolProxy::instance();
}


CEvent::~CEvent()
{
    if(pthread_cancel(m_tid) == 0)
    {
        pthread_join(m_tid, (void **)NULL);
    }
}



void CEvent::SetNoblocking(int v_sockfd)
{
    int opts = fcntl(v_sockfd,F_GETFL);
    if(opts < 0)
    {
        printf("fcntl(sockfd, F_GETFL) failed.");
        opts = opts|O_NONBLOCK;
    }
    fcntl(v_sockfd, F_SETFL, opts);

}


int CEvent::Register_event(int fd, EventType type)
{
    SetNoblocking(fd);
    struct epoll_event ev;
    ev.data.fd = fd;
    ev.events = type;
    if(epoll_ctl(epfd, EPOLL_CTL_ADD, fd, &ev) == -1)
    {
        printf("epoll_ctl: EPOLL_CTL_ADD failed, fd[%d].", &fd);
        return -1;
    }
    return 0;
}


int CEvent::unRegister_event(int fd)
{
    if(epoll_ctl(epfd, EPOLL_CTL_DEL, fd, NULL) == -1)
    {
        printf("epoll_ctl: EPOLL_CTL_DEL failed, fd[%d].",&fd);
        return -1;
    }
    return 0;
}


void* CEvent::EventHandle(void* arg)
{
    CEvent &event = *(CEvent*)arg;
    event.is_running = true;
    while(event.is_running)
    {
        cout<<"run..."<<endl;
        int ret = epoll_wait(event.epfd, event.events, MAX_SIZE, -1);
        if(ret < 0)
        {
            printf("epoll_wait failed, epfd[%d]",&event.epfd);
        }
        for(int i=0; i<ret; i++)
        {


            if(event.events[i].events & EPOLLIN)
            {
                int connfd = event.events[i].data.fd;
                cout<<"EPOLLIN..."<<endl;
                int nread = 0;
                char recv_buffer[BUFFER_SIZE];
                do
                {
                    nread = ::read(connfd, recv_buffer, sizeof(recv_buffer) - 1);
                    if (nread > 0)
                    {
                        recv_buffer[nread] = '\0';
                        cout << recv_buffer << endl;
                        if (nread < int(sizeof(recv_buffer) - 1))
                        {
                            break;//! equal EWOULDBLOCK
                        }
                    }
                    else if (0 == nread) //! eof
                    {
                      //  this->close();//=================
                        printf("end");
                        return NULL;
                    }
                    else
                    {
                        if (errno == EINTR)
                        {
                            continue;
                        }
                        else if (errno == EWOULDBLOCK)
                        {
                            break;
                        }
                        else
                        {
                            //this->close();//=================
                            printf("end111");
                            return NULL;
                        }
                    }
                } while(1);
                /*CTask* ta=new CMyTask;       //  具体的方法自己实现。
                ta->SetConnFd(connfd);
                pool->AddTask(ta);*/
            }
        }
    }
    return NULL;
}
