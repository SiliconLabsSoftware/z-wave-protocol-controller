#ifndef AES_CMAC_H_
#define AES_CMAC_H_

#include <stdint.h>

/**
 * \ingroup crypto
 * @{
 */

/**
 * Calculate an AES-CMAC.
 * \param[in] key The 128-bit key.
 * \param[in] message The message to authenticate.
 * \param[in] message_length The message length in octets.
 * \param[out] mac The 16-byte output buffer.
 * \return 1 on success, 0 on failure.
 */
int aes_cmac_calculate(const uint8_t *key, const uint8_t *message, const uint16_t message_length, uint8_t *mac);

/**
 * @}
 */

#endif
