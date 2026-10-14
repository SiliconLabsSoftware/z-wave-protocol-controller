#include "aes_cmac.h"
#include "s2_psa_primitives.h"

int aes_cmac_calculate(const uint8_t *key, const uint8_t *message, const uint16_t message_length, uint8_t *mac)
{
    return s2_psa_aes_cmac(key, message, message_length, mac) == PSA_SUCCESS;
}
