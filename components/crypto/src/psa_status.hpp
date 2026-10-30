#ifndef CRYPTO_PSA_STATUS_HPP
#define CRYPTO_PSA_STATUS_HPP

#include "psa/crypto.h"
#include "sl_status.h"

namespace crypto
{
    sl_status_t psa_status_to_sl_status(psa_status_t status) noexcept;
}  // namespace crypto

#endif
