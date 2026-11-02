#ifndef C_API_BACKEND_HPP
#define C_API_BACKEND_HPP

#include "crypto_interface.hpp"

namespace crypto
{
    namespace detail
    {
        CryptoInterface *c_api_override();
    }
}  // namespace crypto

#endif
