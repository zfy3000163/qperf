#include <stdio.h>
#include <sys/ioctl.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <strings.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <errno.h>
#include <assert.h>

#include "ff_config.h"
#include "ff_api.h"
#include "ff_epoll.h"

#include <stdarg.h>
#include <netdb.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <ctype.h>


#define MAX_EVENTS 512

struct epoll_event ev;
struct epoll_event events[MAX_EVENTS];

int epfd;
int sockfd;

char html[] = 
"HTTP/1.1 200 OK\r\n"
"Server: F-Stack\r\n"
"Date: Sat, 25 Feb 2017 09:26:33 GMT\r\n"
"Content-Type: text/html\r\n"
"Content-Length: 438\r\n"
"Last-Modified: Tue, 21 Feb 2017 09:44:03 GMT\r\n"
"Connection: keep-alive\r\n"
"Accept-Ranges: bytes\r\n"
"\r\n"
"<!DOCTYPE html>\r\n"
"<html>\r\n"
"<head>\r\n"
"<title>Welcome to F-Stack!</title>\r\n"
"<style>\r\n"
"    body {  \r\n"
"        width: 35em;\r\n"
"        margin: 0 auto; \r\n"
"        font-family: Tahoma, Verdana, Arial, sans-serif;\r\n"
"    }\r\n"
"</style>\r\n"
"</head>\r\n"
"<body>\r\n"
"<h1>Welcome to F-Stack!</h1>\r\n"
"\r\n"
"<p>For online documentation and support please refer to\r\n"
"<a href=\"http://F-Stack.org/\">F-Stack.org</a>.<br/>\r\n"
"\r\n"
"<p><em>Thank you for using F-Stack.</em></p>\r\n"
"</body>\r\n"
"</html>";

int loop(void *arg)
{
    /* Wait for events to happen */

    int nevents = ff_epoll_wait(epfd,  events, 1, 0);
    int i, count = 0, iret;
    if (nevents != 0)
	printf("nevents:%d\n", nevents);

    for (i = 0; i < nevents; ++i) {
        printf("%x, %x\n",events[i].events, events[i].events & EPOLLIN);
        /* Handle new connect */
        if (events[i].data.fd == sockfd && events[i].events & EPOLLIN) {
            //while (1) {
                char buf[256];
                memset(&buf, 0x0, 256);
                size_t readlen ;
                readlen = ff_read( events[i].data.fd, buf, 256);
                printf("buf:%s\n", buf);
                iret = ff_write(sockfd, "world", strlen("world")+1);
                printf("iret write:%d\n", iret);


#if 0
                struct sockaddr_in my_addr;
                bzero(&my_addr, sizeof(my_addr));
                my_addr.sin_family = AF_INET;
                my_addr.sin_port = htons(80);
                //my_addr.sin_addr.s_addr = htonl(INADDR_ANY);
                inet_pton(AF_INET,"192.168.2.3",&my_addr.sin_addr.s_addr);

                int ret = ff_connect(sockfd, &my_addr, sizeof(my_addr));
                printf("ret:%d\n", ret);
                if (ret < 0) {
                    printf("connect is error\n");
                    break;
                }
                int nclientfd = ff_accept(sockfd, NULL, NULL);
                printf("while accept:%d\n", nclientfd);
                if (nclientfd < 0) {
                    break;
                }

                /* Add to event list */
                ev.data.fd = nclientfd;
                ev.events  = EPOLLIN;
                if (ff_epoll_ctl(epfd, EPOLL_CTL_ADD, nclientfd, &ev) != 0) {
                    printf("ff_epoll_ctl failed:%d, %s\n", errno,
                        strerror(errno));
                    break;
                }
#endif


            //}
        } 
        else if (events[i].events & EPOLLOUT ) {
                iret = ff_write(sockfd, "echo", strlen("echo")+1);
                printf("iret write:%d\n", iret);
                ev.data.fd = sockfd;
                ev.events = EPOLLIN | EPOLLET;
                if (ff_epoll_ctl(epfd, EPOLL_CTL_MOD, sockfd, &ev) != 0) {
                    printf("ff_epoll_ctl failed:%d, %s\n", errno,
                        strerror(errno));
                    break;
                }

        }
        else if (events[i].events & EPOLLERR ) {
                /* Simply close socket */
                ff_epoll_ctl(epfd, EPOLL_CTL_DEL,  events[i].data.fd, NULL);
                ff_close(events[i].data.fd);
        } 
        else if (events[i].events & EPOLLIN) {
                printf("read and write\n");
                char buf[256];
                //size_t readlen = ff_read( events[i].data.fd, buf, sizeof(buf));
                count = 0;
                while(1){
                size_t readlen ;

                readlen = ff_read( events[i].data.fd, buf, 2);
                if(readlen != -1 )
                    count += readlen; 
                printf("count:%d, readlen:%d\n", count, readlen);
                printf("buf:%s\n", buf);
                if (count > 4)
                    break;
#if 0
                if(readlen > 0) {
                    ff_write( events[i].data.fd, html, sizeof(html) - 1);
                } else {
                    ff_epoll_ctl(epfd, EPOLL_CTL_DEL,  events[i].data.fd, NULL);
                    ff_close( events[i].data.fd);
                }
#endif
                }
        }
        else {
                printf("unknown event: %8.8X\n", events[i].events);
        }
    }
  
  
}

void
setsockopt_one(int fd, int optname)
{
    int one = 1;

    if (setsockopt(fd, SOL_SOCKET, optname, &one, sizeof(one)) >= 0)
        return;
    printf("setsockopt %d %d to 1 failed", SOL_SOCKET, optname);
}
char *
qasprintf(char *fmt, ...)
{
    int stat;
    char *str;
    va_list alist;

    va_start(alist, fmt);
    stat = vasprintf(&str, fmt, alist);
    va_end(alist);
    if (stat < 0)
        error(0, "out of space");
    return str;
}
struct addrinfo *
getaddrinfo_port(char *node, int port, struct addrinfo *hints)
{
    struct addrinfo *res;
    char *service = qasprintf("%d", port);
    int stat = getaddrinfo(node, service, hints, &res);

    free(service);
    if (stat != 0)
        error(0, "getaddrinfo failed1: %s", gai_strerror(stat));
    if (!res)
        error(0, "getaddrinfo failed: no valid entries");
    return res;
}


typedef struct addrinfo AI;
static int      ListenFD;
static int  ListenPort      = 80;
void server_listen(void)
{
    int iret;
    AI *ai;
    AI hints ={
        .ai_flags    = AI_PASSIVE | AI_NUMERICSERV,
        .ai_family   = AF_UNSPEC,
        .ai_socktype = SOCK_STREAM
    };
    memset(&hints, 0, sizeof(AI));
    printf("ListenPort:%d\n", ListenPort);
    AI *ailist = getaddrinfo_port(0, ListenPort, &hints);

    char ip[46];
    int ipbuf_len = 46;
    memset(&ip, 0x0, 46);
    char *ipbuf = ip;
    for (ai = ailist; ai; ai = ai->ai_next) {
        if(ai->ai_family == 10)
             continue;
        ListenFD = socket(ai->ai_family, ai->ai_socktype, ai->ai_protocol);
        printf("Listen:%d, family:%d, socktype:%d, protocol:%d\n", ListenFD, ai->ai_family, ai->ai_socktype, ai->ai_protocol);
        if (ListenFD < 0)
            continue;
        setsockopt_one(ListenFD, SO_REUSEADDR);
if (ai->ai_family == AF_INET) {
        struct sockaddr_in *sa = (struct sockaddr_in *)ai->ai_addr;
        sa->sin_addr.s_addr = htonl(INADDR_ANY);
        inet_ntop(AF_INET, &(sa->sin_addr), ipbuf, ipbuf_len);
    } else {
        struct sockaddr_in6 *sa = (struct sockaddr_in6 *)ai->ai_addr;
        inet_ntop(AF_INET6, &(sa->sin6_addr), ipbuf, ipbuf_len);
    }
        printf("%d,%s\n", ipbuf_len, ipbuf);
        //if (ff_bind(ListenFD, ai->ai_addr, ai->ai_addrlen) == SUCCESS0)
        iret = bind(ListenFD, (ai->ai_addr), ai->ai_addrlen); 
        printf("iret:%d\n", iret);
        if (iret == 0)
            break;
        close(ListenFD);
    }
    freeaddrinfo(ailist);
    if (!ai)
        printf("unable to bind to listen port");

    if (listen(ListenFD, MAX_EVENTS) < 0)
        printf("listen failed");
}



int main(int argc, char * argv[])
{
    ff_init(argc, argv);


    sockfd = ff_socket(AF_INET, SOCK_STREAM, 0);
    printf("sockfd:%d\n", sockfd);
    if (sockfd < 0) {
        printf("ff_socket failed\n");
        exit(1);
    }

    //int on = 1;
    //ff_ioctl(sockfd, FIONBIO, &on);

    struct sockaddr_in my_addr;
    bzero(&my_addr, sizeof(my_addr));
    my_addr.sin_family = AF_INET;
    my_addr.sin_port = htons(80);
    //my_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    inet_pton(AF_INET,"192.168.2.3",&my_addr.sin_addr.s_addr);


    int ret = ff_connect(sockfd, &my_addr, sizeof(my_addr));
    if (ret < 0) {
        printf("connect error\n");
    }

#if 0
    int ret = ff_bind(sockfd, (struct linux_sockaddr *)&my_addr, sizeof(my_addr));
    if (ret < 0) {
        printf("ff_bind failed\n");
        exit(1);
    }

    ret = ff_listen(sockfd, MAX_EVENTS);
    if (ret < 0) {
        printf("ff_listen failed\n");
        exit(1);
    }
#endif


    printf("ListenFD:%d\n", ListenFD);

    assert((epfd = ff_epoll_create(0)) > 0);
    ev.data.fd = sockfd;
    ev.events = EPOLLIN|EPOLLET|EPOLLOUT;
    ff_epoll_ctl(epfd, EPOLL_CTL_ADD, sockfd, &ev);
    ff_run(loop, NULL);
    return 0;
}
