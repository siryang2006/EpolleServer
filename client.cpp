/*局域网TCP客户端*/
#include <sys/types.h>
#include <sys/socket.h>
#include <stdio.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/shm.h>
 
#define MYPORT  2222
#define BUFFER_SIZE 1024
 
int send_buffer(int sock, const char *szData){
    int ret = 0;
    int pack_length = strlen(szData)+4;
    char *data = (char*)calloc(pack_length, 1); 
    sprintf(data,"%04d", strlen(szData));
    memcpy(data+4, szData, strlen(szData));
    printf("send data %s\n", szData);
    ret = send(sock, data, pack_length, 1);
    free(data);
    return ret;
}

int main()
{
    ///定义sockfd
    int sock_cli = socket(AF_INET,SOCK_STREAM, 0);
 
    ///定义sockaddr_in
    struct sockaddr_in servaddr;
    memset(&servaddr, 0, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_port = htons(MYPORT);  //服务器端口
    servaddr.sin_addr.s_addr = inet_addr("127.0.0.1");  //服务器ip，inet_addr用于IPv4的IP转换（十进制转换为二进制）
    //127.0.0.1是本地预留地址
    //连接服务器，成功返回0，错误返回-1
    if (connect(sock_cli, (struct sockaddr *)&servaddr, sizeof(servaddr)) < 0)
    {
        perror("connect");
        exit(1);
    }
 
    char sendbuf[BUFFER_SIZE];
    char recvbuf[BUFFER_SIZE];
 
int i=0;
    while (i<100)
    {
	send_buffer(sock_cli, "hello word");
i++;
    }
    close(sock_cli);
    return 0;
}
