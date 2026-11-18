#ifndef ZWAVE_S2_NEXT_NONCE_H
#define ZWAVE_S2_NEXT_NONCE_H

#include "crypto.h"
#include "sl_status.h"

/**
 * @brief Initialize the S2 nonce generator from sender and receiver entropy.
 *
 * @param[out] ctx Context initialized on success; must not be NULL and is unchanged on failure.
 * @param entropy_input_sender 16-byte sender entropy input; must not be NULL.
 * @param entropy_input_receiver 16-byte receiver entropy input; must not be NULL.
 * @param nonce_key 32-byte nonce_pers_string from S2 key derivation; must not be NULL.
 * @return SL_STATUS_OK on success, SL_STATUS_NULL_POINTER for a null argument, otherwise a crypto error status.
 */
sl_status_t zwave_s2_next_nonce_instantiate(crypto_aes_ctr_drbg_ctx_t *ctx, const uint8_t *entropy_input_sender, const uint8_t *entropy_input_receiver, const uint8_t *nonce_key);

#endif
