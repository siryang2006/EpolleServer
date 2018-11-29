#include "CEvent.h"
#include <pthread.h>
#include <iostream>

#include <sys/socket.h>
#include <poll.h>
#include <sys/epoll.h>
#include <sys/time.h>
#include <netinet/in.h>

using namespace std;

EpollEventBase::EpollEventBase()
{
}

int EpollEventBase::init()
{
	is_running = false;
    epfd = epoll_create(MAX_SIZE);
    if(epfd == -1)
    {
        printf("epoll_create failed.");
        return;
    }
    pthread_t tid = 0;
    pthread_create(&tid, NULL, eventHandle, (void*)this);
    m_tid = tid;
}

void EpollEventBase::stop()
{
	is_running = false;
	if (pthread_cancel(m_tid) == 0)
    {
        pthread_join(m_tid, (void **)NULL);
    }
}

EpollEventBase::~EpollEventBase()
{
    stop();
}

void EpollEventBase::setnoblocking(int v_sockfd)
{
    int opts = fcntl(v_sockfd,F_GETFL);
    if(opts < 0)
    {
        printf("fcntl(sockfd, F_GETFL) failed.");
        opts = opts|O_NONBLOCK;
    }
    fcntl(v_sockfd, F_SETFL, opts);

}

int EpollEventBase::register_event(int fd, EventType type)
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


int EpollEventBase::unregister_event(int fd)
{
    if(epoll_ctl(epfd, EPOLL_CTL_DEL, fd, NULL) == -1)
    {
        printf("epoll_ctl: EPOLL_CTL_DEL failed, fd[%d].",&fd);
        return -1;
    }
    return 0;
}


/*void EpollEventBase::EventLoop()
{
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
            if(event.events[i].data.fd == event.m_serverfd){
                event.accapt_event();
            }
            else if(event.events[i].events & EPOLLIN)
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
            }
        }
    }
    return NULL;
}*/

void* EpollEventBase::eventHandle(EpollEventBase* arg)
{
    EpollEventBase *pEvent = (EpollEventBase*)arg;
	if (pEvent != NULL) {
		pEvent->EventWork();
	}

	return NULL:
}

//=========================

EpollEventListener::EpollEventListener()
	:EpollEventBase(),listen_sock(0)
{
		
}

int EpollEventListener::init()
{
	struct sockaddr_in server_addr;
    if((listen_sock=socket(AF_INET, SOCK_STREAM, 0)) < 0)
    {
        printf("socket error\n");
        return -1;
    }

    bzero(&server_addr, sizeof(server_addr));
    server_addr.sin_family  =  AF_INET;
    server_addr.sin_port = htons(m_port);
    server_addr.sin_addr.s_addr  =  htonl(INADDR_ANY);

    //bind
    if(bind(listen_sock, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0)
    {
        printf("bind error\n");
        return -1;
    }

    //listen
    if(listen(listen_sock, 5) < 0)
    {
        printf("listen error\n");
        return -1;
    }
	return 0;
}

void EpollEventListener::event_loop()
{
	cout<<"EpollEventListener::event_loop"<<endl;
	is_running = true;
    while (is_running)
    {
        int ret = epoll_wait(event.epfd, event.events, MAX_SIZE, -1);
        if(ret < 0)
        {
            printf("epoll_wait failed, epfd[%d]",&event.epfd);
        }
        for(int i=0; i<ret; i++)
        {
            if (event.events[i].data.fd == event.m_serverfd){
                accapt_event();
            }
            else
            {
				cout<<"fatal:event:"<<&event.epfd<<endl;
			}
        }
    }
}

EpollEventListener::~EpollEventListener()
{
	epoll_event_agent_array[0].stop();
	epoll_event_agent_array[1].stop();
		stop();
}

int EpollEventListener::start(int port)
{
	cout<<"EpollEventListener::start"<<endl;
	m_port = port;
	int port;
	if (init()==0)
	{
		EpollEventBase::init();
		epoll_event_agent_array[0].init();
		//epoll_event_agent_array[1].init();
		return EpollEventBase::register_event(listen_sock, EDEFULT);
	}
	return -1;
}

int EpollEventListener::accapt_event()
{
	cout<<"EpollEventListener::accapt_event"<<endl;
    int fd = ::accept(m_serverfd, NULL, NULL);
	return epoll_event_agent_array[0].register_event(fd, EIN);
}

//======================
void EpollEventAgent::event_loop()
{
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
					  unregister_event(connfd);
                        printf("disconnect....");
						continue;
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
                            unregister_event(connfd);
							printf("disconnect1111....");
                            return NULL;
                        }
                    }
                } while(1);
            }
        }
    }
    return NULL;
}

void EpollEventAgent::read_data()
{

}

//=========================
EpollEventAgent::EpollEventAgent()
{

}

EpollEventAgent::~EpollEventAgent()
{
stop();
}