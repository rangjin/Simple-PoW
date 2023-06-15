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

unsigned long long proofOfWork(const char *data, int target, unsigned long long start) {
    unsigned long long nonce;
    char input[SHA256_DIGEST_LENGTH + sizeof(unsigned long long)];
    unsigned char hash[SHA256_DIGEST_LENGTH];

    for (nonce = start; nonce < start + 100000; nonce++) {
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


int main(int argc, char *argv[]) {
    char data[BUFSIZ], targetBuff[BUFSIZ];
    int dataLen, targetLen, target;

    if (argc < 3) {
        fprintf(stderr, "usage: tcp_client hostname port\n");
        return 1;
    }

    printf("Configuring remote address...\n");
    struct addrinfo hints;
    memset(&hints, 0, sizeof(hints));
    hints.ai_socktype = SOCK_STREAM;
    struct addrinfo *peer_address;
    if (getaddrinfo(argv[1], argv[2], &hints, &peer_address)) {
        fprintf(stderr, "getaddrinfo() failed. (%d)\n", GETSOCKETERRNO());
        return 1;
    }

    printf("Remote address is: ");
    char address_buffer[100];
    char service_buffer[100];
    getnameinfo(peer_address->ai_addr, peer_address->ai_addrlen,
            address_buffer, sizeof(address_buffer),
            service_buffer, sizeof(service_buffer),
            NI_NUMERICHOST);
    printf("%s %s\n", address_buffer, service_buffer);

    printf("Creating socket...\n");
    SOCKET socket_peer;
    socket_peer = socket(peer_address->ai_family,
            peer_address->ai_socktype, peer_address->ai_protocol);
    if (!ISVALIDSOCKET(socket_peer)) {
        fprintf(stderr, "socket() failed. (%d)\n", GETSOCKETERRNO());
        return 1;
    }

    printf("Connecting...\n");
    if (connect(socket_peer,
                peer_address->ai_addr, peer_address->ai_addrlen)) {
        fprintf(stderr, "connect() failed. (%d)\n", GETSOCKETERRNO());
        return 1;
    }
    freeaddrinfo(peer_address);

    dataLen = read(socket_peer, data, sizeof(data));
    data[dataLen] = '\0';

    if (strcmp(data, "Cannot connect to Server. It's fulled.\n") == 0) {
        printf("%s", data);
        CLOSESOCKET(socket_peer);

        printf("Finished.\n");
        return 0;
    } else {
        printf("Connected.\n");
    }

    targetLen = read(socket_peer, targetBuff, sizeof(targetBuff));
    targetBuff[targetLen] = '\0';
    target = atoi(targetBuff);

    printf("%s, %d\n", data, target);

    char rBuff[BUFSIZ], wBuff[BUFSIZ];
    unsigned long long ans, start;

    while(1) {
        int readLen = read(socket_peer, rBuff, sizeof(rBuff));
        if (readLen < 1) {
            printf("Connection closed by peer.\n");
            break;
        }
        rBuff[readLen] = '\0';

        start = strtoull(rBuff, 0, 10);
        printf("%lld\n", start);
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

