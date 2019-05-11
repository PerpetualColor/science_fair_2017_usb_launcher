#include "../include/server.h"

void errorServ(char *msg) {
	perror(msg);
	exit(EXIT_FAILURE);
}

int initServer(const int port, struct accessServer *serverStr) {
	// start the server on host machine
	printf("Initializing server\n");
	serverStr->szbuffer = malloc(SZBUFSZ);
	int err;
	serverStr->sockfd = socket(AF_INET, SOCK_STREAM, 0);
	if (serverStr->sockfd < 0) 
		errorServ("ERROR opening socket");
	memset(&serverStr->serv_addr, 0, sizeof(serverStr->serv_addr));
	serverStr->serv_addr.sin_family = AF_INET;
	serverStr->serv_addr.sin_addr.s_addr = INADDR_ANY;
	serverStr->serv_addr.sin_port = htons(port);
	if (bind(serverStr->sockfd, (struct sockaddr *) &serverStr->serv_addr,
				sizeof(serverStr->serv_addr)) < 0) 
		errorServ("ERROR on binding");
	listen(serverStr->sockfd,5);
	serverStr->clilen = sizeof(serverStr->cli_addr);
	serverStr->accsockfd = accept(serverStr->sockfd, (struct sockaddr *) &serverStr->cli_addr, &serverStr->clilen);
	if (serverStr->accsockfd < 0) 
		errorServ("ERROR on accept");
	return 0; 


}
int termServer(struct accessServer *serverStr) {
	printf("Closing server\n");
	// kill the server on host machine
	close(serverStr->accsockfd);
	close(serverStr->sockfd);
}
int initClient(struct accessServer *clientStr, int port, char *serverName) {
	int err;

	struct hostent *serverHost;
	clientStr->szbuffer = malloc(SZBUFSZ);
	clientStr->sockfd = socket(AF_INET, SOCK_STREAM, 0);
	if (clientStr->sockfd < 0) 
		errorServ("ERROR opening socket");
	serverHost = gethostbyname(serverName);
	if (serverHost == NULL) {
		fprintf(stderr,"ERROR, no such host\n");
		exit(0);
	}
	bzero((char *) &clientStr->serv_addr, sizeof(clientStr->serv_addr));
	clientStr->serv_addr.sin_family = AF_INET;
	bcopy((char *)serverHost->h_addr, 
			(char *)&clientStr->serv_addr.sin_addr.s_addr,
			serverHost->h_length);
	clientStr->serv_addr.sin_port = htons(port);
	if (connect(clientStr->sockfd,(struct sockaddr *)&clientStr->serv_addr,sizeof(clientStr->serv_addr)) < 0) 
		errorServ("ERROR connecting");
	return 0;

}
int termClient(struct accessServer *clientStr) {
	close(clientStr->sockfd);
	return 0;
}
int recvSize(struct accessServer *serverStr) {
	int buffer, err;
	err = recv(serverStr->sockfd, &buffer, 4, 0);
	if (err < 0) {
		printf("Error recv\n");
	}
	printf("Buffer: %d\n", buffer);
}
int recvData(struct accessServer *clientStr) {
	memset(clientStr->szbuffer, 0, SZBUFSZ);
	int err;
	// receive framesize 
	err = recv(clientStr->sockfd, clientStr->szbuffer, SZBUFSZ, 0);
	int cliFramesize;
	memcpy(&cliFramesize, clientStr->szbuffer, 4);
	printf("Framesize: %d\n", cliFramesize);
	clientStr->framebuffer = malloc(cliFramesize);
	clientStr->framesize = cliFramesize;
	int totalRecv = 0;
	int offset = 0;
	recv:
	err = recv(clientStr->sockfd, clientStr->framebuffer + offset, clientStr->framesize, 0);
	printf("Recieved %d bytes\n", err);
	if ((totalRecv = totalRecv + err) < cliFramesize) {
		offset += err;
		goto recv;
	}
	if (err < 0) {
		errorServ("ERROR recv buffer");
	}
	if (totalRecv != cliFramesize) {
		fprintf(stderr, "ERROR: Should have received %d bytes, instead received %d\n", cliFramesize, err);
	}
	return 0;

}

unsigned char sendData(unsigned char *buffer, int framesize, struct accessServer *serverStr) {
	// send data
	int err;
	err = send(serverStr->accsockfd, &framesize, 255, 0);
	printf("Sending %d bytes from %p\n", framesize, buffer);
	err = send(serverStr->accsockfd, buffer, framesize, 0);
	if (err < 0) {
		errorServ("ERROR sending");
	}
	printf("Sent data\n");
	unsigned char returnValue;
	err = recv(serverStr->accsockfd, &returnValue, 1, 0);
	if (err < 0) {
		errorServ("ERROR receiving");
	}
	printf("Received value: %d\n", returnValue, err);
	return returnValue;

}

int returnNet(unsigned char value, struct accessServer *clientStr) {
	int err;
	err = send(clientStr->sockfd, &value, 1, 0);
	if (err < 0) {
		errorServ("ERROR sending");
	} else {
		printf("Sent value %d\n", value);
	}
}
