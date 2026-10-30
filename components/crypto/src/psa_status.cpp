#include "psa_status.hpp"

namespace crypto
{

    sl_status_t psa_status_to_sl_status(psa_status_t status) noexcept
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

}  // namespace crypto
