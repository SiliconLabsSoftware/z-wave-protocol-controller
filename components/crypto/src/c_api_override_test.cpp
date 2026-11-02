#include "c_api_backend.hpp"

namespace crypto
{
    namespace
    {
        CryptoInterface *backend_override = nullptr;
    }  // namespace

    void set_c_api_backend(CryptoInterface &backend)
    {
        backend_override = &backend;
    }

    void reset_c_api_backend()
    {
        backend_override = nullptr;
    }

    namespace detail
    {
        CryptoInterface *c_api_override()
        {
            return backend_override;
        }
    }  // namespace detail
}  // namespace crypto
