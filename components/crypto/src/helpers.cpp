#include "helpers.hpp"

#include "aes.h"

namespace crypto::helpers
{

    psa_status_t import_aes_key(psa_key_usage_t usage, psa_algorithm_t algorithm, const uint8_t *key, psa_key_id_t *key_id)
    {
        psa_key_attributes_t attributes = PSA_KEY_ATTRIBUTES_INIT;

        psa_set_key_usage_flags(&attributes, usage);
        psa_set_key_algorithm(&attributes, algorithm);
        psa_set_key_type(&attributes, PSA_KEY_TYPE_AES);
        psa_set_key_bits(&attributes, CRYPTO_AES_KEY_BITS);

        return psa_import_key(&attributes, key, CRYPTO_AES_KEY_SIZE, key_id);
    }

    sl_status_t psa_status_to_sl_status(psa_status_t status)
    {
        switch (status) {
            case PSA_SUCCESS:
                return SL_STATUS_OK;
            case PSA_ERROR_INVALID_SIGNATURE:
                return SL_STATUS_INVALID_SIGNATURE;
            case PSA_ERROR_INVALID_ARGUMENT:
                return SL_STATUS_INVALID_PARAMETER;
            case PSA_ERROR_NOT_SUPPORTED:
                return SL_STATUS_NOT_SUPPORTED;
            case PSA_ERROR_INSUFFICIENT_MEMORY:
                return SL_STATUS_ALLOCATION_FAILED;
            case PSA_ERROR_BAD_STATE:
                return SL_STATUS_INVALID_STATE;
            default:
                return SL_STATUS_FAIL;
        }
    }

}  // namespace crypto::helpers
