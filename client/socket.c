#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <netdb.h>
#include <sys/socket.h>
#include <errno.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <netinet/tcp.h>
#include "syslog.h"
#include "socket.h"

#define LOG_LEVEL_INFO  1
#define LOG_LEVEL_ERROR 2

int socket_init(socket_t *sock,char *servip,int port)
{
    sock->sockfd=-1;
    sock->connected=0;
    sock->servip=servip;
    sock->port=port;
}

int socket_close(socket_t   *sock)
{
    close(sock->sockfd);
    sock->sockfd=-1;
}

int socket_connect(socket_t *sock)
{
    int                     rv=-1;
    char                    ipstr[INET_ADDRSTRLEN];
    struct sockaddr_in      servaddr;
    struct addrinfo         hints;
    struct addrinfo        *res;
    struct addrinfo        *p;

    memset(&hints,0,sizeof(hints));
    hints.ai_flags=AI_PASSIVE;
    hints.ai_socktype=SOCK_STREAM;
    hints.ai_family=AF_INET;
    hints.ai_protocol=0;

    if(!sock-> servip)
    {
        rv=getaddrinfo(sock->servip,NULL,&hints,&res);
        if(rv!=0)
        {
            log_write(LOG_LEVEL_ERROR,"Analyze failure:%s\n",strerror(errno));
            return -1;
        }
        log_write(LOG_LEVEL_INFO,"Analyze successfully\n");

        for(p=res;p!=NULL;p=p->ai_next)
        {
            servaddr=*((struct sockaddr_in*)p->ai_addr);
            inet_ntop(AF_INET,&(servaddr.sin_addr),ipstr,sizeof(ipstr));
            sock->servip=ipstr;
        }  
        freeaddrinfo(res);
        return 0;
    }

    sock->sockfd=socket(AF_INET,SOCK_STREAM,0);

    if(sock->sockfd<0)
    {
        log_write(LOG_LEVEL_ERROR,"create socket failure:%s\n",strerror(errno));
        return -1;
    }

    log_write(LOG_LEVEL_INFO,"create socket[%d] successfully\n",sock->sockfd);

    memset(&servaddr,0,sizeof(servaddr));

    servaddr.sin_family=AF_INET;

    servaddr.sin_port=htons(sock->port);

    inet_aton(sock->servip,&servaddr.sin_addr);

    rv= connect(sock->sockfd,(struct sockaddr *)&servaddr,sizeof(struct sockaddr_in));
    if(rv<0)
    {
        log_write(LOG_LEVEL_ERROR,"connect to server[%s:%d] failure:%s\n",sock->servip,sock->port,strerror(errno));
    }
}



int socket_check(socket_t *sock)
{
    struct  tcp_info    info;
    int                 len=sizeof(info);

    if(sock->sockfd <=0)
    {
        return -1;
    }
    
    getsockopt(sock->sockfd,IPPROTO_TCP,TCP_INFO,&info,(socklen_t *)&len);
    if((info.tcpi_state==1))
    {
        sock->connected=1;
        log_write(LOG_LEVEL_INFO,"socket Connected!!!\n");
    }
    else
    {
        sock->connected=0;
        log_write(LOG_LEVEL_ERROR,"socket Disconnected!!!\n");

    }
}
   

int   socket_write(socket_t *sock,char *data,size_t data_size)

{
    int rv=-1;
    rv=write(sock->sockfd,data,data_size);
    if(rv<0)
    {
        log_write(LOG_LEVEL_ERROR,"write to server by sockfd[%d] failure:%s\n",sock->sockfd,strerror(errno));
        return -3;
    }
}
