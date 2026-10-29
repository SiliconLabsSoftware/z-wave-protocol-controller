#ifndef HELPERS_HPP
#define HELPERS_HPP

#include <stdint.h>

#include "psa/crypto.h"
#include "sl_status.h"

namespace crypto
{

    class HelpersInterface
    {
        public:
            virtual ~HelpersInterface() = default;

            virtual sl_status_t psa_status_to_sl_status(psa_status_t status)                                                                = 0;
            virtual psa_status_t import_aes_key(psa_key_usage_t usage, psa_algorithm_t algorithm, const uint8_t *key, psa_key_id_t *key_id) = 0;
    };

    class Helpers final : public HelpersInterface
    {
        public:
            sl_status_t psa_status_to_sl_status(psa_status_t status) override;
            psa_status_t import_aes_key(psa_key_usage_t usage, psa_algorithm_t algorithm, const uint8_t *key, psa_key_id_t *key_id) override;
    };

}  // namespace crypto

#endif
