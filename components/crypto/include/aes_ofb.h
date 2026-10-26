#ifndef AES_OFB_H_
#define AES_OFB_H_

#include <stddef.h>
#include <stdint.h>

#include "aes.h"
#include "sl_status.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * AES-128 OFB keystream XOR (NIST SP 800-38A OFB) via PSA_ALG_OFB.
 *
 * Uses \p iv as the OFB initial vector for this call and XORs keystream into \p data.
 */
sl_status_t crypto_aes_ofb_xor(const uint8_t *key, uint8_t iv[16], uint8_t *data, size_t len);

#ifdef __cplusplus
}
#endif

#endif
