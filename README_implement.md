# Ascon-based ML-KEM Implementation in wolfSSL

## English Documentation

### Overview

This document describes the complete implementation process of replacing SHA-3/SHAKE cryptographic primitives in wolfSSL's ML-KEM (Module-Lattice-Based Key Encapsulation Mechanism) with Ascon lightweight cryptography. This creates an independent, non-standard KEM optimized for embedded systems, particularly ARM Cortex-M4 platforms.

### Background

**ML-KEM (FIPS 203)** is a post-quantum key encapsulation mechanism that traditionally uses SHA-3/SHAKE functions for various cryptographic operations. **Ascon** is a lightweight cryptographic suite that won the NIST Lightweight Cryptography competition, making it ideal for resource-constrained environments.

### Key Changes Made

#### 1. Cryptographic Primitive Mapping

| Original (ML-KEM Standard) | Replaced with (Ascon) | Purpose |
|---------------------------|----------------------|---------|
| SHA3-256 | Ascon-Hash256 | Hash function (H) |
| SHAKE-128 | Ascon-XOF128 | PRF, XOF, KDF functions |
| SHAKE-256 | Ascon-XOF128 | Extended output functions |

#### 2. File Modifications

**Modified Files:**
- `/wolfssl/wolfcrypt/wc_mlkem.h` - Type definitions and constants
- `/wolfcrypt/src/wc_mlkem.c` - Core ML-KEM implementation
- `/wolfcrypt/src/wc_mlkem_poly.c` - Polynomial arithmetic with Ascon

**Key Changes in Header (`wc_mlkem.h`):**
```c
// Original
#define MLKEM_HASH_T    wc_Sha3_256
#define MLKEM_PRF_T     wc_Shake128

// Modified
#define MLKEM_HASH_T    wc_AsconHash256
#define MLKEM_PRF_T     wc_AsconXof128
#define XOF_BLOCK_SIZE  16  // Changed from 168 for Ascon
```

**Critical Implementation Functions:**

```c
// Hash function wrapper
int mlkem_hash256(wc_AsconHash256* hash, const byte* data, word32 dataLen, byte* out)
{
    int ret;
    ret = wc_AsconHash256_Init(hash);
    if (ret == 0) {
        ret = wc_AsconHash256_Update(hash, data, dataLen);
    }
    if (ret == 0) {
        ret = wc_AsconHash256_Final(hash, out);
    }
    return ret;
}

// PRF function wrapper
static int mlkem_prf(wc_AsconXof128* prf, byte* out, unsigned int outLen, const byte* key)
{
    int ret;
    ret = wc_AsconXof128_Init(prf);
    if (ret == 0) {
        ret = wc_AsconXof128_Absorb(prf, key, WC_ML_KEM_SYM_SZ + 1);
    }
    if (ret == 0) {
        ret = wc_AsconXof128_Squeeze(prf, out, outLen);
    }
    return ret;
}
```

#### 3. Memory Management Fixes

**Critical Issue:** Stack-allocated Ascon objects cannot use `Free()` functions.

**Solution:** Use `Clear()` functions for stack objects:
```c
// WRONG: wc_AsconXof128_Free(prf);
// CORRECT:
void mlkem_prf_free(wc_AsconXof128* prf) {
    wc_AsconXof128_Clear(prf);
}
```

**ForceZero Fix in `wc_MlKemKey_Free`:**
```c
// Fixed memory addresses and sizes
ForceZero(&key->hash, sizeof(key->hash));  // was: ForceZero(&key->prf, sizeof(key->hash))
ForceZero(&key->prf, sizeof(key->prf));
```

### Implementation Steps

#### Step 1: Prepare Clean Environment
```bash
# Use clean wolfSSL ML-KEM files to avoid build conflicts
cp wc_mlkem_poly_test.c wc_mlkem_poly.c
```

#### Step 2: Update Header Definitions
Edit `/wolfssl/wolfcrypt/wc_mlkem.h`:
- Replace SHA-3/SHAKE type definitions with Ascon equivalents
- Update XOF_BLOCK_SIZE constant
- Ensure proper include statements

#### Step 3: Implement Function Wrappers
Create wrapper functions in `wc_mlkem_poly.c` for:
- `mlkem_hash256()` - Ascon-Hash256 wrapper
- `mlkem_prf()` - Ascon-XOF128 PRF wrapper
- `mlkem_xof()` - Ascon-XOF128 XOF wrapper
- `mlkem_kdf()` - Ascon-XOF128 KDF wrapper

#### Step 4: Fix Memory Management
- Replace all `Free()` calls with `Clear()` for stack objects
- Fix ForceZero parameters in cleanup functions
- Ensure proper memory allocation patterns

#### Step 5: Update All Function Signatures
Replace all function parameters and return types:
- `wc_Sha3_256*` → `wc_AsconHash256*`
- `wc_Shake128*` → `wc_AsconXof128*`

#### Step 6: Build and Test
```bash
# Compile with Ascon support
gcc -DHAVE_ASCON -I./wolfssl test_ascon_mlkem.c -L. -lwolfssl -o test_ascon_mlkem

# Run tests
./test_ascon_mlkem
./test_mlkem512_handshake
./test_cortex_m4_compat
```

### Test Results

#### Basic Functionality Test
```
=== Ascon-based ML-KEM Test ===
✓ ML-KEM key generation successful
✓ ML-KEM encapsulation successful
✓ ML-KEM decapsulation successful
✓ Shared secrets match!
✓ All Ascon functions working correctly
```

#### ML-KEM 512 Handshake Test
```
=== ML-KEM 512 Handshake Simulation ===
✓ Server key pair generated
✓ Encapsulation successful
✓ Decapsulation successful
✓ Shared secrets match!
🎉 ALL TESTS PASSED!
```

#### Cortex-M4 Compatibility
```
=== Cortex-M4 Compatibility: PASSED ===
✓ Pure C implementation confirmed
✓ No assembly dependencies
✓ Suitable for ARM Cortex-M4
✓ Ascon lightweight crypto optimized for embedded
```

### ARM Cortex-M4 Optimizations

#### Disabled Assembly Optimizations
- Removed AVX2/NEON dependencies
- Set `WC_SHA3_NO_ASM` to disable assembly
- Disabled `USE_INTEL_SPEEDUP` and `WOLFSSL_ARMASM`
- Pure C implementation for maximum portability

#### Memory Efficiency
- Ascon uses smaller state sizes than SHA-3/SHAKE
- Optimized for 32-bit ARM architectures
- Reduced memory footprint for embedded systems

### Security Considerations

#### Non-Standard Implementation
**Important:** This implementation creates a **non-standard ML-KEM variant** that is:
- ✅ **Compatible** with wolfSSL's existing API
- ✅ **Optimized** for embedded systems
- ❌ **NOT compatible** with standard ML-KEM implementations
- ❌ **NOT FIPS 203 compliant**

#### Use Cases
- Embedded systems requiring post-quantum security
- Resource-constrained environments
- Custom protocols where interoperability is not required
- Research and development purposes

### Troubleshooting

#### Common Build Issues

**1. Unterminated #if blocks:**
```
Solution: Use clean wc_mlkem_poly_test.c file as starting point
```

**2. Memory segmentation faults:**
```
Error: free(): invalid pointer
Solution: Use Clear() instead of Free() for stack objects
```

**3. Function signature mismatches:**
```
Solution: Update all function declarations to use Ascon types
```

**4. Missing Ascon definitions:**
```
Solution: Include wolfssl/wolfcrypt/ascon.h and compile with -DHAVE_ASCON
```

#### Build Commands
```bash
# Basic compilation
gcc -DHAVE_ASCON -I./wolfssl -I. test_file.c -L. -lwolfssl -o test_file

# For ARM Cortex-M4 cross-compilation
arm-none-eabi-gcc -mcpu=cortex-m4 -mthumb -DHAVE_ASCON -DWOLFSSL_USER_SETTINGS \
    -I./wolfssl test_file.c -L. -lwolfssl -o test_file
```

---

## 한국어 문서

### 개요

이 문서는 wolfSSL의 ML-KEM(Module-Lattice-Based Key Encapsulation Mechanism)에서 SHA-3/SHAKE 암호학적 프리미티브를 Ascon 경량 암호화로 교체하는 완전한 구현 과정을 설명합니다. 이를 통해 임베디드 시스템, 특히 ARM Cortex-M4 플랫폼에 최적화된 독립적이고 비표준 KEM을 만들 수 있습니다.

### 배경

**ML-KEM (FIPS 203)**은 다양한 암호학적 연산에 전통적으로 SHA-3/SHAKE 함수를 사용하는 양자내성 키 캡슐화 메커니즘입니다. **Ascon**은 NIST 경량 암호화 경진대회에서 우승한 경량 암호화 제품군으로, 리소스가 제한된 환경에 이상적입니다.

### 주요 변경사항

#### 1. 암호학적 프리미티브 매핑

| 원본 (ML-KEM 표준) | 교체 (Ascon) | 목적 |
|-------------------|--------------|------|
| SHA3-256 | Ascon-Hash256 | 해시 함수 (H) |
| SHAKE-128 | Ascon-XOF128 | PRF, XOF, KDF 함수 |
| SHAKE-256 | Ascon-XOF128 | 확장 출력 함수 |

#### 2. 파일 수정사항

**수정된 파일:**
- `/wolfssl/wolfcrypt/wc_mlkem.h` - 타입 정의 및 상수
- `/wolfcrypt/src/wc_mlkem.c` - 핵심 ML-KEM 구현
- `/wolfcrypt/src/wc_mlkem_poly.c` - Ascon을 사용한 다항식 연산

#### 3. 메모리 관리 수정

**중요한 문제:** 스택에 할당된 Ascon 객체는 `Free()` 함수를 사용할 수 없습니다.

**해결방법:** 스택 객체에는 `Clear()` 함수 사용:
```c
// 잘못된 방법: wc_AsconXof128_Free(prf);
// 올바른 방법:
void mlkem_prf_free(wc_AsconXof128* prf) {
    wc_AsconXof128_Clear(prf);
}
```

### 구현 단계

#### 1단계: 깨끗한 환경 준비
```bash
# 빌드 충돌을 피하기 위해 깨끗한 wolfSSL ML-KEM 파일 사용
cp wc_mlkem_poly_test.c wc_mlkem_poly.c
```

#### 2단계: 헤더 정의 업데이트
`/wolfssl/wolfcrypt/wc_mlkem.h` 편집:
- SHA-3/SHAKE 타입 정의를 Ascon 등가물로 교체
- XOF_BLOCK_SIZE 상수 업데이트
- 적절한 include 문 확인

#### 3단계: 함수 래퍼 구현
`wc_mlkem_poly.c`에서 다음에 대한 래퍼 함수 생성:
- `mlkem_hash256()` - Ascon-Hash256 래퍼
- `mlkem_prf()` - Ascon-XOF128 PRF 래퍼
- `mlkem_xof()` - Ascon-XOF128 XOF 래퍼
- `mlkem_kdf()` - Ascon-XOF128 KDF 래퍼

#### 4단계: 메모리 관리 수정
- 스택 객체에 대한 모든 `Free()` 호출을 `Clear()`로 교체
- 정리 함수에서 ForceZero 매개변수 수정
- 적절한 메모리 할당 패턴 보장

#### 5단계: 모든 함수 시그니처 업데이트
모든 함수 매개변수 및 반환 타입 교체:
- `wc_Sha3_256*` → `wc_AsconHash256*`
- `wc_Shake128*` → `wc_AsconXof128*`

#### 6단계: 빌드 및 테스트
```bash
# Ascon 지원으로 컴파일
gcc -DHAVE_ASCON -I./wolfssl test_ascon_mlkem.c -L. -lwolfssl -o test_ascon_mlkem

# 테스트 실행
./test_ascon_mlkem
./test_mlkem512_handshake
./test_cortex_m4_compat
```

### 테스트 결과

#### 기본 기능 테스트
```
=== Ascon 기반 ML-KEM 테스트 ===
✓ ML-KEM 키 생성 성공
✓ ML-KEM 캡슐화 성공
✓ ML-KEM 역캡슐화 성공
✓ 공유 비밀 일치!
✓ 모든 Ascon 함수 정상 작동
```

#### ML-KEM 512 핸드셰이크 테스트
```
=== ML-KEM 512 핸드셰이크 시뮬레이션 ===
✓ 서버 키 쌍 생성됨
✓ 캡슐화 성공
✓ 역캡슐화 성공
✓ 공유 비밀 일치!
🎉 모든 테스트 통과!
```

#### Cortex-M4 호환성
```
=== Cortex-M4 호환성: 통과 ===
✓ 순수 C 구현 확인됨
✓ 어셈블리 종속성 없음
✓ ARM Cortex-M4에 적합
✓ 임베디드용 Ascon 경량 암호화 최적화됨
```

### ARM Cortex-M4 최적화

#### 어셈블리 최적화 비활성화
- AVX2/NEON 종속성 제거
- 어셈블리를 비활성화하기 위해 `WC_SHA3_NO_ASM` 설정
- `USE_INTEL_SPEEDUP` 및 `WOLFSSL_ARMASM` 비활성화
- 최대 이식성을 위한 순수 C 구현

#### 메모리 효율성
- Ascon은 SHA-3/SHAKE보다 작은 상태 크기 사용
- 32비트 ARM 아키텍처에 최적화
- 임베디드 시스템을 위한 메모리 사용량 감소

### 보안 고려사항

#### 비표준 구현
**중요:** 이 구현은 다음과 같은 **비표준 ML-KEM 변형**을 만듭니다:
- ✅ wolfSSL의 기존 API와 **호환됨**
- ✅ 임베디드 시스템에 **최적화됨**
- ❌ 표준 ML-KEM 구현과 **호환되지 않음**
- ❌ **FIPS 203 준수하지 않음**

#### 사용 사례
- 양자내성 보안이 필요한 임베디드 시스템
- 리소스가 제한된 환경
- 상호 운용성이 필요하지 않은 사용자 정의 프로토콜
- 연구 및 개발 목적

### 문제 해결

#### 일반적인 빌드 문제

**1. 종료되지 않은 #if 블록:**
```
해결방법: 시작점으로 깨끗한 wc_mlkem_poly_test.c 파일 사용
```

**2. 메모리 세그멘테이션 오류:**
```
오류: free(): invalid pointer
해결방법: 스택 객체에 Free() 대신 Clear() 사용
```

**3. 함수 시그니처 불일치:**
```
해결방법: Ascon 타입을 사용하도록 모든 함수 선언 업데이트
```

**4. Ascon 정의 누락:**
```
해결방법: wolfssl/wolfcrypt/ascon.h 포함 및 -DHAVE_ASCON으로 컴파일
```

#### 빌드 명령어
```bash
# 기본 컴파일
gcc -DHAVE_ASCON -I./wolfssl -I. test_file.c -L. -lwolfssl -o test_file

# ARM Cortex-M4 크로스 컴파일용
arm-none-eabi-gcc -mcpu=cortex-m4 -mthumb -DHAVE_ASCON -DWOLFSSL_USER_SETTINGS \
    -I./wolfssl test_file.c -L. -lwolfssl -o test_file
```

### 성공적인 구현 확인

이 구현을 통해 다음을 달성했습니다:

1. **완전한 암호학적 프리미티브 교체**: SHA-3/SHAKE → Ascon
2. **메모리 관리 최적화**: 임베디드 시스템에 적합한 안전한 메모리 처리
3. **ARM Cortex-M4 호환성**: 어셈블리 종속성 없는 순수 C 구현
4. **성공적인 테스트**: 키 생성, 캡슐화, 역캡슐화, 핸드셰이크 시뮬레이션
5. **경량 암호화 통합**: Ascon의 임베디드 최적화 활용

### 결론

이 프로젝트는 표준 ML-KEM을 Ascon 기반 변형으로 성공적으로 변환하여 임베디드 시스템에서 양자내성 암호화의 새로운 가능성을 열었습니다. 비표준이지만, 리소스가 제한된 환경에서 향후 양자내성 보안 솔루션의 기반을 제공합니다.