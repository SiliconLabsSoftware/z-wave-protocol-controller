#include "aes_cbc_mac.h"
#include "aes_ecb.h"

sl_status_t crypto_aes_cbc_mac(const uint8_t *key, const uint8_t *iv, const uint8_t *data, uint8_t len, uint8_t *mac)
{
    if (crypto_aes_ecb_encrypt(key, iv, mac) != SL_STATUS_OK) {
        return SL_STATUS_FAIL;
    }
    for (uint8_t i = 0; i < len; i++) {
        mac[i & 0xF] ^= data[i];
        if ((i & 0xF) == 0xF) {
            if (crypto_aes_ecb_encrypt(key, mac, mac) != SL_STATUS_OK) {
                return SL_STATUS_FAIL;
            }
        }
    }

    /* if len is not divisible by 16 do the final pass, this is the padding
     * described in the spec */
    if (len & 0xF) {
        if (crypto_aes_ecb_encrypt(key, mac, mac) != SL_STATUS_OK) {
            return SL_STATUS_FAIL;
        }
    }
    return SL_STATUS_OK;
}
