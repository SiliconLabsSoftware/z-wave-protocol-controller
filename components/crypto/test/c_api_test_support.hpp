#ifndef C_API_TEST_SUPPORT_HPP_
#define C_API_TEST_SUPPORT_HPP_

#include "crypto_interface.hpp"

namespace crypto
{
    void set_c_api_backend(CryptoInterface &backend);
    void reset_c_api_backend();
}  // namespace crypto

#endif
