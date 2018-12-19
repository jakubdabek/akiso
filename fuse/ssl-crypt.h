#pragma once

typedef unsigned char cipher_t;

int my_encrypt_base64(const char *plaintext, int plaintext_len, cipher_t *key,
    cipher_t *iv, cipher_t *ciphertext);

int my_decrypt_base64(const cipher_t *ciphertext, int ciphertext_len, cipher_t *key,
    cipher_t *iv, char *plaintext);