#include <stdio.h>
#include <time.h>
#include <limits.h>
#include <string.h>
#include <openssl/sha.h>
#include <stdlib.h>

// PoW 작업 진행 함수
unsigned long long proofOfWork(const char *data, int target) {
    unsigned long long nonce; // nonce 값을 저장하기 위한 변수

    // 해시 계산용 변수 설정
    char input[SHA256_DIGEST_LENGTH + sizeof(unsigned long long)]; 
    unsigned char hash[SHA256_DIGEST_LENGTH];


    for (nonce = 0; nonce < ULLONG_MAX; nonce++) {
        // 100,000 단위로 작업 진행중인 값 출력 
        if (nonce % 100000ULL == 0ULL) {
            printf("%llu\n", nonce);
        }
        sprintf(input, "%s%llu", data, nonce); // 데이터와 nonce 값을 결합하여 입력 생성
        SHA256(input, strlen(input), hash); // 입력의 해시 계산

        // 타겟 값과 비교하여 proof-of-work 조건 충족 여부 판단
        int state = 1, k = target / 2;
        // 앞자리 수 target 개가 0개인지 확인
        for (int i = 0; i < k; i++) {
            if (hash[i] != 0x00) {
                state = 0;
            }
        }
        // 만일 target이 홀수인경우, 마지막 수가 0인지 확인
        if (target % 2 != 0 && hash[k] > 0x0F) {
            state = 0;
        }
        // PoW 조건에 일치하는 nonce 확인시
        if (state) {
            printf("%llu: ", nonce);
            for (int i = 0; i < SHA256_DIGEST_LENGTH; i++) {
                printf("%02x", hash[i]); // 찾은 유효한 해시 출력
            }
            return nonce; // 유효한 nonce 값을 반환하고 종료
        }
    }

    return 0; // 유효한 nonce를 찾지 못한 경우 0을 반환
}

int main(int argc, char *argv[]) {
    // 잘못된 인자 에러 처리
    if (argc < 3) {
        fprintf(stderr, "usage: data target\n");
        return 1;
    }

    const char *data = argv[1]; // 입력 데이터
    const char *target_array = argv[2]; // 목표 타겟 값
    int target = atoi(target_array); // 문자열로 입력된 타겟 값을 정수로 변환

    time_t begin = time(NULL); // 작업 시작 시간 기록
    unsigned long long nonce = proofOfWork(data, target); // proof-of-work 계산 수행
    time_t end = time(NULL); // 작업 종료 시간 기록

    printf("\nnonce: %llu, time spent: %d\n", nonce, (int) end - (int) begin); // 결과 출력

    return 0;
}