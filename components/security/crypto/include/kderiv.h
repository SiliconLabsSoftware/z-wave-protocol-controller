#ifndef KDERIV_H
#define KDERIV_H

#include <stdint.h>

extern void networkkey_expand(uint32_t key_id, const uint8_t *network_key, uint8_t *ccm_key, uint8_t *nonce_pstring, uint8_t *mpan_key);
extern void tempkey_extract(const uint8_t *const ecdh_share_secret, const uint8_t *const auth_tag, uint8_t *pseudo_random_keymat_output);
extern void tempkey_expand(uint32_t key_id, const uint8_t *prk, uint8_t *temp_ccm_key, uint8_t *temp_nonce_pstring, uint8_t *temp_mpan_key);

#endif
