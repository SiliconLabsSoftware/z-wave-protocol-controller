#include "c_api_backend.hpp"

#include "crypto.hpp"
#include "psa_wrappers.hpp"

namespace crypto
{
    namespace
    {
        Crypto &default_crypto()
        {
            static PsaWrappers psa_wrappers;
            static Crypto instance(psa_wrappers);
            return instance;
        }
    }  // namespace

    CryptoInterface &c_api()
    {
        if (CryptoInterface * override = detail::c_api_override()) {
            return *override;
        }
        return default_crypto();
    }
}  // namespace crypto
