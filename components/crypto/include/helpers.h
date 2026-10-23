#ifndef HELPERS_H_
#define HELPERS_H_

#include <stdint.h>

#include "psa/crypto.h"
#include "sl_status.h"

sl_status_t psa_status_to_sl_status(psa_status_t status);
psa_status_t import_aes_key(psa_key_usage_t usage, psa_algorithm_t algorithm, const uint8_t *key, psa_key_id_t *key_id);

#endif
