#ifndef CRYPTO_MOCK_HELPERS_HPP
#define CRYPTO_MOCK_HELPERS_HPP

#include <gmock/gmock.h>

#include "helpers.hpp"

namespace crypto::test
{
    class MockHelpers : public HelpersInterface
    {
        public:
            MOCK_METHOD(sl_status_t, psa_status_to_sl_status, (psa_status_t status), (override));
            MOCK_METHOD(psa_status_t, import_aes_key, (psa_key_usage_t usage, psa_algorithm_t algorithm, const uint8_t *key, psa_key_id_t *key_id), (override));
    };
}  // namespace crypto::test

#endif
