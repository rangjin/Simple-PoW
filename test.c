#include <stdio.h>
#include <time.h>
#include <limits.h>
#include <string.h>
#include <openssl/sha.h>

unsigned int proofOfWork(const char *data, int target) {
    unsigned int nonce;
    char input[SHA256_DIGEST_LENGTH + sizeof(unsigned int)];
    unsigned char hash[SHA256_DIGEST_LENGTH];

    for (nonce = 0; nonce < UINT_MAX; nonce++) {
        sprintf(input, "%s%u", data, nonce);
        SHA256(input, strlen(input), hash);

        int state = 1;
        // printf("%d: ", nonce);
        for (int i = 0; i < target; i++) {
            // printf("%02x", hash[i]);
            if (hash[i] != 0) {
                state = 0;
            }
        }

        if (state) {
            printf("%d: ", nonce);
            for (int i = 0; i < SHA256_DIGEST_LENGTH; i++) {
                printf("%02x", hash[i]);
            }
            return nonce;
        }
    }

    return 0;
}

int main() {
    const char *data = "201928492019284920192849";
    int target = 3;
    double timeSpent = 0.0;
    time_t begin = time(NULL);
    unsigned int nonce = proofOfWork(data, target);

    time_t end = time(NULL);

    printf("nonce: %u, time spent: %lf\n", nonce, (double) (end - begin));

    return 0;
}