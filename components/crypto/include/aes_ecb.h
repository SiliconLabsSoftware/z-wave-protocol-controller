#ifndef AES_ECB_H_
#define AES_ECB_H_

#include <stdint.h>

#include "aes.h"
#include "sl_status.h"

#ifdef __cplusplus
extern "C" {
#endif

sl_status_t crypto_aes_ecb_encrypt(const uint8_t *key, const uint8_t *input, uint8_t *output);

#ifdef UNIT_TEST
sl_status_t crypto_aes_ecb_decrypt(const uint8_t *key, const uint8_t *input, uint8_t *output);
#endif

#ifdef __cplusplus
}
#endif

#endif
