#ifndef AES_OFB_H_
#define AES_OFB_H_

#include <stddef.h>
#include <stdint.h>

#include "sl_status.h"

sl_status_t crypto_aes_ofb_encrypt(const uint8_t *key, const uint8_t *iv, uint8_t *data, size_t data_len);

#endif
