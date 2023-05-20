#ifndef _SOCKET_H_
#define _SOCKET_H_

#include "syslog.h"

typedef struct socket_s
{
    int    sockfd;
    char  *servip;
    int    port;
    int    connected;
}socket_t;

extern void log_write(int level,const char *fmt,...);
extern int socket_init(socket_t *sock, char *servip,int port);
extern int socket_connect(socket_t *sock);
extern int socket_check(socket_t *sock);
extern int socket_close(socket_t *sock);
extern int socket_write(socket_t *sock,char *data,size_t data_size);

#endif
