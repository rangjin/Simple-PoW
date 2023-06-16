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
#include <ctype.h>
#include <time.h>
#include <stdlib.h>

#define ISVALIDSOCKET(s) ((s) >= 0)
#define CLOSESOCKET(s) close(s)
#define SOCKET int
#define GETSOCKETERRNO() (errno)

int main(int argc, char *argv[]) {
    // 프로그램 실행 인자 확인
    // 클라이언트 최대 갯수, data, 난이도를 인자로 입력
    // 잘못된 인자 수 입력시 에러메세지 출력 후 종료
    if (argc < 4) {
        fprintf(stderr, "usage: client_num data target\n");
        return 1;
    }

    // 로컬 주소 구성
    printf("Configuring local address...\n");
    struct addrinfo hints;
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;

    struct addrinfo *bind_address;
    getaddrinfo(0, "8080", &hints, &bind_address);

    // 소켓 생성
    printf("Creating socket...\n");
    SOCKET socket_listen;
    socket_listen = socket(bind_address->ai_family,
            bind_address->ai_socktype, bind_address->ai_protocol);
    // 소켓이 생성되지 않았거나, 유효하지 않다면 에러메세지 출력
    if (!ISVALIDSOCKET(socket_listen)) {
        fprintf(stderr, "socket() failed. (%d)\n", GETSOCKETERRNO());
        return 1;
    }

    // 소켓 바인딩
    printf("Binding socket to local address...\n");
    // 소켓 바인딩이 실패할경우, 에러메세지 출력
    if (bind(socket_listen,
                bind_address->ai_addr, bind_address->ai_addrlen)) {
        fprintf(stderr, "bind() failed. (%d)\n", GETSOCKETERRNO());
        return 1;
    }
    freeaddrinfo(bind_address);

    // 클라이언트 연결 대기, 최대 10개의 클라이언트를 받도록 설정
    // 연결 대기 실패시, 에러메세지 출력 후 프로그램 종료
    printf("Listening...\n");
    if (listen(socket_listen, 10) < 0) {
        fprintf(stderr, "listen() failed. (%d)\n", GETSOCKETERRNO());
        return 1;
    }

    // select 연산을 위해, fd_set 구조체 선언 후 초기화
    fd_set master;
    FD_ZERO(&master);
    FD_SET(socket_listen, &master);
    SOCKET max_socket = socket_listen;

    printf("Waiting for connections...\n");
    
    // 작업증명 초기화
    unsigned long long n, k = 100000L;
    int cnt = 0, max = atoi(argv[1]);
    const char *data = argv[2];
    const char *target = argv[3];
    char rBuff[1024], wBuff[4096];
    int state = 1;
    time_t begin, end;

    while(state) {
        fd_set reads;
        reads = master;
        // select 함수로 클라이언트 연결 요청 확인
        // select 동작시 에러가 발생할경우, 에러메세지 출력
        if (select(max_socket+1, &reads, 0, 0, 0) < 0) {
            fprintf(stderr, "select() failed. (%d)\n", GETSOCKETERRNO());
            return 1;
        }

        // 선형적으로 탐색하며 클라이언트 소켓에 대한 이벤트 확인
        SOCKET i;
        for (i = 1; i <= max_socket; ++i) {
            if (FD_ISSET(i, &reads)) {
                // 새로운 소켓에 대한 연결인경우, 클라이언트 연결 수락
                if (i == socket_listen) {
                    struct sockaddr_storage client_address;
                    socklen_t client_len = sizeof(client_address);
                    // accept 함수로 클라이언트 연결 수락
                    SOCKET socket_client = accept(socket_listen,
                            (struct sockaddr*) &client_address,
                            &client_len);
                    // socket_client가 유효하지 않은경우, 에러메세지를 출력하고 프로그램 종료
                    if (!ISVALIDSOCKET(socket_client)) {
                        fprintf(stderr, "accept() failed. (%d)\n",
                                GETSOCKETERRNO());
                        return 1;
                    }
                    // 연결 가능한 클라이언트 수가 남아있는 경우
                    if (cnt < max) {
                        // socket_client 소켓을 master에 추가 후, 최대 소켓 번호 업데이트
                        FD_SET(socket_client, &master);
                        if (socket_client > max_socket)
                            max_socket = socket_client;
                        cnt++;

                        printf("New connection from %d\n", socket_client);
                        write(socket_client, data, strlen(data));
                        sleep(1);
                        write(socket_client, target, strlen(data));
                    } 
                    // 연결 가능한 클라이언트 수가 모자란경우
                    else {
                        sprintf(wBuff, "Cannot connect to Server. It's fulled.\n");
                        write(socket_client, wBuff, strlen(wBuff));
                        CLOSESOCKET(socket_client);
                        continue;
                    }

                    // 작업증명이 처음 시작되는 경우, 시작 시간 저장
                    if (n == 0 && cnt == max) {
                        printf("start!!!\n");
                        sleep(1);
                        begin = time(NULL);
                        for (int j = 1; j <= max_socket; j++) {
                            if (FD_ISSET(j, &master) && j != socket_listen) {
                                sprintf(wBuff, "%llu", n);
                                write(j, wBuff, strlen(wBuff));
                                n = n + k;
                            }
                        }
                    }
                } 
                // 클라이언트 소켓에 대한 요청 처리
                else {
                    int bytes_received = read(i, rBuff, 1024);
                    // 클라이언트에서 전송한 데이터 확인
                    // 클라이언트로의 입력이 진행되지 않는경우, 연결 종료
                    if (bytes_received < 1) {
                        sprintf(wBuff, "End connection from %d\n", i);
                        FD_CLR(i, &master);
                        CLOSESOCKET(i);
                    } else {
                        rBuff[bytes_received] = '\0';
                    }

                    // Y를 수신한경우, 작업증명 종료임을 판단하고, end 시간 저장
                    printf("%s\n", rBuff);
                    if (rBuff[0] == 'Y') {
                        state = 0;
                        end = time(NULL);
                        for (int j = 1; j <= max_socket; j++) {
                            if (FD_ISSET(j, &master) && j != socket_listen) {
                                CLOSESOCKET(j);
                            }
                        }
                    } 
                    // 그렇지 않은 경우, 다음번 탐색을 시작할 수 증가.
                    else {
                        sprintf(wBuff, "%llu", n);
                        write(i, wBuff, strlen(wBuff));
                        n = n + k;
                    }                    
                }
            }
        }
    }
    
    // 실행시간 출력 후 소켓 닫기
    printf("time spent: %lf\n", (double) end - (double) begin);
    printf("Closing listening socket...\n");
    CLOSESOCKET(socket_listen);

    printf("Finished.\n");

    return 0;
}
