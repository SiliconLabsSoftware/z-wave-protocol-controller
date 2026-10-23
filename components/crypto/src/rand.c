#include "rand.h"
#include "helpers.h"

#include "psa/crypto.h"

sl_status_t crypto_generate_random(uint8_t *output, size_t output_size)
{
    psa_status_t status = psa_crypto_init();
    if (status != PSA_SUCCESS) {
        return psa_status_to_sl_status(status);
    }

    return psa_status_to_sl_status(psa_generate_random(output, output_size));
}
