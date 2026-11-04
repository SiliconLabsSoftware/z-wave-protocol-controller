#include "c_api_backend.hpp"

namespace crypto_c::detail
{
    crypto::CryptoInterface &backend()
    {
        if (auto *override = backend_override()) {
            return *override;
        }
        return crypto::default_crypto();
    }
}  // namespace crypto_c::detail
