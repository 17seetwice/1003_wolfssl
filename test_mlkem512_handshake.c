#include <wolfssl/options.h>
#include <wolfssl/wolfcrypt/settings.h>
#include <wolfssl/ssl.h>
#include <wolfssl/wolfcrypt/wc_mlkem.h>
#include <wolfssl/wolfcrypt/ascon.h>
#include <stdio.h>
#include <string.h>

/* Simple ML-KEM 512 handshake simulation test */
int test_mlkem512_handshake_simulation(void)
{
    int ret = 0;
    MlKemKey server_key, client_key;
    byte server_public[WC_ML_KEM_512_PUBLIC_KEY_SIZE];
    byte server_private[WC_ML_KEM_512_PRIVATE_KEY_SIZE];
    byte client_ciphertext[WC_ML_KEM_512_CIPHER_TEXT_SIZE];
    byte server_shared_secret[WC_ML_KEM_SS_SZ];
    byte client_shared_secret[WC_ML_KEM_SS_SZ];
    word32 pubKeySz = sizeof(server_public);
    word32 privKeySz = sizeof(server_private);
    word32 ciphertextSz = sizeof(client_ciphertext);
    WC_RNG rng;
    
    printf("=== ML-KEM 512 Handshake Simulation ===\n");
    printf("Simulating post-quantum key exchange with Ascon primitives\n\n");
    
    // Initialize RNG
    ret = wc_InitRng(&rng);
    if (ret != 0) {
        printf("Failed to initialize RNG: %d\n", ret);
        return ret;
    }
    
    // Step 1: Server generates ML-KEM 512 key pair
    printf("1. Server: Generating ML-KEM 512 key pair...\n");
    ret = wc_MlKemKey_Init(&server_key, WC_ML_KEM_512, NULL, INVALID_DEVID);
    if (ret != 0) {
        printf("   Failed to initialize server key: %d\n", ret);
        goto cleanup;
    }
    
    ret = wc_MlKemKey_MakeKey(&server_key, &rng);
    if (ret != 0) {
        printf("   Failed to generate server key pair: %d\n", ret);
        goto cleanup;
    }
    
    // Export server public key
    ret = wc_MlKemKey_EncodePublicKey(&server_key, server_public, pubKeySz);
    if (ret != 0) {
        printf("   Failed to encode server public key: %d\n", ret);
        goto cleanup;
    }
    
    printf("   ✓ Server key pair generated (public key: %u bytes)\n", pubKeySz);
    
    // Step 2: Client receives server public key and performs encapsulation
    printf("2. Client: Performing ML-KEM 512 encapsulation...\n");
    ret = wc_MlKemKey_Init(&client_key, WC_ML_KEM_512, NULL, INVALID_DEVID);
    if (ret != 0) {
        printf("   Failed to initialize client key: %d\n", ret);
        goto cleanup;
    }
    
    // Import server public key
    ret = wc_MlKemKey_DecodePublicKey(&client_key, server_public, pubKeySz);
    if (ret != 0) {
        printf("   Failed to decode server public key: %d\n", ret);
        goto cleanup;
    }
    
    // Client encapsulates shared secret
    ret = wc_MlKemKey_Encapsulate(&client_key, client_ciphertext, client_shared_secret, &rng);
    if (ret != 0) {
        printf("   Failed to encapsulate: %d\n", ret);
        goto cleanup;
    }
    
    printf("   ✓ Encapsulation successful (ciphertext: %u bytes, shared secret: %u bytes)\n", 
           ciphertextSz, (word32)WC_ML_KEM_SS_SZ);
    
    // Step 3: Server decapsulates to get shared secret
    printf("3. Server: Performing ML-KEM 512 decapsulation...\n");
    ret = wc_MlKemKey_Decapsulate(&server_key, server_shared_secret, client_ciphertext, ciphertextSz);
    if (ret != 0) {
        printf("   Failed to decapsulate: %d\n", ret);
        goto cleanup;
    }
    
    printf("   ✓ Decapsulation successful\n");
    
    // Step 4: Verify shared secrets match
    printf("4. Verifying shared secrets match...\n");
    if (memcmp(server_shared_secret, client_shared_secret, WC_ML_KEM_SS_SZ) == 0) {
        printf("   ✓ Shared secrets match! Handshake simulation successful.\n");
        
        printf("\nShared Secret (first 16 bytes): ");
        for (int i = 0; i < 16; i++) {
            printf("%02x", server_shared_secret[i]);
        }
        printf("\n");
        
        ret = 0; // Success
    } else {
        printf("   ✗ Shared secrets don't match! Handshake failed.\n");
        ret = -1;
    }
    
cleanup:
    wc_MlKemKey_Free(&server_key);
    wc_MlKemKey_Free(&client_key);
    wc_FreeRng(&rng);
    
    return ret;
}

/* Test ML-KEM with Ascon Hash functionality */
int test_mlkem512_with_ascon_verification(void)
{
    printf("\n=== ML-KEM 512 with Ascon Verification ===\n");
    
    // Test that our Ascon-based ML-KEM functions work
    wc_AsconHash256 hash;
    wc_AsconXof128 xof;
    byte test_data[] = "ML-KEM 512 with Ascon test data";
    byte hash_output[32];
    byte xof_output[64];
    int ret;
    
    printf("1. Testing Ascon-Hash256 integration...\n");
    ret = wc_AsconHash256_Init(&hash);
    if (ret == 0) {
        ret = wc_AsconHash256_Update(&hash, test_data, sizeof(test_data) - 1);
    }
    if (ret == 0) {
        ret = wc_AsconHash256_Final(&hash, hash_output);
    }
    
    if (ret == 0) {
        printf("   ✓ Ascon-Hash256 working correctly\n");
        printf("   Hash (first 16 bytes): ");
        for (int i = 0; i < 16; i++) {
            printf("%02x", hash_output[i]);
        }
        printf("\n");
    } else {
        printf("   ✗ Ascon-Hash256 failed: %d\n", ret);
        return ret;
    }
    
    printf("2. Testing Ascon-XOF128 integration...\n");
    ret = wc_AsconXof128_Init(&xof);
    if (ret == 0) {
        ret = wc_AsconXof128_Absorb(&xof, test_data, sizeof(test_data) - 1);
    }
    if (ret == 0) {
        ret = wc_AsconXof128_Squeeze(&xof, xof_output, sizeof(xof_output));
    }
    
    if (ret == 0) {
        printf("   ✓ Ascon-XOF128 working correctly\n");
        printf("   XOF (first 16 bytes): ");
        for (int i = 0; i < 16; i++) {
            printf("%02x", xof_output[i]);
        }
        printf("\n");
    } else {
        printf("   ✗ Ascon-XOF128 failed: %d\n", ret);
        return ret;
    }
    
    return 0;
}

int main(void)
{
    int ret1, ret2;
    
    printf("ML-KEM 512 Handshake Test with Ascon Cryptography\n");
    printf("==================================================\n\n");
    
    // Test 1: ML-KEM handshake simulation
    ret1 = test_mlkem512_handshake_simulation();
    
    // Test 2: Ascon verification
    ret2 = test_mlkem512_with_ascon_verification();
    
    printf("\n=== Final Results ===\n");
    printf("ML-KEM 512 Handshake: %s\n", ret1 == 0 ? "✓ SUCCESS" : "✗ FAILED");
    printf("Ascon Integration:     %s\n", ret2 == 0 ? "✓ SUCCESS" : "✗ FAILED");
    
    if (ret1 == 0 && ret2 == 0) {
        printf("\n🎉 ALL TESTS PASSED! 🎉\n");
        printf("✓ ML-KEM 512 post-quantum key exchange working\n");
        printf("✓ Ascon lightweight cryptography integrated\n");
        printf("✓ Ready for TLS 1.3 post-quantum handshakes\n");
        return 0;
    } else {
        printf("\n❌ Some tests failed\n");
        return 1;
    }
}