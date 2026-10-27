#include "rand.hpp"
#include "helpers.hpp"

#include "psa/crypto.h"

namespace crypto
{

    sl_status_t generate_random(std::span<uint8_t> output)
    {
        if (output.size() != 0 && output.data() == nullptr) {
            return SL_STATUS_INVALID_PARAMETER;
        }

        psa_status_t status = psa_crypto_init();
        if (status != PSA_SUCCESS) {
            return crypto::helpers::psa_status_to_sl_status(status);
        }

        return crypto::helpers::psa_status_to_sl_status(psa_generate_random(output.data(), output.size()));
    }

}  // namespace crypto

extern "C" {

sl_status_t crypto_generate_random(uint8_t *output, size_t output_size)
{
    return crypto::generate_random(std::span<uint8_t>(output, output_size));
}

}  // extern "C"
