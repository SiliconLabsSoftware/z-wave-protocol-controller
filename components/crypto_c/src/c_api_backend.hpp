#ifndef C_API_BACKEND_HPP
#define C_API_BACKEND_HPP

#include "crypto.hpp"

namespace crypto_c::detail
{
    crypto::CryptoInterface &backend();
    crypto::CryptoInterface *backend_override();
}  // namespace crypto_c::detail

#endif
