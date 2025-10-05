/* user_settings.h
 * Cortex-M4 optimized configuration for ML-KEM 512 with Ascon + MLDSA44
 * Post-Quantum TLS 1.3 Security System
 */

#ifndef CORTEX_M4_MLKEM512_USER_SETTINGS_H
#define CORTEX_M4_MLKEM512_USER_SETTINGS_H

/* Basic wolfSSL settings for Cortex-M4 */
#define WOLFSSL_USER_SETTINGS
#define NO_FILESYSTEM
#define NO_MAIN_DRIVER
#define SINGLE_THREADED
#define NO_WRITEV

/* Ascon lightweight cryptography */
#define HAVE_ASCON

/* ML-KEM 512 ONLY (메모리 최적화) */
#define WOLFSSL_HAVE_MLKEM
#define WOLFSSL_WC_MLKEM
#define WOLFSSL_WC_ML_KEM_512
/* 다른 ML-KEM 비활성화로 메모리 절약 */
#undef WOLFSSL_WC_ML_KEM_768
#undef WOLFSSL_WC_ML_KEM_1024

/* MLDSA44 support for Post-Quantum signatures */
#define HAVE_DILITHIUM
#define WOLFSSL_DILITHIUM_SIGN_SMALL_MEM

/* TLS 1.3 Post-Quantum support */
#define WOLFSSL_TLS13
#define HAVE_TLS_EXTENSIONS
#define HAVE_SUPPORTED_CURVES
#define HAVE_HKDF
#define HAVE_ENCRYPT_THEN_MAC
#define HAVE_EXTENDED_MASTER

/* Cortex-M4 메모리 최적화 */
#define WOLFSSL_SMALL_STACK
#define WOLFSSL_SMALL_CERT_VERIFY
#define WOLFSSL_SMALL_STACK_CACHE
#define NO_SESSION_CACHE
#define ALT_ECC_SIZE
#define ECC_TIMING_RESISTANT
#define TFM_TIMING_RESISTANT

/* 어셈블리 최적화 비활성화 (포팅성) */
#define WC_SHA3_NO_ASM
#define TFM_NO_ASM
#define WOLFSSL_NO_ASM
#define WC_MLKEM_NO_ASM

/* Intel/ARM 특정 최적화 비활성화 */
#undef USE_INTEL_SPEEDUP
#undef WOLFSSL_ARMASM

/* 32비트 Cortex-M4용 수학 라이브러리 */
#define WOLFSSL_SP
#define WOLFSSL_SP_SMALL
#define WOLFSSL_SP_MATH
#define SP_WORD_SIZE 32
#define WOLFSSL_SP_NO_MALLOC

/* Single Precision 지원 알고리즘 */
#define WOLFSSL_HAVE_SP_RSA
#define WOLFSSL_HAVE_SP_ECC

/* ECC 설정 (P-256, P-384만) */
#define HAVE_ECC
#define ECC_USER_CURVES
#undef NO_ECC256
#define HAVE_ECC384
#define ECC_TIMING_RESISTANT

/* RSA 설정 */
#define HAVE_RSA
#define WC_RSA_BLINDING
#define WC_RSA_PSS

/* AES 설정 (경량) */
#define HAVE_AESGCM
#define HAVE_AES_DECRYPT
#define GCM_SMALL

/* ChaCha20/Poly1305 */
#define HAVE_CHACHA
#define HAVE_POLY1305
#define HAVE_ONE_TIME_AUTH

/* 해시 함수 */
#define WOLFSSL_SHA256
#define WOLFSSL_SHA384
#define WOLFSSL_SHA512
#define NO_SHA
#define NO_MD5

/* 임베디드용 네트워킹 */
#define WOLFSSL_USER_IO
#define WOLFSSL_NO_SOCK
#define NO_WOLFSSL_DIR

/* 메모리 절약을 위한 불필요한 기능 비활성화 */
#define NO_FILESYSTEM
#define NO_WRITEV
#define NO_DEV_RANDOM
#define NO_OLD_TLS
#define NO_DSA
#define NO_RC4
#define NO_MD4
#define NO_DES3
#define NO_PSK
#define NO_PWDBASED

/* ASN.1 template (코드 크기 감소) */
#define WOLFSSL_ASN_TEMPLATE

/* 벤치마크/테스트 설정 */
#define BENCH_EMBEDDED
#define USE_CERT_BUFFERS_2048
#define USE_CERT_BUFFERS_256

/* 메모리 관리 */
#define WOLFSSL_GENERAL_ALIGNMENT 4
#define SIZEOF_LONG_LONG 8

/* 임베디드용 시간 함수 */
#define WOLFSSL_USER_CURRTIME

/* RNG 설정 */
#define HAVE_HASHDRBG
#define NO_OLD_RNGNAME

/* ML-KEM 메모리 최적화 (Cortex-M4용) */
#define WOLFSSL_MLKEM_MAKEKEY_SMALL_MEM
#define WOLFSSL_MLKEM_ENCAPSULATE_SMALL_MEM

/* Cortex-M4 스택 크기 제한 */
#define MAX_RECORD_SIZE 1024
#define MAX_HANDSHAKE_SZ 8192

/* 세션 재사용 비활성화 (메모리 절약) */
#define NO_SESSION_CACHE

/* 오류 처리 */
#define WOLFSSL_IGNORE_FILE_WARN

/* SNI 지원 */
#define HAVE_SNI

#endif /* CORTEX_M4_MLKEM512_USER_SETTINGS_H */