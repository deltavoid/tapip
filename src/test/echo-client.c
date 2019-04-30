#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <netdb.h>

#include <stdint.h>
#include <stddef.h>
#include <time.h>


const short PORT = 1234;
#define MAX_BUF_SIZE 4096
char tx_buf[MAX_BUF_SIZE];
char rx_buf[MAX_BUF_SIZE];


int requests;
int buf_size = 1024;
int sock_fd;

void error(const char* msg)
{
    printf("%s\n", msg);
    exit(1);
}

// returns a timestamp in nanoseconds
// based on rdtsc on reasonably configured systems and is hence fast
uint64_t monotonic_time() {
	struct timespec timespec;
	clock_gettime(CLOCK_MONOTONIC, &timespec);
	return timespec.tv_sec * 1000 * 1000 * 1000 + timespec.tv_nsec;
}


void* sender(void* arg)
{
    for (int i = 0; i < buf_size; i++)
        tx_buf[i] = i;
    
    int send_len = 0;
    for (int i = 0; i < requests; i++)
    {
        send_len = send(sock_fd, tx_buf, buf_size, 0);
        usleep(1);

    }
       
    
    return NULL;
}

// bits/second
double calc_speeed(uint64_t last_time/*nanoseconds*/, uint64_t now_time, uint64_t bytes)
{
    double interval = now_time - last_time;
    double bits = bytes * 8;
    double speed  = bits / interval * 1000 * 1000 * 1000;
}

int main(int argc, char** argv)
{
    if  (argc < 3)  error("usage: client <hostname> <request num> [<buf size>]");

    struct hostent* he;
    if  ((he = gethostbyname(argv[1])) == NULL)  error("gethostbyname");

    sscanf(argv[2], "%d", &requests);
    printf("client %s %d\n", argv[1], requests);

    if  (argc >= 4)  sscanf(argv[4], "%d", &buf_size);



    if  ((sock_fd = socket(AF_INET, SOCK_STREAM, 0)) == -1)  error("socket");

    struct sockaddr_in their_addr;
    their_addr.sin_family = AF_INET;
    their_addr.sin_port = htons(PORT);
    their_addr.sin_addr = *((struct in_addr*)he->h_addr);
    bzero(&(their_addr.sin_zero), sizeof(their_addr.sin_zero));

    if  (connect(sock_fd, (struct sockaddr*)&their_addr, sizeof(struct sockaddr)) == -1)  error("connect");

    pthread_t tid;
    if  (pthread_create(&tid, NULL, sender, NULL) == -1)  error("pthread_create sender");

    uint64_t last_time = monotonic_time();
    int recv_len = 0;
    int total_len = buf_size * requests;
    while (recv_len < total_len)
    {
        uint64_t l = recv(sock_fd, rx_buf, buf_size, 0);

        uint64_t now_time = monotonic_time();
        printf("speed: %lf bits/sec\n", calc_speeed(last_time, now_time, l));
        last_time = now_time;

        recv_len += l;
        printf("recv_len: %d\n", recv_len);
    }
    
    pthread_join(tid, NULL);

    return 0;
}