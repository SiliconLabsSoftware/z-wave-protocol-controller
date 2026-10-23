#ifndef AES_ECB_H_
#define AES_ECB_H_

#include <stdint.h>

#include "sl_status.h"

sl_status_t crypto_aes_ecb_encrypt(const uint8_t *key, const uint8_t *input, uint8_t *output);

#endif
