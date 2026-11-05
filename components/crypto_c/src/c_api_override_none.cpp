#include "c_api_backend.hpp"

namespace crypto_c::detail
{
    crypto::CryptoInterface *backend_override()
    {
        return nullptr;
    }
}  // namespace crypto_c::detail
