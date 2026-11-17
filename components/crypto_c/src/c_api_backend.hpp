#ifndef C_API_BACKEND_HPP
#define C_API_BACKEND_HPP

#include "crypto.hpp"

namespace crypto_c
{
    crypto::CryptoInterface &backend();
    crypto::CryptoInterface *backend_override();
}  // namespace crypto_c

#endif
