#include "crypt.h"

#include "tiny-AES-c/aes.h"


int do_crypt(char* buffer, size_t buffer_len, char key[16])
{
    struct AES_ctx ctx;
    uint8_t iv[16]  = { 0xf0, 0xf1, 0xf2, 0xf3, 0xf4, 0xf5, 0xf6, 0xf7, 0xf8, 0xf9, 0xfa, 0xfb, 0xfc, 0xfd, 0xfe, 0xff };
    AES_init_ctx_iv(&ctx, key, iv);

    AES_CTR_xcrypt_buffer(&ctx, buffer, buffer_len);
    return 0;
}