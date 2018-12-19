#pragma once

#include "ssl-crypt.h"

#include <stdio.h>
#include <limits.h>

ssize_t get_real_path(const char * const path, cipher_t out_buff[PATH_MAX], cipher_t key[16], cipher_t iv[16]);