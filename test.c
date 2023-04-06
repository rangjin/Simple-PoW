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
    int target = 8;
    double timeSpent = 0.0;

    time_t begin = time(NULL);
    unsigned int nonce = proofOfWork(data, target);
    time_t end = time(NULL);

    printf("\nnonce: %u, time spent: %lf\n", nonce, (double) end - (double) begin);

    return 0;
}