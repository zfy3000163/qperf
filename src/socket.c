/*
 * qperf - handle socket tests.
 *
 * Copyright (c) 2002-2009 Johann George.  All rights reserved.
 * Copyright (c) 2006-2009 QLogic Corporation.  All rights reserved.
 *
 * This software is available to you under a choice of one of two
 * licenses.  You may choose to be licensed under the terms of the GNU
 * General Public License (GPL) Version 2, available from the file
 * COPYING in the main directory of this source tree, or the
 * OpenIB.org BSD license below:
 *
 *     Redistribution and use in source and binary forms, with or
 *     without modification, are permitted provided that the following
 *     conditions are met:
 *
 *      - Redistributions of source code must retain the above
 *        copyright notice, this list of conditions and the following
 *        disclaimer.
 *
 *      - Redistributions in binary form must reproduce the above
 *        copyright notice, this list of conditions and the following
 *        disclaimer in the documentation and/or other materials
 *        provided with the distribution.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS
 * BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN
 * ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */
#define _GNU_SOURCE
#include <errno.h>
#include <netdb.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>

#include <ctype.h>
#include <errno.h>
#include <fcntl.h>
#include <netdb.h>
#include <sched.h>
#include <stdio.h>
#include <signal.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/wait.h>
#include <sys/ioctl.h>
#include <sys/times.h>
#include <sys/select.h>
#include <sys/utsname.h>
#include "qperf.h"

static uint32_t array_listenfd[1000000] = {0};

#define MAX_EVENTS 512
struct epoll_event ev_tcp_bw;
struct epoll_event events_tcp_bw[MAX_EVENTS];
int epfd_tcp_bw;




/*
 * Parameters.
 */
#define AF_INET_SDP 27                  /* Family for SDP */


/*
 * Kinds.
 */
typedef enum {
    K_SCTP,
    K_SDP,
    K_TCP,
    K_UDP,
} KIND;

char *Kinds[] ={ "SCTP", "SDP", "TCP", "UDP", };


/*
 * Function prototypes.
 */
static void     client_init(int *fd, KIND kind);
static void     datagram_client_bw(KIND kind);
static void     datagram_client_lat(KIND kind);
static void     datagram_server_bw(KIND kind);
static void     datagram_server_init(int *fd, KIND kind);
static void     datagram_server_lat(KIND kind);
static void     get_socket_port(int fd, uint32_t *port);
static AI      *getaddrinfo_kind(int serverflag, KIND kind, int port);
static void     ip_parameters(long msgSize);
static char    *kind_name(KIND kind);
static int      recv_full(int fd, void *ptr, int len);
static int      send_full(int fd, void *ptr, int len);
static void     set_socket_buffer_size(int fd);
static void     stream_client_bw(KIND kind);
static void     stream_client_lat(KIND kind);
static void     stream_server_bw(KIND kind);
static void     stream_server_init(int *fd, KIND kind);
static void     stream_server_lat(KIND kind);


/*
 * Measure SCTP bandwidth (client side).
 */
void
run_client_sctp_bw(void)
{
    par_use(L_ACCESS_RECV);
    par_use(R_ACCESS_RECV);
    ip_parameters(32*1024);
    stream_client_bw(K_SCTP);
}


/*
 * Measure SCTP bandwidth (server side).
 */
void
run_server_sctp_bw(void)
{
    stream_server_bw(K_SCTP);
}


/*
 * Measure SCTP latency (client side).
 */
void
run_client_sctp_lat(void)
{
    ip_parameters(1);
    stream_client_lat(K_SCTP);
}


/*
 * Measure SCTP latency (server side).
 */
void
run_server_sctp_lat(void)
{
    stream_server_lat(K_SCTP);
}


/*
 * Measure SDP bandwidth (client side).
 */
void
run_client_sdp_bw(void)
{
    par_use(L_ACCESS_RECV);
    par_use(R_ACCESS_RECV);
    ip_parameters(64*1024);
    stream_client_bw(K_SDP);
}


/*
 * Measure SDP bandwidth (server side).
 */
void
run_server_sdp_bw(void)
{
    stream_server_bw(K_SDP);
}


/*
 * Measure SDP latency (client side).
 */
void
run_client_sdp_lat(void)
{
    ip_parameters(1);
    stream_client_lat(K_SDP);
}


/*
 * Measure SDP latency (server side).
 */
void
run_server_sdp_lat(void)
{
    stream_server_lat(K_SDP);
}


/*
 * Measure TCP bandwidth (client side).
 */
void
run_client_tcp_bw(void)
{
    par_use(L_ACCESS_RECV);
    par_use(R_ACCESS_RECV);
    ip_parameters(64*1024);
    stream_client_bw(K_TCP);
}


/*
 * Measure TCP bandwidth (server side).
 */
void
run_server_tcp_bw(void)
{
    stream_server_bw(K_TCP);
}


/*
 * Measure TCP latency (client side).
 */
void
run_client_tcp_lat(void)
{
    ip_parameters(1);
    stream_client_lat(K_TCP);
}


/*
 * Measure TCP latency (server side).
 */
void
run_server_tcp_lat(void)
{
    stream_server_lat(K_TCP);
}


/*
 * Measure UDP bandwidth (client side).
 */
void
run_client_udp_bw(void)
{
    par_use(L_ACCESS_RECV);
    par_use(R_ACCESS_RECV);
    ip_parameters(32*1024);
    datagram_client_bw(K_UDP);
}


/*
 * Measure UDP bandwidth (server side).
 */
void
run_server_udp_bw(void)
{
    datagram_server_bw(K_UDP);
}


/*
 * Measure UDP latency (client side).
 */
void
run_client_udp_lat(void)
{
    ip_parameters(1);
    datagram_client_lat(K_UDP);
}


/*
 * Measure UDP latency (server side).
 */
void
run_server_udp_lat(void)
{
    datagram_server_lat(K_UDP);
}


/*
 * Measure stream bandwidth (client side).
 */
static void
stream_client_bw(KIND kind)
{
    char *buf;
    int sockFD;

    client_send_request();

#if 0
    client_init(&sockFD, kind);
    array_listenfd[sockFD] = sockFD;

    buf = qmalloc(Req.msg_size);
    sync_test();
    while (!Finished) {
        int n = send_full(sockFD, buf, Req.msg_size);

        if (Finished)
            break;
        if (n < 0) {
            LStat.s.no_errs++;
            continue;
        }
        LStat.s.no_bytes += n;
        LStat.s.no_msgs++;
    }
    stop_test_timer();
    exchange_results();
    free(buf);
    close(sockFD);
    show_results(BANDWIDTH);
#endif
}

/*Client*/
int stream_client_bw_loop(void *arg)
{
    char *buf = 0;
    int nevents = ff_epoll_wait(epfd_tcp_bw,  events_tcp_bw, 1, 0);
    int i, iret, listenfd = -1, acceptfd = -1;
    if (nevents > 0){
	//printf("child client nevents:%d\n", nevents);
        ;
    }

    for (i = 0; i < nevents; ++i) {
        listenfd = -1;
        listenfd = events_tcp_bw[i].data.fd;
        if (events_tcp_bw[i].data.fd == array_listenfd[listenfd] &&  events_tcp_bw[i].events & EPOLLIN) {
            printf("child client EPOLLIN\n");
            ;
        } 
        else if (events_tcp_bw[i].data.fd == array_listenfd[listenfd] && events_tcp_bw[i].events & EPOLLOUT ) {


            //printf("children read...:%d, Finished: %d\n", Req.msg_size, Finished);
            //remotefd_setup();
            int n = 0;
            //buf = qmalloc(Req.msg_size);
            //Req.msg_size = 1000;
            char pbuf[Req.msg_size];
            memset(&pbuf, 0x0, Req.msg_size);
            char *buf = pbuf;

            n = ff_write(events_tcp_bw[i].data.fd, buf, Req.msg_size);
            //n = send_full(events_tcp_bw[i].data.fd, buf, Req.msg_size);
            //printf("n:%d\n", n);

            if (Finished){
                printf("child finished break:%d, Req.msg_size:%d\n", Finished, Req.msg_size);
                stop_test_timer();
                //exchange_results();
                //free(buf);
                if (events_tcp_bw[i].data.fd >= 0)
                    close(events_tcp_bw[i].data.fd);
                //show_results(BANDWIDTH);

                break;
            }
            if (n <= 0) {
                LStat.s.no_errs++;
                break;
            }
            LStat.s.no_bytes += n;
            LStat.s.no_msgs++;


        }
        else if (events_tcp_bw[i].events & EPOLLERR ) {
                /* Simply close socket */
                printf("child link is close\n");
                ff_epoll_ctl(epfd_tcp_bw, EPOLL_CTL_DEL,  events_tcp_bw[i].data.fd, NULL);
                ff_close(events_tcp_bw[i].data.fd);
        }
        else {
                printf("unknown event: %8.8X\n", events_tcp_bw[i].events);
                return -1;
        }
    }

    return 0;

}

/*Lat Server*/
int stream_server_lat_loop(void *arg)
{
    char *buf = 0;
    int nevents = ff_epoll_wait(epfd_tcp_bw,  events_tcp_bw, 1, 0);
    int i, iret, listenfd = -1, acceptfd = -1;
    if (nevents > 0){
	//printf("child nevents:%d\n", nevents);
        ;
    }

    for (i = 0; i < nevents; ++i) {
        listenfd = -1;
        listenfd = events_tcp_bw[i].data.fd;
        if (listenfd == array_listenfd[listenfd]) {
            while (1) {
               iret = ff_accept(listenfd, 0, 0);
               //printf("***************child ready for requests:acceptfd:%d\n", iret);
               if (iret < 0){
                    printf("child ff_accept failed:%d, %s\n", errno,
                        strerror(errno));
                   close(listenfd);
                   break;
               }
               acceptfd = iret;
               set_socket_buffer_size(acceptfd);

                /* Add to event list */
                ev_tcp_bw.data.fd = acceptfd;
                ev_tcp_bw.events  = EPOLLIN | EPOLLOUT;
                if (ff_epoll_ctl(epfd_tcp_bw, EPOLL_CTL_ADD, acceptfd, &ev_tcp_bw) != 0) {
                    printf("child ff_epoll_ctl failed:%d, %s\n", errno,
                        strerror(errno));
                    break;
                }
            }
        }
        else { 
            if (events_tcp_bw[i].events & EPOLLERR ) {
                /* Simply close socket */
                printf("child link is close\n");
                ff_epoll_ctl(epfd_tcp_bw, EPOLL_CTL_DEL,  events_tcp_bw[i].data.fd, NULL);
                ff_close(events_tcp_bw[i].data.fd);
            } 
            else if (events_tcp_bw[i].events & EPOLLOUT) {
              
                //printf("children write...:%d, Finished: %d\n", Req.msg_size, Finished);

                int n = 0;
                //buf = qmalloc(Req.msg_size);

                char pbuf[Req.msg_size];
                memset(&pbuf, 0x0, Req.msg_size);
                char *buf = pbuf;

                n = ff_write(events_tcp_bw[i].data.fd, buf, Req.msg_size);

                if (Finished){
                    printf("child finished break:%d, Req.msg_size:%d\n", Finished, Req.msg_size);
                    stop_test_timer();
                    exchange_results();
                    //free(buf);
                    if (events_tcp_bw[i].data.fd >= 0)
                        close(events_tcp_bw[i].data.fd);
                    break;
                }

                if (n < 0) {
                    LStat.r.no_errs++;
                    break;
                }

                LStat.r.no_bytes += n;
                LStat.r.no_msgs++;


            }
            else if (events_tcp_bw[i].events & EPOLLIN) {
              

                //printf("children read...:%d, Finished: %d\n", Req.msg_size, Finished);
                //remotefd_setup();
                int n = 0;
                //buf = qmalloc(Req.msg_size);

                char pbuf[Req.msg_size];
                memset(&pbuf, 0x0, Req.msg_size);
                char *buf = pbuf;
                n = ff_read(events_tcp_bw[i].data.fd, buf, Req.msg_size);

                if (n < 0) {
                    LStat.r.no_errs++;
                    break;
                }

                LStat.s.no_bytes += n;
                LStat.s.no_msgs++;

                if (Finished){
                    printf("child finished break:%d, Req.msg_size:%d\n", Finished, Req.msg_size);
                    //stop_test_timer();
                    //exchange_results();
                    //free(buf);
                    //if (events_tcp_bw[i].data.fd >= 0)
                    //    close(events_tcp_bw[i].data.fd);
                    break;
                }


            } 
            else {
                printf("unknown event: %8.8X\n", events_tcp_bw[i].events);
                return -1;
            }
        }
    }

    return 0;

}



/*Server*/
int stream_server_bw_loop(void *arg)
{
    char *buf = 0;
    int nevents = ff_epoll_wait(epfd_tcp_bw,  events_tcp_bw, 1, 0);
    int i, iret, listenfd = -1, acceptfd = -1;
    if (nevents > 0){
	//printf("nevents:%d\n", nevents);
        ;
       
    }

    for (i = 0; i < nevents; ++i) {
        listenfd = -1;
        listenfd = events_tcp_bw[i].data.fd;
        if (listenfd == array_listenfd[listenfd]) {
            while (1) {
               iret = ff_accept(listenfd, 0, 0);
               //printf("***************child ready for requests:acceptfd:%d\n", iret);
               if (iret < 0){
                    printf("ff_accept failed:%d, %s\n", errno,
                        strerror(errno));
                   close(listenfd);
                   break;
               }
               acceptfd = iret;
               set_socket_buffer_size(acceptfd);

                /* Add to event list */
                ev_tcp_bw.data.fd = acceptfd;
                ev_tcp_bw.events  = EPOLLIN;
                if (ff_epoll_ctl(epfd_tcp_bw, EPOLL_CTL_ADD, acceptfd, &ev_tcp_bw) != 0) {
                    printf("ff_epoll_ctl failed:%d, %s\n", errno,
                        strerror(errno));
                    break;
                }
            }
        }
        else { 
            if (events_tcp_bw[i].events & EPOLLERR ) {
                /* Simply close socket */
                printf("child link is close\n");
                ff_epoll_ctl(epfd_tcp_bw, EPOLL_CTL_DEL,  events_tcp_bw[i].data.fd, NULL);
                ff_close(events_tcp_bw[i].data.fd);
            } else if (events_tcp_bw[i].events & EPOLLIN) {
              

                //printf("children read...:%d, Finished: %d\n", Req.msg_size, Finished);
                //remotefd_setup();
                int n = 0;
                //buf = qmalloc(Req.msg_size);

                char pbuf[Req.msg_size];
                memset(&pbuf, 0x0, Req.msg_size);
                char *buf = pbuf;
                n = ff_read(events_tcp_bw[i].data.fd, buf, Req.msg_size);

                if (n < 0) {
                    LStat.r.no_errs++;
                    break;
                }

                LStat.r.no_bytes += n;
                LStat.r.no_msgs++;
                if (Req.access_recv)
                    touch_data(buf, Req.msg_size);

                if (Finished){
                    printf("child finished break:%d, Req.msg_size:%d\n", Finished, Req.msg_size);
                    stop_test_timer();
                    exchange_results();
                    //free(buf);
                    if (events_tcp_bw[i].data.fd >= 0)
                        close(events_tcp_bw[i].data.fd);
                    break;
                }


            } else {
                printf("unknown event: %8.8X\n", events_tcp_bw[i].events);
                return -1;
            }
        }
    }

    return 0;

}

/*
 * Measure stream bandwidth (server side).
 */
static void
stream_server_bw(KIND kind)
{
    int sockFD = -1;
    stream_server_init(&sockFD, kind);
    array_listenfd[sockFD] = sockFD;

#if 0
    char *buf = 0;
    *fd = accept(listenFD, 0, 0);
    if (*fd < 0)
        error(SYS, "accept failed");
    debug("accepted %s connection", kind_name(kind));
    set_socket_buffer_size(*fd);
    close(listenFD);
    debug("receiving to %s port %d", kind_name(kind), port);

    sync_test();
    buf = qmalloc(Req.msg_size);
    while (!Finished) {
        int n = recv_full(sockFD, buf, Req.msg_size);

        if (Finished)
            break;
        if (n < 0) {
            LStat.r.no_errs++;
            continue;
        }
        LStat.r.no_bytes += n;
        LStat.r.no_msgs++;
        if (Req.access_recv)
            touch_data(buf, Req.msg_size);
        printf("n:%d\n", n);
    }
    stop_test_timer();
    exchange_results();
    free(buf);
    if (sockFD >= 0)
        close(sockFD);
#endif

}


/*
 * Measure stream latency (client side).
 */
static void
stream_client_lat(KIND kind)
{
    char *buf;
    int sockFD;
    client_init(&sockFD, kind);
    buf = qmalloc(Req.msg_size);
    sync_test();
    while (!Finished) {
        int n = send_full(sockFD, buf, Req.msg_size);

        if (Finished)
            break;
        if (n < 0) {
            LStat.s.no_errs++;
            continue;
        }
        LStat.s.no_bytes += n;
        LStat.s.no_msgs++;

        n = recv_full(sockFD, buf, Req.msg_size);
        if (Finished)
            break;
        if (n < 0) {
            LStat.r.no_errs++;
            continue;
        }
        LStat.r.no_bytes += n;
        LStat.r.no_msgs++;
    }
    stop_test_timer();
    exchange_results();
    free(buf);
    close(sockFD);
    show_results(LATENCY);
}


/*
 * Measure stream latency (server side).
 */
static void
stream_server_lat(KIND kind)
{
    int sockFD = -1;
    char *buf = 0;

    stream_server_init(&sockFD, kind);
    array_listenfd[sockFD] = sockFD;

#if 0
    sync_test();
    buf = qmalloc(Req.msg_size);
    while (!Finished) {
        int n = recv_full(sockFD, buf, Req.msg_size);

        if (Finished)
            break;
        if (n < 0) {
            LStat.r.no_errs++;
            continue;
        }
        LStat.r.no_bytes += n;
        LStat.r.no_msgs++;

        n = send_full(sockFD, buf, Req.msg_size);
        if (Finished)
            break;
        if (n < 0) {
            LStat.s.no_errs++;
            continue;
        }
        LStat.s.no_bytes += n;
        LStat.s.no_msgs++;
    }
    stop_test_timer();
    exchange_results();
    free(buf);
    close(sockFD);
#endif

}


/*
 * Measure datagram bandwidth (client side).
 */
static void
datagram_client_bw(KIND kind)
{
    char *buf;
    int sockFD;

    client_init(&sockFD, kind);
    buf = qmalloc(Req.msg_size);
    sync_test();
    while (!Finished) {
        int n = write(sockFD, buf, Req.msg_size);

        if (Finished)
            break;
        if (n < 0) {
            LStat.s.no_errs++;
            continue;
        }
        LStat.s.no_bytes += n;
        LStat.s.no_msgs++;
    }
    stop_test_timer();
    exchange_results();
    free(buf);
    close(sockFD);
    show_results(BANDWIDTH_SR);
}


/*
 * Measure datagram bandwidth (server side).
 */
static void
datagram_server_bw(KIND kind)
{
    int sockFD;
    char *buf = 0;

    datagram_server_init(&sockFD, kind);
    sync_test();
    buf = qmalloc(Req.msg_size);
    while (!Finished) {
        int n = recv(sockFD, buf, Req.msg_size, 0);

        if (Finished)
            break;
        if (n < 0) {
            LStat.r.no_errs++;
            continue;
        }
        LStat.r.no_bytes += n;
        LStat.r.no_msgs++;
        if (Req.access_recv)
            touch_data(buf, Req.msg_size);
    }
    stop_test_timer();
    exchange_results();
    free(buf);
    close(sockFD);
}


/*
 * Measure datagram latency (client side).
 */
static void
datagram_client_lat(KIND kind)
{
    char *buf;
    int sockFD;

    client_init(&sockFD, kind);
    buf = qmalloc(Req.msg_size);
    sync_test();
    while (!Finished) {
        int n = write(sockFD, buf, Req.msg_size);

        if (Finished)
            break;
        if (n < 0) {
            LStat.s.no_errs++;
            continue;
        }
        LStat.s.no_bytes += n;
        LStat.s.no_msgs++;

        n = read(sockFD, buf, Req.msg_size);
        if (Finished)
            break;
        if (n < 0) {
            LStat.r.no_errs++;
            continue;
        }
        LStat.r.no_bytes += n;
        LStat.r.no_msgs++;
    }
    stop_test_timer();
    exchange_results();
    free(buf);
    close(sockFD);
    show_results(LATENCY);
}


/*
 * Measure datagram latency (server side).
 */
static void
datagram_server_lat(KIND kind)
{
    int sockfd;
    char *buf = 0;

    datagram_server_init(&sockfd, kind);
    sync_test();
    buf = qmalloc(Req.msg_size);
    while (!Finished) {
        SS clientAddr;
        socklen_t clientLen = sizeof(clientAddr);
        int n = recvfrom(sockfd, buf, Req.msg_size, 0,
                         (SA *)&clientAddr, &clientLen);

        if (Finished)
            break;
        if (n < 0) {
            LStat.r.no_errs++;
            continue;
        }
        LStat.r.no_bytes += n;
        LStat.r.no_msgs++;

        n = sendto(sockfd, buf, Req.msg_size, 0, (SA *)&clientAddr, clientLen);
        if (Finished)
            break;
        if (n < 0) {
            LStat.s.no_errs++;
            continue;
        }
        LStat.s.no_bytes += n;
        LStat.s.no_msgs++;
    }
    stop_test_timer();
    exchange_results();
    free(buf);
    close(sockfd);
}


/*
 * Set default IP parameters and ensure that any that are set are being used.
 */
static void
ip_parameters(long msgSize)
{
    setp_u32(0, L_MSG_SIZE, msgSize);
    setp_u32(0, R_MSG_SIZE, msgSize);
    par_use(L_PORT);
    par_use(R_PORT);
    par_use(L_SOCK_BUF_SIZE);
    par_use(R_SOCK_BUF_SIZE);
    opt_check();
}


void ff_client_init(void)
{
    int sockFD;
    client_init(&sockFD, K_TCP);
    array_listenfd[sockFD] = sockFD;
    /* Add to event list */
    assert((epfd_tcp_bw = ff_epoll_create(0)) > 0);
    ev_tcp_bw.data.fd = sockFD;
    ev_tcp_bw.events = EPOLLIN|EPOLLET|EPOLLOUT;
    //ff_epoll_ctl(epfd_tcp_bw, EPOLL_CTL_ADD, sockFD, &ev_tcp_bw);
    if (ff_epoll_ctl(epfd_tcp_bw, EPOLL_CTL_ADD, sockFD, &ev_tcp_bw) != 0) {
        printf("ff_client_ctl error\n");
    }
    printf("socdFD:%d\n", sockFD);
    //sync_test();

}

/*
 * Socket client initialization.
 */
static void
client_init(int *fd, KIND kind)
{
    uint32_t rport;
    AI *ai, *ailist;
    int iret;

    iret = recv_mesg(&rport, sizeof(rport), "port");
    rport = decode_uint32(&rport);
    printf("rport:%d, kind:%d\n", rport, kind);
    ailist = getaddrinfo_kind(0, kind, rport);
    for (ai = ailist; ai; ai = ai->ai_next) {
        printf("ai->ai_fimily:%d\n", ai->ai_family);
        if (!ai->ai_family)
            continue;
        *fd = socket(ai->ai_family, ai->ai_socktype, ai->ai_protocol);
        printf("child Listen:%d, family:%d, socktype:%d, protocol:%d\n", *fd, ai->ai_family, ai->ai_socktype, ai->ai_protocol);
        if(*fd<0){
            continue;
            printf("*fd:%d\n", *fd);
        }
	setsockopt_one(*fd, SO_REUSEADDR);

        //if (connect(*fd, ai->ai_addr, ai->ai_addrlen) == SUCCESS0)
        connect(*fd, ai->ai_addr, ai->ai_addrlen); 
        break;
        //close(*fd);
    }
    freeaddrinfo(ailist);
    if (!ai)
        error(0, "could not make %s connection to server", kind_name(kind));
    if (Debug) {
        uint32_t lport;
        get_socket_port(*fd, &lport);
        debug("sending from %s port %d to %d", kind_name(kind), lport, rport);
    }
}


/*
 * Socket server initialization.
 */
static void
stream_server_init(int *fd, KIND kind)
{
    uint32_t port;
    AI *ai;

    char ip[46];
    int ipbuf_len = 46;
    memset(&ip, 0x0, 46);
    char *ipbuf = ip;
    AI *ailist = getaddrinfo_kind(1, kind,  Req.port);
    for (ai = ailist; ai; ai = ai->ai_next) {
        if (!ai->ai_family)
            continue;
        *fd = socket(ai->ai_family, ai->ai_socktype, ai->ai_protocol);
        printf("child Listen:%d, family:%d, socktype:%d, protocol:%d\n", *fd, ai->ai_family, ai->ai_socktype, ai->ai_protocol);
        if (*fd < 0)
            continue;

        setsockopt_one(*fd, SO_REUSEADDR);
        int on = 1;
        ff_ioctl(*fd, FIONBIO, &on);
#if 0

        if (ai->ai_family == AF_INET) {
            struct sockaddr_in *sa = (struct sockaddr_in *)ai->ai_addr;
            sa->sin_addr.s_addr = htonl(INADDR_ANY);
            inet_ntop(AF_INET, &(sa->sin_addr), ipbuf, ipbuf_len);
        } else {
            struct sockaddr_in6 *sa = (struct sockaddr_in6 *)ai->ai_addr;
            inet_ntop(AF_INET6, &(sa->sin6_addr), ipbuf, ipbuf_len);
        }
        printf("%d,%s\n", ipbuf_len, ipbuf);
#endif

        if (bind(*fd, ai->ai_addr, ai->ai_addrlen) == SUCCESS0)
            break;
        close(*fd);
        *fd = -1;
    }

    freeaddrinfo(ailist);
    if (!ai)
        error(0, "unable to make %s socket", kind_name(kind));
    if (listen(*fd, 1) < 0)
        error(SYS, "listen failed");


    get_socket_port(*fd, &port);
    encode_uint32(&port, port);

    send_mesg(&port, sizeof(port), "port");
    
    assert((epfd_tcp_bw = ff_epoll_create(0)) > 0);
    ev_tcp_bw.data.fd = *fd;
    ev_tcp_bw.events = EPOLLIN | EPOLLOUT;
    ff_epoll_ctl(epfd_tcp_bw, EPOLL_CTL_ADD, *fd, &ev_tcp_bw);

#if 0
    *fd = accept(*fd, 0, 0);
    if (*fd < 0)
        error(SYS, "accept failed");
    debug("accepted %s connection", kind_name(kind));
    set_socket_buffer_size(*fd);
    close(*fd);
    debug("receiving to %s port %d", kind_name(kind), port);
#endif
    

}


/*
 * Datagram server initialization.
 */
static void
datagram_server_init(int *fd, KIND kind)
{
    uint32_t port;
    AI *ai;
    int sockfd = -1;

    AI *ailist = getaddrinfo_kind(1, kind, Req.port);
    for (ai = ailist; ai; ai = ai->ai_next) {
        if (!ai->ai_family)
            continue;
        sockfd = socket(ai->ai_family, ai->ai_socktype, ai->ai_protocol);
        if (sockfd < 0)
            continue;
        setsockopt_one(sockfd, SO_REUSEADDR);
        if (bind(sockfd, ai->ai_addr, ai->ai_addrlen) == SUCCESS0)
            break;
        close(sockfd);
        sockfd = -1;
    }
    freeaddrinfo(ailist);
    if (!ai)
        error(0, "unable to make %s socket", kind_name(kind));

    set_socket_buffer_size(sockfd);
    get_socket_port(sockfd, &port);
    encode_uint32(&port, port);
    send_mesg(&port, sizeof(port), "port");
    *fd = sockfd;
}


/*
 * A version of getaddrinfo that takes a numeric port and prints out an error
 * on failure.
 */
static AI *
getaddrinfo_kind(int serverflag, KIND kind, int port)
{
    AI *aip, *ailist;
    AI hints ={
        .ai_flags    = AI_NUMERICSERV,
        .ai_family   = AF_UNSPEC,
        .ai_socktype = SOCK_STREAM
    };
    memset(&hints, 0x0, sizeof(AI));

    if (serverflag)
        hints.ai_flags |= AI_PASSIVE;
    if (kind == K_UDP)
        hints.ai_socktype = SOCK_DGRAM;

    ailist = getaddrinfo_port(serverflag ? 0 : ServerName, port, &hints);
    for (aip = ailist; aip; aip = aip->ai_next) {
        if (kind == K_SDP) {
            if (aip->ai_family == AF_INET || aip->ai_family == AF_INET6)
                aip->ai_family = AF_INET_SDP;
            else
                aip->ai_family = 0;
        } else if (kind == K_SCTP) {
            if (aip->ai_protocol == IPPROTO_TCP)
                aip->ai_protocol = IPPROTO_SCTP;
            else
                aip->ai_family = 0;
        }
    }
    return ailist;
}


/*
 * Set both the send and receive socket buffer sizes.
 */
static void
set_socket_buffer_size(int fd)
{
    int size = Req.sock_buf_size;
    printf("size:%d\n", size);

    if (!size)
        return;
    if (setsockopt(fd, SOL_SOCKET, SO_SNDBUF, &size, sizeof(size)) < 0)
        error(SYS, "Failed to set send buffer size on socket");
    if (setsockopt(fd, SOL_SOCKET, SO_RCVBUF, &size, sizeof(size)) < 0)
        error(SYS, "Failed to set receive buffer size on socket");
}


/*
 * Given an open socket, return the port associated with it.  There must be a
 * more efficient way to do this that is portable.
 */
static void
get_socket_port(int fd, uint32_t *port)
{
    char p[NI_MAXSERV];
    SS sa;
    socklen_t salen = sizeof(sa);

    if (ff_getsockname(fd, (struct linux_sockaddr*)&sa, &salen) < 0)
        error(SYS, "getsockname2 failed");
    if (getnameinfo((SA *)&sa, salen, 0, 0, p, sizeof(p), NI_NUMERICSERV) < 0)
        error(SYS, "getnameinfo failed");
    *port = atoi(p);
    printf("fd:%d, port:%d\n", fd, *port);
    if (!port)
        error(0, "invalid port");
}


/*
 * Send a complete message to a socket.  A zero byte write indicates an end of
 * file which suggests that we are finished.
 */
static int
send_full(int fd, void *ptr, int len)
{
    int n = len;

    while (!Finished && n) {
        int i = ff_write(fd, ptr, 1000);
        printf("i:%d\n",i);

        if (i < 0)
            return len-n;
        ptr += i;
        n   -= i;
        if (i == 0)
            set_finished();
    }
    return len-n;
}


/*
 * Receive a complete message from a socket.  A zero byte read indicates an end
 * of file which suggests that we are finished.
 */
static int
recv_full(int fd, void *ptr, int len)
{
    int n = len;
    int i=0;
    int iret = 0;

    while (!Finished && n) {
        i = ff_read(fd, ptr, n);
        if(i >0){
           printf("i:%d, n:%d\n", i, n);
           iret += i;
        }
        if (i < 0){
            //printf("return -1, break:%d\n", iret);
            return iret;
        }
        ptr += i;
        n   -= i;
        if (i == 0)
            set_finished();
    }
    return len-n;
}


/*
 * Return the name of a transport kind.
 */
static char *
kind_name(KIND kind)
{
    if (kind < 0 || kind >= cardof(Kinds))
        return "unknown type";
    else
        return Kinds[kind];
}
