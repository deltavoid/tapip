#ifndef __INET_H
#define __INET_H

#include "socket.h"

struct inet_type {
	struct sock *(*alloc_sock)(int);
	int type;
	int protocol;
};

extern struct socket_ops inet_ops;
extern void inet_init(void);

extern int inet_socket(struct socket *sock, int protocol);
extern int inet_close(struct socket *sock);
extern int inet_listen(struct socket *sock, int backlog);
extern int inet_bind(struct socket *sock, struct sock_addr *skaddr);
extern int inet_connect(struct socket *sock, struct sock_addr *skaddr);
extern int inet_accept(struct socket *sock,	struct socket *newsock, struct sock_addr *skaddr);
extern int inet_read(struct socket *sock, void *buf, int len);
extern int inet_write(struct socket *sock, void *buf, int len);
extern int inet_send(struct socket *sock, void *buf, int size,struct sock_addr *skaddr);
extern struct pkbuf *inet_recv(struct socket *sock);


#endif	/* inet.h */
