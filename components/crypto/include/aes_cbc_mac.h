#ifndef AES_CBC_MAC_H_
#define AES_CBC_MAC_H_

#include <stdint.h>

#include "aes.h"
#include "sl_status.h"

#ifdef __cplusplus
extern "C" {
#endif

sl_status_t crypto_aes_cbc_mac(const uint8_t *key, const uint8_t *iv, const uint8_t *data, uint8_t len, uint8_t *mac);

#ifdef __cplusplus
}
#endif

#endif
