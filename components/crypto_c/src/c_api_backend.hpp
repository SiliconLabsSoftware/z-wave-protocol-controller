#ifndef C_API_BACKEND_HPP
#define C_API_BACKEND_HPP

#include "crypto.hpp"

namespace crypto_c
{
    /**
     * @brief Get the backend used by C API calls: the override if set, otherwise default_crypto().
     * @return Reference to the selected backend.
     */
    crypto::CryptoInterface &backend();

    /**
     * @brief Get a backend to use instead of default_crypto().
     *
     * The default build returns nullptr; tests link their own version.
     *
     * @return Backend pointer (not owned), or nullptr to use default_crypto().
     */
    crypto::CryptoInterface *backend_override();
}  // namespace crypto_c

#endif
