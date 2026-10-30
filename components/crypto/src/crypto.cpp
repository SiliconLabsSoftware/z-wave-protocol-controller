#include "crypto.hpp"

#include "psa_wrappers.hpp"

namespace crypto
{
    Crypto &Crypto::instance()
    {
        static PsaWrappers psa_wrappers;
        static Crypto instance(psa_wrappers);
        return instance;
    }
}  // namespace crypto
