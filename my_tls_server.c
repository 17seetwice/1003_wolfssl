/*
 * TLS Server using WolfSSL with ML-KEM, ASCON, and Dilithium support
 * Target IP: 10.150.63.100
 * Port: 11111
 */

#include <wolfssl/options.h>
#include <wolfssl/ssl.h>
#include <wolfssl/wolfcrypt/settings.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <signal.h>

#define SERVER_PORT 12345
#define SERVER_IP "0.0.0.0"
#define CERT_FILE "server-cert.pem"
#define KEY_FILE "server-key.pem"

volatile int server_running = 1;

void signal_handler(int sig) {
    printf("\n[INFO] Shutting down server...\n");
    server_running = 0;
}

void handle_client(WOLFSSL* ssl) {
    char buffer[1024];
    int ret;
    
    printf("[INFO] Starting TLS handshake...\n");
    
    // TLS 핸드셰이크
    ret = wolfSSL_accept(ssl);
    if (ret != WOLFSSL_SUCCESS) {
        int err = wolfSSL_get_error(ssl, ret);
        printf("[ERROR] TLS handshake failed: %d\n", err);
        char error_string[80];
        wolfSSL_ERR_error_string(err, error_string);
        printf("[ERROR] %s\n", error_string);
        return;
    }
    
    printf("[SUCCESS] TLS handshake completed!\n");
    
    // 클라이언트 정보 출력
    WOLFSSL_CIPHER* cipher = wolfSSL_get_current_cipher(ssl);
    if (cipher) {
        printf("[INFO] Cipher Suite: %s\n", wolfSSL_CIPHER_get_name(cipher));
    }
    
    const char* version = wolfSSL_get_version(ssl);
    printf("[INFO] TLS Version: %s\n", version);
    
    // 환영 메시지 전송
    const char* welcome_msg = 
        "=== WolfSSL TLS Server ===\n"
        "Post-Quantum Crypto Support:\n"
        "- ML-KEM (Kyber): Enabled\n"
        "- ASCON: Enabled\n" 
        "- Dilithium: Enabled\n"
        "Server IP: 10.150.63.100\n"
        "Type 'quit' to exit.\n"
        ">>> ";
    
    ret = wolfSSL_write(ssl, welcome_msg, strlen(welcome_msg));
    if (ret <= 0) {
        printf("[ERROR] Failed to send welcome message\n");
        return;
    }
    
    // 클라이언트와 통신
    while (1) {
        memset(buffer, 0, sizeof(buffer));
        ret = wolfSSL_read(ssl, buffer, sizeof(buffer) - 1);
        
        if (ret <= 0) {
            int err = wolfSSL_get_error(ssl, ret);
            if (err == WOLFSSL_ERROR_ZERO_RETURN) {
                printf("[INFO] Client closed connection\n");
            } else {
                printf("[ERROR] Read error: %d\n", err);
            }
            break;
        }
        
        buffer[ret] = '\0';
        printf("[RECV] %s", buffer);
        
        // 'quit' 명령어 처리
        if (strncmp(buffer, "quit", 4) == 0) {
            const char* goodbye = "Goodbye!\n";
            wolfSSL_write(ssl, goodbye, strlen(goodbye));
            break;
        }
        
        // Echo 응답
        char response[1024];
        snprintf(response, sizeof(response), "[ECHO] %s>>> ", buffer);
        ret = wolfSSL_write(ssl, response, strlen(response));
        
        if (ret <= 0) {
            printf("[ERROR] Failed to send response\n");
            break;
        }
    }
}

int main() {
    int server_fd, client_fd;
    struct sockaddr_in server_addr, client_addr;
    socklen_t client_len = sizeof(client_addr);
    WOLFSSL_CTX* ctx;
    WOLFSSL* ssl;
    
    // Signal handler 설정
    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);
    
    printf("=== WolfSSL TLS Server ===\n");
    printf("Server IP: %s\n", SERVER_IP);
    printf("Server Port: %d\n", SERVER_PORT);
    printf("Features: ML-KEM, ASCON, Dilithium\n\n");
    
    // WolfSSL 초기화
    wolfSSL_Init();
    
    // TLS 1.3 서버 컨텍스트 생성
    ctx = wolfSSL_CTX_new(wolfTLSv1_3_server_method());
    if (ctx == NULL) {
        printf("[ERROR] Failed to create SSL context\n");
        return 1;
    }
    
    // 인증서와 개인키 로드
    if (wolfSSL_CTX_use_certificate_file(ctx, CERT_FILE, WOLFSSL_FILETYPE_PEM) != WOLFSSL_SUCCESS) {
        printf("[ERROR] Failed to load certificate file: %s\n", CERT_FILE);
        wolfSSL_CTX_free(ctx);
        return 1;
    }
    
    if (wolfSSL_CTX_use_PrivateKey_file(ctx, KEY_FILE, WOLFSSL_FILETYPE_PEM) != WOLFSSL_SUCCESS) {
        printf("[ERROR] Failed to load private key file: %s\n", KEY_FILE);
        wolfSSL_CTX_free(ctx);
        return 1;
    }
    
    printf("[INFO] Certificate and key loaded successfully\n");
    
    // 소켓 생성
    server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd < 0) {
        perror("[ERROR] Socket creation failed");
        wolfSSL_CTX_free(ctx);
        return 1;
    }
    
    // SO_REUSEADDR 설정
    int opt = 1;
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
        perror("[ERROR] Setsockopt failed");
        close(server_fd);
        wolfSSL_CTX_free(ctx);
        return 1;
    }
    
    // 서버 주소 설정
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = inet_addr(SERVER_IP);
    server_addr.sin_port = htons(SERVER_PORT);
    
    // 바인드
    if (bind(server_fd, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        perror("[ERROR] Bind failed");
        close(server_fd);
        wolfSSL_CTX_free(ctx);
        return 1;
    }
    
    // 리슨
    if (listen(server_fd, 10) < 0) {
        perror("[ERROR] Listen failed");
        close(server_fd);
        wolfSSL_CTX_free(ctx);
        return 1;
    }
    
    printf("[INFO] Server listening on %s:%d\n", SERVER_IP, SERVER_PORT);
    printf("[INFO] Waiting for connections...\n");
    printf("[INFO] Press Ctrl+C to stop the server\n\n");
    
    // 클라이언트 연결 처리 루프
    while (server_running) {
        // 연결 수락 (non-blocking으로 설정하지 않음)
        client_fd = accept(server_fd, (struct sockaddr*)&client_addr, &client_len);
        
        if (client_fd < 0) {
            if (server_running) {
                perror("[ERROR] Accept failed");
            }
            continue;
        }
        
        printf("[INFO] New connection from %s:%d\n", 
               inet_ntoa(client_addr.sin_addr), 
               ntohs(client_addr.sin_port));
        
        // SSL 객체 생성
        ssl = wolfSSL_new(ctx);
        if (ssl == NULL) {
            printf("[ERROR] Failed to create SSL object\n");
            close(client_fd);
            continue;
        }
        
        // 소켓을 SSL 객체에 연결
        wolfSSL_set_fd(ssl, client_fd);
        
        // 클라이언트 처리
        handle_client(ssl);
        
        // 정리
        wolfSSL_free(ssl);
        close(client_fd);
        
        printf("[INFO] Connection closed\n\n");
    }
    
    // 정리
    close(server_fd);
    wolfSSL_CTX_free(ctx);
    wolfSSL_Cleanup();
    
    printf("[INFO] Server stopped\n");
    return 0;
}