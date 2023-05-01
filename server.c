#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <string.h>
#include <sys/wait.h>
#include <pthread.h>
#include <limits.h>

int i = 0, k = 100000, cnt = 0, state = 1;
const char *data, *target;
unsigned long long ans = ULLONG_MAX;

void * client_module(void * sd) {
	char rBuff[BUFSIZ], tmp[12];
	int readLen;
	int connectSd = *((int *) sd);

	write(connectSd, data, strlen(data));
	sleep(2);
	write(connectSd, target, strlen(target));
	sleep(2);

	while(state) {
        sprintf(tmp, "%d", i);
        write(connectSd, tmp, sizeof(tmp));
        i = i + k;

		readLen = read(connectSd, rBuff, sizeof(rBuff) - 1);
		rBuff[readLen] = '\0';
		printf("Client(%d): %s\n", connectSd, rBuff);
		if (rBuff[0] == 'Y') {
			state = 0;
			char can[sizeof(rBuff) - 2];
			int j = 2;
			for (int j = 0; j < sizeof(rBuff) - 2; j++) {
				can[j] = rBuff[j + 2];
			}
			unsigned long long candidate = strtoull(can, 0, 10);
			if (candidate < ans) {
				ans = candidate;
			}

			break;
		}
	}

	sprintf(tmp, "%s", "END");
	write(connectSd, tmp, sizeof(tmp));
	sleep(2);

	fprintf(stderr, "The client is disconnected.\n");
	cnt--;
	close(connectSd);

	if (cnt == 0) {
		printf("ans = %lld\n", ans);
		exit(0);
	}
}

int main(int argc, char** argv)
{
    data = "201928492019284920192849";
    target = "7";

	int listenSd, connectSd, clntAddrLen, strLen;
	struct sockaddr_in srvAddr, clntAddr;

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
		}	

		cnt++;
		pthread_create(&thread, NULL, client_module, (void *) &connectSd);
		pthread_detach(thread);
	}

	close(listenSd);
	return 0;
}
