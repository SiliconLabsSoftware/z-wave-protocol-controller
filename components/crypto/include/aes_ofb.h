#ifndef AES_OFB_H_
#define AES_OFB_H_

#include <stddef.h>
#include <stdint.h>

#include "sl_status.h"

/**
 * AES-128 OFB keystream XOR (NIST SP 800-38A OFB).
 *
 * Updates \p iv in place (OFB state) and XORs keystream into \p data.
 */
sl_status_t crypto_aes_ofb_xor(const uint8_t *key, uint8_t iv[16], uint8_t *data, size_t len);

#endif
