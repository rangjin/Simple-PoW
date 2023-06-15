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
    if (argc < 4) {
        fprintf(stderr, "usage: client_num data target\n");
        return 1;
    }

    printf("Configuring local address...\n");
    struct addrinfo hints;
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;

    struct addrinfo *bind_address;
    getaddrinfo(0, "8080", &hints, &bind_address);

    printf("Creating socket...\n");
    SOCKET socket_listen;
    socket_listen = socket(bind_address->ai_family,
            bind_address->ai_socktype, bind_address->ai_protocol);
    if (!ISVALIDSOCKET(socket_listen)) {
        fprintf(stderr, "socket() failed. (%d)\n", GETSOCKETERRNO());
        return 1;
    }

    printf("Binding socket to local address...\n");
    if (bind(socket_listen,
                bind_address->ai_addr, bind_address->ai_addrlen)) {
        fprintf(stderr, "bind() failed. (%d)\n", GETSOCKETERRNO());
        return 1;
    }
    freeaddrinfo(bind_address);

    printf("Listening...\n");
    if (listen(socket_listen, 10) < 0) {
        fprintf(stderr, "listen() failed. (%d)\n", GETSOCKETERRNO());
        return 1;
    }

    fd_set master;
    FD_ZERO(&master);
    FD_SET(socket_listen, &master);
    SOCKET max_socket = socket_listen;

    printf("Waiting for connections...\n");

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
        if (select(max_socket+1, &reads, 0, 0, 0) < 0) {
            fprintf(stderr, "select() failed. (%d)\n", GETSOCKETERRNO());
            return 1;
        }

        SOCKET i;
        for (i = 1; i <= max_socket; ++i) {
            if (FD_ISSET(i, &reads)) {
                if (i == socket_listen) {
                    struct sockaddr_storage client_address;
                    socklen_t client_len = sizeof(client_address);
                    SOCKET socket_client = accept(socket_listen,
                            (struct sockaddr*) &client_address,
                            &client_len);
                    if (!ISVALIDSOCKET(socket_client)) {
                        fprintf(stderr, "accept() failed. (%d)\n",
                                GETSOCKETERRNO());
                        return 1;
                    }

                    if (cnt < max) {
                        FD_SET(socket_client, &master);
                        if (socket_client > max_socket)
                            max_socket = socket_client;
                        cnt++;

                        printf("New connection from %d\n", socket_client);
                        write(socket_client, data, strlen(data));
                        sleep(1);
                        write(socket_client, target, strlen(data));
                    } else {
                        sprintf(wBuff, "Cannot connect to Server. It's fulled.\n");
                        write(socket_client, wBuff, strlen(wBuff));
                        CLOSESOCKET(socket_client);
                        continue;
                    }

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
                } else {
                    int bytes_received = read(i, rBuff, 1024);
                    if (bytes_received < 1) {
                        sprintf(wBuff, "End connection from %d\n", i);
                        FD_CLR(i, &master);
                        CLOSESOCKET(i);
                    } else {
                        rBuff[bytes_received] = '\0';
                    }

                    printf("%s\n", rBuff);
                    if (rBuff[0] == 'Y') {
                        state = 0;
                        end = time(NULL);
                        for (int j = 1; j <= max_socket; j++) {
                            if (FD_ISSET(j, &master) && j != socket_listen) {
                                CLOSESOCKET(j);
                            }
                        }
                    } else {
                        sprintf(wBuff, "%llu", n);
                        write(i, wBuff, strlen(wBuff));
                        n = n + k;
                    }                    
                }
            }
        }
    }

    printf("time spent: %lf\n", (double) end - (double) begin);
    printf("Closing listening socket...\n");
    CLOSESOCKET(socket_listen);

    printf("Finished.\n");

    return 0;
}
