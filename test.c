#include <stdio.h>
#include <time.h>
#include <limits.h>
#include <string.h>
#include <openssl/sha.h>

unsigned long long proofOfWork(const char *data, int target) {
    unsigned long long nonce;
    char input[SHA256_DIGEST_LENGTH + sizeof(unsigned long long)];
    unsigned char hash[SHA256_DIGEST_LENGTH];

    for (nonce = 0; nonce < ULLONG_MAX; nonce++) {
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
    unsigned long long nonce = proofOfWork(data, target);
    time_t end = time(NULL);

    printf("\nnonce: %llu, time spent: %lf\n", nonce, (double) end - (double) begin);

    return 0;
}

/*

201928492019284920192849

target = 7
76274466: 0000000fa5ae90e92086f6f84a8bcce56217040f872830ce26499df9d1ac4dfb
nonce: 76274466, time spent: 21.000000

target = 8
8322403718: 000000008c651f58b3076c78ad324a7ea0b5824fe04da0cf05b2092d632b31ea
nonce: 8322403718, time spent: 2371.000000

*/