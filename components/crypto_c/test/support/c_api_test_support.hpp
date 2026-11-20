#ifndef C_API_TEST_SUPPORT_HPP_
#define C_API_TEST_SUPPORT_HPP_

#include "crypto.hpp"

namespace crypto_c::test
{
    void set_backend(crypto::CryptoInterface &backend);
    void reset_backend();
}  // namespace crypto_c::test

#endif
