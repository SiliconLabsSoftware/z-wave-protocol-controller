#include "c_api_backend.hpp"

namespace crypto::detail
{
    CryptoInterface *c_api_override()
    {
        return nullptr;
    }
}  // namespace crypto::detail
