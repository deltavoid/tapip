#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <sys/types.h>
#include <pthread.h>

#include "netif.h"
#include "ether.h"
#include "ip.h"
#include "arp.h"
#include "route.h"
#include "lib.h"

extern void test_shell(char *);

static pthread_t threads[2];

/*
 * this func will not be used, we use multi-thread model insteal of multi-process
 */
int newproc(void *(*proc)(void), int notty)
{
	int pid;
	pid = fork();
	if (pid < 0) {
		perror("fork");
		exit(EXIT_FAILURE);
	} else if (pid == 0) {		/* child */
		/* new process has no controlling tty */
		if (notty && setsid() == -1) {
			perror("setsid");
			exit(EXIT_FAILURE);
		}
		close(0);		/* kill stdin */
		proc();
		exit(0);
	}
	/* return child pid, and parent runs continuely */
	return pid;
}

typedef void *(*pfunc_t)(void *);

int newthread(pfunc_t thread_func)
{
	pthread_t tid;
	if (pthread_create(&tid, NULL, thread_func, NULL))
		perrx("pthread_create");
	return tid;
}

void net_stack_init(void)
{
	netdev_init();
	arp_cache_init();
	rt_init();
}

void net_stack_run(void)
{
	/* create timer thread */
	threads[0] = newthread((pfunc_t)net_timer);
	/* create netdev thread */
	threads[1] = newthread((pfunc_t)netdev_interrupt);
	/* net shell runs! */
	test_shell(NULL);
}

void net_stack_exit(void)
{
	if (pthread_cancel(threads[0]))
		perror("kill child 0");
	if (pthread_cancel(threads[1]))
		perror("kill child 1");
}

int main(int argc, char **argv)
{
	struct pkbuf *pkb;
	int len;

	net_stack_init();
	net_stack_run();
	net_stack_exit();

	return 0;
}