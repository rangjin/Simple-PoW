/*
 * MIT License
 *
 * Copyright (c) 2018 Lewis Van Winkle
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

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

#define ISVALIDSOCKET(s) ((s) >= 0)
#define CLOSESOCKET(s) close(s)
#define SOCKET int
#define GETSOCKETERRNO() (errno)

// 입력 데이터와 목표값에 대한 Proof of Work 작업 수행
// 작업 결과로 nonce 리턴
unsigned long long proofOfWork(const char *data, int target, unsigned long long start) {
    unsigned long long nonce;
    char input[SHA256_DIGEST_LENGTH + sizeof(unsigned long long)];
    unsigned char hash[SHA256_DIGEST_LENGTH];

    // 십만 범위의 반복을 통해 해시 계산
    for (nonce = start; nonce < start + 100000; nonce++) {
        sprintf(input, "%s%llu", data, nonce); // input 문자열 생성
        SHA256(input, strlen(input), hash); // 문자열 해시 계산

        int state = 1, k = target / 2;
        for (int i = 0; i < k; i++) {
            if (hash[i] != 0x00) {
                state = 0;
            }
        }

        if (target % 2 != 0 && hash[k] > 0x0F) {
            state = 0;
        }
        // nonce 검증 성공
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


int main(int argc, char *argv[]) {
    char data[BUFSIZ], targetBuff[BUFSIZ];
    int dataLen, targetLen, target;
    
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
    unsigned long long ans, start;
    
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

        // 획득한 데이터를 부호없는 10진수로 변환해 저장.
        start = strtoull(rBuff, 0, 10);
        printf("%lld\n", start);

        // 작업증명 수행
        ans = proofOfWork(data, target, start);

        if (ans == 0ULL) {
            sprintf(wBuff, "N %lld", start);
            write(socket_peer, wBuff, strlen(wBuff));
        } else {
            sprintf(wBuff, "Y %lld", ans);
            write(socket_peer, wBuff, strlen(wBuff));
        }
    }

    printf("Closing socket...\n");
    CLOSESOCKET(socket_peer);

    printf("Finished.\n");
    return 0;
}

