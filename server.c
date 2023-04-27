#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <string.h>
#include <sys/wait.h>
#include <pthread.h>

int i = 0, k = 100000;
int state = 1;

void * client_module(void * data)
{
	char rBuff[BUFSIZ];
	int readLen;
	int connectSd;
	connectSd = *((int *) data);
	while(state)
	{
        char tmp[12] = {0x0};
        sprintf(tmp, "%d", i);
        write(connectSd, tmp, sizeof(tmp));
        i = i + k;

		readLen = read(connectSd, rBuff, sizeof(rBuff)-1);
		if(readLen <= 0) break;	
		rBuff[readLen] = '\0';
		printf("Client(%d): %s\n",connectSd,rBuff);
		if (rBuff[0] == 'a') {
			state = 0;
			break;
		}
	}
	fprintf(stderr,"The client is disconnected.\n");
	close(connectSd);
}

int main(int argc, char** argv)
{
    const char *data = "201928492019284920192849";
    const char *target = "7";

	int listenSd, connectSd;
	struct sockaddr_in srvAddr, clntAddr;
	int clntAddrLen, strLen;

	pthread_t thread;
	
	if(argc != 2)
	{
		printf("Usage: %s [Port Number]\n", argv[0]);
		return -1;
	}

	printf("Server start...\n");
	listenSd = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);
	
	memset(&srvAddr, 0, sizeof(srvAddr));
	srvAddr.sin_addr.s_addr = htonl(INADDR_ANY);
	srvAddr.sin_family = AF_INET;
	srvAddr.sin_port = htons(atoi(argv[1]));

	bind(listenSd, (struct sockaddr *) &srvAddr, sizeof(srvAddr));
	listen(listenSd, 5);
	
	clntAddrLen = sizeof(clntAddr);
	while(1) {
		connectSd = accept(listenSd, (struct sockaddr *) &clntAddr, &clntAddrLen);
		if(connectSd == -1) {
			continue;
		} else {
			printf("A client is connected...\n");
			write(connectSd, data, strlen(data));
			sleep(2);
			write(connectSd, target, strlen(target));
			sleep(3);
		}	

		pthread_create(&thread, NULL, client_module, (void *) &connectSd);
		pthread_detach(thread);
	}
	close(listenSd);
	return 0;
}
