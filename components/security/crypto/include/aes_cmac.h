#ifndef AES_CMAC_H_
#define AES_CMAC_H_

#include <stdint.h>

int aes_cmac_calculate(const uint8_t *key, const uint8_t *message, const uint16_t message_length, uint8_t *mac);

#endif
