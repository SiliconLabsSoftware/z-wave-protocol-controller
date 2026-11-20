#include "crypto.hpp"

#include "crypto_impl.hpp"
#include "psa/psa_wrappers_impl.hpp"

namespace crypto
{
    CryptoInterface &default_crypto()
    {
        static PsaWrappers psa_wrappers;
        static Crypto instance(psa_wrappers);
        return instance;
    }
}  // namespace crypto
