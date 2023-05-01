#include <stdio.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <arpa/inet.h>
#include <limits.h>
#include <openssl/sha.h>

void err_proc();

unsigned long long proofOfWork(const char *data, int target, unsigned long long start) {
    unsigned long long nonce;
    char input[SHA256_DIGEST_LENGTH + sizeof(unsigned long long)];
    unsigned char hash[SHA256_DIGEST_LENGTH];

    for (nonce = start; nonce < start + 100000; nonce++) {
        // printf("%lld\n", nonce);
        sprintf(input, "%s%llu", data, nonce);
        SHA256(input, strlen(input), hash);

        int state = 1, k = target / 2;
        for (int i = 0; i < k; i++) {
            if (hash[i] != 0x00) {
                state = 0;
            }
        }

        if (target % 2 != 0 && hash[k] > 0x0F) {
            state = 0;
        }

        if (state) {
            printf("%llu: ", nonce);
            for (int i = 0; i < SHA256_DIGEST_LENGTH; i++) {
                printf("%02x", hash[i]);
            }
            printf("\n");
            return nonce;
        }
    }

    return 0;
}

int main(int argc, char** argv)
{
	int clntSd, readLen, dataLen, targetLen, target;
	struct sockaddr_in clntAddr;
	char wBuff[BUFSIZ], rBuff[BUFSIZ], data[BUFSIZ], targetBuff[BUFSIZ];

    unsigned long long ans, start;

	if(argc != 3) {
		printf("Usage: %s [IP Address] [Port]\n", argv[0]);
	
	}
	clntSd = socket(AF_INET, SOCK_STREAM, 0);
	if(clntSd == -1) err_proc();
	printf("==== client program =====\n");

	memset(&clntAddr, 0, sizeof(clntAddr));
	clntAddr.sin_family = AF_INET;
	clntAddr.sin_addr.s_addr = inet_addr(argv[1]);
	clntAddr.sin_port = htons(atoi(argv[2]));

	if(connect(clntSd, (struct sockaddr *) &clntAddr, sizeof(clntAddr)) == -1)
	{
		close(clntSd);
		err_proc();	
	}

    dataLen = read(clntSd, data, sizeof(data));
    data[dataLen] = '\0';

    targetLen = read(clntSd, targetBuff, sizeof(targetBuff));
    targetBuff[targetLen] = '\0';
    target = atoi(targetBuff);

    printf("%s, %d\n", data, target);

    while(1) {
        readLen = read(clntSd, rBuff, sizeof(rBuff));
        rBuff[readLen] = '\0';
        printf("%s\n", rBuff);
        if (rBuff[0] == 'E') {
            break;
        }

        start = strtoull(rBuff, 0, 10);

        ans = proofOfWork(data, target, start);
        if (ans == 0ULL) {
            sprintf(wBuff, "N %lld", start);
            write(clntSd, wBuff, strlen(wBuff));
        } else {
            sprintf(wBuff, "Y %lld", ans);
            write(clntSd, wBuff, strlen(wBuff));
        }
	}

	close(clntSd);
	return 0;
}

void err_proc()
{
	fprintf(stderr,"Error: %s\n", strerror(errno));
	exit(errno);
} 
