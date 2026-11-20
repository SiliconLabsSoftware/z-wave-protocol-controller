#include "c_api_backend.hpp"

namespace crypto_c
{
    crypto::CryptoInterface *backend_override()
    {
        return nullptr;
    }
}  // namespace crypto_c
