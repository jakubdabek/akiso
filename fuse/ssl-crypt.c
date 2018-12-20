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

static char* strn_replace(char *str, size_t n, char original, char replacement)
{
    char *ptr = str;
    if (str == NULL)
        return NULL;
    size_t counter = 0;
    while (*str != '\0' && counter++ < n)
    {
        if (*str == original)
            *str = replacement;
        str++;
    }

    return ptr;
}

int my_encrypt_base64(const char *plaintext, int plaintext_len, cipher_t *key,
  cipher_t *iv, cipher_t *ciphertext)
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
    strn_replace((char*)ciphertext, ciphertext_len, '/', '^');
    BIO_free_all(encrypter);

    return ciphertext_len;
}

static int calcDecodeLength(const char* b64input) { //Calculates the length of a decoded base64 string
  int len = strlen(b64input);
  int padding = 0;

  if (b64input[len-1] == '=' && b64input[len-2] == '=') //last two chars are =
    padding = 2;
  else if (b64input[len-1] == '=') //last char is =
    padding = 1;

  return (int)len*0.75 - padding;
}

int my_decrypt_base64(const cipher_t *ciphertext, int ciphertext_len, cipher_t *key,
    cipher_t *iv, char *plaintext)
{
    BIO *main_bio;
    BIO *encrypter;
    if ((encrypter = BIO_new(BIO_f_cipher())) == NULL)
        handleErrors();
    BIO_set_cipher(encrypter, EVP_aes_128_cbc(), key, iv, 0); //0 for decrypt
    BIO *base64;
    if ((base64 = BIO_new(BIO_f_base64())) == NULL)
        handleErrors();
    BIO_set_flags(base64, BIO_FLAGS_BASE64_NO_NL);
    // main_bio = BIO_push(encrypter, base64);
    strn_replace((char*)ciphertext, ciphertext_len, '^', '/');
    //printf("'%s', %d, %ld\n", (const char*)ciphertext, ciphertext_len, strlen((const char*)ciphertext));
    BIO *memm = BIO_new_mem_buf(ciphertext, ciphertext_len);
    if (memm == NULL)
        handleErrors();
    BIO_push(base64, memm);
    BIO_push(encrypter, base64);
    //int size=calcDecodeLength((char*)ciphertext);
    int plaintext_len = BIO_read(encrypter, plaintext, 2048);
    plaintext[plaintext_len] = '\0';
    BIO_free_all(memm);

    return plaintext_len;
}