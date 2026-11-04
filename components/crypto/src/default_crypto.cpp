#include "crypto.hpp"

#include "algorithms/crypto_impl.hpp"
#include "psa/psa_wrappers.hpp"

namespace crypto
{
    CryptoInterface &default_crypto()
    {
        static PsaWrappers psa_wrappers;
        static Crypto instance(psa_wrappers);
        return instance;
    }
}  // namespace crypto
