#!/bin/bash

# ASCON 기반 ML-KEM TLS 1.3 성능 측정 스크립트
# 핸드셰이크 완료 시간, CPU 사용량, 메모리 사용량 측정

echo "=== ASCON 기반 ML-KEM TLS 1.3 성능 측정 ==="
echo "테스트 시작 시간: $(date)"
echo

# 테스트 설정
SERVER_CMD="./examples/server/server -v 4 -l TLS_AES_256_GCM_SHA384 -A ./osp/oqs/mldsa44_root_cert.pem -c ./osp/oqs/mldsa44_entity_cert.pem -k ./osp/oqs/mldsa44_entity_key.pem --pqc ML_KEM_512"
CLIENT_CMD="./examples/client/client -v 4 -l TLS_AES_256_GCM_SHA384 -A ./osp/oqs/mldsa44_root_cert.pem -c ./osp/oqs/mldsa44_entity_cert.pem -k ./osp/oqs/mldsa44_entity_key.pem --pqc ML_KEM_512"

# 결과 파일
RESULTS_FILE="ascon_performance_results.txt"
echo "ASCON 기반 ML-KEM TLS 1.3 성능 측정 결과" > $RESULTS_FILE
echo "측정 시간: $(date)" >> $RESULTS_FILE
echo "=======================================" >> $RESULTS_FILE

# 1. 핸드셰이크 완료 시간 측정
echo "1. 핸드셰이크 완료 시간 측정 중..."
handshake_times=()

for i in {1..10}; do
    echo "  테스트 $i/10 진행 중..."
    
    # 서버 시작 (백그라운드)
    timeout 30s $SERVER_CMD > server_$i.log 2>&1 &
    SERVER_PID=$!
    sleep 2  # 서버 시작 대기
    
    # 클라이언트 연결 시간 측정
    start_time=$(date +%s.%N)
    timeout 10s $CLIENT_CMD > client_$i.log 2>&1
    CLIENT_EXIT_CODE=$?
    end_time=$(date +%s.%N)
    
    # 서버 종료
    kill $SERVER_PID 2>/dev/null
    wait $SERVER_PID 2>/dev/null
    
    if [ $CLIENT_EXIT_CODE -eq 0 ]; then
        handshake_time=$(echo "$end_time - $start_time" | bc -l)
        handshake_times+=($handshake_time)
        echo "    핸드셰이크 시간: ${handshake_time}초"
    else
        echo "    핸드셰이크 실패"
    fi
    
    sleep 1
done

# 핸드셰이크 시간 통계 계산
if [ ${#handshake_times[@]} -gt 0 ]; then
    total=0
    for time in "${handshake_times[@]}"; do
        total=$(echo "$total + $time" | bc -l)
    done
    avg_handshake_time=$(echo "scale=6; $total / ${#handshake_times[@]}" | bc -l)
    
    echo "" >> $RESULTS_FILE
    echo "1. 핸드셰이크 완료 시간 측정 결과" >> $RESULTS_FILE
    echo "성공한 핸드셰이크: ${#handshake_times[@]}/10" >> $RESULTS_FILE
    echo "평균 핸드셰이크 시간: ${avg_handshake_time}초" >> $RESULTS_FILE
    echo "개별 측정값: ${handshake_times[*]}" >> $RESULTS_FILE
    
    echo "✓ 평균 핸드셰이크 시간: ${avg_handshake_time}초"
else
    echo "✗ 모든 핸드셰이크 실패"
    echo "✗ 모든 핸드셰이크 실패" >> $RESULTS_FILE
fi

# 2. CPU 및 메모리 사용량 측정
echo
echo "2. CPU 및 메모리 사용량 측정 중..."

# 서버 시작 (성능 모니터링용)
$SERVER_CMD > server_perf.log 2>&1 &
SERVER_PID=$!
sleep 2

# 성능 데이터 수집 시작
top -p $SERVER_PID -b -n 10 -d 1 > server_performance.log &
TOP_PID=$!

# 여러 클라이언트 연결로 부하 생성
echo "  클라이언트 연결 부하 생성 중..."
for i in {1..5}; do
    timeout 5s $CLIENT_CMD > /dev/null 2>&1 &
    sleep 1
done

sleep 10  # 성능 데이터 수집 대기

# 프로세스 종료
kill $TOP_PID 2>/dev/null
kill $SERVER_PID 2>/dev/null
wait $SERVER_PID 2>/dev/null

# CPU 사용률 분석
if [ -f server_performance.log ]; then
    cpu_usage=$(grep -E "^\s*[0-9]+" server_performance.log | awk '{print $9}' | tail -n +2 | head -n 9)
    if [ ! -z "$cpu_usage" ]; then
        avg_cpu=$(echo "$cpu_usage" | awk '{sum+=$1} END {print sum/NR}')
        max_cpu=$(echo "$cpu_usage" | sort -n | tail -1)
        
        echo "" >> $RESULTS_FILE
        echo "2. CPU 및 메모리 사용량 측정 결과" >> $RESULTS_FILE
        echo "평균 CPU 사용률: ${avg_cpu}%" >> $RESULTS_FILE
        echo "최대 CPU 사용률: ${max_cpu}%" >> $RESULTS_FILE
        
        echo "✓ 평균 CPU 사용률: ${avg_cpu}%"
        echo "✓ 최대 CPU 사용률: ${max_cpu}%"
    fi
fi

# 3. 메모리 사용량 분석
echo
echo "3. 메모리 사용량 측정 중..."

# 프로세스별 메모리 사용량 측정
$SERVER_CMD > server_mem.log 2>&1 &
SERVER_PID=$!
sleep 2

# 메모리 사용량 측정
for i in {1..10}; do
    if ps -p $SERVER_PID > /dev/null; then
        mem_usage=$(ps -p $SERVER_PID -o rss= | xargs)
        echo "메모리 사용량 측정 $i: ${mem_usage}KB"
        echo "$mem_usage" >> memory_usage.log
    fi
    sleep 1
done

kill $SERVER_PID 2>/dev/null
wait $SERVER_PID 2>/dev/null

# 메모리 사용량 통계
if [ -f memory_usage.log ]; then
    avg_memory=$(awk '{sum+=$1} END {print sum/NR}' memory_usage.log)
    max_memory=$(sort -n memory_usage.log | tail -1)
    
    echo "" >> $RESULTS_FILE
    echo "3. 메모리 사용량 측정 결과" >> $RESULTS_FILE
    echo "평균 메모리 사용량: ${avg_memory}KB" >> $RESULTS_FILE
    echo "최대 메모리 사용량: ${max_memory}KB" >> $RESULTS_FILE
    
    echo "✓ 평균 메모리 사용량: ${avg_memory}KB"
    echo "✓ 최대 메모리 사용량: ${max_memory}KB"
fi

# 4. 키 생성 성능 측정
echo
echo "4. ASCON-XOF128 키 유도 성능 측정 중..."

# 키 생성 성능 테스트용 간단한 C 프로그램 작성
cat > key_derivation_test.c << 'EOF'
#include <wolfssl/options.h>
#include <wolfssl/wolfcrypt/ascon.h>
#include <wolfssl/wolfcrypt/wc_mlkem.h>
#include <time.h>
#include <stdio.h>

int main() {
    clock_t start, end;
    double cpu_time_used;
    int ret;
    wc_AsconXof128 xof;
    byte key[32];
    byte output[32];
    int iterations = 1000;
    
    printf("ASCON-XOF128 키 유도 성능 테스트\n");
    printf("반복 횟수: %d\n", iterations);
    
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
            printf("키 유도 실패: %d\n", ret);
            return -1;
        }
    }
    
    end = clock();
    cpu_time_used = ((double) (end - start)) / CLOCKS_PER_SEC;
    
    printf("총 소요 시간: %f초\n", cpu_time_used);
    printf("키 유도당 평균 시간: %f마이크로초\n", (cpu_time_used * 1000000) / iterations);
    
    return 0;
}
EOF

# 키 유도 성능 테스트 컴파일 및 실행
if gcc -DHAVE_ASCON -I./wolfssl -I. key_derivation_test.c -L. -lwolfssl -o key_derivation_test 2>/dev/null; then
    echo "키 유도 성능 테스트 실행 중..."
    ./key_derivation_test > key_derivation_results.log 2>&1
    
    if [ -f key_derivation_results.log ]; then
        echo "" >> $RESULTS_FILE
        echo "4. ASCON-XOF128 키 유도 성능 측정 결과" >> $RESULTS_FILE
        cat key_derivation_results.log >> $RESULTS_FILE
        
        echo "✓ 키 유도 성능 측정 완료"
        cat key_derivation_results.log
    fi
else
    echo "✗ 키 유도 성능 테스트 컴파일 실패"
fi

# 정리
echo
echo "=== 성능 측정 완료 ==="
echo "상세 결과는 $RESULTS_FILE 파일을 확인하세요."
echo

# 요약 결과 출력
echo "" >> $RESULTS_FILE
echo "=======================================" >> $RESULTS_FILE
echo "성능 측정 요약" >> $RESULTS_FILE
echo "측정 완료 시간: $(date)" >> $RESULTS_FILE

# 임시 파일 정리
rm -f server_*.log client_*.log server_performance.log memory_usage.log key_derivation_test.c key_derivation_test key_derivation_results.log

echo "✓ 모든 성능 측정 완료"