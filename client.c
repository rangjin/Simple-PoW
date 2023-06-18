#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <unistd.h>
#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <openssl/sha.h>
#include <pthread.h>

#define ISVALIDSOCKET(s) ((s) >= 0)
#define CLOSESOCKET(s) close(s)
#define SOCKET int
#define GETSOCKETERRNO() (errno)

// 스레드에 넘겨줄 인자
typedef struct MultipleArg {
    char *data;
    int target;
    unsigned long long start;
} ARGS;

// 입력 데이터와 목표값에 대한 Proof of Work 작업 수행
// 작업 결과로 nonce 리턴
void *poWThread(void *arg) {
    ARGS* pp = (ARGS*) arg;
    unsigned long long start = pp->start;
    int target = pp->target;
    char *data = pp->data;
    unsigned long long nonce;
    char input[SHA256_DIGEST_LENGTH + sizeof(unsigned long long)];
    unsigned char hash[SHA256_DIGEST_LENGTH];

    // 이만 범위의 반복을 통해 해시 계산
    for (nonce = start; nonce < start + 20000ULL; nonce++) {
        sprintf(input, "%s%llu", data, nonce); // input 문자열 생성
        SHA256(input, strlen(input), hash); // 문자열 해시 계산

        // 작업 증명
        int state = 1, k = target / 2;
        for (int i = 0; i < k; i++) {
            if (hash[i] != 0x00) {
                state = 0;
            }
        }

        if (target % 2 != 0 && hash[k] > 0x0F) {
            state = 0;
        }

        // 작업 증명 성공 시 해당 nonce 리턴
        if (state) {
            printf("%llu: ", nonce);
            for (int i = 0; i < SHA256_DIGEST_LENGTH; i++) {
                printf("%02x", hash[i]);
            }
            printf("\n");
            return (void *)nonce;
        }
    }
    
    // 실패 시 0 리턴
    return (void *)0;
}

int main(int argc, char *argv[]) {
    char data[BUFSIZ], targetBuff[BUFSIZ];
    int target, dataLen, targetLen;
    
    // 프로그램 실행 인자 오류 처리
    if (argc < 3) {
        fprintf(stderr, "usage: tcp_client hostname port\n");
        return 1;
    }

    printf("Configuring remote address...\n");
    // 원격지 주소 설정(서버)
    // 연결할 서버의 IP 및 포트번호를 설정합니다.
    struct addrinfo hints;
    memset(&hints, 0, sizeof(hints));
    hints.ai_socktype = SOCK_STREAM;
    struct addrinfo *peer_address;
    if (getaddrinfo(argv[1], argv[2], &hints, &peer_address)) {
        fprintf(stderr, "getaddrinfo() failed. (%d)\n", GETSOCKETERRNO());
        return 1;
    }

    // 원격 주소 출력
    printf("Remote address is: ");
    char address_buffer[100];
    char service_buffer[100];
    getnameinfo(peer_address->ai_addr, peer_address->ai_addrlen,
            address_buffer, sizeof(address_buffer),
            service_buffer, sizeof(service_buffer),
            NI_NUMERICHOST);
    printf("%s %s\n", address_buffer, service_buffer);

    // 소켓 생성
    printf("Creating socket...\n");
    SOCKET socket_peer;
    socket_peer = socket(peer_address->ai_family,
            peer_address->ai_socktype, peer_address->ai_protocol);
    // 소켓 생성 실패시, 에러메세지 출력
    if (!ISVALIDSOCKET(socket_peer)) {
        fprintf(stderr, "socket() failed. (%d)\n", GETSOCKETERRNO());
        return 1;
    }

    // 소켓을 원격 서버에 연결
    printf("Connecting...\n");
    // 서버 연결 실패시, 에러메세지 출력
    if (connect(socket_peer,
                peer_address->ai_addr, peer_address->ai_addrlen)) {
        fprintf(stderr, "connect() failed. (%d)\n", GETSOCKETERRNO());
        return 1;
    }
    // 연결 성공시 peer_address 해제
    freeaddrinfo(peer_address);

    // 서버로부터 데이터 수신(연결 가능 확인)
    dataLen = read(socket_peer, data, sizeof(data));
    data[dataLen] = '\0';
    // 서버가 처리가능한 클라이언트 수를 넘어설경우, 에러메세지 출력
    if (strcmp(data, "Cannot connect to Server. It's fulled.\n") == 0) {
        printf("%s", data);
        CLOSESOCKET(socket_peer);

        printf("Finished.\n");
        return 0;
    } else {
        printf("Connected.\n");
    }

    // 서버로부터 목표값을 읽어와, atoi 함수를 이용해 정수로 변환 후 저장
    targetLen = read(socket_peer, targetBuff, sizeof(targetBuff));
    targetBuff[targetLen] = '\0';
    target = atoi(targetBuff);

    printf("%s, %d\n", data, target);

    char rBuff[BUFSIZ], wBuff[BUFSIZ];
    unsigned long long ans = 0ULL, start, res;
    
    // 멀티 스레드에 사용될 스레드 배열과 각 스레드에 들어갈 인자들을 담은 구조체
    pthread_t thread[5];
    ARGS args[5];

    // 작업증명 작업 진행
    while(1) {
        // 원격 서버로부터 데이터 획득
        int readLen = read(socket_peer, rBuff, sizeof(rBuff));
        // 데이터 획득 실패시, 연결 종료
        if (readLen < 1) {
            printf("Connection closed by peer.\n");
            break;
        }
        rBuff[readLen] = '\0';

        start = strtoull(rBuff, 0, 10);
        printf("%lld\n", start);

        // 멀티 스레드 생성
        for (int i = 0; i < 5; i++) {
            args[i].data = data;
            args[i].target = target;
            args[i].start = start + (20000ULL * (unsigned long long)i);
            pthread_create(&thread[i], NULL, poWThread, (void *)&args[i]);
        }

        // 각 스레드의 작업 결과 판단
        for (int i = 0; i < 5; i++) {
            pthread_join(thread[i], (void **) &res);
            if (res != 0ULL) {
                ans = res;
            }
        }

        // 결과 종합, 서버에 전달
        if (ans == 0ULL) {
            // 할당된 범위 내에서 작업 증명 실패 시
            sprintf(wBuff, "N %lld", start);
            write(socket_peer, wBuff, strlen(wBuff));
        } else {
            // 할당된 범위 내에서 작업 증명 성공 시
            sprintf(wBuff, "Y %lld", ans);
            write(socket_peer, wBuff, strlen(wBuff));
        }
    }

    // 소켓 close, 프로그램 종료
    printf("Closing socket...\n");
    CLOSESOCKET(socket_peer);

    printf("Finished.\n");
    return 0;
}

