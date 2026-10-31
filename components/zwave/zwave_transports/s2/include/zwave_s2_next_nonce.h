#ifndef ZWAVE_S2_NEXT_NONCE_H
#define ZWAVE_S2_NEXT_NONCE_H

#include "crypto.h"
#include "sl_status.h"

sl_status_t zwave_s2_next_nonce_instantiate(crypto_aes_ctr_drbg_ctx_t *ctx, const uint8_t *entropy_input_sender, const uint8_t *entropy_input_receiver, const uint8_t *nonce_key);

#endif
