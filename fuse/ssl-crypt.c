#include "ssl-crypt.h"

#include <openssl/buffer.h>
#include <openssl/conf.h>
#include <openssl/evp.h>
#include <openssl/bio.h>
#include <openssl/err.h>
#include <string.h>

static void handleErrors(void)
{
    ERR_print_errors_fp(stderr);
    abort();
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