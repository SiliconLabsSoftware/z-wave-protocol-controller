#ifndef PSA_HELPERS_HPP
#define PSA_HELPERS_HPP

#include "psa/crypto.h"
#include "sl_status.h"

namespace crypto
{
    /**
     * @brief Convert a PSA status to an sl_status_t.
     *
     * @param status PSA status.
     * @return SL_STATUS_OK for success, a mapped error status when available, otherwise SL_STATUS_FAIL.
     */
    sl_status_t psa_status_to_sl_status(psa_status_t status) noexcept;
}  // namespace crypto

#endif
