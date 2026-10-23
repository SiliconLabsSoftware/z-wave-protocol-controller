#ifndef ZWAVE_S2_KEY_DERIVATION_H
#define ZWAVE_S2_KEY_DERIVATION_H

#include <stdint.h>

#include "sl_status.h"

sl_status_t zwave_s2_extract_pseudo_random_key(const uint8_t *ecdh_shared_secret, const uint8_t *public_keys, uint8_t *pseudo_random_key);

sl_status_t zwave_s2_derive_temp_key(const uint8_t *pseudo_random_key, uint8_t *ccm_key, uint8_t *nonce_pers_string, uint8_t *mpan_key);

sl_status_t zwave_s2_derive_network_key(const uint8_t *network_key, uint8_t *ccm_key, uint8_t *nonce_pers_string, uint8_t *mpan_key);

#endif
