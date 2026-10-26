#ifndef AES_CMAC_H_
#define AES_CMAC_H_

#include <stddef.h>
#include <stdint.h>

#include "aes.h"
#include "sl_status.h"

#ifdef __cplusplus
extern "C" {
#endif

#define CRYPTO_AES_CMAC_OUTPUT_LEN CRYPTO_AES_BLOCK_SIZE

sl_status_t crypto_aes_cmac_compute(const uint8_t *key, const uint8_t *message, size_t message_length, uint8_t *mac);

#ifdef __cplusplus
}
#endif

#endif
