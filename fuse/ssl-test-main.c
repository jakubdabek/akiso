#include "ssl-crypt.h"

#include <openssl/buffer.h>
#include <openssl/conf.h>
#include <openssl/evp.h>
#include <openssl/bio.h>
#include <openssl/err.h>
#include <string.h>

void handleErrors(void)
{
    ERR_print_errors_fp(stderr);
    abort();
}

int encrypt(unsigned char *plaintext, int plaintext_len, unsigned char *key,
  unsigned char *iv, unsigned char *ciphertext)
{
    EVP_CIPHER_CTX *ctx;

    int len;

    int ciphertext_len;

    /* Create and initialise the context */
    if ((ctx = EVP_CIPHER_CTX_new()) == NULL)
        handleErrors();

    /* Initialise the encryption operation. IMPORTANT - ensure you use a key
    * and IV size appropriate for your cipher
    * In this example we are using 256 bit AES (i.e. a 256 bit key). The
    * IV size for *most* modes is the same as the block size. For AES this
    * is 128 bits */
    if (EVP_EncryptInit_ex(ctx, EVP_aes_128_cbc(), NULL, key, iv) != 1)
        handleErrors();

    /* Provide the message to be encrypted, and obtain the encrypted output.
    * EVP_EncryptUpdate can be called multiple times if necessary
    */
    if (EVP_EncryptUpdate(ctx, ciphertext, &len, plaintext, plaintext_len) != 1)
        handleErrors();
    ciphertext_len = len;

    /* Finalise the encryption. Further ciphertext bytes may be written at
    * this stage.
    */
    if (EVP_EncryptFinal_ex(ctx, ciphertext + len, &len) != 1)
        handleErrors();
    ciphertext_len += len;

    /* Clean up */
    EVP_CIPHER_CTX_free(ctx);

    return ciphertext_len;
}

int decrypt(unsigned char *ciphertext, int ciphertext_len, unsigned char *key,
    unsigned char *iv, unsigned char *plaintext)
{
    EVP_CIPHER_CTX *ctx;

    int len;

    int plaintext_len;

    /* Create and initialise the context */
    if((ctx = EVP_CIPHER_CTX_new()) == NULL)
        handleErrors();

    /* Initialise the decryption operation. IMPORTANT - ensure you use a key
    * and IV size appropriate for your cipher
    * In this example we are using 256 bit AES (i.e. a 256 bit key). The
    * IV size for *most* modes is the same as the block size. For AES this
    * is 128 bits */
    if (EVP_DecryptInit_ex(ctx, EVP_aes_128_cbc(), NULL, key, iv) != 1)
        handleErrors();

    /* Provide the message to be decrypted, and obtain the plaintext output.
    * EVP_DecryptUpdate can be called multiple times if necessary
    */
    if (EVP_DecryptUpdate(ctx, plaintext, &len, ciphertext, ciphertext_len) != 1)
        handleErrors();
    plaintext_len = len;

    /* Finalise the decryption. Further plaintext bytes may be written at
    * this stage.
    */
    if(1 != EVP_DecryptFinal_ex(ctx, plaintext + len, &len)) handleErrors();
    plaintext_len += len;

    /* Clean up */
    EVP_CIPHER_CTX_free(ctx);

    return plaintext_len;
}

#ifdef TEST1
void test1(char *plaintext)
{
    /* A 256 bit key */
    unsigned char *key = (unsigned char *)"0123456789012345";
    BIO_dump_fp (stdout, (const char*)key, 16);

    /* A 128 bit IV */
    unsigned char *iv = (unsigned char *)"0123456789012345";

    /* Message to be encrypted */
    

    /* Buffer for ciphertext. Ensure the buffer is long enough for the
    * ciphertext which may be longer than the plaintext, dependant on the
    * algorithm and mode
    */
    unsigned char ciphertext[128];

    /* Buffer for the decrypted text */
    unsigned char decryptedtext[128];

    int decryptedtext_len, ciphertext_len;

    /* Encrypt the plaintext */
    ciphertext_len = encrypt ((cipher_t*)plaintext, strlen ((char *)plaintext), key, iv,
                                ciphertext);

    /* Do something useful with the ciphertext here */
    printf("Ciphertext is:\n");
    BIO_dump_fp (stdout, (const char *)ciphertext, ciphertext_len);
    unsigned char ciphertext_base64[1024];
    int ciphertext_base64_len;
    ciphertext_base64_len = my_encrypt_base64(plaintext, strlen ((char *)plaintext), key, iv,
                                ciphertext_base64);
    printf("MyCiphertext is:\n");
    printf("len: %d, '%*s'\n", ciphertext_base64_len, ciphertext_base64_len, ciphertext_base64);

    /* Decrypt the ciphertext */
    // decryptedtext_len = decrypt(ciphertext, ciphertext_len, key, iv,
    //     decryptedtext);

    // /* Add a NULL terminator. We are expecting printable text */
    // decryptedtext[decryptedtext_len] = '\0';

    // /* Show the decrypted text */
    // printf("Decrypted text is:\n");
    // printf("'%s'\n", decryptedtext);

    decryptedtext_len = my_decrypt_base64(ciphertext_base64, ciphertext_base64_len, key, iv, (char*)decryptedtext);
    BIO_dump_fp (stdout, (const char *)decryptedtext, decryptedtext_len);
    decryptedtext[decryptedtext_len] = '\0';
    printf("MyDecrypted text is:\n");
    printf("len: %d, '%s'\n", decryptedtext_len, decryptedtext);
}
#endif

#ifdef TEST2
void test2(int argc, char *argv[])
{
    cipher_t key[16];
    cipher_t *iv = (cipher_t*)"0123456789012345";
    if (argc < 2)
    {
        fprintf(stderr, "Usage: %s <key>\n", argv[0]);
        exit(1);
    }
    strncpy((char*)key, argv[argc-1], 16);
    memset(key + strlen((char*)key), 0, 16 - strlen((char*)key));
    argv[argc-2] = argv[argc-1];
    argv[argc-1] = NULL;
    argc--;
    BIO_dump_fp (stdout, (const char*)key, 16);
    char *plaintext;
    cipher_t ciphertext_base64[2048];
    char decryptedtext[2048];
    size_t size = 0;
    int ret;
    while ((ret = getline(&plaintext, &size, stdin)) != -1)
    {
        plaintext[ret - 1] = '\0';
        int ciphertext_base64_len;
        ciphertext_base64_len = my_encrypt_base64(plaintext, strlen(plaintext), key, iv, ciphertext_base64);
        printf("Ciphertext is:\n");
        printf("'%*s'\n", ciphertext_base64_len, ciphertext_base64);


        int decryptedtext_len = my_decrypt_base64(ciphertext_base64, ciphertext_base64_len, key, iv, decryptedtext);
        // decryptedtext[decryptedtext_len] = '\0';
        printf("MyDecrypted text is:\n");
        printf("'%s'\n", decryptedtext);
    }
}
#endif
int main (int argc, char *argv[])
{
#ifdef TEST1
    char *plaintext = (char *)"The quick brown fox jumps over the lazy dog";
    test1(plaintext);
    plaintext = (char *)"asd";
    test1(plaintext);
    plaintext = (char *)"Why the fuck";
    test1(plaintext);
#endif
#ifdef TEST2
    test2(argc, argv);
#endif
}
