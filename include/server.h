#ifndef server
#define server

#include <sys/socket.h>
#include <sys/types.h>
#include <stdio.h>
#include <netinet/in.h>
#include <netdb.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#define SZBUFSZ 255

struct accessServer {
    int sockfd, clilen, accsockfd;
    struct sockaddr_in serv_addr, cli_addr;
    unsigned char* szbuffer;
    unsigned char* framebuffer;
    int framesize;
};

int initServer(const int port, struct accessServer *serverStr);

int termServer(struct accessServer *serverStr);

unsigned char sendData(unsigned char *buffer, int framesize, struct accessServer *serverStr);

int initClient(struct accessServer *clientStr, int port, char *servername);

int termClient(struct accessServer *clientStr);

int recvData(struct accessServer *serverStr);
int returnNet(unsigned char value, struct accessServer *serverStr);

#endif
