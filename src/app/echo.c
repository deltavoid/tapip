#include "lib.h"
#include "netif.h"
#include "ip.h"
//#include "udp.h"
#include "route.h"
#include "socket.h"
#include "sock.h"
#include "netcfg.h"





void error(const char* msg)
{
    dbg("%s\n", msg);
    //exit(1);
}

extern void _set_recv_handler(struct sock* sk, recv_handler_t func, void* arg);

void echo_recv_handler(void* arg, struct sock* sk, void* buf, int len)
{
    sk->ops->send_buf(sk, buf, len, NULL);
}

int echo()
{
    #define BUF_SIZE 4096
    char buf[BUF_SIZE];

    struct socket* listenfd;
    if  ((listenfd = _socket(AF_INET, SOCK_STREAM, 0)) == NULL)   error("socket");


    struct sock_addr skaddr;
    const short PORT = 1234;
    skaddr.src_addr = FAKE_IXY_IPADDR;
    skaddr.src_port = _htons(PORT);

    if (_bind(listenfd, &skaddr) < 0) error("bind");

    const int BACKLOG = 10;
    if  (_listen(listenfd, BACKLOG) == -1)  error("listen");

    struct socket* newfd;
    if  ((newfd =  _accept(listenfd, &skaddr)) == NULL) error("accept");


    // int recv_len = 0, send_len = 0;
    // int total_recv = 0, total_send = 0;
    // while ((recv_len = _read(newfd, buf, BUF_SIZE)) > 0)
    // {
    //     send_len = _write(newfd, buf, recv_len);
    //     dbg("recv: %d  send: %d", recv_len, send_len);
        
    //     total_recv += recv_len;  total_send += send_len;
    //     dbg("total recv: %d  total send: %d", total_recv, total_send);
        

    // }
    
    _set_recv_handler(newfd->sk, echo_recv_handler, NULL);
    sleep(10000);


    

    _close(newfd);

    return 0;
}