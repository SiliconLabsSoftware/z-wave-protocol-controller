#include "aes_ofb.h"
#include "aes_ecb.h"

sl_status_t crypto_aes_ofb_xor(const uint8_t *key, uint8_t iv[16], uint8_t *data, size_t len)
{
    for (size_t i = 0; i < len; i++) {
        if ((i & 0xFU) == 0U) {
            if (crypto_aes_ecb_encrypt(key, iv, iv) != SL_STATUS_OK) {
                return SL_STATUS_FAIL;
            }
        }
        data[i] ^= iv[i & 0xFU];
    }
    return SL_STATUS_OK;
}
