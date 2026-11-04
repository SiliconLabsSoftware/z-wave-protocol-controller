#include "c_api_test_support.hpp"
#include "c_api_backend.hpp"

namespace
{
    crypto::CryptoInterface *override_backend = nullptr;
}

namespace crypto_c::test
{
    void set_backend(crypto::CryptoInterface &backend)
    {
        override_backend = &backend;
    }

    void reset_backend()
    {
        override_backend = nullptr;
    }
}  // namespace crypto_c::test

namespace crypto_c::detail
{
    crypto::CryptoInterface *backend_override()
    {
        return override_backend;
    }
}  // namespace crypto_c::detail
