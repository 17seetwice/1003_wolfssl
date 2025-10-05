/*
 * Cortex-M4 Post-Quantum TLS 1.3 Client
 * ML-KEM 512 + MLDSA44 + Ascon implementation
 * 메모리 최적화: CA 인증서로 서버 검증만 수행
 */

#include "cortex_m4_config/user_settings.h"
#include <wolfssl/ssl.h>
#include <wolfssl/wolfcrypt/wc_mlkem.h>
#include <wolfssl/wolfcrypt/dilithium.h>
#include <wolfssl/wolfcrypt/ascon.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#define SERVER_IP "127.0.0.1"
#define SERVER_PORT 12345

/* Cortex-M4 최적화 버퍼 */
#define CLIENT_BUFFER_SIZE 512
#define MAX_MESSAGE_LENGTH 256

/* =================================================================== */
/* CA 인증서 메모리 버퍼 (서버 검증 전용)                               */
/* 실제 배포시: xxd -i ./osp/oqs/mldsa44_root_cert.pem                 */
/* =================================================================== */

const unsigned char ca_cert_buffer[] = {
    /* TODO: 실제 mldsa44_root_cert.pem을 배열로 변환 */
    0x2D, 0x2D, 0x2D, 0x2D, 0x2D, 0x42, 0x45, 0x47, 0x49, 0x4E, 0x20, 0x43, 0x45, 0x52, 0x54, 0x49
    /* ... CA 인증서 데이터만 (클라이언트 인증서 없음) ... */
};
const int ca_cert_size = sizeof(ca_cert_buffer);

/* =================================================================== */
/* User I/O 콜백 (하드웨어 직접 제어)                                   */
/* =================================================================== */

static int socket_fd = -1;

int EmbedReceive(WOLFSSL* ssl, char* buf, int sz, void* ctx) {
    (void)ssl;
    (void)ctx;
    
    if (socket_fd < 0) {
        return WOLFSSL_CBIO_ERR_CONN_CLOSE;
    }
    
    /* 실제 Cortex-M4: W5500_recv(buf, sz) */
    int received = recv(socket_fd, buf, sz, 0);
    if (received <= 0) {
        return WOLFSSL_CBIO_ERR_WANT_READ;
    }
    
    return received;
}

int EmbedSend(WOLFSSL* ssl, char* buf, int sz, void* ctx) {
    (void)ssl;
    (void)ctx;
    
    if (socket_fd < 0) {
        return WOLFSSL_CBIO_ERR_CONN_CLOSE;
    }
    
    /* 실제 Cortex-M4: W5500_send(buf, sz) */
    int sent = send(socket_fd, buf, sz, 0);
    if (sent <= 0) {
        return WOLFSSL_CBIO_ERR_WANT_WRITE;
    }
    
    return sent;
}

int cortex_m4_network_connect() {
    struct sockaddr_in server_addr;
    
    /* 실제 Cortex-M4: W5500_init(), ESP32_wifi_connect() 등 */
    socket_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (socket_fd < 0) {
        printf("[ERROR] Socket creation failed\n");
        return -1;
    }
    
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(SERVER_PORT);
    server_addr.sin_addr.s_addr = inet_addr(SERVER_IP);
    
    if (connect(socket_fd, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        printf("[ERROR] Connection failed\n");
        close(socket_fd);
        socket_fd = -1;
        return -1;
    }
    
    printf("[INFO] TCP connection established\n");
    return 0;
}

void cortex_m4_network_cleanup() {
    if (socket_fd >= 0) {
        close(socket_fd);
        socket_fd = -1;
    }
}

/* =================================================================== */
/* 메인 함수 - Cortex-M4 최적화 완료                                    */
/* =================================================================== */

void print_connection_info(WOLFSSL* ssl) {
    const char* version = wolfSSL_get_version(ssl);
    WOLFSSL_CIPHER* cipher = wolfSSL_get_current_cipher(ssl);
    
    printf("\n=== Cortex-M4 Post-Quantum Connection ===\n");
    printf("TLS Version: %s\n", version);
    if (cipher) {
        printf("Cipher Suite: %s\n", wolfSSL_CIPHER_get_name(cipher));
    }
    printf("Key Exchange: ML-KEM 512 (Ascon-based)\n");
    printf("Server Auth: MLDSA44 (CA 검증)\n");
    printf("Client Auth: None (메모리 최적화)\n");
    printf("========================================\n\n");
}

int main() {
    WOLFSSL_CTX* ctx;
    WOLFSSL* ssl;
    char buffer[CLIENT_BUFFER_SIZE];
    int ret;
    
    printf("=== Cortex-M4 Post-Quantum TLS Client ===\n");
    printf("Target: ARM Cortex-M4 Microcontroller\n");
    printf("Crypto: ML-KEM 512 + MLDSA44 + Ascon\n");
    printf("Server: %s:%d\n", SERVER_IP, SERVER_PORT);
    printf("Mode: Server verification only (memory optimized)\n\n");
    
    /* 1. 네트워크 하드웨어 초기화 및 연결 */
    if (cortex_m4_network_connect() != 0) {
        return 1;
    }
    
    /* 2. wolfSSL 초기화 */
    wolfSSL_Init();
    
    /* 3. TLS 1.3 클라이언트 컨텍스트 생성 */
    ctx = wolfSSL_CTX_new(wolfTLSv1_3_client_method());
    if (ctx == NULL) {
        printf("[ERROR] Failed to create SSL context\n");
        return 1;
    }
    
    /* 4. User I/O 콜백 등록 (하드웨어 직접 제어) */
    wolfSSL_SetIORecv(ctx, EmbedReceive);
    wolfSSL_SetIOSend(ctx, EmbedSend);
    
    /* 5. CA 인증서만 로드 (서버 검증용) - 핵심! */
    ret = wolfSSL_CTX_load_verify_buffer(ctx, ca_cert_buffer, ca_cert_size, WOLFSSL_FILETYPE_PEM);
    if (ret != WOLFSSL_SUCCESS) {
        printf("[ERROR] Failed to load CA certificate from buffer\n");
        wolfSSL_CTX_free(ctx);
        return 1;
    }
    
    /* 클라이언트 인증서 및 개인키 로드 부분은 메모리 최적화를 위해 완전히 제거 */
    
    /* 6. ML-KEM 512 그룹 설정 */
    ret = wolfSSL_CTX_set1_groups_list(ctx, "ML_KEM_512");
    if (ret != WOLFSSL_SUCCESS) {
        printf("[WARNING] Failed to set ML-KEM 512 group\n");
    }
    
    printf("[INFO] Configuration complete. Server verification only.\n");
    printf("[INFO] Memory footprint minimized for Cortex-M4\n");
    
    /* 7. SSL 객체 생성 */
    ssl = wolfSSL_new(ctx);
    if (ssl == NULL) {
        printf("[ERROR] Failed to create SSL object\n");
        wolfSSL_CTX_free(ctx);
        return 1;
    }
    
    /* 8. Post-Quantum TLS 1.3 핸드셰이크 시작 */
    printf("[INFO] Starting Post-Quantum TLS 1.3 handshake...\n");
    printf("  - ML-KEM 512 key exchange (Ascon-based)\n");
    printf("  - MLDSA44 server authentication\n");
    printf("  - No client certificates (memory optimized)\n\n");
    
    ret = wolfSSL_connect(ssl);
    if (ret != WOLFSSL_SUCCESS) {
        int err = wolfSSL_get_error(ssl, ret);
        printf("[ERROR] TLS handshake failed: %d\n", err);
        char error_string[80];
        wolfSSL_ERR_error_string(err, error_string);
        printf("[ERROR] %s\n", error_string);
        wolfSSL_free(ssl);
        wolfSSL_CTX_free(ctx);
        cortex_m4_network_cleanup();
        return 1;
    }
    
    printf("[SUCCESS] Post-Quantum TLS 1.3 connected!\n");
    print_connection_info(ssl);
    
    /* 9. 서버 환영 메시지 수신 */
    printf("[INFO] Waiting for server welcome message...\n");
    memset(buffer, 0, sizeof(buffer));
    ret = wolfSSL_read(ssl, buffer, sizeof(buffer) - 1);
    if (ret > 0) {
        buffer[ret] = '\0';
        printf("[RECV] %s", buffer);
    }
    
    /* 10. 메시지 송수신 루프 */
    printf("\nEnter messages (type 'quit' to exit):\n");
    while (1) {
        printf(">>> ");
        fflush(stdout);
        
        if (fgets(buffer, sizeof(buffer), stdin) == NULL) {
            break;
        }
        
        buffer[strcspn(buffer, "\n")] = '\0';
        
        if (strncmp(buffer, "quit", 4) == 0) {
            break;
        }
        
        /* Cortex-M4 메모리 제한 체크 */
        if (strlen(buffer) > MAX_MESSAGE_LENGTH) {
            printf("[WARNING] Message too long for Cortex-M4 (max: %d)\n", MAX_MESSAGE_LENGTH);
            continue;
        }
        
        /* 송신 */
        ret = wolfSSL_write(ssl, buffer, strlen(buffer));
        if (ret <= 0) {
            printf("[ERROR] Send failed\n");
            break;
        }
        
        /* 수신 */
        memset(buffer, 0, sizeof(buffer));
        ret = wolfSSL_read(ssl, buffer, sizeof(buffer) - 1);
        if (ret > 0) {
            buffer[ret] = '\0';
            printf("[RECV] %s", buffer);
        } else {
            printf("[ERROR] Receive failed\n");
            break;
        }
    }
    
    /* 11. 정리 */
    printf("\n[INFO] Closing Post-Quantum TLS connection...\n");
    wolfSSL_free(ssl);
    wolfSSL_CTX_free(ctx);
    wolfSSL_Cleanup();
    cortex_m4_network_cleanup();
    
    printf("[INFO] Cortex-M4 Post-Quantum TLS client finished\n");
    printf("=== Memory-Optimized Implementation Complete ===\n");
    
    return 0;
}