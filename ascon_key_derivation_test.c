#include <wolfssl/options.h>
#include <wolfssl/wolfcrypt/ascon.h>
#include <wolfssl/wolfcrypt/wc_mlkem.h>
#include <time.h>
#include <stdio.h>
#include <string.h>

int main() {
    clock_t start, end;
    double cpu_time_used;
    int ret;
    wc_AsconXof128 xof;
    byte key[32];
    byte output[32];
    int iterations = 1000;
    
    printf("=== ASCON-XOF128 키 유도 성능 테스트 ===\n");
    printf("반복 횟수: %d\n", iterations);
    printf("키 길이: %d바이트\n", (int)sizeof(key));
    printf("출력 길이: %d바이트\n", (int)sizeof(output));
    
    // 테스트용 키 데이터 초기화
    memset(key, 0xAA, sizeof(key));
    
    // 단일 키 유도 시간 측정
    start = clock();
    ret = wc_AsconXof128_Init(&xof);
    if (ret == 0) {
        ret = wc_AsconXof128_Absorb(&xof, key, sizeof(key));
    }
    if (ret == 0) {
        ret = wc_AsconXof128_Squeeze(&xof, output, sizeof(output));
    }
    wc_AsconXof128_Clear(&xof);
    end = clock();
    
    if (ret == 0) {
        double single_time = ((double) (end - start)) / CLOCKS_PER_SEC;
        printf("단일 키 유도 시간: %f마이크로초\n", single_time * 1000000);
    } else {
        printf("단일 키 유도 실패: %d\n", ret);
        return -1;
    }
    
    // 반복 키 유도 성능 측정
    printf("\n반복 키 유도 성능 측정 시작...\n");
    start = clock();
    
    for (int i = 0; i < iterations; i++) {
        ret = wc_AsconXof128_Init(&xof);
        if (ret == 0) {
            ret = wc_AsconXof128_Absorb(&xof, key, sizeof(key));
        }
        if (ret == 0) {
            ret = wc_AsconXof128_Squeeze(&xof, output, sizeof(output));
        }
        wc_AsconXof128_Clear(&xof);
        
        if (ret != 0) {
            printf("키 유도 실패 (반복 %d): %d\n", i, ret);
            return -1;
        }
    }
    
    end = clock();
    cpu_time_used = ((double) (end - start)) / CLOCKS_PER_SEC;
    
    printf("총 소요 시간: %f초\n", cpu_time_used);
    printf("키 유도당 평균 시간: %f마이크로초\n", (cpu_time_used * 1000000) / iterations);
    printf("초당 키 유도 횟수: %.0f\n", iterations / cpu_time_used);
    
    // 다양한 출력 길이 테스트
    printf("\n=== 다양한 출력 길이별 성능 ===\n");
    int output_lengths[] = {16, 32, 64, 128};
    int num_lengths = sizeof(output_lengths) / sizeof(output_lengths[0]);
    
    for (int len_idx = 0; len_idx < num_lengths; len_idx++) {
        int output_len = output_lengths[len_idx];
        byte* test_output = malloc(output_len);
        if (!test_output) continue;
        
        start = clock();
        for (int i = 0; i < 100; i++) {
            ret = wc_AsconXof128_Init(&xof);
            if (ret == 0) {
                ret = wc_AsconXof128_Absorb(&xof, key, sizeof(key));
            }
            if (ret == 0) {
                ret = wc_AsconXof128_Squeeze(&xof, test_output, output_len);
            }
            wc_AsconXof128_Clear(&xof);
            
            if (ret != 0) break;
        }
        end = clock();
        
        if (ret == 0) {
            double time_per_op = ((double) (end - start)) / CLOCKS_PER_SEC / 100;
            printf("%d바이트 출력: %f마이크로초\n", output_len, time_per_op * 1000000);
        } else {
            printf("%d바이트 출력: 실패\n", output_len);
        }
        
        free(test_output);
    }
    
    return 0;
}