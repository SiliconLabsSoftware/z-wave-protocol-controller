#include "rand.hpp"

#include "psa/crypto.h"
#include "psa_status.hpp"
#include "psa_wrappers_interface.hpp"

namespace crypto
{

    sl_status_t Crypto::generate_random(std::span<uint8_t> output) const
    {
        if (!output.empty() && output.data() == nullptr) {
            return SL_STATUS_INVALID_PARAMETER;
        }

        psa_status_t status = psa_wrappers_.crypto_init();
        if (status != PSA_SUCCESS) {
            return psa_status_to_sl_status(status);
        }

        return psa_status_to_sl_status(psa_wrappers_.generate_random(output));
    }

    sl_status_t generate_random(std::span<uint8_t> output)
    {
        return Crypto::instance().generate_random(output);
    }

}  // namespace crypto

extern "C" {

sl_status_t crypto_generate_random(uint8_t *output, size_t output_size)
{
    return crypto::generate_random(std::span<uint8_t>(output, output_size));
}

}  // extern "C"
