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

int my_encrypt_base64(unsigned char *plaintext, int plaintext_len, unsigned char *key,
  unsigned char *iv, unsigned char *ciphertext)
{
    BIO *encrypter;
    if ((encrypter = BIO_new(BIO_f_cipher())) == NULL)
        handleErrors();
    BIO_set_cipher(encrypter, EVP_aes_128_cbc(), key, iv, 1); //1 for encrypt
    // BIO *output1;
    // if ((output1 = BIO_new_fp(stdout, BIO_NOCLOSE)) == NULL)
    //     handleErrors();
    // BIO_push(encrypter, output1);
    BIO *base64;
    if ((base64 = BIO_new(BIO_f_base64())) == NULL)
        handleErrors();
    BIO_set_flags(base64, BIO_FLAGS_BASE64_NO_NL);
    BIO_push(encrypter, base64);
    // BIO *output;
    // if ((output = BIO_new_fp(stdout, BIO_NOCLOSE)) == NULL)
    //     handleErrors();
    // BIO_push(encrypter, output);
    BIO *mem;
    if ((mem = BIO_new(BIO_s_mem())) == NULL)
        handleErrors();
    BIO_push(encrypter, mem);

    BIO_write(encrypter, plaintext, plaintext_len);
    BIO_flush(encrypter);
    char *data;
    long ciphertext_len = BIO_get_mem_data(mem, &data);
    //printf("%ld read: %*s\n",ciphertext_len, ciphertext_len, data);
    memcpy(ciphertext, data, ciphertext_len);
    ciphertext[ciphertext_len] = '\0';
    BIO_free_all(encrypter);

    return ciphertext_len;
}

int my_decrypt_base64(unsigned char *ciphertext, int ciphertext_len, unsigned char *key,
    unsigned char *iv, unsigned char *plaintext)
{
    BIO *encrypter;
    if ((encrypter = BIO_new(BIO_f_cipher())) == NULL)
        handleErrors();
    BIO_set_cipher(encrypter, EVP_aes_128_cbc(), key, iv, 0); //0 for decrypt
    BIO *base64;
    if ((base64 = BIO_new(BIO_f_base64())) == NULL)
        handleErrors();
    BIO_set_flags(base64, BIO_FLAGS_BASE64_NO_NL);
    BIO_push(encrypter, base64);
    BIO *memm = BIO_new_mem_buf(ciphertext, ciphertext_len);
    if ((memm = BIO_new(BIO_s_mem())) == NULL)
        handleErrors();
    BIO *mem;
    if ((mem = BIO_new(BIO_s_mem())) == NULL)
        handleErrors();
    BIO_push(encrypter, mem);
    int plaintext_len = BIO_read(mem, plaintext, 128);
    BIO_free_all(encrypter);

    return plaintext_len;
}

int main (void)
{
    /* Set up the key and iv. Do I need to say to not hard code these in a
    * real application? :-)
    */

    /* A 256 bit key */
    unsigned char *key = (unsigned char *)"0123456789012345";

    /* A 128 bit IV */
    unsigned char *iv = (unsigned char *)"0123456789012345";

    /* Message to be encrypted */
    unsigned char *plaintext =
                    (unsigned char *)"The quick brown fox jumps over the lazy dog";

    /* Buffer for ciphertext. Ensure the buffer is long enough for the
    * ciphertext which may be longer than the plaintext, dependant on the
    * algorithm and mode
    */
    unsigned char ciphertext[128];

    /* Buffer for the decrypted text */
    unsigned char decryptedtext[128];

    int decryptedtext_len, ciphertext_len;

    /* Encrypt the plaintext */
    ciphertext_len = encrypt (plaintext, strlen ((char *)plaintext), key, iv,
                                ciphertext);

    /* Do something useful with the ciphertext here */
    printf("Ciphertext is:\n");
    BIO_dump_fp (stdout, (const char *)ciphertext, ciphertext_len);
    printf("My cipher:\n");
    unsigned char ciphertext_base64[128];
    int ciphertext_base64_len;
    ciphertext_base64_len = my_encrypt_base64(plaintext, strlen ((char *)plaintext), key, iv,
                                ciphertext_base64);
    printf("Ciphertext is:\n");
    printf("%*s\n", ciphertext_base64_len, ciphertext_base64);

    /* Decrypt the ciphertext */
    decryptedtext_len = decrypt(ciphertext, ciphertext_len, key, iv,
        decryptedtext);

    /* Add a NULL terminator. We are expecting printable text */
    decryptedtext[decryptedtext_len] = '\0';

    /* Show the decrypted text */
    printf("Decrypted text is:\n");
    printf("'%s'\n", decryptedtext);

    decryptedtext_len = my_decrypt_base64(ciphertext_base64, ciphertext_base64_len, key, iv, decryptedtext);
    decryptedtext[decryptedtext_len] = '\0';
    printf("MyDecrypted text is:\n");
    printf("'%s'\n", decryptedtext);


    return 0;
}