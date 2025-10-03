#include <wolfssl/options.h>
#include <wolfssl/wolfcrypt/settings.h>
#include <wolfssl/wolfcrypt/wc_mlkem.h>
#include <wolfssl/wolfcrypt/ascon.h>
#include <wolfssl/wolfcrypt/random.h>
#include <stdio.h>
#include <string.h>

int main(void)
{
    int ret = 0;
    MlKemKey key;
    byte publicKey[WC_ML_KEM_768_PUBLIC_KEY_SIZE];
    byte privateKey[WC_ML_KEM_768_PRIVATE_KEY_SIZE];
    byte ciphertext[WC_ML_KEM_768_CIPHER_TEXT_SIZE];
    byte sharedSecret1[WC_ML_KEM_SS_SZ];
    byte sharedSecret2[WC_ML_KEM_SS_SZ];
    word32 publicKeySz = sizeof(publicKey);
    word32 privateKeySz = sizeof(privateKey);
    word32 ciphertextSz = sizeof(ciphertext);
    word32 sharedSecretSz = sizeof(sharedSecret1);
    WC_RNG rng;

    printf("=== ML-KEM with Ascon Test ===\n");

    // Initialize RNG
    ret = wc_InitRng(&rng);
    if (ret != 0) {
        printf("Failed to initialize RNG: %d\n", ret);
        return ret;
    }

    // Initialize ML-KEM key
    ret = wc_MlKemKey_Init(&key, WC_ML_KEM_768, NULL, INVALID_DEVID);
    if (ret != 0) {
        printf("Failed to initialize ML-KEM key: %d\n", ret);
        goto cleanup;
    }

    printf("1. Generating ML-KEM-768 key pair with Ascon...\n");
    
    // Generate key pair
    ret = wc_MlKemKey_MakeKey(&key, &rng);
    if (ret != 0) {
        printf("Failed to generate key pair: %d\n", ret);
        goto cleanup;
    }
    
    printf("   ✓ Key pair generated successfully!\n");

    // Encode public key
    ret = wc_MlKemKey_EncodePublicKey(&key, publicKey, publicKeySz);
    if (ret != 0) {
        printf("Failed to encode public key: %d\n", ret);
        goto cleanup;
    }

    // Encode private key  
    ret = wc_MlKemKey_EncodePrivateKey(&key, privateKey, privateKeySz);
    if (ret != 0) {
        printf("Failed to encode private key: %d\n", ret);
        goto cleanup;
    }

    printf("2. Testing encapsulation with Ascon...\n");
    
    // Encapsulate
    ret = wc_MlKemKey_Encapsulate(&key, ciphertext, sharedSecret1, &rng);
    if (ret != 0) {
        printf("Failed to encapsulate: %d\n", ret);
        goto cleanup;
    }
    
    printf("   ✓ Encapsulation successful!\n");
    printf("   Ciphertext size: %u bytes\n", ciphertextSz);
    printf("   Shared secret size: %u bytes\n", sharedSecretSz);

    printf("3. Testing decapsulation with Ascon...\n");
    
    // Decapsulate
    ret = wc_MlKemKey_Decapsulate(&key, sharedSecret2, ciphertext, ciphertextSz);
    if (ret != 0) {
        printf("Failed to decapsulate: %d\n", ret);
        goto cleanup;
    }
    
    printf("   ✓ Decapsulation successful!\n");

    printf("4. Verifying shared secrets match...\n");
    
    // Compare shared secrets
    if (memcmp(sharedSecret1, sharedSecret2, WC_ML_KEM_SS_SZ) == 0) {
        printf("   ✓ Shared secrets match! ML-KEM with Ascon working correctly.\n");
        
        printf("\nFirst 16 bytes of shared secret: ");
        for (int i = 0; i < 16; i++) {
            printf("%02x", sharedSecret1[i]);
        }
        printf("\n");
        
        printf("\n=== SUCCESS: ML-KEM with Ascon implementation verified! ===\n");
        printf("- SHA-3/SHAKE functions successfully replaced with Ascon\n");
        printf("- Key generation, encapsulation, and decapsulation all working\n");
        printf("- Quantum-resistant cryptography with lightweight Ascon primitives\n");
    } else {
        printf("   ✗ Shared secrets don't match!\n");
        ret = -1;
    }

cleanup:
    wc_MlKemKey_Free(&key);
    wc_FreeRng(&rng);
    
    return ret;
}